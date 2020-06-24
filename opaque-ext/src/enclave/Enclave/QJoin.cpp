#include "QJoin.h"

#include "ExpressionEvaluation.h"
#include "QFlatbuffersReaders.h"
#include "QFlatbuffersWriters.h"
#include "common.h"

void qscan_collect_last_primary(
  uint8_t *join_expr, size_t join_expr_length,
  uint8_t *input_rows, size_t input_rows_length,
  uint8_t **output_rows, size_t *output_rows_length){

  FlatbuffersJoinExprEvaluator join_expr_eval(join_expr, join_expr_length);
  QRowReader r(BufferRefView<qix::QEncryptedBlocks>(input_rows, input_rows_length));
  QRowWriter w;

  FlatbuffersTemporaryRow last_primary;

  while (r.has_next()){
    const tuix::Row *row = r.next();
    if(join_expr_eval.is_primary(row)){
      if(!last_primary.get() || !join_expr_eval.is_same_group(last_primary.get(), row)){
        w.clear();
        last_primary.set(row);
      }

      w.append(row);
    } else {
      w.clear();
      last_primary.set(nullptr);
    }
  }
  w.set_meta(r.meta());
  w.output_buffer(output_rows, output_rows_length);

}

void qsort_merge_join(
    uint8_t *join_expr, size_t join_expr_length,
    uint8_t *input_rows, size_t input_rows_length,
    uint8_t *join_row, size_t join_row_length,
    uint8_t **output_rows, size_t *output_rows_length){

    FlatbuffersJoinExprEvaluator join_expr_eval(join_expr, join_expr_length);
    QRowReader r(BufferRefView<qix::QEncryptedBlocks>(input_rows, input_rows_length));
    QRowReader j(BufferRefView<qix::QEncryptedBlocks>(join_row, join_row_length));
    QRowWriter w;

    QRowWriter primary_group;
    FlatbuffersTemporaryRow last_primary_of_group;
    while(j.has_next()){
      const tuix::Row *row = j.next();
      primary_group.append(row);
      last_primary_of_group.set(row);
    }

    while (r.has_next()){
      const tuix::Row *current = r.next();

      if(join_expr_eval.is_primary(current)){
        if(last_primary_of_group.get() && join_expr_eval.is_same_group(last_primary_of_group.get(), current)){
          primary_group.append(current);
          last_primary_of_group.set(current);
        }else{
          primary_group.clear();
          primary_group.append(current);
          last_primary_of_group.set(current);
        }
      }else{
        if(last_primary_of_group.get() && join_expr_eval.is_same_group(last_primary_of_group.get(), current)){
          primary_group.set_meta(r.meta());
          auto primary_group_buffer = primary_group.output_buffer();
          QRowReader primary_group_reader(primary_group_buffer.view());
          while(primary_group_reader.has_next()){
            const tuix::Row *primary = primary_group_reader.next();

            if(!join_expr_eval.is_same_group(primary, current)){
              throw std::runtime_error(
                std::string("Invariant violation: rows of primary_group "
                            "are not of the same group: ")
                + to_string(primary)
                + std::string(" vs ")
                + to_string(current));
            }

            w.append(primary, current);
          }
        }
      }
    }

    flatbuffers::FlatBufferBuilder meta_builder;
    const flatbuffers::Offset<qix::QMeta> meta_new = w.unary_update_meta(r.meta(),
                                                                            false,
                                                                            "qjoin",
                                                                            meta_builder);
    meta_builder.Finish(meta_new);
    w.set_meta(flatbuffers::GetRoot<qix::QMeta>(meta_builder.GetBufferPointer()));
    meta_builder.Clear();

    w.output_buffer(output_rows, output_rows_length);
}
