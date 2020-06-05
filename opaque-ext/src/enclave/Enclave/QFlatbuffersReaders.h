#include "Flatbuffers.h"

#ifndef QFLATBUFFERS_READERS_H
#define QFLATBUFFERS_READERS_H

using namespace edu::xjtu::cs::cyx::qshield;
using namespace edu::berkeley::cs::rise::opaque;

class QEncryptedTokenToQTokenReader {
public:
  QEncryptedTokenToQTokenReader() : tk(nullptr), initialized(false) {}
  void reset(const qix::QEncryptedToken *encrypted_token);
  const qix::QToken *get_token() {
    if(initialized){
      return tk;
    }
    return nullptr;
  }
private:
  std::unique_ptr<uint8_t> tk_buf;
  const qix::QToken *tk;
  bool initialized;
};

class QTokenReader {
public:
  QTokenReader(BufferRefView<qix::QEncryptedToken> buf);

  void reset(const qix::QEncryptedToken *encrypted_token);

  uint32_t w();
  uint32_t c();
  void sk_b(uint8_t **data, uint32_t *size);
private:
  void init_enc_tk_reader();
  const qix::QEncryptedToken *encrypted_token;
  QEncryptedTokenToQTokenReader enc_tk_reader;
};

class QBlockToQRowReader {
public:
  QBlockToQRowReader() : rows(nullptr), initialized(false) {}

  void reset(const qix::QBlock *block);

  bool has_next() {
    return initialized && row_idx < rows->rows()->size();
  }

  const tuix::Row *next(){
    return rows->rows()->Get(row_idx++);
  }

  flatbuffers::Vector<flatbuffers::Offset<tuix::Row>>::const_iterator begin() {
    return rows->rows()->begin();
  }

  flatbuffers::Vector<flatbuffers::Offset<tuix::Row>>::const_iterator end() {
    return rows->rows()->end();
  }

private:
  const tuix::Rows *rows;
  uint32_t row_idx;
  bool initialized;
};

/** An iterator-style reader for Rows organized into QEncryptedBlocks*/
class QRowReader {
public:
  QRowReader(BufferRefView<qix::QEncryptedBlocks> buf);
  QRowReader(const qix::QEncryptedBlocks *encrypted_blocks);

  void reset(BufferRefView<qix::QEncryptedBlocks> buf);
  void reset(const qix::QEncryptedBlocks *encrypted_blocks);

  uint32_t num_rows();

  bool has_next();

  const tuix::Row *next();

  const qix::QMeta *meta();

private:
  void init_block_reader();

  std::unique_ptr<uint8_t> blocks_buf;
  const qix::QBlocks *blocks;
  uint32_t block_idx;
  QBlockToQRowReader block_reader;
};

#endif//QFLATBUFFERS_READERS_H
