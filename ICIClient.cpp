//
// ICIClient
//
static const char * CPRTTXT =
"Copyright (c) 2013-2016 Meisaka Yukara"
;
static const char * LICTXT =
"Permission is hereby granted, free of charge, to any person obtaining a copy "
"of this software and associated documentation files (the \"Software\"), to deal "
"in the Software without restriction, including without limitation the rights "
"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell "
"copies of the Software, and to permit persons to whom the Software is "
"furnished to do so, subject to the following conditions:\n\n"

"The above copyright notice and this permission notice shall be included in all "
"copies or substantial portions of the Software.\n\n"

"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR "
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, "
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE "
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER "
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, "
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE "
"SOFTWARE."
;

#include "ici.h"

#ifdef WIN32
// Windows target headers
#include "targetver.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include "resource.h"
#include <tchar.h>
#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "glew32s.lib")
#define ICI_GET_ERROR WSAGetLastError()
#define SHUT_RDWR SD_BOTH
#else
#define ZeroMemory(a,l)  memset((a), 0, (l))
#define SOCKET int
#define INVALID_SOCKET -1
#define ICI_GET_ERROR errno
#define closesocket(a) close(a)
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netdb.h>
#endif

#include <malloc.h>
#include <memory.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static int apprun;

static SOCKET lcs;
static bool unet;
static union msockaddr {
	sockaddr sa;
	sockaddr_in ip;
	sockaddr_in6 ip6;
} remokon;
static unsigned char *netbuff;
static unsigned char *msgbuff;
static uint32_t keybid = 0;
static uint32_t dcpuid = 0;
static uint32_t updates = 0;
static int lookups = 0;

static unsigned short virtmeme[0x10000];

struct clsobj {
	uint32_t id;
	uint32_t cid;
	char * name;
	int rec;
	void * rvstate;
	void * svlstate;
	size_t rvsize;
	icitexture uitex;
	uint32_t *pixbuf;
	ici_rasterfn draw;
	int sbw;
	int sbh;
	int rendw;
	int rendh;
	ImVec2 uvfar;
	bool win;
};

static clsobj **objlist = NULL;
static int objcount = 0;

int UpdateDisplay();
void InitKeyMap();
extern bool uiconsole_open;
bool ui_vkeyboard = false;

int NetworkClose()
{
	uint32_t i;
	int k;
	i = ICI_GET_ERROR;
	shutdown(lcs, SHUT_RDWR);
	closesocket(lcs);
	unet = false;
	if(i) LogMessage("Disconnected (Error:%d)\n", i);
	else LogMessage("Disconnected\n");

	keybid = 0;
	lookups = 0;
	updates = 0;
	for(k = 0; k < objcount; k++) {
		clsobj *kobj = objlist[k];
		if(kobj) {
			kobj->id = 0;
		}
	}
	return 0;
}

int NetworkMessageOut()
{
	timeval tvl;
	fd_set fdsr;
	int i;
	size_t mlen;
	mlen = (*(uint32_t*)msgbuff) & 0x1fff;
	mlen += 4;
	while(mlen & 3) { msgbuff[mlen] = 0; mlen++; }
	*(uint32_t*)(msgbuff+mlen) = 0xFF8859EA;
	mlen += 4;
	if(unet) {
		FD_ZERO(&fdsr);
		FD_SET(lcs, &fdsr);
		tvl.tv_sec = 0;
		tvl.tv_usec = 0;
		i = select(lcs+1, NULL, &fdsr, NULL, &tvl);
		if(i) {
			i = send(lcs, (char*)msgbuff, mlen, 0);
			if(!i) {
				// close
				NetworkClose();
				return -1;
			} else if(i > 0) {
				// data
			} else {
				// error
				NetworkClose();
				return -1;
			}
		}
	}
	return 0;
}

int WriteKey(unsigned char code, int state)
{
	if(!keybid) return -1;
	*(uint32_t*)(msgbuff) = 0x08000008;
	*(uint32_t*)(msgbuff+4) = keybid; // TODO ID the keyboard
	*(uint16_t*)(msgbuff+8) = 0x20E7 + state;
	*(uint16_t*)(msgbuff+10) = code;
	return NetworkMessageOut();
}

int ResetCPU()
{
	if(!unet) return 0;
	uint32_t *nf = (uint32_t*)msgbuff;
	if(dcpuid) {
		nf[0] = 0x02400004;
		nf[1] = dcpuid;
		NetworkMessageOut();
	}
	return 0;
}

int NetControlUpdate()
{
	if(!unet) return 0;
	uint32_t *nf = (uint32_t*)msgbuff;
	switch(lookups) {
	case 0:
		lookups++;
		nf[0] = 0x01300000; // List classes
		NetworkMessageOut();
	case 1: break;
	case 2:
		lookups++;
		nf[0] = 0x01100000; // List obj
		NetworkMessageOut();
	case 3: break;
	case 4:
		lookups++;
		nf[0] = 0x01400000; // List Heir
		NetworkMessageOut();
	case 5: break;
	case 6:
		lookups++;
		nf[0] = 0x01200000; // Syncall
		NetworkMessageOut();
		break;
	case 7:
		if(dcpuid) {
			lookups++;
		}
		break;
	}
	return 0;
}

int Object_Create(int type)
{
	int i;
	void * mem;
	if(!objlist) {
		i = 100;
		mem = malloc(i * sizeof(clsobj*));
		if(!mem) return -1;
		objlist = (clsobj**)mem;
		ZeroMemory(mem, i * sizeof(clsobj*));
		objcount = i;
	}
	for(i = 0; i < objcount; i++) {
		if(!objlist[i]) break;
	}
	if(i >= objcount) return -1;
	clsobj *nobj;
	nobj = objlist[i] = (clsobj*)malloc(sizeof(clsobj));
	ZeroMemory(nobj, sizeof(clsobj));
	nobj->rec = type;
	nobj->rendw = nobj->sbw = 512;
	nobj->rendh = nobj->sbh = 512;
	switch(type) {
	case 0:
		nobj->rendw = 140;
		nobj->rendh = 108;
		nobj->sbh = 128;
		nobj->sbw = 256;
		nobj->rvstate = malloc((nobj->rvsize = sizeof(NyaLEM)));
		nobj->draw = LEMRaster;
		nobj->cid = 0x5002;
		break;
	case 1:
		nobj->rendw = 320;
		nobj->rendh = 200;
		nobj->sbh = 256;
		nobj->rvstate = malloc((nobj->rvsize = sizeof(imva_nvstate)));
		nobj->draw = imva_raster;
		nobj->cid = 0x5005;
		break;
	default:
		break;
	}
	if(nobj->rvstate) ZeroMemory(nobj->rvstate, nobj->rvsize);
	nobj->win = true;
	ICIC_CreateHWTexture(&nobj->uitex, nobj->sbw, nobj->sbh);
	nobj->pixbuf = (uint32_t*)malloc(sizeof(uint32_t) * nobj->sbw * nobj->sbh);
	if(nobj->rendw && nobj->rendh) {
		nobj->uvfar = ImVec2(nobj->rendw / (float)nobj->sbw, nobj->rendh / (float)nobj->sbh);
		uint32_t *clsr = nobj->pixbuf;
		for(int c = nobj->sbw * nobj->sbh; c--; *(clsr++) = 0x55000000);
	}
	return i;
}

int Reconnect() {
	int crs;
	if(!remokon.sa.sa_family)
		return -1;
	if(unet) {
		NetworkClose();
	}
	lcs = socket(remokon.sa.sa_family, SOCK_STREAM, 0);
	if(lcs == INVALID_SOCKET) {
		return ICI_GET_ERROR;
	}
	int i = 1;
	if(setsockopt(lcs, IPPROTO_TCP, TCP_NODELAY, (char*)&i, sizeof(int)) < 0) {
		return ICI_GET_ERROR;
	}
	crs = connect(lcs, &remokon.sa, sizeof(remokon));
	if(crs < 0) {
		NetworkClose();
		return crs;
	} else {
		unet = true;
	}
	return 0;
}

int Connect(const char *addr, int port) {
	int crs;
	addrinfo hint;
	addrinfo *res, *ritr;
	char sport[16];
	bool found = false;
	if(unet) {
		NetworkClose();
	}
	if(!port) port = 58704;
	ZeroMemory(&remokon, sizeof(remokon));
	ZeroMemory(&hint, sizeof(addrinfo));
	snprintf(sport, 16, "%u", (uint16_t)port);
	hint.ai_family = AF_INET;
	hint.ai_protocol = IPPROTO_TCP;
	hint.ai_socktype = SOCK_STREAM;
	getaddrinfo(addr, sport, &hint, &res);
	ritr = res;
	while(ritr) {
		size_t la;
		la = ritr->ai_addrlen;
		if(la > sizeof(remokon)) la = sizeof(remokon);
		memcpy(&remokon, ritr->ai_addr, la);
		found = true;
		break;
	}
	freeaddrinfo(res);
	if(!found) return -1;
	lcs = socket(remokon.sa.sa_family, SOCK_STREAM, 0);
	if(lcs == INVALID_SOCKET) {
		return ICI_GET_ERROR;
	}
	int i = 1;
	if(setsockopt(lcs, IPPROTO_TCP, TCP_NODELAY, (char*)&i, sizeof(int)) < 0) {
		return ICI_GET_ERROR;
	}
	crs = connect(lcs, &remokon.sa, sizeof(remokon));
	if(crs < 0) {
		NetworkClose();
		return crs;
	} else {
		unet = true;
	}
	return 0;
}

void assign_id(uint32_t clazz, uint32_t id)
{
	int k;
	for(k = 0; k < objcount; k++) {
		clsobj *kobj = objlist[k];
		if(kobj && !kobj->id && kobj->cid == clazz) {
			kobj->id = id;
			return;
		}
	}
}

int NetworkMessageIn(unsigned char *in, size_t len)
{
	// data
	uint32_t mh;
	uint32_t ml;
	uint16_t mc;
	uint32_t i, l;
	int k;
	uint32_t *mi = (uint32_t*)in;
	uint16_t *mw = (uint16_t*)in;
	if(*(uint32_t*)(in+4+len) != 0xFF8859EA) {
		LogMessage("Network Packet Invalid\n");
	}
	mh = *(uint32_t*)in;
	mc = (mh >> 20) & 0xfff;
	ml = mh & 0x1fff;
	switch(mc) {
	case 0xE0:
	{
		uint32_t vsa = 0;
		uint16_t vrl = 0;
		uint8_t *inh = in + 8;
		uint8_t *ine = in + 4 + ml;
		uint8_t *omh = (uint8_t*)virtmeme;
		while(inh < ine) {
			if(vrl) {
				omh[vsa] = *inh;
				vrl--; inh++; vsa++;
				vsa &= 0x1ffff;
			} else {
				vsa = inh[0] | (inh[1] << 8);
				vrl = inh[2] | (inh[3] << 8);
				inh += 4;
				if(vsa >= 0x20000) {
					LogMessage("memsync invalid address\n");
					break;
				}
			}
		}
	}
		break;
	case 0xE1:
	{
		uint32_t vsa = 0;
		uint16_t vrl = 0;
		uint8_t *inh = in + 8;
		uint8_t *ine = in + 4 + ml;
		uint8_t *omh = (uint8_t*)virtmeme;
		while(inh < ine) {
			if(vrl) {
				omh[vsa] = *inh;
				vrl--; inh++; vsa++;
				vsa &= 0x1ffff;
			} else {
				vsa = inh[0] | (inh[1] << 8) | (inh[2] << 16) | (inh[3] << 24);
				vrl = inh[4] | (inh[5] << 8);
				inh += 6;
				if(inh < ine && vsa >= 0x20000) {
					LogMessage("memsync invalid address %04x\n", vsa);
					break;
				}
			}
		}
	}
		break;
	case 0xE2:
		l = (ml - 4);
		LogMessage("RV sync [%08x] +%d\n", mi[1], l);
		for(k = 0; k < objcount; k++) {
			clsobj *kobj = objlist[k];
			if(!kobj) continue;
			if(mi[1] == kobj->id && l <= kobj->rvsize) {
				LogMessage("sync copy\n");
				memcpy(kobj->rvstate, mi+2, l);
			}
		}
		break;
	case 0xE3:
		break;
	case 0xE4:
		break;
	case 0x200:
	case 0x201:
		LogMessage("Obj List\n");
		for(i = 0; i < (ml / 8); i++) {
			LogMessage("Obj [%08x]: %x\n", mi[1+(i*2)], mi[2+(i*2)]);
			assign_id(mi[2+(i*2)], mi[1+(i*2)]);
			switch(mi[2+(i*2)]) {
			case 0x3001:
				dcpuid = mi[1+(i*2)];
				LogMessage("DCPU [%08x]\n", mi[1+(i*2)]);
				break;
			case 0x5000:
				keybid = mi[1+(i*2)];
				LogMessage("Keyboard [%08x]\n", mi[1+(i*2)]);
				break;
			case 0x5002:
				LogMessage("Nya LEM [%08x]\n", mi[1+(i*2)]);
				break;
			}
		}
		if(mc == 0x201) lookups++;
		break;
	case 0x213:
	case 0x313:
		LogMessage("Obj Classes\n");
		{
			uint32_t iclass = 0;
			uint32_t iflag = 0;
			char iname[256];
			char idesc[256];
			int rdpoint = 0;
			int rdtype = 0;
			for(i = 0; i < ml; i++) {
				switch(rdtype) {
				case 0:
					iclass |= in[4+i] << (8*rdpoint);
					if(++rdpoint > 3) { rdpoint = 0; rdtype++; }
					break;
				case 1:
					iflag |= in[4+i] << (8*rdpoint);
					if(++rdpoint > 3) { rdpoint = 0; rdtype++; }
					break;
				case 2:
					iname[rdpoint] = in[4+i];
					if(++rdpoint > 254 || !in[4+i]) {
						iname[rdpoint] = 0;
						rdpoint = 0;
						rdtype++;
					}
					break;
				case 3:
					idesc[rdpoint] = in[4+i];
					if(++rdpoint > 254 || !in[4+i]) {
						iname[rdpoint] = 0;
						rdpoint = 0;
						rdtype = 0;
						LogMessage("Class [%08x]: F=%x \"%s\" -- %s\n", iclass, iflag, iname, idesc);
						iclass = 0;
						iflag = 0;
					}
					break;
				}
			}
		}
		if(mc == 0x313) lookups++;
		break;
	case 0x214:
	case 0x314:
		LogMessage("Obj Heirarchy\n");
		ml /= 4;
		for(i = 0; i < ml; i+=4) {
			LogMessage("Obj [%08x]: U[%08x] D[%08x] M[%08x]\n", mi[1+i], mi[3+i], mi[2+i], mi[4+i]);
		}
		if(mc == 0x314) lookups++;
		break;
	default:
		break;
	}
	return 0;
}

static const ImGuiWindowFlags icidefwin =
	ImGuiWindowFlags_NoCollapse
	| ImGuiWindowFlags_NoResize
	| ImGuiWindowFlags_NoSavedSettings
	| ImGuiWindowFlags_ShowBorders;

int ICIMain()
{
	int crs, i;
	timeval tvl;
	fd_set fdsr;

	StartConsole();
	LogMessage("Starting\n");

	if(SDL_Init(SDL_INIT_EVERYTHING)) {
		LogMessage("Error Starting SDL: %s\n", SDL_GetError());
		return 1;
	}
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	SDL_DisplayMode current;
	SDL_GetCurrentDisplayMode(0, &current);
	SDL_Window *window = SDL_CreateWindow("ICI Client", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
	SDL_GLContext glcontext = SDL_GL_CreateContext(window);
	SDL_GL_MakeCurrent(window, glcontext);
	LogMessage("Starting GL: %s\n", glGetString(GL_VERSION));
	glewExperimental = true;
	if(glewInit()) {
		LogMessage("Error Starting GL\n");
		return 1;
	}
	ICIC_Init(window);

#ifdef WIN32
	{
		ImGuiIO& io = ImGui::GetIO();
		char sysdir[512] = "";
		char sysfont[1024] = "";
		DWORD h;
		GetSystemWindowsDirectoryA(sysdir, 512);
		ImFontConfig fc;
		fc.MergeMode = true;
		io.Fonts->AddFontDefault();
		snprintf(sysfont, 1024, "%s%s", sysdir, "\\Fonts\\arial.ttf");
		h = GetFileAttributesA(sysfont);
		if(h != ~0) {
			LogMessage("Loading Font: %s\n", sysfont);
			io.Fonts->AddFontFromFileTTF(sysfont, 12, &fc, io.Fonts->GetGlyphRangesDefault());
			io.Fonts->AddFontFromFileTTF(sysfont, 12, &fc, io.Fonts->GetGlyphRangesCyrillic());
		}
		snprintf(sysfont, 1024, "%s%s", sysdir, "\\Fonts\\meiryo.ttc");
		h = GetFileAttributesA(sysfont);
		if(h != ~0) {
			LogMessage("Loading Font: %s\n", sysfont);
			io.Fonts->AddFontFromFileTTF(sysfont, 18, &fc, io.Fonts->GetGlyphRangesJapanese());
		}
		snprintf(sysfont, 1024, "%s%s", sysdir, "\\Fonts\\MSNeoGothic.ttf");
		h = GetFileAttributesA(sysfont);
		if(h != ~0) {
			LogMessage("Loading Font: %s\n", sysfont);
			io.Fonts->AddFontFromFileTTF(sysfont, 16, &fc, io.Fonts->GetGlyphRangesKorean());
		}
	}
#endif
	netbuff = (unsigned char *)malloc(8192);
	msgbuff = (unsigned char *)malloc(2048);

	Object_Create(1);
	Object_Create(0);

	for(i = 0; i < 16; i++) {
		virtmeme[20+i] = (i << 12) | (i << 8);
	}
	for(i = 0; i < 25; i++) {
		virtmeme[36+i] = "ICIClient - not connected"[i] | 0xF000;
	}
	apprun = 1;
	unsigned short vmmp = 0;

	unet = false;
	ZeroMemory(&remokon, sizeof(sockaddr_in));

	// Main message loop:
	int zm, zl, zil, nlimit;
	uint32_t zh;
	zm = zl = zil = zh = 0;
	bool ui_exit = false;
	bool ui_connect = false;
	bool ui_about = false;
	bool ui_test = false;
	char serveraddr[256] = "";
	int serverport = 0;
	StartGUIConsole();
	ShowUIConsole();
	InitKeyMap();

	while (apprun) {
		SDL_Event sdlevent;
		while (SDL_PollEvent(&sdlevent)) {
			ICIC_ProcessEvent(&sdlevent);
			ICIC_Emu_ProcessEvent(&sdlevent);
			if(sdlevent.type == SDL_QUIT) apprun = false;
		}
		ICIC_NewFrame(window);
		if(ImGui::BeginMainMenuBar()) {
			if(ImGui::BeginMenu("File")) {
				if(ImGui::MenuItem("Connect...")) {
					ui_connect = true;
					ImGui::SetWindowFocus("Connect");
				}
				if(ImGui::MenuItem("Reconnect")) {
					Reconnect();
				}
				ImGui::MenuItem("Console", 0, &uiconsole_open);
				ImGui::Separator();
				if(ImGui::MenuItem("Exit")) apprun = false;
				ImGui::EndMenu();
			}
			if(ImGui::BeginMenu("Control")) {
				if(ImGui::MenuItem("Reset CPU")) ResetCPU();
				ImGui::EndMenu();
			}
			if(ImGui::BeginMenu("Help")) {
				if(ImGui::MenuItem("About ICI")) {
					ui_about = true;
					ImGui::SetWindowFocus("About ICIClient##iciabout");
				}
				if(ImGui::MenuItem("Test UI")) ui_test = true;
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}
		if(ui_test) {
			ImGui::ShowTestWindow(&ui_test);
		}
		if(ui_about) {
			if(ImGui::Begin("About ICIClient##iciabout", &ui_about, icidefwin)) {
				ImGui::Indent();
				ImGui::Text("ICIClient Version 0.8");
				ImGui::Text(CPRTTXT);
				ImGui::SetWindowSize(ImVec2(ImGui::GetItemRectSize().x * 2.0f, 400.0f), ImGuiSetCond_Appearing);
				ImGui::Unindent();
				ImGui::Separator();
				ImGui::TextWrapped(LICTXT);
				ImGui::Separator();
			}
			ImGui::End();
		}
		if(ui_connect) {
			if(ImGui::Begin("Connect", &ui_connect, icidefwin)) {
				ImGui::SetWindowPos(ImVec2(150, 150), ImGuiSetCond_Appearing);
				ImGui::PushItemWidth(200);
				if(ImGui::InputText("Server", serveraddr, 256, ImGuiInputTextFlags_EnterReturnsTrue)) {
					if(serverport < 0) serverport = 0;
					if(serverport > 65535) serverport = 65535;
					Connect(serveraddr, serverport);
					ui_connect = false;
				}
				ImGui::SliderInt("Port", &serverport, 0, 65535);
				if(serverport < 0) serverport = 0;
				if(serverport > 65535) serverport = 65535;
				if(ImGui::Button("Connect", ImVec2(100,0))) {
					Connect(serveraddr, serverport);
					ui_connect = false;
				}
				ImGui::SameLine();
				if(ImGui::Button("Cancel", ImVec2(100,0))) {
					ui_connect = false;
				}
			}
			ImGui::End();
		}
		if(unet) {
			nlimit = 0;
			do {
				FD_ZERO(&fdsr);
				FD_SET(lcs, &fdsr);
				tvl.tv_sec = 0;
				tvl.tv_usec = 0;
				crs = select(lcs+1, &fdsr, NULL, NULL, &tvl);
				if(crs) {
					if(zm < 4) {
						i = recv(lcs, (char*)(netbuff+zm), 4-zm, 0);
					} else if(zil < zl) {
						i = recv(lcs, (char*)(netbuff+zm+zil), zl-zil, 0);
					} else {
						LogMessage("Framing Error [0x%08x]: %x\n", *(unsigned int*)netbuff, zil);
						zm = zil = zl = zh = 0;
						i = 0;
					}
					if(!i) {
						// close
						NetworkClose();
					} else if(i > 0) {
						if(zm < 4) zm += i;
						if(zm >= 4) {
							if(!zh) {
								zh = *(unsigned int*)netbuff;
								if(!zh) {
									zm = 0; // handle keepalive
									zl = zil = 0;
									LogMessage("Net Keepalive\n");
								} else {
									zl = zh & 0x1fff;
									if(zl & 3) zl += 4 - (zl & 3);
									zl += 4;
									zil = 0;
								}
							} else if(zil < zl) {
								zil += i;
							}
							if(zil >= zl) {
								NetworkMessageIn(netbuff, zil-4);
								zl = zil = zh = zm = 0;
							}
						}
					} else {
						// error
						NetworkClose();
						crs = 0;
					}
				}
			} while(crs && ++nlimit < 32);
		} else {
			zm = zl = zil = zh = 0;
		}
		NetControlUpdate();
		UpdateDisplay();
		DrawUIConsole();
		// Rendering
		{
			ImGuiIO &io = ImGui::GetIO();
			glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
		}
		glClearColor(0.1f, 0.1f, 0.1f, 0);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui::Render();
		SDL_GL_SwapWindow(window);
		vmmp = rand();
	}
	if(objlist) {
		for(i = 0; i < objcount; i++) {
			if(objlist[i]) {
				free(objlist[i]);
			}
		}
		free(objlist);
	}
	free(netbuff);
	free(msgbuff);
	SDL_GL_DeleteContext(glcontext);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}

/* Main entry points, no command line options */
#ifdef WIN32
int APIENTRY
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	WSADATA lwsa;
	WSAStartup(MAKEWORD(2,0), &lwsa);
	int r = ICIMain();
	WSACleanup();
	return r;
}
#else
int main(int argc, char**argv, char**env)
{
	return ICIMain();
}
#endif

int UpdateDisplay()
{
	int i;
	char wtitle[256];
	ui_vkeyboard = false;
	for(i = 0; i < objcount; i++) {
		clsobj *nobj = objlist[i];
		if((!nobj)) continue;
		if(!nobj->rvstate) continue;
		snprintf(wtitle, 256, "Output###display-%x", i);
		if(nobj->win) {
			if(ImGui::Begin(wtitle, &nobj->win, icidefwin)) {
				if(nobj->draw) {
					nobj->draw(nobj->rvstate, virtmeme, nobj->pixbuf, nobj->sbw);
					ICIC_UpdateHWTexture(&nobj->uitex, nobj->sbw, nobj->sbh, nobj->pixbuf);
					ImGui::Image(&nobj->uitex, ImVec2(nobj->rendw*2, nobj->rendh*2), ImVec2(0,0), nobj->uvfar);
				}
				if(ImGui::IsRootWindowOrAnyChildFocused()) {
					ui_vkeyboard = true;
				}
			}
			ImGui::End();
		}
	}
	return 0;
}

static int modflag = 0;

uint16_t vkeyb_n[512];
uint16_t vkeyb_s[512];
uint16_t vkeyb_c[512];
uint8_t vkeyb_dn[512];

void InitKeyMap() /* DCPU Generic Keyboard keymapping */
{
	memset(vkeyb_n, 0, sizeof(vkeyb_n));
	memset(vkeyb_s, 0, sizeof(vkeyb_s));
	memset(vkeyb_c, 0, sizeof(vkeyb_c));
	memset(vkeyb_dn, 0, sizeof(vkeyb_dn));
	int i;
	for(i = 0; i < 9; i++) {
		vkeyb_n[SDL_SCANCODE_1+i] = '1' + i;
	}
	vkeyb_n[SDL_SCANCODE_0] = '0';
	vkeyb_n[SDL_SCANCODE_COMMA] = ',';
	vkeyb_n[SDL_SCANCODE_PERIOD] = '.';
	vkeyb_n[SDL_SCANCODE_SLASH] = '/';
	vkeyb_n[SDL_SCANCODE_SEMICOLON] = ';';
	vkeyb_n[SDL_SCANCODE_APOSTROPHE] = '\'';
	vkeyb_n[SDL_SCANCODE_LEFTBRACKET] = '[';
	vkeyb_n[SDL_SCANCODE_RIGHTBRACKET] = ']';
	vkeyb_n[SDL_SCANCODE_BACKSLASH] = '\\';
	vkeyb_n[SDL_SCANCODE_MINUS] = '-';
	vkeyb_n[SDL_SCANCODE_EQUALS] = '=';
	vkeyb_n[SDL_SCANCODE_GRAVE] = '`';
	vkeyb_n[SDL_SCANCODE_SPACE] = ' ';
	vkeyb_n[SDL_SCANCODE_BACKSPACE] = 0x10;
	vkeyb_n[SDL_SCANCODE_RETURN] = 0x11;
	vkeyb_n[SDL_SCANCODE_KP_ENTER] = 0x11;
	vkeyb_n[SDL_SCANCODE_INSERT] = 0x12;
	vkeyb_n[SDL_SCANCODE_DELETE] = 0x13;
	vkeyb_n[SDL_SCANCODE_UP] = 0x80;
	vkeyb_n[SDL_SCANCODE_DOWN] = 0x81;
	vkeyb_n[SDL_SCANCODE_LEFT] = 0x82;
	vkeyb_n[SDL_SCANCODE_RIGHT] = 0x83;
	vkeyb_n[SDL_SCANCODE_LSHIFT] = 0x90;
	vkeyb_n[SDL_SCANCODE_RSHIFT] = 0x90;
	vkeyb_n[SDL_SCANCODE_LCTRL] = 0x91;
	vkeyb_n[SDL_SCANCODE_RCTRL] = 0x91;
	memcpy(vkeyb_s, vkeyb_n, sizeof(vkeyb_n));
	memcpy(vkeyb_c, vkeyb_n, sizeof(vkeyb_n));
	for(i = 0; i < 26; i++) {
		vkeyb_n[SDL_SCANCODE_A+i] = 'a' + i;
		vkeyb_s[SDL_SCANCODE_A+i] = 'A' + i;
		vkeyb_c[SDL_SCANCODE_A+i] = 'A' + i;
	}
	vkeyb_s[SDL_SCANCODE_1] = '!';
	vkeyb_s[SDL_SCANCODE_2] = '@';
	vkeyb_s[SDL_SCANCODE_3] = '#';
	vkeyb_s[SDL_SCANCODE_4] = '$';
	vkeyb_s[SDL_SCANCODE_5] = '%';
	vkeyb_s[SDL_SCANCODE_6] = '^';
	vkeyb_s[SDL_SCANCODE_7] = '&';
	vkeyb_s[SDL_SCANCODE_8] = '*';
	vkeyb_s[SDL_SCANCODE_9] = '(';
	vkeyb_s[SDL_SCANCODE_0] = ')';
	vkeyb_s[SDL_SCANCODE_COMMA] = '<';
	vkeyb_s[SDL_SCANCODE_PERIOD] = '>';
	vkeyb_s[SDL_SCANCODE_SLASH] = '?';
	vkeyb_s[SDL_SCANCODE_SEMICOLON] = ':';
	vkeyb_s[SDL_SCANCODE_APOSTROPHE] = '"';
	vkeyb_s[SDL_SCANCODE_LEFTBRACKET] = '{';
	vkeyb_s[SDL_SCANCODE_RIGHTBRACKET] = '}';
	vkeyb_s[SDL_SCANCODE_BACKSLASH] = '|';
	vkeyb_s[SDL_SCANCODE_MINUS] = '_';
	vkeyb_s[SDL_SCANCODE_EQUALS] = '+';
	vkeyb_s[SDL_SCANCODE_GRAVE] = '~';
}

int KeyTrans(uint32_t sc, bool isdown)
{
	sc &= 0x00ffff;
	if(sc >= sizeof(vkeyb_n)) return 0;
	if(isdown) {
		switch(sc) {
		case SDL_SCANCODE_LSHIFT: modflag |= 1; break;
		case SDL_SCANCODE_RSHIFT: modflag |= 2; break;
		case SDL_SCANCODE_LCTRL: modflag |= 4; break;
		case SDL_SCANCODE_RCTRL: modflag |= 8; break;
		}
	} else {
		switch(sc) { /* prevent sending keyup if both modifiers were held */
		case SDL_SCANCODE_LSHIFT: modflag &= ~1;
			if(modflag & 3) return 0;
			break;
		case SDL_SCANCODE_RSHIFT: modflag &= ~2;
			if(modflag & 3) return 0;
			break;
		case SDL_SCANCODE_LCTRL: modflag &= ~4;
			if(modflag & 12) return 0;
			break;
		case SDL_SCANCODE_RCTRL: modflag &= ~8;
			if(modflag & 12) return 0;
			break;
		}
	}
	if(modflag & 3) return vkeyb_s[sc];
	if(modflag & 12) return vkeyb_c[sc];
	return vkeyb_n[sc];
}

bool ICIC_Emu_ProcessEvent(SDL_Event* event)
{
	ImGuiIO& io = ImGui::GetIO();
	switch (event->type) {
	case SDL_MOUSEWHEEL:
		return true;
	case SDL_MOUSEBUTTONDOWN:
		return true;
	case SDL_TEXTINPUT:
		return true;
	case SDL_KEYDOWN:
		if(!io.WantCaptureKeyboard && ui_vkeyboard && !event->key.repeat) {
			int kt = KeyTrans(event->key.keysym.scancode, true);
			if(!kt) {
				LogMessage("Unmapped KeyDn: %d %d\n", event->key.keysym.scancode, event->key.keysym.sym);
			} else {
				WriteKey(kt, 0);
				vkeyb_dn[kt] = 1;
			}
		}
		return true;
	case SDL_KEYUP:
		if(!io.WantCaptureKeyboard && ui_vkeyboard && !event->key.repeat) {
			int kt = KeyTrans(event->key.keysym.scancode, false);
			if(!kt) {
				LogMessage("Unmapped KeyUp: %d %d\n", event->key.keysym.scancode, event->key.keysym.sym);
			} else {
				if(vkeyb_dn[kt]) WriteKey(kt, 1);
				vkeyb_dn[kt] = 0;
			}
		}
		return true;
	}
	if(modflag && (io.WantCaptureKeyboard || !ui_vkeyboard)) {
		for(int i = 0; i < 512; i++) {
			if(vkeyb_dn[i]) {
				WriteKey(i, 1);
				vkeyb_dn[i] = 0;
			}
		}
		modflag = 0;
	}
	return false;
}
