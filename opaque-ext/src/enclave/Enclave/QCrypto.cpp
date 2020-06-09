#include "QCrypto.h"

#include <stdexcept>
#include <sgx_trts.h>
#include <sgx_tkey_exchange.h>

#include "common.h"
#include "util.h"
#include "Crypto.h"

sgx_aes_gcm_128bit_key_t rdd_key = {0};

std::unique_ptr<KeySchedule> rdd_ks;

void init_rdd_key_schedule(){
  // one enclave, for test only.
  // rdd_key should be negotiated by all enclaves through attestation for a practicl distributed environment.
  sgx_read_rand(reinterpret_cast<uint8_t *>(rdd_key), SGX_AESGCM_KEY_SIZE);
  rdd_ks.reset(new KeySchedule(reinterpret_cast<unsigned char*>(rdd_key), SGX_AESGCM_KEY_SIZE));
}

void rdd_encrypt(uint8_t *plaintext, uint32_t plaintext_length, uint8_t *ciphertext){

  if(!rdd_ks){
    throw std::runtime_error(
      "Cannot encrypt rdd without a negotiated key. Ensure all enclaves have same rdd_key.");
  }

  uint8_t *iv_ptr = ciphertext;
  uint8_t *ciphertext_ptr = ciphertext + SGX_AESGCM_IV_SIZE;
  sgx_aes_gcm_128bit_tag_t *mac_ptr =
    (sgx_aes_gcm_128bit_tag_t *) (ciphertext + SGX_AESGCM_IV_SIZE + plaintext_length);

  sgx_read_rand(iv_ptr, SGX_AESGCM_IV_SIZE);

  AesGcm cipher(rdd_ks.get(), iv_ptr, SGX_AESGCM_IV_SIZE);
  cipher.encrypt(plaintext, plaintext_length, ciphertext_ptr, plaintext_length);
  memcpy(mac_ptr, cipher.tag().t, SGX_AESGCM_MAC_SIZE);
}

void rdd_decrypt(const uint8_t *ciphertext, uint32_t ciphertext_length, uint8_t *plaintext){

  if (!rdd_ks) {
    throw std::runtime_error(
      "Cannot decrypt rdd without a negotiated key. Ensure all enclaves have same rdd_key.");
  }

  uint32_t plaintext_length = dec_size(ciphertext_length);

  uint8_t *iv_ptr = (uint8_t *) ciphertext;
  uint8_t *ciphertext_ptr = (uint8_t *) (ciphertext + SGX_AESGCM_IV_SIZE);
  sgx_aes_gcm_128bit_tag_t *mac_ptr =
    (sgx_aes_gcm_128bit_tag_t *) (ciphertext + SGX_AESGCM_IV_SIZE + plaintext_length);

  AesGcm decipher(rdd_ks.get(), iv_ptr, SGX_AESGCM_IV_SIZE);
  decipher.decrypt(ciphertext_ptr, plaintext_length, plaintext, plaintext_length);
  if (memcmp(mac_ptr, decipher.tag().t, SGX_AESGCM_MAC_SIZE) != 0) {
    printf("Decrypt: invalid mac\n");
  }
}
