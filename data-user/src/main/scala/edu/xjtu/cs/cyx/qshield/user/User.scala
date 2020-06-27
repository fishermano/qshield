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

package edu.xjtu.cs.cyx.qshield.user

import scalaj.http._

import edu.xjtu.cs.cyx.qshield.qix.QToken
import edu.xjtu.cs.cyx.qshield.qix.QEncryptedToken

import java.nio.ByteBuffer
import java.security.SecureRandom
import com.google.flatbuffers.FlatBufferBuilder

import javax.crypto._
import javax.crypto.spec.GCMParameterSpec
import javax.crypto.spec.SecretKeySpec

object User {

  // define some parameters for AES[GCM] encryption scheme
  final val GCM_IV_LENGTH = 12
  final val GCM_KEY_LENGTH = 16
  final val GCM_TAG_LENGTH = 16

  /**
   * Symmetric key used to encrypt token.
   */
  val tokenKey: Array[Byte] = "Opaque devel key".getBytes("UTF-8")
  assert(tokenKey.size == GCM_KEY_LENGTH)

  // encryption data of Bytes with AES[GCM]
  def encrypt(data: Array[Byte]): Array[Byte] = {
    val random = SecureRandom.getInstance("SHA1PRNG")
    val cipherKey = new SecretKeySpec(tokenKey, "AES")
    val iv = new Array[Byte](GCM_IV_LENGTH)
    random.nextBytes(iv)
    val spec = new GCMParameterSpec(GCM_TAG_LENGTH * 8, iv)
    val cipher = Cipher.getInstance("AES/GCM/NoPadding", "SunJCE")
    cipher.init(Cipher.ENCRYPT_MODE, cipherKey, spec)
    val cipherText = cipher.doFinal(data)
    iv ++ cipherText
  }


  def main(args: Array[String]): Unit = {

    println("Welcome to QShield User Module !!!")

    val builder = new FlatBufferBuilder
    val cipherBuilder = new FlatBufferBuilder

    val cVal: Long = 10002011
    val wVal: Long = 20
    val skBVal: Array[Byte] = "Opaque devel key".getBytes("UTF-8")

    val tkOffset = QToken.createQToken(builder, cVal, wVal,
            QToken.createSkBVector(builder, skBVal))

    builder.finish(tkOffset)
    val tk_plain = builder.sizedByteArray()

    val tk_cipher = encrypt(tk_plain)
    val tk_cipherOffset = QEncryptedToken.createQEncryptedToken(cipherBuilder,
            QEncryptedToken.createEncTkVector(cipherBuilder, tk_cipher))
    cipherBuilder.finish(tk_cipherOffset)
    val tk = cipherBuilder.sizedByteArray()

    val res1 = Http("http://localhost:9090/qshield/query")
      .charset("UTF-8")
      .postForm(Seq("st" -> "select pageURL, pageRank from RANKINGS where pageRank < 20", "p" -> "test"))
      .postMulti(MultiPart("tk", "", "", tk))
      .asString.body

    val res2 = Http("http://localhost:9090/qshield/query")
      .charset("UTF-8")
      .postForm(Seq("st" -> "select sourceIP, visitDate from USERVISITS", "p" -> "test"))
      .postMulti(MultiPart("tk", "", "", tk))
      .asString.body

    val res3 = Http("http://localhost:9090/qshield/selector")
      .charset("UTF-8")
      .postForm(Seq("st" -> "sourceIP,duration", "t" -> "USERVISITS"))
      .postMulti(MultiPart("tk", "", "", tk))
      .asString.body

    val res4 = Http("http://localhost:9090/qshield/filter")
      .charset("UTF-8")
      .postForm(Seq("st" -> "pageRank<20", "t" -> "RANKINGS"))
      .postMulti(MultiPart("tk", "", "", tk))
      .asString.body

    val res5 = Http("http://localhost:9090/qshield/sorter")
      .charset("UTF-8")
      .postForm(Seq("st" -> "pageRank", "t" -> "RANKINGS", "asc" -> "False"))
      .postMulti(MultiPart("tk", "", "", tk))
      .asString.body

    val res6 = Http("http://localhost:9090/qshield/joiner")
      .charset("UTF-8")
      .postForm(Seq("st" -> "pageURL", "t1" -> "RANKINGS", "t2" -> "RANKINGS", "mode" -> "inner"))
      .postMulti(MultiPart("tk", "", "", tk))
      .asString.body

    //println(res1)
    //println(res2)
    //println(res3)
    //println(res4)
    //println(res5)
    println(res6)
  }

}
