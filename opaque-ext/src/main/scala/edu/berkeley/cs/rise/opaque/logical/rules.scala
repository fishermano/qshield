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
  * The Optimizer apply self-defined rules to transform Spark LogicalPlan to
  * Opaque LogicalPlan that supports computation over ciphertexts within
  * enclave.
  */

package edu.berkeley.cs.rise.opaque.logical

import edu.berkeley.cs.rise.opaque.EncryptedScan
import edu.berkeley.cs.rise.opaque.Utils
import edu.berkeley.cs.rise.opaque.execution.OpaqueOperatorExec
import org.apache.spark.sql.InMemoryRelationMatcher
import org.apache.spark.sql.UndoCollapseProject
import org.apache.spark.sql.catalyst.expressions.And
import org.apache.spark.sql.catalyst.expressions.Ascending
import org.apache.spark.sql.catalyst.expressions.IsNotNull
import org.apache.spark.sql.catalyst.expressions.SortOrder
import org.apache.spark.sql.catalyst.plans.logical._
import org.apache.spark.sql.catalyst.rules.Rule
import org.apache.spark.sql.execution.SparkPlan
import org.apache.spark.sql.execution.datasources.LogicalRelation

/**
 * @annotated by cyx
 *
 * transform Encrypt objects to EncryptedLocalRelation objects
 *
 * Encrypt @ ./operator.scala extends UnaryNode with OpaqueOperator
 * EncryptedLocalRelation @ ./operator.scala extends LeafNode with OpaqueOperator
 * OpqueOperator @ ./operator.scala extends LogicalPlan
 */
object EncryptLocalRelation extends Rule[LogicalPlan] {
  def apply(plan: LogicalPlan): LogicalPlan = plan transform {
    case Encrypt(LocalRelation(output, data, false)) =>
      EncryptedLocalRelation(output, data)
  }
}


/**
 * @annotated by cyx
 *
 * self-define rules to transform Spark SQL operator objects (LogicalPlan)
 * to opaque operator objects (LogicalPlan)
 */
object ConvertToOpaqueOperators extends Rule[LogicalPlan] {

  // judge whether the current logical plan is encrypted
  // if the current logical plan is an instance of OpaqueOperator, return true.
  def isEncrypted(plan: LogicalPlan): Boolean = {
    plan.find {
      case _: OpaqueOperator => true
      case _ => false
    }.nonEmpty
  }

  // judge whether the current spark (physical) plan is encrypted
  // if the current spark plan is an instance of OpaqueOperatorExec, return true.
  def isEncrypted(plan: SparkPlan): Boolean = {
    plan.find {
      case _: OpaqueOperatorExec => true
      case _ => false
    }.nonEmpty
  }

  def apply(plan: LogicalPlan): LogicalPlan = plan transformUp {

    // if the current spark sql LogicalPlan is an instance of LogicalRelation
    // that is created from EncryptedScan,
    // return an instance of EncryptedBlockRDD as opaque LogicalPlan
    case l @ LogicalRelation(baseRelation: EncryptedScan, _, _, false) =>
      EncryptedBlockRDD(l.output, baseRelation.buildBlockedScan())

    // if the current spark sql LogicalPlan is an instance of Project
    // and its child is encrypted,
    // return an instance of EncryptedProject as opaque LogicalPlan
    case p @ Project(projectList, child) if isEncrypted(child) =>
      EncryptedProject(projectList, child.asInstanceOf[OpaqueOperator])

    // if the current spark sql LogicalPlan is an instance of Filter
    // and its child is encrypted,
    // return an instance of EncryptedFilter as opaque LogicalPlan
    // We don't support null values yet, so there's no point in checking whether the output of an
    // encrypted operator is null
    case p @ Filter(And(IsNotNull(_), IsNotNull(_)), child) if isEncrypted(child) =>
      child
    case p @ Filter(IsNotNull(_), child) if isEncrypted(child) =>
      child

    case p @ Filter(condition, child) if isEncrypted(child) =>
      EncryptedFilter(condition, child.asInstanceOf[OpaqueOperator])

    // if the current spark sql LogicalPlan is an instance of Sort
    // and its child is encrypted,
    // return an instance of EncryptedSort as opaque LogicalPlan
    case p @ Sort(order, true, child) if isEncrypted(child) =>
      EncryptedSort(order, child.asInstanceOf[OpaqueOperator])

    // if the current spark sql LogicalPlan is an instance of Join
    // and it is encrypted,
    // return an instance of EncryptedJoin as opaque LogicalPlan
    case p @ Join(left, right, joinType, condition) if isEncrypted(p) =>
      EncryptedJoin(
        left.asInstanceOf[OpaqueOperator], right.asInstanceOf[OpaqueOperator], joinType, condition)

    // if the current spark sql LogicalPlan is an instance of Aggregate
    // and it is encrypted,
    // return instances of EncryptedProject<-EncryptedAggregate<-EncryptedSort
    // or EncryptedAggregate<-EncryptedSort as opaque LogicalPlan
    case p @ Aggregate(groupingExprs, aggExprs, child) if isEncrypted(p) =>
      UndoCollapseProject.separateProjectAndAgg(p) match {
        case Some((projectExprs, aggExprs)) =>
          EncryptedProject(
            projectExprs,
            EncryptedAggregate(
              groupingExprs, aggExprs,
              EncryptedSort(
                groupingExprs.map(e => SortOrder(e, Ascending)),
                child.asInstanceOf[OpaqueOperator])))
        case None =>
          EncryptedAggregate(
            groupingExprs, aggExprs,
            EncryptedSort(
              groupingExprs.map(e => SortOrder(e, Ascending)),
              child.asInstanceOf[OpaqueOperator]))
      }

    // if the current spark sql LogicalPlan is an instance of Union
    // and it is encrypted,
    // return an instance of EncryptedUnion as opaque LogicalPlan
    case p @ Union(Seq(left, right)) if isEncrypted(p) =>
      EncryptedUnion(left.asInstanceOf[OpaqueOperator], right.asInstanceOf[OpaqueOperator])

    // if the current spark sql LogicalPlan is an instance of InMemoryRelation
    // and its child is encrypted,
    // return an instance of EncryptedBlockRDD as opaque LogicalPlan
    case InMemoryRelationMatcher(output, storageLevel, child) if isEncrypted(child) =>
      EncryptedBlockRDD(
        output,
        Utils.ensureCached(
          child.asInstanceOf[OpaqueOperatorExec].executeBlocked(),
          storageLevel))
  }
}
