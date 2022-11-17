//
// ICIClient
//
static const char * CPRTTXT =
"Copyright (c) 2013-2022 Meisaka Yukara"
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
#include "isiCPU/netmsg.h"
#include "isiCPU/isidefs.h"

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
#else
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netdb.h>
#endif
#ifdef __APPLE__
#include <OpenGL/gl.h>
#endif
#include "glapi/glad.h"

#undef ZeroMemory
#include <malloc.h>
#include <memory.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

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
static uint32_t updates = 0;
static int lookups = 0;
extern bool uiconsole_open;
bool ui_vkeyboard = false;
bool ui_showdev = false;
int net_listobj = 0;
int net_listclass = 0;
int net_listheir = 0;

class devclass;

clsobj::~clsobj() {
	if(rvstate) free(rvstate);
	if(svlstate) free(svlstate);
}
void clsobj::reset()
{
	this->id = 0;
	this->cid = 0;
	this->pid = 0;
	this->kid = 0;
	this->hasleaf = false;
}

enum EParameter : int {
	PARAM_INT = 0,
	PARAM_STR = 1,
	PARAM_ID = 2,
	PARAM_LID = 3,
	PARAM_TYPE_BOOL = 4,
	PARAM_BOOL = 0x10004,
	PARAM_OPTIONAL = 0x10000,
};

void devclass::AddParameter(int code, const std::string& name, int type)
{
	auto r = make_ic_ptr<devparameter>();
	r->name = name;
	r->type = type;
	if((type & 0xffff) == PARAM_LID) { r->buf.resize(16); }
	r->code = code;
	instparam.push_back(std::move(r));
}
int toPO2(int x) {
	int i = 1;
	while(i < x) {
		i <<=1;
	}
	return i;
}
void devclass::AddDisplayArea(int w, int h)
{
	rendw = w; rendh = h;
	sbw = toPO2(w);
	sbh = toPO2(h);
	hasui = true;
}

static ic_list<devclass> clslist;
static ic_list<clsobj> objlist;
static clsobj *attachpoint = NULL;

static const ImGuiWindowFlags icidefwin =
	ImGuiWindowFlags_NoResize
	| ImGuiWindowFlags_NoSavedSettings
	| ImGuiWindowFlags_ShowBorders;

static const ImGuiWindowFlags iciszwin =
	ImGuiWindowFlags_NoSavedSettings
	| ImGuiWindowFlags_ShowBorders;

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
	net_listclass = net_listobj = net_listheir = 0;
	for(k = 0; k < objlist.size(); k++) {
		clsobj *kobj = objlist[k].get();
		if(kobj) {
			kobj->reset();
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
		i = select(SELECT_NFD(lcs), NULL, &fdsr, NULL, &tvl);
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

int server_write_msg(clsobj * nobj, uint16_t *msg, int len)
{
	if(!nobj || !nobj->id) return -1;
	if(len > 100) len = 100;
	*(uint32_t*)(msgbuff) = 0x08000004 + (len * 2);
	*(uint32_t*)(msgbuff+4) = nobj->id;
	memcpy(msgbuff+8, msg, len * 2);
	return NetworkMessageOut();
}

int server_write_key(unsigned char code, int state)
{
	if(!keybid) return -1;
	*(uint32_t*)(msgbuff) = 0x08000008;
	*(uint32_t*)(msgbuff+4) = keybid;
	*(uint16_t*)(msgbuff+8) = 0x20E7 + state;
	*(uint16_t*)(msgbuff+10) = code;
	return NetworkMessageOut();
}

int server_reset_cpu(uint32_t dcpuid)
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

int server_stop_cpu(uint32_t dcpuid)
{
	if(!unet) return 0;
	uint32_t *nf = (uint32_t*)msgbuff;
	if(dcpuid) {
		nf[0] = 0x02500004;
		nf[1] = dcpuid;
		NetworkMessageOut();
	}
	return 0;
}

int NetControlUpdate() {
	if(!unet) return 0;
	uint32_t *nf = (uint32_t*)msgbuff;
	uint16_t *u16 = ((uint16_t*)msgbuff) + 2;
	switch(lookups) {
	case 0:
		nf[0] = 0x00200016; // hello
		u16[0] = 2;
		u16[1] = 0;
		nf++;
		nf[1] = 0xeeeeeeee;
		nf[2] = 0x12345678;
		nf[3] = 0x9abcdef0;
		nf[4] = 0;
		nf[5] = 0;
		lookups++;
		NetworkMessageOut();
		break;
	case 2:
		lookups++;
		break;
	case 3:
		if(net_listheir > 1) {
			lookups++;
			nf[0] = 0x01200000; // Syncall
			NetworkMessageOut();
		}
		break;
	}
	if(lookups < 2) {
		// must have seen hello
	} else if(net_listclass < 2) {
		if(!net_listclass) {
			nf[0] = 0x01300000; // List classes
			NetworkMessageOut();
			net_listclass = 1;
		}
	} else if(net_listobj < 2) {
		if(!net_listobj) {
			nf[0] = 0x01100000; // List obj
			NetworkMessageOut();
			net_listobj = 1;
		}
	} else if(net_listheir < 2) {
		if(!net_listheir) {
			nf[0] = 0x01400000; // List Heir
			NetworkMessageOut();
			net_listheir = 1;
		}
	}
	return 0;
}

devclass * Class_Register(uint32_t cid, const char *cname, const char *desc){
	devclass *ncls = nullptr;
	for(int i = 0; i < clslist.size(); i++) {
		devclass *fcls = clslist[i].get();
		if(fcls && (fcls->name == cname)) {
			ncls = fcls;
			break;
		}
	}
	if(!ncls) {
		auto ncp = make_ic_ptr<devclass>();
		if(!ncp) return nullptr;
		ncls = ncp.get();
		ncls->name = cname;
		clslist.push_back(std::move(ncp));
	}
	if(desc) {
		ncls->desc = desc;
	}
	if(cid) ncls->cid = cid;
	return ncls;
}

devclass * GetClassName(const std::string &name) {
	for(auto &p : clslist) {
		devclass *fcls = p.get();
		if(fcls && (fcls->name == name)) {
			return fcls;
		}
	}
	return nullptr;
}
devclass * GetClassID(uint32_t cid) {
	for(auto &p : clslist) {
		devclass *fcls = p.get();
		if(fcls && fcls->cid == cid) {
			return fcls;
		}
	}
	return nullptr;
}
clsobj * Object_Create(uint32_t cid)
{
	if(!cid) return NULL;
	devclass *ncls = GetClassID(cid);
	ic_ptr<clsobj> nobj;
	if(ncls->proto) {
		nobj = ncls->proto->make();
	} else {
		nobj = make_ic_ptr<clsobj>();
	}
	if(!nobj) return NULL;
	if(ncls) {
		nobj->iclazz = ncls;
		nobj->cid = ncls->cid;
		if(ncls->rvsize) {
			nobj->rvstate = malloc((nobj->rvsize = ncls->rvsize));
		}
		if(nobj->rvstate) iciZero(nobj->rvstate, nobj->rvsize);
		if(ncls->svsize) {
			nobj->svlstate = malloc((nobj->svsize = ncls->svsize));
		}
		if(nobj->svlstate) iciZero(nobj->svlstate, nobj->svsize);
		if(ncls->hasui)
			nobj->win = true;
		if(ncls->rendw && ncls->rendh) {
			nobj->win = true;
			ICIC_CreateHWTexture(&nobj->uitex, ncls->sbw, ncls->sbh);
			nobj->pixbuf = ic_array_ptr<uint32_t>(new uint32_t[ncls->sbw * ncls->sbh]);
			nobj->uvfar = ImVec2(ncls->rendw / (float)ncls->sbw, ncls->rendh / (float)ncls->sbh);
			uint32_t *clsr = nobj->pixbuf.get();
			for(int c = ncls->sbw * ncls->sbh; c--; *(clsr++) = 0x55000000);
		}
	} else {
		nobj->cid = cid;
	}
	auto nptr = nobj.get();
	objlist.push_back(std::move(nobj));
	return nptr;
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
	iciZero(&remokon, sizeof(remokon));
	iciZero(&hint, sizeof(addrinfo));
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

/** Attach B to A */
void server_object_attach(clsobj *a, clsobj *b)
{
	if(!unet) return;
	if(!a || !b) return;
	uint32_t *nf = (uint32_t*)msgbuff;
	nf[0] = 0x02200008;
	nf[1] = a->id;
	nf[2] = b->id;
	NetworkMessageOut();
	return;
}
/** Attach B to A */
void server_object_attach(clsobj *a, clsobj *b, int ap, int bp)
{
	if(!unet) return;
	if(!a || !b) return;
	uint32_t *nf = (uint32_t*)msgbuff;
	nf[0] = 0x02600010;
	nf[1] = a->id;
	nf[2] = b->id;
	nf[3] = (uint32_t)ap;
	nf[4] = (uint32_t)bp;
	NetworkMessageOut();
	return;
}
/** Attach B to A */
void server_object_deattach(clsobj *a, int32_t idx)
{
	if(!unet) return;
	if(!a) return;
	uint32_t *nf = (uint32_t*)msgbuff;
	nf[0] = 0x02300008;
	nf[1] = a->id;
	nf[2] = (uint32_t)idx;
	NetworkMessageOut();
	return;
}

int isi_text_dec(const char *text, size_t len, size_t limit, void *vv, size_t olen)
{
	int cs;
	int32_t ce;
	size_t i;
	unsigned char *tid = (unsigned char *)vv;
	size_t s;
	s = 0;
	ce = 0;
	if(len > limit) len = limit;
	for(i = 0; i < len; i++) {
		cs = text[i];
		if(cs >= '0') {
			if(cs <= '9') ce = (ce<<6) | (52+cs-'0');
			else if(cs >= 'A') {
				if(cs <= 'Z') ce = (ce<<6) | (cs - 'A');
				else if(cs >= 'a' && cs <= 'z') ce = (ce<<6) | (26+cs-'a');
				else if(cs == '_') ce = (ce<<6) | 63;
				else return -1;
			} else return -1;
		} else if(cs == '-') ce = (ce<<6) | 62;
		else return -1;
		if(i+1 == len) {
			while((i&3) != 3) {
				ce <<= 6; i++;
			}
		}
		if((i & 3) == 3) {
			if(s < olen) tid[s] = ((ce >> 16) & 0xff);
			if(s+1 < olen) tid[s+1] = ((ce >> 8) & 0xff);
			if(s+2 < olen) tid[s+2] = ((ce) & 0xff);
			s += 3;
			ce = 0;
		}
	}
	return 0;
}

void server_object_load(devclass * ncls, const char *luid)
{
	if(!unet) return;
	if(!ncls) return;
	if(!ncls->cid) return;
	uint32_t *nf = (uint32_t*)msgbuff;
	nf[1] = 0;
	nf[2] = ncls->cid;
	//case PARAM_LID:
	nf[3] = nf[4] = 0;
	isi_text_dec(luid, strlen(luid), 11, nf+3, 8);
	nf[0] = 0x03A00010;
	NetworkMessageOut();
	return;
}

void server_object_create(devclass * ncls)
{
	if(!unet) return;
	if(!ncls) return;
	if(!ncls->cid) return;
	uint32_t *nf = (uint32_t*)msgbuff;
	nf[1] = ncls->cid;
	uint8_t *pwp = (uint8_t*)(nf+2);
	int apl = 0;
	for(int i = 0; i < ncls->instparam.size(); i++) {
		devparameter *dp = ncls->instparam[i].get();
		if(!(dp->type & PARAM_OPTIONAL) || dp->use) {
			pwp[0] = dp->code;
			apl+=2;
			switch(dp->type & 0xffff) {
			case PARAM_INT:
			case PARAM_ID:
				apl += pwp[1] = 4;
				if(pwp[1]) {
					memcpy(pwp+2, &dp->ival, pwp[1]);
				}
				pwp += 2+pwp[1];
				break;
			case PARAM_STR:
				apl += pwp[1] = strlen((const char*)dp->buf.data());
				if(pwp[1]) {
					memcpy(pwp+2, dp->buf.data(), pwp[1]);
				}
				pwp += 2+pwp[1];
				break;
			case PARAM_LID:
				apl += pwp[1] = 8;
				isi_text_dec((const char*)dp->buf.data(), strlen((const char*)dp->buf.data()), 11, &dp->ival, 8);
				memcpy(pwp+2, &dp->ival, 8);
				pwp += 2+pwp[1];
				break;
			case PARAM_TYPE_BOOL:
			default:
				pwp[1] = 0;
				break;
			}
		}
	}
	if(apl > 1300) apl = 1300;
	nf[0] = 0x02000004 + apl;
	NetworkMessageOut();
	return;
}

void process_objects() {
	for(auto ki = objlist.begin(); ki != objlist.end(); ki++) {
		clsobj *kobj = (*ki).get();
		if(kobj && !kobj->id) {
			LogMessage("Invalidated Object %d", kobj);
			ki = objlist.erase(ki);
			continue;
		}
	}
}

void process_classes() {
	for(auto &k : objlist) {
		clsobj *kobj = k.get();
		if(kobj && kobj->iclazz) {
			kobj->cid = kobj->iclazz->cid;
		}
	}
}

void process_heirarchy() {
	for(int k = 0; k < objlist.size(); k++) {
		clsobj *kobj = objlist[k].get();
		if(kobj->pid) {
			bool pfound = false;
			for(int i = 0; i < objlist.size(); i++) {
				clsobj *iobj = objlist[i].get();
				if(iobj && iobj->id == kobj->pid) {
					pfound = iobj->hasleaf = true;
					break;
				}
			}
			if(!pfound) {
				LogMessage("Invalid Parent [%04X] for [%04X]", kobj->pid, kobj->id);
				kobj->pid = 0;
			}
		}
		if(kobj && kobj->mid && kobj->iclazz && kobj->iclazz->iskeyboard) {
			for(int i = 0; i < objlist.size(); i++) {
				clsobj *iobj = objlist[i].get();
				if(iobj
					&& iobj->iclazz
					&& iobj->iclazz->hasui
					&& iobj->pid
					&& iobj->mid == kobj->mid
					&& iobj->id != kobj->id) {
					iobj->kid = kobj->id;
				}
			}
		}
	}
}

void assign_id(uint32_t clazz, uint32_t id) {
	for(auto &k : objlist) {
		clsobj *kobj = k.get();
		if(kobj && !kobj->id && kobj->cid == clazz) {
			kobj->id = id;
			return;
		}
	}
	clsobj * nobj = Object_Create(clazz);
	if(nobj) nobj->id = id;
}

void assign_heirarchy(uint32_t id, uint32_t up, uint32_t mem) {
	for(auto &k : objlist) {
		clsobj *kobj = k.get();
		if(kobj && kobj->id == id) {
			kobj->pid = up;
			kobj->mid = mem;
			return;
		}
	}
}

int LIDTextEditCallback(ImGuiTextEditCallbackData *data) {
	if(data->EventFlag & ImGuiInputTextFlags_CallbackCharFilter) {
		int c = data->EventChar;
		if(c >= 'A' && c <= 'Z') return 0;
		if(c >= 'a' && c <= 'z') return 0;
		if(c >= '0' && c <= '9') return 0;
		if(c == '-' || c == '_') return 0;
		return 1;
	}
	return 0;
}

void RequestLoadObject(devclass *dcls, const char *lid) {
	server_object_load(dcls, lid);
	LogMessage("Load Object %s", dcls->name.c_str());
}

void RequestNewObject(devclass *dcls) {
	server_object_create(dcls);
	LogMessage("Add new %s", dcls->name.c_str());
	for(int i = 0; i < dcls->instparam.size(); i++) {
		devparameter * dp = dcls->instparam[i].get();
		if(!(dp->type & PARAM_OPTIONAL) || dp->use) {
			LogMessage(" .. with param %d", dp->code);
		}
	}
}

void ParamNewPopup(devclass *dcls)
{
	if(dcls->instparam.empty()) return;
	char title[128];
	for(int i = 0; i < dcls->instparam.size(); i++) {
		devparameter * dp = dcls->instparam[i].get();
		if(dp->type & PARAM_OPTIONAL) {
			snprintf(title, 128, "##O%X", i);
			ImGui::Checkbox(title, &dp->use);
		}
		if(!(dp->type & PARAM_OPTIONAL) || dp->use) {
			if(dp->type & PARAM_OPTIONAL) ImGui::SameLine();
			ImGui::PushItemWidth(150);
			switch(dp->type & 0xffff) {
			case PARAM_INT:
				snprintf(title, 128, "Int###V%X", i);
				ImGui::InputInt(title, (int*)&dp->ival);
				break;
			case PARAM_STR:
				snprintf(title, 128, "String###V%X", i);
				ImGui::TextDisabled("STRING INPUT");
				break;
			case PARAM_ID:
				snprintf(title, 128, "ID###V%X", i);
				ImGui::TextDisabled("ID INPUT");
				break;
			case PARAM_LID:
				snprintf(title, 128, "L-ID###V%X", i);
				ImGui::InputText(title, (char*)&dp->buf[0], dp->buf.size(), ImGuiInputTextFlags_CallbackCharFilter | ImGuiInputTextFlags_CallbackAlways, LIDTextEditCallback, 0);
				break;
			case PARAM_TYPE_BOOL:
				break;
			default:
				ImGui::TextDisabled("Unknown Type %d", dp->type & 0xffff);
				break;
			}
			ImGui::PopItemWidth();
		}
		ImGui::SameLine();
		ImGui::Text("(%d) %s", dp->code, dp->name.c_str());
	}
	if(ImGui::Selectable("Create")) {
		RequestNewObject(dcls);
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
	mh = *(mi);
	mc = (mh >> 20) & 0xfff;
	ml = mh & 0x1fff;
	uint32_t u32_len = (ml >> 2) + ((ml & 3) ? 1 : 0);
	if(((2 + u32_len) > len) || (mi[1 + u32_len] != 0xFF8859EA)) {
		LogMessage("Network Packet Invalid");
	}
	in += 4;
	uint8_t * const in_end = in + ml;
	mi = (uint32_t*)in;
	mw = (uint16_t*)in;
	switch(mc) {
	case ISIM_PING:
		break;
	case ISIM_HELLO:
		LogMessage("IMXP %d.%d, Server %08x", mw[0], mw[1], mi[1]);
		lookups = 2;
		break;
	case ISIM_GOODBYE:
		LogMessage("Fatal Session Error %d", mi[0]);
		break;
	case ISIM_MSGOBJ:
	{
		char bad1d3a[256];
		int w = 0;
		w += snprintf(bad1d3a, 256 - w, "MsgIn [%04x]:", mi[0]);
		ml -= 4;
		ml /= 2;
		if(ml > 20) ml = 20;
		while(ml) {
			w += snprintf(bad1d3a + w, 256 - w, " %04x", *mw);
			ml--; mw++;
		}
		LogMessage(bad1d3a);
		break;
	}
	case ISIM_MSGCHAN:
	{
		char bad1d3a[256];
		int w = 0;
		w += snprintf(bad1d3a, 256 - w, "MsgIn (%d):", mi[0]);
		ml -= 4;
		ml /= 2;
		if(ml > 20) ml = 20;
		while(ml) {
			w += snprintf(bad1d3a + w, 256 - w, " %04x", *mw);
			ml--; mw++;
		}
		LogMessage(bad1d3a);
		break;
	}
	case ISIM_SYNCMEM16:
	{
		uint32_t vsa = 0;
		uint16_t vrl = 0;
		uint8_t *omh = 0;
		for(k = 0; k < objlist.size(); k++) {
			clsobj *kobj = objlist[k].get();
			if(!kobj) continue;
			if(mi[0] == kobj->id && kobj->cid < 0x2100000) {
				omh = (uint8_t*)kobj->rvstate;
				break;
			}
		}
		if(!omh) break;
		while(in < in_end) {
			if(vrl) {
				omh[vsa] = *in;
				vrl--; in++; vsa++;
				vsa &= 0x1ffff;
			} else {
				vsa = in[0] | (in[1] << 8);
				vrl = in[2];
				in += 3;
				if(vsa >= 0x20000) {
					LogMessage("memsync invalid address");
					break;
				}
			}
		}
		break;
	}
	case ISIM_SYNCMEM32:
	{
		uint32_t vsa = 0;
		uint16_t vrl = 0;
		uint8_t *omh = 0;
		for(k = 0; k < objlist.size(); k++) {
			clsobj *kobj = objlist[k].get();
			if(!kobj) continue;
			if(mi[0] == kobj->id && kobj->cid < ISIT_MEMORY_END) {
				omh = (uint8_t*)kobj->rvstate;
				break;
			}
		}
		if(!omh) break;
		while(in < in_end) {
			if(vrl) {
				omh[vsa] = *in;
				vrl--; in++; vsa++;
				vsa &= 0x1ffff;
			} else {
				vsa = in[0] | (in[1] << 8) | (in[2] << 16) | (in[3] << 24);
				vrl = in[4];
				in += 5;
				if(in < in_end && vsa >= 0x20000) {
					LogMessage("memsync invalid address %04x", vsa);
					break;
				}
			}
		}
		if(vrl) {
			LogMessage("memsync excess clipped at %04x:%04x ld:%x", vsa, vrl, in - in_end);
		}
		break;
	}
	case ISIM_SYNCRVS:
		l = (ml - 4);
		for(k = 0; k < objlist.size(); k++) {
			clsobj *kobj = objlist[k].get();
			if(!kobj) continue;
			devclass *ncls = kobj->iclazz;
			if(!ncls) continue;
			if(mi[0] == kobj->id && l <= kobj->rvsize) {
				memcpy(kobj->rvstate, mi+1, l);
				if(kobj->memptr) {
					kobj->update((uint16_t*)kobj->memptr->rvstate);
				}
			}
		}
		break;
	case ISIM_SYNCSVS:
		break;
	case ISIM_SYNCNVSO:
		break;
	case ISIM_R_GETOBJ:
		LogMessage("Obj List");
		for(i = 0; i < (ml / 8); i++) {
			LogMessage("Obj [%08x]: %x", mi[i*2], mi[1+(i*2)]);
			assign_id(mi[1+(i*2)], mi[i*2]);
		}
		if(mc == ISIM_R_GETOBJ && net_listobj < 2) { // TODO multi-flag last frame
			net_listobj = 2;
			process_objects();
		}
		break;
	case ISIM_R_GETCLASSES:
	{
		LogMessage("Obj Classes");
		uint32_t iclass = 0;
		uint32_t iflag = 0;
		char iname[256];
		char idesc[256];
		int rdpoint = 0;
		int rdtype = 0;
		for(i = 0; i < ml; i++) {
			switch(rdtype) {
			case 0:
				iclass |= in[i] << (8*rdpoint);
				if(++rdpoint > 3) { rdpoint = 0; rdtype++; }
				break;
			case 1:
				iflag |= in[i] << (8*rdpoint);
				if(++rdpoint > 3) { rdpoint = 0; rdtype++; }
				break;
			case 2:
				iname[rdpoint] = in[i];
				if(++rdpoint > 254 || !in[i]) {
					iname[rdpoint] = 0;
					rdpoint = 0;
					rdtype++;
				}
				break;
			case 3:
				idesc[rdpoint] = in[i];
				if(++rdpoint > 254 || !in[i]) {
					idesc[rdpoint] = 0;
					rdpoint = 0;
					rdtype = 0;
					Class_Register(iclass, iname, idesc);
					LogMessage("Class [%08x]: F=%x \"%s\" -- %s", iclass, iflag, iname, idesc);
					iclass = 0;
					iflag = 0;
				}
				break;
			}
		}
		if(ISIM_R_GETCLASSES && net_listclass < 2) { // TODO last frame multi-flag
		process_classes();
		net_listclass = 2;
		}
		break;
	}
	case ISIM_R_GETHEIR:
		LogMessage("Obj Heirarchy");
		ml /= sizeof(uint32_t);
		for(i = 0; i < ml; i+=3) {
			assign_heirarchy(mi[i], mi[1+i], mi[2+i]);
			LogMessage("Obj [%08x]: U[%08x] M[%08x]", mi[i], mi[1+i], mi[2+i]);
		}
		if(mc == ISIM_R_GETHEIR) { // TODO last frame multi-flag
			process_heirarchy();
			net_listheir = 2;
		}
		break;
	case ISIM_R_NEWOBJ:
		if(mi[1]) {
			LogMessage("server: Object [%04x] Created with class [%04x]", mi[1], mi[2]);
			assign_id(mi[2], mi[1]);
		} else {
			LogMessage("[error] server: %d - Object Create class [%04x]", mi[0], mi[2]);
		}
		break;
	case ISIM_R_DELOBJ:
		LogMessage("server: Object [%04x] Deleted", mi[0]);
		break;
	case ISIM_R_ATTACH:
		if(mi[0]) LogMessage("[error] server: %d - Object [%04x] Attach", mi[0], mi[1]);
		else {
			LogMessage("server: Object [%04x]A(%d) [%04x]B(%d) Attached", mi[1], mi[3], mi[2], mi[4]);
			net_listheir = 0;
		}
		break;
	case ISIM_R_DEATTACH:
		if(mi[0]) LogMessage("[error] server: %d - Object [%04x] Deattach", mi[0], mi[1]);
		else {
			LogMessage("server: Object [%04x] Deattached", mi[1]);
		}
		break;
	case ISIM_R_START:
		if(ml > 4 && mi[0]) LogMessage("[error] server: Object [%04x] Reset - %d", mi[1], mi[0]);
		else LogMessage("server: Object [%04x] Reset", mi[1]);
		break;
	case ISIM_R_STOP:
		if(ml > 4 && mi[0]) LogMessage("[error] server: Object [%04x] Stop - %d", mi[1], mi[0]);
		else LogMessage("server: Object [%04x] Stopped", mi[1]);
		break;
	case ISIM_R_LOADOBJ: // TODO update fields
		if(!mi[0]) {
			LogMessage("server: Object [%04x] Loaded with class [%04x]", mi[1], mi[2]);
			assign_id(mi[2], mi[1]);
		} else {
			LogMessage("[error] server: %d - Object load class [%04x]", mi[0], mi[2]);
		}
		break;

	default:
		LogMessage("server: unknown message 0x%03x", mc);
		break;
	}
	return 0;
}

void InitICIClasses()
{
	devclass * ncls;

	ncls = Class_Register(0, "txc_gen_keyboard", 0);
	ncls->iskeyboard = true;
	ncls = Class_Register(0, "tcm_gen_keyboard", 0);
	ncls->iskeyboard = true;
	ncls = Class_Register(0, "trk_gen_speaker", 0);
	ncls->AddClass<speakerdev>(); ncls->hasui = true;
	ncls = Class_Register(0, "trk_gen_eprom", 0);
	ncls->AddParameter(1, "Size", PARAM_INT | PARAM_OPTIONAL);
	ncls->AddParameter(2, "Image ID", PARAM_LID | PARAM_OPTIONAL);
	ncls->AddParameter(3, "Endian Flip Image", PARAM_BOOL | PARAM_OPTIONAL);
	ncls = Class_Register(0, "disk", 0);
	ncls->AddParameter(1, "Image ID", PARAM_LID);
	ncls = Class_Register(0, "memory_64kx16", 0);
	ncls->rvsize = sizeof(uint16_t) * 0x10000;
	ncls->hasui = true;
	ncls = Class_Register(0, "txc_nya_lem", 0);
	ncls->AddDisplayArea(140, 108);
	ncls->AddClass<nyalem>();
	ncls = Class_Register(0, "tcm_nya_lem", 0);
	ncls->AddDisplayArea(140, 108);
	ncls->AddClass<nyalem>();
	ncls = Class_Register(0, "trk_mei_imva", 0);
	ncls->AddDisplayArea(320, 200);
	ncls->AddClass<meiimva>();
}

static int afr = 0;
static int speaker_rate1 = 0;
static int speaker_rate2 = 0;
void ICI_AudioGen(void *usr, Uint8 *stream, int len)
{
	static int lse = 0;
	static int lsr = 0;
	int16_t *vse = (int16_t*)stream;
	while(len) {
		float fv = 0.0f;
		float fc;
		if(speaker_rate1) {
			fc = sinf(6.2831853072f * 2.08333333e-5f * lse);
			if(fc < 0.8f) {
				if(fc > -0.8f) fc = 0;
				else fc = -1.0f;
			} else fc = 1.0f;
			fv += 10000.0f * fc;
			lse += speaker_rate1;
		}
		if(speaker_rate2) {
			fc = sinf(6.2831853072f * 2.08333333e-5f * lsr);
			if(fc < 0.8f) {
				if(fc > -0.8f) fc = 0;
				else fc = -1.0f;
			} else fc = 1.0f;
			fv += 7000.0f * fc;
			lsr += speaker_rate2;
		}
		int16_t v = int(fv);
		vse[0] = v;
		vse[1] = v;
		vse+=2;
		len-= 4;
		if(lse >= 48000) lse -= 48000;
		if(lsr >= 48000) lsr -= 48000;
	}
}

int speakerdev::update(uint16_t *ram)
{
	speaker_rvstate *nvs = (speaker_rvstate*)rvstate;
	if(win) {
		speaker_rate1 = nvs->ch_a;
		speaker_rate2 = nvs->ch_b;
	} else {
		speaker_rate1 = 0;
		speaker_rate2 = 0;
	}
	return 0;
}

int ICIMain()
{
	int crs;
	timeval tvl;
	fd_set fdsr;
	SDL_AudioSpec sdla_want, sdla_have;
	SDL_AudioDeviceID sdla_dev;
	memset(&sdla_want, 0, sizeof(SDL_AudioSpec));

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
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_DisplayMode current;
	SDL_GetCurrentDisplayMode(0, &current);
	SDL_Window *window = SDL_CreateWindow("ICI Client", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
	SDL_GLContext glcontext = SDL_GL_CreateContext(window);
	SDL_GL_MakeCurrent(window, glcontext);
	LogMessage("Starting GL...\n");
	if(!gladLoadGLLoader(SDL_GL_GetProcAddress)) {
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
	netbuff = (unsigned char *)malloc(0x2020);
	msgbuff = (unsigned char *)malloc(2048);

	InitICIClasses();
	apprun = 1;
	unet = false;
	iciZero(&remokon, sizeof(sockaddr_in));

	// Main message loop:
	int zm, zl, zil, nlimit;
	uint32_t zh;
	zm = zl = zil = zh = 0;
	bool ui_exit = false;
	bool ui_connect = false;
	bool ui_showconnect = false;
	bool ui_about = false;
	bool ui_test = false;
	int ui_pcenter = 0;
	char serveraddr[256] = "";
	int serverport = 0;
	StartGUIConsole();
	ShowUIConsole();
	InitKeyMap();

	sdla_want.freq = 48000;
	sdla_want.format = AUDIO_S16;
	sdla_want.channels = 2;
	sdla_want.callback = ICI_AudioGen;
	sdla_dev = SDL_OpenAudioDevice(NULL, 0, &sdla_want, &sdla_have, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
	if(sdla_dev < 1) {
		LogMessage("SDL: Open Audio Device fails: %s", SDL_GetError());
	} else {
		afr = sdla_have.samples;
		SDL_PauseAudioDevice(sdla_dev, 0);
	}
	while (apprun) {
		SDL_Event sdlevent;
		int i = SDL_WaitEventTimeout(&sdlevent, 1);
		while(i) {
			ICIC_ProcessEvent(&sdlevent);
			ICIC_Emu_ProcessEvent(&sdlevent);
			if(sdlevent.type == SDL_QUIT) apprun = false;
			i = SDL_PollEvent(&sdlevent);
		}
		ICIC_NewFrame(window);
		if(ImGui::BeginMainMenuBar()) {
			if(ImGui::BeginMenu("File")) {
				if(unet) {
					if(ImGui::MenuItem("Disconnect")) {
						NetworkClose();
					}
				} else {
					if(ImGui::MenuItem("Connect...")) {
						ui_showconnect = ui_connect = true;
						ImGui::SetWindowFocus("Connect");
					}
					if(ImGui::MenuItem("Connect Local...")) {
						ui_connect = false;
						Connect("127.0.0.1", 0);
					}
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
				ImGui::MenuItem("Device List", 0, &ui_showdev);
				ImGui::Separator();
				ShowDevMenu();
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
			ImGui::SetNextWindowPosCenter(ui_pcenter == 2 ? ImGuiSetCond_Appearing : ImGuiSetCond_Always);
			if(ImGui::Begin("About ICIClient##iciabout", &ui_about, icidefwin)) {
				ImGui::Indent();
				ImGui::Text("ICIClient Version 0.11");
				ImGui::Text(CPRTTXT);
				ImGui::SetWindowSize(ImVec2(ImGui::GetItemRectSize().x * 2.0f, 400.0f), ImGuiSetCond_Always);
				ImGui::Unindent();
				ImGui::Separator();
				ImGui::TextWrapped(LICTXT);
				ImGui::Separator();
				if(ui_pcenter < 2) ui_pcenter++;
			}
			ImGui::End();
		}
		if(ui_connect) {
			ImGui::SetNextWindowPosCenter(ImGuiSetCond_Appearing);
			if(ui_showconnect) ImGui::SetNextWindowFocus();
			if(ImGui::Begin("Connect", &ui_connect, icidefwin)) {
				ImGui::PushItemWidth(200);
				if(ui_showconnect) ImGui::SetKeyboardFocusHere();
				if(ImGui::InputText("Server", serveraddr, 256, ImGuiInputTextFlags_EnterReturnsTrue)) {
					if(serverport < 0) serverport = 0;
					if(serverport > 65535) serverport = 65535;
					Connect(serveraddr, serverport);
					ui_connect = false;
				}
				ImGui::PopItemWidth();
				ImGui::InputInt("Port", &serverport, 1, 512);
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
			ui_showconnect = false;
			ImGui::End();
		}
		if(unet) {
			nlimit = 0;
			do {
				FD_ZERO(&fdsr);
				FD_SET(lcs, &fdsr);
				tvl.tv_sec = 0;
				tvl.tv_usec = 0;
				crs = select(SELECT_NFD(lcs), &fdsr, NULL, NULL, &tvl);
				if(crs) {
					if(zm < 4) {
						i = recv(lcs, (char*)(netbuff+zm), 4-zm, 0);
					} else if(zil < zl) {
						i = recv(lcs, (char*)(netbuff+zm+zil), zl-zil, 0);
					} else {
						LogMessage("Framing Error [0x%08x]: %x", *(unsigned int*)netbuff, zil);
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
									LogMessage("Net Keepalive");
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
								NetworkMessageIn(netbuff, zm + zl);
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
		UpdateDevViewer();
		DrawUIConsole();
		// Rendering
		{
			ImGuiIO &io = ImGui::GetIO();
			glDisable(GL_SCISSOR_TEST);
			glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
		}
		glClearColor(0.1f, 0.1f, 0.1f, 0);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui::Render();
		SDL_GL_SwapWindow(window);
	}
	if(sdla_dev) SDL_CloseAudioDevice(sdla_dev);
	objlist.clear();
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
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	WSADATA lwsa;
	WSAStartup(MAKEWORD(2,2), &lwsa);
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

int ShowDevMenu() {
	char wtitle[256];
	if(ImGui::BeginMenu("Show Devices")) {
		for(int i = 0; i < objlist.size(); i++) {
			clsobj *nobj = objlist[i].get();
			if(!nobj) continue;
			if(!nobj->iclazz) continue;
			if(!nobj->iclazz->hasui) continue;
			snprintf(wtitle, 256, "%s %04x###dev%X", nobj->iclazz->name.c_str(), nobj->id, i);
			ImGui::MenuItem(wtitle, 0, &nobj->win);
			if(!nobj->rvstate) continue;
		}
		ImGui::EndMenu();
	}
	return 0;
}

void ShowAttachPop(clsobj *aobj, bool mem) {
	char wtitle[256];
	int i;
	if(!aobj) return;
	if(ImGui::BeginPopup(mem?"MemAttach":"ItemAttach")) {
		for(i = 0; i < objlist.size(); i++) {
			clsobj *nobj = objlist[i].get();
			if(!nobj) continue;
			if(nobj->pid) continue;
			if(!mem && nobj->cid < ISIT_MEMORY_END) continue;
			if(mem && nobj->cid >= ISIT_MEMORY_END) continue;
			if(nobj->id == aobj->id) continue;
			devclass *ncls = nobj->iclazz;
			if(ncls) {
				snprintf(wtitle, 256, "%s [%04x]###IA-%X", nobj->iclazz->name.c_str(), nobj->id, i);
			} else {
				snprintf(wtitle, 256, "Class-%04X [%04x]###IA-%X", nobj->cid, nobj->id, i);
			}
			if(ImGui::Selectable(wtitle)) {
				LogMessage("Attach [%04X] to [%04X]", nobj->id, aobj->id);
				server_object_attach(aobj, nobj);
			}
		}
		ImGui::EndPopup();
	}
}

static char fetchitemtext[256];
bool fetchitems(void * data, int index, const char **out) {
	clsobj *nobj = objlist[index].get();
	if(!nobj) {
		snprintf(fetchitemtext, 256, "<NULL>");
		*out = fetchitemtext;
		return true;
	}
	snprintf(fetchitemtext, 256, "%s [%04x]", nobj->iclazz->name.c_str(), nobj->id);
	*out = fetchitemtext;
	return true;
}

void ShowAttachAtPop(clsobj *aobj) {
	static int deva = 0;
	static int devb = 0;
	static int pointa = 0;
	static int pointb = 0;
	if(!aobj) return;
	if(ImGui::BeginPopup("ItemAttachAt")) {
		ImGui::Combo("Dev A", &deva, &fetchitems, 0, objlist.size(), -1);
		ImGui::SameLine();
		ImGui::InputInt("Point A", &pointa, 1, 100, 0);
		ImGui::Separator();
		ImGui::Combo("Dev B", &devb, &fetchitems, 0, objlist.size(), -1);
		ImGui::SameLine();
		ImGui::InputInt("Point B", &pointb, 1, 100, 0);
		if(ImGui::Selectable("Confirm")) {
			if(deva >= objlist.size() || devb >= objlist.size()) {
				LogMessage("Error: invalid devices selected");
				return;
			}
			clsobj *aobj = objlist[deva].get();
			clsobj *bobj = objlist[devb].get();
			LogMessage("Attach [%04X] to [%04X]", aobj->id, bobj->id);
			server_object_attach(aobj, bobj, pointa, pointb);
		}
		ImGui::EndPopup();
	}
}

void UpdateMemViewer(bool &ui_memview, int memid) {
	if(!ui_memview) return;
	static char wtitle[256];
	snprintf(wtitle, 256, "Memory [%04X]", memid);
	if(!ImGui::Begin(wtitle, &ui_memview, ImVec2(500, 500), -1, iciszwin)) {
		ImGui::End();
		return;
	}
	uint32_t address_lo = 0, address_hi = 0;
	ImGui::Text("Address: 0x%06X - 0x%06X", address_lo, address_hi);
	ImGui::BeginChild("DeviceLayout", ImVec2(0,-ImGui::GetItemsLineHeightWithSpacing()), true, ImGuiWindowFlags_HorizontalScrollbar);
	wtitle[0] = ':';
	wtitle[1] = ' ';
	wtitle[10] = 0;
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(1.f, 0.f));
	const int mem_size = 0x1000;
	ImGuiListClipper clipper(mem_size / 8);
	while (clipper.Step()) {
		uint32_t addr = (uint32_t)clipper.DisplayStart * 8;
		const uint32_t addr_end = (uint32_t)clipper.DisplayEnd * 8;
		while(addr < addr_end) {
			ImGui::Text("%06X: ", addr);
			do {
				ImGui::SameLine();
				ImGui::Text("%02X ", 0);
				wtitle[2 + (addr & 7)] = '0' + (addr & 0x1f);
				addr++;
			} while(addr & 7);
			ImGui::SameLine();
			ImGui::TextUnformatted(wtitle);
		}
	}
	ImGui::PopStyleVar();
	ImGui::EndChild();
	ImGui::End();
	return;
}

void UpdateDevViewer()
{
	char wtitle[256];
	static char loadlid[16];
	if(!ui_showdev) return;
	ImGui::SetNextWindowPosCenter(ImGuiSetCond_Once);
	if(!ImGui::Begin("Devices", &ui_showdev, ImVec2(500, 500), -1, iciszwin)) {
		ImGui::End();
		return;
	}
	int pstack[10];
	int pidstack[10];
	int pscan = 0;
	int pidscan = 0;
	int i;
	ImGui::Text("Devices List");
	ImGui::SameLine();
	if(ImGui::SmallButton("Add New"))
		ImGui::OpenPopup("ItemAdd");
	if(ImGui::BeginPopup("ItemAdd")) {
		if(!unet) {
			ImGui::TextColored(ImColor(255,10,0), "Not Connected!");
		} else {
			for(i = 0; i < clslist.size(); i++) {
				devclass *dcls = clslist[i].get();
				snprintf(wtitle, 256, "%s###CX%X", dcls->desc.empty() ? dcls->name.c_str() : dcls->desc.c_str(), i);
				if(dcls->instparam.size()) {
					if(ImGui::BeginMenu(wtitle)) {
						ParamNewPopup(dcls);
						ImGui::EndMenu();
					}
				} else if(ImGui::Selectable(wtitle)) {
					RequestNewObject(dcls);
				}
			}
		}
		ImGui::EndPopup();
	}
	ImGui::SameLine();
	if(ImGui::SmallButton("Load Dev"))
		ImGui::OpenPopup("ItemLoad");
	if(ImGui::BeginPopup("ItemLoad")) {
		if(!unet) {
			ImGui::TextColored(ImColor(255,10,0), "Not Connected!");
		} else {
			ImGui::InputText("L-ID to Load###LOAD-ID", loadlid, 12, ImGuiInputTextFlags_CallbackCharFilter | ImGuiInputTextFlags_CallbackAlways, LIDTextEditCallback, 0);
			ImGui::Separator();
			for(i = 0; i < clslist.size(); i++) {
				devclass *dcls = clslist[i].get();
				snprintf(wtitle, 256, "%s###LX%X", dcls->desc.empty() ? dcls->name.c_str() : dcls->desc.c_str(), i);
				if(ImGui::Selectable(wtitle)) {
					RequestLoadObject(dcls, loadlid);
				}
			}
		}
		ImGui::EndPopup();
	}
	ImGui::Separator();
	ImGui::BeginChild("DeviceLayout", ImVec2(0,-ImGui::GetItemsLineHeightWithSpacing()), false, ImGuiWindowFlags_HorizontalScrollbar);
	ImGui::Columns(2, "Col List");
	for(i = 0; pscan || i < objlist.size(); i++) {
		if(pscan && !(i < objlist.size())) {
			ImGui::TreePop();
			pscan--;
			i = pstack[pscan];
			if(pscan)
				pidscan = pidstack[pscan];
			else
				pidscan = 0;
			continue;
		}
		clsobj *nobj = objlist[i].get();
		if(!nobj) continue;
		if(pidscan) {
			if(nobj->pid != pidscan) continue;
		} else if(nobj->pid && nobj->pid < nobj->id) {
			continue;
		}
		nobj->rparent = true;
		devclass *ncls = nobj->iclazz;
		if(ncls) {
			snprintf(wtitle, 256, "%s [%04x]", nobj->iclazz->name.c_str(), nobj->id);
		} else {
			snprintf(wtitle, 256, "Class-%04X [%04x]###dev%X", nobj->cid, nobj->id, i);
		}
		bool isopen = ImGui::TreeNode(wtitle);
		ImGui::NextColumn();
		if(nobj->pid) {
			snprintf(wtitle, 256, "Detach###dt%X", nobj->id);
			if(ImGui::SmallButton(wtitle)) {
				server_object_deattach(nobj, -3);
			}
		}
		if(ncls && !ncls->desc.empty()) {
			if(nobj->pid) ImGui::SameLine();
			ImGui::Text("%s", ncls->desc.c_str());
		}
		ImGui::NextColumn();
		if(isopen) {
			ImGui::Text("Class"); ImGui::NextColumn();
			ImGui::Text("%08X", nobj->cid); ImGui::NextColumn();
			if(nobj->cid >= 0x3000000) {
				ImGui::Text("EMEI"); ImGui::NextColumn();
				if(ImGui::SmallButton("Subscribe")) {
					uint16_t cm = 0xffff;
					server_write_msg(nobj, &cm, 1);
				}
				ImGui::NextColumn();
			}
			if(nobj->mid || (nobj->cid >= 0x3000000)) {
				ImGui::Text("Memory ID"); ImGui::NextColumn();
				if(nobj->mid) {
					ImGui::Text("%04X", nobj->mid);
				} else {
					if(ImGui::Button("Attach Mem")) {
						ImGui::OpenPopup("MemAttach");
						attachpoint = nobj;
					}
					ShowAttachPop(nobj, true);
				}
				ImGui::NextColumn();
			}
			if(nobj->kid) {
				ImGui::Text("Keyboard ID"); ImGui::NextColumn();
				ImGui::Text("%04X", nobj->kid); ImGui::NextColumn();
			}
			if(ncls && ncls->rendw) {
				ImGui::Text("Size"); ImGui::NextColumn();
				ImGui::Text("%d x %d", ncls->rendw, ncls->rendh); ImGui::NextColumn();
			}
			if(nobj->rvstate) {
				ImGui::Text("State Storage"); ImGui::NextColumn();
				ImGui::Text("%d bytes", nobj->rvsize); ImGui::NextColumn();
				if(ncls && ncls->hasui) {
					ImGui::Text("Window"); ImGui::NextColumn();
					snprintf(wtitle, 256, "Show###sdev%X", nobj->id);
					ImGui::Checkbox(wtitle, &nobj->win); ImGui::NextColumn();
				}
			}
			if(nobj->cid >= 0x3000000 && nobj->cid < 0x4000000) {
				ImGui::Text("Control"); ImGui::NextColumn();
				if(ImGui::SmallButton("Reset")) {
					server_reset_cpu(nobj->id);
				}
				ImGui::SameLine();
				if(ImGui::SmallButton("Stop")) {
					server_stop_cpu(nobj->id);
				}
				ImGui::NextColumn();
			}
			if(nobj->cid >= 0x3000000) {
				ImGui::Text("Attachment"); ImGui::NextColumn();
				if(ImGui::SmallButton("Attach")) {
					ImGui::OpenPopup("ItemAttach");
					attachpoint = nobj;
				}
				ImGui::SameLine();
				if(ImGui::SmallButton("AttachAt")) {
					ImGui::OpenPopup("ItemAttachAt");
					attachpoint = nobj;
				}
				ShowAttachPop(nobj, false);
				ShowAttachAtPop(nobj);
				ImGui::NextColumn();
			}
			if(nobj->id && nobj->hasleaf) {
				pstack[pscan] = i;
				pidstack[pscan] = pidscan;
				i = -1;
				pscan++;
				pidscan = nobj->id;
				continue;
			}
			ImGui::TreePop();
		}
	}
	ImGui::Columns();
	ImGui::EndChild();

	ImGui::End();
}

int UpdateDisplay() {
	int i, wo;
	char wtitle[256];
	ui_vkeyboard = false;
	wo = 0;
	for(i = 0; i < objlist.size(); i++) {
		clsobj *nobj = objlist[i].get();
		if(!nobj) continue;
		devclass *ncls = nobj->iclazz;
		if(!ncls) continue;
		if(!nobj->rvstate) continue;
		if(nobj->mid && !nobj->memptr) {
			for(int k = 0; k < objlist.size(); k++) {
				if(objlist[k]->id == nobj->mid) {
					nobj->memptr = objlist[k].get();
					break;
				}
			}
			if(!nobj->memptr) {
				LogMessage("Invalid MemID [%04X] on [%04X]", nobj->mid, nobj->id);
				nobj->mid = 0;
			}
		}
		if(nobj->memptr) {
			//snprintf(wtitle, 256, "%s [%04X]###win-%x", ncls->desc ? ncls->desc.get() : ncls->name.get(), nobj->id, i);
			//float cas = 40.f+20.f*(wo&15);
			//wo++;
			nobj->update((uint16_t*)nobj->memptr->rvstate);
			//ImGui::SetNextWindowPos(ImVec2(cas, cas), ImGuiSetCond_Appearing);
			//if(ImGui::Begin(wtitle, &nobj->win, icidefwin | ImGuiWindowFlags_AlwaysAutoResize)) {
			//}
			//ImGui::End();
		}
		if(ncls->hasui && nobj->cid < ISIT_MEMORY_END) {
			UpdateMemViewer(nobj->win, nobj->id);
		} else if(ncls->hasui && nobj->win && nobj->memptr) {
			snprintf(wtitle, 256, "%s [%04X]###win-%x", !ncls->desc.empty() ? ncls->desc.c_str() : ncls->name.c_str(), nobj->id, i);
			float cas = 40.f+20.f*(wo&15);
			wo++;
			ImGui::SetNextWindowPos(ImVec2(cas, cas), ImGuiSetCond_Appearing);
			if(ImGui::Begin(wtitle, &nobj->win, icidefwin | ImGuiWindowFlags_AlwaysAutoResize)) {
				nobj->rasterfn();
				ICIC_UpdateHWTexture(&nobj->uitex, ncls->sbw, ncls->sbh, nobj->pixbuf.get());
				ImGui::Image(&nobj->uitex, ImVec2(ncls->rendw*2, ncls->rendh*2), ImVec2(0,0), nobj->uvfar);
				if(ImGui::IsRootWindowOrAnyChildFocused() && nobj->kid) {
					ui_vkeyboard = true;
					keybid = nobj->kid;
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

void InitKeyMap() { /* DCPU Generic Keyboard keymapping */
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

int KeyTrans(uint32_t sc, bool isdown) {
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

bool ICIC_Emu_ProcessEvent(SDL_Event* event) {
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
				server_write_key(kt, 0);
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
				if(vkeyb_dn[kt]) server_write_key(kt, 1);
				vkeyb_dn[kt] = 0;
			}
		}
		return true;
	}
	if(modflag && (io.WantCaptureKeyboard || !ui_vkeyboard)) {
		for(int i = 0; i < 512; i++) {
			if(vkeyb_dn[i]) {
				server_write_key(i, 1);
				vkeyb_dn[i] = 0;
			}
		}
		keybid = 0;
		modflag = 0;
	}
	return false;
}
