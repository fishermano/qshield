#include "ResPrepared.h"

#include "FlatbuffersWriters.h"

#include "QFlatbuffersReaders.h"

#include <string.h>

using namespace edu::berkeley::cs::rise::opaque;
using namespace edu::xjtu::cs::cyx::qshield;

void res_prepared(uint8_t *input_rows, size_t input_rows_length,
                    uint8_t **output_rows, size_t *output_rows_length){

  QRowReader row_r(BufferRefView<qix::QEncryptedBlocks>(input_rows, input_rows_length));
  RowWriter row_w;

  while(row_r.has_next()){
    const tuix::Row *row = row_r.next();
    row_w.append(row);
  }

  row_w.output_buffer(output_rows, output_rows_length);

}
