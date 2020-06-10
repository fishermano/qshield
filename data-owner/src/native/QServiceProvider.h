#ifndef QSERVICE_PROVIDER_H
#define QSERVICE_PROVIDER_H

#include <string>

#include "ServiceProvider.h"
#include "escheme/e-scheme.h"

class QServiceProvider: public ServiceProvider{
public:
  QServiceProvider(const std::string &spid, bool is_production, bool linkable_signature)
    :ServiceProvider(spid, is_production, linkable_signature) {
      is_setup = false;
    }
  void setup(const char *param, int count);
  void encrypt(uint8_t *plaintext, uint32_t plaintext_length, uint8_t *ciphertext);
  uint32_t enc_size(uint32_t plaintext_size);

private:
  e_sk g_e_sk;
  bool is_setup;
};

extern QServiceProvider qservice_provider;

#endif//QSERVICE_PROVIDER_H
