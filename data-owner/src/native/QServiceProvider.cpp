#include "QServiceProvider.h"

// Your 16-byte Service Provider ID (SPID), assigned by Intel.
const uint8_t spid[] = {0xA4,0x62,0x09,0x2E,0x1B,0x59,0x26,0xDF,0x44,0x69,0xD5,0x61,0xE2,0x54,0xB0,0x1E};

// The EPID security policy you chose (linkable -> true, unlinkable -> false).
const bool linkable_signature = false;

QServiceProvider qservice_provider(
  std::string(reinterpret_cast<const char *>(spid), sizeof(spid)),
  linkable_signature,
  false);

void QServiceProvider::setup(const char *param, int count){
  sgx_status_t ret = ekeygen(&g_e_sk, param, count);
  if(SGX_SUCCESS != ret){
    throw std::runtime_error(
      std::string("e scheme key generate error. "));
  }
  is_setup = true;
}

uint32_t QServiceProvider::enc_size(uint32_t plaintext_size) {
  return plaintext_size + E_AESGCM_IV_SIZE + E_AESGCM_MAC_SIZE;
}

void QServiceProvider::encrypt(uint8_t *plaintext, uint32_t plaintext_length,
                                uint8_t *ciphertext){
  if(!is_setup){
    throw std::runtime_error(
      "Cannot encrypt without a key. Please call setup() first.");
  }

  if(!ciphertext){
    throw std::runtime_error(
      "memory of ciphertext should be malloced by the caller.");
  }

  uint8_t *iv_ptr = ciphertext;
  uint8_t *ciphertext_ptr = ciphertext + E_AESGCM_IV_SIZE;
  e_aes_gcm_128bit_tag_t *mac_ptr =
    (e_aes_gcm_128bit_tag_t *) (ciphertext + E_AESGCM_IV_SIZE + plaintext_length);

  sgx_status_t ret = eenc(&g_e_sk.sk,
                            plaintext, plaintext_length,
                            ciphertext_ptr,
                            iv_ptr, E_AESGCM_IV_SIZE,
                            NULL, 0,
                            mac_ptr);
  if(SGX_SUCCESS != ret){
    throw std::runtime_error(
      std::string("e scheme data encrypt error. "));
  }
}
