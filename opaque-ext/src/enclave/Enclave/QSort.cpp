#include "QSort.h"

#include <algorithm>
#include <queue>

#include "ExpressionEvaluation.h"
#include "QFlatbuffersReaders.h"
#include "QFlatbuffersWriters.h"

#include "qdebug.h"

class MergeItem {
 public:
  const tuix::Row *v;
  uint32_t run_idx;
};

void qexternal_merge(
  QSortedRunsReader &r,
  uint32_t run_start,
  uint32_t num_runs,
  QSortedRunsWriter &w,
  FlatbuffersSortOrderEvaluator &sort_eval) {

  // Maintain a priority queue with one row per run
  auto compare = [&sort_eval](const MergeItem &a, const MergeItem &b) {
    return sort_eval.less_than(b.v, a.v);
  };
  std::priority_queue<MergeItem, std::vector<MergeItem>, decltype(compare)>
    queue(compare);

  // Initialize the priority queue with the first row from each run
  for (uint32_t i = run_start; i < run_start + num_runs; i++) {
    debug("external_merge: Read first row from run %d\n", i);
    MergeItem item;
    item.v = r.next_from_run(i);
    item.run_idx = i;
    queue.push(item);
  }

  // Merge the runs using the priority queue
  int j = 0;
  while (!queue.empty()) {

    MergeItem item = queue.top();
    queue.pop();
    w.append(item.v);

    // Read another row from the same run that this one came from
    if (r.run_has_next(item.run_idx)) {
      if(item.run_idx == 1){
        j++;
      }
      item.v = r.next_from_run(item.run_idx);
      queue.push(item);
    }
  }
  debug("buffer 1 totally has %d items\n", j);
  w.finish_run();
}

void qsort_single_block(
  QSortedRunsWriter &w,
  const qix::QEncryptedBlock *block,
  FlatbuffersSortOrderEvaluator &sort_eval) {

  QEncryptedBlockToQRowReader r;
  r.reset(block);
  std::vector<const tuix::Row *> sort_ptrs(r.begin(), r.end());

  std::sort(
    sort_ptrs.begin(), sort_ptrs.end(),
    [&sort_eval](const tuix::Row *a, const tuix::Row *b) {
      return sort_eval.less_than(a, b);
    });

  int i = 0;
  for (auto it = sort_ptrs.begin(); it != sort_ptrs.end(); ++it, i++) {
    w.append(*it);
  }
  // debug("Totally append %d rows\n", i);
  w.finish_run();
}

void qexternal_sort(uint8_t *sort_order, size_t sort_order_length,
                   uint8_t *input_rows, size_t input_rows_length,
                   uint8_t **output_rows, size_t *output_rows_length) {
  FlatbuffersSortOrderEvaluator sort_eval(sort_order, sort_order_length);

  // 1. Sort each QEncryptedBlock individually by decrypting it, sorting within the enclave, and
  // re-encrypting to a different buffer.
  #if QSHIELD_TP
    const qix::QMeta *meta = br.meta();
  #endif

  QSortedRunsWriter w;
  {
    QEncryptedBlocksToQEncryptedBlockReader r(
      BufferRefView<qix::QEncryptedBlocks>(input_rows, input_rows_length));
    uint32_t i = 0;
    for (auto it = r.begin(); it != r.end(); ++it, ++i) {
      debug("Sorting buffer %d with %d rows\n", i, it->num_rows());
      #if QSHIELD_TP
        w.set_meta(meta);
      #endif
      qsort_single_block(w, *it, sort_eval);

    }
    // debug("w = %d\n", w.num_runs());
    if (w.num_runs() <= 1) {
      // Only 0 or 1 runs, so we are done - no need to merge runs
      w.as_row_writer()->output_buffer(output_rows, output_rows_length);
      return;
    }
  }

  // 2. Merge sorted runs. Initially each buffer forms a sorted run. We merge B runs at a time by
  // decrypting an QEncryptedBlock from each one, merging them within the enclave using a priority
  // queue, and re-encrypting to a different buffer.
  auto runs_buf = w.output_buffer();
  QSortedRunsReader r(runs_buf.view());
  debug("totally have %d runs\n", r.num_runs());

  while (r.num_runs() > 1) {
    debug("external_sort: Merging %d runs, up to %d at a time\n",
         r.num_runs(), MAX_NUM_STREAMS);

    w.clear();
    for (uint32_t run_start = 0; run_start < r.num_runs(); run_start += MAX_NUM_STREAMS) {
      uint32_t num_runs =
        std::min(MAX_NUM_STREAMS, static_cast<uint32_t>(r.num_runs()) - run_start);
      debug("external_sort: Merging buffers %d-%d\n", run_start, run_start + num_runs - 1);
      #if QSHIELD_TP
        w.set_meta(meta);
      #endif

      qexternal_merge(r, run_start, num_runs, w, sort_eval);
    }

    if (w.num_runs() > 1) {
      runs_buf = w.output_buffer();
      r.reset(runs_buf.view());
    } else {
      // Done merging. Return the single remaining sorted run.
      w.as_row_writer()->output_buffer(output_rows, output_rows_length);
      return;
    }
  }
}

void qsample(uint8_t *input_rows, size_t input_rows_length,
			uint8_t **output_rows, size_t *output_rows_length) {
  QRowReader r(BufferRefView<qix::QEncryptedBlocks>(input_rows, input_rows_length));
  QRowWriter w;

  #if QSHIELD_TP
    w.set_meta(r.meta());
  #endif

  // Sample ~5% of the rows or 1000 rows, whichever is greater
  uint16_t sampling_ratio;
  if (r.num_rows() > 1000 * 20) {
    sampling_ratio = 3276; // 5% of 2^16
  } else {
    sampling_ratio = 16383;
  }

  while (r.has_next()) {
    const tuix::Row *row = r.next();

    uint16_t rand;
    sgx_read_rand(reinterpret_cast<uint8_t *>(&rand), 2);
    if (rand <= sampling_ratio) {
      w.append(row);
    }
  }

  w.output_buffer(output_rows, output_rows_length);
}

void qfind_range_bounds(uint8_t *sort_order, size_t sort_order_length,
                       uint32_t num_partitions,
                       uint8_t *input_rows, size_t input_rows_length,
                       uint8_t **output_rows, size_t *output_rows_length) {

  // Sort the input rows
  uint8_t *sorted_rows;
  size_t sorted_rows_length;
  qexternal_sort(sort_order, sort_order_length,
                input_rows, input_rows_length,
                &sorted_rows, &sorted_rows_length);

  // Split them into one range per partition
  QRowReader r(BufferRefView<qix::QEncryptedBlocks>(sorted_rows, sorted_rows_length));
  QRowWriter w;
  #if QSHIELD_TP
    w.set_meta(r.meta());
  #endif

  uint32_t num_rows_per_part = r.num_rows() / num_partitions;
  uint32_t current_rows_in_part = 0;
  while (r.has_next()) {
    const tuix::Row *row = r.next();
    if (current_rows_in_part == num_rows_per_part) {
      w.append(row);
      current_rows_in_part = 0;
	} else {
	  ++current_rows_in_part;
	}
  }

  w.output_buffer(output_rows, output_rows_length);

  ocall_free(sorted_rows);
}

void qpartition_for_sort(uint8_t *sort_order, size_t sort_order_length,
                        uint32_t num_partitions,
                        uint8_t *input_rows, size_t input_rows_length,
                        uint8_t *boundary_rows, size_t boundary_rows_length,
                        uint8_t **output_partition_ptrs, size_t *output_partition_lengths) {
  // Sort the input rows
  uint8_t *sorted_rows;
  size_t sorted_rows_length;
  qexternal_sort(sort_order, sort_order_length,
                input_rows, input_rows_length,
                &sorted_rows, &sorted_rows_length);

  // Scan through the input rows and copy each to the appropriate output partition specified by the
  // ranges encoded in the given boundary_rows. A range contains all rows greater than or equal to
  // one boundary row and less than the next boundary row. The first range contains all rows less
  // than the first boundary row, and the last range contains all rows greater than or equal to the
  // last boundary row.
  FlatbuffersSortOrderEvaluator sort_eval(sort_order, sort_order_length);
  QRowReader r(BufferRefView<qix::QEncryptedBlocks>(sorted_rows, sorted_rows_length));
  QRowWriter w;

  uint32_t output_partition_idx = 0;

  QRowReader b(BufferRefView<qix::QEncryptedBlocks>(boundary_rows, boundary_rows_length));
  // Invariant: b_upper is the first boundary row strictly greater than the current range, or
  // nullptr if we are in the last range
  FlatbuffersTemporaryRow b_upper(b.has_next() ? b.next() : nullptr);

  while (r.has_next()) {
    const tuix::Row *row = r.next();

    // Advance boundary rows to maintain the invariant on b_upper
    while (b_upper.get() != nullptr && !sort_eval.less_than(row, b_upper.get())) {
      b_upper.set(b.has_next() ? b.next() : nullptr);

      #if QSHIELD_TP
        w.set_meta(r.meta());
      #endif

      // Write out the newly-finished partition
      w.output_buffer(
        &output_partition_ptrs[output_partition_idx],
        &output_partition_lengths[output_partition_idx]);
      w.clear();
      output_partition_idx++;
    }

    w.append(row);
  }

  // Write out the final partition. If there were fewer boundary rows than expected output
  // partitions, write out enough empty partitions to ensure the expected number of output
  // partitions.
  while (output_partition_idx < num_partitions) {

    #if QSHIELD_TP
      w.set_meta(r.meta());
    #endif
    w.output_buffer(
      &output_partition_ptrs[output_partition_idx],
      &output_partition_lengths[output_partition_idx]);
    w.clear();
    output_partition_idx++;
  }

  ocall_free(sorted_rows);
}
