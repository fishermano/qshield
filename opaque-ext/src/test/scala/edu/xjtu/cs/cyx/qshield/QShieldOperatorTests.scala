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
import org.apache.spark.sql.SQLImplicits
import org.apache.spark.sql.SparkSession
import org.scalactic.Equality
import org.scalatest.BeforeAndAfterAll
import org.scalatest.FunSuite

import edu.xjtu.cs.cyx.qshield.benchmark._

trait QShieldOperatorTests extends FunSuite with BeforeAndAfterAll { self =>
  def spark: SparkSession
  def numPartitions: Int

  protected object testImplicits extends SQLImplicits {
    protected override def _sqlContext: SQLContext = self.spark.sqlContext
  }

  override def beforeAll(): Unit = {
    QShieldUtils.initQShieldSQLContext(spark.sqlContext)
  }

  override def afterAll(): Unit = {
    spark.stop()
  }

  private def equalityToArrayEquality[A : Equality](): Equality[Array[A]] = {
    new Equality[Array[A]] {
      def areEqual(a: Array[A], b: Any): Boolean = {
        b match {
          case b: Array[_] =>
            (a.length == b.length
              && a.zip(b).forall {
                case (x, y) => implicitly[Equality[A]].areEqual(x, y)
              })
          case _ => false
        }
      }
      override def toString: String = s"TolerantArrayEquality"
    }
  }

  def testQShield(name: String)(f: QSecurityLevel => Unit): Unit = {
    test(name + " - q encrypted") {
      f(QEncrypted)
    }
  }
/**
  testQShield("big data 1") { qsecurityLevel =>
    QBigDataBenchmark.q1(spark, qsecurityLevel, "big", numPartitions).collect
  }

  testQShield("big data 2") { qsecurityLevel =>
    QBigDataBenchmark.q2(spark, qsecurityLevel, "big", numPartitions).collect
  }
*/
  testQShield("big data 3") { qsecurityLevel =>
    val res = QBigDataBenchmark.q3(spark, qsecurityLevel, "big", numPartitions).collect
    println("..............")
    println(res.size)
  }

/**
  testQShield("operator") { qsecurityLevel =>
    val res = QBigDataBenchmark.q(spark, qsecurityLevel, "medium", numPartitions).collect
    println("..............")
    println(res.size)
  }
*/
}

class QShieldSinglePartitionSuite extends QShieldOperatorTests {

  override val spark = SparkSession.builder()
    .master("local[1]")
    .config("spark.debug.maxToStringFields", "1000")
    .config("spark.executor.memory", "12g")
    .appName("QShield QEDSuite")
    .getOrCreate()

/**
  override val spark = SparkSession.builder()
    .master("spark://SGX:7077")
    .config("spark.jars", "/home/hadoop/QShield-DP/opaque-ext/target/scala-2.11/opaque-ext_2.11-0.1.jar,/home/hadoop/QShield-DP/data-owner/target/scala-2.11/data-owner_2.11-0.1.jar")
    .config("spark.executor.memory", "12g")
    .config("spark.driver.memory", "2g")
    .config("spark.memory.fraction", 0.9)
    .config("spark.debug.maxToStringFields", 1000)
    .appName("QShield QEDSuite")
    .getOrCreate()
*/
  override def numPartitions: Int = 1
}
