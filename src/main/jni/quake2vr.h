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

#include <jni.h>

#include <memory>
#include <string>
#include <thread>
#include <vector>

//#include <GLES3/gl3.h>
#include "cardboard.h"

/**
 * This is a sample app for the Cardboard SDK. It loads a simple environment and
 * objects that you can click on.
 */
class Quake2VR {
 public:
  /**
   * Creates a HelloCardboardApp.
   *
   * @param vm JavaVM pointer.
   * @param obj Android activity object.
   * @param asset_mgr_obj The asset manager object.
   */
  Quake2VR(JavaVM *vm, jobject obj, const char *g);

  ~Quake2VR();

  /**
   * Sets screen parameters.
   *
   * @param width Screen width
   * @param height Screen height
   */
  void SetScreenParams(int width, int height);

  /**
   * Pauses head tracking.
   */
  void OnPause() const;

  /**
   * Resumes head tracking.
   */
  void OnResume();

  static void RunMain();

  static void SwitchViewer();

  CardboardHeadTracker* head_tracker_;

  CardboardDistortionRenderer* distortion_renderer_{};

  CardboardEyeTextureDescription right_eye_texture_description_;
  CardboardEyeTextureDescription left_eye_texture_description_;

  bool UpdateDeviceParams();

private:
  CardboardLensDistortion* lens_distortion_{};

  bool screen_params_changed_{};
  bool device_params_changed_{};

  float projection_matrices_[2][16]{};
  float eye_matrices_[2][16]{};

};
