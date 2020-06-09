#ifndef QSERVICE_PROVIDER_H
#define QSERVICE_PROVIDER_H

#include <string>

#include "ServiceProvider.h"
#include "escheme/e-scheme.h"

class QServiceProvider: public ServiceProvider{
public:
  QServiceProvider(const std::string &spid, bool is_production, bool linkable_signature)
    :ServiceProvider(spid, is_production, linkable_signature) {}
  void setup(const char *param, int count);
private:
  e_sk g_e_sk;
};

extern QServiceProvider qservice_provider;

#endif//QSERVICE_PROVIDER_H
