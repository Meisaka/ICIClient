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
static LPDIRECTDRAWSURFACE7 lpddofs = NULL;

static HWND hwMain;
static HDC hdcMain;
static int apprun;

static SOCKET lcs;
static bool unet;
static sockaddr_in remokon;

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
	unsigned int cachepal[16];
	unsigned short cachefont[256];
	unsigned short cachedisp[384];
} MyStdDisp;

ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	DlgConnect(HWND, UINT, WPARAM, LPARAM);
int UpdateDisplay(NyaLEM*);

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

int WriteKey(unsigned char code)
{
	timeval tvl;
	fd_set fdsr;
	unsigned char nf[4];
	int crs, i;
	nf[0] = 0x0080;
	nf[1] = 0x00E7;
	nf[2] = code;
	if(unet) {
		FD_ZERO(&fdsr);
		FD_SET(lcs, &fdsr);
		tvl.tv_sec = 0;
		tvl.tv_usec = 0;
		crs = select(3, NULL, &fdsr, NULL, &tvl);
		if(crs) {
			i = send(lcs, (char*)nf, 3, 0);
			if(!i) {
				// close
				closesocket(lcs);
				unet = false;
				apprun = false;
			} else if(i > 0) {
				// data
			} else {
				// error
				i = WSAGetLastError();
				closesocket(lcs);
				unet = false;
				apprun = false;
			}
		}
	}
	return 0;
}

int Reconnect() {
	int crs;
	if(!remokon.sin_addr.S_un.S_addr)
		return -1;
	if(unet) {
		closesocket(lcs);
		lcs = INVALID_SOCKET;
		unet = false;
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
		crs = WSAGetLastError();
		closesocket(lcs);
		lcs = INVALID_SOCKET;
		unet = false;
		return crs;
	} else {
		unet = true;
	}
	return 0;
}

int Connect(const char *addr, int port) {
	int crs;
	if(unet) {
		closesocket(lcs);
		lcs = INVALID_SOCKET;
		unet = false;
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
		crs = WSAGetLastError();
		closesocket(lcs);
		lcs = INVALID_SOCKET;
		unet = false;
		return crs;
	} else {
		unet = true;
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
	int crs, i, k, kmx, kmb;
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
		ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
		ddsd.dwSize = sizeof(DDSURFACEDESC2);
		ddsd.dwHeight = 328;
		ddsd.dwWidth = 420;
		ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
		hr = lpdd7->CreateSurface(&ddsd, &lpddofs, NULL);
		hr = lpdd7->CreateClipper(0, &lpddclp, NULL);
		hr = lpddclp->SetHWnd(0, hwMain);
		hr = lpddpri->SetClipper(lpddclp);
	}

	MyStdDisp.status = 1;
	MyStdDisp.dspmem = 4;
	MyStdDisp.fontmem = 0;
	MyStdDisp.palmem = 0;
	apprun = TRUE;
	unsigned short vmmp = 0;

	for(i = 0; i < 384; i++) MyStdDisp.cachedisp[i] = 0x0000f120;
	unet = false;
	ZeroMemory(&remokon, sizeof(sockaddr_in));
	
	// Main message loop:
	int zm, zl, zil, nlimit;
	zm = 0;
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
					char *vmbase;
					vmbase = (char*)(virtmeme+0x0);
					i = recv(lcs, vmbase, 2, 0);

					if(!i) {
						// close
						closesocket(lcs);
						unet = false;
						LogMessage("Disconnected\n");
					} else if(i > 0) {
						// data
						zil = 0;
						zm = 2;
						if(virtmeme[0x0] == 0xE7AA && zil < 2) {
							if(zm < 10) { 
								//UpdateDisplay(&MyStdDisp);
								i = recv(lcs, vmbase+zm, 10 - zm, 0);
								if(i > 0) {
									zm += i;
								}
							}
							kmb = (virtmeme[0x03]) & 0xffff; // start addr
							kmx = (virtmeme[0x04]) & 0xffff; // Length (words)
							if(kmx > 384) break;
							zl = (kmx) * 2;
							LogMessage("[NET %03d %03d %03d]\n",kmb,kmx,zl);
							while(zl > 0 && i > 0) { // Load from network stack
								i = recv(lcs, vmbase+zm, zl, 0);
								if(i > 0) {
									zm += i; zl -= i;
									LogMessage("<RD %d %d %d>",i,zm,zl);
								}
							}
							if(kmb < 384) {
								for(k = 0; k < kmx && kmb+k < 384; k++) {
									MyStdDisp.cachedisp[kmb+k] = virtmeme[0x05+k];
								}
							} else {
								// Failed validation
								LogMessage("Failed validation\n");
								kmx = kmx;
							}
							virtmeme[0x00] = 0;
							LogMessage("\n");
							if(zm > zl) { // should never happen now
								zm -= zl;
							}
						} else if(virtmeme[0x00] == 0xE7AB && zil < 2) {
							if(zm < 6) { 
								//UpdateDisplay(&MyStdDisp);
								i = recv(lcs, vmbase+zm, 6 - zm, 0);
								if(i > 0) zm += i;
							}
							MyStdDisp.border = (virtmeme[0x01]);
							//MyStdDisp.status = (virtmeme[0x1002]); // Not yet
							zm -= 6;
						} else if(virtmeme[0x00] == 0xE7AC && zil < 2) {
							if(zm < 4) { 
								i = recv(lcs, vmbase+zm, 4 - zm, 0);
								if(i > 0) zm += i;
							}
							kmx = (virtmeme[0x01]) & 0x01ff;
							zl = 32;
							while(zm < zl && i > 0) { // Load from network stack
								i = recv(lcs, vmbase+zm, zl, 0);
								if(i > 0) { zm += i; zl -= i; }
							}
							for(k = 0; k < 16; k++) {
								MyStdDisp.cachepal[k] = virtmeme[0x02+k];
							}
							virtmeme[0x00] = 0;
						} else if(virtmeme[0x00] == 0xE7AF && zil < 2) {
							if(zm < 6) { 
								i = recv(lcs, vmbase+zm, 6 - zm, 0);
								if(i > 0) zm += i;
							}
							kmb = (virtmeme[0x01]) & 0x00ff;
							kmx = (virtmeme[0x02]) & 0x00ff;
							zl = (kmx) * 2;
							while(zl > 0 && i > 0) { // Load from network stack
								i = recv(lcs, vmbase+zm, zl, 0);
								if(i > 0) { zm += i; zl -= i; }
							}
							if(kmx <= 256 && kmb+kmx <= 256) {
								for(k = 0; k < kmx; k++) {
									MyStdDisp.cachefont[kmb+k] = virtmeme[0x03+k];
								}
							}
							virtmeme[0x00] = 0;
						}
					} else {
						// error
						i = WSAGetLastError();
						closesocket(lcs);
						unet = false;
						LogMessage("Disconnected (Error:%d)\n", i);
					}
				}
			} while(crs && ++nlimit < 15);
		}
		UpdateDisplay(&MyStdDisp);
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
	if(lpddofs) lpddofs->Release();
	if(lpddclp) lpddclp->Release();
	if(lpdd7) lpdd7->Release();
	WSACleanup();
	return (int) msg.wParam;
}

int UpdateDisplay(NyaLEM* StdDisp)
{
	HRESULT hr;
	int dn;
	int x,y,sl,yo,yu;
	int pco;
	unsigned int vtw, vtb, vta;
	unsigned int cla, clb, clf, ctk;
	int wsx, wsy, wsp, wso;
	unsigned short vmmp;
	unsigned short* lclfont;
	
	DDSURFACEDESC2 SDSC;
	dn = 0;
	if(StdDisp->status & 8) {
		ctk = GetTickCount();
		if(ctk < StdDisp->TTI) return 1;
		StdDisp->status = 5;
	}
	if(StdDisp->status == 0x0010) {
		return 1;
	}
	ZeroMemory(&SDSC, sizeof(DDSURFACEDESC2));
	SDSC.dwSize = sizeof(DDSURFACEDESC2);
	hr = lpddofs->Lock(NULL, &SDSC,DDLOCK_WAIT | DDLOCK_NOSYSLOCK,0);
	if(StdDisp->fontmem) {
		lclfont = StdDisp->cachefont;
	} else {
		lclfont = deffont;
	}
	if(StdDisp->palmem) {
		//lclfont = StdDisp->cachefont;
	}
	if(hr) {
		return -5;
	}
	pco = SDSC.lPitch >> 2;
	ctk = GetTickCount();
	if(StdDisp->status == 1) {
		StdDisp->status = 9;
		dn = 2;
		StdDisp->TTI = ctk + 2500;
	}
	if((StdDisp->status & 0x0011) == 0) {
		dn = 4;
		StdDisp->status = 0x0010;
	}
	switch(dn) {
	case 0:
		yo = 0;
		for(y = SDSC.dwHeight; y > 0; y--) {
			for(x = 0; x < SDSC.dwWidth; x++) {
				((int*)SDSC.lpSurface)[x+yo] = 0x000000FF ;
			}
			yo += pco;
		}

		yo = 0;
		//vmmp = StdDisp->dspmem;
		vmmp = 0;
		if(ctk > StdDisp->TTFlip || StdDisp->TTFlip > ctk + 10000) {
			StdDisp->status ^= 2;
			StdDisp->TTFlip = ctk + 800;
		}
		vtb = 0x008000;
		for(sl = 0; sl < 12; sl++) {
			yo = (pco * 7) + (pco * 8 * sl)+ (pco * 6);
			yo *= 3;
			vtb = 0x008000;
			vta = 0x0080;
			for(y = 8; y > 0; y--) {
				wso = 6*3;
				for(x = 0; x < 32; x++) {
					//vtw = virtmeme[vmmp + x];
					vtw = StdDisp->cachedisp[vmmp + x];
					clb = StdDisp->cachepal[(vtw >> 8) & 0x0f];
					clf = StdDisp->cachepal[(vtw >> 12) & 0x0f];
					if((vtw & 0x0080) && (StdDisp->status & 2))
						cla = clb;
					else
						cla = clf;
					vtw &= 0x0000007f;
					vtw <<= 1;
					wsp = yo;
					for(wsy = 0; wsy < 3;wsy++) {
						for(wsx = wso+3; wsx > wso; wsx--) {
							((int*)SDSC.lpSurface)[wsx+wsp+0] = ((lclfont[vtw]) & vtb) ? cla : clb ;
							((int*)SDSC.lpSurface)[wsx+wsp+3] = ((lclfont[vtw]) & vta) ? cla : clb ;
							((int*)SDSC.lpSurface)[wsx+wsp+6] = ((lclfont[vtw+1]) & vtb) ? cla : clb ;
							((int*)SDSC.lpSurface)[wsx+wsp+9] = ((lclfont[vtw+1]) & vta) ? cla : clb ;
						}
						wsp += pco;
					}
					wso += 4*3;
				}
				vtb >>= 1;
				vta >>= 1;
				yo -= pco*3;
			}
			vmmp += 32;
		}
		break;
	case 1:
		yo = 0;
		vmmp = 0;
		vtb = 0x008000;
		vtw = virtmeme[vmmp++];
		for(y = SDSC.dwHeight; y > 0; y--) {
			for(x = 0; x < SDSC.dwWidth; x++) {
				((int*)SDSC.lpSurface)[x+yo] = ((vtw) & vtb) ? 0x00FFFFFF : 0 ;
				if(!(vtb >>= 1)) {vtb = 0x008000; vtw = virtmeme[vmmp++];}
			}
			yo += pco;
		}
		break;
	case 2:
		yo = 0;
		for(y = SDSC.dwHeight; y > 0; y--) {
			for(x = 0; x < SDSC.dwWidth; x++) {
				((int*)SDSC.lpSurface)[x+yo] = 0x000000FF ;
			}
			yo += pco;
		}
		vmmp = 0;
		vtw = NyaLogo[vmmp++];
		vtb = 0x80000000;
		for(y = 26+6; y < 54+6; y++) {
			yo = pco*y*3;
			wso = (6+45) * 3;
			for(x = 45; x < 61; x++) {
				if( (vtw & vtb) ) {
					wsp = yo; for(wsy = 0; wsy < 2;wsy++) {
						for(wsx = wso+2; wsx > wso; wsx--) {
							((int*)SDSC.lpSurface)[wsx+wsp] = 0x00FFFF00;
						}
						wsp += pco;
					}
				}
				wso += 3;
				if(!(vtb >>= 1)) {vtb = 0x80000000; vtw = NyaLogo[vmmp++];}
			}
		}
		vtb = 0x80000000;
			for(y = 60+6; y < 63+6; y++) {
			yo = pco*y*3;
			wso = (6+41) * 3;
			for(x = 41; x < 93; x++) {
				if(((vtw) & vtb)) {
					wsp = yo; for(wsy = 0; wsy < 2;wsy++) {
						for(wsx = wso+2; wsx > wso; wsx--) {
				((int*)SDSC.lpSurface)[wsx+wsp] = 0x00FFFF00;
						} wsp += pco; }
				}
				wso += 3;
				if(!(vtb >>= 1)) {vtb = 0x80000000; vtw = NyaLogo[vmmp++];}
			}
		}
		vtb = 0x80000000;
		vta = 0x00008000;
		vtw = NyaLogo[vmmp++];
		yo = pco*43*3;
		yu = pco*54*3;
		wso = (6+68) * 3;
		for(x = 0; x < 16; x++) {
			if(((vtw) & vtb)) {
				wsp = yo;
				for(wsy = 0; wsy < 2;wsy++) {
					for(wsx = wso+2; wsx > wso; wsx--) {
						((int*)SDSC.lpSurface)[wsx+wsp] = 0x00FFFF00;
					}
					wsp += pco;
				}
			}
			if(((vtw) & vta)) {
				wsp = yu;
				for(wsy = 0; wsy < 2;wsy++) {
					for(wsx = wso+2; wsx > wso; wsx--) {
						((int*)SDSC.lpSurface)[wsx+wsp] = 0x00FFFF00;
					}
					wsp += pco;
				}
			}
			wso += 3;
			vtb >>= 1; vta >>= 1;
			//if(!(vtb >>= 1)) {vtb = 0x80000000; vtw = NyaLogo[vmmp++];}
		}

		for(x = 0; x < 16; x++) {
			vtb = defpal[x];
			vta = ((vtb & 0x000f) | ((vtb & 0x000f) << 4));
			vta |= (((vtb & 0x000f0) << 4) | ((vtb & 0x000f0) << 8));
			vta |= (((vtb & 0x000f00) << 8) | ((vtb & 0x000f00) << 12));
			StdDisp->cachepal[x] = vta;
		}
		break;
	}
	lpddofs->Unlock(NULL);
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

	hwMain = hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,
		CW_USEDEFAULT, 0, 440, 380, NULL, NULL, hInstance, NULL);

	if (!hWnd) {
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);
	InvalidateRect(hwMain, NULL, FALSE);

	return TRUE;
}

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
		case 0x08: // Backspace
			WriteKey(0x10);
			break;
		case VK_SEPARATOR:
		case 0x0D: // Return
			WriteKey(0x11);
			break;
		case 0x10: // Shift
			WriteKey(0x90);
			break;
		case 0x11: // Control
			WriteKey(0x91);
			break;
		//case 0x20: // Space (ASCII)
			//WriteKey(0x20);
			//break;
		case 0x9:
			for(i = 0; i < 8; i++)
			WriteKey('f');
			break;
		case 0x25: // LEFT
			WriteKey(0x82);
			break;
		case 0x26: // UP
			WriteKey(0x80);
			break;
		case 0x27: // RIGHT
			WriteKey(0x83);
			break;
		case 0x28: // DOWN
			WriteKey(0x81);
			break;
		case 0x2d: // Insert
			WriteKey(0x12);
			break;
		case 0x2e: // Delete
			WriteKey(0x13);
			break;
		default:
			break;
		}
		break;
	case WM_CHAR:
		if(wParam >= 0x0020 && wParam <= 0x007f) {
		WriteKey(wParam);
		} else {
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		{ // Added drawing code here
			RECT ri,rct,srt;
			SetRect(&srt, 0, 0, 140*3, 108*3);
			SetRect(&rct,0,0,140*3,108*3);
			OffsetRect(&rct, 0, 0);
			if(lpddpri) {
				GetWindowRect(hwMain, &ri);
				OffsetRect(&rct,ri.left+8,ri.top+50);
				lpddpri->Blt(&rct,lpddofs, &srt,DDBLT_ASYNC,0);
			}
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
