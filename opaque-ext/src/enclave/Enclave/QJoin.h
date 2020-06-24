#include <cstddef>
#include <cstdint>

#ifndef QJOIN_H
#define QJOIN_H

void qscan_collect_last_primary(
  uint8_t *join_expr, size_t join_expr_length,
  uint8_t *input_rows, size_t input_rows_length,
  uint8_t **output_rows, size_t *output_rows_length);

void qsort_merge_join(
    uint8_t *join_expr, size_t join_expr_length,
    uint8_t *input_rows, size_t input_rows_length,
    uint8_t *join_row, size_t join_row_length,
    uint8_t **output_rows, size_t *output_rows_length);

#endif
