// ImGui SDL2 binding with OpenGL3 (slightly modified)
// In this binding, ImTextureID is used to store an OpenGL 'GLuint' texture identifier. Read the FAQ about ImTextureID in imgui.cpp.

#include "ici.h"
// SDL WM
#include <SDL2/SDL_syswm.h>

// Data
static double  g_Time = 0.0f;
static bool    g_MousePressed[5] = { false, false, false, false, false };
static float   g_MouseWheel = 0.0f;
static icitexture  g_FontTexture;
static int     g_ShaderHandle = 0, g_VertHandle = 0, g_FragHandle = 0;
static struct AttribLocs {
	int Tex;
	int Offset;
	int Mode;
	int Position;
	int UV;
	int Color;
} g_Attribs = {0,};
static unsigned int g_VboHandle = 0, g_VaoHandle = 0, g_ElementsHandle = 0, g_SamplerHandle = 0;

// This is the main rendering function that you have to implement and provide to ImGui (via setting up 'RenderDrawListsFn' in the ImGuiIO structure)
// If text or lines are blurry when integrating ImGui in your engine:
// - in your Render function, try translating your projection matrix by (0.5f,0.5f) or (0.375f,0.375f)
void ICIC_RenderDrawLists(ImDrawData* draw_data)
{
	// Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
	ImGuiIO& io = ImGui::GetIO();
	int fb_width = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
	int fb_height = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
	if (fb_width == 0 || fb_height == 0)
		return;
	draw_data->ScaleClipRects(io.DisplayFramebufferScale);

	// Backup GL state
	GLint last_program; glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
	GLint last_texture; glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
	GLint last_active_texture; glGetIntegerv(GL_ACTIVE_TEXTURE, &last_active_texture);
	GLint last_array_buffer; glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
	GLint last_element_array_buffer; glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &last_element_array_buffer);
	GLint last_vertex_array; glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);
	GLint last_blend_src; glGetIntegerv(GL_BLEND_SRC, &last_blend_src);
	GLint last_blend_dst; glGetIntegerv(GL_BLEND_DST, &last_blend_dst);
	GLint last_blend_equation_rgb; glGetIntegerv(GL_BLEND_EQUATION_RGB, &last_blend_equation_rgb);
	GLint last_blend_equation_alpha; glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &last_blend_equation_alpha);
	GLint last_viewport[4]; glGetIntegerv(GL_VIEWPORT, last_viewport);
	GLboolean last_enable_blend = glIsEnabled(GL_BLEND);
	GLboolean last_enable_cull_face = glIsEnabled(GL_CULL_FACE);
	GLboolean last_enable_depth_test = glIsEnabled(GL_DEPTH_TEST);
	GLboolean last_enable_scissor_test = glIsEnabled(GL_SCISSOR_TEST);

	// Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_SCISSOR_TEST);
	glActiveTexture(GL_TEXTURE0);

	// Setup orthographic projection matrix
	glViewport(0, 0, (GLsizei)fb_width, (GLsizei)fb_height);
	const float screen_offset[2] = {
		2.0f / io.DisplaySize.x, 2.0f / -io.DisplaySize.y
	};
	glUseProgram(g_ShaderHandle);
	glUniform1i(g_Attribs.Tex, 0);
	glUniform1i(g_Attribs.Mode, 0);
	glUniform2fv(g_Attribs.Offset, 1, screen_offset);
	glBindVertexArray(g_VaoHandle);

	for (int n = 0; n < draw_data->CmdListsCount; n++) {
		const ImDrawList* cmd_list = draw_data->CmdLists[n];
		const ImDrawIdx* idx_buffer_offset = 0;

		glBindBuffer(GL_ARRAY_BUFFER, g_VboHandle);
		glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)cmd_list->VtxBuffer.size() * sizeof(ImDrawVert), (GLvoid*)&cmd_list->VtxBuffer.front(), GL_STREAM_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_ElementsHandle);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)cmd_list->IdxBuffer.size() * sizeof(ImDrawIdx), (GLvoid*)&cmd_list->IdxBuffer.front(), GL_STREAM_DRAW);

		for (const ImDrawCmd* pcmd = cmd_list->CmdBuffer.begin(); pcmd != cmd_list->CmdBuffer.end(); pcmd++)
		{
			if (pcmd->UserCallback) {
				pcmd->UserCallback(cmd_list, pcmd);
			} else {
				icitexture *ici = (icitexture*)pcmd->TextureId;
				if(ici) {
					glBindTexture(GL_TEXTURE_2D, ici->handle);
					glUniform1i(g_Attribs.Mode, ici->type);
				} else {
					glBindTexture(GL_TEXTURE_2D, 0);
					glUniform1i(g_Attribs.Mode, 0);
				}
				glScissor((int)pcmd->ClipRect.x, (int)(fb_height - pcmd->ClipRect.w), (int)(pcmd->ClipRect.z - pcmd->ClipRect.x), (int)(pcmd->ClipRect.w - pcmd->ClipRect.y));
				glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer_offset);
			}
			idx_buffer_offset += pcmd->ElemCount;
		}
	}

	// Restore modified GL state
	glUseProgram(last_program);
	glActiveTexture(last_active_texture);
	glBindTexture(GL_TEXTURE_2D, last_texture);
	glBindVertexArray(last_vertex_array);
	glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, last_element_array_buffer);
	glBlendEquationSeparate(last_blend_equation_rgb, last_blend_equation_alpha);
	glBlendFunc(last_blend_src, last_blend_dst);
	if (last_enable_blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
	if (last_enable_cull_face) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
	if (last_enable_depth_test) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
	if (last_enable_scissor_test) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
	glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
}

static const char* ICIC_GetClipboardText()
{
	return SDL_GetClipboardText();
}

static void ICIC_SetClipboardText(const char* text)
{
	SDL_SetClipboardText(text);
}

bool ICIC_ProcessEvent(SDL_Event* event)
{
	ImGuiIO& io = ImGui::GetIO();
	int key;
	switch (event->type) {
	case SDL_MOUSEWHEEL:
		if (event->wheel.y > 0)
			g_MouseWheel = 1;
		if (event->wheel.y < 0)
			g_MouseWheel = -1;
		return true;
	case SDL_MOUSEBUTTONDOWN:
		if (event->button.button == SDL_BUTTON_LEFT) g_MousePressed[0] = true;
		if (event->button.button == SDL_BUTTON_RIGHT) g_MousePressed[1] = true;
		if (event->button.button == SDL_BUTTON_MIDDLE) g_MousePressed[2] = true;
		if (event->button.button == SDL_BUTTON_X1) g_MousePressed[3] = true;
		if (event->button.button == SDL_BUTTON_X2) g_MousePressed[4] = true;
		return true;
	case SDL_TEXTINPUT:
		io.AddInputCharactersUTF8(event->text.text);
		return true;
	case SDL_KEYDOWN:
	case SDL_KEYUP:
		key = event->key.keysym.sym & ~SDLK_SCANCODE_MASK;
		if(key == SDL_SCANCODE_KP_ENTER)
			key = SDLK_RETURN;
		io.KeysDown[key] = (event->type == SDL_KEYDOWN);
		io.KeyShift = ((SDL_GetModState() & KMOD_SHIFT) != 0);
		io.KeyCtrl = ((SDL_GetModState() & KMOD_CTRL) != 0);
		io.KeyAlt = ((SDL_GetModState() & KMOD_ALT) != 0);
		io.KeySuper = ((SDL_GetModState() & KMOD_GUI) != 0);
		return true;
	}
	return false;
}

void ICIC_UpdateHWTexture(icitexture *tex, int width, int height, uint32_t *pixels)
{
	// Upload texture to graphics system
	GLint last_texture;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
	glBindTexture(GL_TEXTURE_2D, tex->handle);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	// Restore state
	glBindTexture(GL_TEXTURE_2D, last_texture);
}

void ICIC_CreateHWTexture(icitexture *tex, int width, int height)
{
	// Upload texture to graphics system
	GLint last_texture;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
	glGenTextures(1, &tex->handle);
	glBindTexture(GL_TEXTURE_2D, tex->handle);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	tex->type = 1;
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

	// Restore state
	glBindTexture(GL_TEXTURE_2D, last_texture);
}

void ICIC_CreateFontsTexture()
{
	// Build texture atlas
	ImGuiIO& io = ImGui::GetIO();
	unsigned char* pixels;
	int width, height;
	LogMessage("Generating Atlas...\n");
	io.Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);

	// Upload texture to graphics system
	GLint last_texture;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
	glGenTextures(1, &g_FontTexture.handle);
	glBindTexture(GL_TEXTURE_2D, g_FontTexture.handle);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, pixels);

	// Store our identifier
	io.Fonts->TexID = (void *)&g_FontTexture;

	// Restore state
	glBindTexture(GL_TEXTURE_2D, last_texture);
}

bool ICIC_CreateDeviceObjects()
{
	LogMessage("Building GL Objects\n");

	// Backup GL state
	GLint last_texture, last_array_buffer, last_vertex_array;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
	glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
	glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);

	const GLchar *vertex_shader =
		"#version 330\n"
		"uniform vec2 Offset;\n"
		"in vec2 Position;\n"
		"in vec2 UV;\n"
		"in vec4 Color;\n"
		"out vec2 Frag_UV;\n"
		"out vec4 Frag_Color;\n"
		"void main()\n"
		"{\n"
		"	Frag_UV = UV;\n"
		"	Frag_Color = Color;\n"
		"	gl_Position = vec4((Offset * Position.xy) + vec2(-1.0, 1.0),0,1);\n"
		"}\n";

	const GLchar* fragment_shader =
		"#version 330\n"
		"uniform sampler2D Texture;\n"
		"uniform int Mode;\n"
		"in vec2 Frag_UV;\n"
		"in vec4 Frag_Color;\n"
		"out vec4 Out_Color;\n"
		"void main()\n"
		"{\n"
		"	if(Mode == 1) {\n"
		"		Out_Color = Frag_Color * texture(Texture, Frag_UV.st);\n"
		"	} else {\n"
		"		Out_Color = Frag_Color * vec4(1,1,1,texture(Texture, Frag_UV.st).r);\n"
		"	}\n"
		"}\n";

	g_ShaderHandle = glCreateProgram();
	g_VertHandle = glCreateShader(GL_VERTEX_SHADER);
	g_FragHandle = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(g_VertHandle, 1, &vertex_shader, 0);
	glShaderSource(g_FragHandle, 1, &fragment_shader, 0);
	glCompileShader(g_VertHandle);
	glCompileShader(g_FragHandle);
	glAttachShader(g_ShaderHandle, g_VertHandle);
	glAttachShader(g_ShaderHandle, g_FragHandle);
	glLinkProgram(g_ShaderHandle);

	g_Attribs.Tex = glGetUniformLocation(g_ShaderHandle, "Texture");
	g_Attribs.Offset = glGetUniformLocation(g_ShaderHandle, "Offset");
	g_Attribs.Mode = glGetUniformLocation(g_ShaderHandle, "Mode");
	g_Attribs.Position = glGetAttribLocation(g_ShaderHandle, "Position");
	g_Attribs.UV = glGetAttribLocation(g_ShaderHandle, "UV");
	g_Attribs.Color = glGetAttribLocation(g_ShaderHandle, "Color");

	glGenBuffers(1, &g_VboHandle);
	glGenBuffers(1, &g_ElementsHandle);

	glGenVertexArrays(1, &g_VaoHandle);
	glBindVertexArray(g_VaoHandle);
	glBindBuffer(GL_ARRAY_BUFFER, g_VboHandle);
	glEnableVertexAttribArray(g_Attribs.Position);
	glEnableVertexAttribArray(g_Attribs.UV);
	glEnableVertexAttribArray(g_Attribs.Color);

	glGenSamplers(1, &g_SamplerHandle);
	glSamplerParameteri(g_SamplerHandle, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glSamplerParameteri(g_SamplerHandle, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glBindSampler(0, g_SamplerHandle);

#define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
	glVertexAttribPointer(g_Attribs.Position, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, pos));
	glVertexAttribPointer(g_Attribs.UV, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, uv));
	glVertexAttribPointer(g_Attribs.Color, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, col));
#undef OFFSETOF

	ICIC_CreateFontsTexture();

	// Restore modified GL state
	glBindTexture(GL_TEXTURE_2D, last_texture);
	glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
	glBindVertexArray(last_vertex_array);
	LogMessage("Complete\n");

	return true;
}

void ICIC_InvalidateDeviceObjects()
{
	if (g_VaoHandle) glDeleteVertexArrays(1, &g_VaoHandle);
	if (g_VboHandle) glDeleteBuffers(1, &g_VboHandle);
	if (g_ElementsHandle) glDeleteBuffers(1, &g_ElementsHandle);
	g_VaoHandle = g_VboHandle = g_ElementsHandle = 0;

	glDetachShader(g_ShaderHandle, g_VertHandle);
	glDeleteShader(g_VertHandle);
	g_VertHandle = 0;

	glDetachShader(g_ShaderHandle, g_FragHandle);
	glDeleteShader(g_FragHandle);
	g_FragHandle = 0;

	glDeleteProgram(g_ShaderHandle);
	g_ShaderHandle = 0;

	if(g_FontTexture.handle) {
		glDeleteTextures(1, &g_FontTexture.handle);
		ImGui::GetIO().Fonts->TexID = 0;
		g_FontTexture.handle = 0;
	}
}

bool ICIC_Init(SDL_Window* window)
{
	ImGuiIO& io = ImGui::GetIO();
	io.KeyMap[ImGuiKey_Tab] = SDLK_TAB;
	io.KeyMap[ImGuiKey_LeftArrow] = SDL_SCANCODE_LEFT;
	io.KeyMap[ImGuiKey_RightArrow] = SDL_SCANCODE_RIGHT;
	io.KeyMap[ImGuiKey_UpArrow] = SDL_SCANCODE_UP;
	io.KeyMap[ImGuiKey_DownArrow] = SDL_SCANCODE_DOWN;
	io.KeyMap[ImGuiKey_PageUp] = SDL_SCANCODE_PAGEUP;
	io.KeyMap[ImGuiKey_PageDown] = SDL_SCANCODE_PAGEDOWN;
	io.KeyMap[ImGuiKey_Home] = SDL_SCANCODE_HOME;
	io.KeyMap[ImGuiKey_End] = SDL_SCANCODE_END;
	io.KeyMap[ImGuiKey_Delete] = SDLK_DELETE;
	io.KeyMap[ImGuiKey_Backspace] = SDLK_BACKSPACE;
	io.KeyMap[ImGuiKey_Enter] = SDLK_RETURN;
	io.KeyMap[ImGuiKey_Escape] = SDLK_ESCAPE;
	io.KeyMap[ImGuiKey_A] = SDLK_a;
	io.KeyMap[ImGuiKey_C] = SDLK_c;
	io.KeyMap[ImGuiKey_V] = SDLK_v;
	io.KeyMap[ImGuiKey_X] = SDLK_x;
	io.KeyMap[ImGuiKey_Y] = SDLK_y;
	io.KeyMap[ImGuiKey_Z] = SDLK_z;

	io.RenderDrawListsFn = ICIC_RenderDrawLists;
	io.SetClipboardTextFn = ICIC_SetClipboardText;
	io.GetClipboardTextFn = ICIC_GetClipboardText;

	ImGuiStyle& style = ImGui::GetStyle();
	style.AntiAliasedLines    = false;
	style.AntiAliasedShapes   = false;
	style.WindowPadding       = ImVec2(6.f, 6.f);
	style.WindowRounding      = 0;
	style.ChildWindowRounding = 0;
	style.FramePadding        = ImVec2(3.f, 3.f);
	style.FrameRounding       = 0;
	style.ItemSpacing         = ImVec2(6.f, 2.f);
	style.ItemInnerSpacing    = ImVec2(2.f, 2.f);
	style.IndentSpacing       = 20;
	style.ScrollbarSize       = 14;
	style.ScrollbarRounding   = 0;
	style.GrabMinSize         = 10;
	style.GrabRounding        = 0;
	style.Colors[ImGuiCol_Text]                  = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
	style.Colors[ImGuiCol_TextDisabled]          = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
	style.Colors[ImGuiCol_WindowBg]              = ImVec4(0.00f, 0.02f, 0.13f, 0.90f);
	style.Colors[ImGuiCol_ChildWindowBg]         = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	style.Colors[ImGuiCol_PopupBg]               = ImVec4(0.00f, 0.07f, 0.25f, 0.85f);
	style.Colors[ImGuiCol_Border]                = ImVec4(0.00f, 0.71f, 0.56f, 0.57f);
	style.Colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	style.Colors[ImGuiCol_FrameBg]               = ImVec4(0.42f, 0.42f, 0.54f, 0.44f);
	style.Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.83f, 0.65f, 0.72f, 0.45f);
	style.Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.90f, 0.51f, 0.69f, 0.67f);
	style.Colors[ImGuiCol_TitleBg]               = ImVec4(0.12f, 0.33f, 0.38f, 0.73f);
	style.Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.40f, 0.83f, 0.97f, 0.14f);
	style.Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.24f, 0.51f, 0.55f, 0.94f);
	style.Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.24f, 0.31f, 0.42f, 0.53f);
	style.Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.20f, 0.38f, 0.41f, 0.67f);
	style.Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.40f, 0.71f, 0.85f, 0.43f);
	style.Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.40f, 0.83f, 0.95f, 0.43f);
	style.Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.80f, 1.00f, 1.00f, 0.84f);
	style.Colors[ImGuiCol_ComboBg]               = ImVec4(0.20f, 0.20f, 0.20f, 0.99f);
	style.Colors[ImGuiCol_CheckMark]             = ImVec4(0.28f, 1.00f, 0.59f, 0.72f);
	style.Colors[ImGuiCol_SliderGrab]            = ImVec4(1.00f, 1.00f, 1.00f, 0.30f);
	style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.92f, 0.59f, 0.37f, 1.00f);
	style.Colors[ImGuiCol_Button]                = ImVec4(0.67f, 0.25f, 0.92f, 0.60f);
	style.Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.67f, 0.50f, 1.00f, 1.00f);
	style.Colors[ImGuiCol_ButtonActive]          = ImVec4(0.84f, 0.66f, 1.00f, 1.00f);
	style.Colors[ImGuiCol_Header]                = ImVec4(0.40f, 0.40f, 0.90f, 0.45f);
	style.Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.45f, 0.45f, 0.90f, 0.80f);
	style.Colors[ImGuiCol_HeaderActive]          = ImVec4(0.53f, 0.53f, 0.87f, 0.80f);
	style.Colors[ImGuiCol_Column]                = ImVec4(0.41f, 0.25f, 0.40f, 1.00f);
	style.Colors[ImGuiCol_ColumnHovered]         = ImVec4(1.00f, 0.60f, 0.69f, 1.00f);
	style.Colors[ImGuiCol_ColumnActive]          = ImVec4(1.00f, 0.87f, 0.72f, 1.00f);
	style.Colors[ImGuiCol_ResizeGrip]            = ImVec4(1.00f, 1.00f, 1.00f, 0.50f);
	style.Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(1.00f, 1.00f, 1.00f, 0.71f);
	style.Colors[ImGuiCol_ResizeGripActive]      = ImVec4(1.00f, 1.00f, 1.00f, 0.90f);
	style.Colors[ImGuiCol_CloseButton]           = ImVec4(0.70f, 0.50f, 1.00f, 0.55f);
	style.Colors[ImGuiCol_CloseButtonHovered]    = ImVec4(0.96f, 0.43f, 0.78f, 1.00f);
	style.Colors[ImGuiCol_CloseButtonActive]     = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
	style.Colors[ImGuiCol_PlotLines]             = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	style.Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_PlotHistogram]         = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.00f, 1.00f, 1.00f, 0.37f);
	style.Colors[ImGuiCol_ModalWindowDarkening]  = ImVec4(0.05f, 0.00f, 0.00f, 0.69f);

#ifdef _WIN32
	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	SDL_GetWindowWMInfo(window, &wmInfo);
	io.ImeWindowHandle = wmInfo.info.win.window;
#else
	(void)window;
#endif

	return true;
}

void ICIC_Shutdown()
{
	ICIC_InvalidateDeviceObjects();
	ImGui::Shutdown();
}

void ICIC_NewFrame(SDL_Window* window)
{
	if (!g_FontTexture.handle)
		ICIC_CreateDeviceObjects();

	ImGuiIO& io = ImGui::GetIO();

	// Setup display size (every frame to accommodate for window resizing)
	int w, h;
	int display_w, display_h;
	SDL_GetWindowSize(window, &w, &h);
	SDL_GL_GetDrawableSize(window, &display_w, &display_h);
	io.DisplaySize = ImVec2((float)w, (float)h);
	io.DisplayFramebufferScale = ImVec2(w > 0 ? ((float)display_w / w) : 0, h > 0 ? ((float)display_h / h) : 0);

	// Setup time step
	Uint32	time = SDL_GetTicks();
	double current_time = time / 1000.0;
	io.DeltaTime = g_Time > 0.0 ? (float)(current_time - g_Time) : (float)(1.0f / 60.0f);
	g_Time = current_time;

	// Setup inputs
	// (we already got mouse wheel, keyboard keys & characters from SDL_PollEvent())
	int mx, my;
	Uint32 mouseMask = SDL_GetMouseState(&mx, &my);
	if (SDL_GetWindowFlags(window) & SDL_WINDOW_MOUSE_FOCUS)
		io.MousePos = ImVec2((float)mx, (float)my);   // Mouse position, in pixels (set to -1,-1 if no mouse / on another screen, etc.)
	else
		io.MousePos = ImVec2(-1, -1);

	io.MouseDown[0] = g_MousePressed[0] || (mouseMask & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;		// If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
	io.MouseDown[1] = g_MousePressed[1] || (mouseMask & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
	io.MouseDown[2] = g_MousePressed[2] || (mouseMask & SDL_BUTTON(SDL_BUTTON_MIDDLE)) != 0;
	io.MouseDown[3] = g_MousePressed[3] || (mouseMask & SDL_BUTTON(SDL_BUTTON_X1)) != 0;
	io.MouseDown[4] = g_MousePressed[4] || (mouseMask & SDL_BUTTON(SDL_BUTTON_X2)) != 0;
	g_MousePressed[0] = g_MousePressed[1] = g_MousePressed[2] = false;

	io.MouseWheel = g_MouseWheel;
	g_MouseWheel = 0.0f;

	// Hide OS mouse cursor if ImGui is drawing it
	SDL_ShowCursor(io.MouseDrawCursor ? 0 : 1);

	// Start the frame
	ImGui::NewFrame();
}
