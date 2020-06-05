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
import edu.berkeley.cs.rise.opaque.execution.OpaqueOperatorExec
import edu.berkeley.cs.rise.opaque.Utils

import org.apache.spark.rdd.RDD
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
 * define QEncryptedFilterExec opaque physical plan, which executes filter over
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
        Block(enclave.Filter(eid, conditionSer, block.bytes))
      }
    }
  }
}

/**
 * @annotated by cyx
 *
 * define QEncryptedProjectExec opaque physical plan, which executes project over
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
        Block(enclave.Project(eid, projectListSer, block.bytes))
      }
    }
  }
}
