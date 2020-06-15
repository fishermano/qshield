#include <cstddef>
#include <cstdint>

#ifndef QPROJECT_H
#define QPROJECT_H

void qproject(uint8_t *project_list, size_t project_list_length,
             uint8_t *input_rows, size_t input_rows_length,
             uint8_t **output_rows, size_t *output_rows_length);

#endif // QPROJECT_H
