# GPUTOP #


# Bulding #


## Specifing libgpuperfcnt include and library path

	$ cmake -DCMAKE_TOOLCHAIN_FILE=cmake/OEToolchainConfig.cmake \
	-DGPUPERFCNT_INCLUDE_PATH=/path/to/libgpuperfcnt/include/ \
	-DGPUPERFCNT_LIB_PATH=/path/to/where/the/library/exists/libgpuperfcnt/ .. 
