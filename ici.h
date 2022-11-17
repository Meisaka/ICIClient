#ifndef ICI_H_INC
#define ICI_H_INC

#include <stdint.h>
#include <stdlib.h>
#include <SDL.h>

#include "ui/imgui.h"
#include <new>
#include <memory>
#include <vector>
#include <string>

#define iciZero(a,l)  memset((a), 0, (l))

#ifdef WIN32
#define ICI_GET_ERROR WSAGetLastError()
#define SHUT_RDWR SD_BOTH
#define SELECT_NFD(a) 0
#else
#define SOCKET int
#define INVALID_SOCKET -1
#define ICI_GET_ERROR errno
#define closesocket(a) close(a)
#define SELECT_NFD(a) (a)+1
#endif

class clsobj;
class devclass;

int imva_raster(void *vimva, uint16_t *ram, uint32_t *rgba, uint32_t slack);
int LEMRaster(void *vlem, uint16_t *ram, uint32_t *surface, uint32_t pitch);
int speaker_update(clsobj *state, uint16_t *ram);

struct icitexture {
	uint32_t handle;
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
void UpdateDevViewer();
int ShowDevMenu();
int UpdateDisplay();
void InitKeyMap();

#if defined(WIN32) && defined(_MSC_VER) && (_MSC_VER < 1900)
int snprintf(char * buf, size_t len, const char * fmt, ...);
#endif

template<typename T>
using ic_ptr = std::unique_ptr<T>;
template<typename T>
using ic_array_ptr = std::unique_ptr<T[]>;
template<typename T>
using ic_list = std::vector<ic_ptr<T>>;

typedef std::basic_string<uint8_t> ic_buffer;

template<typename T>
ic_ptr<T> make_ic_ptr() { return ic_ptr<T>(new T()); }


class clsobj {
public:
	uint32_t id;
	uint32_t cid;
	uint32_t pid;
	uint32_t mid;
	uint32_t kid;
	bool hasleaf;
	bool win;
	bool rparent;
	std::string name;
	ic_array_ptr<uint32_t> pixbuf;
	size_t rvsize;
	size_t svsize;
	devclass *iclazz;
	clsobj * memptr;
	void * rvstate;
	void * svlstate;
	icitexture uitex;
	ImVec2 uvfar;
public:
	virtual int rasterfn() { return 0; }
	virtual int command() { return 0; }
	virtual int update(uint16_t *ram) { return 0; }
	virtual ~clsobj();
	virtual void reset();
	virtual void init() {}
};

struct devparameter {
	std::string name;
	ic_buffer buf;
	int code;
	int type;
	int len;
	bool use;
	uint64_t ival;
};

class devclass {
public:
	uint32_t cid;
	std::string name;
	std::string desc;
	bool hasui;
	bool iskeyboard;
	bool hasupdates;
	size_t rvsize;
	size_t svsize;
	int sbw;
	int sbh;
	int rendw;
	int rendh;
	ic_list<devparameter> instparam;
	void AddParameter(int code, const std::string &name, int type);
	void AddDisplayArea(int w, int h);
	struct instmaker {
		virtual ic_ptr<clsobj> make() { return nullptr; }
	};
	template<class T>
	struct instmaker_t : public instmaker {
		ic_ptr<clsobj> make() {
			return ic_ptr<clsobj>(new T());
		}
	};
	struct instmaker *proto;

	template<typename T>
	void AddClass() {
		proto = new instmaker_t<T>();
		rvsize = sizeof(T::rvtype);
	}
};

struct nyalem_rv {
	unsigned short dspmem;
	unsigned short fontmem;
	unsigned short palmem;
	unsigned short border;
	unsigned short version;
	uint32_t TTI;
	uint32_t TTFlip;
	int status;
};
class nyalem : public clsobj {
public:
	typedef struct nyalem_rv rvtype;
	int rasterfn();
};

struct imva_rvstate {
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

class meiimva : public clsobj {
public:
	typedef struct imva_rvstate rvtype;
	void reset();
	int rasterfn();
};

struct speaker_rvstate {
	uint16_t ch_a;
	uint16_t ch_b;
};

class speakerdev : public clsobj {
public:
	typedef struct speaker_rvstate rvtype;
	int update(uint16_t *ram);
};

#endif
