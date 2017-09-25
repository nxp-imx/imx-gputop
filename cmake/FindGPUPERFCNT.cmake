# - Vivante headers and libraries
set (GPUPERFCNT_INC_SEARCH_PATH "usr/include/gpuperfcnt")
set (GPUPERFCNT_LIB_SEARCH_PATH "usr/lib")
find_path (GPUPERFCNT_INCLUDE_DIR gpuperfcnt.h
	PATHS ${GPUPERFCNT_INC_SEARCH_PATH}
	DOC "The directory where gpuperfcnt resides"
	)

find_library (GPUPERFCNT_LIBRARY libgpuperfcnt.so
	PATHS ${GPUPERFCNT_LIB_SEARCH_PATH}
	DOC "The directory where libgpuperfcnt resides"
	)

if (GPUPERFCNT_INCLUDE_DIR AND GPUPERFCNT_LIBRARY)
	set (GPUPERFCNT_FOUND 1)
endif ()

mark_as_advanced (
	GPUPERFCNT_INCLUDE_DIR
	GPUPERFCNT_LIBRARY
)

mark_as_advanced (
	GPUPERFCNT_FOUND
)
