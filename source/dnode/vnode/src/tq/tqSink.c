/*
 * Copyright (c) 2019 TAOS Data, Inc. <jhtao@taosdata.com>
 *
 * This program is free software: you can use, redistribute, and/or modify
 * it under the terms of the GNU Affero General Public License, version 3
 * or later ("AGPL"), as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "tcommon.h"
#include "tmsg.h"
#include "tq.h"

#define MAX_CACHE_TABLE_INFO_NUM 10240

typedef struct STableSinkInfo {
  uint64_t uid;
  tstr     name;
} STableSinkInfo;

static int32_t doSinkResultBlock(SVnode* pVnode, int32_t blockIndex, char* stbFullName, int64_t suid,
                                 SSDataBlock* pDataBlock, SStreamTask* pTask);
static int32_t doSinkDeleteBlock(SVnode* pVnode, char* stbFullName, SSDataBlock* pDataBlock, SStreamTask* pTask,
                                 int64_t suid);

int32_t tqBuildDeleteReq(const char* stbFullName, const SSDataBlock* pDataBlock, SBatchDeleteReq* deleteReq,
                         const char* pIdStr) {
  int32_t          totalRows = pDataBlock->info.rows;
  SColumnInfoData* pStartTsCol = taosArrayGet(pDataBlock->pDataBlock, START_TS_COLUMN_INDEX);
  SColumnInfoData* pEndTsCol = taosArrayGet(pDataBlock->pDataBlock, END_TS_COLUMN_INDEX);
  SColumnInfoData* pGidCol = taosArrayGet(pDataBlock->pDataBlock, GROUPID_COLUMN_INDEX);
  SColumnInfoData* pTbNameCol = taosArrayGet(pDataBlock->pDataBlock, TABLE_NAME_COLUMN_INDEX);

  tqDebug("s-task:%s build %d rows delete msg for table:%s", pIdStr, totalRows, stbFullName);

  for (int32_t row = 0; row < totalRows; row++) {
    int64_t skey = *(int64_t*)colDataGetData(pStartTsCol, row);
    int64_t ekey = *(int64_t*)colDataGetData(pEndTsCol, row);
    int64_t groupId = *(int64_t*)colDataGetData(pGidCol, row);

    char*   name;
    void*   varTbName = NULL;
    if (!colDataIsNull(pTbNameCol, totalRows, row, NULL)) {
      varTbName = colDataGetVarData(pTbNameCol, row);
    }

    if (varTbName != NULL && varTbName != (void*)-1) {
      name = taosMemoryCalloc(1, TSDB_TABLE_NAME_LEN);
      memcpy(name, varDataVal(varTbName), varDataLen(varTbName));
    } else {
      name = buildCtbNameByGroupId(stbFullName, groupId);
    }

    tqDebug("s-task:%s build delete msg groupId:%" PRId64 ", name:%s, skey:%" PRId64 " ekey:%" PRId64,
            pIdStr, groupId, name, skey, ekey);

    SSingleDeleteReq req = { .startTs = skey, .endTs = ekey};
    strncpy(req.tbname, name, TSDB_TABLE_NAME_LEN - 1);
    taosMemoryFree(name);

    taosArrayPush(deleteReq->deleteReqs, &req);
  }

  return 0;
}

static int32_t encodeCreateChildTableForRPC(SVCreateTbBatchReq* pReqs, int32_t vgId, void** pBuf, int32_t* contLen) {
  int32_t ret = 0;

  tEncodeSize(tEncodeSVCreateTbBatchReq, pReqs, *contLen, ret);
  if (ret < 0) {
    ret = -1;
    goto end;
  }
  *contLen += sizeof(SMsgHead);
  *pBuf = rpcMallocCont(*contLen);
  if (NULL == *pBuf) {
    ret = -1;
    goto end;
  }
  ((SMsgHead*)(*pBuf))->vgId = vgId;
  ((SMsgHead*)(*pBuf))->contLen = htonl(*contLen);
  SEncoder coder = {0};
  tEncoderInit(&coder, POINTER_SHIFT(*pBuf, sizeof(SMsgHead)), (*contLen) - sizeof(SMsgHead));
  if (tEncodeSVCreateTbBatchReq(&coder, pReqs) < 0) {
    rpcFreeCont(*pBuf);
    *pBuf = NULL;
    *contLen = 0;
    tEncoderClear(&coder);
    ret = -1;
    goto end;
  }
  tEncoderClear(&coder);

end:
  return ret;
}

static bool tqGetTableInfo(SSHashObj* pTableInfoMap,uint64_t groupId, STableSinkInfo** pInfo) {
  void* pVal = tSimpleHashGet(pTableInfoMap, &groupId, sizeof(uint64_t));
  if (pVal) {
    *pInfo = *(STableSinkInfo**)pVal;
    return true;
  }

  return false;
}

static int32_t tqPutTableInfo(SSHashObj* tblInfo ,uint64_t groupId, STableSinkInfo* pTbl) {
  if (tSimpleHashGetSize(tblInfo) > MAX_CACHE_TABLE_INFO_NUM) {
    return TSDB_CODE_FAILED;
  }

  return tSimpleHashPut(tblInfo, &groupId, sizeof(uint64_t), &pTbl, POINTER_BYTES);
}

static int32_t tqPutReqToQueue(SVnode* pVnode, SVCreateTbBatchReq* pReqs) {
  void*   buf = NULL;
  int32_t tlen = 0;
  encodeCreateChildTableForRPC(pReqs, TD_VID(pVnode), &buf, &tlen);

  SRpcMsg msg = { .msgType = TDMT_VND_CREATE_TABLE, .pCont = buf, .contLen = tlen };
  if (tmsgPutToQueue(&pVnode->msgCb, WRITE_QUEUE, &msg) != 0) {
    tqError("failed to put into write-queue since %s", terrstr());
  }

  return TSDB_CODE_SUCCESS;
}


void tqSinkToTablePipeline(SStreamTask* pTask, void* vnode, void* data) {
  const SArray* pBlocks = (const SArray*)data;
  SVnode*       pVnode = (SVnode*)vnode;
  int64_t       suid = pTask->tbSink.stbUid;
  char*         stbFullName = pTask->tbSink.stbFullName;
  STSchema*     pTSchema = pTask->tbSink.pTSchema;
  int32_t       vgId = TD_VID(pVnode);
  int32_t       numOfBlocks = taosArrayGetSize(pBlocks);
  int32_t       code = TSDB_CODE_SUCCESS;

  tqDebug("vgId:%d, s-task:%s write %d stream resBlock(s) into table", vgId, pTask->id.idStr, numOfBlocks);

  SArray* tagArray = NULL;
  SArray* pVals = NULL;
  SArray* crTblArray = NULL;

  for (int32_t i = 0; i < numOfBlocks; i++) {
    SSDataBlock* pDataBlock = taosArrayGet(pBlocks, i);
    int32_t      rows = pDataBlock->info.rows;

    if (pDataBlock->info.type == STREAM_DELETE_RESULT) {
      code = doSinkDeleteBlock(pVnode, stbFullName, pDataBlock, pTask, suid);
    } else if (pDataBlock->info.type == STREAM_CREATE_CHILD_TABLE) {
      tqDebug("s-task:%s build create table msg", pTask->id.idStr);

      SVCreateTbBatchReq reqs = {0};
      crTblArray = reqs.pArray = taosArrayInit(1, sizeof(SVCreateTbReq));
      if (NULL == reqs.pArray) {
        goto _end;
      }

      for (int32_t rowId = 0; rowId < rows; rowId++) {
        SVCreateTbReq* pCreateTbReq = &((SVCreateTbReq){0});

        // set const
        pCreateTbReq->flags = 0;
        pCreateTbReq->type = TSDB_CHILD_TABLE;
        pCreateTbReq->ctb.suid = suid;

        // set super table name
        SName name = {0};
        tNameFromString(&name, stbFullName, T_NAME_ACCT | T_NAME_DB | T_NAME_TABLE);
        pCreateTbReq->ctb.stbName = taosStrdup((char*)tNameGetTableName(&name));  // taosStrdup(stbFullName);

        // set tag content
        int32_t size = taosArrayGetSize(pDataBlock->pDataBlock);
        if (size == 2) {
          tagArray = taosArrayInit(1, sizeof(STagVal));

          if (!tagArray) {
            tdDestroySVCreateTbReq(pCreateTbReq);
            goto _end;
          }

          STagVal tagVal = {
              .cid = pTSchema->numOfCols + 1, .type = TSDB_DATA_TYPE_UBIGINT, .i64 = pDataBlock->info.id.groupId};

          taosArrayPush(tagArray, &tagVal);

          // set tag name
          SArray* tagName = taosArrayInit(1, TSDB_COL_NAME_LEN);
          char    tagNameStr[TSDB_COL_NAME_LEN] = "group_id";
          taosArrayPush(tagName, tagNameStr);
          pCreateTbReq->ctb.tagName = tagName;
        } else {
          tagArray = taosArrayInit(size - 1, sizeof(STagVal));
          if (!tagArray) {
            tdDestroySVCreateTbReq(pCreateTbReq);
            goto _end;
          }

          for (int32_t tagId = UD_TAG_COLUMN_INDEX, step = 1; tagId < size; tagId++, step++) {
            SColumnInfoData* pTagData = taosArrayGet(pDataBlock->pDataBlock, tagId);

            STagVal tagVal = {.cid = pTSchema->numOfCols + step, .type = pTagData->info.type};
            void*   pData = colDataGetData(pTagData, rowId);
            if (colDataIsNull_s(pTagData, rowId)) {
              continue;
            } else if (IS_VAR_DATA_TYPE(pTagData->info.type)) {
              tagVal.nData = varDataLen(pData);
              tagVal.pData = (uint8_t*) varDataVal(pData);
            } else {
              memcpy(&tagVal.i64, pData, pTagData->info.bytes);
            }
            taosArrayPush(tagArray, &tagVal);
          }

        }
        pCreateTbReq->ctb.tagNum = TMAX(size - UD_TAG_COLUMN_INDEX, 1);

        STag* pTag = NULL;
        tTagNew(tagArray, 1, false, &pTag);
        tagArray = taosArrayDestroy(tagArray);
        if (pTag == NULL) {
          tdDestroySVCreateTbReq(pCreateTbReq);
          terrno = TSDB_CODE_OUT_OF_MEMORY;
          goto _end;
        }

        pCreateTbReq->ctb.pTag = (uint8_t*)pTag;

        // set table name
        if (!pDataBlock->info.parTbName[0]) {
          SColumnInfoData* pGpIdColInfo = taosArrayGet(pDataBlock->pDataBlock, UD_GROUPID_COLUMN_INDEX);
          void*            pGpIdData = colDataGetData(pGpIdColInfo, rowId);
          pCreateTbReq->name = buildCtbNameByGroupId(stbFullName, *(uint64_t*)pGpIdData);
        } else {
          pCreateTbReq->name = taosStrdup(pDataBlock->info.parTbName);
        }

        taosArrayPush(reqs.pArray, pCreateTbReq);
        tqDebug("s-task:%s build create table:%s msg complete", pTask->id.idStr, pCreateTbReq->name);
      }

      reqs.nReqs = taosArrayGetSize(reqs.pArray);
      if (tqPutReqToQueue(pVnode, &reqs) != TSDB_CODE_SUCCESS) {
        goto _end;
      }

      tagArray = taosArrayDestroy(tagArray);
      taosArrayDestroyEx(crTblArray, (FDelete)tdDestroySVCreateTbReq);
      crTblArray = NULL;
    } else if (pDataBlock->info.type == STREAM_CHECKPOINT) {
      continue;
    } else {
      code = doSinkResultBlock(pVnode, i, stbFullName, suid, pDataBlock, pTask);
    }
  }

  tqDebug("vgId:%d, s-task:%s write results completed", vgId, pTask->id.idStr);

_end:
  taosArrayDestroy(tagArray);
  taosArrayDestroy(pVals);
  taosArrayDestroyEx(crTblArray, (FDelete)tdDestroySVCreateTbReq);
  // TODO: change
}

int32_t doSinkDeleteBlock(SVnode* pVnode, char* stbFullName, SSDataBlock* pDataBlock, SStreamTask* pTask,
                          int64_t suid) {
  SBatchDeleteReq deleteReq = {.suid = suid, .deleteReqs = taosArrayInit(0, sizeof(SSingleDeleteReq))};

  int32_t code = tqBuildDeleteReq(stbFullName, pDataBlock, &deleteReq, pTask->id.idStr);
  if (code != TSDB_CODE_SUCCESS) {
    return code;
  }

  if (taosArrayGetSize(deleteReq.deleteReqs) == 0) {
    taosArrayDestroy(deleteReq.deleteReqs);
    return TSDB_CODE_SUCCESS;
  }

  int32_t len;
  tEncodeSize(tEncodeSBatchDeleteReq, &deleteReq, len, code);
  if (code != TSDB_CODE_SUCCESS) {
    qError("s-task:%s failed to encode delete request", pTask->id.idStr);
    return code;
  }

  SEncoder encoder;
  void*    serializedDeleteReq = rpcMallocCont(len + sizeof(SMsgHead));
  void*    abuf = POINTER_SHIFT(serializedDeleteReq, sizeof(SMsgHead));
  tEncoderInit(&encoder, abuf, len);
  tEncodeSBatchDeleteReq(&encoder, &deleteReq);
  tEncoderClear(&encoder);
  taosArrayDestroy(deleteReq.deleteReqs);

  ((SMsgHead*)serializedDeleteReq)->vgId = TD_VID(pVnode);

  SRpcMsg msg = {.msgType = TDMT_VND_BATCH_DEL, .pCont = serializedDeleteReq, .contLen = len + sizeof(SMsgHead)};
  if (tmsgPutToQueue(&pVnode->msgCb, WRITE_QUEUE, &msg) != 0) {
    tqDebug("failed to put delete req into write-queue since %s", terrstr());
  }

  return TSDB_CODE_SUCCESS;
}

static bool isValidDestChildTable(SMetaReader* pReader, int32_t vgId, char* ctbName, int64_t suid) {
  if (pReader->me.type != TSDB_CHILD_TABLE) {
    tqError("vgId:%d, failed to write into %s, since table type:%d incorrect", vgId, ctbName, pReader->me.type);
    return false;
  }

  if (pReader->me.ctbEntry.suid != suid) {
    tqError("vgId:%d, failed to write into %s, since suid mismatch, expect suid:%" PRId64 ", actual:%" PRId64,
            vgId, ctbName, suid, pReader->me.ctbEntry.suid);
    return false;
  }

  return true;
}

static SVCreateTbReq* buildAutoCreateTableReq(char* stbFullName, int64_t suid, int32_t numOfCols, SSDataBlock* pDataBlock) {
  char* ctbName = pDataBlock->info.parTbName;

  SVCreateTbReq* pCreateTbReq = taosMemoryCalloc(1, sizeof(SVCreateStbReq));
  if (pCreateTbReq == NULL) {
    terrno = TSDB_CODE_OUT_OF_MEMORY;
    return NULL;
  }

  // set tag content
  SArray* tagArray = taosArrayInit(1, sizeof(STagVal));
  if (tagArray == NULL) {
    terrno = TSDB_CODE_OUT_OF_MEMORY;
    tdDestroySVCreateTbReq(pCreateTbReq);
    taosMemoryFreeClear(pCreateTbReq);
    return NULL;
  }

  // set const
  pCreateTbReq->flags = 0;
  pCreateTbReq->type = TSDB_CHILD_TABLE;
  pCreateTbReq->ctb.suid = suid;

  // set super table name
  SName name = {0};
  tNameFromString(&name, stbFullName, T_NAME_ACCT | T_NAME_DB | T_NAME_TABLE);
  pCreateTbReq->ctb.stbName = taosStrdup((char*)tNameGetTableName(&name));

  STagVal tagVal = { .cid = numOfCols, .type = TSDB_DATA_TYPE_UBIGINT, .i64 = pDataBlock->info.id.groupId};
  taosArrayPush(tagArray, &tagVal);
  pCreateTbReq->ctb.tagNum = taosArrayGetSize(tagArray);

  STag* pTag = NULL;
  tTagNew(tagArray, 1, false, &pTag);
  taosArrayDestroy(tagArray);

  if (pTag == NULL) {
    tdDestroySVCreateTbReq(pCreateTbReq);
    taosMemoryFreeClear(pCreateTbReq);
    terrno = TSDB_CODE_OUT_OF_MEMORY;
    return NULL;
  }

  pCreateTbReq->ctb.pTag = (uint8_t*)pTag;

  // set tag name
  SArray* tagName = taosArrayInit(1, TSDB_COL_NAME_LEN);
  char k[TSDB_COL_NAME_LEN] = "group_id";
  taosArrayPush(tagName, k);

  pCreateTbReq->ctb.tagName = tagName;

  // set table name
  pCreateTbReq->name = taosStrdup(ctbName);
  return pCreateTbReq;
}

static int32_t doPutIntoCache(SSHashObj* pSinkTableMap, STableSinkInfo* pTableSinkInfo, uint64_t groupId, uint64_t uid,
                              const char* id) {
  pTableSinkInfo->uid = uid;

  int32_t code = tqPutTableInfo(pSinkTableMap, groupId, pTableSinkInfo);
  if (code != TSDB_CODE_SUCCESS) {
    taosMemoryFreeClear(pTableSinkInfo);
    tqError("s-task:%s failed to put tableSinkInfo in to cache, code:%s", id, tstrerror(code));
  } else {
    tqDebug("s-task:%s new dst table:%s(uid:%" PRIu64 ") added into cache, total:%d", id, pTableSinkInfo->name.data,
            pTableSinkInfo->uid, tSimpleHashGetSize(pSinkTableMap));
  }

  return code;
}

int32_t doSinkResultBlock(SVnode* pVnode, int32_t blockIndex, char* stbFullName, int64_t suid, SSDataBlock* pDataBlock,
                          SStreamTask* pTask) {
  int32_t     numOfRows = pDataBlock->info.rows;
  int32_t     vgId = TD_VID(pVnode);
  uint64_t    groupId = pDataBlock->info.id.groupId;
  STSchema*   pTSchema = pTask->tbSink.pTSchema;
  int32_t     code = TSDB_CODE_SUCCESS;
  void*       pBuf = NULL;
  SArray*     pVals = NULL;
  const char* id = pTask->id.idStr;

  SSubmitTbData tbData = {.suid = suid, .uid = 0, .sver = pTSchema->version};
  tqDebug("s-task:%s sink data pipeline, build submit msg from %d-th resBlock, including %d rows, dst suid:%" PRId64,
          id, blockIndex + 1, numOfRows, suid);

  tbData.aRowP = taosArrayInit(numOfRows, sizeof(SRow*));
  pVals = taosArrayInit(pTSchema->numOfCols, sizeof(SColVal));

  if (tbData.aRowP == NULL || pVals == NULL) {
    taosArrayDestroy(tbData.aRowP);
    taosArrayDestroy(pVals);

    code = TSDB_CODE_OUT_OF_MEMORY;
    tqError("s-task:%s vgId:%d failed to prepare write stream res blocks, code:%s", id, vgId, tstrerror(code));
    return code;
  }

  STableSinkInfo* pTableSinkInfo = NULL;
  bool exist = tqGetTableInfo(pTask->tbSink.pTblInfo, groupId, &pTableSinkInfo);

  char* dstTableName = pDataBlock->info.parTbName;
  if (exist) {
    if (dstTableName[0] == 0) {
      tstrncpy(dstTableName, pTableSinkInfo->name.data, pTableSinkInfo->name.len + 1);
      tqDebug("s-task:%s vgId:%d, gropuId:%" PRIu64 " datablock table name is null, set name:%s", id, vgId, groupId,
              dstTableName);
    } else {
      if (pTableSinkInfo->uid != 0) {
        tqDebug("s-task:%s write %d rows into groupId:%" PRIu64 " dstTable:%s(uid:%" PRIu64 ")", id, numOfRows, groupId,
                dstTableName, pTableSinkInfo->uid);
      } else {
        tqDebug("s-task:%s write %d rows into groupId:%" PRIu64 " dstTable:%s(not set uid yet for the secondary block)",
                id, numOfRows, groupId, dstTableName);
      }
    }
  } else {  // not exist
    if (dstTableName[0] == 0) {
      memset(dstTableName, 0, TSDB_TABLE_NAME_LEN);
      buildCtbNameByGroupIdImpl(stbFullName, groupId, dstTableName);
    }

    int32_t nameLen = strlen(dstTableName);
    pTableSinkInfo = taosMemoryCalloc(1, sizeof(STableSinkInfo) + nameLen);

    pTableSinkInfo->name.len = nameLen;
    memcpy(pTableSinkInfo->name.data, dstTableName, nameLen);
    tqDebug("s-task:%s build new sinkTableInfo to add cache, dstTable:%s", id, dstTableName);
  }

  if (exist) {
    tbData.uid = pTableSinkInfo->uid;

    if (tbData.uid == 0) {
      tqDebug("s-task:%s cached tableInfo uid is invalid, acquire it from meta", id);
    }

    while (pTableSinkInfo->uid == 0) {
      // wait for the table to be created
      SMetaReader mr = {0};
      metaReaderDoInit(&mr, pVnode->pMeta, 0);

      code = metaGetTableEntryByName(&mr, dstTableName);
      if (code == 0) {  // table alreay exists, check its type and uid
        bool isValid = isValidDestChildTable(&mr, vgId, dstTableName, suid);
        if (!isValid) { // not valid table, ignore it
          metaReaderClear(&mr);

          taosArrayDestroy(tbData.aRowP);
          taosArrayDestroy(pVals);

          return TSDB_CODE_SUCCESS;
        } else {
          tqDebug("s-task:%s set uid:%"PRIu64" for dstTable:%s from meta", id, mr.me.uid, pTableSinkInfo->name.data);

          tbData.uid = mr.me.uid;
          pTableSinkInfo->uid = mr.me.uid;
          metaReaderClear(&mr);
        }
      } else { // not exist, wait and retry
        metaReaderClear(&mr);
        taosMsleep(100);
        tqDebug("s-task:%s wait for the table:%s ready before insert data", id, dstTableName);
      }
    }

  } else {
    // todo: this check is not safe, and results in losing of submit message from WAL.
    // The auto-create option will always set to be open for those submit messages, which arrive during the period
    // the creating of the destination table, due to the absence of the user-specified table in TSDB. When scanning
    // data from WAL, those submit messages, with auto-created table option, will be discarded expect the first, for
    // those mismatched table uids. Only the FIRST table has the correct table uid, and those remain all have
    // randomly generated false table uid in the WAL.
    SMetaReader mr = {0};
    metaReaderDoInit(&mr, pVnode->pMeta, 0);

    // table not in cache, let's try the extract it from tsdb meta
    if (metaGetTableEntryByName(&mr, dstTableName) < 0) {
      metaReaderClear(&mr);

      tqDebug("s-task:%s stream write into table:%s, table auto created", id, dstTableName);

      tbData.flags = SUBMIT_REQ_AUTO_CREATE_TABLE;
      tbData.pCreateTbReq = buildAutoCreateTableReq(stbFullName, suid, pTSchema->numOfCols + 1, pDataBlock);
      if (tbData.pCreateTbReq == NULL) {
        tqError("s-task:%s failed to build auto create table req, code:%s", id, tstrerror(terrno));

        taosArrayDestroy(tbData.aRowP);
        taosArrayDestroy(pVals);

        return terrno;
      }

      doPutIntoCache(pTask->tbSink.pTblInfo, pTableSinkInfo, groupId, 0, id);
    } else {
      bool isValid = isValidDestChildTable(&mr, vgId, dstTableName, suid);
      if (!isValid) {
        metaReaderClear(&mr);
        taosMemoryFree(pTableSinkInfo);
        taosArrayDestroy(tbData.aRowP);
        taosArrayDestroy(pVals);
        return TSDB_CODE_SUCCESS;
      } else {
        tbData.uid = mr.me.uid;
        metaReaderClear(&mr);

        doPutIntoCache(pTask->tbSink.pTblInfo, pTableSinkInfo, groupId, tbData.uid, id);
      }
    }
  }

  // rows
  for (int32_t j = 0; j < numOfRows; j++) {
    taosArrayClear(pVals);

    int32_t dataIndex = 0;
    for (int32_t k = 0; k < pTSchema->numOfCols; k++) {
      const STColumn* pCol = &pTSchema->columns[k];
      if (k == 0) {
        SColumnInfoData* pColData = taosArrayGet(pDataBlock->pDataBlock, dataIndex);
        void*            colData = colDataGetData(pColData, j);
        tqDebug("s-task:%s tq sink pipe2, row %d, col %d ts %" PRId64, id, j, k, *(int64_t*)colData);
      }

      if (IS_SET_NULL(pCol)) {
        SColVal cv = COL_VAL_NULL(pCol->colId, pCol->type);
        taosArrayPush(pVals, &cv);
      } else {
        SColumnInfoData* pColData = taosArrayGet(pDataBlock->pDataBlock, dataIndex);
        if (colDataIsNull_s(pColData, j)) {
          SColVal cv = COL_VAL_NULL(pCol->colId, pCol->type);
          taosArrayPush(pVals, &cv);
          dataIndex++;
        } else {
          void* colData = colDataGetData(pColData, j);
          if (IS_STR_DATA_TYPE(pCol->type)) {
            // address copy, no value
            SValue  sv = (SValue){.nData = varDataLen(colData), .pData = varDataVal(colData)};
            SColVal cv = COL_VAL_VALUE(pCol->colId, pCol->type, sv);
            taosArrayPush(pVals, &cv);
          } else {
            SValue sv;
            memcpy(&sv.val, colData, tDataTypes[pCol->type].bytes);
            SColVal cv = COL_VAL_VALUE(pCol->colId, pCol->type, sv);
            taosArrayPush(pVals, &cv);
          }
          dataIndex++;
        }
      }
    }

    SRow* pRow = NULL;
    code = tRowBuild(pVals, (STSchema*)pTSchema, &pRow);
    if (code != TSDB_CODE_SUCCESS) {
      tDestroySubmitTbData(&tbData, TSDB_MSG_FLG_ENCODE);

      taosArrayDestroy(tbData.aRowP);
      taosArrayDestroy(pVals);
      return code;
    }

    ASSERT(pRow);
    taosArrayPush(tbData.aRowP, &pRow);
  }

  SSubmitReq2 submitReq = {0};
  if (!(submitReq.aSubmitTbData = taosArrayInit(1, sizeof(SSubmitTbData)))) {
    tDestroySubmitTbData(&tbData, TSDB_MSG_FLG_ENCODE);

    taosArrayDestroy(tbData.aRowP);
    taosArrayDestroy(pVals);
    return TSDB_CODE_OUT_OF_MEMORY;
  }

  taosArrayPush(submitReq.aSubmitTbData, &tbData);

  // encode
  int32_t len = 0;
  tEncodeSize(tEncodeSubmitReq, &submitReq, len, code);

  SEncoder encoder;
  len += sizeof(SSubmitReq2Msg);

  pBuf = rpcMallocCont(len);
  if (NULL == pBuf) {
    tDestroySubmitReq(&submitReq, TSDB_MSG_FLG_ENCODE);
    taosArrayDestroy(tbData.aRowP);
    taosArrayDestroy(pVals);
  }

  ((SSubmitReq2Msg*)pBuf)->header.vgId = vgId;
  ((SSubmitReq2Msg*)pBuf)->header.contLen = htonl(len);
  ((SSubmitReq2Msg*)pBuf)->version = htobe64(1);

  tEncoderInit(&encoder, POINTER_SHIFT(pBuf, sizeof(SSubmitReq2Msg)), len - sizeof(SSubmitReq2Msg));
  if (tEncodeSubmitReq(&encoder, &submitReq) < 0) {
    terrno = TSDB_CODE_OUT_OF_MEMORY;
    tqError("failed to encode submit req, code:%s, ignore and continue", terrstr());
    tEncoderClear(&encoder);
    rpcFreeCont(pBuf);
    tDestroySubmitReq(&submitReq, TSDB_MSG_FLG_ENCODE);

    return code;
  }

  tEncoderClear(&encoder);
  tDestroySubmitReq(&submitReq, TSDB_MSG_FLG_ENCODE);

  SRpcMsg msg = { .msgType = TDMT_VND_SUBMIT, .pCont = pBuf, .contLen = len };
  code = tmsgPutToQueue(&pVnode->msgCb, WRITE_QUEUE, &msg);

  if(code == TSDB_CODE_SUCCESS) {
    tqDebug("s-task:%s send submit msg to dstTable:%s, numOfRows:%d", id, pTableSinkInfo->name.data, numOfRows);
  } else {
    tqError("s-task:%s failed to put into write-queue since %s", id, terrstr());
  }

  taosArrayDestroy(pVals);
  return code;
}