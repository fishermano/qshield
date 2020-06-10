#include <jni.h>

#ifndef _Included_QSP
#define _Included_QSP
#ifdef __cplusplus
extern "C" {
#endif

  JNIEXPORT void JNICALL Java_edu_xjtu_cs_cyx_qshield_owner_SP_QInit
    (JNIEnv *, jobject, jbyteArray, jstring, jstring);

  JNIEXPORT void JNICALL Java_edu_xjtu_cs_cyx_qshield_owner_SP_QSPProcMsg0
    (JNIEnv *, jobject, jbyteArray);

  JNIEXPORT jbyteArray JNICALL Java_edu_xjtu_cs_cyx_qshield_owner_SP_QSPProcMsg1
    (JNIEnv *, jobject, jbyteArray);

  JNIEXPORT jbyteArray JNICALL Java_edu_xjtu_cs_cyx_qshield_owner_SP_QSPProcMsg3
    (JNIEnv *, jobject, jbyteArray);

#ifdef __cplusplus
}
#endif
#endif
