/*
 * Copyright 2019 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <android/log.h>
#include <jni.h>

#include <memory>

#include "quake2vr.h"

#define JNI_METHOD(return_type, method_name) \
  JNIEXPORT return_type JNICALL              \
      Java_org_echoline_quake2vr_VrActivity_##method_name

namespace {

inline jlong jptr(Quake2VR* native_app) {
  return reinterpret_cast<intptr_t>(native_app);
}

inline Quake2VR* native(jlong ptr) {
  return reinterpret_cast<Quake2VR*>(ptr);
}

JavaVM* javaVm;

}  // anonymous namespace

extern "C" {

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
  javaVm = vm;
  return JNI_VERSION_1_6;
}

JNI_METHOD(jlong, nativeOnCreate)
(JNIEnv* env, jobject obj, jobject asset_mgr, jstring path, jobjectArray array) {
  jboolean isCopy = 1;
  jint l = env->GetArrayLength(array);
  char **a = (char**)malloc(l * sizeof(char*));
  for (int i = 0; i < l; i++)
    a[i] = const_cast<char *>(env->GetStringUTFChars(static_cast<jstring>(env->GetObjectArrayElement(array, i)), &isCopy));
  const char *t = env->GetStringUTFChars(path, &isCopy);
  jlong ret = jptr(new Quake2VR(javaVm, obj, asset_mgr, t, l, a));
  for (int i = 0; i < l; i++)
    env->ReleaseStringUTFChars(static_cast<jstring>(env->GetObjectArrayElement(array, i)), a[i]);
  free(a);
  env->ReleaseStringUTFChars(path, t);
  return ret;
}

JNI_METHOD(void, nativeOnDestroy)
(JNIEnv* env, jobject obj, jlong native_app) { delete native(native_app); }

JNI_METHOD(void, nativeOnPause)
(JNIEnv* env, jobject obj, jlong native_app) { native(native_app)->OnPause(); }

JNI_METHOD(void, nativeOnResume)
(JNIEnv* env, jobject obj, jlong native_app) { native(native_app)->OnResume(); }

JNI_METHOD(void, nativeSwitchViewer)
(JNIEnv* env, jobject obj, jlong native_app) { native(native_app)->SwitchViewer(); }

JNI_METHOD(void, nativeSetScreenParams)
(JNIEnv* env, jobject obj, jlong native_app, jint width, jint height) {
  native(native_app)->SetScreenParams(width, height);
}

JNI_METHOD(void, nativeRunMain)
(JNIEnv *env, jobject clazz, jlong native_app) {
  native(native_app)->RunMain();
}

}  // extern "C"
