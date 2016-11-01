
# This module defines
# SDL2_LIBRARY, the name of the library to link against
# SDL2_FOUND, if false, do not try to link to SDL2
# SDL2_INCLUDE_DIR, where to find SDL.h
#
# This module responds to the the flag:
# SDL2_MAIN
# If this is defined, then SDL2main will be linked in.
#
# Don't forget to include SDLmain.h and SDLmain.m your project for the
# OS X framework based version. (Other versions link to -lSDL2main which
# this module will try to find on your behalf.) Also for OS X, this
# module will automatically add the -framework Cocoa on your behalf.
#
# $SDL2DIR is an environment variable that would
# correspond to the ./configure --prefix=$SDL2DIR
# used in building SDL2.
# l.e.galup  9-20-02
#
# Modified by Meisaka Yukara (2016-11-01)
# Removed the Temp Variable.
# Changed to not add the SDL2main library by default
# since that is a bad default for general C/C++ applications (that define main)
# Added some Windows search options
# Added command to change result variables to advanced when finished successfully
# this version from: https://github.com/Meisaka/ICIClient
#
# Modified by Eric Wing.
# Added code to assist with automated building by using environmental variables
# and providing a more controlled/consistent search behavior.
# Added new modifications to recognize OS X frameworks and
# additional Unix paths (FreeBSD, etc).
# Also corrected the header search path to follow "proper" SDL guidelines.
# Added a search for SDL2main which is needed by some platforms.
# Added a search for threads which is needed by some platforms.
# Added needed compile switches for MinGW.
#
# On OSX, this will prefer the Framework version (if found) over others.
# People will have to manually change the cache values of
# SDL2_LIBRARY to override this selection or set the CMake environment
# CMAKE_INCLUDE_PATH to modify the search paths.
#
# Note that the header path has changed from SDL2/SDL.h to just SDL.h
# This needed to change because "proper" SDL convention
# is #include "SDL.h", not <SDL2/SDL.h>. This is done for portability
# reasons because not all systems place things in SDL2/ (see FreeBSD).

#=============================================================================
# Copyright 2003-2009 Kitware, Inc.
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

if(WIN32)
# search relative to the current project, never know where things could be on Windows
file(GLOB SDL2_RELATIVE_PATHS "${PROJECT_SOURCE_DIR}/../SDL2*" "${PROJECT_SOURCE_DIR}/SDL2*")
endif()

SET(SDL2_SEARCH_PATHS
	~/Library/Frameworks
	/Library/Frameworks
	/usr/local
	/usr
	/sw # Fink
	/opt/local # DarwinPorts
	/opt/csw # Blastwave
	/opt
	${SDL2_PATH}
)

IF(MSVC_VERSION)
	# see if we have this stuff in the MSVC system path (probably not)
	file(GLOB MSVC_PATHS "$ENV{PROGRAMFILES}/Microsoft Visual Studio */VC" "$ENV{PROGRAMW6432}/Microsoft Visual Studio */VC")
	SET(SDL2_SEARCH_PATHS ${SDL2_SEARCH_PATHS} ${MSVC_PATHS})
ENDIF()

FIND_PATH(SDL2_INCLUDE_DIR SDL.h
	HINTS
	$ENV{SDL2DIR} ${SDL2_RELATIVE_PATHS}
	PATH_SUFFIXES include/SDL2 include
	PATHS ${SDL2_SEARCH_PATHS}
)

FIND_LIBRARY(SDL2_LIBRARY
	NAMES SDL2
	HINTS
	$ENV{SDL2DIR} ${SDL2_RELATIVE_PATHS}
	PATH_SUFFIXES lib64 lib lib/x64 lib/x86 lib/amd64
	PATHS ${SDL2_SEARCH_PATHS}
)

IF(SDL2_MAIN)
	IF(NOT ${SDL2_INCLUDE_DIR} MATCHES ".framework")
		# Non-OS X framework versions expect you to also dynamically link to
		# SDL2main. This is mainly for Windows and OS X. Other (Unix) platforms
		# seem to provide SDL2main for compatibility even though they don't
		# necessarily need it.
		FIND_LIBRARY(SDL2_MAIN_LIBRARY
			NAMES SDL2main
			HINTS
			$ENV{SDL2DIR} ${SDL2_RELATIVE_PATHS}
			PATH_SUFFIXES lib64 lib lib/x64 lib/x86 lib/amd64
			PATHS ${SDL2_SEARCH_PATHS}
		)
	ENDIF(NOT ${SDL2_INCLUDE_DIR} MATCHES ".framework")
ENDIF()

# SDL2 may require threads on your system.
# The Apple build may not need an explicit flag because one of the
# frameworks may already provide it.
# But for non-OSX systems, I will use the CMake Threads package.
IF(NOT APPLE)
	FIND_PACKAGE(Threads)
ENDIF(NOT APPLE)

# MinGW needs an additional link flag, -mwindows
# It's total link flags should look like -lmingw32 -lSDL2main -lSDL2 -mwindows
IF(MINGW)
	SET(MINGW32_LIBRARY mingw32 "-mwindows" CACHE STRING "mwindows for MinGW")
ENDIF(MINGW)

IF(SDL2_LIBRARY)
	# For SDL2main
	IF(SDL2_MAIN)
		IF(SDL2_MAIN_LIBRARY)
			SET(SDL2_LIBRARY ${SDL2_MAIN_LIBRARY} ${SDL2_LIBRARY})
		ENDIF(SDL2_MAIN_LIBRARY)
	ENDIF()

	# For OS X, SDL2 uses Cocoa as a backend so it must link to Cocoa.
	# CMake doesn't display the -framework Cocoa string in the UI even
	# though it actually is there if I modify a pre-used variable.
	# I think it has something to do with the CACHE STRING.
	# So I use a temporary variable until the end so I can set the
	# "real" variable in one-shot.
	IF(APPLE)
		SET(SDL2_LIBRARY ${SDL2_LIBRARY} "-framework Cocoa")
	ENDIF(APPLE)

	# For threads, as mentioned Apple doesn't need this.
	# In fact, there seems to be a problem if I used the Threads package
	# and try using this line, so I'm just skipping it entirely for OS X.
	IF(NOT APPLE)
		SET(SDL2_LIBRARY ${SDL2_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})
	ENDIF(NOT APPLE)

	# For MinGW library
	IF(MINGW)
		SET(SDL2_LIBRARY ${MINGW32_LIBRARY} ${SDL2_LIBRARY})
	ENDIF(MINGW)

	# Set the final string here so the GUI reflects the final state.
	SET(SDL2_LIBRARY ${SDL2_LIBRARY} CACHE STRING "Where the SDL2 Library can be found")
ENDIF(SDL2_LIBRARY)

INCLUDE(FindPackageHandleStandardArgs)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(SDL2 REQUIRED_VARS SDL2_LIBRARY SDL2_INCLUDE_DIR)

mark_as_advanced(SDL2_LIBRARY SDL2_INCLUDE_DIR)
