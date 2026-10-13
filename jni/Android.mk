LOCAL_PATH := $(call my-dir)

# The Cardputer game next to this repository, and the M5GFX library that
# PlatformIO downloaded for its simulator (pio run -e simulateur).
GAME_DIR ?= $(abspath $(LOCAL_PATH)/../..)/Escape-Game-Cardputer
GFX_DIR ?= $(GAME_DIR)/.pio/libdeps/simulateur/M5GFX/src
# Copies of the game sources made by tools/build.sh: Android texts, with the
# buttons of each device, and bridge.inc appended to main.cpp. One library
# per device; Java loads the right one.
SRC_XPERIA ?= $(GAME_DIR)/src
SRC_OUYA ?= $(GAME_DIR)/src
SRC_TOUCH ?= $(GAME_DIR)/src

ANDROID_SOURCES := game.cpp audio.cpp input.cpp onscreen_keyboard.cpp role_menu.cpp display_m5gfx.cpp mirror_android.cpp
GFX_SOURCES := \
  $(wildcard $(GFX_DIR)/lgfx/v1/*.cpp) \
  $(GFX_DIR)/lgfx/v1/panel/Panel_M5HDMI.cpp \
  $(wildcard $(GFX_DIR)/lgfx/utility/*.c) \
  $(wildcard $(GFX_DIR)/lgfx/Fonts/efont/*.c) \
  $(wildcard $(GFX_DIR)/lgfx/Fonts/IPA/*.c)
# M5GFX "Linux" platform (Android is one). Only the functions Java calls are
# exported, which lets the linker drop the fonts the game does not use
# (45 MB otherwise).
COMMON_CFLAGS := -DLGFX_LINUX_FB -DM5GFX_BOARD=board_M5Cardputer \
  -fvisibility=hidden -ffunction-sections -fdata-sections -Wno-everything
# C++14: the libc++ of NDK r15c lacks std::invoke_result (M5GFX has a C++14
# fallback); M5GFX forgets <cstdlib> and <cmath> in a few places.
COMMON_CPPFLAGS := -std=gnu++14 -include cstdlib -include cmath
COMMON_LDFLAGS := -Wl,--no-undefined -Wl,--gc-sections -Wl,--exclude-libs,ALL

# ---- Xperia Play: PlayStation buttons in the texts
include $(CLEAR_VARS)
LOCAL_MODULE := explorer3_xperia
LOCAL_SRC_FILES := $(ANDROID_SOURCES) $(SRC_XPERIA)/main.cpp $(GFX_SOURCES)
# include/ first: it replaces the simulator's Preferences.h
LOCAL_C_INCLUDES := $(LOCAL_PATH)/include $(LOCAL_PATH) $(GAME_DIR)/sim/include $(SRC_XPERIA) $(GFX_DIR)
LOCAL_CFLAGS := $(COMMON_CFLAGS)
LOCAL_CPPFLAGS := $(COMMON_CPPFLAGS)
LOCAL_LDLIBS := -llog
LOCAL_LDFLAGS := $(COMMON_LDFLAGS)
include $(BUILD_SHARED_LIBRARY)

# ---- OUYA: O U Y A buttons in the texts
include $(CLEAR_VARS)
LOCAL_MODULE := explorer3_ouya
LOCAL_SRC_FILES := $(ANDROID_SOURCES) $(SRC_OUYA)/main.cpp $(GFX_SOURCES)
LOCAL_C_INCLUDES := $(LOCAL_PATH)/include $(LOCAL_PATH) $(GAME_DIR)/sim/include $(SRC_OUYA) $(GFX_DIR)
LOCAL_CFLAGS := $(COMMON_CFLAGS) -DEXPLORER3_OUYA
LOCAL_CPPFLAGS := $(COMMON_CPPFLAGS)
LOCAL_LDLIBS := -llog
LOCAL_LDFLAGS := $(COMMON_LDFLAGS)
include $(BUILD_SHARED_LIBRARY)

# ---- No gamepad at start: texts for the finger (user's choice)
include $(CLEAR_VARS)
LOCAL_MODULE := explorer3_touch
LOCAL_SRC_FILES := $(ANDROID_SOURCES) $(SRC_TOUCH)/main.cpp $(GFX_SOURCES)
LOCAL_C_INCLUDES := $(LOCAL_PATH)/include $(LOCAL_PATH) $(GAME_DIR)/sim/include $(SRC_TOUCH) $(GFX_DIR)
LOCAL_CFLAGS := $(COMMON_CFLAGS) -DEXPLORER3_TOUCH
LOCAL_CPPFLAGS := $(COMMON_CPPFLAGS)
LOCAL_LDLIBS := -llog
LOCAL_LDFLAGS := $(COMMON_LDFLAGS)
include $(BUILD_SHARED_LIBRARY)

# ---- Mission control: the protocol, pages, banner and sound of the web
# page (control/, the same core as Explorer3-RaspberryPi's src/control/), the
# Android part (control.c, gl_draw.c, net.c, ui.c), and what tools/build.sh
# takes from the game (CONTROL_GEN: page_texts.inc, game_tables.inc).
CONTROL_GEN ?= $(LOCAL_PATH)/control
CONTROL_SOURCES := addr banner clocksync control draw font_8x16 gamedata gl_draw msg net panel rows session sound text texts ui ws
include $(CLEAR_VARS)
LOCAL_MODULE := explorer3_control
LOCAL_SRC_FILES := $(foreach f,$(CONTROL_SOURCES),control/$(f).c)
LOCAL_C_INCLUDES := $(LOCAL_PATH)/control $(CONTROL_GEN)
LOCAL_CFLAGS := -std=gnu11 -O2 -fvisibility=hidden -ffunction-sections -fdata-sections -Wall
LOCAL_LDLIBS := -lGLESv2 -llog -lm
LOCAL_LDFLAGS := $(COMMON_LDFLAGS)
include $(BUILD_SHARED_LIBRARY)
