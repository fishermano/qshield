#include "QFilter.h"

#include "ExpressionEvaluation.h"
#include "QFlatbuffersReaders.h"
#include "QFlatbuffersWriters.h"

#include <string.h>

#include "qdebug.h"

using namespace edu::berkeley::cs::rise::opaque;
using namespace edu::xjtu::cs::cyx::qshield;

void qfilter(uint8_t *condition, size_t condition_length,
            uint8_t *input_rows, size_t input_rows_length,
            uint8_t **output_rows, size_t *output_rows_length){

  BufferRefView<tuix::FilterExpr> condition_buf(condition, condition_length);
  condition_buf.verify();

  FlatbuffersExpressionEvaluator condition_eval(condition_buf.root()->condition());

  QRowReader row_r(BufferRefView<qix::QEncryptedBlocks>(input_rows, input_rows_length));
  QRowWriter row_w;
  #if QSHIELD_TP
    flatbuffers::FlatBufferBuilder meta_builder;
    const flatbuffers::Offset<qix::QMeta> meta_new = row_w.unary_update_meta(row_r.meta(),
                                                                              false,
                                                                              "qfilter",
                                                                              meta_builder);

    meta_builder.Finish(meta_new);
    row_w.set_meta(flatbuffers::GetRoot<qix::QMeta>(meta_builder.GetBufferPointer()));
    meta_builder.Clear();
  #endif

  while(row_r.has_next()){
    const tuix::Row *row = row_r.next();
    const tuix::Field *condition_result = condition_eval.eval(row);

    if(condition_result->value_type() != tuix::FieldUnion_BooleanField){
      throw std::runtime_error(
        std::string("QFilter expression expected to return BooleanField, instead returned ")
        + std::string(tuix::EnumNameFieldUnion(condition_result->value_type())));
    }
    if(condition_result->is_null()){
      throw std::runtime_error("QFilter expression returned null");
    }

    bool keep_row = static_cast<const tuix::BooleanField *>(condition_result->value())->value();
    if(keep_row){
      row_w.append(row);
    }
  }

  row_w.output_buffer(output_rows, output_rows_length);

}
