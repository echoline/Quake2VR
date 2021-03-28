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
    char **storagedirs = nullptr;
    int android_main(int argc, char **argv);
    #include "../yquake2/src/common/header/common.h"
    int deviceWidth, deviceHeight;
    void CardboardSetup();
}

static constexpr uint64_t kNanosInSeconds = 1000000000;
static constexpr uint64_t kPredictionTimeWithoutVsyncNanos = 50000000;
static constexpr float kZNear = 0.1f;
static constexpr float kZFar = 100.f;

long GetMonotonicTimeNano() {
  struct timespec res{};
  clock_gettime(CLOCK_MONOTONIC, &res);
  return res.tv_sec * kNanosInSeconds + res.tv_nsec;
}

Quake2VR *thisApp;

Quake2VR::Quake2VR(JavaVM *vm, jobject obj, jobject asset_mgr_obj, const char *g, int len, char **pString) {
  JNIEnv* env;
  vm->GetEnv((void**)&env, JNI_VERSION_1_6);
  gamedir = strdup(g);
  storagedirs = static_cast<char **>(calloc(len + 1, sizeof(char *)));
  for (int i = 0; i < len; i++)
      storagedirs[i] = strdup(pString[i]);
  Cardboard_initializeAndroid(vm, obj);
  head_tracker_ = CardboardHeadTracker_create();

  thisApp = this;
}

Quake2VR::~Quake2VR() {
  CardboardHeadTracker_destroy(head_tracker_);
  CardboardLensDistortion_destroy(lens_distortion_);
  CardboardDistortionRenderer_destroy(distortion_renderer_);
}

void Quake2VR::SetScreenParams(int width, int height) {
  deviceWidth = width;
  deviceHeight = height;
  screen_params_changed_ = true;
}

void Quake2VR::OnPause() const { CardboardHeadTracker_pause(head_tracker_); }

void Quake2VR::OnResume() {
  CardboardHeadTracker_resume(head_tracker_);

  // Parameters may have changed.
  device_params_changed_ = true;

  // Check for device parameters existence in external storage. If they're
  // missing, we must scan a Cardboard QR code and save the obtained parameters.
  uint8_t* buffer;
  int size;
  CardboardQrCode_getSavedDeviceParams(&buffer, &size);
  if (size == 0) {
    SwitchViewer();
  }
  CardboardQrCode_destroy(buffer);
}

void Quake2VR::SwitchViewer() {
  CardboardQrCode_scanQrCodeAndSaveDeviceParams();
}

bool Quake2VR::UpdateDeviceParams() {
    // Checks if screen or device parameters changed
    if (!screen_params_changed_ && !device_params_changed_) {
        return true;
    }

    // Get saved device parameters
    uint8_t* buffer;
    int size;
    CardboardQrCode_getSavedDeviceParams(&buffer, &size);

    // If there are no parameters saved yet, returns false.
    if (size == 0) {
        return false;
    }

    if (lens_distortion_ != nullptr)
        CardboardLensDistortion_destroy(lens_distortion_);
    lens_distortion_ = CardboardLensDistortion_create(buffer, size, deviceWidth, deviceHeight);

    CardboardQrCode_destroy(buffer);

    CardboardSetup();

    if (distortion_renderer_ != nullptr)
        CardboardDistortionRenderer_destroy(distortion_renderer_);
    distortion_renderer_ = CardboardOpenGlEs3DistortionRenderer_create();

    CardboardMesh left_mesh;
    CardboardMesh right_mesh;
    CardboardLensDistortion_getDistortionMesh(lens_distortion_,kLeft, &left_mesh);
    CardboardLensDistortion_getDistortionMesh(lens_distortion_,kRight, &right_mesh);

    CardboardDistortionRenderer_setMesh(distortion_renderer_, &left_mesh,kLeft);
    CardboardDistortionRenderer_setMesh(distortion_renderer_, &right_mesh,kRight);

    // Get eye matrices
    CardboardLensDistortion_getEyeFromHeadMatrix(
            lens_distortion_, kLeft, eye_matrices_[0]);
    CardboardLensDistortion_getEyeFromHeadMatrix(
            lens_distortion_, kRight, eye_matrices_[1]);
    CardboardLensDistortion_getProjectionMatrix(
            lens_distortion_, kLeft, kZNear, kZFar, projection_matrices_[0]);
    CardboardLensDistortion_getProjectionMatrix(
            lens_distortion_, kRight, kZNear, kZFar, projection_matrices_[1]);

    screen_params_changed_ = false;
    device_params_changed_ = false;

    return true;
}

extern "C" {
    float *GetHeadPose() {
        float o[4];
        float p[3];
        static float ret[2];
        long monotonic_time_nano = GetMonotonicTimeNano();
        monotonic_time_nano += kPredictionTimeWithoutVsyncNanos;
        CardboardHeadTracker_getPose(thisApp->head_tracker_, monotonic_time_nano, p, o);
        ret[0] = asin(2 * (o[3] * o[0] + o[2] * o[1]));
        ret[1] = -atan2(2 * (o[3] * o[1] + o[0] * o[2]), 1 - 2 * (o[0] * o[0] + o[1] * o[1]));
        return ret;
    }

    void SetTexture(unsigned int tex) {
        thisApp->left_eye_texture_description_.texture = tex;
        thisApp->left_eye_texture_description_.left_u = 0;
        thisApp->left_eye_texture_description_.right_u = 0.5;
        thisApp->left_eye_texture_description_.top_v = 1;
        thisApp->left_eye_texture_description_.bottom_v = 0;

        thisApp->right_eye_texture_description_.texture = tex;
        thisApp->right_eye_texture_description_.left_u = 0.5;
        thisApp->right_eye_texture_description_.right_u = 1;
        thisApp->right_eye_texture_description_.top_v = 1;
        thisApp->right_eye_texture_description_.bottom_v = 0;
    }

    void RenderEyeToDisplay(unsigned int display) {
      if (!thisApp->UpdateDeviceParams()) return;

      CardboardDistortionRenderer_renderEyeToDisplay(thisApp->distortion_renderer_, display,
              0, 0, deviceWidth, deviceHeight,
                &thisApp->left_eye_texture_description_, &thisApp->right_eye_texture_description_);
    }
}

void Quake2VR::RunMain() {
  char *argv[] = {(char*)"quake2vr", (char*)"-cfgdir", gamedir, (char*)"+set", (char*)"game", (char*)"demo", nullptr};
  android_main(6, argv);
}
