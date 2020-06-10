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

package edu.xjtu.cs.cyx.qshield.owner

import edu.berkeley.cs.rise.opaque.tuix

import ch.jodersky.jni.nativeLoader

import com.google.flatbuffers.FlatBufferBuilder
import scala.collection.mutable.ArrayBuilder

import org.apache.spark.sql.SparkSession
import org.apache.spark.sql.types._
import org.apache.spark.unsafe.types.CalendarInterval
import org.apache.spark.unsafe.types.UTF8String
import org.apache.spark.sql.catalyst.util.ArrayData
import org.apache.spark.sql.catalyst.util.MapData
import org.apache.spark.storage.StorageLevel
import org.apache.spark.sql.DataFrame
import org.apache.spark.sql.Dataset
import org.apache.spark.sql.Row

case class Block(bytes: Array[Byte]) extends Serializable

@nativeLoader("ra_jni")
class QSP extends java.io.Serializable {

  // define max size of a Block
  val MaxBlockSize = 1000

  def QOutsource(spark: SparkSession, srcFilePath: String,
                  tableName: String, schema: StructType,
                  numPartitions: Int): String = {

    def positions(length: Long, numSlices: Int): Iterator[(Int, Int)] = {
      (0 until numSlices).iterator.map { i =>
        val start = ((i * length) / numSlices).toInt
        val end = (((i + 1) * length) / numSlices).toInt
        (start, end)
      }
    }

    def ensureCached[T](ds: Dataset[T]): Dataset[T] = {
      if (ds.storageLevel == StorageLevel.NONE) {
        ds.persist(StorageLevel.MEMORY_ONLY)
      } else {
        ds
      }
    }

    // convert a value with scala data type to a flatbuffer field
    def flatbuffersCreateField(
        builder: FlatBufferBuilder, value: Any, dataType: DataType, isNull: Boolean): Int = {
      (value, dataType) match {
        case (b: Boolean, BooleanType) =>
          tuix.Field.createField(
            builder,
            tuix.FieldUnion.BooleanField,
            tuix.BooleanField.createBooleanField(builder, b),
            isNull)
        case (null, BooleanType) =>
          tuix.Field.createField(
            builder,
            tuix.FieldUnion.BooleanField,
            tuix.BooleanField.createBooleanField(builder, false),
            isNull)
        case (x: Int, IntegerType) =>
          tuix.Field.createField(
            builder,
            tuix.FieldUnion.IntegerField,
            tuix.IntegerField.createIntegerField(builder, x),
            isNull)
        case (null, IntegerType) =>
          tuix.Field.createField(
            builder,
            tuix.FieldUnion.IntegerField,
            tuix.IntegerField.createIntegerField(builder, 0),
            isNull)
        case (x: Long, LongType) =>
          tuix.Field.createField(
            builder,
            tuix.FieldUnion.LongField,
            tuix.LongField.createLongField(builder, x),
            isNull)
        case (null, LongType) =>
          tuix.Field.createField(
            builder,
            tuix.FieldUnion.LongField,
            tuix.LongField.createLongField(builder, 0L),
            isNull)
        case (x: Float, FloatType) =>
          tuix.Field.createField(
            builder,
            tuix.FieldUnion.FloatField,
            tuix.FloatField.createFloatField(builder, x),
            isNull)
        case (null, FloatType) =>
          tuix.Field.createField(
            builder,
            tuix.FieldUnion.FloatField,
            tuix.FloatField.createFloatField(builder, 0),
            isNull)
        case (x: Double, DoubleType) =>
          tuix.Field.createField(
            builder,
            tuix.FieldUnion.DoubleField,
            tuix.DoubleField.createDoubleField(builder, x),
            isNull)
        case (null, DoubleType) =>
          tuix.Field.createField(
            builder,
            tuix.FieldUnion.DoubleField,
            tuix.DoubleField.createDoubleField(builder, 0.0),
            isNull)
        case (x: Int, DateType) =>
          tuix.Field.createField(
            builder,
            tuix.FieldUnion.DateField,
            tuix.DateField.createDateField(builder, x),
            isNull)
        case (null, DateType) =>
          tuix.Field.createField(
            builder,
            tuix.FieldUnion.DateField,
            tuix.DateField.createDateField(builder, 0),
            isNull)
        case (x: Array[Byte], BinaryType) =>
          val length = x.size
          tuix.Field.createField(
            builder,
            tuix.FieldUnion.BinaryField,
            tuix.BinaryField.createBinaryField(
              builder,
              tuix.BinaryField.createValueVector(builder, x),
              length),
            isNull)
        case (null, BinaryType) =>
          tuix.Field.createField(
            builder,
            tuix.FieldUnion.BinaryField,
            tuix.BinaryField.createBinaryField(
              builder,
              tuix.BinaryField.createValueVector(builder, Array.empty),
              0),
            isNull)
        case (x: Byte, ByteType) =>
          tuix.Field.createField(
            builder,
            tuix.FieldUnion.ByteField,
            tuix.ByteField.createByteField(builder, x),
            isNull)
        case (null, ByteType) =>
          tuix.Field.createField(
            builder,
            tuix.FieldUnion.ByteField,
            tuix.ByteField.createByteField(builder, 0),
            isNull)
        case (x: CalendarInterval, CalendarIntervalType) =>
          val months = x.months
          val microseconds = x.microseconds
          tuix.Field.createField(
            builder,
            tuix.FieldUnion.CalendarIntervalField,
            tuix.CalendarIntervalField.createCalendarIntervalField(builder, months, microseconds),
            isNull)
        case (null, CalendarIntervalType) =>
          tuix.Field.createField(
            builder,
            tuix.FieldUnion.CalendarIntervalField,
            tuix.CalendarIntervalField.createCalendarIntervalField(builder, 0, 0L),
            isNull)
        case (x: Byte, NullType) =>
          tuix.Field.createField(
            builder,
            tuix.FieldUnion.NullField,
            tuix.NullField.createNullField(builder, x),
            isNull)
        case (null, NullType) =>
          tuix.Field.createField(
            builder,
            tuix.FieldUnion.NullField,
            tuix.NullField.createNullField(builder, 0),
            isNull)
        case (x: Short, ShortType) =>
          tuix.Field.createField(
            builder,
            tuix.FieldUnion.ShortField,
            tuix.ShortField.createShortField(builder, x),
            isNull)
        case (null, ShortType) =>
          tuix.Field.createField(
            builder,
            tuix.FieldUnion.ShortField,
            tuix.ShortField.createShortField(builder, 0),
            isNull)
        case (x: Long, TimestampType) =>
          tuix.Field.createField(
            builder,
            tuix.FieldUnion.TimestampField,
            tuix.TimestampField.createTimestampField(builder, x),
            isNull)
        case (null, TimestampType) =>
          tuix.Field.createField(
            builder,
            tuix.FieldUnion.TimestampField,
            tuix.TimestampField.createTimestampField(builder, 0),
            isNull)
        case (x: ArrayData, ArrayType(elementType, containsNull)) =>
          // Iterate through each element in x and turn it into Field type
          val fieldsArray = new ArrayBuilder.ofInt
          for (i <- 0 until x.numElements) {
            val field = flatbuffersCreateField(builder, x.get(i, elementType), elementType, isNull)
            fieldsArray += field
          }
          tuix.Field.createField(
            builder,
            tuix.FieldUnion.ArrayField,
            tuix.ArrayField.createArrayField(
              builder,
              tuix.ArrayField.createValueVector(builder, fieldsArray.result)),
            isNull)
        case (null, ArrayType(elementType, containsNull)) =>
          tuix.Field.createField(
            builder,
            tuix.FieldUnion.ArrayField,
            tuix.ArrayField.createArrayField(
              builder,
              tuix.ArrayField.createValueVector(builder, Array.empty)),
            isNull)
        case (x: MapData, MapType(keyType, valueType, valueContainsNull)) =>
          var keys = new ArrayBuilder.ofInt()
          var values = new ArrayBuilder.ofInt()
          for (i <- 0 until x.numElements) {
            keys += flatbuffersCreateField(
              builder, x.keyArray.get(i, keyType), keyType, isNull)
            values += flatbuffersCreateField(
              builder, x.valueArray.get(i, valueType), valueType, isNull)
          }
          tuix.Field.createField(
            builder,
            tuix.FieldUnion.MapField,
            tuix.MapField.createMapField(
              builder,
              tuix.MapField.createKeysVector(builder, keys.result),
              tuix.MapField.createValuesVector(builder, values.result)),
            isNull)
        case (null, MapType(keyType, valueType, valueContainsNull)) =>
          tuix.Field.createField(
            builder,
            tuix.FieldUnion.MapField,
            tuix.MapField.createMapField(
              builder,
              tuix.MapField.createKeysVector(builder, Array.empty),
              tuix.MapField.createValuesVector(builder, Array.empty)),
            isNull)
        case (s: UTF8String, StringType) =>
          val utf8 = s.getBytes()
          tuix.Field.createField(
            builder,
            tuix.FieldUnion.StringField,
            tuix.StringField.createStringField(
              builder,
              // TODO: pad strings to upper bound for obliviousness
              tuix.StringField.createValueVector(builder, utf8),
              utf8.length),
            isNull)
        case (null, StringType) =>
          tuix.Field.createField(
            builder,
            tuix.FieldUnion.StringField,
            tuix.StringField.createStringField(
              builder,
              // TODO: pad strings to upper bound for obliviousness
              tuix.StringField.createValueVector(builder, Array.empty),
              0),
            isNull)
      }
    }

    def encryptRowsFlatbuffers(rows: Seq[Row], types: Seq[DataType]): Block = {

      // For the encrypted blocks
      val builder2 = new FlatBufferBuilder
      val encryptedBlockOffsets = ArrayBuilder.make[Int]

      // 1. Serialize the rows as plaintext using tuix.Rows
      var builder = new FlatBufferBuilder
      var rowsOffsets = ArrayBuilder.make[Int]

      def finishBlock(): Unit = {
        val rowsOffsetsArray = rowsOffsets.result
        builder.finish(
          tuix.Rows.createRows(
            builder,
            tuix.Rows.createRowsVector(
              builder,
              rowsOffsetsArray)))
        val plaintext = builder.sizedByteArray()

        //2. Encrypt the row data and put it into a tuix.EncryptedBlock
        val ciphertext = QEncrypt(plaintext)

        encryptedBlockOffsets += tuix.EncryptedBlock.createEncryptedBlock(
          builder2,
          rowsOffsetsArray.size,
          tuix.EncryptedBlock.createEncRowsVector(builder2, ciphertext))

        builder = new FlatBufferBuilder
        rowsOffsets = ArrayBuilder.make[Int]
      }

      for(row <- rows){
        rowsOffsets += tuix.Row.createRow(
          builder,
          tuix.Row.createFieldValuesVector(
            builder,
            row.toSeq.zip(types).zipWithIndex.map{
              case ((value, dataType), i) =>
                flatbuffersCreateField(builder, value, dataType, row.isNullAt(i))
            }.toArray),
          false)
        if(builder.offset() > MaxBlockSize){
          finishBlock()
        }
      }
      if(builder.offset() > 0){
        finishBlock()
      }

      //3. Put the tuix.EncryptedBock objects into a tuix.encryptedBlocks
      builder2.finish(
        tuix.EncryptedBlocks.createEncryptedBlocks(
          builder2,
          tuix.EncryptedBlocks.createBlocksVector(
            builder2,
            encryptedBlockOffsets.result)))
      val encryptedBlockBytes = builder2.sizedByteArray()

      //4. Wrap the serialized tuix.EncryptedBlocks in a Scala Block objects
      Block(encryptedBlockBytes)
    }

    val df = spark.read.schema(schema).csv(srcFilePath).repartition(numPartitions)
    val dfCached = ensureCached(df)
    dfCached.queryExecution.executedPlan match {
      case p => p.execute()
    }

    val rows = dfCached.collect
    val slicedPlaintextData: Seq[Seq[Row]] =
      positions(rows.length, spark.sparkContext.defaultParallelism).map{
        case (start, end) => rows.slice(start, end).toSeq
      }.toSeq

    val encryptedPartitions: Seq[Block] =
      slicedPlaintextData.map( slice =>
        encryptRowsFlatbuffers(slice, schema.map(_.dataType))
      )

    srcFilePath
  }

  // Remote attestation, master side
  @native def QInit(sharedKey: Array[Byte], param: String, intelCert: String): Unit
  @native def QSPProcMsg0(msg0Input: Array[Byte]): Unit
  @native def QSPProcMsg1(msg1Input: Array[Byte]): Array[Byte]
  @native def QSPProcMsg3(msg3Input: Array[Byte]): Array[Byte]
  @native def QEncrypt(pt: Array[Byte]): Array[Byte]

}
