#ifndef ICI_H_INC
#define ICI_H_INC

#include <stdint.h>
#include <stdlib.h>
#include <SDL.h>
#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/glew.h>
#endif
#include "ui/imgui.h"
#include <new>

#define iciZero(a,l)  memset((a), 0, (l))

#ifdef WIN32
#define ICI_GET_ERROR WSAGetLastError()
#define SHUT_RDWR SD_BOTH
#else
#define SOCKET int
#define INVALID_SOCKET -1
#define ICI_GET_ERROR errno
#define closesocket(a) close(a)
#endif

class clsobj;
class devclass;

int imva_raster(void *vimva, uint16_t *ram, uint32_t *rgba, uint32_t slack);
int LEMRaster(void *vlem, uint16_t *ram, uint32_t *surface, uint32_t pitch);
int speaker_update(clsobj *state, uint16_t *ram);

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
int UpdateDevViewer();
int ShowDevMenu();
int UpdateDisplay();
void InitKeyMap();

#if defined(WIN32) && defined(_MSC_VER) && (_MSC_VER < 1900)
int snprintf(char * buf, size_t len, const char * fmt, ...);
#endif

template<typename T>
class Ptr {
public:
	T *p;
	Ptr() { p = 0; }
	Ptr(T *n) { p = n; }
	~Ptr() { if(p) free(p); p = 0; }
	static T* make() {
		T* t = (T*)malloc(sizeof(T));
		if(t) iciZero(t, sizeof(T));
		return t;
	}
	static T* make_array(size_t c) {
		T* t = (T*)malloc(sizeof(T) * c);
		if(t) iciZero(t, sizeof(T) * c);
		return t;
	}
	T* get() { return p; }
	T* operator=(T *n) { return (p = n); }
	T* operator=(T &n) { return (p = &n); }
	operator bool() const { return (p!=(T*)0); }
	T& operator *() { return *p; }
	const T& operator *() const { return *p; }
	T& operator[](size_t n) { return p[n]; }
	const T& operator[](size_t n) const { return p[n]; }
	T& operator->() { return *p; }
	const T& operator->() const { return *p; }
	T* operator++() { return ++p; }
	T* operator++(int) { return p++; }
	T* operator+(size_t n) const { return p+n; }
	T* operator+=(size_t n) { return p+=n; }
	T* operator--() { return --p; }
	T* operator--(int) { return p--; }
	T* operator-(size_t n) const { return p-n; }
	T* operator-=(size_t n) { return p-=n; }
};

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
	Ptr<char> name;
	Ptr<uint32_t> pixbuf;
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

template<typename T>
struct ItemFree {
	static void Free(T *m) { free(m); }
};
template<>
struct ItemFree<clsobj> {
	static void Free(clsobj *m) {
		m->~clsobj();
		free(m);
	}
};

template<class T, class K = ItemFree<T> > struct ItemTable {
	T **list;
	int count;
	int limit;
	ItemTable() {
		list = NULL;
		count = limit = 0;
	}
	int AddItem(T *item)
	{
		int i;
		if(!list) {
			i = 64;
			void *mem = malloc(i * sizeof(T*));
			if(!mem) return -1;
			iciZero(mem, i * sizeof(T*));
			list = (T**)mem;
			limit = i;
		} else if(count >= limit) {
			i = count * 2;
			void *mem = malloc(i * sizeof(T*));
			if(!mem) return -1;
			memcpy(mem, list, count * sizeof(T*));
			list = (T**)mem;
			limit = i;
		}
		list[count] = item;
		return count++;
	}
	int AddItem(Ptr<T> &item) { return AddItem(item.get); }
	T* RemoveItem(int idx)
	{
		if(!list || !count || !(idx < count)) return NULL;
		T *item = list[idx];
		count--;
		int i;
		for(i = idx; i < count; i++) {
			list[i] = list[i+1];
		}
		list[i] = NULL;
		return item;
	}
	void DeleteItem(int idx)
	{
		T *item = RemoveItem(idx);
		if(item) K::Free(item);
	}
	void Clear()
	{
		for(int i = 0; i < count; i++) {
			K::Free(list[i]);
		}
	}
};

struct devparameter {
	Ptr<char> name;
	Ptr<char> buf;
	int code;
	int type;
	int len;
	bool use;
	uint64_t ival;
};

class devclass {
public:
	uint32_t cid;
	Ptr<char> name;
	Ptr<char> desc;
	bool hasui;
	bool iskeyboard;
	bool hasupdates;
	size_t rvsize;
	size_t svsize;
	int sbw;
	int sbh;
	int rendw;
	int rendh;
	ItemTable<devparameter> instparam;
	void AddParameter(int code, const char * name, int type);
	void AddDisplayArea(int w, int h);
	struct instmaker {
		virtual clsobj * make() { return nullptr; }
	};
	template<class T>
	struct instmaker_t : public instmaker {
		clsobj * make() {
			clsobj * p = Ptr<T>::make();
			if(!p) return nullptr;
			return new(p) T();
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
