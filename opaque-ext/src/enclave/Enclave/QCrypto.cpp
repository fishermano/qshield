#include "QCrypto.h"

#include <stdexcept>
#include <sgx_trts.h>
#include <sgx_tkey_exchange.h>

#include "common.h"
#include "util.h"
#include "Crypto.h"
#include "escheme/e-scheme.h"

e_ska sk_a;
element_t sk_b[USER_NUM];

extern sgx_aes_gcm_128bit_key_t shared_key;

sgx_aes_gcm_128bit_key_t rdd_key = {0};
std::unique_ptr<KeySchedule> rdd_ks;

sgx_aes_gcm_128bit_key_t tk_key = {0};
std::unique_ptr<KeySchedule> tk_ks;

void init_rdd_key_schedule(){
  // one enclave, for test only.
  // rdd_key should be negotiated by all enclaves through attestation for a practicl distributed environment.
  sgx_read_rand(reinterpret_cast<uint8_t *>(rdd_key), SGX_AESGCM_KEY_SIZE);
  rdd_ks.reset(new KeySchedule(reinterpret_cast<unsigned char*>(rdd_key), SGX_AESGCM_KEY_SIZE));
}

void init_tk_key_schedule(){
  // one enclave, for test only.
  // rdd_key should be negotiated by all enclaves through attestation for a practicl distributed environment.
  char tk_key_str[16] = {'O','p','a','q','u','e',' ','d','e','v','e','l',' ','k','e','y'};
  memcpy(reinterpret_cast<uint8_t *>(tk_key), reinterpret_cast<uint8_t *>(tk_key_str), SGX_AESGCM_KEY_SIZE);
  tk_ks.reset(new KeySchedule(reinterpret_cast<unsigned char*>(tk_key), SGX_AESGCM_KEY_SIZE));
}

void set_ska(sgx_ra_context_t context, uint8_t *msg4_bytes, uint32_t msg4_size){
  const q_ra_msg4_t *msg4 = reinterpret_cast<q_ra_msg4_t *>(msg4_bytes);

  if (msg4_size != sizeof(q_ra_msg4_t) + msg4->shared_key_size) {
    throw std::runtime_error("Remote attestation step 4: Invalid message size.");
  }

  sgx_ec_key_128bit_t sk_key;
  (void)context;
  sgx_check(sgx_ra_get_keys(context, SGX_RA_KEY_SK, &sk_key));

  uint8_t aes_gcm_iv[SGX_AESGCM_IV_SIZE] = {0};
  sgx_check(sgx_rijndael128GCM_decrypt(&sk_key,
                                       &msg4->shared_key_ciphertext[0], msg4->shared_key_size,
                                       &sk_a.ph,
                                       &aes_gcm_iv[0], SGX_AESGCM_IV_SIZE,
                                       nullptr, 0,
                                       &msg4->shared_key_mac));

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

void tk_decrypt(const uint8_t *ciphertext, uint32_t ciphertext_length, uint8_t *plaintext){

  if (!tk_ks) {
    throw std::runtime_error(
      "Cannot decrypt token without a negotiated key. Ensure all enclaves have same tk_key.");
  }

  uint32_t plaintext_length = dec_size(ciphertext_length);

  uint8_t *iv_ptr = (uint8_t *) ciphertext;
  uint8_t *ciphertext_ptr = (uint8_t *) (ciphertext + SGX_AESGCM_IV_SIZE);
  sgx_aes_gcm_128bit_tag_t *mac_ptr =
    (sgx_aes_gcm_128bit_tag_t *) (ciphertext + SGX_AESGCM_IV_SIZE + plaintext_length);

  AesGcm decipher(tk_ks.get(), iv_ptr, SGX_AESGCM_IV_SIZE);
  decipher.decrypt(ciphertext_ptr, plaintext_length, plaintext, plaintext_length);
  if (memcmp(mac_ptr, decipher.tag().t, SGX_AESGCM_MAC_SIZE) != 0) {
    printf("Decrypt: invalid mac\n");
  }
}

void init_pairing_env(const char *param, uint32_t count){
  e_sk g_e_sk;
  sgx_status_t ret = ekeygen(&g_e_sk, param, count);
  if(SGX_SUCCESS != ret){
    throw std::runtime_error(
      std::string("enclave e scheme key generate error. "));
  }
  memcpy(&sk_a.ph, &g_e_sk.ska.ph, sizeof(e_ska));
  memcpy(&sk_b, &g_e_sk.skb, USER_NUM*sizeof(element_t));
  (void)g_e_sk;
}
