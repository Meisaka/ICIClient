#include "ici.h"

static unsigned short deffont[] = {
0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,
0x242e,0x2400,0x082A,0x0800,0x0008,0x0000,0x0808,0x0808,
0x00ff,0x0000,0x00f8,0x0808,0x08f8,0x0000,0x080f,0x0000,
0x000f,0x0808,0x00ff,0x0808,0x08f8,0x0808,0x08ff,0x0000,
0x080f,0x0808,0x08ff,0x0808,0x6633,0x99cc,0x9933,0x66cc,
0xfef8,0xe080,0x7f1f,0x0701,0x0107,0x1f7f,0x80e0,0xf8fe,
0x5500,0xAA00,0x55AA,0x55AA,0xffAA,0xff55,0x0f0f,0x0f0f,
0xf0f0,0xf0f0,0x0000,0xffff,0xffff,0x0000,0xffff,0xffff,
0x0000,0x0000,0x005f,0x0000,0x0300,0x0300,0x3e14,0x3e00,
0x266b,0x3200,0x611c,0x4300,0x3629,0x7650,0x0002,0x0100,
0x1c22,0x4100,0x4122,0x1c00,0x1408,0x1400,0x081C,0x0800,
0x4020,0x0000,0x0808,0x0800,0x0040,0x0000,0x601c,0x0300,
0x3e49,0x3e00,0x427f,0x4000,0x6259,0x4600,0x2249,0x3600,
0x0f08,0x7f00,0x2745,0x3900,0x3e49,0x3200,0x6119,0x0700,
0x3649,0x3600,0x2649,0x3e00,0x0024,0x0000,0x4024,0x0000,
0x0814,0x2241,0x1414,0x1400,0x4122,0x1408,0x0259,0x0600,
0x3e59,0x5e00,0x7e09,0x7e00,0x7f49,0x3600,0x3e41,0x2200,
0x7f41,0x3e00,0x7f49,0x4100,0x7f09,0x0100,0x3e41,0x7a00,
0x7f08,0x7f00,0x417f,0x4100,0x2040,0x3f00,0x7f08,0x7700,
0x7f40,0x4000,0x7f06,0x7f00,0x7f01,0x7e00,0x3e41,0x3e00,
0x7f09,0x0600,0x3e41,0xbe00,0x7f09,0x7600,0x2649,0x3200,
0x017f,0x0100,0x3f40,0x3f00,0x1f60,0x1f00,0x7f30,0x7f00,
0x7708,0x7700,0x0778,0x0700,0x7149,0x4700,0x007f,0x4100,
0x031c,0x6000,0x0041,0x7f00,0x0201,0x0200,0x8080,0x8000,
0x0001,0x0200,0x2454,0x7800,0x7f44,0x3800,0x3844,0x2800,
0x3844,0x7f00,0x3854,0x5800,0x087e,0x0900,0x4854,0x3c00,
0x7f04,0x7800,0x447d,0x4000,0x2040,0x3d00,0x7f10,0x6c00,
0x417f,0x4000,0x7c18,0x7c00,0x7c04,0x7800,0x3844,0x3800,
0x7c14,0x0800,0x0814,0x7c00,0x7c04,0x0800,0x4854,0x2400,
0x043e,0x4400,0x3c40,0x7c00,0x1c60,0x1c00,0x7c30,0x7c00,
0x6c10,0x6c00,0x4c50,0x3c00,0x6454,0x4c00,0x0836,0x4100,
0x0077,0x0000,0x4136,0x0800,0x0201,0x0201,0x0205,0x0200
};
static unsigned short defpal[] = {
0x0000,0x000a,0x00a0,0x00aa,0x0a00,0x0a0a,0x0a50,0x0aaa,
0x0555,0x055f,0x05f5,0x05ff,0x0f55,0x0f5f,0x0ff5,0x0fff
};
static unsigned int NyaLogo[] = {
0x04010401,0x02010201,0x01010101,0x80818081,0xC041C041,
0xA021A021,0x90119011,0x88098809,0x84058405,0x82038203,
0x81018101,0x80808080,0x80408040,0x80208020,0xCA61D1D5,
0xDD353AEE,0x19198992,0x67A4A1DD,0xD4956550,0x3FFC3FFC
};
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
	imva->fgcolor = fg | 0xff000000;
	imva->bgcolor = bg | 0xff000000;
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

/* imva is the device state
 * ram is the entire 64k words
 * rgba is a 320x200 RGBA pixel array (256000 bytes minimum)
 * pitch is num pixels to move to next line
 */
#define IMVA_RD(m,a)  (m[a])
int imva_raster(void *vimva, uint16_t *ram, uint32_t *rgba, uint32_t pitch)
{
	struct imva_nvstate *imva = (struct imva_nvstate *)vimva;
	imva_colors(imva);
	uint32_t bg, fg;
	uint32_t slack = pitch - 320;
	uint16_t raddr, ova, ovo, ove;
	bg = imva->bgcolor;
	fg = imva->fgcolor;
	raddr = imva->base;
	if(!raddr) return 0; /* stand-by mode */

	if(imva->ovmode & 0xf) {
		uint32_t ctk = SDL_GetTicks();
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

int LEMRaster(void *vlem, uint16_t *ram, uint32_t *surface, uint32_t pitch)
{
	NyaLEM *lem = (NyaLEM *)vlem;
	unsigned int x,y,sl,yo,yu;
	unsigned int vtw, vtb, vta;
	unsigned int cla, clb, clf, ctk;
	unsigned short vmmp;
	unsigned short* lclfont;
	unsigned int ccpal[16];
	int dn;
	dn = 0;

	ctk = SDL_GetTicks();
	switch(lem->status) {
	case 0:
		lem->status = 1;
		dn = 2;
		lem->TTI = ctk + 2500;
		break;
	case 1:
		dn = 2;
		if(ctk >= lem->TTI) {
			lem->status = 4;
			if(!lem->dspmem) lem->dspmem = 4;
		}
		break;
	case 4:
	case 6:
		dn = 0;
		break;
	default:
		lem->status = 0;
		dn = 1;
	}
	if(lem->fontmem) {
		lclfont = ram + lem->fontmem;
	} else {
		lclfont = deffont;
	}

	switch(dn) {
	case 0:
		for(x = 0; x < 16; x++) {
			if(lem->palmem) {
				vtb = ram[(lem->palmem + x) & 0xffff];
			} else {
				vtb = defpal[x];
			}
			vta  = (((vtb & 0x00000f) << 16) | ((vtb & 0x00000f) << 20));
			vta |= (((vtb & 0x0000f0) <<  4) | ((vtb & 0x0000f0) <<  8));
			vta |= (((vtb & 0x000f00) >>  4) | ((vtb & 0x000f00) >>  8));
			ccpal[x] = vta | 0xff000000;
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
					vtw = ram[(uint16_t)(vmmp + x)];
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
		vtw = ram[vmmp++];
		for(y = 96+12; y > 0; y--) {
			for(x = 0; x < 128+12; x++) {
				(surface)[x+yo] = ((vtw) & vtb) ? 0xFFFFFFFF : 0xFF000000 ;
				if(!(vtb >>= 1)) {vtb = 0x008000; vtw = ram[vmmp++];}
			}
			yo += pitch;
		}
		break;
	case 2:
		yo = 0;
		for(y = 96+12; y > 0; y--) {
			for(x = 0; x < 128+12; x++) {
				(surface)[x+yo] = 0xAFFF0000 ;
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
					(surface)[x+yo] = 0xFF00FFFF;
				}
				if(!(vtb >>= 1)) {vtb = 0x80000000; vtw = NyaLogo[vmmp++];}
			}
		}
		vtb = 0x80000000;
		for(y = 55+6; y < 55+3+6; y++) {
			yo = pitch*y;
			for(x = (35+6); x < (35+6+52); x++) {
				if(((vtw) & vtb)) {
					(surface)[x+yo] = 0xFF00FFFF;
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
				(surface)[yo] = 0xFF00FFFF;
			}
			if(((vtw) & vta)) {
				(surface)[yu] = 0xFF00FFFF;
			}
			yo++; yu++;
			vtb >>= 1; vta >>= 1;
		}
		break;
	}
	return 0;
}
