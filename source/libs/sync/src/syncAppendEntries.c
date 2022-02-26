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

#include "syncAppendEntries.h"

void appendEntries(SRaft *pRaft, const SyncAppendEntries *pMsg) {
  // TLA+ Spec
  // AppendEntries(i, j) ==
  //    /\ i /= j
  //    /\ state[i] = Leader
  //    /\ LET prevLogIndex == nextIndex[i][j] - 1
  //           prevLogTerm == IF prevLogIndex > 0 THEN
  //                              log[i][prevLogIndex].term
  //                          ELSE
  //                              0
  //           \* Send up to 1 entry, constrained by the end of the log.
  //           lastEntry == Min({Len(log[i]), nextIndex[i][j]})
  //           entries == SubSeq(log[i], nextIndex[i][j], lastEntry)
  //       IN Send([mtype          |-> AppendEntriesRequest,
  //                mterm          |-> currentTerm[i],
  //                mprevLogIndex  |-> prevLogIndex,
  //                mprevLogTerm   |-> prevLogTerm,
  //                mentries       |-> entries,
  //                \* mlog is used as a history variable for the proof.
  //                \* It would not exist in a real implementation.
  //                mlog           |-> log[i],
  //                mcommitIndex   |-> Min({commitIndex[i], lastEntry}),
  //                msource        |-> i,
  //                mdest          |-> j])
  //    /\ UNCHANGED <<serverVars, candidateVars, leaderVars, logVars>>
}

void onAppendEntries(SRaft *pRaft, const SyncAppendEntries *pMsg) {
  // TLA+ Spec
  // HandleAppendEntriesRequest(i, j, m) ==
  //    LET logOk == \/ m.mprevLogIndex = 0
  //                 \/ /\ m.mprevLogIndex > 0
  //                    /\ m.mprevLogIndex <= Len(log[i])
  //                    /\ m.mprevLogTerm = log[i][m.mprevLogIndex].term
  //    IN /\ m.mterm <= currentTerm[i]
  //       /\ \/ /\ \* reject request
  //                \/ m.mterm < currentTerm[i]
  //                \/ /\ m.mterm = currentTerm[i]
  //                   /\ state[i] = Follower
  //                   /\ \lnot logOk
  //             /\ Reply([mtype           |-> AppendEntriesResponse,
  //                       mterm           |-> currentTerm[i],
  //                       msuccess        |-> FALSE,
  //                       mmatchIndex     |-> 0,
  //                       msource         |-> i,
  //                       mdest           |-> j],
  //                       m)
  //             /\ UNCHANGED <<serverVars, logVars>>
  //          \/ \* return to follower state
  //             /\ m.mterm = currentTerm[i]
  //             /\ state[i] = Candidate
  //             /\ state' = [state EXCEPT ![i] = Follower]
  //             /\ UNCHANGED <<currentTerm, votedFor, logVars, messages>>
  //          \/ \* accept request
  //             /\ m.mterm = currentTerm[i]
  //             /\ state[i] = Follower
  //             /\ logOk
  //             /\ LET index == m.mprevLogIndex + 1
  //                IN \/ \* already done with request
  //                       /\ \/ m.mentries = << >>
  //                          \/ /\ m.mentries /= << >>
  //                             /\ Len(log[i]) >= index
  //                             /\ log[i][index].term = m.mentries[1].term
  //                          \* This could make our commitIndex decrease (for
  //                          \* example if we process an old, duplicated request),
  //                          \* but that doesn't really affect anything.
  //                       /\ commitIndex' = [commitIndex EXCEPT ![i] =
  //                                              m.mcommitIndex]
  //                       /\ Reply([mtype           |-> AppendEntriesResponse,
  //                                 mterm           |-> currentTerm[i],
  //                                 msuccess        |-> TRUE,
  //                                 mmatchIndex     |-> m.mprevLogIndex +
  //                                                     Len(m.mentries),
  //                                 msource         |-> i,
  //                                 mdest           |-> j],
  //                                 m)
  //                       /\ UNCHANGED <<serverVars, log>>
  //                   \/ \* conflict: remove 1 entry
  //                       /\ m.mentries /= << >>
  //                       /\ Len(log[i]) >= index
  //                       /\ log[i][index].term /= m.mentries[1].term
  //                       /\ LET new == [index2 \in 1..(Len(log[i]) - 1) |->
  //                                          log[i][index2]]
  //                          IN log' = [log EXCEPT ![i] = new]
  //                       /\ UNCHANGED <<serverVars, commitIndex, messages>>
  //                   \/ \* no conflict: append entry
  //                       /\ m.mentries /= << >>
  //                       /\ Len(log[i]) = m.mprevLogIndex
  //                       /\ log' = [log EXCEPT ![i] =
  //                                      Append(log[i], m.mentries[1])]
  //                       /\ UNCHANGED <<serverVars, commitIndex, messages>>
  //       /\ UNCHANGED <<candidateVars, leaderVars>>
  //
}
