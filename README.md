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

Source your toolchain

	$ . /path/to/your/toolchain/environment

Build

	$ cmake -DCMAKE_TOOLCHAIN_FILE=cmake/OEToolchainConfig.cmake \
	-DGPUPERFCNT_INCLUDE_PATH=/path/to/libgpuperfcnt/include/ \
	-DGPUPERFCNT_LIB_PATH=/path/to/where/the/library/exists/libgpuperfcnt/ .. 

Install:

	$ make install

Again specify -DCMAKE_INSTALL_PREFIX where to install the package.
