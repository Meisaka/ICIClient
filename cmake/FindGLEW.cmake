# - Find the OpenGL Extension Wrangler Library (GLEW)
# This module defines the following variables:
#  GLEW_INCLUDE_DIRS - include directories for GLEW
#  GLEW_LIBRARIES - libraries to link against GLEW
#  GLEW_FOUND - true if GLEW has been found and can be used

#=============================================================================
# Copyright 2012 Benjamin Eikel
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)

file(GLOB GLEW_RELATIVE_PATHS "${PROJECT_SOURCE_DIR}/../glew*" "${PROJECT_SOURCE_DIR}/glew*")

if(WIN32)
	if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "64")
		set(GLEW_TYPE_SUFFIX x64 amd64)
	else()
		set(GLEW_TYPE_SUFFIX x86 win32)
	endif()
endif()

IF(CMAKE_BUILD_TYPE STREQUAL "Debug")
set(GLEW_LIB_SUFFIX "Debug")
ELSE()
set(GLEW_LIB_SUFFIX "Release")
ENDIF()

find_path(GLEW_INCLUDE_DIR
	GL/glew.h
	HINTS ${GLEW_RELATIVE_PATHS}
	PATH_SUFFIXES include
)
if(GLEW_STATIC)
	set(GLEW_NAMES glew32s glews)
else()
	set(GLEW_NAMES glew32 glew)
endif()

find_library(GLEW_LIBRARY
	NAMES GLEW ${GLEW_NAMES}
	HINTS ${GLEW_RELATIVE_PATHS}
	PATH_SUFFIXES lib lib64 lib/${GLEW_LIB_SUFFIX}/${GLEW_TYPE_SUFFIX}
)

set(GLEW_INCLUDE_DIRS ${GLEW_INCLUDE_DIR})
set(GLEW_LIBRARIES ${GLEW_LIBRARY})

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(GLEW
	REQUIRED_VARS GLEW_INCLUDE_DIR GLEW_LIBRARY)

mark_as_advanced(GLEW_INCLUDE_DIR GLEW_LIBRARY)
