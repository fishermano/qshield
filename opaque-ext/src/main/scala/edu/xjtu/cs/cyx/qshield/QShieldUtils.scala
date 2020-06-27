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

import edu.xjtu.cs.cyx.qshield.execution.QShieldSGXEnclave
import edu.xjtu.cs.cyx.qshield.logical._
import edu.berkeley.cs.rise.opaque.logical.ConvertToOpaqueOperators
import edu.berkeley.cs.rise.opaque.Utils
import edu.berkeley.cs.rise.opaque.execution.Block

import java.nio.ByteBuffer
import scala.collection.mutable.ArrayBuilder
import com.google.flatbuffers.FlatBufferBuilder
import org.apache.spark.internal.Logging
import org.apache.spark.sql.SQLContext
import org.apache.spark.sql.types._

object QShieldUtils extends Logging{

  // initialize spark sql context
  // register self-defined rules and strategies to sql context
  // perform remote attestation with sql context
  def initQShieldSQLContext(sqlContext: SQLContext): Unit = {
    sqlContext.experimental.extraOptimizations =
      (Seq(ACPolicyApplyEncryptedBlockRDD, ConvertToOpaqueOperators, ConvertToQShieldOperators) ++
        sqlContext.experimental.extraOptimizations)
    sqlContext.experimental.extraStrategies =
      (Seq(QShieldOperators) ++
        sqlContext.experimental.extraStrategies)

    QOwnerStuber.initRA(sqlContext)

    QOwnerStuber.dataOut(sqlContext,
                          "file:///home/hadoop/QShield-DP/data/bdb/rankings/tiny",
                          "RANKINGS",
                          StructType(Seq(
                            StructField("pageURL", StringType),
                            StructField("pageRank", IntegerType),
                            StructField("avgDuration", IntegerType))),
                          sqlContext.sparkContext.defaultParallelism)

    QOwnerStuber.dataOut(sqlContext,
                          "file:///home/hadoop/QShield-DP/data/bdb/uservisits/tiny",
                          "USERVISITS",
                          StructType(Seq(
                            StructField("sourceIP", StringType),
                            StructField("destURL", StringType),
                            StructField("visitDate", StringType),
                            StructField("adRevenue", FloatType),
                            StructField("userAgent", StringType),
                            StructField("countryCode", StringType),
                            StructField("languageCode", StringType),
                            StructField("searchWord", StringType),
                            StructField("duration", IntegerType))),
                          sqlContext.sparkContext.defaultParallelism)

  }

  var eid = 0L

  def initEnclave(): (QShieldSGXEnclave, Long) = {
    this.synchronized {
      if (eid == 0L) {
        val enclave = new QShieldSGXEnclave()
        eid = enclave.StartEnclave(Utils.findLibraryAsResource("enclave_trusted_signed"))
        logInfo("Starting an enclave")
        (enclave, eid)
      } else {
        val enclave = new QShieldSGXEnclave()
        (enclave, eid)
      }
    }
  }

  def emptyBlock: Block = {
    val builder = new FlatBufferBuilder
    builder.finish(
      qix.QEncryptedBlocks.createQEncryptedBlocks(
        builder, qix.QEncryptedBlocks.createEncBlocksVector(builder, Array.empty)))
    Block(builder.sizedByteArray())
  }

  def concatQEncryptedBlocks(blocks: Seq[Block]): Block = {
    val runs = ArrayBuilder.make[qix.QEncryptedBlocks]
    for (block <- blocks) {
      val qEncryptedBlocks = qix.QEncryptedBlocks.getRootAsQEncryptedBlocks(ByteBuffer.wrap(block.bytes))
      runs += qEncryptedBlocks
    }

    val builder = new FlatBufferBuilder
    builder.finish(
      qix.QSortedRuns.createQSortedRuns(
        builder, qix.QSortedRuns.createRunsVector(builder, runs.result.map { run =>
          val runBytes = new Array[Byte](run.encBlocksLength)
          run.encBlocksAsByteBuffer.get(runBytes)
          qix.QEncryptedBlocks.createQEncryptedBlocks(
            builder, qix.QEncryptedBlocks.createEncBlocksVector(builder, runBytes))
        })))
    Block(builder.sizedByteArray())
  }
}
