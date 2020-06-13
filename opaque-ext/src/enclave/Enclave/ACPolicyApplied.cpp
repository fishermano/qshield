#include "ACPolicyApplied.h"

#include "FlatbuffersReaders.h"

#include "QFlatbuffersReaders.h"
#include "QFlatbuffersWriters.h"

#include "escheme/e-scheme.h"

#include <string.h>

extern unsigned char *sk_str;
extern sgx_aes_gcm_128bit_key_t shared_key;

using namespace edu::berkeley::cs::rise::opaque;
using namespace edu::xjtu::cs::cyx::qshield;

void ac_policy_applied(uint8_t *input_rows, size_t input_rows_length,
                        uint8_t *tk, size_t tk_length,
                        uint8_t **output_rows, size_t *output_rows_length) {

  //reconstruct shared secret key
  // uint32_t sk_len = element_length_in_bytes(g_sk.sk);

  // unsigned char sk_str[sk_len];
  // element_to_bytes(sk_str, g_sk.sk);
  // (void)sk_str;
  memcpy(reinterpret_cast<uint8_t *>(shared_key), reinterpret_cast<uint8_t *>(sk_str), SGX_AESGCM_KEY_SIZE);

  // char shared_key_str[16] = {'O','p','a','q','u','e',' ','d','e','v','e','l',' ','k','e','y'};
  // memcpy(reinterpret_cast<uint8_t *>(shared_key), reinterpret_cast<uint8_t *>(shared_key_str), SGX_AESGCM_KEY_SIZE);
  initKeySchedule();

  RowReader row_r(BufferRefView<tuix::EncryptedBlocks>(input_rows, input_rows_length));
  QTokenReader tk_r(BufferRefView<qix::QEncryptedToken>(tk, tk_length));
  QRowWriter row_w;

  uint32_t w = tk_r.w();
  uint32_t c = tk_r.c();
  uint8_t *sk_b_data = nullptr;
  uint32_t sk_b_length = 0;
  tk_r.sk_b(&sk_b_data, &sk_b_length);

  std::string u_id_str = "cyx";
  uint8_t *u_id = (uint8_t *)u_id_str.c_str();

  std::string sk_b_str = "Opaque devel key";
  uint8_t *sk_b = (uint8_t *)sk_b_str.c_str();

  if(cmp(sk_b_data, sk_b, sk_b_length) == -1){
    ocall_throw("sk_b transfers failed!!!");
  }

  flatbuffers::FlatBufferBuilder meta_builder;
  std::vector<flatbuffers::Offset<qix::QTrace>> trace_values(1);
  trace_values[0] = qix::CreateQTrace(
                      meta_builder,
                      meta_builder.CreateString("ac_policy_applied"),
                      meta_builder.CreateString("init"),
                      meta_builder.CreateString("null"));
  const flatbuffers::Offset<qix::QMeta> meta_new = qix::CreateQMeta(
                                  meta_builder,
                                  meta_builder.CreateVector(u_id, u_id_str.length()),
                                  c,
                                  w,
                                  meta_builder.CreateVector(trace_values));
  meta_builder.Finish(meta_new);
  row_w.set_meta(flatbuffers::GetRoot<qix::QMeta>(meta_builder.GetBufferPointer()));
  meta_builder.Clear();

  while (row_r.has_next()) {
    const tuix::Row *row = row_r.next();
    row_w.append(row);
  }

  row_w.output_buffer(output_rows, output_rows_length);
}
