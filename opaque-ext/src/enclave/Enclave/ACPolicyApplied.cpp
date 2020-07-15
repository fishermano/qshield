#include "ACPolicyApplied.h"

#include "FlatbuffersReaders.h"

#include "QFlatbuffersReaders.h"
#include "QFlatbuffersWriters.h"

#include <string.h>

#include "escheme/e-scheme.h"

#include "qdebug.h"

extern sgx_aes_gcm_128bit_key_t shared_key;
extern e_ska sk_a;
extern element_t sk_b[USER_NUM];

using namespace edu::berkeley::cs::rise::opaque;
using namespace edu::xjtu::cs::cyx::qshield;

void ac_policy_applied(uint8_t *input_rows, size_t input_rows_length,
                        uint8_t *tk, size_t tk_length,
                        uint8_t **output_rows, size_t *output_rows_length) {

  #if QSHIELD_TP
    std::string u_id_str = "cyx";
    uint8_t *u_id = (uint8_t *)u_id_str.c_str();
    QTokenReader tk_r(BufferRefView<qix::QEncryptedToken>(tk, tk_length));
    uint32_t w = tk_r.w();
    uint32_t c = tk_r.c();
    uint8_t *sk_b_data = nullptr;
    uint32_t sk_b_length = 0;
    tk_r.sk_b(&sk_b_data, &sk_b_length);
    (void)sk_b_data;
    (void)sk_b_length;

    // std::string sk_b_str = "Opaque devel key";
    // uint8_t *sk_b = (uint8_t *)sk_b_str.c_str();
    //
    // if(cmp(sk_b_data, sk_b, sk_b_length) == -1){
    //   ocall_throw("sk_b transfers failed!!!");
    // }
  #else
    (void)tk;
    (void)tk_length;
  #endif

  #if QSHIELD_AC
    //reconstruct decryption key
    unsigned char *sk_bytes = (unsigned char *)malloc(SGX_AESGCM_KEY_SIZE);
    if (SGX_SUCCESS != erec(&sk_a, &sk_b[0], reinterpret_cast<void **>(&sk_bytes))){
        ocall_throw("sk reconstruction failed!!!");
    }
    memcpy(reinterpret_cast<uint8_t *>(shared_key), reinterpret_cast<uint8_t *>(sk_bytes), SGX_AESGCM_KEY_SIZE);
    initKeySchedule();
  #endif

  RowReader row_r(BufferRefView<tuix::EncryptedBlocks>(input_rows, input_rows_length));
  QRowWriter row_w;

  while (row_r.has_next()) {
    const tuix::Row *row = row_r.next();
    row_w.append(row);
  }

  #if QSHIELD_TP
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
  #endif

  row_w.output_buffer(output_rows, output_rows_length);
}
