/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package edu.xjtu.cs.cyx.qshield.execution

import ch.jodersky.jni.nativeLoader
import edu.berkeley.cs.rise.opaque.execution.SGXEnclave

@nativeLoader("enclave_jni")
class QShieldSGXEnclave extends SGXEnclave with java.io.Serializable {
  @native def ACPolicyApplied(eid: Long, input: Array[Byte], tk: Array[Byte]): Array[Byte]
  @native def ResPrepared(eid: Long, input: Array[Byte]): Array[Byte]

  @native def QProject(eid: Long, projectList: Array[Byte], input: Array[Byte]): Array[Byte]
  @native def QFilter(eid: Long, condition: Array[Byte], input: Array[Byte]): Array[Byte]

  @native def QAggregateStep1(
    eid: Long, aggOp: Array[Byte], inputRows: Array[Byte]): (Array[Byte], Array[Byte], Array[Byte])
  @native def QAggregateStep2(
    eid: Long, aggOp: Array[Byte], inputRows: Array[Byte], nextPartitionFirstRow: Array[Byte],
    prevPartitionLastGroup: Array[Byte], prevPartitionLastRow: Array[Byte]): Array[Byte]

  @native def QSample(eid: Long, input: Array[Byte]): Array[Byte]
  @native def QFindRangeBounds(
    eid: Long, order: Array[Byte], numPartitions: Int, input: Array[Byte]): Array[Byte]
  @native def QPartitionForSort(
    eid: Long, order: Array[Byte], numPartitions: Int, input: Array[Byte],
    boundaries: Array[Byte]): Array[Array[Byte]]
  @native def QExternalSort(eid: Long, order: Array[Byte], input: Array[Byte]): Array[Byte]

  @native def QScanCollectLastPrimary(
    eid: Long, joinExpr: Array[Byte], input: Array[Byte]): Array[Byte]
  @native def QSortMergeJoin(
    eid: Long, joinExpr: Array[Byte], input: Array[Byte], joinRow: Array[Byte]): Array[Byte]

  @native def InitPairing(eid: Long, param: Array[Byte]): Unit
}
