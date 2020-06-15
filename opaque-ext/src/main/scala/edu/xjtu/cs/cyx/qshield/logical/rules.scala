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

import edu.berkeley.cs.rise.opaque.logical.EncryptedBlockRDD
import edu.berkeley.cs.rise.opaque.logical.OpaqueOperator

import org.apache.spark.sql.catalyst.rules.Rule
import org.apache.spark.sql.catalyst.plans.logical.LogicalPlan

object ACPolicyApplyEncryptedBlockRDD extends Rule[LogicalPlan] {

  def apply(plan: LogicalPlan): LogicalPlan = plan transform {
    case ACPolicyApply(EncryptedBlockRDD(output, rdd), tk) =>
      ACPolicyAppliedEncryptedBlockRDD(output, rdd, tk)
  }
}

object ConvertToQShieldOperators extends Rule[LogicalPlan] {

  def isEncrypted(plan: LogicalPlan): Boolean = {
    plan.find {
      case _: OpaqueOperator => true
      case _ => false
    }.nonEmpty
  }

  def apply(plan: LogicalPlan): LogicalPlan = plan transformUp {

    case r @ ResPrepared(child) if isEncrypted(child) =>
      ResPreparedEncryptedBlockRDD(child.asInstanceOf[OpaqueOperator])
  }
}
