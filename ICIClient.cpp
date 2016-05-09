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
#include "targetver.h"
#define WIN32_LEAN_AND_MEAN
// Windows headers
#include <windows.h>
#include <WinSock2.h>

#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <stdarg.h>
#include <tchar.h>
#include <stdio.h>
#include <stdint.h>

extern "C" {
#include <ddraw.h>
}
#include "resource.h"

#define MAX_LOADSTRING 100

// Global Variables:
static HINSTANCE hInst;							// current instance
static TCHAR szTitle[MAX_LOADSTRING];		// The title bar text
static TCHAR szWindowClass[MAX_LOADSTRING];	// the main window class name
static HANDLE ECCON; // The console

static HMODULE hmodDDRAW;

static LPDIRECTDRAW7 lpdd7 = NULL;
static LPDIRECTDRAWCLIPPER lpddclp = NULL;
static LPDIRECTDRAWSURFACE7 lpddpri = NULL;
static LPDIRECTDRAWSURFACE7 lpddback = NULL;

static HWND hwMain;
static HDC hdcMain;
static int apprun;
static POINT dragp;
static POINT mpoint;
static int dragon = -1;

static SOCKET lcs;
static bool unet;
static sockaddr_in remokon;
static unsigned char *netbuff;
static uint32_t keybid = 0;
static uint32_t nyaid = 0;
static uint32_t updates = 0;
static int lookups = 0;

static unsigned short virtmeme[0x10000];
static unsigned short deffont[] = {
0xffff,0xffff, 0xffff,0xffff, 0xffff,0xffff, 0xffff,0xffff, 0x242e,0x2400, 0x082A,0x0800, 0x0008,0x0000, 0x0808,0x0808,
0x00ff,0x0000, 0x00f8,0x0808, 0x08f8,0x0000, 0x080f,0x0000, 0x000f,0x0808, 0x00ff,0x0808, 0x08f8,0x0808, 0x08ff,0x0000,
0x080f,0x0808, 0x08ff,0x0808, 0x6633,0x99cc, 0x9933,0x66cc, 0xfef8,0xe080, 0x7f1f,0x0701, 0x0107,0x1f7f, 0x80e0,0xf8fe,
0x5500,0xAA00, 0x55AA,0x55AA, 0xffAA,0xff55, 0x0f0f,0x0f0f, 0xf0f0,0xf0f0, 0x0000,0xffff, 0xffff,0x0000, 0xffff,0xffff,
0x0000,0x0000, 0x005f,0x0000, 0x0300,0x0300, 0x3e14,0x3e00, 0x266b,0x3200, 0x611c,0x4300, 0x3629,0x7650, 0x0002,0x0100,
0x1c22,0x4100, 0x4122,0x1c00, 0x1408,0x1400, 0x081C,0x0800, 0x4020,0x0000, 0x0808,0x0800, 0x0040,0x0000, 0x601c,0x0300,
0x3e49,0x3e00, 0x427f,0x4000, 0x6259,0x4600, 0x2249,0x3600, 0x0f08,0x7f00, 0x2745,0x3900, 0x3e49,0x3200, 0x6119,0x0700,
0x3649,0x3600, 0x2649,0x3e00, 0x0024,0x0000, 0x4024,0x0000, 0x0814,0x2241, 0x1414,0x1400, 0x4122,0x1408, 0x0259,0x0600,
0x3e59,0x5e00, 0x7e09,0x7e00, 0x7f49,0x3600, 0x3e41,0x2200, 0x7f41,0x3e00, 0x7f49,0x4100, 0x7f09,0x0100, 0x3e41,0x7a00,
0x7f08,0x7f00, 0x417f,0x4100, 0x2040,0x3f00, 0x7f08,0x7700, 0x7f40,0x4000, 0x7f06,0x7f00, 0x7f01,0x7e00, 0x3e41,0x3e00,
0x7f09,0x0600, 0x3e41,0xbe00, 0x7f09,0x7600, 0x2649,0x3200, 0x017f,0x0100, 0x3f40,0x3f00, 0x1f60,0x1f00, 0x7f30,0x7f00,
0x7708,0x7700, 0x0778,0x0700, 0x7149,0x4700, 0x007f,0x4100, 0x031c,0x6000, 0x0041,0x7f00, 0x0201,0x0200, 0x8080,0x8000,
0x0001,0x0200, 0x2454,0x7800, 0x7f44,0x3800, 0x3844,0x2800, 0x3844,0x7f00, 0x3854,0x5800, 0x087e,0x0900, 0x4854,0x3c00,
0x7f04,0x7800, 0x447d,0x4000, 0x2040,0x3d00, 0x7f10,0x6c00, 0x417f,0x4000, 0x7c18,0x7c00, 0x7c04,0x7800, 0x3844,0x3800,
0x7c14,0x0800, 0x0814,0x7c00, 0x7c04,0x0800, 0x4854,0x2400, 0x043e,0x4400, 0x3c40,0x7c00, 0x1c60,0x1c00, 0x7c30,0x7c00,
0x6c10,0x6c00, 0x4c50,0x3c00, 0x6454,0x4c00, 0x0836,0x4100, 0x0077,0x0000, 0x4136,0x0800, 0x0201,0x0201, 0x0205,0x0200};
static unsigned short defpal[] = {
	0x0000,0x000a,0x00a0,0x00aa,
	0x0a00,0x0a0a,0x0a50,0x0aaa,
	0x0555,0x055f,0x05f5,0x05ff,
	0x0f55,0x0f5f,0x0ff5,0x0fff
};
static unsigned int NyaLogo[] = {
0x04010401,0x02010201,0x01010101,0x80818081,0xC041C041,
0xA021A021,0x90119011,0x88098809,0x84058405,0x82038203,
0x81018101,0x80808080,0x80408040,0x80208020,
0xCA61D1D5,0xDD353AEE,0x19198992,0x67A4A1DD,0xD4956550,
0x3FFC3FFC
};
struct NyaLEM {
	unsigned short dspmem;
	unsigned short fontmem;
	unsigned short palmem;
	unsigned short border;
	unsigned short version;
	DWORD TTI;
	DWORD TTFlip;
	int status;
	unsigned short cachepal[16];
	unsigned short cachefont[256];
	unsigned short cachedisp[384];
} MyStdDisp;

ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	DlgConnect(HWND, UINT, WPARAM, LPARAM);
int UpdateDisplay();

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
} MyIMVA;

/* how we access memory */
#define IMVA_RD(m,a)  (m[a])

static void imva_colors(struct imva_nvstate *imva)
{
	uint8_t r,g,b,c;
	uint32_t fg, bg;
	fg = imva->colors & 0xfff;
	c = (imva->colors & 0xf000) >> 9;
	b = ((fg & 0xf00) >> 4 | (fg & 0xf00) >> 8);
	g = ((fg & 0x0f0) >> 4 | (fg & 0x0f0));
	r = ((fg & 0x00f) << 4 | (fg & 0x00f));
	bg = ((r >> 5) + c) | (((g >> 5) + c) << 8) | (((b >> 5) + c) << 16);
	if(c > r) r = c + 15; else r -= c;
	if(c > g) g = c + 15; else g -= c;
	if(c > b) b = c + 15; else b -= c;
	fg = r | (g << 8) | (b << 16);
	imva->fgcolor = fg;
	imva->bgcolor = bg;
}

void imva_reset(struct imva_nvstate *imva)
{
	imva->base = 0;
	imva->ovbase = 0;
	imva->colors = 0x011f1;
	imva->ovmode = 0;
	imva->blink = 0;
	imva->blink_time = 0;
	imva_colors(imva);
}

/* msg is assumed to point at registers A-J in order */
void imva_interrupt(struct imva_nvstate *imva, uint16_t *msg)
{
	switch(msg[0]) {
	case 0:
		imva->base = msg[1];
		break;
	case 1:
		imva->ovbase = msg[1];
		imva->ovoffset = msg[2];
		break;
	case 2:
		if(msg[1] != imva->colors) {
			imva->colors = msg[1];
			imva_colors(imva);
		}
		imva->ovmode = msg[2];
		break;
	case 0x0ffff:
		imva_reset(imva);
		break;
	default:
		break;
	}
}

/* imva is the device state
 * ram is the entire 64k words
 * rgba is a 320x200 RGBA pixel array (256000 bytes minimum)
 * slack is extra pixels to move to next line (0 if exactly sized buffer)
 */
int imva_raster(struct imva_nvstate *imva, uint16_t *ram, uint32_t *rgba, uint32_t slack)
{
	uint32_t bg, fg;
	uint16_t raddr, ova, ovo, ove;
	bg = imva->bgcolor;
	fg = imva->fgcolor;
	raddr = imva->base;
	if(!raddr) return 0; /* stand-by mode */

	if(imva->ovmode & 0xf) {
		uint32_t ctk = GetTickCount();
		if(!imva->blink_time) imva->blink_time = ctk;
		if(ctk >= imva->blink_time) {
			imva->blink_time += (imva->ovmode & 0xf) * 100;
			imva->blink =! imva->blink;
		}
	}
	int omode;
	omode = (imva->ovmode >> 4) & 3;
	uint32_t x, y, z, ovflag, cell, of, v, vv;
	cell = 0;
	z = 0;
	ovflag = 0;
	of = 0;
	ova = imva->ovbase;
	ovo = raddr + imva->ovoffset;
	ove = ovo + (40*8); /* cell line words */
	if(!ova || imva->blink) ovo = raddr - 1;
	for(y = 200; y--; of ^= 8) {
		z = 0;
		for(cell = 40; cell--; ) {
			v = (IMVA_RD(ram,raddr) >> of) & 0x00ff;
			if(raddr == ovo || z) {
				vv = IMVA_RD(ram,ova + z) >> of;
				ovflag = 1;
				z ^= 1;
				switch(omode) {
				case 0: v |= vv; break;
				case 1: v ^= vv; break;
				case 2: v &= vv; break;
				case 3: v = vv; break;
				}
			}
			for(x = 8; x--; *(rgba++) = (v & (1 << (x & 7))) ? fg : bg);
			raddr++;
		}
		rgba += slack;
		if(!of) {
			raddr -= 40;
		} else {
			if(ovflag) {
				ova += 2;
				ovflag = 0;
				if(ovo != ove) ovo += 40;
			}
		}
	}
	return 1;
}

void LogMessage(const char *fmt, ...) {
	DWORD cw;
	char conbuf[512];
	int conlen;
	va_list val;
	va_start(val, fmt);
	conlen = vsnprintf(conbuf, sizeof(conbuf), fmt, val);
	va_end(val);
	WriteConsoleA(ECCON, conbuf, conlen, &cw, NULL);
}

int NetworkClose()
{
	uint32_t i;
	i = WSAGetLastError();
	shutdown(lcs, SD_BOTH);
	closesocket(lcs);
	unet = false;
	if(i) LogMessage("Disconnected (Error:%d)\n", i);
	else LogMessage("Disconnected\n");

	keybid = 0;
	nyaid = 0;
	lookups = 0;
	updates = 0;
	return 0;
}

int NetworkMessageOut(unsigned char *in, size_t len)
{
	timeval tvl;
	fd_set fdsr;
	int i;
	if(unet) {
		FD_ZERO(&fdsr);
		FD_SET(lcs, &fdsr);
		tvl.tv_sec = 0;
		tvl.tv_usec = 0;
		i = select(3, NULL, &fdsr, NULL, &tvl);
		if(i) {
			i = send(lcs, (char*)in, len, 0);
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
	unsigned char nf[16];
	*(uint32_t*)(nf) = 0x08000008;
	*(uint32_t*)(nf+4) = keybid; // TODO ID the keyboard
	*(uint16_t*)(nf+8) = 0x20E7 + state;
	*(uint16_t*)(nf+10) = code;
	return NetworkMessageOut(nf, 12);
}

int NetControlUpdate()
{
	if(!unet) return 0;
	unsigned char nf[16];
	switch(lookups) {
	case 0:
		lookups++;
		*(uint32_t*)(nf) = 0x01100000;
		NetworkMessageOut(nf, 4);
	case 1:
		break;
	case 2:
		lookups++;
		*(uint32_t*)(nf) = 0x01200000;
		NetworkMessageOut(nf, 4);
		break;
	}
	return 0;
}

struct clsobj {
	uint32_t id;
	char * name;
	int rec;
	void * rvstate;
	void * svlstate;
	LPDIRECTDRAWSURFACE7 lpddofs;
	RECT rect;
	RECT srect;
	POINT scrn;
};

static clsobj **objlist = NULL;
static int objcount = 0;

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
	objlist[i] = (clsobj*)malloc(sizeof(clsobj));
	ZeroMemory(objlist[i], sizeof(clsobj));
	objlist[i]->rec = type;
	DDSURFACEDESC2 ddsd;
	HRESULT hr;
	ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	ddsd.dwHeight = 512;
	ddsd.dwWidth = 512;
	switch(type) {
	case 0:
		SetRect(&objlist[i]->srect,0,0,140,108);
		ddsd.dwHeight = 128;
		ddsd.dwWidth = 256;
		break;
	case 1:
		SetRect(&objlist[i]->srect,0,0,320,200);
		ddsd.dwHeight = 256;
		break;
	default:
		SetRect(&objlist[i]->srect,0,0,512,512);
		break;
	}
	ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
	hr = lpdd7->CreateSurface(&ddsd, &objlist[i]->lpddofs, NULL);

	SetRect(&objlist[i]->rect,0,0,objlist[i]->srect.right*2,objlist[i]->srect.bottom*2);
	return i;
}

int Reconnect() {
	int crs;
	if(!remokon.sin_addr.S_un.S_addr)
		return -1;
	if(unet) {
		NetworkClose();
	}
	lcs = socket(AF_INET, SOCK_STREAM, 0);
	if(lcs == INVALID_SOCKET) {
		return WSAGetLastError();
	}
	int i = 1;
	if(setsockopt(lcs, IPPROTO_TCP, TCP_NODELAY, (char*)&i, sizeof(int)) < 0) {
		return WSAGetLastError();
	}
	crs = connect(lcs, (struct sockaddr*)&remokon, sizeof(struct sockaddr_in));
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
	if(unet) {
		NetworkClose();
	}
	ZeroMemory(&remokon, sizeof(sockaddr_in));
	remokon.sin_addr.S_un.S_addr = inet_addr(addr);
	if(!remokon.sin_addr.S_un.S_addr)
		return -1;
	remokon.sin_family = AF_INET;
	remokon.sin_port = htons((u_short)port);
	lcs = socket(AF_INET, SOCK_STREAM, 0);
	if(lcs == INVALID_SOCKET) {
		return WSAGetLastError();
	}
	int i = 1;
	if(setsockopt(lcs, IPPROTO_TCP, TCP_NODELAY, (char*)&i, sizeof(int)) < 0) {
		return WSAGetLastError();
	}
	crs = connect(lcs, (struct sockaddr*)&remokon, sizeof(struct sockaddr_in));
	if(crs < 0) {
		NetworkClose();
		return crs;
	} else {
		unet = true;
	}
	return 0;
}

int NetworkMessageIn(unsigned char *in, size_t len)
{
	// data
	uint32_t mh;
	uint32_t ml;
	uint16_t mc;
	uint32_t i, l;
	uint32_t *mi = (uint32_t*)in;
	uint16_t *mw = (uint16_t*)in;
	mh = *(uint32_t*)in;
	mc = (mh >> 20) & 0xfff;
	ml = mh & 0xfffff;
	switch(mc) {
	case 0xE0:
		l = (ml - 6);
		if((mw[4]) + (l) < 0x20000) {
			memcpy(virtmeme+(mw[4]/2), mw+5, l);
			updates++;
		}
		break;
	case 0xE1:
		l = (ml - 8);
		if((mi[2] & 0x1ffff) + l < 0x20000) {
			memcpy(virtmeme+(mi[2]/2), mi+3, l);
			updates++;
		}
		break;
	case 0xE2:
		l = (ml - 4);
		LogMessage("RV sync [%08x] +%d\n", mi[1], l);
		if(mi[1] == nyaid && l <= sizeof(struct NyaLEM)) {
			LogMessage("sync copy\n");
			memcpy(&MyStdDisp, mi+2, l);
		}
		break;
	case 0xE3:
		break;
	case 0xE4:
		break;
	case 0x200:
	case 0x201:
		for(i = 0; i < (ml / 8); i++) {
			LogMessage("Obj [%08x]: %x\n", mi[1+(i*2)], mi[2+(i*2)]);
			switch(mi[2+(i*2)]) {
			case 0x5000:
				keybid = mi[1+(i*2)];
				LogMessage("Keyboard [%08x]\n", mi[1+(i*2)]);
				break;
			case 0x5002:
				nyaid = mi[1+(i*2)];
				LogMessage("Nya LEM [%08x]\n", mi[1+(i*2)]);
				break;
			}
		}
		if(mc == 0x201) lookups++;
		break;
	default:
		break;
	}
	return 0;
}

int APIENTRY
_tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	MSG msg;
	HACCEL hAccelTable;
	WSADATA lwsa;
	int crs, i;
	timeval tvl;
	fd_set fdsr;

	COORD dws = {80,400};
	AllocConsole();
	ECCON = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	SetConsoleScreenBufferSize(ECCON, dws);
	SetConsoleActiveScreenBuffer(ECCON);

	LogMessage("Start\n");

	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_ICIC, szWindowClass, MAX_LOADSTRING);
	netbuff = (unsigned char *)malloc(8192);
	MyRegisterClass(hInstance);
	if (!InitInstance (hInstance, nCmdShow)) {
		return FALSE;
	}
	WSAStartup(MAKEWORD(2,0), &lwsa);
	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_ICIC));

	DirectDrawCreateEx(NULL ,(LPVOID*)&lpdd7, IID_IDirectDraw7, NULL);
	lpdd7->SetCooperativeLevel(hwMain, DDSCL_NORMAL | DDSCL_MULTITHREADED);
	{
		DDSURFACEDESC2 ddsd;
		HRESULT hr;
		ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
		ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
		ddsd.dwSize = sizeof(DDSURFACEDESC2);
		ddsd.dwFlags = DDSD_CAPS;
		hr = lpdd7->CreateSurface(&ddsd, &lpddpri, NULL);
		lpddpri->GetSurfaceDesc(&ddsd);
		LogMessage("Main surface: %u, %u\n", ddsd.dwWidth, ddsd.dwHeight);
		hr = lpdd7->CreateClipper(0, &lpddclp, NULL);
		hr = lpddclp->SetHWnd(0, hwMain);
		hr = lpddpri->SetClipper(lpddclp);

		uint32_t w, h;
		w = ddsd.dwWidth;
		h = ddsd.dwHeight;
		ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY;
		ddsd.dwSize = sizeof(DDSURFACEDESC2);
		ddsd.dwHeight = h;
		ddsd.dwWidth = w;
		ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
		hr = lpdd7->CreateSurface(&ddsd, &lpddback, NULL);
		if(hr) LogMessage("ERR: %x\n", hr);
	}

	Object_Create(1);
	Object_Create(0);
	MyStdDisp.status = 1;
	MyStdDisp.dspmem = 4;
	MyStdDisp.fontmem = 0;
	MyStdDisp.palmem = 0;
	imva_reset(&MyIMVA);
	MyIMVA.base = 0xe900;
	MyIMVA.ovbase = 1;
	MyIMVA.ovoffset = 41;
	MyIMVA.ovmode = 0x0035;
	virtmeme[1] = 0xAAAA;
	virtmeme[2] = 0x5555;
	virtmeme[3] = 0xffff;
	virtmeme[41] = 0xAAAA;
	virtmeme[42] = 0x5555;
	virtmeme[43] = 0xffff;
	apprun = TRUE;
	unsigned short vmmp = 0;

	for(i = 0; i < 384; i++) MyStdDisp.cachedisp[i] = 0x0000f120;
	unet = false;
	ZeroMemory(&remokon, sizeof(sockaddr_in));
	
	// Main message loop:
	int zm, zl, zil, nlimit;
	uint32_t zh;
	zm = zl = zil = zh = 0;
	while (apprun) {
		while(PeekMessage(&msg, NULL, 0, 0, TRUE)) {
			if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		if(unet) {
			nlimit = 0;
			do {
				FD_ZERO(&fdsr);
				FD_SET(lcs, &fdsr);
				tvl.tv_sec = 0;
				tvl.tv_usec = 0;
				crs = select(3, &fdsr, NULL, NULL, &tvl);
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
									zl = zh & 0xfffff;
									zil = 0;
								}
							} else if(zil < zl) {
								zil += i;
							}
							if(zil >= zl) {
								NetworkMessageIn(netbuff, zil);
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
		vmmp = rand();
		Sleep(10);
	}
	PostQuitMessage(0);
	while (GetMessage(&msg, NULL, 0, 0)) {
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	if(lpddpri) lpddpri->Release();
	if(objlist) {
		for(i = 0; i < objcount; i++) {
			if(objlist[i]) {
				if(objlist[i]->lpddofs) objlist[i]->lpddofs->Release();
				free(objlist[i]);
			}
		}
		free(objlist);
	}
	free(netbuff);
	if(lpddclp) lpddclp->Release();
	if(lpddback) lpddback->Release();
	if(lpdd7) lpdd7->Release();
	WSACleanup();
	return (int) msg.wParam;
}

int LEMRaster(NyaLEM *lem, unsigned int height, unsigned int width, unsigned int *surface, unsigned int pitch)
{
	unsigned int x,y,sl,yo,yu;
	unsigned int vtw, vtb, vta;
	unsigned int cla, clb, clf, ctk;
	unsigned short vmmp;
	unsigned short* lclfont;
	unsigned int ccpal[16];
	int dn;
	dn = 0;

	if(lem->status & 8) {
		ctk = GetTickCount();
		if(ctk < lem->TTI) return 1;
		lem->status = 5;
	}
	if(lem->status == 0x0010) {
		return 1;
	}
	if(lem->fontmem) {
		lclfont = lem->cachefont;
	} else {
		lclfont = deffont;
	}

	ctk = GetTickCount();
	if(lem->status == 1) {
		lem->status = 9;
		dn = 2;
		lem->TTI = ctk + 2500;
	}
	if((lem->status & 0x0011) == 0) {
		dn = 4;
		lem->status = 0x0010;
	}

	switch(dn) {
	case 0:
		
		for(x = 0; x < 16; x++) {
			if(lem->palmem) {
				vtb = virtmeme[(lem->palmem + x) & 0xffff];
			} else {
				vtb = defpal[x];
			}
			vta = ((vtb & 0x000f) | ((vtb & 0x000f) << 4));
			vta |= (((vtb & 0x000f0) << 4) | ((vtb & 0x000f0) << 8));
			vta |= (((vtb & 0x000f00) << 8) | ((vtb & 0x000f00) << 12));
			ccpal[x] = vta;
		}

		vta = ccpal[lem->border & 0x0f];
		yo = 0;
		for(y = 96+12; y > 0; y--) {
			for(x = 0; x < 128+12; x++) {
				(surface)[x+yo] = vta;
			}
			yo += pitch;
		}

		yo = 0;
		vmmp = lem->dspmem;
		//vmmp = 0;
		if(ctk > lem->TTFlip || lem->TTFlip > ctk + 10000) {
			lem->status ^= 2;
			lem->TTFlip = ctk + 800;
		}
		yo = (pitch * 6) + 6;
		for(sl = 0; sl < 12; sl++) {
			vtb = 0x00100;
			vta = 0x0001;
			for(y = 8; y > 0; y--) {
				for(x = 0; x < 32; x++) {
					vtw = virtmeme[(uint16_t)(vmmp + x)];
					//vtw = lem->cachedisp[vmmp + x];
					clb = ccpal[(vtw >> 8) & 0x0f];
					clf = ccpal[(vtw >> 12) & 0x0f];
					if((vtw & 0x0080) && (lem->status & 2))
						cla = clb;
					else
						cla = clf;
					vtw &= 0x0000007f;
					vtw <<= 1;
					(surface)[yo+0] = ((lclfont[vtw]) & vtb) ? cla : clb ;
					(surface)[yo+1] = ((lclfont[vtw]) & vta) ? cla : clb ;
					(surface)[yo+2] = ((lclfont[vtw+1]) & vtb) ? cla : clb ;
					(surface)[yo+3] = ((lclfont[vtw+1]) & vta) ? cla : clb ;
					yo += 4;
				}
				vtb <<= 1;
				vta <<= 1;
				yo += pitch - (128);
			}
			vmmp += 32;
		}
		break;
	case 1:
		yo = 0;
		vmmp = 0;
		vtb = 0x008000;
		vtw = virtmeme[vmmp++];
		for(y = height; y > 0; y--) {
			for(x = 0; x < width; x++) {
				(surface)[x+yo] = ((vtw) & vtb) ? 0x00FFFFFF : 0 ;
				if(!(vtb >>= 1)) {vtb = 0x008000; vtw = virtmeme[vmmp++];}
			}
			yo += pitch;
		}
		break;
	case 2:
		yo = 0;
		for(y = 96+12; y > 0; y--) {
			for(x = 0; x < 128+12; x++) {
				(surface)[x+yo] = 0x000000FF ;
			}
			yo += pitch;
		}
		vmmp = 0;
		vtw = NyaLogo[vmmp++];
		vtb = 0x80000000;
		for(y = 22+6; y < 22+28+6; y++) {
			yo = pitch*y;
			for(x = (44+6); x < (44+6+16); x++) {
				if( (vtw & vtb) ) {
					(surface)[x+yo] = 0x00FFFF00;
				}
				if(!(vtb >>= 1)) {vtb = 0x80000000; vtw = NyaLogo[vmmp++];}
			}
		}
		vtb = 0x80000000;
		for(y = 55+6; y < 55+3+6; y++) {
			yo = pitch*y;
			for(x = (35+6); x < (35+6+52); x++) {
				if(((vtw) & vtb)) {
					(surface)[x+yo] = 0x00FFFF00;
				}
				if(!(vtb >>= 1)) {vtb = 0x80000000; vtw = NyaLogo[vmmp++];}
			}
		}
		vtb = 0x80000000;
		vta = 0x00008000;
		vtw = NyaLogo[vmmp++];
		yo = pitch*(33+6)    + (6+62);
		yu = pitch*(33+6+11) + (6+62);
		for(x = 0; x < 16; x++) {
			if(((vtw) & vtb)) {
				(surface)[yo] = 0x00FFFF00;
			}
			if(((vtw) & vta)) {
				(surface)[yu] = 0x00FFFF00;
			}
			yo++; yu++;
			vtb >>= 1; vta >>= 1;
		}
		break;
	}
	return 0;
}

int UpdateDisplay()
{
	HRESULT hr;
	DDSURFACEDESC2 SDSC;
	int i;
	for(i = 0; i < objcount; i++) {
		if((!objlist[i]) || (!objlist[i]->lpddofs)) continue;
		ZeroMemory(&SDSC, sizeof(DDSURFACEDESC2));
		SDSC.dwSize = sizeof(DDSURFACEDESC2);
		hr = objlist[i]->lpddofs->Lock(NULL, &SDSC,DDLOCK_WAIT | DDLOCK_NOSYSLOCK,0);
		if(hr) {
			return -5;
		}
		switch(objlist[i]->rec) {
		case 0:
			LEMRaster(&MyStdDisp, SDSC.dwHeight, SDSC.dwWidth, (unsigned int *)SDSC.lpSurface, SDSC.lPitch >> 2);
			break;
		case 1:
			imva_raster(&MyIMVA, virtmeme, (unsigned int *)SDSC.lpSurface, (SDSC.lPitch >> 2) - (320));
			break;
		}
		objlist[i]->lpddofs->Unlock(NULL);
	}
	InvalidateRect(hwMain, NULL, FALSE);
	return 0;
}

// Windows stuff
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= 0;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= 0;
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= 0; //(HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_ICIC);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= 0;

	return RegisterClassEx(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;

	hInst = hInstance;

	hwMain = hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_THICKFRAME,
		CW_USEDEFAULT, 0, 440, 380, NULL, NULL, hInstance, NULL);

	if (!hWnd) {
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);
	InvalidateRect(hwMain, NULL, FALSE);

	return TRUE;
}

static int ctrlflag = 0;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent, i;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId) {
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case ID_FILE_CONNECT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG1), hWnd, DlgConnect);
			break;
		case ID_FILE_RECONNECT:
			if(!Reconnect()) {
				LogMessage("Connected\n");
			}
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_KEYDOWN:
		switch(wParam) {
		case 0x08: WriteKey(0x10, 0); break; /* Backspace */
		case VK_SEPARATOR:
		case 0x0D: WriteKey(0x11, 0); break; // Return
		case 0x10: WriteKey(0x90, 0); break; // Shift
		case 0x11: WriteKey(0x91, 0); ctrlflag = 1; break; // Control
		//case 0x20: WriteKey(0x20, 0); break; // Space (ASCII)
		case 0x9: for(i = 0; i < 8; i++) WriteKey('f', 0); break;
		case 0x25: WriteKey(0x82, 0); break; // LEFT
		case 0x26: WriteKey(0x80, 0); break; // UP
		case 0x27: WriteKey(0x83, 0); break; // RIGHT
		case 0x28: WriteKey(0x81, 0); break; // DOWN
		case 0x2d: WriteKey(0x12, 0); break; // Insert
		case 0x2e: WriteKey(0x13, 0); break; // Delete
		default:
			if(ctrlflag && wParam >= 'A' && wParam <= 'Z') {
				WriteKey(wParam, 0);WriteKey(wParam, 1);
			}
			break;
		}
		break;
	case WM_KEYUP:
		switch(wParam) {
		case 0x08: WriteKey(0x10, 1); break; /* Backspace */
		case VK_SEPARATOR:
		case 0x0D: WriteKey(0x11, 1); break; // Return
		case 0x10: WriteKey(0x90, 1); break; // Shift
		case 0x11: WriteKey(0x91, 1); ctrlflag = 0; break; // Control
		//case 0x20: WriteKey(0x20, 0); break; // Space (ASCII)
		case 0x9: for(i = 0; i < 8; i++) WriteKey('f', 1); break;
		case 0x25: WriteKey(0x82, 1); break; // LEFT
		case 0x26: WriteKey(0x80, 1); break; // UP
		case 0x27: WriteKey(0x83, 1); break; // RIGHT
		case 0x28: WriteKey(0x81, 1); break; // DOWN
		case 0x2d: WriteKey(0x12, 1); break; // Insert
		case 0x2e: WriteKey(0x13, 1); break; // Delete
		default:
			if(ctrlflag && wParam >= 'A' && wParam <= 'Z') {
				WriteKey(wParam, 1);
			}
			break;
		}
		break;
	case WM_CHAR:
		if(wParam >= 0x0020 && wParam <= 0x007f) {
			WriteKey(wParam, 0); WriteKey(wParam, 1);
		} else {
		}
		break;
	case WM_LBUTTONDOWN:
		{
			int32_t x,y;
			x = (uint16_t)(lParam);
			y = (uint16_t)(lParam >> 16);
			dragp.x = x;
			dragp.y = y;
			RECT rct;
			for(i = objcount; i-->0;) {
				if(!objlist[i]) continue;
				CopyRect(&rct, &objlist[i]->rect);
				OffsetRect(&rct,objlist[i]->scrn.x,objlist[i]->scrn.y);
				if(x >= rct.left && x < rct.right && y >= rct.top && y < rct.bottom) {
					LogMessage("Mouse ObjClick: %d, %d [%d]\n", x, y, i);
					mpoint.x = x - dragp.x;
					mpoint.y = y - dragp.y;
					dragon = i;
					break;
				}
			}
		}
		break;
	case WM_LBUTTONUP:
		{
			int32_t x,y;
			x = (uint16_t)(lParam);
			y = (uint16_t)(lParam >> 16);
			if(dragon > -1 && objlist[dragon]) {
				mpoint.x = x - dragp.x;
				mpoint.y = y - dragp.y;
				POINT sc;
				sc.x = objlist[dragon]->scrn.x;
				sc.y = objlist[dragon]->scrn.y;
				if((sc.x + mpoint.x) < 0) mpoint.x = -sc.x;
				if((sc.y + mpoint.y) < 0) mpoint.y = -sc.y;
				objlist[dragon]->scrn.x += mpoint.x;
				objlist[dragon]->scrn.y += mpoint.y;
				InvalidateRect(hwMain, NULL, 1);
			}
			dragon = -1;
		}
		break;
	case WM_MOUSEMOVE:
		if(wParam & MK_LBUTTON) {
			int32_t x,y;
			x = (uint16_t)(lParam);
			y = (uint16_t)(lParam >> 16);
			if(dragon > -1) {
				mpoint.x = x - dragp.x;
				mpoint.y = y - dragp.y;
				POINT sc;
				sc.x = objlist[dragon]->scrn.x;
				sc.y = objlist[dragon]->scrn.y;
				if((sc.x + mpoint.x) < 0) mpoint.x = -sc.x;
				if((sc.y + mpoint.y) < 0) mpoint.y = -sc.y;
				InvalidateRect(hwMain, NULL, 0);
			}
		} else if(dragon > -1) {
			dragon = -1;
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		{ // Added drawing code here
			RECT rct,srt;
			if(!lpddpri) break;
			if(!objlist) break;
			WINDOWINFO wi;
			GetWindowInfo(hwMain, &wi);
			for(i = 0; i < objcount; i++) {
				if(!objlist[i]) continue;
				CopyRect(&rct, &objlist[i]->rect);
				CopyRect(&srt, &objlist[i]->srect);
				//OffsetRect(&rct,wi.rcClient.left,wi.rcClient.top);
				if(dragon == i)
					OffsetRect(&rct,mpoint.x,mpoint.y);
				if(objlist[i] && objlist[i]->lpddofs) {
					OffsetRect(&rct,objlist[i]->scrn.x,objlist[i]->scrn.y);
					lpddback->Blt(&rct,objlist[i]->lpddofs, &srt,DDBLT_WAIT,0);
				}
			}
			SetRect(&rct,0,0,wi.rcClient.right - wi.rcClient.left,wi.rcClient.bottom - wi.rcClient.top);
			lpddpri->Blt(&wi.rcClient,lpddback, &rct,DDBLT_WAIT,0);
		}
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		apprun = FALSE;
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		SetDlgItemTextA(hDlg, IDC_COPYRIGHT, CPRTTXT);
		SetDlgItemTextA(hDlg, IDC_LICENSE, LICTXT);
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

// Message handler for connect box.
INT_PTR CALLBACK DlgConnect(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message) {
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
			if( LOWORD(wParam) == IDOK ) {
				char ipstr[32];
				int port;
				BOOL bport = FALSE;
				GetDlgItemTextA(hDlg, IDC_IPADDRESS1, ipstr, sizeof(ipstr));
				port = GetDlgItemInt(hDlg, IDC_EDIT1, &bport, FALSE);
				if(!bport) {
					port = 58704;
				}
				if(!Connect(ipstr, port)) {
					LogMessage("Connected\n");
				}
			}
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
