#include <jni.h>

#ifndef _Included_SP
#define _Included_SP
#ifdef __cplusplus
extern "C" {
#endif
  JNIEXPORT void JNICALL Java_edu_xjtu_cs_cyx_qshield_owner_SP_Init(
    JNIEnv *, jobject, jbyteArray, jstring);

  JNIEXPORT void JNICALL Java_edu_xjtu_cs_cyx_qshield_owner_SP_SPProcMsg0(
    JNIEnv *, jobject, jbyteArray);

  JNIEXPORT jbyteArray JNICALL Java_edu_xjtu_cs_cyx_qshield_owner_SP_SPProcMsg1(
    JNIEnv *, jobject, jbyteArray);

  JNIEXPORT jbyteArray JNICALL Java_edu_xjtu_cs_cyx_qshield_owner_SP_SPProcMsg3(
    JNIEnv *, jobject, jbyteArray);

#ifdef __cplusplus
}
#endif
#endif
