#include <jni.h>
#include <dlfcn.h>

#define JNI_METHOD(return_type, method_name) \
  JNIEXPORT return_type JNICALL              \
      Java_org_echoline_quake2vr_Loader_##method_name

extern "C" {

JNI_METHOD(jboolean, nativeLoadLibrary)
(JNIEnv *env, jclass clazz, jstring name) {
    return dlopen(env->GetStringUTFChars(name, NULL), RTLD_LAZY | RTLD_GLOBAL) != nullptr;
}

}