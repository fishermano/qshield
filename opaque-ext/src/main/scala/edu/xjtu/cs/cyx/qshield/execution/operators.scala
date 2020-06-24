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
  * define qshield physical plan (SparkPlan)
  */

package edu.xjtu.cs.cyx.qshield.execution

import edu.xjtu.cs.cyx.qshield.QShieldUtils

import edu.berkeley.cs.rise.opaque.execution.Block
import edu.berkeley.cs.rise.opaque.execution.UnaryExecNode
import edu.berkeley.cs.rise.opaque.execution.LeafExecNode
import edu.berkeley.cs.rise.opaque.execution.BinaryExecNode
import edu.berkeley.cs.rise.opaque.execution.OpaqueOperatorExec
import edu.berkeley.cs.rise.opaque.Utils

import org.apache.spark.rdd.RDD
import org.apache.spark.sql.catalyst.plans.JoinType
import org.apache.spark.sql.catalyst.expressions._
import org.apache.spark.sql.execution.SparkPlan


case class ACPolicyAppliedEncryptedBlockRDDExec(
    output: Seq[Attribute],
    rdd: RDD[Block],
    tk: Array[Byte])
  extends LeafExecNode with OpaqueOperatorExec {

    override def executeBlocked(): RDD[Block] = {
      timeOperator(rdd, "ACPolicyAppliedEncryptedBlockRDDExec") {
        initRDD => initRDD.map { block =>
          val (enclave, eid) = QShieldUtils.initEnclave()
          Block(enclave.ACPolicyApplied(eid, block.bytes, tk))
        }
      }
    }
}

case class ResPreparedEncryptedBlockRDDExec(child: SparkPlan)
  extends UnaryExecNode with OpaqueOperatorExec{

  override def output: Seq[Attribute] = child.output

  override def executeBlocked(): RDD[Block] = {
    timeOperator(child.asInstanceOf[OpaqueOperatorExec].executeBlocked(), "ResPreparedEncryptedBlockRDDExec") {
      childRDD => childRDD.map { block =>
        val (enclave, eid) = QShieldUtils.initEnclave()

        Block(enclave.ResPrepared(eid, block.bytes))
      }
    }
  }
}

/**
 * @annotated by cyx
 *
 * define QEncryptedFilterExec qshield physical plan, which executes filter over
 * its child output within enclave.
 */
case class QEncryptedFilterExec(condition: Expression, child: SparkPlan)
  extends UnaryExecNode with OpaqueOperatorExec{

  override def output: Seq[Attribute] = child.output

  override def executeBlocked(): RDD[Block] = {
    val conditionSer = Utils.serializeFilterExpression(condition, child.output)
    timeOperator(child.asInstanceOf[OpaqueOperatorExec].executeBlocked(), "EncryptedFilterExec") {
      childRDD => childRDD.map { block =>
        val (enclave, eid) = QShieldUtils.initEnclave()
        Block(enclave.QFilter(eid, conditionSer, block.bytes))
      }
    }
  }
}

/**
 * @annotated by cyx
 *
 * define QEncryptedProjectExec qshield physical plan, which executes project over
 * its child output within enclave.
 */
case class QEncryptedProjectExec(projectList: Seq[NamedExpression], child: SparkPlan)
  extends UnaryExecNode with OpaqueOperatorExec {

  override def output: Seq[Attribute] = projectList.map(_.toAttribute)

  override def executeBlocked(): RDD[Block] = {
    val projectListSer = Utils.serializeProjectList(projectList, child.output)
    timeOperator(child.asInstanceOf[OpaqueOperatorExec].executeBlocked(), "EncryptedProjectExec") {
      childRDD => childRDD.map { block =>
        val (enclave, eid) = QShieldUtils.initEnclave()
        Block(enclave.QProject(eid, projectListSer, block.bytes))
      }
    }
  }
}

/**
 * @annotated by cyx
 *
 * define QEncryptedAggregateExec opaque qshield plan, which executes aggregate over
 * its child output within enclave.
 */
case class QEncryptedAggregateExec(
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
      "QEncryptedAggregateExec") { childRDD =>

        val (firstRows, lastGroups, lastRows) = childRDD.map { block =>
          val (enclave, eid) = QShieldUtils.initEnclave()
          val (firstRow, lastGroup, lastRow) = enclave.QAggregateStep1(eid, aggExprSer, block.bytes)
          (Block(firstRow), Block(lastGroup), Block(lastRow))
        }.collect.unzip3

        // Send first row to previous partition and last group to next partition
        val shiftedFirstRows = firstRows.drop(1) :+ QShieldUtils.emptyBlock
        val shiftedLastGroups = QShieldUtils.emptyBlock +: lastGroups.dropRight(1)
        val shiftedLastRows = QShieldUtils.emptyBlock +: lastRows.dropRight(1)
        val shifted = (shiftedFirstRows, shiftedLastGroups, shiftedLastRows).zipped.toSeq
        assert(shifted.size == childRDD.partitions.length)
        val shiftedRDD = sparkContext.parallelize(shifted, childRDD.partitions.length)

        childRDD.zipPartitions(shiftedRDD) { (blockIter, boundaryIter) =>
          (blockIter.toSeq, boundaryIter.toSeq) match {
            case (Seq(block), Seq(Tuple3(
              nextPartitionFirstRow, prevPartitionLastGroup, prevPartitionLastRow))) =>
              val (enclave, eid) = QShieldUtils.initEnclave()
              Iterator(Block(enclave.QAggregateStep2(
                eid, aggExprSer, block.bytes, nextPartitionFirstRow.bytes, prevPartitionLastGroup.bytes, prevPartitionLastRow.bytes)))
          }
        }
      }
  }
}

/**
 * @annotated by cyx
 *
 * define QEncryptedSortExec qshield physical plan, which executes sort over
 * its child output within enclave.
 */
case class QEncryptedSortExec(order: Seq[SortOrder], child: SparkPlan)
  extends UnaryExecNode with OpaqueOperatorExec{

  override def output: Seq[Attribute] = child.output

  override def executeBlocked(): RDD[Block] = {
    val orderSer = Utils.serializeSortOrder(order, child.output)
    QEncryptedSortExec.sort(child.asInstanceOf[OpaqueOperatorExec].executeBlocked(), orderSer)
  }
}

object QEncryptedSortExec {
  import Utils.time

  def sort(childRDD: RDD[Block], orderSer: Array[Byte]): RDD[Block] = {
    Utils.ensureCached(childRDD)
    time("force child of QEncryptedSort") {childRDD.count}

    time("sort"){
      val numPartitions = childRDD.partitions.length
      val result =
        if (numPartitions <= 1){
          childRDD.map { block =>
            val (enclave, eid) = QShieldUtils.initEnclave()
            val sortedRows = enclave.QExternalSort(eid, orderSer, block.bytes)
            Block(sortedRows)
          }
        }else{
          // Collect a sample of the input rows
          val sampled = time("sort - QSample"){
            QShieldUtils.concatQEncryptedBlocks(childRDD.map { block =>
                          val (enclave, eid) = QShieldUtils.initEnclave()
                          val sampledBlock = enclave.QSample(eid, block.bytes)
                          Block(sampledBlock)
                        }.collect)
          }

          // Find range boundaries parceled out to a single worker
          val boundaries = time("sort - QFindRangeBounds"){
            childRDD.context.parallelize(Array(sampled.bytes), 1).map { sampledBytes =>
              val (enclave, eid) = QShieldUtils.initEnclave()
              enclave.QFindRangeBounds(eid, orderSer, numPartitions, sampledBytes)
            }.collect.head
          }

          // Broadcast the range boundaries and use them to partition the input
          childRDD.flatMap { block =>
            val (enclave, eid) = QShieldUtils.initEnclave()
            val partitions = enclave.QPartitionForSort(
              eid, orderSer, numPartitions, block.bytes, boundaries)
            partitions.zipWithIndex.map {
              case (partition, i) => (i, Block(partition))
            }
          }
          //Shuffle the input to achieve range partitioning and sort locally
            .groupByKey(numPartitions).map{
              case (i, blocks) =>
                val blockRuns = QShieldUtils.concatQEncryptedBlocks(blocks.toSeq)
                val (enclave, eid) = QShieldUtils.initEnclave()
                val concatedBlock = enclave.QConcatBlocks(eid, blockRuns.bytes)
                Block(enclave.QExternalSort(eid, orderSer, concatedBlock))
            }
        }
      Utils.ensureCached(result)
      result.count()
      result
    }
  }
}

/**
 * @annotated by cyx
 *
 * define QEncryptedSortMergeJoinExec opaque physical plan, which executes sort merge join over
 * its children output within enclave.
 */
case class QEncryptedSortMergeJoinExec(
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
      "QEncryptedSortMergeJoinExec"){ childRDD =>

      val lastPrimaryRows = childRDD.map { block =>
        val (enclave, eid) = QShieldUtils.initEnclave()
        Block(enclave.QScanCollectLastPrimary(eid, joinExprSer, block.bytes))
      }.collect

      val shifted = QShieldUtils.emptyBlock +: lastPrimaryRows.dropRight(1)
      assert(shifted.size == childRDD.partitions.length)
      val processedJoinRowsRDD =
        sparkContext.parallelize(shifted, childRDD.partitions.length)

      childRDD.zipPartitions(processedJoinRowsRDD) { (blockIter, joinRowIter) =>
        (blockIter.toSeq, joinRowIter.toSeq) match {
          case (Seq(block), Seq(joinRow)) =>
            val (enclave, eid) = QShieldUtils.initEnclave()
            val sorted = enclave.QSortMergeJoin(
              eid, joinExprSer, block.bytes, joinRow.bytes)
            Iterator(Block(sorted))
        }
      }

    }
  }
}

/**
 * @annotated by cyx
 *
 * define QEncryptedUnionExec opaque physical plan, which executes union over
 * its child output within enclave.
 */
case class QEncryptedUnionExec(
    left: SparkPlan,
    right: SparkPlan)
  extends BinaryExecNode with OpaqueOperatorExec {

  import Utils.time

  override def output: Seq[Attribute] = left.output

  override def executeBlocked(): RDD[Block] = {
    var leftRDD = left.asInstanceOf[OpaqueOperatorExec].executeBlocked()
    var rightRDD = right.asInstanceOf[OpaqueOperatorExec].executeBlocked()
    Utils.ensureCached(leftRDD)
    time("Force left child of QEncryptedUnionExec") { leftRDD.count }
    Utils.ensureCached(rightRDD)
    time("Force right child of QEncryptedUnionExec") { rightRDD.count }

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
        val blockRuns = QShieldUtils.concatQEncryptedBlocks(leftBlockIter.toSeq ++
                                                            rightBlockIter.toSeq)
        val (enclave, eid) = QShieldUtils.initEnclave()
        val concatedBlock = enclave.QConcatBlocks(eid, blockRuns.bytes)
        Iterator(Block(concatedBlock))
    }
    Utils.ensureCached(unioned)
    time("QEncryptedUnionExec") {unioned.count}
    unioned
  }
}
