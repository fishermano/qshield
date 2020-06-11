#include "Flatbuffers.h"
#include "QCrypto.h"

#ifndef QFLATBUFFERS_WRITERS_H
#define QFLATBUFFERS_WRITERS_H

using namespace edu::berkeley::cs::rise::opaque;
using namespace edu::xjtu::cs::cyx::qshield;

class UntrustedMemoryAllocator : public flatbuffers::Allocator {
public:
  virtual uint8_t *allocate(size_t size) {
    uint8_t *result = nullptr;
    ocall_malloc(size, &result);
    return result;
  }
  virtual void deallocate(uint8_t *p, size_t size) {
    (void)size;
    ocall_free(p);
  }
};

class QRowWriter {
public:
  QRowWriter()
    : builder(), rows_vector(), total_num_rows(0), untrusted_alloc(),
      enc_blocks_builder(2048, &untrusted_alloc), finished(false), meta() {}

  void clear();

  void append(const tuix::Row *row);

  void append(const std::vector<const tuix::Field *> &row_fields);

  void append(const tuix::Row *row1, const tuix::Row *row2);

  void set_meta(const qix::QMeta *meta);

  UntrustedBufferRef<qix::QEncryptedBlocks> output_buffer();

  void output_buffer(uint8_t **output_rows, size_t *output_rows_length);

  uint32_t num_rows();

private:
  flatbuffers::Offset<qix::QMeta> flatbuffers_copy_meta(
    const qix::QMeta *meta, flatbuffers::FlatBufferBuilder& builder);
  void maybe_finish_block();
  void finish_block();
  flatbuffers::Offset<qix::QEncryptedBlocks> finish_blocks();

  flatbuffers::FlatBufferBuilder builder;
  std::vector<flatbuffers::Offset<tuix::Row>> rows_vector;
  uint32_t total_num_rows;

  UntrustedMemoryAllocator untrusted_alloc;

  std::vector<flatbuffers::Offset<qix::QBlock>> blocks_vector;

  flatbuffers::FlatBufferBuilder enc_blocks_builder;

  bool finished;

  flatbuffers::Offset<qix::QMeta> meta;
};

#endif//QFLATBUFFERS_WRITERS_H