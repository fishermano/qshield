#include <cstddef>
#include <cstdint>

#ifndef ACPOLICYAPPLIED_H
#define ACPOLICYAPPLIED_H

void ac_policy_applied(uint8_t *input_rows, size_t input_rows_length,
                        uint8_t *tk, size_t tk_length,
                        uint8_t **output_rows, size_t *output_rows_length);
#endif
