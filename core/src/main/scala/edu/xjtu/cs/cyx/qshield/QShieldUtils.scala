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
import com.google.flatbuffers.FlatBufferBuilder
import org.apache.spark.internal.Logging
import org.apache.spark.sql.SQLContext
import org.apache.spark.sql.types._

object QShieldUtils extends Logging{

  val qshieldHome = System.getenv("QSHIELD_HOME")

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
                          "file://".concat(qshieldHome).concat("/data/bdb/rankings/tiny"),
                          "tiny/RANKINGS",
                          StructType(Seq(
                            StructField("pageURL", StringType),
                            StructField("pageRank", IntegerType),
                            StructField("avgDuration", IntegerType))),
                          sqlContext.sparkContext.defaultParallelism)

    QOwnerStuber.dataOut(sqlContext,
                          "file://".concat(qshieldHome).concat("/data/bdb/rankings/medium"),
                          "medium/RANKINGS",
                          StructType(Seq(
                            StructField("pageURL", StringType),
                            StructField("pageRank", IntegerType),
                            StructField("avgDuration", IntegerType))),
                          sqlContext.sparkContext.defaultParallelism)

    QOwnerStuber.dataOut(sqlContext,
                          "file://".concat(qshieldHome).concat("/data/bdb/rankings/big"),
                          "big/RANKINGS",
                          StructType(Seq(
                            StructField("pageURL", StringType),
                            StructField("pageRank", IntegerType),
                            StructField("avgDuration", IntegerType))),
                          sqlContext.sparkContext.defaultParallelism)

    QOwnerStuber.dataOut(sqlContext,
                          "file://".concat(qshieldHome).concat("/data/bdb/uservisits/tiny"),
                          "tiny/USERVISITS",
                          StructType(Seq(
                            StructField("sourceIP", StringType),
                            StructField("destURL", StringType),
                            StructField("visitDate", DateType),
                            StructField("adRevenue", FloatType),
                            StructField("userAgent", StringType),
                            StructField("countryCode", StringType),
                            StructField("languageCode", StringType),
                            StructField("searchWord", StringType),
                            StructField("duration", IntegerType))),
                          sqlContext.sparkContext.defaultParallelism)

    QOwnerStuber.dataOut(sqlContext,
                          "file://".concat(qshieldHome).concat("/data/bdb/uservisits/medium"),
                          "medium/USERVISITS",
                          StructType(Seq(
                            StructField("sourceIP", StringType),
                            StructField("destURL", StringType),
                            StructField("visitDate", DateType),
                            StructField("adRevenue", FloatType),
                            StructField("userAgent", StringType),
                            StructField("countryCode", StringType),
                            StructField("languageCode", StringType),
                            StructField("searchWord", StringType),
                            StructField("duration", IntegerType))),
                          sqlContext.sparkContext.defaultParallelism)

    QOwnerStuber.dataOut(sqlContext,
                          "file://".concat(qshieldHome).concat("/data/bdb/uservisits/big"),
                          "big/USERVISITS",
                          StructType(Seq(
                            StructField("sourceIP", StringType),
                            StructField("destURL", StringType),
                            StructField("visitDate", DateType),
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
        builder, qix.QEncryptedBlocks.createBlocksVector(builder, Array.empty)))
    Block(builder.sizedByteArray())
  }

  def concatQEncryptedBlocks(blocks: Seq[Block]): Block = {
    val allBlocks = for {
      block <- blocks
      encryptedBlocks = qix.QEncryptedBlocks.getRootAsQEncryptedBlocks(ByteBuffer.wrap(block.bytes))
      i <- 0 until encryptedBlocks.blocksLength
    } yield encryptedBlocks.blocks(i)

    val builder = new FlatBufferBuilder
    builder.finish(
      qix.QEncryptedBlocks.createQEncryptedBlocks(
        builder, qix.QEncryptedBlocks.createBlocksVector(builder, allBlocks.map { encryptedBlock =>
          val encRows = new Array[Byte](encryptedBlock.encRowsLength)
          encryptedBlock.encRowsAsByteBuffer.get(encRows)
          qix.QEncryptedBlock.createQEncryptedBlock(
            builder,
            encryptedBlock.numRows,
            qix.QEncryptedBlock.createEncRowsVector(builder, encRows))
        }.toArray)))
    Block(builder.sizedByteArray())
  }

}
