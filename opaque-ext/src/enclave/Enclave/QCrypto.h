#include <sgx_tcrypto.h>
#include <sgxaes.h>
#include <sgx_key_exchange.h>

#ifndef QCRYPTO_H
#define QCRYPTO_H

void init_rdd_key_schedule();

void init_tk_key_schedule();

void rdd_encrypt(uint8_t *plaintext, uint32_t plaintext_length, uint8_t *ciphertext);

void rdd_decrypt(const uint8_t *ciphertext, uint32_t ciphertext_length, uint8_t *plaintext);

void tk_decrypt(const uint8_t *ciphertext, uint32_t ciphertext_length, uint8_t *plaintext);

void set_ska(sgx_ra_context_t context, uint8_t *msg4_bytes, uint32_t msg4_size);

//for test only
void init_pairing_env(const char *param, uint32_t count);

#endif//QCRYPTO_H
