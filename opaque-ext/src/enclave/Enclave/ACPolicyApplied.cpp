#include "ACPolicyApplied.h"

#include "FlatbuffersReaders.h"
#include "FlatbuffersWriters.h"

#include "QFlatbuffersReaders.h"

#include <string.h>

using namespace edu::berkeley::cs::rise::opaque;
using namespace edu::xjtu::cs::cyx::qshield;

void ac_policy_applied(uint8_t *input_rows, size_t input_rows_length,
                        uint8_t *tk, size_t tk_length,
                        uint8_t **output_rows, size_t *output_rows_length) {

  RowReader row_r(BufferRefView<tuix::EncryptedBlocks>(input_rows, input_rows_length));
  RowWriter row_w;

  QTokenReader tk_r(BufferRefView<qix::QEncryptedToken>(tk, tk_length));

  //uint32_t w = tk_r.w();
  //uint32_t c = tk_r.c();
  uint8_t *sk_b_data = nullptr;
  uint32_t sk_b_length = 0;
  tk_r.sk_b(&sk_b_data, &sk_b_length);

  if(sk_b_length != 16){
    ocall_throw("ACPolicyApplied: sk_b_lenth != 16.");
  }

  while (row_r.has_next()) {
    const tuix::Row *row = row_r.next();
    row_w.append(row);
  }

  row_w.output_buffer(output_rows, output_rows_length);
}
