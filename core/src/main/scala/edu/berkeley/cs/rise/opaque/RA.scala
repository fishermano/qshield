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

package edu.berkeley.cs.rise.opaque

import org.apache.spark.SparkContext
import org.apache.spark.internal.Logging

import edu.xjtu.cs.cyx.qshield.owner.SP

/**
 * @annotated by cyx
 *
 * define RA object, which launches remote attestation
 */
object RA extends Logging {
  def initRA(sc: SparkContext): Unit = {

    // val rdd = sc.makeRDD(Seq.fill(sc.defaultParallelism) { () })
    /**
     * modified by cyx
     * attestation is not successful when setting partitions number larger than 1 (why?)
     */
    val rdd = sc.makeRDD(Seq.fill(1) { () }, 1)

    val intelCert = Utils.findResource("AttestationReportSigningCACert.pem")

    val sp = new SP()

    // Retry attestation a few times in case of transient failures
    Utils.retry(3) {
      sp.Init(Utils.sharedKey, intelCert)

      val epids = rdd.mapPartitions { _ =>
        val (enclave, eid) = Utils.initEnclave()
        val epid = enclave.RemoteAttestation0(eid)
        Iterator(epid)
      }.collect

      for (epid <- epids) {
        sp.SPProcMsg0(epid)
      }

      val msg1s = rdd.mapPartitionsWithIndex { (i, _) =>
        val (enclave, eid) = Utils.initEnclave()
        val msg1 = enclave.RemoteAttestation1(eid)
        Iterator((i, msg1))
      }.collect.toMap

      val msg2s = msg1s.mapValues(msg1 => sp.SPProcMsg1(msg1)).map(identity)

      val msg3s = rdd.mapPartitionsWithIndex { (i, _) =>
        val (enclave, eid) = Utils.initEnclave()
        val msg3 = enclave.RemoteAttestation2(eid, msg2s(i))
        Iterator((i, msg3))
      }.collect.toMap

      val msg4s = msg3s.mapValues(msg3 => sp.SPProcMsg3(msg3)).map(identity)

      val statuses = rdd.mapPartitionsWithIndex { (i, _) =>
        val (enclave, eid) = Utils.initEnclave()
        enclave.RemoteAttestation3(eid, msg4s(i))
        Iterator((i, true))
      }.collect.toMap

      assert(statuses.keySet == msg4s.keySet)
    }
  }
}
