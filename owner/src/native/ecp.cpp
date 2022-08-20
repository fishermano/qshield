/*
 * Copyright (C) 2011-2016 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <stdlib.h>
#include <string.h>
#include "ecp.h"

#define MAC_KEY_SIZE       16

#define EC_DERIVATION_BUFFER_SIZE(label_length) ((label_length) +4)

const char str_SMK[] = "SMK";
const char str_SK[] = "SK";
const char str_MK[] = "MK";
const char str_VK[] = "VK";

// Derive key from shared key and key id.
// key id should be sample_derive_key_type_t.
bool derive_key(const lc_ec256_dh_shared_t *p_shared_key,
                uint8_t key_id,
                lc_aes_gcm_128bit_key_t *derived_key)
{
    lc_status_t ret = LC_SUCCESS;
    uint8_t cmac_key[MAC_KEY_SIZE];
    sgx_aes_gcm_128bit_key_t key_derive_key;
    
    memset(&cmac_key, 0, MAC_KEY_SIZE);

    ret = lc_rijndael128_cmac_msg((lc_cmac_128bit_key_t *) &*cmac_key,
                                  (uint8_t*)p_shared_key,
                                  sizeof(sgx_ec256_dh_shared_t),
                                  (lc_cmac_128bit_tag_t *)&key_derive_key);

    if (ret != LC_SUCCESS) {
      // memset here can be optimized away by compiler, so please use memset_s on
      // windows for production code and similar functions on other OSes.
      memset(&key_derive_key, 0, sizeof(key_derive_key));
      return false;
    }

    const char *label = NULL;
    uint32_t label_length = 0;
    switch (key_id) {
    case SAMPLE_DERIVE_KEY_SMK:
        label = str_SMK;
        label_length = sizeof(str_SMK) -1;
        break;
    case SAMPLE_DERIVE_KEY_SK:
        label = str_SK;
        label_length = sizeof(str_SK) -1;
        break;
    case SAMPLE_DERIVE_KEY_MK:
        label = str_MK;
        label_length = sizeof(str_MK) -1;
        break;
    case SAMPLE_DERIVE_KEY_VK:
        label = str_VK;
        label_length = sizeof(str_VK) -1;
        break;
    default:
        // memset here can be optimized away by compiler, so please use memset_s on
        // windows for production code and similar functions on other OSes.
        memset(&key_derive_key, 0, sizeof(key_derive_key));
        return false;
        break;
    }
    /* derivation_buffer = counter(0x01) || label || 0x00 || output_key_len(0x0080) */
    uint32_t derivation_buffer_length = EC_DERIVATION_BUFFER_SIZE(label_length);
    uint8_t *p_derivation_buffer = (uint8_t *) malloc(derivation_buffer_length);
    if (p_derivation_buffer == NULL) {
      // memset here can be optimized away by compiler, so please use memset_s on
      // windows for production code and similar functions on other OSes.
      memset(&key_derive_key, 0, sizeof(key_derive_key));
      return false;
    }
    memset(p_derivation_buffer, 0, derivation_buffer_length);

    /*counter = 0x01 */
    p_derivation_buffer[0] = 0x01;
    /*label*/
    memcpy(&p_derivation_buffer[1], label, label_length);
    /*output_key_len=0x0080*/
    uint16_t *key_len = (uint16_t *)(&(p_derivation_buffer[derivation_buffer_length - 2]));
    *key_len = 0x0080;


    ret = lc_rijndael128_cmac_msg((lc_cmac_128bit_key_t *)&key_derive_key,
                                  p_derivation_buffer,
                                  derivation_buffer_length,
                                  (lc_cmac_128bit_tag_t *)derived_key);
    free(p_derivation_buffer);
    // memset here can be optimized away by compiler, so please use memset_s on
    // windows for production code and similar functions on other OSes.
    memset(&key_derive_key, 0, sizeof(key_derive_key));
    if (ret != LC_SUCCESS) {
      return false;
    }
    return true;
}
