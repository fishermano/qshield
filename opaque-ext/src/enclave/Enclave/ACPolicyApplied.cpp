#include "ACPolicyApplied.h"

#include "FlatbuffersReaders.h"
#include "FlatbuffersWriters.h"

#include <string.h>

using namespace edu::berkeley::cs::rise::opaque;

void ac_policy_applied(uint8_t *input_rows, size_t input_rows_length,
                        uint8_t *tk, size_t tk_length,
                        uint8_t **output_rows, size_t *output_rows_length) {

  RowReader r(BufferRefView<tuix::EncryptedBlocks>(input_rows, input_rows_length));
  RowWriter w;

  while (r.has_next()) {
    const tuix::Row *row = r.next();
    w.append(row);
  }

  w.output_buffer(output_rows, output_rows_length);
}
