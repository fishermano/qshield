#ifndef QSERVICE_PROVIDER_H
#define QSERVICE_PROVIDER_H

#include <string>

#include "ServiceProvider.h"
#include "escheme/e-scheme.h"
#include "sample_libcrypto.h"

#include <sgx_key_exchange.h>

class QServiceProvider: public ServiceProvider{
public:
  QServiceProvider(const std::string &spid, bool is_production, bool linkable_signature)
    :ServiceProvider(spid, is_production, linkable_signature) {
      is_setup = false;
    }
  void setup(const char *param, int count);
  void encrypt(uint8_t *plaintext, uint32_t plaintext_length, uint8_t *ciphertext);
  uint32_t enc_size(uint32_t plaintext_size);
  element_t *get_skb(uint32_t id);
  void get_sk(uint8_t **sk);

  //overridden
  std::unique_ptr<q_ra_msg4_t> process_msg3(
    sgx_ra_msg3_t *msg3, uint32_t msg3_size, uint32_t *msg4_size);

private:
  e_sk g_e_sk;
  bool is_setup;
};

extern QServiceProvider qservice_provider;

#endif//QSERVICE_PROVIDER_H
