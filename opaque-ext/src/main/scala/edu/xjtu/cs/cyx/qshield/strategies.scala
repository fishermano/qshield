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
import org.apache.spark.sql.execution.SparkPlan

object QShieldOperators extends Strategy {
  def apply(plan: LogicalPlan): Seq[SparkPlan] = plan match {

    case ACPolicyAppliedEncryptedBlockRDD(output, rdd, tk) =>
      ACPolicyAppliedEncryptedBlockRDDExec(output, rdd, tk) :: Nil

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

    case _ => Nil
  }
}
