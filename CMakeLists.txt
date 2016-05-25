
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

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

add_executable(ICIClient ${ICISRC})
target_link_libraries(ICIClient SDL2 GL GLEW)

