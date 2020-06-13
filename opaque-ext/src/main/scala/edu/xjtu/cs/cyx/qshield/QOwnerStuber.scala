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

package edu.xjtu.cs.cyx.qshield

import org.apache.spark.sql.SQLContext
import org.apache.spark.internal.Logging
import org.apache.spark.sql.types._

import edu.xjtu.cs.cyx.qshield.owner.QSP
import edu.berkeley.cs.rise.opaque.Utils

/**
 * @annotated by cyx
 *
 * define OwnerStuber object, which simulates the data owner:
 * launching remote attestation
 * outsourcing data
 */
object QOwnerStuber extends Logging {
  val qsp = new QSP()

  def initRA(sqlContext: SQLContext): Unit = {

    val sc = sqlContext.sparkContext

    // val rdd = sc.makeRDD(Seq.fill(sc.defaultParallelism) { () })
    /**
     * modified by cyx
     * attestation is not successful when setting partitions number larger than 1 (why?)
     */
    val rdd = sc.makeRDD(Seq.fill(1) { () }, 1)

    val intelCert = Utils.findResource("AttestationReportSigningCACert.pem")
    val param = Utils.findResource("a.param");

    // Retry attestation a few times in case of transient failures
    Utils.retry(3) {

      qsp.QInit(Utils.sharedKey, param, intelCert)

      val epids = rdd.mapPartitions { _ =>
        val (enclave, eid) = QShieldUtils.initEnclave()
        val epid = enclave.RemoteAttestation0(eid)
        Iterator(epid)
      }.collect

      for (epid <- epids) {
        qsp.QSPProcMsg0(epid)
      }

      val msg1s = rdd.mapPartitionsWithIndex { (i, _) =>
        val (enclave, eid) = QShieldUtils.initEnclave()
        val msg1 = enclave.RemoteAttestation1(eid)
        Iterator((i, msg1))
      }.collect.toMap

      val msg2s = msg1s.mapValues(msg1 => qsp.QSPProcMsg1(msg1)).map(identity)

      val msg3s = rdd.mapPartitionsWithIndex { (i, _) =>
        val (enclave, eid) = QShieldUtils.initEnclave()
        val msg3 = enclave.RemoteAttestation2(eid, msg2s(i))
        Iterator((i, msg3))
      }.collect.toMap

      val msg4s = msg3s.mapValues(msg3 => qsp.QSPProcMsg3(msg3)).map(identity)

      val statuses = rdd.mapPartitionsWithIndex { (i, _) =>
        val (enclave, eid) = QShieldUtils.initEnclave()
        enclave.RemoteAttestation3(eid, msg4s(i))
        Iterator((i, true))
      }.collect.toMap

      assert(statuses.keySet == msg4s.keySet)
    }
  }

  def dataOut(sqlContext: SQLContext, srcFilePath: String,
                  tableName: String, schema: StructType,
                  numPartitions: Int): Unit = {
    val dstFilePath = qsp.QOutsource(sqlContext.sparkSession,
                                      srcFilePath,
                                      tableName,
                                      schema,
                                      numPartitions)

    println("***Data haved been outsourced to " + dstFilePath)
  }

  // for test only
  def getSk(): Array[Byte] = {
    return qsp.QSk()
  }
}
