#ifndef ICI_H_INC
#define ICI_H_INC

#include <stdint.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#define GLEW_STATIC
#include <GL/glew.h>
#include "ui/imgui.h"

typedef int (*ici_rasterfn)(void *imva, uint16_t *ram, uint32_t *rgba, uint32_t slack);
int imva_raster(void *vimva, uint16_t *ram, uint32_t *rgba, uint32_t slack);
int LEMRaster(void *vlem, uint16_t *ram, uint32_t *surface, uint32_t pitch);

struct icitexture {
	GLuint handle;
	int type;
	icitexture() : handle(0), type(0) {}
};

bool ICIC_Init(SDL_Window* window);
void ICIC_Shutdown();
void ICIC_NewFrame(SDL_Window* window);
bool ICIC_ProcessEvent(SDL_Event* event);
bool ICIC_Emu_ProcessEvent(SDL_Event* event);
void ICIC_CreateHWTexture(icitexture *tex, int width, int height);
void ICIC_UpdateHWTexture(icitexture *tex, int width, int height, uint32_t *pixels);

void StartConsole();
void StartGUIConsole();
void ShowUIConsole();
void DrawUIConsole();
void LogMessage(const char *fmt, ...);

#ifdef WIN32
int snprintf(char * buf, size_t len, const char * fmt, ...);
#endif

struct NyaLEM {
	unsigned short dspmem;
	unsigned short fontmem;
	unsigned short palmem;
	unsigned short border;
	unsigned short version;
	uint32_t TTI;
	uint32_t TTFlip;
	int status;
};

struct imva_nvstate {
	uint16_t base; /* hw spec */
	uint16_t ovbase; /* hw spec */
	uint16_t ovoffset; /* hw spec */
	uint16_t colors; /* hw spec */
	uint16_t ovmode; /* hw spec */
	int blink; /* bool blink state  */
	uint32_t blink_time;
	uint32_t fgcolor;
	uint32_t bgcolor;
};

#endif
