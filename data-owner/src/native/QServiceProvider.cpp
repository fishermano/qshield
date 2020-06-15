#include <openssl/pem.h>
#include <cassert>
#include <fstream>
#include <iomanip>
#include <sgx_tcrypto.h>

#include "ecp.h"
#include "ias_ra.h"
#include "iasrequest.h"
#include "crypto.h"
#include "base64.h"
#include "json.hpp"

#include "QServiceProvider.h"

extern void lc_check(lc_status_t ret);

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
  return plaintext_size + SAMPLE_AESGCM_IV_SIZE + SAMPLE_AESGCM_MAC_SIZE;
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

  /*
   * Error: undefined symbol: memset_s
   */
  // uint8_t *iv_ptr = ciphertext;
  // uint8_t *ciphertext_ptr = ciphertext + SGX_AESGCM_IV_SIZE;
  // sgx_aes_gcm_128bit_tag_t *mac_ptr =
  //   (sgx_aes_gcm_128bit_tag_t *) (ciphertext + SGX_AESGCM_IV_SIZE + plaintext_length);
  // sgx_status_t ret = eenc(&g_e_sk.sk,
  //                           plaintext, plaintext_length,
  //                           ciphertext_ptr,
  //                           iv_ptr, SGX_AESGCM_IV_SIZE,
  //                           NULL, 0,
  //                           mac_ptr);
  // if(SGX_SUCCESS != ret){
  //   throw std::runtime_error(
  //     std::string("e scheme data encrypt error. "));
  // }

  uint8_t *iv_ptr = ciphertext;
  uint8_t *ciphertext_ptr = ciphertext + SAMPLE_AESGCM_IV_SIZE;
  sample_aes_gcm_128bit_tag_t *mac_ptr =
    (sample_aes_gcm_128bit_tag_t *) (ciphertext + SAMPLE_AESGCM_IV_SIZE + plaintext_length);
  sample_status_t ret = SAMPLE_SUCCESS;

  uint32_t sk_len = element_length_in_bytes(g_e_sk.sk);
  unsigned char sk_str[sk_len];
  element_to_bytes(sk_str, g_e_sk.sk);
  uint8_t u_key[SAMPLE_AESGCM_KEY_SIZE];
  memcpy(u_key, reinterpret_cast<uint8_t *>(sk_str), SAMPLE_AESGCM_KEY_SIZE);

  ret = sample_rijndael128GCM_encrypt(reinterpret_cast<sample_aes_gcm_128bit_key_t *>(u_key),
                                    plaintext, plaintext_length,
                                    ciphertext_ptr,
                                    iv_ptr, SAMPLE_AESGCM_IV_SIZE,
                                    NULL, 0,
                                    mac_ptr);

  if(SAMPLE_SUCCESS != ret){
    throw std::runtime_error(
      std::string("e scheme data encrypt error. "));
  }
}

element_t *QServiceProvider::get_skb(uint32_t id){
  if (id >= USER_NUM){
    throw std::runtime_error(
      std::string("no such a user. "));
  }

  return &g_e_sk.skb[id];
}

std::unique_ptr<q_ra_msg4_t> QServiceProvider::process_msg3(
  sgx_ra_msg3_t *msg3, uint32_t msg3_size, uint32_t *msg4_size){

  if (msg3_size < sizeof(sgx_ra_msg3_t)) {
    throw std::runtime_error("process_msg3: msg3 is invalid (expected sgx_ra_msg3_t).");
  }

  // "Verify that Ga in msg3 matches Ga in msg1."
  if (memcmp(&sp_db.g_a, &msg3->g_a, sizeof(lc_ec256_public_t))) {
    throw std::runtime_error("process_msg3: g_a mismatch.");
  }

  // "Verify CMAC_SMK(M)."
  uint32_t mac_size = msg3_size - sizeof(sgx_mac_t);
  const uint8_t *msg3_cmaced = reinterpret_cast<const uint8_t*>(msg3) + sizeof(sgx_mac_t);
  lc_cmac_128bit_tag_t mac;
  lc_check(lc_rijndael128_cmac_msg(&sp_db.smk_key, msg3_cmaced, mac_size, &mac));
  if (memcmp(&msg3->mac, mac, sizeof(mac))) {
    throw std::runtime_error("process_msg3: MAC mismatch.");
  }

  // "Extract the quote from msg3."
  sample_quote_t *quote = reinterpret_cast<sample_quote_t *>(msg3->quote);
  // Use the fact that msg3.quote is a flexible array to calculate the quote size
  uint32_t quote_size = msg3_size - sizeof(sgx_ra_msg3_t);

  // "Verify that the first 32-bytes of the report data match the SHA-256 digest of
  // (Ga || Gb || VK), where || denotes concatenation. VK is derived by performing an AES-128 CMAC
  // over the following byte sequence, using the KDK as the key:
  // 0x01 || "VK" || 0x00 || 0x80 || 0x00
  lc_sha_state_handle_t sha_handle;
  lc_sha256_init(&sha_handle);
  lc_sha256_update(reinterpret_cast<const uint8_t *>(&sp_db.g_a), sizeof(sp_db.g_a), sha_handle);
  lc_sha256_update(reinterpret_cast<const uint8_t *>(&sp_db.g_b), sizeof(sp_db.g_b), sha_handle);
  lc_sha256_update(reinterpret_cast<const uint8_t *>(&sp_db.vk_key), sizeof(sp_db.vk_key), sha_handle);
  lc_sha256_hash_t hash;
  lc_sha256_get_hash(sha_handle, &hash);
  lc_sha256_close(sha_handle);
  if (memcmp(reinterpret_cast<const uint8_t *>(&hash),
             reinterpret_cast<const uint8_t *>(&quote->report_body.report_data),
             LC_SHA256_HASH_SIZE)) {
    throw std::runtime_error("process_msg3: Report data digest mismatch.");
  }

  // Verify that the EPID group ID in the quote matches the one from msg1
  if (memcmp(sp_db.gid, quote->epid_group_id, sizeof(sgx_epid_group_id_t))) {
    throw std::runtime_error("process_msg3: EPID GID mismatch");
  }

  // Encode the quote as base64
  std::string quote_base64(base64_encode(reinterpret_cast<char *>(quote), quote_size));

  // "Submit the quote to IAS, calling the API function to verify attestation evidence."
  std::string content;
  try {
    if (!ias) {
      throw std::runtime_error("process_msg3 called before ensure_ias_connection");
    }
    IAS_Request req(ias.get(), ias_api_version);

    std::map<std::string, std::string> payload;
    payload.insert(std::make_pair("isvEnclaveQuote", quote_base64));

    std::vector<std::string> messages;
    ias_check(req.report(payload, content, messages));
  } catch (const std::runtime_error &e) {
    if (require_attestation) {
      throw;
    }
  }

  if (!content.empty()) {
    json::JSON reportObj = json::JSON::Load(content);

    if (!reportObj.hasKey("version")) {
      throw std::runtime_error("IAS: Report is missing an API version.");
    }

    unsigned int rversion = (unsigned int)reportObj["version"].ToInt();
    if (rversion != ias_api_version) {
      throw std::runtime_error(
        std::string("IAS: Report version ")
        + std::to_string(rversion)
        + std::string(" does not match API version ")
        + std::to_string(ias_api_version));
    }

    // "Extract the attestation status for the enclave.
    // Decide whether or not to trust the enclave."
    if (reportObj["isvEnclaveQuoteStatus"].ToString().compare("OK") == 0) {
      // Enclave is trusted
    } else if (reportObj["isvEnclaveQuoteStatus"].ToString().compare("CONFIGURATION_NEEDED") == 0) {
      throw std::runtime_error(
        "Enclave not trusted. IAS reports CONFIGURATION_NEEDED. Check the BIOS.");
    } else if (reportObj["isvEnclaveQuoteStatus"].ToString().compare("GROUP_OUT_OF_DATE") != 0) {
      throw std::runtime_error(
        "Enclave not trusted. IAS reports GROUP_OUT_OF_DATE. Update the BIOS.");
    } else {
      if (require_attestation) {
        throw std::runtime_error("Enclave not trusted.");
      }
    }
  }

  // Generate msg4, containing ska to be sent to the enclave.
  uint32_t sk_a_len = sizeof(e_ska);
  *msg4_size = sizeof(q_ra_msg4_t) + sk_a_len;
  void *ct = (void *)malloc(*msg4_size);
  uint8_t aes_gcm_iv[LC_AESGCM_IV_SIZE] = {0};
  lc_check(lc_rijndael128GCM_encrypt(&sp_db.sk_key,
                                     &g_e_sk.ska.ph, sk_a_len,
                                     ((q_ra_msg4_t *)ct)->shared_key_ciphertext,
                                     &aes_gcm_iv[0], LC_AESGCM_IV_SIZE,
                                     nullptr, 0,
                                     &((q_ra_msg4_t *)ct)->shared_key_mac));
  ((q_ra_msg4_t *)ct)->shared_key_size = sk_a_len;

  std::unique_ptr<q_ra_msg4_t> msg4((q_ra_msg4_t *)ct);

  return msg4;
}

void QServiceProvider::get_sk(uint8_t **sk){
  uint32_t sk_len = element_length_in_bytes(g_e_sk.sk);
  unsigned char sk_str[sk_len];
  element_to_bytes(sk_str, g_e_sk.sk);
  memcpy(*sk, reinterpret_cast<uint8_t *>(sk_str), SAMPLE_AESGCM_KEY_SIZE);
}
