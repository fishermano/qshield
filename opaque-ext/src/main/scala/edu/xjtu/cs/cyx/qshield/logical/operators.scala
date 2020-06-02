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

package edu.xjtu.cs.cyx.qshield.logical

import edu.berkeley.cs.rise.opaque.execution.Block
import edu.berkeley.cs.rise.opaque.logical.OpaqueOperator

import org.apache.spark.rdd.RDD
import org.apache.spark.sql.catalyst.expressions.Attribute
import org.apache.spark.sql.catalyst.analysis.MultiInstanceRelation
import org.apache.spark.sql.catalyst.plans.logical.LogicalPlan
import org.apache.spark.sql.catalyst.plans.logical.UnaryNode
import org.apache.spark.sql.catalyst.plans.logical.LeafNode


/**
* @annotated by cyx
*
* define a PolicyApply operator (LogicalPlan)
*/
case class ACPolicyApply(child: LogicalPlan, tk: Array[Byte])
  extends UnaryNode with OpaqueOperator {

 override def output: Seq[Attribute] = child.output

}

case class ACPolicyAppliedEncryptedBlockRDD(
    output: Seq[Attribute],
    rdd: RDD[Block],
    tk: Array[Byte])
  extends LeafNode with MultiInstanceRelation with OpaqueOperator {

    override final def newInstance(): this.type = {
      ACPolicyAppliedEncryptedBlockRDD(output.map(_.newInstance()), rdd, tk).asInstanceOf[this.type]
    }

    override protected def stringArgs = Iterator(output)
}
