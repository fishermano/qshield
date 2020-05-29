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
  * The planner applies self-defined strategies to transform Opaque LogicalPlan to
  * Opaque PhysicalPlan (SparkPlan) that can be submitted to spark core for
  * computation over ciphertexts within enclave.
  */

package edu.berkeley.cs.rise.opaque

import org.apache.spark.sql.Strategy
import org.apache.spark.sql.catalyst.expressions.Alias
import org.apache.spark.sql.catalyst.expressions.Ascending
import org.apache.spark.sql.catalyst.expressions.Attribute
import org.apache.spark.sql.catalyst.expressions.Expression
import org.apache.spark.sql.catalyst.expressions.Literal
import org.apache.spark.sql.catalyst.expressions.NamedExpression
import org.apache.spark.sql.catalyst.expressions.SortOrder
import org.apache.spark.sql.catalyst.planning.ExtractEquiJoinKeys
import org.apache.spark.sql.catalyst.plans.logical.Join
import org.apache.spark.sql.catalyst.plans.logical.LogicalPlan
import org.apache.spark.sql.execution.SparkPlan

import edu.berkeley.cs.rise.opaque.execution._
import edu.berkeley.cs.rise.opaque.logical._

/**
 * @annotated by cyx
 *
 * self-defined strategy to transform opaque operator objects (LogicalPlan) to
 * (Iterator[SparkPlan])
 */
object OpaqueOperators extends Strategy {
  def apply(plan: LogicalPlan): Seq[SparkPlan] = plan match {

    // if the current opaque LogicalPlan is an instance of EncryptedProject,
    // return a Seq including an instance of EncryptedProjectExec as opaque
    // physical plans.
    case EncryptedProject(projectList, child) =>
      EncryptedProjectExec(projectList, planLater(child)) :: Nil

    // if the current opaque LogicalPlan is an instance of EncryptedFilter,
    // return a Seq including an instance of EncryptedFilterExec as opaque
    // physical plans.
    case EncryptedFilter(condition, child) =>
      EncryptedFilterExec(condition, planLater(child)) :: Nil

    // if the current opaque LogicalPlan is an instance of EncryptedSort,
    // return a Seq including an instance of EncryptedSortExec as opaque
    // physical plans.
    case EncryptedSort(order, child) =>
      EncryptedSortExec(order, planLater(child)) :: Nil

    // if the current opaque LogicalPlan is an instance of EncryptedJoin,
    // return a Seq including an instance of ([EncryptedFilterExec <-]
    //                                         EncryptedProjectExec
    //                                         <- EncryptedSortMergeJoinExec
    //                                         <- EncryptedProjectExec, EncryptedProjectExec)
    // as opaque physical plans.
    case EncryptedJoin(left, right, joinType, condition) =>
      Join(left, right, joinType, condition) match {
        case ExtractEquiJoinKeys(_, leftKeys, rightKeys, condition, _, _) =>
          val (leftProjSchema, leftKeysProj, tag) = tagForJoin(leftKeys, left.output, true)
          val (rightProjSchema, rightKeysProj, _) = tagForJoin(rightKeys, right.output, false)
          val leftProj = EncryptedProjectExec(leftProjSchema, planLater(left))
          val rightProj = EncryptedProjectExec(rightProjSchema, planLater(right))
          val unioned = EncryptedUnionExec(leftProj, rightProj)
          val sorted = EncryptedSortExec(sortForJoin(leftKeysProj, tag, unioned.output), unioned)
          val joined = EncryptedSortMergeJoinExec(
            joinType,
            leftKeysProj,
            rightKeysProj,
            leftProjSchema.map(_.toAttribute),
            rightProjSchema.map(_.toAttribute),
            (leftProjSchema ++ rightProjSchema).map(_.toAttribute),
            sorted)
          val tagsDropped = EncryptedProjectExec(dropTags(left.output, right.output), joined)
          val filtered = condition match {
            case Some(condition) => EncryptedFilterExec(condition, tagsDropped)
            case None => tagsDropped
          }
          filtered :: Nil
        case _ => Nil
      }

    // if the current opaque LogicalPlan is an instance of EncryptedAggregate,
    // return a Seq including an instance of EncryptedAggregateExec
    // as opaque physical plans.
    case a @ EncryptedAggregate(groupingExpressions, aggExpressions, child) =>
      EncryptedAggregateExec(groupingExpressions, aggExpressions, planLater(child)) :: Nil

    // if the current opaque LogicalPlan is an instance of EncryptedUnion,
    // return a Seq including an instance of EncryptedUnionExec
    // as opaque physical plans.
    case EncryptedUnion(left, right) =>
      EncryptedUnionExec(planLater(left), planLater(right)) :: Nil

    // if the current opaque LogicalPlan is an instance of Encrypt,
    // return a Seq including an instance of EncryptExec as opaque
    // physical plans.
    case Encrypt(child) =>
      EncryptExec(planLater(child)) :: Nil

    // if the current opaque LogicalPlan is an instance of EncryptedLocalRelation,
    // return a Seq including an instance of EncryptedLocalTableScanExec
    // as opaque physical plans.
    case EncryptedLocalRelation(output, plaintextData) =>
      EncryptedLocalTableScanExec(output, plaintextData) :: Nil

    // if the current opaque LogicalPlan is an instance of EncryptedBlockRDD,
    // return a Seq including an instance of EncryptedBlockRDDScanExec
    // as opaque physical plans.
    case EncryptedBlockRDD(output, rdd) =>
      EncryptedBlockRDDScanExec(output, rdd) :: Nil

    case _ => Nil
  }

  /**
   * @annotated by cyx
   *
   * add tags to join keys
   */
  private def tagForJoin(
      keys: Seq[Expression], input: Seq[Attribute], isLeft: Boolean)
    : (Seq[NamedExpression], Seq[NamedExpression], NamedExpression) = {
    val keysProj = keys.zipWithIndex.map { case (k, i) => Alias(k, "_" + i)() }
    val tag = Alias(Literal(if (isLeft) 0 else 1), "_tag")()
    (Seq(tag) ++ keysProj ++ input, keysProj.map(_.toAttribute), tag.toAttribute)
  }

  /**
   * @annotated by cyx
   *
   * add sort order for tags
   */
  private def sortForJoin(
      leftKeys: Seq[Expression], tag: Expression, input: Seq[Attribute]): Seq[SortOrder] =
    leftKeys.map(k => SortOrder(k, Ascending)) :+ SortOrder(tag, Ascending)

    /**
     * @annotated by cyx
     *
     * combine the left output with right output
     */
  private def dropTags(
      leftOutput: Seq[Attribute], rightOutput: Seq[Attribute]): Seq[NamedExpression] =
    leftOutput ++ rightOutput
}
