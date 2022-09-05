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

#ifndef _TD_COMMON_TOKEN_H_
#define _TD_COMMON_TOKEN_H_

#define TK_OR                   1
#define TK_AND                  2
#define TK_UNION                3
#define TK_ALL                  4
#define TK_MINUS                5
#define TK_EXCEPT               6
#define TK_INTERSECT            7
#define TK_NK_BITAND            8
#define TK_NK_BITOR             9
#define TK_NK_LSHIFT            10
#define TK_NK_RSHIFT            11
#define TK_NK_PLUS              12
#define TK_NK_MINUS             13
#define TK_NK_STAR              14
#define TK_NK_SLASH             15
#define TK_NK_REM               16
#define TK_NK_CONCAT            17
#define TK_CREATE               18
#define TK_ACCOUNT              19
#define TK_NK_ID                20
#define TK_PASS                 21
#define TK_NK_STRING            22
#define TK_ALTER                23
#define TK_PPS                  24
#define TK_TSERIES              25
#define TK_STORAGE              26
#define TK_STREAMS              27
#define TK_QTIME                28
#define TK_DBS                  29
#define TK_USERS                30
#define TK_CONNS                31
#define TK_STATE                32
#define TK_USER                 33
#define TK_ENABLE               34
#define TK_NK_INTEGER           35
#define TK_SYSINFO              36
#define TK_DROP                 37
#define TK_GRANT                38
#define TK_ON                   39
#define TK_TO                   40
#define TK_REVOKE               41
#define TK_FROM                 42
#define TK_NK_COMMA             43
#define TK_READ                 44
#define TK_WRITE                45
#define TK_NK_DOT               46
#define TK_DNODE                47
#define TK_PORT                 48
#define TK_DNODES               49
#define TK_NK_IPTOKEN           50
#define TK_LOCAL                51
#define TK_QNODE                52
#define TK_BNODE                53
#define TK_SNODE                54
#define TK_MNODE                55
#define TK_DATABASE             56
#define TK_USE                  57
#define TK_FLUSH                58
#define TK_TRIM                 59
#define TK_IF                   60
#define TK_NOT                  61
#define TK_EXISTS               62
#define TK_BUFFER               63
#define TK_CACHEMODEL           64
#define TK_CACHESIZE            65
#define TK_COMP                 66
#define TK_DURATION             67
#define TK_NK_VARIABLE          68
#define TK_MAXROWS              69
#define TK_MINROWS              70
#define TK_KEEP                 71
#define TK_PAGES                72
#define TK_PAGESIZE             73
#define TK_PRECISION            74
#define TK_REPLICA              75
#define TK_STRICT               76
#define TK_VGROUPS              77
#define TK_SINGLE_STABLE        78
#define TK_RETENTIONS           79
#define TK_SCHEMALESS           80
#define TK_WAL_LEVEL            81
#define TK_WAL_FSYNC_PERIOD     82
#define TK_WAL_RETENTION_PERIOD 83
#define TK_WAL_RETENTION_SIZE   84
#define TK_WAL_ROLL_PERIOD      85
#define TK_WAL_SEGMENT_SIZE     86
#define TK_SST_TRIGGER          87
#define TK_TABLE_PREFIX         88
#define TK_TABLE_SUFFIX         89
#define TK_NK_COLON             90
#define TK_TABLE                91
#define TK_NK_LP                92
#define TK_NK_RP                93
#define TK_STABLE               94
#define TK_ADD                  95
#define TK_COLUMN               96
#define TK_MODIFY               97
#define TK_RENAME               98
#define TK_TAG                  99
#define TK_SET                  100
#define TK_NK_EQ                101
#define TK_USING                102
#define TK_TAGS                 103
#define TK_COMMENT              104
#define TK_BOOL                 105
#define TK_TINYINT              106
#define TK_SMALLINT             107
#define TK_INT                  108
#define TK_INTEGER              109
#define TK_BIGINT               110
#define TK_FLOAT                111
#define TK_DOUBLE               112
#define TK_BINARY               113
#define TK_TIMESTAMP            114
#define TK_NCHAR                115
#define TK_UNSIGNED             116
#define TK_JSON                 117
#define TK_VARCHAR              118
#define TK_MEDIUMBLOB           119
#define TK_BLOB                 120
#define TK_VARBINARY            121
#define TK_DECIMAL              122
#define TK_MAX_DELAY            123
#define TK_WATERMARK            124
#define TK_ROLLUP               125
#define TK_TTL                  126
#define TK_SMA                  127
#define TK_FIRST                128
#define TK_LAST                 129
#define TK_SHOW                 130
#define TK_DATABASES            131
#define TK_TABLES               132
#define TK_STABLES              133
#define TK_MNODES               134
#define TK_MODULES              135
#define TK_QNODES               136
#define TK_FUNCTIONS            137
#define TK_INDEXES              138
#define TK_ACCOUNTS             139
#define TK_APPS                 140
#define TK_CONNECTIONS          141
#define TK_LICENCES             142
#define TK_GRANTS               143
#define TK_QUERIES              144
#define TK_SCORES               145
#define TK_TOPICS               146
#define TK_VARIABLES            147
#define TK_BNODES               148
#define TK_SNODES               149
#define TK_CLUSTER              150
#define TK_TRANSACTIONS         151
#define TK_DISTRIBUTED          152
#define TK_CONSUMERS            153
#define TK_SUBSCRIPTIONS        154
#define TK_VNODES               155
#define TK_LIKE                 156
#define TK_INDEX                157
#define TK_FUNCTION             158
#define TK_INTERVAL             159
#define TK_TOPIC                160
#define TK_AS                   161
#define TK_WITH                 162
#define TK_META                 163
#define TK_CONSUMER             164
#define TK_GROUP                165
#define TK_DESC                 166
#define TK_DESCRIBE             167
#define TK_RESET                168
#define TK_QUERY                169
#define TK_CACHE                170
#define TK_EXPLAIN              171
#define TK_ANALYZE              172
#define TK_VERBOSE              173
#define TK_NK_BOOL              174
#define TK_RATIO                175
#define TK_NK_FLOAT             176
#define TK_OUTPUTTYPE           177
#define TK_AGGREGATE            178
#define TK_BUFSIZE              179
#define TK_STREAM               180
#define TK_INTO                 181
#define TK_TRIGGER              182
#define TK_AT_ONCE              183
#define TK_WINDOW_CLOSE         184
#define TK_IGNORE               185
#define TK_EXPIRED              186
#define TK_KILL                 187
#define TK_CONNECTION           188
#define TK_TRANSACTION          189
#define TK_BALANCE              190
#define TK_VGROUP               191
#define TK_MERGE                192
#define TK_REDISTRIBUTE         193
#define TK_SPLIT                194
#define TK_DELETE               195
#define TK_INSERT               196
#define TK_NULL                 197
#define TK_NK_QUESTION          198
#define TK_NK_ARROW             199
#define TK_ROWTS                200
#define TK_TBNAME               201
#define TK_QSTART               202
#define TK_QEND                 203
#define TK_QDURATION            204
#define TK_WSTART               205
#define TK_WEND                 206
#define TK_WDURATION            207
#define TK_CAST                 208
#define TK_NOW                  209
#define TK_TODAY                210
#define TK_TIMEZONE             211
#define TK_CLIENT_VERSION       212
#define TK_SERVER_VERSION       213
#define TK_SERVER_STATUS        214
#define TK_CURRENT_USER         215
#define TK_COUNT                216
#define TK_LAST_ROW             217
#define TK_BETWEEN              218
#define TK_IS                   219
#define TK_NK_LT                220
#define TK_NK_GT                221
#define TK_NK_LE                222
#define TK_NK_GE                223
#define TK_NK_NE                224
#define TK_MATCH                225
#define TK_NMATCH               226
#define TK_CONTAINS             227
#define TK_IN                   228
#define TK_JOIN                 229
#define TK_INNER                230
#define TK_SELECT               231
#define TK_DISTINCT             232
#define TK_WHERE                233
#define TK_PARTITION            234
#define TK_BY                   235
#define TK_SESSION              236
#define TK_STATE_WINDOW         237
#define TK_SLIDING              238
#define TK_FILL                 239
#define TK_VALUE                240
#define TK_NONE                 241
#define TK_PREV                 242
#define TK_LINEAR               243
#define TK_NEXT                 244
#define TK_HAVING               245
#define TK_RANGE                246
#define TK_EVERY                247
#define TK_ORDER                248
#define TK_SLIMIT               249
#define TK_SOFFSET              250
#define TK_LIMIT                251
#define TK_OFFSET               252
#define TK_ASC                  253
#define TK_NULLS                254
#define TK_ABORT                255
#define TK_AFTER                256
#define TK_ATTACH               257
#define TK_BEFORE               258
#define TK_BEGIN                259
#define TK_BITAND               260
#define TK_BITNOT               261
#define TK_BITOR                262
#define TK_BLOCKS               263
#define TK_CHANGE               264
#define TK_COMMA                265
#define TK_COMPACT              266
#define TK_CONCAT               267
#define TK_CONFLICT             268
#define TK_COPY                 269
#define TK_DEFERRED             270
#define TK_DELIMITERS           271
#define TK_DETACH               272
#define TK_DIVIDE               273
#define TK_DOT                  274
#define TK_EACH                 275
#define TK_END                  276
#define TK_FAIL                 277
#define TK_FILE                 278
#define TK_FOR                  279
#define TK_GLOB                 280
#define TK_ID                   281
#define TK_IMMEDIATE            282
#define TK_IMPORT               283
#define TK_INITIALLY            284
#define TK_INSTEAD              285
#define TK_ISNULL               286
#define TK_KEY                  287
#define TK_NK_BITNOT            288
#define TK_NK_SEMI              289
#define TK_NOTNULL              290
#define TK_OF                   291
#define TK_PLUS                 292
#define TK_PRIVILEGE            293
#define TK_RAISE                294
#define TK_REPLACE              295
#define TK_RESTRICT             296
#define TK_ROW                  297
#define TK_SEMI                 298
#define TK_STAR                 299
#define TK_STATEMENT            300
#define TK_STRING               301
#define TK_TIMES                302
#define TK_UPDATE               303
#define TK_VALUES               304
#define TK_VARIABLE             305
#define TK_VIEW                 306
#define TK_WAL                  307

#define TK_NK_SPACE   300
#define TK_NK_COMMENT 301
#define TK_NK_ILLEGAL 302
#define TK_NK_HEX     303  // hex number  0x123
#define TK_NK_OCT     304  // oct number
#define TK_NK_BIN     305  // bin format data 0b111

#define TK_NK_NIL 65535

#endif /*_TD_COMMON_TOKEN_H_*/
