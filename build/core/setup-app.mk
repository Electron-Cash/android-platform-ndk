# Copyright (C) 2009 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# this file is included repeatedly from build/core/main.mk
# and is used to prepare for app-specific build rules.
#

$(call assert-defined,_app)

_map := NDK_APP.$(_app)

# ok, let's parse all Android.mk source files in order to build
# the modules for this app.
#

# Restore the APP_XXX variables just for this pass as NDK_APP_XXX
#
NDK_APP_NAME           := $(_app)
NDK_APP_APPLICATION_MK := $(call get,$(_map),Application.mk)

$(foreach __name,$(NDK_APP_VARS),\
  $(eval NDK_$(__name) := $(call get,$(_map),$(__name)))\
)

# make the application depend on the modules it requires
.PHONY: ndk-app-$(_app)
ndk-app-$(_app): $(NDK_APP_MODULES)
all: ndk-app-$(_app)

# which platform/abi/toolchain are we going to use?
TARGET_PLATFORM := $(call get,$(_map),APP_PLATFORM)

# The ABI(s) to use
NDK_APP_ABI := $(strip $(NDK_APP_ABI))
ifndef NDK_APP_ABI
    # the default ABI for now is armeabi
    NDK_APP_ABI := armeabi
endif

# If APP_ABI is 'all', then set it to all supported ABIs
# Otherwise, check that we don't have an invalid value here.
#
ifeq ($(NDK_APP_ABI),all)
    NDK_APP_ABI := $(NDK_ALL_ABIS)
else
    # check the target ABIs for this application
    _bad_abis = $(strip $(filter-out $(NDK_ALL_ABIS),$(NDK_APP_ABI)))
    ifneq ($(_bad_abis),)
        $(call __ndk_info,NDK Application '$(_app)' targets unknown ABI(s): $(_bad_abis))
        $(call __ndk_info,Please fix the APP_ABI definition in $(NDK_APP_APPLICATION_MK))
        $(call __ndk_error,Aborting)
    endif
endif

ifndef APP_USE_CPP0X
    NDK_USE_CPP0X := $(DEFAULT_USE_CPP0X)
else
    NDK_USE_CPP0X := $(strip $(APP_USE_CPP0X))
    ifneq ($(NDK_USE_CPP0X),true)
        ifneq ($(NDK_USE_CPP0X),false)
            ifneq ($(NDK_USE_CPP0X),strict)
                $(call __ndk_info,Wrong value of APP_USE_CPP0X: $(APP_USE_CPP0X))
                $(call __ndk_info,The only allowed values are 'true' 'strict' or 'false')
                $(call __ndk_info,  'true'   - use C++ 0x with GNU extensions)
                $(call __ndk_info,  'strict' - use C++ 0x without GNU extensions)
                $(call __ndk_info,  'false'  - don\'t use C++ 0x)
                $(call __ndk_error,Aborting)
            endif
        endif
    endif
endif
TARGET_USE_CPP0X := $(NDK_USE_CPP0X)

ifndef APP_TOOLCHAIN_VERSION
    NDK_TOOLCHAIN_VERSION := $(DEFAULT_TOOLCHAIN_VERSION)
else
    NDK_TOOLCHAIN_VERSION = $(strip $(APP_TOOLCHAIN_VERSION))
endif

# Clear all installed binaries for this application
# This ensures that if the build fails, you're not going to mistakenly
# package an obsolete version of it. Or if you change the ABIs you're targetting,
# you're not going to leave a stale shared library for the old one.
#
ifeq ($(NDK_APP.$(_app).cleaned_binaries),)
    NDK_APP.$(_app).cleaned_binaries := true
    clean-installed-binaries::
	$(hide) $(call host-rm,$(NDK_ALL_ABIS:%=$(NDK_APP_PROJECT_PATH)/libs/%/lib*.so))
	$(hide) $(call host-rm,$(NDK_ALL_ABIS:%=$(NDK_APP_PROJECT_PATH)/libs/%/gdbserver))
	$(hide) $(call host-rm,$(NDK_ALL_ABIS:%=$(NDK_APP_PROJECT_PATH)/libs/%/gdb.setup))
endif

$(foreach _abi,$(NDK_APP_ABI),\
    $(eval TARGET_ARCH_ABI := $(_abi))\
    $(eval include $(BUILD_SYSTEM)/setup-abi.mk) \
)
