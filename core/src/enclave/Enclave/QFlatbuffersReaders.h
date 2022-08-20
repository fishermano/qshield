#include "Flatbuffers.h"
#include "QCrypto.h"

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

class QEncryptedBlockToQRowReader {
public:
  QEncryptedBlockToQRowReader() : rows(nullptr), initialized(false) {}

  void reset(const qix::QEncryptedBlock *enc_block);

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

  const qix::QMeta *meta();

private:
  const qix::QBlock *block;
  std::unique_ptr<uint8_t> rows_buf;
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

  const qix::QEncryptedBlocks *enc_blocks;
  uint32_t block_idx;
  QEncryptedBlockToQRowReader block_reader;
};

/**
 * A reader for Rows organized into sorted runs.
 *
 * Different runs can be read independently. Within a run, access is performed using an
 * iterator-style sequential interface.
 */
class QSortedRunsReader {
public:
  QSortedRunsReader(BufferRefView<qix::QSortedRuns> buf);

  void reset(BufferRefView<qix::QSortedRuns> buf);

  uint32_t num_runs();
  bool run_has_next(uint32_t run_idx);
  /**
   * Access the next Row from the given run. Invalidates any previously-returned Row pointers from
   * the same run.
   */
  const tuix::Row *next_from_run(uint32_t run_idx);

  const qix::QMeta *meta();

private:
  const qix::QSortedRuns *sorted_runs;
  std::vector<QRowReader> run_readers;
};

/** A range-style reader for QBlock objects within an QEncryptedBlocks object. */
class QEncryptedBlocksToQEncryptedBlockReader {
public:
  QEncryptedBlocksToQEncryptedBlockReader(BufferRefView<qix::QEncryptedBlocks> buf){
    buf.verify();
    enc_blocks = buf.root();
  }
  flatbuffers::Vector<flatbuffers::Offset<qix::QEncryptedBlock>>::const_iterator begin() {
    return enc_blocks->blocks()->begin();
  }
  flatbuffers::Vector<flatbuffers::Offset<qix::QEncryptedBlock>>::const_iterator end() {
    return enc_blocks->blocks()->end();
  }

private:
  const qix::QEncryptedBlocks *enc_blocks;
};

#endif//QFLATBUFFERS_READERS_H
