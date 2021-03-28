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

#include "quake2vr.h"

#include <android/log.h>

#include <array>
#include <cmath>
#include <fstream>

extern "C" {
    char *gamedir = nullptr;
    int android_main(int argc, char **argv);
    #include "../yquake2/src/common/header/common.h"
    int deviceWidth, deviceHeight;
}

static constexpr uint64_t kNanosInSeconds = 1000000000;
constexpr uint64_t kPredictionTimeWithoutVsyncNanos = 50000000;

long GetMonotonicTimeNano() {
  struct timespec res{};
  clock_gettime(CLOCK_MONOTONIC, &res);
  return res.tv_sec * kNanosInSeconds + res.tv_nsec;
}

Quake2VR *thisApp;

Quake2VR::Quake2VR(JavaVM* vm, jobject obj, jobject asset_mgr_obj, const char *g)
    : head_tracker_(nullptr) {
  JNIEnv* env;
  vm->GetEnv((void**)&env, JNI_VERSION_1_6);
  gamedir = strdup(g);

  Cardboard_initializeAndroid(vm, obj);
  head_tracker_ = CardboardHeadTracker_create();

  thisApp = this;
}

Quake2VR::~Quake2VR() {
  CardboardHeadTracker_destroy(head_tracker_);
}

void Quake2VR::SetScreenParams(int width, int height) {
  deviceWidth = width;
  deviceHeight = height;
}

void Quake2VR::OnPause() const { CardboardHeadTracker_pause(head_tracker_); }

void Quake2VR::OnResume() const { CardboardHeadTracker_resume(head_tracker_); }

extern "C" {
  float *GetHeadPose() {
    float o[4];
    float p[3];
    static float ret[2];
    float t;
    long monotonic_time_nano = GetMonotonicTimeNano();
    monotonic_time_nano += kPredictionTimeWithoutVsyncNanos;
    CardboardHeadTracker_getPose(thisApp->head_tracker_, monotonic_time_nano, p, o);
    ret[0] = asin(2 * (o[3] * o[0] + o[2] * o[1]));
    ret[1] = -atan2(2 * (o[3] * o[1] + o[0] * o[2]), 1 - 2 * (o[0] * o[0] + o[1] * o[1]));
    return ret;
  }
}

void Quake2VR::RunMain() {
  char *argv[] = {(char*)"quake2vr", (char*)"-cfgdir", gamedir, (char*)"-datadir", gamedir, (char*)"-portable", nullptr};
  strcpy(datadir, "");
  strcpy(cfgdir, gamedir);
  android_main(6, argv);
}
