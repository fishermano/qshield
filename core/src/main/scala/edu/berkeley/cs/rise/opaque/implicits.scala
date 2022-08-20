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

package edu.berkeley.cs.rise.opaque

import org.apache.spark.sql.Dataset
import org.apache.spark.sql.OpaqueDatasetFunctions

/**
 * @annotated by cyx
 *
 * define a function to implicitly convert Dataset object to OpaqueDatasetFunctions,
 * such that the Dataset object supports the additional encrypted() method defined
 * in OpaqueDatasetFunctions.
 */

// scalastyle:off
// Disable style checker so "implicits" object can start with lowercase i
object implicits {
  import scala.language.implicitConversions

  implicit def datasetToOpaqueDatasetFunctions[T](ds: Dataset[T]): OpaqueDatasetFunctions[T] =
    new OpaqueDatasetFunctions[T](ds)
}
// scalastyle:on
