#include <cstddef>
#include <cstdint>

#ifndef RESPREPARED_H
#define RESPREPARED_H

void res_prepared(uint8_t *input_rows, size_t input_rows_length,
                    uint8_t **output_rows, size_t *output_rows_length);

#endif
