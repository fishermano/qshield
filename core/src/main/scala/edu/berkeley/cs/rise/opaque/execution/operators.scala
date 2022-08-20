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

 /**
  * @annotated by cyx
  *
  * define opaque physical plan (SparkPlan)
  */


package edu.berkeley.cs.rise.opaque.execution

import scala.collection.mutable.ArrayBuffer

import edu.berkeley.cs.rise.opaque.Utils
import org.apache.spark.rdd.RDD
import org.apache.spark.sql.catalyst.InternalRow
import org.apache.spark.sql.catalyst.expressions.AttributeSet
import org.apache.spark.sql.catalyst.expressions._
import org.apache.spark.sql.catalyst.plans.JoinType
import org.apache.spark.sql.catalyst.plans.physical.Partitioning
import org.apache.spark.sql.execution.SparkPlan

/**
 * @annotated by cyx
 *
 * redefine LeafExecNode trait
 */
trait LeafExecNode extends SparkPlan {
  override final def children: Seq[SparkPlan] = Nil
  override def producedAttributes: AttributeSet = outputSet
}

/**
 * @annotated by cyx
 *
 * redefine UnaryExecNode trait
 */
trait UnaryExecNode extends SparkPlan {
  def child: SparkPlan

  override final def children: Seq[SparkPlan] = child :: Nil

  override def outputPartitioning: Partitioning = child.outputPartitioning
}

/**
 * @annotated by cyx
 *
 * redefine BinaryExecNode trait
 */
trait BinaryExecNode extends SparkPlan {
  def left: SparkPlan
  def right: SparkPlan

  override final def children: Seq[SparkPlan] = Seq(left, right)
}

/**
 * @annotated by cyx
 *
 * define OpaqueOperatorExec trait, within which (re)defines several common functions
 * used by its successors
 */
trait OpaqueOperatorExec extends SparkPlan {
  def executeBlocked(): RDD[Block]

  // evaluating the performance when executing functions
  def timeOperator[A](childRDD: RDD[A], desc: String)(f: RDD[A] => RDD[Block]): RDD[Block] = {
    import Utils.time
    Utils.ensureCached(childRDD)
    time(s"Force child of $desc") { childRDD.count }
    time(desc) {
      val result = f(childRDD)
      Utils.ensureCached(result)
      result.count
      result
    }
  }

  /**
   * An Opaque operator cannot return plaintext rows, so this method should normally not be invoked.
   * Instead use executeBlocked, which returns the data as encrypted blocks.
   *
   * However, when encrypted data is cached, Spark SQL's InMemoryRelation attempts to call this
   * method and persist the resulting RDD. [[ConvertToOpaqueOperators]] later eliminates the dummy
   * relation from the logical plan, but this only happens after InMemoryRelation has called this
   * method. We therefore have to silently return an empty RDD here.
   */
  override def doExecute(): RDD[InternalRow] = {
    sqlContext.sparkContext.emptyRDD
    // throw new UnsupportedOperationException("use executeBlocked")
  }

  // redefine executeCollect()
  // decrypt block and serialize the plaintext as InternalRow
  override def executeCollect(): Array[InternalRow] = {
    executeBlocked().collect().flatMap { block =>
      Utils.decryptBlockFlatbuffers(block)
    }
  }

  // redefine executeTake()
  // take first InternalRows
  override def executeTake(n: Int): Array[InternalRow] = {
    if (n == 0) {
      return new Array[InternalRow](0)
    }

    val childRDD = executeBlocked()

    val buf = new ArrayBuffer[InternalRow]
    val totalParts = childRDD.partitions.length
    var partsScanned = 0
    while (buf.size < n && partsScanned < totalParts) {
      // The number of partitions to try in this iteration. It is ok for this number to be
      // greater than totalParts because we actually cap it at totalParts in runJob.
      var numPartsToTry = 1L
      if (partsScanned > 0) {
        // If we didn't find any rows after the first iteration, just try all partitions next.
        // Otherwise, interpolate the number of partitions we need to try, but overestimate it
        // by 50%.
        if (buf.size == 0) {
          numPartsToTry = totalParts - 1
        } else {
          numPartsToTry = (1.5 * n * partsScanned / buf.size).toInt
        }
      }
      numPartsToTry = math.max(0, numPartsToTry)  // guard against negative num of partitions

      val p = partsScanned.until(math.min(partsScanned + numPartsToTry, totalParts).toInt)
      val sc = sqlContext.sparkContext
      val res = sc.runJob(childRDD,
        (it: Iterator[Block]) => if (it.hasNext) Some(it.next()) else None, p)

      res.foreach {
        case Some(block) =>
          buf ++= Utils.decryptBlockFlatbuffers(block)
        case None =>
      }

      partsScanned += p.size
    }

    if (buf.size > n) {
      buf.take(n).toArray
    } else {
      buf.toArray
    }
  }
}

/**
 * @annotated by cyx
 *
 * define EncryptedLocalTableScanExec opaque physical plan, which create encrypted
 * block RDD from local table (InternalRow)
 */
case class EncryptedLocalTableScanExec(
    output: Seq[Attribute],
    plaintextData: Seq[InternalRow])
  extends LeafExecNode with OpaqueOperatorExec {

  private val unsafeRows: Array[InternalRow] = {
    val proj = UnsafeProjection.create(output, output)
    val result: Array[InternalRow] = plaintextData.map(r => proj(r).copy()).toArray
    result
  }

  override def executeBlocked(): RDD[Block] = {
    // Locally partition plaintextData using the same logic as ParallelCollectionRDD.slice
    def positions(length: Long, numSlices: Int): Iterator[(Int, Int)] = {
      (0 until numSlices).iterator.map { i =>
        val start = ((i * length) / numSlices).toInt
        val end = (((i + 1) * length) / numSlices).toInt
        (start, end)
      }
    }
    val slicedPlaintextData: Seq[Seq[InternalRow]] =
      positions(unsafeRows.length, sqlContext.sparkContext.defaultParallelism).map {
        case (start, end) => unsafeRows.slice(start, end).toSeq
      }.toSeq

    // Encrypt each local partition
    val encryptedPartitions: Seq[Block] =
      slicedPlaintextData.map(slice =>
        Utils.encryptInternalRowsFlatbuffers(slice, output.map(_.dataType), useEnclave = false))

    // Make an RDD from the encrypted partitions
    sqlContext.sparkContext.parallelize(encryptedPartitions)
  }
}

/**
 * @annotated by cyx
 *
 * define EncryptExec opaque physical plan, which create encrypted block RDD from
 * a SparkPlan
 */
case class EncryptExec(child: SparkPlan)
  extends UnaryExecNode with OpaqueOperatorExec {

  override def output: Seq[Attribute] = child.output

  override def executeBlocked(): RDD[Block] = {
    child.execute().mapPartitions { rowIter =>
      Iterator(Utils.encryptInternalRowsFlatbuffers(
        rowIter.toSeq, output.map(_.dataType), useEnclave = true))
    }
  }
}

/**
 * @annotated by cyx
 *
 * define EncryptedBlockRDDScanExec opaque physical plan, which create encrypted
 * block RDD from a Block.
 */
case class EncryptedBlockRDDScanExec(
    output: Seq[Attribute],
    rdd: RDD[Block])
  extends LeafExecNode with OpaqueOperatorExec {

  override def executeBlocked(): RDD[Block] = rdd
}

/**
 * @annotated by cyx
 *
 */
case class Block(bytes: Array[Byte]) extends Serializable


/**
 * @annotated by cyx
 *
 * define EncryptedProjectExec opaque physical plan, which executes project over
 * its child output within enclave.
 */
case class EncryptedProjectExec(projectList: Seq[NamedExpression], child: SparkPlan)
  extends UnaryExecNode with OpaqueOperatorExec {

  override def output: Seq[Attribute] = projectList.map(_.toAttribute)

  override def executeBlocked(): RDD[Block] = {
    val projectListSer = Utils.serializeProjectList(projectList, child.output)
    timeOperator(child.asInstanceOf[OpaqueOperatorExec].executeBlocked(), "EncryptedProjectExec") {
      childRDD => childRDD.map { block =>
        val (enclave, eid) = Utils.initEnclave()
        Block(enclave.Project(eid, projectListSer, block.bytes))
      }
    }
  }
}

/**
 * @annotated by cyx
 *
 * define EncryptedFilterExec opaque physical plan, which executes filter over
 * its child output within enclave.
 */
case class EncryptedFilterExec(condition: Expression, child: SparkPlan)
  extends UnaryExecNode with OpaqueOperatorExec {

  override def output: Seq[Attribute] = child.output

  override def executeBlocked(): RDD[Block] = {
    val conditionSer = Utils.serializeFilterExpression(condition, child.output)
    timeOperator(child.asInstanceOf[OpaqueOperatorExec].executeBlocked(), "EncryptedFilterExec") {
      childRDD => childRDD.map { block =>
        val (enclave, eid) = Utils.initEnclave()
        Block(enclave.Filter(eid, conditionSer, block.bytes))
      }
    }
  }
}

/**
 * @annotated by cyx
 *
 * define EncryptedAggregateExec opaque physical plan, which executes aggregate over
 * its child output within enclave.
 */
case class EncryptedAggregateExec(
    groupingExpressions: Seq[Expression],
    aggExpressions: Seq[NamedExpression],
    child: SparkPlan)
  extends UnaryExecNode with OpaqueOperatorExec {

  override def producedAttributes: AttributeSet =
    AttributeSet(aggExpressions) -- AttributeSet(groupingExpressions)

  override def output: Seq[Attribute] = aggExpressions.map(_.toAttribute)

  override def executeBlocked(): RDD[Block] = {
    val aggExprSer = Utils.serializeAggOp(groupingExpressions, aggExpressions, child.output)

    timeOperator(
      child.asInstanceOf[OpaqueOperatorExec].executeBlocked(),
      "EncryptedAggregateExec") { childRDD =>

      val (firstRows, lastGroups, lastRows) = childRDD.map { block =>
        val (enclave, eid) = Utils.initEnclave()
        val (firstRow, lastGroup, lastRow) = enclave.NonObliviousAggregateStep1(
          eid, aggExprSer, block.bytes)
        (Block(firstRow), Block(lastGroup), Block(lastRow))
      }.collect.unzip3

      // Send first row to previous partition and last group to next partition
      val shiftedFirstRows = firstRows.drop(1) :+ Utils.emptyBlock
      val shiftedLastGroups = Utils.emptyBlock +: lastGroups.dropRight(1)
      val shiftedLastRows = Utils.emptyBlock +: lastRows.dropRight(1)
      val shifted = (shiftedFirstRows, shiftedLastGroups, shiftedLastRows).zipped.toSeq
      assert(shifted.size == childRDD.partitions.length)
      val shiftedRDD = sparkContext.parallelize(shifted, childRDD.partitions.length)

      childRDD.zipPartitions(shiftedRDD) { (blockIter, boundaryIter) =>
        (blockIter.toSeq, boundaryIter.toSeq) match {
          case (Seq(block), Seq(Tuple3(
            nextPartitionFirstRow, prevPartitionLastGroup, prevPartitionLastRow))) =>
            val (enclave, eid) = Utils.initEnclave()
            Iterator(Block(enclave.NonObliviousAggregateStep2(
              eid, aggExprSer, block.bytes,
              nextPartitionFirstRow.bytes, prevPartitionLastGroup.bytes,
              prevPartitionLastRow.bytes)))
        }
      }
    }
  }
}

/**
 * @annotated by cyx
 *
 * define EncryptedSortMergeJoinExec opaque physical plan, which executes sort merge join over
 * its children output within enclave.
 */
case class EncryptedSortMergeJoinExec(
    joinType: JoinType,
    leftKeys: Seq[Expression],
    rightKeys: Seq[Expression],
    leftSchema: Seq[Attribute],
    rightSchema: Seq[Attribute],
    output: Seq[Attribute],
    child: SparkPlan)
  extends UnaryExecNode with OpaqueOperatorExec {

  override def executeBlocked(): RDD[Block] = {
    val joinExprSer = Utils.serializeJoinExpression(
      joinType, leftKeys, rightKeys, leftSchema, rightSchema)

    timeOperator(
      child.asInstanceOf[OpaqueOperatorExec].executeBlocked(),
      "EncryptedSortMergeJoinExec") { childRDD =>

      val lastPrimaryRows = childRDD.map { block =>
        val (enclave, eid) = Utils.initEnclave()
        Block(enclave.ScanCollectLastPrimary(eid, joinExprSer, block.bytes))
      }.collect
      val shifted = Utils.emptyBlock +: lastPrimaryRows.dropRight(1)
      assert(shifted.size == childRDD.partitions.length)
      val processedJoinRowsRDD =
        sparkContext.parallelize(shifted, childRDD.partitions.length)

      childRDD.zipPartitions(processedJoinRowsRDD) { (blockIter, joinRowIter) =>
        (blockIter.toSeq, joinRowIter.toSeq) match {
          case (Seq(block), Seq(joinRow)) =>
            val (enclave, eid) = Utils.initEnclave()
            Iterator(Block(enclave.NonObliviousSortMergeJoin(
              eid, joinExprSer, block.bytes, joinRow.bytes)))
        }
      }
    }
  }
}

/**
 * @annotated by cyx
 *
 * define EncryptedUnionExec opaque physical plan, which executes union over
 * its child output within enclave.
 */
case class EncryptedUnionExec(
    left: SparkPlan,
    right: SparkPlan)
  extends BinaryExecNode with OpaqueOperatorExec {
  import Utils.time

  override def output: Seq[Attribute] =
    left.output

  override def executeBlocked(): RDD[Block] = {
    var leftRDD = left.asInstanceOf[OpaqueOperatorExec].executeBlocked()
    var rightRDD = right.asInstanceOf[OpaqueOperatorExec].executeBlocked()
    Utils.ensureCached(leftRDD)
    time("Force left child of EncryptedUnionExec") { leftRDD.count }
    Utils.ensureCached(rightRDD)
    time("Force right child of EncryptedUnionExec") { rightRDD.count }

    // RA.initRA(leftRDD)

    val num_left_partitions = leftRDD.getNumPartitions
    val num_right_partitions = rightRDD.getNumPartitions
    if (num_left_partitions != num_right_partitions) {
      if (num_left_partitions > num_right_partitions) {
        leftRDD = leftRDD.coalesce(num_right_partitions)
      } else {
        rightRDD = rightRDD.coalesce(num_left_partitions)
      }
    }
    val unioned = leftRDD.zipPartitions(rightRDD) {
      (leftBlockIter, rightBlockIter) =>
        Iterator(Utils.concatEncryptedBlocks(leftBlockIter.toSeq ++ rightBlockIter.toSeq))
    }
    Utils.ensureCached(unioned)
    time("EncryptedUnionExec") { unioned.count }
    unioned
  }
}
