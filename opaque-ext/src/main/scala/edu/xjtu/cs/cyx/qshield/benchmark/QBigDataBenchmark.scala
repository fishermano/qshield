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

package edu.xjtu.cs.cyx.qshield.benchmark

import edu.berkeley.cs.rise.opaque.Utils

import org.apache.spark.sql.DataFrame
import org.apache.spark.sql.SparkSession
import org.apache.spark.sql.functions._
import org.apache.spark.sql.types._

import edu.xjtu.cs.cyx.qshield.implicits._

object QBigDataBenchmark {

  def rankings(
      spark: SparkSession, qsecurityLevel: QSecurityLevel, size: String, numPartitions: Int)
    : DataFrame =
    qsecurityLevel.applyTo(
      spark.read.format("edu.berkeley.cs.rise.opaque.EncryptedSource")
        .schema(StructType(Seq(
          StructField("pageURL", StringType),
          StructField("pageRank", IntegerType),
          StructField("avgDuration", IntegerType))))
        .load("outsourced/".concat(size).concat("/RANKINGS")))

  def uservisits(
      spark: SparkSession, qsecurityLevel: QSecurityLevel, size: String, numPartitions: Int)
    : DataFrame =
    qsecurityLevel.applyTo(
      spark.read.format("edu.berkeley.cs.rise.opaque.EncryptedSource")
        .schema(StructType(Seq(
          StructField("sourceIP", StringType),
          StructField("destURL", StringType),
          StructField("visitDate", DateType),
          StructField("adRevenue", FloatType),
          StructField("userAgent", StringType),
          StructField("countryCode", StringType),
          StructField("languageCode", StringType),
          StructField("searchWord", StringType),
          StructField("duration", IntegerType))))
        .load("outsourced/".concat(size).concat("/USERVISITS")))

  def q1(spark: SparkSession, qsecurityLevel: QSecurityLevel, size: String, numPartitions: Int)
    : DataFrame = {
    import spark.implicits._
    val rankingsDF = Utils.ensureCached(rankings(spark, qsecurityLevel, size, numPartitions))
    Utils.time("load rankings") { Utils.force(rankingsDF) }
    Utils.timeBenchmark(
      "distributed" -> (numPartitions > 1),
      "query" -> "big data 1",
      "system" -> qsecurityLevel.name,
      "size" -> size) {
      val df = rankingsDF.filter($"pageRank" > 100)
      val dfRes = df.resPrepared
      Utils.force(dfRes)
      dfRes
    }
  }

  def q2(spark: SparkSession, qsecurityLevel: QSecurityLevel, size: String, numPartitions: Int)
    : DataFrame = {
    import spark.implicits._
    val uservisitsDF = Utils.ensureCached(uservisits(spark, qsecurityLevel, size, numPartitions))
    Utils.time("load uservisits") { Utils.force(uservisitsDF) }
    Utils.timeBenchmark(
      "distributed" -> (numPartitions > 1),
      "query" -> "big data 2",
      "system" -> qsecurityLevel.name,
      "size" -> size) {
      val df = uservisitsDF
        .select(substring($"sourceIP", 0, 8).as("sourceIPSubstr"), $"adRevenue")
        .groupBy($"sourceIPSubstr").sum("adRevenue")
      val dfRes = df.resPrepared
      Utils.force(dfRes)
      dfRes
    }
  }

  def q3(spark: SparkSession, qsecurityLevel: QSecurityLevel, size: String, numPartitions: Int)
    : DataFrame = {
    import spark.implicits._
    val uservisitsDF = Utils.ensureCached(uservisits(spark, qsecurityLevel, size, numPartitions))
    Utils.time("load uservisits") { Utils.force(uservisitsDF) }
    val rankingsDF = Utils.ensureCached(rankings(spark, qsecurityLevel, size, numPartitions))
    Utils.time("load rankings") { Utils.force(rankingsDF) }
    Utils.timeBenchmark(
      "distributed" -> (numPartitions > 1),
      "query" -> "big data 3",
      "system" -> qsecurityLevel.name,
      "size" -> size) {
      val userDF = uservisitsDF
        .filter($"duration" >= 2 && $"duration" <= 3)
        .select($"destURL", $"sourceIP", $"adRevenue")
      val joinDF = rankingsDF
        .join(userDF
          ,
          rankingsDF("pageURL") === uservisitsDF("destURL"))
      val df = joinDF
        .select($"sourceIP", $"pageRank")
        .orderBy($"pageRank".asc)
      val dfRes = df.resPrepared
      Utils.force(dfRes)
      dfRes
    }
  }

  def q(spark: SparkSession, qsecurityLevel: QSecurityLevel, size: String, numPartitions: Int) : DataFrame = {
    import spark.implicits._
    val uservisitsDF = Utils.ensureCached(uservisits(spark, qsecurityLevel, size, numPartitions))
    Utils.time("load uservisits") { Utils.force(uservisitsDF) }
    val rankingsDF = Utils.ensureCached(rankings(spark, qsecurityLevel, size, numPartitions))
    Utils.time("load rankings") { Utils.force(rankingsDF) }

    Utils.timeBenchmark(
      "distributed" -> (numPartitions > 1),
      "query" -> "big data",
      "system" -> qsecurityLevel.name,
      "size" -> size) {
      val df1 = uservisitsDF.select($"adRevenue", $"destURL", $"sourceIP")
      val df = df1.orderBy($"adRevenue".asc)
        //.join(
        //  uservisitsDF,
        //  rankingsDF("pageURL") === uservisitsDF("destURL"))
      val dfRes = df.resPrepared
      Utils.force(dfRes)
      dfRes
    }
  }
}
