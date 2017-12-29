project (gputop C CXX)

cmake_minimum_required(VERSION 3.0)
set (CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include(GNUInstallDirs)

option (ENABLE_DEBUG    "Enable debug." OFF)
option (ENABLE_SHARED	"Build against shared library." OFF)
option (ENABLE_STATIC	"Build agasint static library." OFF)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -Wall -Wextra -Werror -Wstrict-prototypes -Wmissing-prototypes -std=c99 -O2")

execute_process(COMMAND git log COMMAND head -1 COMMAND awk "{print $2}" COMMAND head -c 12 OUTPUT_VARIABLE GIT_SHA_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
message(STATUS "Git version is ${GIT_SHA_VERSION}")

# if neither static nor shared is enabled default to shared
if (NOT ENABLE_SHARED AND NOT ENABLE_STATIC)
	set(ENABLE_SHARED ON)
endif()

if (EXISTS ${GPUPERFCNT_INCLUDE_PATH})
	# try to find the libraries headers
	find_path(GPUPERFCNT_INCLUDE_DIR gpuperfcnt PATHS ${GPUPERFCNT_INCLUDE_PATH} NO_CMAKE_FIND_ROOT_PATH)
	if (GPUPERFCNT_INCLUDE_DIR)
		message(STATUS "Found GPUPERFCNT_INCLUDE_PATH @ ${GPUPERFCNT_INCLUDE_PATH}")
		set(GPUPERFCNT_INC_DIR_FOUND 1)
	else()
		message(FATAL_ERROR "Could not found include headers for libgpuperfcnt")
	endif()

	# try to find the library
	find_path(GPUPERFCNT_LIB_SHARED_DIR libgpuperfcnt.so PATHS ${GPUPERFCNT_LIB_PATH} NO_CMAKE_FIND_ROOT_PATH)
	if (GPUPERFCNT_LIB_SHARED_DIR)
		message(STATUS "Found GPUPERFCNT_LIB_PATH @ ${GPUPERFCNT_LIB_PATH}")
		set(GPUPERFCNT_LIB_FOUND 1)
	else()
		message(FATAL_ERROR "Could not found shared library for libgpuperfcnt")
	endif()
else()
	find_package(GPUPERFCNT REQUIRED)
	if (GPUPERFCNT_FOUND)
		message(STATUS "libgpuperfcnt found")
		include_directories(${GPUPERFCNT_INCLUDE_DIR})
	endif()
endif()

if (GPUPERFCNT_FOUND)
	include_directories(${GPUPERFCNT_INCLUDE_DIR})
else()
	include_directories(${GPUPERFCNT_INCLUDE_PATH})
endif()

if (${CMAKE_SYSTEM_NAME} STREQUAL "QNX")
	add_definitions(-D__QNXTO__)
	include_directories($ENV{QNX_TARGET}/usr/include)
endif()

add_definitions(-D_POSIX_C_SOURCE=200809L)
add_definitions(-DGIT_SHA=${GIT_SHA_VERSION})

if (ENABLE_DEBUG)
	add_definitions(-DUSE_DEBUG=1)
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g3 -ggdb3")
else()
	add_definitions(-D_FORTIFY_SOURCE=2)
endif()

add_executable(gputop gputop/top.c gputop/debugfs.c)

if (ENABLE_STATIC)
	message(STATUS "Build against static...")
	# frist check if we are using the package for detection
	if (GPUPERFCNT_FOUND)
		target_link_libraries(gputop ${GPUPERFCNT_LIBRARY_DIR})
	else()
		# rely on the paths provided to do the linking
		SET( CMAKE_EXE_LINKER_FLAGS "-L ${GPUPERFCNT_LIB_PATH}")
		target_link_libraries(gputop gpuperfcnt)
	endif()
else()
	message(STATUS "Build against shared...")
	if (GPUPERFCNT_FOUND)
		target_link_libraries(gputop ${GPUPERFCNT_LIBRARY_DIR})
	else()
		SET( CMAKE_EXE_LINKER_FLAGS "-L ${GPUPERFCNT_LIB_PATH}")
		target_link_libraries(gputop gpuperfcnt)
	endif()
endif()


add_custom_target(cscope
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
	COMMAND find ../gputop/ -name "*.[csh]" > cscope.files
	COMMAND cscope -b
)

add_custom_target(tags
	DEPENDS cscope
	COMMAND ctags -R --c++-kinds=+p --fields=+iaS --extra=+q . -L cscope.files
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_custom_target(distclean
		COMMAND rm -rf cscope.out tags cscope.files man/ doxygen.conf
)

find_program(FOUND_PANDOC NAMES pandoc PATHS /usr)

if (FOUND_PANDOC)
	message(STATUS "Found pandoc. Creating man target")
	add_custom_target(man
		COMMAND pandoc -s -t man ${CMAKE_SOURCE_DIR}/man/gputop.md > ${CMAKE_SOURCE_DIR}/man/gputop.8
		COMMAND mkdir -p ${CMAKE_BINARY_DIR}/man
		COMMAND cp ${CMAKE_SOURCE_DIR}/man/*.md ${CMAKE_BINARY_DIR}/man
		COMMAND pandoc -s -t man man/gputop.md > man/gputop.8
	)
add_dependencies(gputop man)
endif()


install(TARGETS gputop DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES ${CMAKE_SOURCE_DIR}/man/gputop.8 DESTINATION ${CMAKE_INSTALL_MANDIR}/man8/)
