
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT(ICIClient)

File(GLOB ICISRC
	"console.cpp"
	"displays.cpp"
	"gl_impl.cpp"
	"ICIClient.cpp"
	"ui/imgui.cpp"
	"ui/imgui_draw.cpp"
	"ui/imgui_demo.cpp"
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
list(APPEND CMAKE_CXX_FLAGS "-std=c++11")
endif()

add_executable(ICIClient ${ICISRC})
target_link_libraries(ICIClient SDL2 GL GLEW)

