# Android 2.3: API 9 on the native side (also fine for 2.3.3), built with NDK
# r15c and NDK_MIN_PLATFORM_LEVEL=9, see tools/build.sh.
APP_PLATFORM := android-9
# ABIS=armeabi-v7a+x86 on the command line adds x86 (PC emulator)
APP_ABI := $(if $(ABIS),$(subst +, ,$(ABIS)),armeabi-v7a)
APP_STL := c++_static
APP_CPPFLAGS := -fexceptions -frtti
APP_OPTIM := release
