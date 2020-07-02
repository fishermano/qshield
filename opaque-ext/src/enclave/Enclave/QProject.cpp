#include "QProject.h"

#include "ExpressionEvaluation.h"
#include "QFlatbuffersReaders.h"
#include "QFlatbuffersWriters.h"

#include <string.h>

using namespace edu::berkeley::cs::rise::opaque;
using namespace edu::xjtu::cs::cyx::qshield;

void qproject(uint8_t *project_list, size_t project_list_length,
             uint8_t *input_rows, size_t input_rows_length,
             uint8_t **output_rows, size_t *output_rows_length){

   BufferRefView<tuix::ProjectExpr> project_list_buf(project_list, project_list_length);
   project_list_buf.verify();

   const tuix::ProjectExpr *project_expr = project_list_buf.root();
   std::vector<std::unique_ptr<FlatbuffersExpressionEvaluator>> project_eval_list;
   for (auto it = project_expr->project_list()->begin();
        it != project_expr->project_list()->end();
        ++it){
      project_eval_list.emplace_back(new FlatbuffersExpressionEvaluator(*it));
    }

    QRowReader row_r(BufferRefView<qix::QEncryptedBlocks>(input_rows, input_rows_length));

    QRowWriter row_w;

    flatbuffers::FlatBufferBuilder meta_builder;
    const flatbuffers::Offset<qix::QMeta> meta_new = row_w.unary_update_meta(row_r.meta(),
                                                                              false,
                                                                              "qproject",
                                                                              meta_builder);
    meta_builder.Finish(meta_new);
    row_w.set_meta(flatbuffers::GetRoot<qix::QMeta>(meta_builder.GetBufferPointer()));
    meta_builder.Clear();

    std::vector<const tuix::Field *> out_fields(project_eval_list.size());
    while(row_r.has_next()){
      const tuix::Row *row = row_r.next();
      for (uint32_t j = 0; j < project_eval_list.size(); j++){
        out_fields[j] = project_eval_list[j]->eval(row);
      }
      row_w.append(out_fields);
    }

    row_w.output_buffer(output_rows, output_rows_length);

}
