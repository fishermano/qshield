#include "QFlatbuffersWriters.h"

flatbuffers::Offset<qix::QMeta> QRowWriter::unary_update_meta(
  const qix::QMeta *meta, bool is_first_node, std::string parent, flatbuffers::FlatBufferBuilder& builder){

    if(meta->w() <= 0){
      throw std::runtime_error(
        parent
        + std::string(" refuses to run for insufficient w."));
    }

    if(is_first_node){

    }

    uint32_t trace_num = meta->exe_trace()->size() +1;
    std::vector<flatbuffers::Offset<qix::QTrace>> trace_values(trace_num);
    flatbuffers::uoffset_t i = 0;
    for(; i < trace_num -1; i++){
      trace_values[i] = qix::CreateQTrace(
                          builder,
                          builder.CreateString(meta->exe_trace()->Get(i)->parent()),
                          builder.CreateString(meta->exe_trace()->Get(i)->child_left()),
                          builder.CreateString(meta->exe_trace()->Get(i)->child_right()));
    }
    trace_values[i] = qix::CreateQTrace(
                          builder,
                          builder.CreateString(parent),
                          builder.CreateString(meta->exe_trace()->Get(i-1)->parent()),
                          builder.CreateString("null"));
    auto result = qix::CreateQMeta(
                            builder,
                            builder.CreateVector(meta->uid()->data(), meta->uid()->size()),
                            meta->c(),
                            meta->w()-1,
                            builder.CreateVector(trace_values));
    return result;
}

flatbuffers::Offset<qix::QMeta> QRowWriter::flatbuffers_copy_meta(
  const qix::QMeta *meta, flatbuffers::FlatBufferBuilder& builder){

    flatbuffers::uoffset_t num_traces = meta->exe_trace()->size();
    std::vector<flatbuffers::Offset<qix::QTrace>> trace_values(num_traces);
    for(flatbuffers::uoffset_t i = 0; i < num_traces; i++){
      trace_values[i] = qix::CreateQTrace(
                          builder,
                          builder.CreateString(meta->exe_trace()->Get(i)->parent()),
                          builder.CreateString(meta->exe_trace()->Get(i)->child_left()),
                          builder.CreateString(meta->exe_trace()->Get(i)->child_right()));
    }

    auto result = qix::CreateQMeta(
                      builder,
                      builder.CreateVector(meta->uid()->data(), meta->uid()->size()),
                      meta->c(),
                      meta->w(),
                      builder.CreateVector(trace_values));

    return result;

}

void QRowWriter::clear() {
  builder.Clear();
  rows_vector.clear();
  total_num_rows = 0;
  enc_block_builder.Clear();
  enc_block_vector.clear();
  finished = false;
}

void QRowWriter::maybe_finish_block(){
  if(builder.GetSize() >= MAX_BLOCK_SIZE){
    finish_block();
  }
}

void QRowWriter::finish_block(){

  #if QSHIELD_TP
    auto meta_tmp = flatbuffers_copy_meta(meta, builder);

    builder.Finish(qix::CreateQBlock(
                    builder,
                    meta_tmp.o,
                    tuix::CreateRowsDirect(builder, &rows_vector)));
    size_t enc_rows_len = enc_size(builder.GetSize());

    uint8_t *enc_rows_ptr = nullptr;
    ocall_malloc(enc_rows_len, &enc_rows_ptr);

    std::unique_ptr<uint8_t, decltype(&ocall_free)> enc_rows(enc_rows_ptr, &ocall_free);
    rdd_encrypt(builder.GetBufferPointer(), builder.GetSize(), enc_rows.get());

    enc_block_vector.push_back(
      qix::CreateQEncryptedBlock(
        enc_block_builder,
        rows_vector.size(),
        enc_block_builder.CreateVector(enc_rows.get(), enc_rows_len)));

    builder.Clear();
    rows_vector.clear();
  #else
    builder.Finish(tuix::CreateRowsDirect(builder, &rows_vector));
    size_t enc_rows_len = enc_size(builder.GetSize());

    uint8_t *enc_rows_ptr = nullptr;
    ocall_malloc(enc_rows_len, &enc_rows_ptr);

    std::unique_ptr<uint8_t, decltype(&ocall_free)> enc_rows(enc_rows_ptr, &ocall_free);
    rdd_encrypt(builder.GetBufferPointer(), builder.GetSize(), enc_rows.get());

    enc_block_vector.push_back(
      qix::CreateQEncryptedBlock(
        enc_block_builder,
        rows_vector.size(),
        enc_block_builder.CreateVector(enc_rows.get(), enc_rows_len)));

    builder.Clear();
    rows_vector.clear();    
  #endif
}

void QRowWriter::set_meta(const qix::QMeta *mt){
  meta = mt;
}

flatbuffers::Offset<qix::QEncryptedBlocks> QRowWriter::finish_blocks(){
  if(rows_vector.size() > 0){
    finish_block();
  }

  auto result = qix::CreateQEncryptedBlocksDirect(
    enc_block_builder,
    &enc_block_vector);
  enc_block_builder.Finish(result);

  finished = true;

  return result;
}

void QRowWriter::append(const tuix::Row *row){
  rows_vector.push_back(flatbuffers_copy(row, builder));
  total_num_rows++;
  maybe_finish_block();
}

void QRowWriter::append(const std::vector<const tuix::Field *> &row_fields) {
  flatbuffers::uoffset_t num_fields = row_fields.size();
  std::vector<flatbuffers::Offset<tuix::Field>> field_values(num_fields);
  for (flatbuffers::uoffset_t i = 0; i < num_fields; i++) {
    field_values[i] = flatbuffers_copy<tuix::Field>(row_fields[i], builder);
  }
  rows_vector.push_back(tuix::CreateRowDirect(builder, &field_values));
  total_num_rows++;
  maybe_finish_block();
}

void QRowWriter::append(const tuix::Row *row1, const tuix::Row *row2) {
  flatbuffers::uoffset_t num_fields = row1->field_values()->size() + row2->field_values()->size();
  std::vector<flatbuffers::Offset<tuix::Field>> field_values(num_fields);
  flatbuffers::uoffset_t i = 0;
  for (auto it = row1->field_values()->begin(); it != row1->field_values()->end(); ++it, ++i) {
    field_values[i] = flatbuffers_copy<tuix::Field>(*it, builder);
  }
  for (auto it = row2->field_values()->begin(); it != row2->field_values()->end(); ++it, ++i) {
    field_values[i] = flatbuffers_copy<tuix::Field>(*it, builder);
  }
  rows_vector.push_back(tuix::CreateRowDirect(builder, &field_values));
  total_num_rows++;
  maybe_finish_block();
}

UntrustedBufferRef<qix::QEncryptedBlocks> QRowWriter::output_buffer(){
  if(!finished){
    finish_blocks();
  }

  uint8_t *buf_ptr;
  ocall_malloc(enc_block_builder.GetSize(), &buf_ptr);

  std::unique_ptr<uint8_t, decltype(&ocall_free)> buf(buf_ptr, &ocall_free);
  memcpy(buf.get(), enc_block_builder.GetBufferPointer(), enc_block_builder.GetSize());

  UntrustedBufferRef<qix::QEncryptedBlocks> buffer(
    std::move(buf), enc_block_builder.GetSize());
    return buffer;
}

void QRowWriter::output_buffer(uint8_t **output_rows, size_t *output_rows_length){
  auto result = output_buffer();
  *output_rows = result.buf.release();
  *output_rows_length = result.len;
}

uint32_t QRowWriter::num_rows(){
  return total_num_rows;
}

void QSortedRunsWriter::clear(){
  container.clear();
  runs.clear();
}

void QSortedRunsWriter::append(const tuix::Row *row){
  container.append(row);
}

void QSortedRunsWriter::append(const std::vector<const tuix::Field *> &row_fields) {
  container.append(row_fields);
}

void QSortedRunsWriter::append(const tuix::Row *row1, const tuix::Row *row2) {
  container.append(row1, row2);
}

void QSortedRunsWriter::finish_run() {
  runs.push_back(container.finish_blocks());
}

uint32_t QSortedRunsWriter::num_runs() {
  return runs.size();
}

void QSortedRunsWriter::set_meta(const qix::QMeta *mt){
  container.set_meta(mt);
}

UntrustedBufferRef<qix::QSortedRuns> QSortedRunsWriter::output_buffer() {
  container.enc_block_builder.Finish(
    qix::CreateQSortedRunsDirect(container.enc_block_builder, &runs));

  uint8_t *buf_ptr;
  ocall_malloc(container.enc_block_builder.GetSize(), &buf_ptr);

  std::unique_ptr<uint8_t, decltype(&ocall_free)> buf(buf_ptr, &ocall_free);
  memcpy(buf.get(),
         container.enc_block_builder.GetBufferPointer(),
         container.enc_block_builder.GetSize());

  UntrustedBufferRef<qix::QSortedRuns> buffer(
    std::move(buf), container.enc_block_builder.GetSize());
  return buffer;
}

QRowWriter *QSortedRunsWriter::as_row_writer() {
  if (runs.size() > 1) {
    throw std::runtime_error("Invalid attempt to convert QSortedRunsWriter with more than one run "
                             "to QRowWriter");
  }

  return &container;
}
