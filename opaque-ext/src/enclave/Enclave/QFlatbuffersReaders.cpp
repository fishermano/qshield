#include "QFlatbuffersReaders.h"

void QEncryptedTokenToQTokenReader::reset(const qix::QEncryptedToken *encrypted_token){
  const size_t tk_len = dec_size(encrypted_token->enc_tk()->size());
  tk_buf.reset(new uint8_t[tk_len]);
  decrypt(encrypted_token->enc_tk()->data(), encrypted_token->enc_tk()->size(), tk_buf.get());
  BufferRefView<qix::QToken> buf(tk_buf.get(), tk_len);
  buf.verify();

  tk = buf.root();
  initialized = true;
}

QTokenReader::QTokenReader(BufferRefView<qix::QEncryptedToken> buf){
  buf.verify();
  reset(buf.root());
}

void QTokenReader::reset(const qix::QEncryptedToken *encrypted_token){
  this->encrypted_token = encrypted_token;
  init_enc_tk_reader();
}

void QTokenReader::init_enc_tk_reader(){
  enc_tk_reader.reset(encrypted_token);
}

uint32_t QTokenReader::w(){
  return enc_tk_reader.get_token()->w();
}

uint32_t QTokenReader::c(){
  return enc_tk_reader.get_token()->c();
}

void QTokenReader::sk_b(uint8_t **data, uint32_t *size){
  *data = (uint8_t *)enc_tk_reader.get_token()->sk_b()->data();
  *size = enc_tk_reader.get_token()->sk_b()->size();
}
