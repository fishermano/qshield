#include <stdarg.h>
#include <stdio.h>      /* vsnprintf */
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "e-scheme.h"

bool etag_is_same(sgx_sha256_hash_t* skbi_gti_tag_user,sgx_sha256_hash_t* skbi_gti_tag_ska){
  for (size_t i = 0; i < SKBI_GTI_TAG_LEN; i++) {
  if ((*skbi_gti_tag_user)[i]!=(*skbi_gti_tag_ska)[i]) {
    return false;
   }
  }
  return true;
}

sgx_status_t epairinginit(pairing_t pairing, const char* param, int count){
    sgx_status_t ret = SGX_SUCCESS;
    if (count < 0){
      ret = SGX_ERROR_UNEXPECTED;
      return ret;
    }

    if(pairing_init_set_buf(pairing, param, count)){
      ret = SGX_ERROR_UNEXPECTED;
    }
    return ret;
}

sgx_status_t ekeygen(e_sk *esk, const char* param, int count){
  sgx_status_t ret = SGX_SUCCESS;
  /**
  init pairing
  */
  epairinginit(esk->ska.pairing, param, count);
  /**
  randomly picks up two numbers r and m */
  element_t tmp;
  element_t rm;
  element_t gm;
  element_init_Zr(esk->r,esk->ska.pairing);
  element_init_Zr(esk->m,esk->ska.pairing);
  element_init_Zr(rm,esk->ska.pairing);
  element_init_G1(esk->g,esk->ska.pairing);
  element_init_G1(tmp,esk->ska.pairing);
  element_init_G1(gm,esk->ska.pairing);
  element_random(esk->r);
  element_random(esk->m);
  element_random(esk->g);
  element_add(rm,esk->r,esk->m);
  element_pow_zn(tmp,esk->g,rm);

   /**
   generate sk
   */
  element_pow_zn(gm, esk->g,esk->m);
  element_init_GT(esk->sk,esk->ska.pairing);
  pairing_apply(esk->sk,esk->g,gm,esk->ska.pairing);


  element_init_GT(esk->ska.sk_a,esk->ska.pairing);
  pairing_apply(esk->ska.sk_a,esk->g,tmp,esk->ska.pairing);


  /**
  generate ska;
  */
  element_t m_2r;
  element_init_Zr(m_2r,esk->ska.pairing);
  element_add(m_2r,rm,esk->r);

  for(size_t i = 0; i < USER_NUM; i++){
    /**
    generate gti
    */
    element_t ti;
    element_init_Zr(ti, esk->ska.pairing);
    element_random(ti);
    element_init_G1(esk->ska.comps[i].gti,esk->ska.pairing);
    element_pow_zn(esk->ska.comps[i].gti,esk->g,ti);
    /**
    generate skbi
    */
    element_t m_2r_ti;
    element_init_Zr(m_2r_ti,esk->ska.pairing);
    element_init_G1(esk->skb[i],esk->ska.pairing);
    element_div(m_2r_ti,m_2r,ti);
    element_pow_zn(esk->skb[i],esk->g,m_2r_ti);

    /**
    generate skbi_tag
    */
    size_t len=element_length_in_bytes(esk->skb[i]);
    uint8_t skbi_str[len];
    element_to_bytes(skbi_str, esk->skb[i]);

    sgx_sha_state_handle_t sha_context;
    ret = sgx_sha256_init(&sha_context);
     if (ret != SGX_SUCCESS)
     {
         return ret;
     }
    ret = sgx_sha256_update(skbi_str,len, sha_context);
       if (ret != SGX_SUCCESS)
       {
         sgx_sha256_close(sha_context);
         return ret;
       }
    ret = sgx_sha256_get_hash(sha_context,&esk->ska.comps[i].skbi_tag);
    if (ret != SGX_SUCCESS)
    {
        sgx_sha256_close(sha_context);
        return ret;
    }
    sgx_sha256_close(sha_context);
    element_clear(ti);
  }
  element_clear(rm);
  element_clear(gm);
  element_clear(tmp);
  return ret;
}

sgx_status_t eenc(element_t *sk,
                  const uint8_t *p_src, uint32_t src_len,
                  uint8_t *p_dst,
                  const uint8_t *p_iv, uint32_t iv_len,
                  const uint8_t *p_aad, uint32_t aad_len,
                  e_aes_gcm_128bit_tag_t *p_out_mac){
  sgx_status_t ret = SGX_SUCCESS;

  uint32_t sk_len = element_length_in_bytes(*sk);
  uint8_t sk_str[sk_len];
  element_to_bytes(sk_str, *sk);
  uint8_t u_key[16];
  strncpy(u_key, sk_str, 16);
  ret = sgx_rijndael128GCM_encrypt(&u_key,
                                    p_src, src_len,
                                    p_dst,
                                    p_iv, iv_len,
                                    p_aad, aad_len,
                                    p_out_mac);
  return ret;
}

sgx_status_t edec(e_ska *sk_a, element_t *sk_b,
                  uint8_t *ciphertext, uint32_t ciphertext_len,
                  uint8_t *plaintext,
                  uint8_t *p_iv, uint32_t iv_len,
                  uint8_t *p_aad, uint32_t aad_len,
                  e_aes_gcm_128bit_tag_t *p_in_mac){
  sgx_status_t ret = SGX_SUCCESS;

  sgx_sha256_hash_t sk_b_tag;

  uint32_t sk_b_len = element_length_in_bytes(*sk_b);
  uint8_t sk_b_str[sk_b_len];
  element_to_bytes(sk_b_str, *sk_b);
  sgx_sha_state_handle_t sha_context;
  ret = sgx_sha256_init(&sha_context);
  if(SGX_SUCCESS != ret){
    return ret;
  }
  ret = sgx_sha256_update(sk_b_str, sk_b_len, sha_context);
  if(SGX_SUCCESS != ret){
    return ret;
  }
  ret = sgx_sha256_get_hash(sha_context, &sk_b_tag);
  if(SGX_SUCCESS != ret){
    sgx_sha256_close(sha_context);
    return ret;
  }
  sgx_sha256_close(sha_context);

  //find the gti of sk_b
  bool flag = false;
  element_t gti;
  element_init_G1(gti, sk_a->pairing);
  for(int i = 0; i < USER_NUM; i++){
    flag=etag_is_same(&sk_b_tag, &(sk_a->comps[i].skbi_tag));
    if(flag){
      element_set(gti, sk_a->comps[i].gti);
      break;
    }
  }
  if(!flag){
    return SGX_ERROR_UNEXPECTED;
  }

  //get sk
  element_t tmp1, tmp2;
  element_init_GT(tmp1, sk_a->pairing);
  element_init_GT(tmp2, sk_a->pairing);
  pairing_apply(tmp1, gti, *sk_b, sk_a->pairing);
  element_mul(tmp2, sk_a->sk_a, sk_a->sk_a);
  element_t sk;
  element_init_GT(sk, sk_a->pairing);
  element_div(sk, tmp2, tmp1);

  //decrypt aes_gcm_data_t
  uint32_t len = element_length_in_bytes(sk);
  uint8_t sk_str[len];
  element_to_bytes(sk_str, sk);
  uint8_t u_key[16];
  strncpy(u_key, sk_str, 16);
  ret = sgx_rijndael128GCM_decrypt(&u_key,
                                    ciphertext, ciphertext_len,
                                    plaintext,
                                    p_iv, iv_len,
                                    p_aad, aad_len,
                                    p_in_mac);
  return ret;
}
