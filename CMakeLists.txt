# Copyright 2020 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.4.1)

# C++ flags.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

add_definitions(-DYQ2OSTYPE="Android" -DYQ2ARCH="${CMAKE_ANDROID_ARCH_ABI}")

# Standard Android dependencies
find_library(android-lib android)
find_library(GLESv2-lib GLESv2)
find_library(GLESv3-lib GLESv3)
find_library(log-lib log)

set(libs_dir ${CMAKE_CURRENT_SOURCE_DIR}/libraries)

# === Cardboard Sample ===
# Sources
file(GLOB yquake2_srcs "src/main/jni/*.cc"
        "src/main/yquake2/src/common/*.c"
        "src/main/yquake2/src/common/shared/*.c"
        "src/main/yquake2/src/common/unzip/*.c"
        "src/main/yquake2/src/backends/generic/*.c"
        "src/main/yquake2/src/backends/unix/*.c"
        "src/main/yquake2/src/backends/unix/shared/hunk.c"
        "src/main/yquake2/src/client/*.c"
        "src/main/yquake2/src/client/sound/ogg.c"
        "src/main/yquake2/src/client/sound/sound.c"
        "src/main/yquake2/src/client/sound/wave.c"
        "src/main/yquake2/src/client/sound/sdl.c"
        "src/main/yquake2/src/client/menu/*.c"
        "src/main/yquake2/src/client/vid/vid.c"
        "src/main/yquake2/src/client/vid/glimp_sdl.c"
        "src/main/yquake2/src/client/input/sdl.c"
        "src/main/yquake2/src/server/*.c"
        #"src/main/yquake2/src/client/refresh/soft/*.c"
        "src/main/yquake2/src/client/refresh/gl3/*.c"
        "src/main/yquake2/src/client/refresh/gl3/glad/src/*.c"
        "src/main/yquake2/src/client/refresh/files/*.c")
file(GLOB game_srcs "src/main/yquake2/src/game/*.c"
        "src/main/yquake2/src/game/monster/*/*.c"
        "src/main/yquake2/src/game/player/*.c"
        "src/main/yquake2/src/game/savegame/*.c"
        "src/main/yquake2/src/common/shared/*.c")
file(GLOB xatrix_srcs "src/main/xatrix/src/*.c"
        "src/main/xatrix/src/monster/*/*.c"
        "src/main/xatrix/src/player/*.c"
        "src/main/xatrix/src/savegame/*.c"
        "src/main/xatrix/src/shared/*.c")
file(GLOB rogue_srcs "src/main/rogue/src/*.c"
        "src/main/rogue/src/dm/*.c"
        "src/main/rogue/src/monster/*/*.c"
        "src/main/rogue/src/player/*.c"
        "src/main/rogue/src/savegame/*.c"
        "src/main/rogue/src/shared/*.c")
file(GLOB ctf_srcs "src/main/ctf/src/*.c"
        "src/main/ctf/src/monster/move.c"
        "src/main/ctf/src/menu/*.c"
        "src/main/ctf/src/player/*.c"
        "src/main/ctf/src/savegame/*.c"
        "src/main/ctf/src/shared/*.c")
# Output binary
add_library(yquake2 SHARED ${yquake2_srcs})
add_library(game SHARED ${game_srcs})
add_library(xatrix SHARED ${xatrix_srcs})
add_library(rogue SHARED ${rogue_srcs})
add_library(ctf SHARED ${ctf_srcs})
# Includes
target_include_directories(yquake2 PRIVATE ${libs_dir} "src/main/SDL/include" "src/main/yquake2/src/client/refresh/gl3/glad/include")
# Build
target_link_libraries(game)
target_link_libraries(xatrix)
target_link_libraries(rogue)
target_link_libraries(ctf)
target_link_libraries(yquake2
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main/SDL/build/android/lib/${ANDROID_ABI}/libSDL2.so
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main/SDL/build/android/lib/${ANDROID_ABI}/libhidapi.so
    ${android-lib}
    ${GLESv3-lib}
    ${log-lib}
    ${libs_dir}/jni/${ANDROID_ABI}/libcardboard_api.so)
