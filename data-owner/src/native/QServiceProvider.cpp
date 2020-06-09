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
      std::string("e scheme key generate error: "));
  }
}
