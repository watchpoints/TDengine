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

#ifndef _TD_QUERY_H_
#define _TD_QUERY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "tarray.h"

typedef SVgroupListRspMsg SVgroupListInfo;

typedef struct SDBVgroupInfo {
  int32_t vgroupVersion;
  SArray *vgId;
  int32_t hashRange;
} SDBVgroupInfo;

typedef struct SUseDbOutput {
  SVgroupListInfo *vgroupList;
  char db[TSDB_TABLE_FNAME_LEN];
  SDBVgroupInfo *dbVgroup;
} SUseDbOutput;


extern int32_t (*queryBuildMsg[TSDB_MSG_TYPE_MAX])(void* input, char **msg, int32_t msgSize, int32_t *msgLen);
extern int32_t (*queryProcessMsgRsp[TSDB_MSG_TYPE_MAX])(void* output, char *msg, int32_t msgSize);


#ifdef __cplusplus
}
#endif

#endif /*_TD_QUERY_H_*/
