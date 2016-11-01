
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT(ICIClient)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")

set(ICIC_SRC
	"console.cpp"
	"displays.cpp"
	"gl_impl.cpp"
	"ICIClient.cpp"
	"ui/imgui.cpp"
	"ui/imgui_draw.cpp"
	"ui/imgui_demo.cpp"
)

set(ICIC_INC
	"ici.h"
)

if(WIN32)
set(ICIC_SRC ${ICIC_SRC}
	"ICIClient.rc"
)
set(ICIC_INC ${ICIC_INC}
	"resource.h"
	"targetver.h"
)
endif()

file(GLOB ICIC_SRC_FILES ${ICIC_SRC})
file(GLOB ICIC_INC_FILES ${ICIC_INC})

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
list(APPEND CMAKE_CXX_FLAGS "-std=c++11")
endif()

set(GLEW_STATIC TRUE CACHE BOOL "Use static Glew library")

if(GLEW_STATIC)
add_definitions(-DGLEW_STATIC)
endif()

find_package(SDL2 REQUIRED)
if(NOT APPLE)
find_package(GLEW REQUIRED)
endif()
find_package(OpenGL REQUIRED)

include_directories(${SDL2_INCLUDE_DIR} ${GLEW_INCLUDE_DIR})

if(WIN32)
	get_filename_component(SDL2_LIB_PATH ${SDL2_LIBRARY} DIRECTORY)
	find_file(SDL2_DLL SDL2.dll HINTS ${SDL2_LIB_PATH})
	get_filename_component(SDL2_DLL_PATH ${SDL2_LIBRARY} DIRECTORY)
	if(NOT SDL2_DLL)
	message("SDL2.dll is not found, program may fail to run")
	elseif(${SDL2_LIB_PATH} STREQUAL ${SDL2_DLL_PATH})
	set(COPY_SDL2_DLL TRUE CACHE BOOL "Copy SDL2.dll to build output")
	else()
	set(COPY_SDL2_DLL FALSE CACHE BOOL "Copy SDL2.dll to build output")
	endif()

	add_executable(ICIClient WIN32 ${ICIC_SRC_FILES} ${ICIC_INC_FILES} README.txt LICENSE.txt)
else()
	add_executable(ICIClient ${ICIC_SRC_FILES} ${ICIC_INC_FILES} README.txt LICENSE.txt)
endif()

source_group("Include Files" FILES ${ICIC_INC_FILES})
source_group("Text" FILES README.txt LICENSE.txt)

target_link_libraries(ICIClient ${SDL2_LIBRARY} ${OPENGL_gl_LIBRARY} ${GLEW_LIBRARY})

if(COPY_SDL2_DLL)
	if(NOT SDL2_DLL)
		message(FATAL_ERROR "SDL2.dll is not found, required for copy option.")
	endif()

	add_custom_command(TARGET ICIClient POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SDL2_DLL} $<TARGET_FILE_DIR:ICIClient>
	)
endif()

