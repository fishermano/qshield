#include "Enclave_t.h"

#include <cstdint>
#include <cassert>

#include <sgx_lfence.h>
#include <sgx_tkey_exchange.h>

#include "Aggregate.h"
#include "Crypto.h"
#include "QCrypto.h"
#include "Filter.h"
#include "Join.h"
#include "Project.h"
#include "Sort.h"
#include "util.h"

#include "ACPolicyApplied.h"
#include "ResPrepared.h"
#include "QFilter.h"
#include "QProject.h"
#include "QAggregate.h"
#include "QSort.h"
#include "QJoin.h"

// This file contains definitions of the ecalls declared in Enclave.edl. Errors originating within
// these ecalls are signaled by throwing a std::runtime_error, which is caught at the top level of
// the ecall (i.e., within these definitions), and are then rethrown as Java exceptions using
// ocall_throw.

void ecall_encrypt(uint8_t *plaintext, uint32_t plaintext_length,
                   uint8_t *ciphertext, uint32_t cipher_length) {
  // Guard against encrypting or overwriting enclave memory
  assert(sgx_is_outside_enclave(plaintext, plaintext_length) == 1);
  assert(sgx_is_outside_enclave(ciphertext, cipher_length) == 1);
  sgx_lfence();

  try {
    // IV (12 bytes) + ciphertext + mac (16 bytes)
    assert(cipher_length >= plaintext_length + SGX_AESGCM_IV_SIZE + SGX_AESGCM_MAC_SIZE);
    (void)cipher_length;
    (void)plaintext_length;
    encrypt(plaintext, plaintext_length, ciphertext);
  } catch (const std::runtime_error &e) {
    ocall_throw(e.what());
  }
}

void ecall_project(uint8_t *condition, size_t condition_length,
                   uint8_t *input_rows, size_t input_rows_length,
                   uint8_t **output_rows, size_t *output_rows_length) {
  // Guard against operating on arbitrary enclave memory
  assert(sgx_is_outside_enclave(input_rows, input_rows_length) == 1);
  sgx_lfence();

  try {
    project(condition, condition_length,
            input_rows, input_rows_length,
            output_rows, output_rows_length);
  } catch (const std::runtime_error &e) {
    ocall_throw(e.what());
  }
}

void ecall_filter(uint8_t *condition, size_t condition_length,
                  uint8_t *input_rows, size_t input_rows_length,
                  uint8_t **output_rows, size_t *output_rows_length) {
  // Guard against operating on arbitrary enclave memory
  assert(sgx_is_outside_enclave(input_rows, input_rows_length) == 1);
  sgx_lfence();

  try {
    filter(condition, condition_length,
           input_rows, input_rows_length,
           output_rows, output_rows_length);
  } catch (const std::runtime_error &e) {
    ocall_throw(e.what());
  }
}

void ecall_sample(uint8_t *input_rows, size_t input_rows_length,
                  uint8_t **output_rows, size_t *output_rows_length) {
  // Guard against operating on arbitrary enclave memory
  assert(sgx_is_outside_enclave(input_rows, input_rows_length) == 1);
  sgx_lfence();

  try {
    sample(input_rows, input_rows_length,
           output_rows, output_rows_length);
  } catch (const std::runtime_error &e) {
    ocall_throw(e.what());
  }
}

void ecall_find_range_bounds(uint8_t *sort_order, size_t sort_order_length,
                             uint32_t num_partitions,
                             uint8_t *input_rows, size_t input_rows_length,
                             uint8_t **output_rows, size_t *output_rows_length) {
  // Guard against operating on arbitrary enclave memory
  assert(sgx_is_outside_enclave(input_rows, input_rows_length) == 1);
  sgx_lfence();

  try {
    find_range_bounds(sort_order, sort_order_length,
                      num_partitions,
                      input_rows, input_rows_length,
                      output_rows, output_rows_length);
  } catch (const std::runtime_error &e) {
    ocall_throw(e.what());
  }
}

void ecall_partition_for_sort(uint8_t *sort_order, size_t sort_order_length,
                              uint32_t num_partitions,
                              uint8_t *input_rows, size_t input_rows_length,
                              uint8_t *boundary_rows, size_t boundary_rows_length,
                              uint8_t **output_partitions, size_t *output_partition_lengths) {
  // Guard against operating on arbitrary enclave memory
  assert(sgx_is_outside_enclave(input_rows, input_rows_length) == 1);
  assert(sgx_is_outside_enclave(boundary_rows, boundary_rows_length) == 1);
  sgx_lfence();

  try {
    partition_for_sort(sort_order, sort_order_length,
                       num_partitions,
                       input_rows, input_rows_length,
                       boundary_rows, boundary_rows_length,
                       output_partitions, output_partition_lengths);
  } catch (const std::runtime_error &e) {
    ocall_throw(e.what());
  }
}

void ecall_external_sort(uint8_t *sort_order, size_t sort_order_length,
                         uint8_t *input_rows, size_t input_rows_length,
                         uint8_t **output_rows, size_t *output_rows_length) {
  // Guard against operating on arbitrary enclave memory
  assert(sgx_is_outside_enclave(input_rows, input_rows_length) == 1);
  sgx_lfence();

  try {
    external_sort(sort_order, sort_order_length,
                  input_rows, input_rows_length,
                  output_rows, output_rows_length);
  } catch (const std::runtime_error &e) {
    ocall_throw(e.what());
  }
}

void ecall_scan_collect_last_primary(uint8_t *join_expr, size_t join_expr_length,
                                     uint8_t *input_rows, size_t input_rows_length,
                                     uint8_t **output_rows, size_t *output_rows_length) {
  // Guard against operating on arbitrary enclave memory
  assert(sgx_is_outside_enclave(input_rows, input_rows_length) == 1);
  sgx_lfence();

  try {
    scan_collect_last_primary(join_expr, join_expr_length,
                              input_rows, input_rows_length,
                              output_rows, output_rows_length);
  } catch (const std::runtime_error &e) {
    ocall_throw(e.what());
  }
}

void ecall_non_oblivious_sort_merge_join(uint8_t *join_expr, size_t join_expr_length,
                                         uint8_t *input_rows, size_t input_rows_length,
                                         uint8_t *join_row, size_t join_row_length,
                                         uint8_t **output_rows, size_t *output_rows_length) {
  // Guard against operating on arbitrary enclave memory
  assert(sgx_is_outside_enclave(input_rows, input_rows_length) == 1);
  assert(sgx_is_outside_enclave(join_row, join_row_length) == 1);
  sgx_lfence();

  try {
    non_oblivious_sort_merge_join(join_expr, join_expr_length,
                                  input_rows, input_rows_length,
                                  join_row, join_row_length,
                                  output_rows, output_rows_length);
  } catch (const std::runtime_error &e) {
    ocall_throw(e.what());
  }
}

void ecall_non_oblivious_aggregate_step1(
  uint8_t *agg_op, size_t agg_op_length,
  uint8_t *input_rows, size_t input_rows_length,
  uint8_t **first_row, size_t *first_row_length,
  uint8_t **last_group, size_t *last_group_length,
  uint8_t **last_row, size_t *last_row_length) {
  // Guard against operating on arbitrary enclave memory
  assert(sgx_is_outside_enclave(input_rows, input_rows_length) == 1);
  sgx_lfence();

  try {
    non_oblivious_aggregate_step1(
      agg_op, agg_op_length,
      input_rows, input_rows_length,
      first_row, first_row_length,
      last_group, last_group_length,
      last_row, last_row_length);
  } catch (const std::runtime_error &e) {
    ocall_throw(e.what());
  }
}

void ecall_non_oblivious_aggregate_step2(
  uint8_t *agg_op, size_t agg_op_length,
  uint8_t *input_rows, size_t input_rows_length,
  uint8_t *next_partition_first_row, size_t next_partition_first_row_length,
  uint8_t *prev_partition_last_group, size_t prev_partition_last_group_length,
  uint8_t *prev_partition_last_row, size_t prev_partition_last_row_length,
  uint8_t **output_rows, size_t *output_rows_length) {
  // Guard against operating on arbitrary enclave memory
  assert(sgx_is_outside_enclave(input_rows, input_rows_length) == 1);
  assert(sgx_is_outside_enclave(next_partition_first_row, next_partition_first_row_length) == 1);
  assert(sgx_is_outside_enclave(prev_partition_last_group, prev_partition_last_group_length) == 1);
  assert(sgx_is_outside_enclave(prev_partition_last_row, prev_partition_last_row_length) == 1);
  sgx_lfence();

  try {
    non_oblivious_aggregate_step2(
      agg_op, agg_op_length,
      input_rows, input_rows_length,
      next_partition_first_row, next_partition_first_row_length,
      prev_partition_last_group, prev_partition_last_group_length,
      prev_partition_last_row, prev_partition_last_row_length,
      output_rows, output_rows_length);
  } catch (const std::runtime_error &e) {
    ocall_throw(e.what());
  }
}

sgx_status_t ecall_enclave_init_ra(sgx_ra_context_t *context) {
  try {
    return sgx_ra_init(&g_sp_pub_key, false, context);
  } catch (const std::runtime_error &e) {
    ocall_throw(e.what());
    // We return success so that the exception just thrown doesn't get overridden by another
    // exception due to the return code.
    return SGX_SUCCESS;
  }
}


void ecall_enclave_ra_close(sgx_ra_context_t context) {
  try {
    sgx_ra_close(context);
  } catch (const std::runtime_error &e) {
    ocall_throw(e.what());
  }
}

void ecall_ra_proc_msg4(
  sgx_ra_context_t context, uint8_t *msg4, uint32_t msg4_size) {
  try {
    set_ska(context, msg4, msg4_size);
    init_rdd_key_schedule();
    init_tk_key_schedule();
  } catch (const std::runtime_error &e) {
    ocall_throw(e.what());
  }
}

void ecall_ac_policy_applied(
  uint8_t *input_rows, size_t input_rows_length,
  uint8_t *tk, size_t tk_length,
  uint8_t **output_rows, size_t *output_rows_length){

    // Guard against operating on arbitrary enclave memory
    assert(sgx_is_outside_enclave(input_rows, input_rows_length) == 1);
    sgx_lfence();

    try {
      ac_policy_applied(input_rows, input_rows_length,
                        tk, tk_length,
                        output_rows, output_rows_length);
    } catch (const std::runtime_error &e) {
      ocall_throw(e.what());
    }
}

void ecall_res_prepared(
  uint8_t *input_rows, size_t input_rows_length,
  uint8_t **output_rows, size_t *output_rows_length){

    // Guard against operating on arbitrary enclave memory
    assert(sgx_is_outside_enclave(input_rows, input_rows_length) == 1);
    sgx_lfence();

    try{
      res_prepared(input_rows, input_rows_length,
                    output_rows, output_rows_length);
    } catch (const std::runtime_error &e) {
      ocall_throw(e.what());
    }
}


void ecall_pairing_init(
  uint8_t *param, size_t param_length){

    // Guard against operating on arbitrary enclave memory
    assert(sgx_is_outside_enclave(param, param_length) == 1);
    sgx_lfence();

    try{
      init_pairing_env(reinterpret_cast<const char*>(param), param_length);
    } catch (const std::runtime_error &e) {
      ocall_throw(e.what());
    }
}

void ecall_qproject(uint8_t *project_list, size_t project_list_length,
                   uint8_t *input_rows, size_t input_rows_length,
                   uint8_t **output_rows, size_t *output_rows_length) {
  // Guard against operating on arbitrary enclave memory
  assert(sgx_is_outside_enclave(input_rows, input_rows_length) == 1);
  sgx_lfence();

  try {
    qproject(project_list, project_list_length,
            input_rows, input_rows_length,
            output_rows, output_rows_length);
  } catch (const std::runtime_error &e) {
    ocall_throw(e.what());
  }
}

void ecall_qfilter(uint8_t *condition, size_t condition_length,
                  uint8_t *input_rows, size_t input_rows_length,
                  uint8_t **output_rows, size_t *output_rows_length) {
  // Guard against operating on arbitrary enclave memory
  assert(sgx_is_outside_enclave(input_rows, input_rows_length) == 1);
  sgx_lfence();

  try {
    qfilter(condition, condition_length,
           input_rows, input_rows_length,
           output_rows, output_rows_length);
  } catch (const std::runtime_error &e) {
    ocall_throw(e.what());
  }
}

void ecall_qaggregate_step1(
  uint8_t *agg_op, size_t agg_op_length,
  uint8_t *input_rows, size_t input_rows_length,
  uint8_t **first_row, size_t *first_row_length,
  uint8_t **last_group, size_t *last_group_length,
  uint8_t **last_row, size_t *last_row_length) {
  // Guard against operating on arbitrary enclave memory
  assert(sgx_is_outside_enclave(input_rows, input_rows_length) == 1);
  sgx_lfence();

  try {
    qaggregate_step1(
      agg_op, agg_op_length,
      input_rows, input_rows_length,
      first_row, first_row_length,
      last_group, last_group_length,
      last_row, last_row_length);
  } catch (const std::runtime_error &e) {
    ocall_throw(e.what());
  }
}

void ecall_qaggregate_step2(
  uint8_t *agg_op, size_t agg_op_length,
  uint8_t *input_rows, size_t input_rows_length,
  uint8_t *next_partition_first_row, size_t next_partition_first_row_length,
  uint8_t *prev_partition_last_group, size_t prev_partition_last_group_length,
  uint8_t *prev_partition_last_row, size_t prev_partition_last_row_length,
  uint8_t **output_rows, size_t *output_rows_length) {
  // Guard against operating on arbitrary enclave memory
  assert(sgx_is_outside_enclave(input_rows, input_rows_length) == 1);
  assert(sgx_is_outside_enclave(next_partition_first_row, next_partition_first_row_length) == 1);
  assert(sgx_is_outside_enclave(prev_partition_last_group, prev_partition_last_group_length) == 1);
  assert(sgx_is_outside_enclave(prev_partition_last_row, prev_partition_last_row_length) == 1);
  sgx_lfence();

  try {
    qaggregate_step2(
      agg_op, agg_op_length,
      input_rows, input_rows_length,
      next_partition_first_row, next_partition_first_row_length,
      prev_partition_last_group, prev_partition_last_group_length,
      prev_partition_last_row, prev_partition_last_row_length,
      output_rows, output_rows_length);
  } catch (const std::runtime_error &e) {
    ocall_throw(e.what());
  }
}

void ecall_qsample(uint8_t *input_rows, size_t input_rows_length,
                  uint8_t **output_rows, size_t *output_rows_length) {
  // Guard against operating on arbitrary enclave memory
  assert(sgx_is_outside_enclave(input_rows, input_rows_length) == 1);
  sgx_lfence();

  try {
    qsample(input_rows, input_rows_length,
           output_rows, output_rows_length);
  } catch (const std::runtime_error &e) {
    ocall_throw(e.what());
  }
}

void ecall_qfind_range_bounds(uint8_t *sort_order, size_t sort_order_length,
                             uint32_t num_partitions,
                             uint8_t *input_rows, size_t input_rows_length,
                             uint8_t **output_rows, size_t *output_rows_length) {
  // Guard against operating on arbitrary enclave memory
  assert(sgx_is_outside_enclave(input_rows, input_rows_length) == 1);
  sgx_lfence();

  try {
    qfind_range_bounds(sort_order, sort_order_length,
                      num_partitions,
                      input_rows, input_rows_length,
                      output_rows, output_rows_length);
  } catch (const std::runtime_error &e) {
    ocall_throw(e.what());
  }
}

void ecall_qpartition_for_sort(uint8_t *sort_order, size_t sort_order_length,
                              uint32_t num_partitions,
                              uint8_t *input_rows, size_t input_rows_length,
                              uint8_t *boundary_rows, size_t boundary_rows_length,
                              uint8_t **output_partitions, size_t *output_partition_lengths) {
  // Guard against operating on arbitrary enclave memory
  assert(sgx_is_outside_enclave(input_rows, input_rows_length) == 1);
  assert(sgx_is_outside_enclave(boundary_rows, boundary_rows_length) == 1);
  sgx_lfence();

  try {
    qpartition_for_sort(sort_order, sort_order_length,
                       num_partitions,
                       input_rows, input_rows_length,
                       boundary_rows, boundary_rows_length,
                       output_partitions, output_partition_lengths);
  } catch (const std::runtime_error &e) {
    ocall_throw(e.what());
  }
}

void ecall_qexternal_sort(uint8_t *sort_order, size_t sort_order_length,
                         uint8_t *input_rows, size_t input_rows_length,
                         uint8_t **output_rows, size_t *output_rows_length) {
  // Guard against operating on arbitrary enclave memory
  assert(sgx_is_outside_enclave(input_rows, input_rows_length) == 1);
  sgx_lfence();

  try {
    qexternal_sort(sort_order, sort_order_length,
                  input_rows, input_rows_length,
                  output_rows, output_rows_length);
  } catch (const std::runtime_error &e) {
    ocall_throw(e.what());
  }
}

void ecall_qscan_collect_last_primary(uint8_t *join_expr, size_t join_expr_length,
                                     uint8_t *input_rows, size_t input_rows_length,
                                     uint8_t **output_rows, size_t *output_rows_length) {
  // Guard against operating on arbitrary enclave memory
  assert(sgx_is_outside_enclave(input_rows, input_rows_length) == 1);
  sgx_lfence();

  try {
    qscan_collect_last_primary(join_expr, join_expr_length,
                              input_rows, input_rows_length,
                              output_rows, output_rows_length);
  } catch (const std::runtime_error &e) {
    ocall_throw(e.what());
  }
}

void ecall_qsort_merge_join(uint8_t *join_expr, size_t join_expr_length,
                                         uint8_t *input_rows, size_t input_rows_length,
                                         uint8_t *join_row, size_t join_row_length,
                                         uint8_t **output_rows, size_t *output_rows_length) {
  // Guard against operating on arbitrary enclave memory
  assert(sgx_is_outside_enclave(input_rows, input_rows_length) == 1);
  assert(sgx_is_outside_enclave(join_row, join_row_length) == 1);
  sgx_lfence();

  try {
    qsort_merge_join(join_expr, join_expr_length,
                                  input_rows, input_rows_length,
                                  join_row, join_row_length,
                                  output_rows, output_rows_length);
  } catch (const std::runtime_error &e) {
    ocall_throw(e.what());
  }
}
