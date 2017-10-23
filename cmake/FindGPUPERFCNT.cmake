# - Vivante headers and libraries
#set (GPUPERFCNT_INC_SEARCH_PATH "$SDKTARGETSYSROOT/usr/include/gpuperfcnt")
#set (GPUPERFCNT_LIB_SEARCH_PATH "$SDKTARGETSYSROOT/usr/lib")

find_path (GPUPERFCNT_INCLUDE_DIR gpuperfcnt/gpuperfcnt.h
	ONLY_CMAKE_FIND_ROOT_PATH
	DOC "The directory where libgpuperfcnt header resides"
	)

find_library (GPUPERFCNT_LIBRARY_DIR libgpuperfcnt.so
	ONLY_CMAKE_FIND_ROOT_PATH
	DOC "The directory where libgpuperfcnt library resides"
	)

if (GPUPERFCNT_INCLUDE_DIR) 
	message(STATUS "(1) Found libgpuperfcnt headers")
endif()

if (GPUPERFCNT_LIBRARY_DIR)
	message(STATUS "(2) Found libgpuperfcnt library")
endif()

if (GPUPERFCNT_INCLUDE_DIR AND GPUPERFCNT_LIBRARY_DIR)
	message(STATUS "OK: Found libgpuperfcnt library...")
	set (GPUPERFCNT_FOUND 1)
else()
	message(STATUS "NOK: Did not found libgpuperfcnt library...")
endif ()

mark_as_advanced (
	GPUPERFCNT_INCLUDE_DIR
	GPUPERFCNT_LIBRARY_DIR
)

mark_as_advanced (
	GPUPERFCNT_FOUND
)
