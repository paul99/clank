#!/bin/bash

# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

export ANDROID_EABI_TOOLCHAIN=$ANDROID_BUILD_TOP/prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.6/bin

# MIPS on mips-master
export ANDROID_MIPS_TOOLCHAIN=$ANDROID_BUILD_TOP/prebuilts/gcc/linux-x86/mips/mipsel-linux-android-4.6/bin

# plind, make this conditional
export ANDROID_TOOLCHAIN=$ANDROID_MIPS_TOOLCHAIN

# Sets up environment for building Chromium for Android.
if [ -z "$ANDROID_BUILD_TOP" -o -z "$ANDROID_TOOLCHAIN" -o \
  -z "$ANDROID_PRODUCT_OUT" ]; then
  echo "Android build environment variables must be set."
  echo "Please cd to the root of your Android tree and do: "
  echo "  . build/envsetup.sh"
  echo "  lunch"
  echo "Then try this again."
  return 1
fi

host_os=$(uname -s | sed -e 's/Linux/linux/;s/Darwin/mac/')

# We export "TOP" here so that "mmm" can be run to build Java code without
# having to cd to $ANDROID_BUILD_TOP. Ex: 'mmm $PWD/clank/java'.
export TOP="$ANDROID_BUILD_TOP"

# We export "ANDROID_NDK_ROOT" for building Chromium for Android by NDK.
export ANDROID_NDK_ROOT=${ANDROID_BUILD_TOP}/prebuilts/ndk/android-ndk-r8b

# Find the root of the clank sources. The first try assumes that clank is
# embedded inside an Android checkout. The second assumes that the android
# and clank source trees are parallel, just like the buildbots used to do.
# The final one assumes you are in the top level directory of your clank
# tree (so ". build/android/envsetup.sh" works).
unset CLANK_SRC
for x in \
  "${TOP}"/external/chrome "${TOP}"/../src "${TOP}"/../clank/src "$PWD" ; do
  if [ -f "${x}/build/android/envsetup.sh" ] ; then
    export CLANK_SRC=$(cd "$x"; pwd)
    break
  fi
done
if [ -z "$CLANK_SRC" ]; then
  echo "Error: Unable to find clank sources."
  return 1
fi

# Helper function so that when we run "make" to build for clank it exports
# the toolchain variables to make.
make() {
  # Only cross-compile if the build is being done either from Chromium's src/
  # directory, or through WebKit, in which case the WEBKIT_ANDROID_BUILD
  # environment variable will be defined. WebKit uses a different directory.
  if [ -f "$PWD/build/android/envsetup.sh" ] ||
     [ -n "${WEBKIT_ANDROID_BUILD}" ]; then
    CC="${CROSS_CC}" CXX="${CROSS_CXX}" \
    LINK="${CROSS_LINK}" AR="${CROSS_AR}" RANLIB="${CROSS_RANLIB}" \
      command make CC.host="$HOST_CC" \
        CXX.host="$HOST_CXX" LINK.host="$HOST_LINK" $*
  else
    command make $*
  fi
}

# Performs a gyp_chromium run to convert gyp->Makefile for clank code.
clank_gyp() {
  "$CLANK_SRC"/build/gyp_chromium --depth="$CLANK_SRC" \
    -I"$CLANK_SRC"/build/android/clank_internal.gypi --check
}

# Repo syncs only the clank trees and then run gyp_chromium.
clank_sync() {
  local projects=$(repo forall -c "$CLANK_SRC"/build/android/clank_repo_filter.sh)
  if [ $(uname) == "Darwin" ]; then
    local cores=$(hwprefs cpu_count)
  else
    local cores=$(grep processor /proc/cpuinfo | wc -l)
  fi
  echo "Syncing Projects using -j${cores} : ${projects} "
  repo sync -j${cores} ${projects}
  clank_gyp
}

firstword() {
  echo "${1}"
}

echo "The ANDROID_TOOLCHAIN is called $ANDROID_TOOLCHAIN"   # plind debug
export CROSS_AR="$(firstword "${ANDROID_TOOLCHAIN}"/*-ar)"
export CROSS_CC="$(firstword "${ANDROID_TOOLCHAIN}"/*-gcc)"
export CROSS_CXX="$(firstword "${ANDROID_TOOLCHAIN}"/*-g++)"
export CROSS_LINK="$(firstword "${ANDROID_TOOLCHAIN}"/*-gcc)"
export CROSS_RANLIB="$(firstword "${ANDROID_TOOLCHAIN}"/*-ranlib)"
export HOST_CC=gcc
export HOST_CXX=g++
export HOST_LINK=g++
export OBJCOPY="$(firstword "${ANDROID_TOOLCHAIN}"/*-objcopy)"
export STRIP="$(firstword "${ANDROID_TOOLCHAIN}"/*-strip)"

# The set of GYP_DEFINES to pass to gyp. Use 'readlink -e' on directories
# to canonicalize them (remove double '/', remove trailing '/', etc).
DEFINES="OS=android"
if [ -z "$CLANK_BUNDLED_BUILD" ]; then
  DEFINES+=" android_build_type=0"
else
  DEFINES+=" android_build_type=1"
fi
DEFINES+=" host_os=${host_os}"
DEFINES+=" android_product_out=$(echo -n ${ANDROID_PRODUCT_OUT})"
DEFINES+=" linux_fpic=1"
DEFINES+=" release_optimize=s"
DEFINES+=" linux_use_tcmalloc=0"
DEFINES+=" disable_nacl=1"
DEFINES+=" remoting=0"
DEFINES+=" p2p_apis=0"
DEFINES+=" enable_touch_events=1"
DEFINES+=" build_ffmpegsumo=0"
# TODO(bulach): use "shared_libraries" once the transition from executable
# is over.
DEFINES+=" gtest_target_type=executable"
if [ -z "$CLANK_OFFICIAL_BUILD" ]; then
  DEFINES+=" branding=Chromium"
else
  DEFINES+=" branding=Chrome"
  DEFINES+=" buildtype=Official"

  # These defines are used by various chrome build scripts to tag the binary's
  # version string as 'official' in linux builds (e.g. in
  # chrome/trunk/src/chrome/tools/build/version.py).
  export OFFICIAL_BUILD=1
  export CHROMIUM_BUILD="_google_chrome"
  export CHROME_BUILD_TYPE="_official"

  # Used by chrome_version_info_posix.cc to display the channel name in the app.
  # Currently Chrome on Android is always beta. When we have multiple channels,
  # we'll need to update the script to set this appropriately.
  # Valid values: "unstable", "stable", "dev", "beta".
  export CHROME_VERSION_EXTRA="beta"
fi

# The order file specifies the order of symbols in the .text section of the
# shared library, libchromeview.so.  The file is an order list of section names
# and the library is linked with option --section-ordering-file=<orderfile>.
# The order file is updated by profiling startup of clank compiled with the
# order_profiling=1 GYP_DEFINES flag.
ORDER_DEFINES="order_text_section=./orderfiles/orderfile.out"

# The following defines will affect ARM code generation of both C/C++ compiler
# and V8 mksnapshot.
# "passion"'s TARGET_ARCH_VARIANT is "armv7-a-neon". So it needs to match the
# settings in Android's armv7-a-neon.mk.
# "trygon"'s TARGET_ARCH_VARIANT is "armv7-a". So it needs to match the settings
# in Android's armv7-a.mk.
# "full"'s TARGET_ARCH_VARIANT is "armv5te". So it needs to match the settings
# in Android's armv5te.mk.
case "${TARGET_PRODUCT}" in
  "passion"|"soju"|"sojua"|"sojus"|"yakju"|"mysid")
    DEFINES+=" target_arch=arm"
    DEFINES+=" arm_neon=1 armv7=1 arm_thumb=1"
    DEFINES+=" ${ORDER_DEFINES}"
    ;;
  "trygon"|"tervigon")
    DEFINES+=" target_arch=arm"
    DEFINES+=" arm_neon=0 armv7=1 arm_thumb=1 arm_fpu=vfpv3-d16"
    DEFINES+=" ${ORDER_DEFINES}"
    ;;
  "full")
    DEFINES+=" target_arch=arm"
    DEFINES+=" arm_neon=0 armv7=0 arm_thumb=1 arm_fpu=vfp"
    ;;
  "full_mips")
    DEFINES+=" target_arch=mipsel"
    DEFINES+=""
    ;;
  *x86*)
    # TODO(tedbo): The ia32 build fails on ffmpeg, so we disable it here.
    DEFINES+=" target_arch=ia32 use_libffmpeg=0"
    ;;
  *)
    echo "TARGET_PRODUCT: ${TARGET_PRODUCT} is not supported." >& 2
    return 1
esac

if [ -n "$CLANK_VALGRIND_BUILD" ]; then
# arm_thumb=0 is a workaround for https://bugs.kde.org/show_bug.cgi?id=270709
  DEFINES+=" arm_thumb=0 release_extra_cflags='-fno-inline\
 -fno-omit-frame-pointer -fno-builtin' release_valgrind_build=1\
 release_optimize=1"
fi

export GYP_DEFINES="${DEFINES}"

# Use the "android" flavor of the Makefile generator for both Linux and OS X.
export GYP_GENERATORS="make-android"

# Use our All target as the default
export GYP_GENERATOR_FLAGS="${GYP_GENERATOR_FLAGS} default_target=All"

# We want to use our version of "all" targets.
export CHROMIUM_GYP_FILE="${CLANK_SRC}/build/all_android.gyp"

# Maybe extend the path to include our tool scripts.
echo $PATH | grep "${CLANK_SRC}/clank/bin" > /dev/null 2>&1
if [ $? -ne 0 ]; then
  export PATH="$PATH:${CLANK_SRC}/clank/bin"
fi

#
# Print list of devices attached to the machine
#
function adb_get_dev_names() {
  devices_=`adb_get_dev_serials`
  for i in ${devices_}; do
    device_=$(adb -s ${i} shell getprop ro.product.device)
    echo ${device_%?} # trim the last weird character
  done
}

#
# Select a device to use with adb.  If no device is specified, the list of
# attached devices is printed to the screen.
#
# Example: adb_select_dev wingray
#
function adb_select_dev() {
  devices_=`adb_get_dev_serials`
  echo
  if [ "${1}x" == "x" ]; then
    echo 'Available devices:'
    for i in ${devices_}; do
      echo -n "  ${i} "
      adb -s ${i} shell getprop ro.product.device
    done
  else
    for i in ${devices_}; do
      candidate=$(adb -s ${i} shell getprop ro.product.device)
      candidate=${candidate%?} # trim the last weird character
      if [ "$1" == "$candidate" ]; then
        echo "Setting ANDROID_SERIAL to ${i}"
        export ANDROID_SERIAL=${i}
        echo
        return
      fi
    done
    echo "Unable to find device \"${1}\""
  fi
  echo
}

#
# Autocomplete support for adb_select_dev
#
_adb_select_dev_show() {
  local cur opts

  cur="${COMP_WORDS[COMP_CWORD]}"
  opts=$(adb_get_dev_names)
  COMPREPLY=( $(compgen -W "${opts}" ${cur}) )
}
complete -F _adb_select_dev_show adb_select_dev

#
# Greps over all Clank repositories. Accepts the same arguments as git grep.
# Colored output is on by default but can be switched off with --color=never.
#
# Example: clank_grep --color=never -i 'chrome\w*client'
#
clank_grep() {
  # Escape "$@" so it can be passed into repo forall.
  local args=""
  for arg in "$@"; do
    args="$args $(printf %q "$arg")"
  done
  (
    cd "$ANDROID_BUILD_TOP"
    # --color=auto doesn't work here so we use 'always'.
    repo forall -p -c \
      '[ "$REPO_REMOTE" = "clank" ] && git grep --color=always '"${args}"
  )
}

#
# Greps over the projects listed in $REPO_GREP_PROJECTS, or over all projects
# if $REPO_GREP_PROJECTS is empty. Accepts the same arguments as git grep.
# Colored output is on by default but can be switched off with --color=never.
#
# Example: Search for 'chrome_view' in .cc files in two projects.
#   > REPO_GREP_PROJECTS="clank/chromium clank/internal/apps"
#   > repo_grep 'chrome_view' -- '*.cc'
#
repo_grep() {
  # Check if REPO_GREP_PROJECTS is not set.
  if [ -z "${REPO_GREP_PROJECTS+x}" ]; then
    echo 'repo_grep: first set REPO_GREP_PROJECTS to a list of projects'
    echo 'to search.  Or set it to empty to search all projects.'
    echo '  ex: REPO_GREP_PROJECTS="clank/chromium clank/internal/apps"'
    echo '  ex: REPO_GREP_PROJECTS=""  # search all projects'
    echo 'You can see the list of all projects or of Clank projects with:'
    echo "  repo forall -c 'echo \$REPO_PROJECT'"
    echo '  repo forall -c "$CLANK_SRC/build/android/clank_repo_filter.sh"'
    return 1
  fi
  (
    cd "$ANDROID_BUILD_TOP"
    # --color=auto doesn't work here so we use 'always'.
    repo forall -p $REPO_GREP_PROJECTS -c git grep --color=always "$@"
  )
}

# FLOCK needs to be null on system that has no flock
which flock > /dev/null || export FLOCK=
