#include <cstddef>
#include <cstdint>

#ifndef QFILTER_H
#define QFILTER_H

/** Non-oblivious filter. */
void qfilter(uint8_t *condition, size_t condition_length,
            uint8_t *input_rows, size_t input_rows_length,
            uint8_t **output_rows, size_t *output_rows_length);

#endif
