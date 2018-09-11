# GPUTOP #

*gputop* is an example application using libgpuperfnct library. See
man/gputop.md on how to use it.

# Bulding #

This assumes that the library is already installed in your toolchain, otherwise
proceed to ``Specifing libgpuperfcnt include and library path'' section.

Source your toolchain:

	$ . /path/to/your/toolchain/environment

Build:

	$ cmake -DCMAKE_TOOLCHAIN_FILE=cmake/OEToolchainConfig.cmake ..


Install:

	$ make install

By default this installs into /usr. If you want to specify where to install use
-DCMAKE_INSTALL_PREFIX to specify the directory where to install.


## Specifing libgpuperfcnt include and library path

### Linux/QNX

Source your toolchain

	$ . /path/to/your/toolchain/environment

Build

	$ cmake -DCMAKE_TOOLCHAIN_FILE=cmake/OEToolchainConfig.cmake \
	-DGPUPERFCNT_INCLUDE_PATH=/path/to/libgpuperfcnt/include/ \
	-DGPUPERFCNT_LIB_PATH=/path/to/where/the/library/exists/libgpuperfcnt/ .. 

Install:

	$ make install

Again specify -DCMAKE_INSTALL_PREFIX where to install the package.

### Android

Like in Linux/QNX you need to export the include directory and
library path where you build the library. You'll need
a NDK version in order to build it.

```
cd tools
# to build in tools/ directory
export NDK_PROJECT_PATH=.
# specify which plaform to build for
export TARGET_PLATFORM=armeabi-v7a|arm64-v8a
# the next two are required to find the include and library previously built
export GPUPERFCNT_INCLUDE_PATH=/path/to/libgpuperfcnt/include
export GPUPERFCNT_LIB_PATH=/path/to/libgpuperfcnt/libs/ARCH/
ndk-build NDK_APPLICATION_MK=./Application.mk
cd -
```

tools/obj/local/ARCH/gputop will contain the final executable.
