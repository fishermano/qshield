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

import edu.xjtu.cs.cyx.qshield.execution._
import edu.xjtu.cs.cyx.qshield.logical._

import edu.berkeley.cs.rise.opaque.logical._

import org.apache.spark.sql.Strategy
import org.apache.spark.sql.catalyst.plans.logical.LogicalPlan
import org.apache.spark.sql.catalyst.plans.logical.Join
import org.apache.spark.sql.catalyst.planning.ExtractEquiJoinKeys
import org.apache.spark.sql.catalyst.expressions.NamedExpression
import org.apache.spark.sql.catalyst.expressions.Expression
import org.apache.spark.sql.catalyst.expressions.Attribute
import org.apache.spark.sql.catalyst.expressions.SortOrder
import org.apache.spark.sql.catalyst.expressions.Alias
import org.apache.spark.sql.catalyst.expressions.Literal
import org.apache.spark.sql.catalyst.expressions.Ascending
import org.apache.spark.sql.execution.SparkPlan

object QShieldOperators extends Strategy {
  def apply(plan: LogicalPlan): Seq[SparkPlan] = plan match {

    case ACPolicyAppliedEncryptedBlockRDD(output, rdd, tk) =>
      ACPolicyAppliedEncryptedBlockRDDExec(output, rdd, tk) :: Nil

    case ResPreparedEncryptedBlockRDD(child) =>
      ResPreparedEncryptedBlockRDDExec(planLater(child)) :: Nil

    // if the current opaque LogicalPlan is an instance of EncryptedProject,
    // return a Seq including an instance of QEncryptedProjectExec as qshield
    // physical plans.
    case EncryptedProject(projectList, child) =>
      QEncryptedProjectExec(projectList, planLater(child)) :: Nil

    // if the current opaque LogicalPlan is an instance of EncryptedFilter,
    // return a Seq including an instance of QEncryptedFilterExec as qshield
    // physical plans.
    case EncryptedFilter(condition, child) =>
      QEncryptedFilterExec(condition, planLater(child)) :: Nil

    case EncryptedAggregate(groupingExpressions, aggExpressions, child) =>
      QEncryptedAggregateExec(groupingExpressions, aggExpressions, planLater(child)) :: Nil

    case EncryptedSort(order, child) =>
      QEncryptedSortExec(order, planLater(child)) :: Nil

    case EncryptedUnion(left, right) =>
      QEncryptedUnionExec(planLater(left), planLater(right)) :: Nil

    case EncryptedJoin(left, right, joinType, condition) =>
      Join(left, right, joinType, condition) match {
        case ExtractEquiJoinKeys(_, leftKeys, rightKeys, condition, _, _) =>
          val (leftProjSchema, leftKeysProj, tag) = tagForJoin(leftKeys, left.output, true)
          val (rightProjSchema, rightKeysProj, _) = tagForJoin(rightKeys, right.output, false)
          val leftProj = QEncryptedProjectExec(leftProjSchema, planLater(left))
          val rightProj = QEncryptedProjectExec(rightProjSchema, planLater(right))
          val unioned = QEncryptedUnionExec(leftProj, rightProj)
          val sorted = QEncryptedSortExec(sortForJoin(leftKeysProj, tag, unioned.output), unioned)
          val joined = QEncryptedSortMergeJoinExec(
            joinType,
            leftKeysProj,
            rightKeysProj,
            leftProjSchema.map(_.toAttribute),
            rightProjSchema.map(_.toAttribute),
            (leftProjSchema ++ rightProjSchema).map(_.toAttribute),
            sorted)
          val tagsDropped = QEncryptedProjectExec(dropTags(left.output, right.output), joined)
          val filtered = condition match {
            case Some(condition) => QEncryptedFilterExec(condition, tagsDropped)
            case None => tagsDropped
          }
          filtered :: Nil
        case _ => Nil
      }

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
