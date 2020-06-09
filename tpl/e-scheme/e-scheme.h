#ifndef E_SCHEME_H
#define E_SCHEME_H


#include "pbc/pbc.h"
#include "sgx_tcrypto.h"

#define E_AESGCM_MAC_SIZE             16
#define SKBI_GTI_TAG_LEN 32
#define USER_NUM 10

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t e_aes_gcm_128bit_tag_t[E_AESGCM_MAC_SIZE];

typedef struct{
  element_t gti;
  sgx_sha256_hash_t skbi_tag;
}e_ska_compon;

typedef struct{
 element_t sk_a;
 pairing_t pairing;
 e_ska_compon comps[USER_NUM];
}e_ska;

typedef struct{
  element_t g;
  element_t m;
  element_t r;
  e_ska ska;
  element_t sk;
  element_t skb[USER_NUM];
}e_sk;


/**
core functions
*/
sgx_status_t ekeygen(e_sk *esk, const char* param, int count);
sgx_status_t eenc(element_t *sk,
                  const uint8_t *p_src, uint32_t src_len,
                  uint8_t *p_dst,
                  const uint8_t *p_iv, uint32_t iv_len,
                  const uint8_t *p_aad, uint32_t aad_len,
                  e_aes_gcm_128bit_tag_t *p_out_mac);
sgx_status_t edec(e_ska *sk_a, element_t *sk_b,
                  uint8_t *ciphertext, uint32_t ciphertext_len,
                  uint8_t *plaintext,
                  uint8_t *p_iv, uint32_t iv_len,
                  uint8_t *p_aad, uint32_t aad_len,
                  e_aes_gcm_128bit_tag_t *p_in_mac);

#ifdef __cplusplus
};
#endif

#endif//E_SCHEME_H
