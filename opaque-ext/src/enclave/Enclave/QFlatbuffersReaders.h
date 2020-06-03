#include "Flatbuffers.h"

#ifndef QFLATBUFFERS_READERS_H
#define QFLATBUFFERS_READERS_H

using namespace edu::xjtu::cs::cyx::qshield;

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

#endif//QFLATBUFFERS_READERS_H
