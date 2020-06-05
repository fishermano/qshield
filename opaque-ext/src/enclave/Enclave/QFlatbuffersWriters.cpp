#include "QFlatbuffersWriters.h"

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
  enc_blocks_builder.Clear();
  blocks_vector.clear();
  finished = false;
}

void QRowWriter::maybe_finish_block(){
  if(builder.GetSize() >= MAX_BLOCK_SIZE){
    finish_block();
  }
}

void QRowWriter::finish_block(){
  blocks_vector.push_back(
      qix::CreateQBlock(
      builder,
      rows_vector.size(),
      tuix::CreateRowsDirect(builder, &rows_vector)));

  rows_vector.clear();
}

void QRowWriter::set_meta(const qix::QMeta *mt){
  // this->meta = flatbuffers_copy_meta(mt, blocks_builder);
  this->meta = flatbuffers_copy_meta(mt, builder);
}

flatbuffers::Offset<qix::QEncryptedBlocks> QRowWriter::finish_blocks(){
  if(rows_vector.size() > 0){
    finish_block();
  }

  auto blocks_buf = qix::CreateQBlocks(
    builder,
    meta.o,
    builder.CreateVector(blocks_vector));

  builder.Finish(blocks_buf);

  size_t enc_blocks_len = enc_size(builder.GetSize());

  uint8_t *enc_blocks_ptr = nullptr;
  ocall_malloc(enc_blocks_len, &enc_blocks_ptr);

  std::unique_ptr<uint8_t, decltype(&ocall_free)> enc_blocks(enc_blocks_ptr, &ocall_free);
  rdd_encrypt(builder.GetBufferPointer(), builder.GetSize(), enc_blocks.get());

  builder.Clear();
  blocks_vector.clear();

  auto result = qix::CreateQEncryptedBlocks(
    enc_blocks_builder,
    enc_blocks_builder.CreateVector(enc_blocks.get(), enc_blocks_len));

  enc_blocks_builder.Finish(result);

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
  ocall_malloc(enc_blocks_builder.GetSize(), &buf_ptr);

  std::unique_ptr<uint8_t, decltype(&ocall_free)> buf(buf_ptr, &ocall_free);
  memcpy(buf.get(), enc_blocks_builder.GetBufferPointer(), enc_blocks_builder.GetSize());

  UntrustedBufferRef<qix::QEncryptedBlocks> buffer(
    std::move(buf), enc_blocks_builder.GetSize());
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
