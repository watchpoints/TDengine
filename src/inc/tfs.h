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

#ifndef TD_TFS_H
#define TD_TFS_H

#include "tglobal.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  int level;
  int id;
} SDiskID;

#define TFS_UNDECIDED_LEVEL -1
#define TFS_UNDECIDED_ID -1

// tfs.c ====================================
int  tfsInit(SDiskCfg *pDiskCfg, int ndisk);
void tfsDestroy();
int  tfsUpdateInfo();

const char *tfsGetDiskName(int level, int id);
const char *tfsPrimaryPath();

// tfcntl.c ====================================
typedef struct TFSFILE {
  int  level;
  int  id;
  char rname[TSDB_FILENAME_LEN];  // REL name
  char aname[TSDB_FILENAME_LEN];  // ABS name
} TFSFILE;

const char *tfsAbsName(TFSFILE *pfile);
const char *tfsRelName(TFSFILE *pfile);
void        tfsDirName(TFSFILE *pfile, char dest[]);
void        tfsBaseName(TFSFILE *pfile, char dest[]);
int         tfsopen(TFSFILE *pfile, int flags);
int         tfsclose(int fd);
int         tfsRemoveFiles(int nfile, ...);
SDiskID     tfsFileID(TFSFILE *pfile);

typedef struct TFSDIR TFSDIR;

int            tfsCreateDir(char *dirname);
int            tfsRemoveDir(char *dirname);
int            tfsRename(char *oldpath, char *newpath);
TFSDIR *       tfsOpenDir(char *dir);
void           tfsCloseDir(TFSDIR *tdir);
const TFSFILE *tfsReadDir(TFSDIR *tdir);

#ifdef __cplusplus
}
#endif

#endif