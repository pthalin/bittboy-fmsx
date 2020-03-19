/** fMSX: portable MSX emulator ******************************/
/**                                                         **/
/**                        SDLgfx.c                         **/
/**                                                         **/
/** This file contains SDL library-dependent subroutines    **/
/** and drivers. It includes common drivers from Common.h.  **/
/**                                                         **/
/** Copyright (C) Vincent van Dam 2001-2002                 **/
/**               Marat Fayzullin 1994-2000                 **/
/**               Elan Feingold   1995                      **/
/**               Ville Hallik    1996                      **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/

/** Private #includes ****************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include "MSX.h"
#include "Sound.h"
#include "SDLfnt.h"
#include "SDLfilter.h"
#include "psp_danzeff.h"
#include "psp_kbd.h"
#include "psp_joy.h"
#include "tape.h"

int loaded_msx = 0;

/** Standard #includes ***************************************/
#ifdef UNIX
#include <signal.h>
#endif

/** SDL #includes ********************************************/
#include <SDL.h>
#include "psp_sdl.h"

/** Public parameters ****************************************/
int  UseSound  = 44100;         /* Sound driver frequency    */
char *Disks[2][MAXDISKS+1];     /* Disk names for each drive */

//LUDO:
int psp_screenshot_mode = 0;

int UseFilter = 0;
char *filter_name[6] = 
{ "\n Full scanlines \n\n","\n Half scanlines \n\n",
  "\n Mix scanlines \n\n","\n No scanlines \n\n",
  "\n Half blur \n\n","\n Full blur \n\n" };

#ifdef FULLSCREEN
#define SDL_INIT       SDL_INIT_VIDEO | SDL_INIT_TIMER
#define SDL_VIDEOMODE  SDL_SWSURFACE  | SDL_FULLSCREEN
#else
#define SDL_INIT       SDL_INIT_VIDEO | SDL_INIT_TIMER
#define SDL_VIDEOMODE  SDL_SWSURFACE
#endif

/** Various variables ****************************************/

static unsigned int BPal[256],XPal[80],XPal0; 
static byte JoyState;
# define XKeyMap KeyMap
static int   CurDisk[2];
static pixel *XBuf;

int UseStatic = 0;
int UseZoom   = 0;
int SaveCPU   = 0;

/** Various SDL related variables ****************************/
extern SDL_Surface *back_surface;
extern enum cas_type_t CasType;

/** These functions are called on signals ********************/
#ifdef UNIX
static void OnBreak(int Arg) { ExitNow=1;signal(Arg,OnBreak); }
#endif

void
psp_msx_wait_vsync()
{
# if defined(PSP)
  static int loc_pv = 0;
  int cv = sceDisplayGetVcount();
  if (loc_pv == cv) {
    sceDisplayWaitVblankCB();
  }
  loc_pv = sceDisplayGetVcount();
# endif
}

void
msx_synchronize(void)
{
  static u32 nextclock = 1;
  static u32 next_sec_clock = 0;
  static u32 cur_num_frame = 0;

  u32 curclock = SDL_GetTicks();

  if (MSX.msx_speed_limiter) {
    while (curclock < nextclock) {
     curclock = SDL_GetTicks();
    }
    u32 f_period = 1000 / MSX.msx_speed_limiter;
    nextclock += f_period;
    if (nextclock < curclock) nextclock = curclock + f_period;
  }

  if (MSX.msx_view_fps) {
    cur_num_frame++;
    if (curclock > next_sec_clock) {
      next_sec_clock = curclock + 1000;
      MSX.msx_current_fps = cur_num_frame;
      cur_num_frame = 0;
    }
  }
}

int direct_mode = 0; 
void
msx_set_direct_surface()
{
  XBuf = (ushort*)( (u8*)back_surface->pixels + (320 - MSX_WIDTH) + 6 * 2 * 320);
  direct_mode = 1;
}

void
msx_set_blit_surface()
{
  XBuf = (ushort*)(blit_surface->pixels);
  direct_mode = 0;
}

void
msx_save_back_to_blit()
{
  int x; int y;
  /* Bakcup direct back_surface to blit_surface for thumb images ! */
  if (direct_mode) {
    // u16 *s = (u16*)XBuf;
    // u16 *d = (u16*)blit_surface->pixels;
    // memmove(d, s, MSX_WIDTH * MSX_HEIGHT * sizeof(u16));
    u16* pt_src = (u16*)XBuf;
    u16* pt_dst = (u16*)blit_surface->pixels;
    for (y = 0; y < MSX_HEIGHT; y++) {
      for (x = 0; x < MSX_WIDTH; x++) {
        *pt_dst++ = pt_src[x];
      }
      pt_src += PSP_LINE_SIZE; // rs97 fix
    }
  }
}

void
msx_change_render_mode(int new_render_mode)
{
  if (new_render_mode > MSX_RENDER_FAST) {
    msx_set_blit_surface();
  } else {
    msx_set_direct_surface();
  }
  MSX.msx_render_mode = new_render_mode;
}

extern char ZipFile[256];

/** InitMachine() ********************************************/
/** Allocate resources needed by Unix/X-dependent code.     **/
/*************************************************************/
int InitMachine(int argc, char* argv[])
{
	int J,I;
	char * pch;
	Verbose=0;

	msx_default_settings();
	psp_joy_default_settings();
	psp_kbd_default_settings();

	/* Default disk images */
	if (argc > 1)
	{
		Disks[0][1]=Disks[1][1]=0;
		if ( strstr (argv[1],".rom") != NULL || strstr (argv[1],".mx1") != NULL || strstr (argv[1],".mx2") != NULL ||
	             strstr (argv[1],".ROM") != NULL || strstr (argv[1],".MX1") != NULL || strstr (argv[1],".MX2") != NULL )
		{
			if (Verbose) printf("File is a raw ROM file\n");
			snprintf(CartA, sizeof(CartA), "%s", argv[1]);
			loaded_msx = 1;
		}
		else if ( strstr (argv[1],".zip") != NULL ||
                     strstr (argv[1],".ZIP") != NULL )
		{
			if (Verbose) printf("File is a ROM file (ZIP file)\n");
			snprintf(ZipFile, sizeof(ZipFile), "%s", argv[1]);
			loaded_msx = 3;
		}
		else if ( strstr (argv[1],".dsk") != NULL ||
			  strstr (argv[1],".DSK") != NULL )
		{
			if (Verbose) printf("File is a DSK file\n");
			snprintf(DiskA, sizeof(DiskA), "%s", argv[1]);
			loaded_msx = 2;
		}
		else if ( strstr (argv[1],".cas") != NULL ||
			  strstr (argv[1],".CAS") != NULL )
		{
			if (Verbose) printf("File is a CAS file\n");
			snprintf(CasName, sizeof(CasName), "%s", argv[1]);
  			CasType = cas_type(CasName);
                        skip_disk_rom = 1;
			loaded_msx = 4;
		}
	}

	Disks[0][1]=Disks[1][1]=0;
	Disks[0][0]=DiskA;
	Disks[1][0]=DiskB;
		
	/* Reset all variables */
	memset(XKeyMap,0xFF,sizeof(XKeyMap));
	JoyState=0x00;
	CurDisk[0]=CurDisk[1]=0;

	/* Init image buffer */
	//LUDO: XBuf = (ushort *)blit_surface->pixels;
  
	/* Reset the palette */
	for(J=0;J<16;J++) XPal[J]=0;
		XPal0=0;
  
	/* Set SCREEN8 colors */
	for(J=0;J<64;J++) 
	{
		I=(J&0x03)+(J&0x0C)*16+(J&0x30)/2;
			XPal[J+16]=SDL_MapRGB(back_surface->format,
			  (J>>4)*255/3,((J>>2)&0x03)*255/3,(J&0x03)*255/3);
		BPal[I]=BPal[I|0x04]=BPal[I|0x20]=BPal[I|0x24]=XPal[J+16];
	}

	/* Initialize sound */   
	InitSound(UseSound,Verbose);
  
	/* clear on-back_surface message */
	update_save_name("");

	msx_load_settings();
	msx_kbd_load();
	msx_joy_load();
	msx_load_cheat();

	myPowerSetClockFrequency(MSX.psp_cpu_clock);

	InitMSX();

	return(1);
}

/** TrashMachine() *******************************************/
/** Deallocate all resources taken by InitMachine().        **/
/*************************************************************/
void TrashMachine(void)
{
  if(Verbose) printf("Shutting down...\n");

  /* Delete graphic buffer */
#ifdef SOUND
  TrashSound();
#endif
}

# define CENTER_X( W )  ((320 - (W)) >> 1)
# define CENTER_Y( H )  ((240 - (H)) >> 1)

/** PutImage() ***********************************************/
/** Put an image on the back_surface.                             **/
/*************************************************************/
static inline void 
PutImage_normal(void)
{
  int x;
  int y;
  uint *ptr_dst = back_surface->pixels;
  uint *ptr_src = (uint *) XBuf;
 
  ptr_dst += 12 + (160 * 6);

  for (y = 0; y < SCR_HEIGHT; y++) {
    for (x = 0; x < SCR_WIDTH/2; x++) {
      ptr_dst[x] = ptr_src[x];
    }
    ptr_src += SCR_WIDTH/2;
    ptr_dst += 320/2;
  }
}

static inline void 
PutImage_fit_width(void)
{
  int x_d, x_s;
  int y_s;
  int y;
  short *ptr_dst = back_surface->pixels + (PSP_SDL_SCREEN_HEIGHT - MSX_HEIGHT) * 320;
  short *ptr_src = XBuf;

  for (y = 0; y < SCR_HEIGHT; y++) {
    for (x_d = 0; x_d < 320; x_d++) {
      x_s = 16 + (((x_d << 1) + x_d) >> 2);
      ptr_dst[x_d] = ptr_src[x_s];
    }
    ptr_src += SCR_WIDTH;
    ptr_dst += 320;
  }
}


#define COLAVG(C1,C2) ((((C1&0xF800)>>1)+((C2&0xF800)>>1))&0xF800) | \
           ((((C1&0x07E0)+(C2&0x07E0))>>1)&0x07E0) | \
           ((((C1&0x001F)+(C2&0x001F))>>1)&0x001F);

static inline void
PutImage_zoomed(void)
{
  int x;
  int y;
  word w, wa, wb, wc;
  short *p, *prev, *next;
  short *ptr_dst = back_surface->pixels;
  short *ptr_src = XBuf;

  for (y = 0; y < SCR_HEIGHT; y++) {

    if ((y+1)%19)
    {
      p=ptr_dst;
      for (x = 0; x < SCR_WIDTH; x+=6) { // Normal line
        *p++=ptr_src[x+0];
        *p++=ptr_src[x+1];
        wa  =ptr_src[x+2];
        wb  =ptr_src[x+3];
        *p++=wa;
        *p++=COLAVG(wa, wb);
        *p++=wb;
        *p++=ptr_src[x+4];
        *p++=ptr_src[x+5];
      }
    }
    else
    {
      prev=ptr_dst-320;
      next=ptr_dst+320;
      p=ptr_dst;
      for (x = 0; x < SCR_WIDTH; x+=6) { // Interpolated line
        wa=ptr_src[x+0];
        *next++=wa;
        w=*prev++;
        *p++=COLAVG(w, wa);

        wa=ptr_src[x+1];
        *next++=wa;
        w=*prev++;
        *p++=COLAVG(w, wa);

        wa=ptr_src[x+2];
        wb=ptr_src[x+3];
        wc=COLAVG(wa, wb);

        *next++=wa;
        w=*prev++;
        *p++=COLAVG(w, wa);

        *next++=wc;
        w=*prev++;
        *p++=COLAVG(w, wc);

        *next++=wb;
        w=*prev++;
        *p++=COLAVG(w, wb);

        wa=ptr_src[x+4];
        *next++=wa;
        w=*prev++;
        *p++=COLAVG(w, wa);

        wa=ptr_src[x+5];
        *next++=wa;
        w=*prev++;
        *p++=COLAVG(w, wa);
      }
      ptr_dst += 320;
    }
    ptr_src += SCR_WIDTH;
    ptr_dst += 320;
  }
}


static inline void
PutImage_fullscreen(void)
{
  int x, y, max_y, mod_y;
  word w, wa, wb, wc;
  short *prev, *next;
  short *ptr_dst = back_surface->pixels;
  short *ptr_src = XBuf;

  if (ScanLines212) {
    max_y = 214; mod_y = 8; // 2 extra lines (border)
    ptr_src += 8 + (7 * SCR_WIDTH);
  } else {
    max_y = 192; mod_y = 4;
    ptr_src += 8 + (18 * SCR_WIDTH);
  }

  for (y = 0; y < max_y; y++) {

    if ((y+1)%mod_y)
    {
      for (x = 0; x < 256; x+=4) { // Normal line
        *ptr_dst++=ptr_src[x+0];
        wa        =ptr_src[x+1];
        wb        =ptr_src[x+2];
        *ptr_dst++=wa;
        *ptr_dst++=COLAVG(wa, wb);
        *ptr_dst++=wb;
        *ptr_dst++=ptr_src[x+3];
      }
    }
    else
    {
      prev=ptr_dst-320;
      next=ptr_dst+320;
      for (x = 0; x < 256; x+=4) { // Interpolated line

        wa=ptr_src[x+0];
        *next++=wa;
        w=*prev++;
        *ptr_dst++=COLAVG(w, wa);

        wa=ptr_src[x+1];
        wb=ptr_src[x+2];
        wc=COLAVG(wa, wb);

        *next++=wa;
        w=*prev++;
        *ptr_dst++=COLAVG(w, wa);

        *next++=wc;
        w=*prev++;
        *ptr_dst++=COLAVG(w, wc);

        *next++=wb;
        w=*prev++;
        *ptr_dst++=COLAVG(w, wb);

        wa=ptr_src[x+3];
        *next++=wa;
        w=*prev++;
        *ptr_dst++=COLAVG(w, wa);
      }
      ptr_dst=next;
    }
    ptr_src+=SCR_WIDTH;
  }
}


void
MsxKeyUp(int index, int bit_mask)
{
  XKeyMap[index]|=bit_mask;
}

void
MsxKeyDown(int index, int bit_mask)
{
  XKeyMap[index]&=~bit_mask;
}

void
MsxJoyUp(int bit_mask)
{
  JoyState |=bit_mask;
}

void
MsxJoyDown(int bit_mask)
{
  JoyState &= ~bit_mask;
}

/** Joystick() ***********************************************/
/** Query position of a joystick connected to port N.       **/
/** Returns 0.0.F2.F1.R.L.D.U.                              **/
/*************************************************************/
byte Joystick(register byte N) { return(JoyState); }

/** Mouse() **************************************************/
/** Query coordinates of a mouse connected to port N.       **/
/** Returns F2.F1.Y.Y.Y.Y.Y.Y.Y.Y.X.X.X.X.X.X.X.X.          **/
/*************************************************************/
int Mouse(register byte N)
{
  return 0;
}

/** SetColor() ***********************************************/
/** Set color N (0..15) to R,G,B.                           **/
/*************************************************************/
void SetColor(register byte N,register byte R,register byte G,register byte B)
{
  unsigned int J;
  
  J=SDL_MapRGB(back_surface->format,R,G,B);
  
  if(N) XPal[N]=J; else XPal0=J;
}

static int FirstLine = 18;     /* First scanline in the XBuf */

static void  Sprites(byte Y,pixel *Line);
static void  ColorSprites(byte Y,byte *ZBuf);
static pixel *RefreshBorder(byte Y,pixel C);
static void  ClearLine(pixel *P,pixel C);
static pixel YJKColor(int Y,int J,int K);

/** RefreshScreen() ******************************************/
/** Refresh back_surface. This function is called in the end of   **/
/** refresh cycle to show the entire back_surface.                **/
/*************************************************************/
void RefreshScreen(void) 
{ 
  msx_apply_cheats();

  if (MSX.psp_skip_cur_frame <= 0) {

    MSX.psp_skip_cur_frame = MSX.psp_skip_max_frame;

    // if (MSX.msx_render_mode != MSX_RENDER_FAST) {

    //   if (MSX.msx_render_mode == MSX_RENDER_NORMAL) PutImage_normal(); 
    //   else /* if (MSX.msx_render_mode == MSX_RENDER_FIT   ) */ PutImage_fit_width(); 
    // }
    if (MSX.msx_render_mode == MSX_RENDER_FIT) PutImage_fit_width();
    else if (MSX.msx_render_mode == MSX_RENDER_ZOOM) PutImage_zoomed();
    else if (MSX.msx_render_mode == MSX_RENDER_FULL) PutImage_fullscreen();
    else /*if (MSX.msx_render_mode == MSX_RENDER_FAST) */ msx_set_direct_surface();

    if (psp_kbd_is_danzeff_mode()) {
      danzeff_moveTo(-30, -75);
      danzeff_render();
    }

    if (MSX.msx_view_fps) {
      char buffer[32];
      sprintf(buffer, "%03d %3d", MSX.msx_current_clock, (int)MSX.msx_current_fps );
      psp_sdl_fill_print(0, 0, buffer, 0xffffff, 0 );
    }

    if (MSX.msx_vsync) {
      psp_msx_wait_vsync();
    }
    psp_sdl_flip();

  
    if (psp_screenshot_mode) {
      psp_screenshot_mode--;
      if (psp_screenshot_mode <= 0) {
        psp_sdl_save_screenshot();
        psp_screenshot_mode = 0;
      }
    }

  } else if (MSX.psp_skip_max_frame) {
    MSX.psp_skip_cur_frame--;
  }

  msx_synchronize();
}

/** ClearLine() **********************************************/
/** Clear 256 pixels from P with color C.                   **/
/*************************************************************/
static void ClearLine(register pixel *P,register pixel C)
{
  register int J;

  for(J=0;J<256;J++) P[J]=C;
}

/** YJKColor() ***********************************************/
/** Given a color in YJK format, return the corresponding   **/
/** palette entry.                                          **/
/*************************************************************/
static inline pixel YJKColor(register int Y,register int J,register int K)
{
  register int R,G,B;
		
  R=Y+J;
  G=Y+K;
  B=((5*Y-2*J-K)/4);

  R=R<0? 0:R>31? 31:R;
  G=G<0? 0:G>31? 31:G;
  B=B<0? 0:B>31? 31:B;

  return(BPal[(R&0x1C)|((G&0x1C)<<3)|(B>>3)]);
}

/** RefreshBorder() ******************************************/
/** This function is called from RefreshLine#() to refresh  **/
/** the back_surface border. It returns a pointer to the start of **/
/** scanline Y in XBuf or 0 if scanline is beyond XBuf.     **/
/*************************************************************/
pixel *
RefreshBorder_blit(register byte Y,register pixel C)
{
  register pixel *P;
  register int H;

  /* First line number in the buffer */
  if(!Y) FirstLine=(ScanLines212? 8:18)+VAdjust;

  /* Return 0 if we've run out of the back_surface buffer due to overscan */
  if(Y+FirstLine>=SCR_HEIGHT) return(0);

  /* Set up the transparent color */
  XPal[0]=(!BGColor||SolidColor0)? XPal0:XPal[BGColor];

  /* Start of the buffer */
  P=(pixel *)XBuf;

  /* Paint top of the back_surface */
  if(!Y) for(H=SCR_WIDTH*FirstLine-1;H>=0;H--) P[H]=C;

  /* Start of the line */
  P+=SCR_WIDTH*(FirstLine+Y);

  /* Paint left/right borders */
  for(H=(SCR_WIDTH-256)/2+HAdjust;H>0;H--) P[H-1]=C;
  for(H=(SCR_WIDTH-256)/2-HAdjust;H>0;H--) P[SCR_WIDTH-H]=C;

  /* Paint bottom of the back_surface */
  H=ScanLines212? 212:192;
  if(Y==H-1) for(H=SCR_WIDTH*(SCR_HEIGHT-H-FirstLine+1)-1;H>=SCR_WIDTH;H--) P[H]=C;

  /* Return pointer to the scanline in XBuf */
  return(P+(SCR_WIDTH-256)/2+HAdjust);
}

pixel*
RefreshBorder_direct(register byte Y,register pixel C)
{
/* PSP Pitch */ 
# define SCR_PITCH PSP_LINE_SIZE 
  register pixel *P;
  register int H;
  int x;

  /* First line number in the buffer */
  if(!Y) FirstLine=(ScanLines212? 8:18)+VAdjust;

  /* Return 0 if we've run out of the back_surface buffer due to overscan */
  if(Y+FirstLine>=SCR_HEIGHT) return(0);

  /* Set up the transparent color */
  XPal[0]=(!BGColor||SolidColor0)? XPal0:XPal[BGColor];

  /* Start of the buffer */
  P=(pixel *)XBuf;

  /* Paint top of the back_surface */
  if(!Y) {
    for(H=FirstLine-1;H>=0;H--) {
      pixel* scan_buf = P + (H*SCR_PITCH);
      for (x = 0; x < SCR_WIDTH; x++) {
       *scan_buf++=C;
      }
    }
  }

  /* Start of the line */
  P+=SCR_PITCH*(FirstLine+Y);

  /* Paint left/right borders */
  for(H=(SCR_WIDTH-256)/2+HAdjust;H>0;H--) P[H-1]=C;
  for(H=(SCR_WIDTH-256)/2-HAdjust;H>0;H--) P[SCR_WIDTH-H]=C;

  /* Paint bottom of the back_surface */
  H=ScanLines212? 212:192;
  if(Y==H-1) {
    for(H=(SCR_HEIGHT-H-FirstLine+1)-1;H>=1;H--) {
      pixel* scan_buf = P + (H*SCR_PITCH);
      for (x = 0; x < SCR_WIDTH; x++) {
       *scan_buf++=C;
      }
    }
  }

  /* Return pointer to the scanline in XBuf */
  return(P+(SCR_WIDTH-256)/2+HAdjust);
}

static inline pixel*
RefreshBorder(register byte Y,register pixel C)
{
  if (! direct_mode) {
    return RefreshBorder_blit(Y, C);
  }
  return RefreshBorder_direct(Y, C);
}


/** Sprites() ************************************************/
/** This function is called from RefreshLine#() to refresh  **/
/** sprites in SCREENs 1-3.                                 **/
/*************************************************************/
void Sprites(register byte Y,register pixel *Line)
{
  register pixel *P,C;
  register byte H,*PT,*AT;
  register unsigned int M;
  register int L,K;

  /* Assign initial values before counting */
  H=Sprites16x16? 16:8;
  C=0;M=0;L=0;
  AT=SprTab-4;
  Y+=VScroll;

  /* Count displayed sprites */
  do
  {
    M<<=1;AT+=4;L++;    /* Iterating through SprTab      */
    K=AT[0];            /* K = sprite Y coordinate       */
    if(K==208) break;   /* Iteration terminates if Y=208 */
    if(K>256-H) K-=256; /* Y coordinate may be negative  */

    /* Mark all valid sprites with 1s, break at MAXSPRITE1 sprites */
    if((Y>K)&&(Y<=K+H)) { M|=1;C++;if(C==MAXSPRITE1) break; }
  }
  while(L<32);

  /* Draw all marked sprites */
  for(;M;M>>=1,AT-=4)
    if(M&1)
    {
      C=AT[3];                  /* C = sprite attributes */
      L=C&0x80? AT[1]-32:AT[1]; /* Sprite may be shifted left by 32 */
      C&=0x0F;                  /* C = sprite color */

      if((L<256)&&(L>-H)&&C)
      {
        K=AT[0];                /* K = sprite Y coordinate */
        if(K>256-H) K-=256;     /* Y coordinate may be negative */

        P=Line+L;
        PT=SprGen+((int)(H>8? AT[2]&0xFC:AT[2])<<3)+Y-K-1;
        C=XPal[C];

        /* Mask 1: clip left sprite boundary */
        K=L>=0? 0x0FFFF:(0x10000>>-L)-1;
        /* Mask 2: clip right sprite boundary */
        if(L>256-H) K^=((0x00200>>(H-8))<<(L-257+H))-1;
        /* Get and clip the sprite data */
        K&=((int)PT[0]<<8)|(H>8? PT[16]:0x00);

        /* Draw left 8 pixels of the sprite */
        if(K&0xFF00)
        {
          if(K&0x8000) P[0]=C;if(K&0x4000) P[1]=C;
          if(K&0x2000) P[2]=C;if(K&0x1000) P[3]=C;
          if(K&0x0800) P[4]=C;if(K&0x0400) P[5]=C;
          if(K&0x0200) P[6]=C;if(K&0x0100) P[7]=C;
        }

        /* Draw right 8 pixels of the sprite */
        if(K&0x00FF)
        {
          if(K&0x0080) P[8]=C; if(K&0x0040) P[9]=C;
          if(K&0x0020) P[10]=C;if(K&0x0010) P[11]=C;
          if(K&0x0008) P[12]=C;if(K&0x0004) P[13]=C;
          if(K&0x0002) P[14]=C;if(K&0x0001) P[15]=C;
        }
      }
    }
}

/** ColorSprites() *******************************************/
/** This function is called from RefreshLine#() to refresh  **/
/** color sprites in SCREENs 4-8. The result is returned in **/
/** ZBuf, whose size must be 304 bytes (32+256+16).         **/
/*************************************************************/
void ColorSprites(register byte Y,byte *ZBuf)
{
  register byte C,H,J,OrThem;
  register byte *P,*PT,*AT;
  register int L,K;
  register unsigned int M;

  /* Clear ZBuffer and exit if sprites are off */
  memset(ZBuf+32,0,256);
  if(SpritesOFF) return;

  /* Assign initial values before counting */
  H=Sprites16x16? 16:8;
  C=0;M=0;L=0;
  AT=SprTab-4;
  OrThem=0x00;

  /* Count displayed sprites */
  do
  {
    M<<=1;AT+=4;L++;          /* Iterating through SprTab      */
    K=AT[0];                  /* Read Y from SprTab            */
    if(K==216) break;         /* Iteration terminates if Y=216 */
    K=(byte)(K-VScroll);      /* Sprite's actual Y coordinate  */
    if(K>256-H) K-=256;       /* Y coordinate may be negative  */

    /* Mark all valid sprites with 1s, break at MAXSPRITE2 sprites */
    if((Y>K)&&(Y<=K+H)) { M|=1;C++;if(C==MAXSPRITE2) break; }
  }
  while(L<32);

  /* Draw all marked sprites */
  for(;M;M>>=1,AT-=4)
    if(M&1)
    {
      K=(byte)(AT[0]-VScroll); /* K = sprite Y coordinate */
      if(K>256-H) K-=256;      /* Y coordinate may be negative */

      J=Y-K-1;
      C=SprTab[-0x0200+((AT-SprTab)<<2)+J];
      OrThem|=C&0x40;

      if(C&0x0F)
      {
        PT=SprGen+((int)(H>8? AT[2]&0xFC:AT[2])<<3)+J;
        P=ZBuf+AT[1]+(C&0x80? 0:32);
        C&=0x0F;
        J=PT[0];

        if(OrThem&0x20)
        {
          if(J&0x80) P[0]|=C;if(J&0x40) P[1]|=C;
          if(J&0x20) P[2]|=C;if(J&0x10) P[3]|=C;
          if(J&0x08) P[4]|=C;if(J&0x04) P[5]|=C;
          if(J&0x02) P[6]|=C;if(J&0x01) P[7]|=C;
          if(H>8)
          {
            J=PT[16];
            if(J&0x80) P[8]|=C; if(J&0x40) P[9]|=C;
            if(J&0x20) P[10]|=C;if(J&0x10) P[11]|=C;
            if(J&0x08) P[12]|=C;if(J&0x04) P[13]|=C;
            if(J&0x02) P[14]|=C;if(J&0x01) P[15]|=C;
          }
        }
        else
        {
          if(J&0x80) P[0]=C;if(J&0x40) P[1]=C;
          if(J&0x20) P[2]=C;if(J&0x10) P[3]=C;
          if(J&0x08) P[4]=C;if(J&0x04) P[5]=C;
          if(J&0x02) P[6]=C;if(J&0x01) P[7]=C;
          if(H>8)
          {
            J=PT[16];
            if(J&0x80) P[8]=C; if(J&0x40) P[9]=C;
            if(J&0x20) P[10]=C;if(J&0x10) P[11]=C;
            if(J&0x08) P[12]=C;if(J&0x04) P[13]=C;
            if(J&0x02) P[14]=C;if(J&0x01) P[15]=C;
          }
        }
      }

      /* Update overlapping flag */
      OrThem>>=1;
    }
}

/** RefreshLineF() *******************************************/
/** Dummy refresh function called for non-existing screens. **/
/*************************************************************/
void RefreshLineF(register byte Y)
{
  register pixel *P;

  if(Verbose>1)
    printf
    (
      "ScrMODE %d: ChrTab=%X ChrGen=%X ColTab=%X SprTab=%X SprGen=%X\n",
      ScrMode,ChrTab-VRAM,ChrGen-VRAM,ColTab-VRAM,SprTab-VRAM,SprGen-VRAM
    );

  P=RefreshBorder(Y,XPal[BGColor]);
  if(P) ClearLine(P,XPal[BGColor]);
}

/** RefreshLine0() *******************************************/
/** Refresh line Y (0..191/211) of SCREEN0.                 **/
/*************************************************************/
void RefreshLine0(register byte Y)
{
  register pixel *P,FC,BC;
  register byte X,*T,*G;

  BC=XPal[BGColor];
  P=RefreshBorder(Y,BC);
  if(!P) return;

  if(!ScreenON) ClearLine(P,BC);
  else
  {
    P[0]=P[1]=P[2]=P[3]=P[4]=P[5]=P[6]=P[7]=P[8]=BC;

    G=(UseFont&&FontBuf? FontBuf:ChrGen)+((Y+VScroll)&0x07);
    T=ChrTab+40*(Y>>3);
    FC=XPal[FGColor];
    P+=9;

    for(X=0;X<40;X++,T++,P+=6)
    {
      Y=G[(int)*T<<3];
      P[0]=Y&0x80? FC:BC;P[1]=Y&0x40? FC:BC;
      P[2]=Y&0x20? FC:BC;P[3]=Y&0x10? FC:BC;
      P[4]=Y&0x08? FC:BC;P[5]=Y&0x04? FC:BC;
    }

    P[0]=P[1]=P[2]=P[3]=P[4]=P[5]=P[6]=BC;
  }
}

/** RefreshLine1() *******************************************/
/** Refresh line Y (0..191/211) of SCREEN1, including       **/
/** sprites in this line.                                   **/
/*************************************************************/
void RefreshLine1(register byte Y)
{
  register pixel *P,FC,BC;
  register byte K,X,*T,*G;

  P=RefreshBorder(Y,XPal[BGColor]);
  if(!P) return;

  if(!ScreenON) ClearLine(P,XPal[BGColor]);
  else
  {
    Y+=VScroll;
    G=(UseFont&&FontBuf? FontBuf:ChrGen)+(Y&0x07);
    T=ChrTab+((int)(Y&0xF8)<<2);

    for(X=0;X<32;X++,T++,P+=8)
    {
      K=ColTab[*T>>3];
      FC=XPal[K>>4];
      BC=XPal[K&0x0F];
      K=G[(int)*T<<3];
      P[0]=K&0x80? FC:BC;P[1]=K&0x40? FC:BC;
      P[2]=K&0x20? FC:BC;P[3]=K&0x10? FC:BC;
      P[4]=K&0x08? FC:BC;P[5]=K&0x04? FC:BC;
      P[6]=K&0x02? FC:BC;P[7]=K&0x01? FC:BC;
    }

    if(!SpritesOFF) Sprites(Y,P-256);
  }
}

/** RefreshLine2() *******************************************/
/** Refresh line Y (0..191/211) of SCREEN2, including       **/
/** sprites in this line.                                   **/
/*************************************************************/
void RefreshLine2(register byte Y)
{
  register pixel *P,FC,BC;
  register byte K,X,*T;
  register int I,J;

  P=RefreshBorder(Y,XPal[BGColor]);
  if(!P) return;

  if(!ScreenON) ClearLine(P,XPal[BGColor]);
  else
  {
    Y+=VScroll;
    T=ChrTab+((int)(Y&0xF8)<<2);
    I=((int)(Y&0xC0)<<5)+(Y&0x07);

    for(X=0;X<32;X++,T++,P+=8)
    {
      J=(int)*T<<3;
      K=ColTab[(I+J)&ColTabM];
      FC=XPal[K>>4];
      BC=XPal[K&0x0F];
      K=ChrGen[(I+J)&ChrGenM];
      P[0]=K&0x80? FC:BC;P[1]=K&0x40? FC:BC;
      P[2]=K&0x20? FC:BC;P[3]=K&0x10? FC:BC;
      P[4]=K&0x08? FC:BC;P[5]=K&0x04? FC:BC;
      P[6]=K&0x02? FC:BC;P[7]=K&0x01? FC:BC;
    }

    if(!SpritesOFF) Sprites(Y,P-256);
  }
}

/** RefreshLine3() *******************************************/
/** Refresh line Y (0..191/211) of SCREEN3, including       **/
/** sprites in this line.                                   **/
/*************************************************************/
void RefreshLine3(register byte Y)
{
  register pixel *P;
  register byte X,K,*T,*G;

  P=RefreshBorder(Y,XPal[BGColor]);
  if(!P) return;

  if(!ScreenON) ClearLine(P,XPal[BGColor]);
  else
  {
    Y+=VScroll;
    T=ChrTab+((int)(Y&0xF8)<<2);
    G=ChrGen+((Y&0x1C)>>2);

    for(X=0;X<32;X++,T++,P+=8)
    {
      K=G[(int)*T<<3];
      P[0]=P[1]=P[2]=P[3]=XPal[K>>4];
      P[4]=P[5]=P[6]=P[7]=XPal[K&0x0F];
    }

    if(!SpritesOFF) Sprites(Y,P-256);
  }
}

/** RefreshLine4() *******************************************/
/** Refresh line Y (0..191/211) of SCREEN4, including       **/
/** sprites in this line.                                   **/
/*************************************************************/
void RefreshLine4(register byte Y)
{
  register pixel *P,FC,BC;
  register byte K,X,C,*T,*R;
  register int I,J;
  byte ZBuf[304];

  P=RefreshBorder(Y,XPal[BGColor]);
  if(!P) return;

  if(!ScreenON) ClearLine(P,XPal[BGColor]);
  else
  {
    ColorSprites(Y,ZBuf);
    R=ZBuf+32;
    Y+=VScroll;
    T=ChrTab+((int)(Y&0xF8)<<2);
    I=((int)(Y&0xC0)<<5)+(Y&0x07);

    for(X=0;X<32;X++,R+=8,P+=8,T++)
    {
      J=(int)*T<<3;
      K=ColTab[(I+J)&ColTabM];
      FC=XPal[K>>4];
      BC=XPal[K&0x0F];
      K=ChrGen[(I+J)&ChrGenM];

      C=R[0];P[0]=C? XPal[C]:(K&0x80)? FC:BC;
      C=R[1];P[1]=C? XPal[C]:(K&0x40)? FC:BC;
      C=R[2];P[2]=C? XPal[C]:(K&0x20)? FC:BC;
      C=R[3];P[3]=C? XPal[C]:(K&0x10)? FC:BC;
      C=R[4];P[4]=C? XPal[C]:(K&0x08)? FC:BC;
      C=R[5];P[5]=C? XPal[C]:(K&0x04)? FC:BC;
      C=R[6];P[6]=C? XPal[C]:(K&0x02)? FC:BC;
      C=R[7];P[7]=C? XPal[C]:(K&0x01)? FC:BC;
    }
  }
}

/** RefreshLine5() *******************************************/
/** Refresh line Y (0..191/211) of SCREEN5, including       **/
/** sprites in this line.                                   **/
/*************************************************************/
void RefreshLine5(register byte Y)
{
  register pixel *P;
  register byte I,X,*T,*R;
  byte ZBuf[304];

  P=RefreshBorder(Y,XPal[BGColor]);
  if(!P) return;

  if(!ScreenON) ClearLine(P,XPal[BGColor]);
  else
  {
    ColorSprites(Y,ZBuf);
    R=ZBuf+32;
    T=ChrTab+(((int)(Y+VScroll)<<7)&ChrTabM&0x7FFF);

    for(X=0;X<16;X++,R+=16,P+=16,T+=8)
    {
      I=R[0];P[0]=XPal[I? I:T[0]>>4];
      I=R[1];P[1]=XPal[I? I:T[0]&0x0F];
      I=R[2];P[2]=XPal[I? I:T[1]>>4];
      I=R[3];P[3]=XPal[I? I:T[1]&0x0F];
      I=R[4];P[4]=XPal[I? I:T[2]>>4];
      I=R[5];P[5]=XPal[I? I:T[2]&0x0F];
      I=R[6];P[6]=XPal[I? I:T[3]>>4];
      I=R[7];P[7]=XPal[I? I:T[3]&0x0F];
      I=R[8];P[8]=XPal[I? I:T[4]>>4];
      I=R[9];P[9]=XPal[I? I:T[4]&0x0F];
      I=R[10];P[10]=XPal[I? I:T[5]>>4];
      I=R[11];P[11]=XPal[I? I:T[5]&0x0F];
      I=R[12];P[12]=XPal[I? I:T[6]>>4];
      I=R[13];P[13]=XPal[I? I:T[6]&0x0F];
      I=R[14];P[14]=XPal[I? I:T[7]>>4];
      I=R[15];P[15]=XPal[I? I:T[7]&0x0F];
    }
  }
}

/** RefreshLine8() *******************************************/
/** Refresh line Y (0..191/211) of SCREEN8, including       **/
/** sprites in this line.                                   **/
/*************************************************************/
void RefreshLine8(register byte Y)
{
  static byte SprToScr[16] =
  {
    0x00,0x02,0x10,0x12,0x80,0x82,0x90,0x92,
    0x49,0x4B,0x59,0x5B,0xC9,0xCB,0xD9,0xDB
  };
  register pixel *P;
  register byte C,X,*T,*R;
  byte ZBuf[304];

  P=RefreshBorder(Y,BPal[VDP[7]]);
  if(!P) return;

  if(!ScreenON) ClearLine(P,BPal[VDP[7]]);
  else
  {
    ColorSprites(Y,ZBuf);
    R=ZBuf+32;
    T=ChrTab+(((int)(Y+VScroll)<<8)&ChrTabM&0xFFFF);

    for(X=0;X<32;X++,T+=8,R+=8,P+=8)
    {
      C=R[0];P[0]=BPal[C? SprToScr[C]:T[0]];
      C=R[1];P[1]=BPal[C? SprToScr[C]:T[1]];
      C=R[2];P[2]=BPal[C? SprToScr[C]:T[2]];
      C=R[3];P[3]=BPal[C? SprToScr[C]:T[3]];
      C=R[4];P[4]=BPal[C? SprToScr[C]:T[4]];
      C=R[5];P[5]=BPal[C? SprToScr[C]:T[5]];
      C=R[6];P[6]=BPal[C? SprToScr[C]:T[6]];
      C=R[7];P[7]=BPal[C? SprToScr[C]:T[7]];
    }
  }
}

/** RefreshLine10() ******************************************/
/** Refresh line Y (0..191/211) of SCREEN10/11, including   **/
/** sprites in this line.                                   **/
/*************************************************************/
void RefreshLine10(register byte Y)
{
  register pixel *P;
  register byte C,X,*T,*R;
  register int J,K;
  byte ZBuf[304];

  P=RefreshBorder(Y,BPal[VDP[7]]);
  if(!P) return;

  if(!ScreenON) ClearLine(P,BPal[VDP[7]]);
  else
  {
    ColorSprites(Y,ZBuf);
    R=ZBuf+32;
    T=ChrTab+(((int)(Y+VScroll)<<8)&ChrTabM&0xFFFF);

    /* Draw first 4 pixels */
    C=R[0];P[0]=C? XPal[C]:BPal[VDP[7]];
    C=R[1];P[1]=C? XPal[C]:BPal[VDP[7]];
    C=R[2];P[2]=C? XPal[C]:BPal[VDP[7]];
    C=R[3];P[3]=C? XPal[C]:BPal[VDP[7]];
    R+=4;P+=4;

    for(X=0;X<63;X++,T+=4,R+=4,P+=4)
    {
      K=(T[0]&0x07)|((T[1]&0x07)<<3);
      if(K&0x20) K-=64;
      J=(T[2]&0x07)|((T[3]&0x07)<<3);
      if(J&0x20) J-=64;

      C=R[0];Y=T[0]>>3;P[0]=C? XPal[C]:Y&1? XPal[Y>>1]:YJKColor(Y,J,K);
      C=R[1];Y=T[1]>>3;P[1]=C? XPal[C]:Y&1? XPal[Y>>1]:YJKColor(Y,J,K);
      C=R[2];Y=T[2]>>3;P[2]=C? XPal[C]:Y&1? XPal[Y>>1]:YJKColor(Y,J,K);
      C=R[3];Y=T[3]>>3;P[3]=C? XPal[C]:Y&1? XPal[Y>>1]:YJKColor(Y,J,K);
    }
  }
}

/** RefreshLine12() ******************************************/
/** Refresh line Y (0..191/211) of SCREEN12, including      **/
/** sprites in this line.                                   **/
/*************************************************************/
void RefreshLine12(register byte Y)
{
  register pixel *P;
  register byte C,X,*T,*R;
  register int J,K;
  byte ZBuf[304];

  P=RefreshBorder(Y,BPal[VDP[7]]);
  if(!P) return;

  if(!ScreenON) ClearLine(P,BPal[VDP[7]]);
  else
  {
    ColorSprites(Y,ZBuf);
    R=ZBuf+32;
    T=ChrTab+(((int)(Y+VScroll)<<8)&ChrTabM&0xFFFF);

    if(HScroll512&&(HScroll>255)) T=(byte *)((int)T^0x10000);
    T+=HScroll&0xFC;

    /* Draw first 4 pixels */
    C=R[0];P[0]=C? XPal[C]:BPal[VDP[7]];
    C=R[1];P[1]=C? XPal[C]:BPal[VDP[7]];
    C=R[2];P[2]=C? XPal[C]:BPal[VDP[7]];
    C=R[3];P[3]=C? XPal[C]:BPal[VDP[7]];
    R+=4;P+=4;

    for(X=1;X<64;X++,T+=4,R+=4,P+=4)
    {
      K=(T[0]&0x07)|((T[1]&0x07)<<3);
      if(K&0x20) K-=64;
      J=(T[2]&0x07)|((T[3]&0x07)<<3);
      if(J&0x20) J-=64;

      C=R[0];P[0]=C? XPal[C]:YJKColor(T[0]>>3,J,K);
      C=R[1];P[1]=C? XPal[C]:YJKColor(T[1]>>3,J,K);
      C=R[2];P[2]=C? XPal[C]:YJKColor(T[2]>>3,J,K);
      C=R[3];P[3]=C? XPal[C]:YJKColor(T[3]>>3,J,K);
    }
  }
}

/** RefreshLine6() *******************************************/
/** Refresh line Y (0..191/211) of SCREEN6, including       **/
/** sprites in this line.                                   **/
/*************************************************************/
void RefreshLine6(register byte Y)
{
  register pixel *P;
  register byte X,*T,*R,C;
  byte ZBuf[304];

  P=RefreshBorder(Y,XPal[BGColor&0x03]);
  if(!P) return;

  if(!ScreenON) ClearLine(P,XPal[BGColor&0x03]);
  else
  {
    ColorSprites(Y,ZBuf);
    R=ZBuf+32;
    T=ChrTab+(((int)(Y+VScroll)<<7)&ChrTabM&0x7FFF);

    for(X=0;X<32;X++)
    {
      C=R[0];P[0]=XPal[C? C:T[0]>>6];
      C=R[1];P[1]=XPal[C? C:(T[0]>>2)&0x03];
      C=R[2];P[2]=XPal[C? C:T[1]>>6];
      C=R[3];P[3]=XPal[C? C:(T[1]>>2)&0x03];
      C=R[4];P[4]=XPal[C? C:T[2]>>6];
      C=R[5];P[5]=XPal[C? C:(T[2]>>2)&0x03];
      C=R[6];P[6]=XPal[C? C:T[3]>>6];
      C=R[7];P[7]=XPal[C? C:(T[3]>>2)&0x03];
      R+=8;P+=8;T+=4;
    }
  }
}
  
/** RefreshLine7() *******************************************/
/** Refresh line Y (0..191/211) of SCREEN7, including       **/
/** sprites in this line.                                   **/
/*************************************************************/
void RefreshLine7(register byte Y)
{
  register pixel *P;
  register byte C,X,*T,*R;
  byte ZBuf[304];

  P=RefreshBorder(Y,XPal[BGColor]);
  if(!P) return;

  if(!ScreenON) ClearLine(P,XPal[BGColor]);
  else
  {
    ColorSprites(Y,ZBuf);
    R=ZBuf+32;
    T=ChrTab+(((int)(Y+VScroll)<<8)&ChrTabM&0xFFFF);

    for(X=0;X<32;X++)
    {
      C=R[0];P[0]=XPal[C? C:T[0]>>4];
      C=R[1];P[1]=XPal[C? C:T[1]>>4];
      C=R[2];P[2]=XPal[C? C:T[2]>>4];
      C=R[3];P[3]=XPal[C? C:T[3]>>4];
      C=R[4];P[4]=XPal[C? C:T[4]>>4];
      C=R[5];P[5]=XPal[C? C:T[5]>>4];
      C=R[6];P[6]=XPal[C? C:T[6]>>4];
      C=R[7];P[7]=XPal[C? C:T[7]>>4];
      R+=8;P+=8;T+=8;
    }
  }
}

/** RefreshLineTx80() ****************************************/
/** Refresh line Y (0..191/211) of TEXT80.                  **/
/*************************************************************/
void RefreshLineTx80(register byte Y)
{
  register pixel *P,FC,BC;
  register byte X,M,*T,*C,*G;

  BC=XPal[BGColor];
  P=RefreshBorder(Y,BC);
  if(!P) return;

  if(!ScreenON) ClearLine(P,BC);
  else
  {
    P[0]=P[1]=P[2]=P[3]=P[4]=P[5]=P[6]=P[7]=P[8]=BC;
    G=(UseFont&&FontBuf? FontBuf:ChrGen)+((Y+VScroll)&0x07);
    T=ChrTab+((80*(Y>>3))&ChrTabM);
    C=ColTab+((10*(Y>>3))&ColTabM);
    P+=9;

    for(X=0,M=0x00;X<80;X++,T++,P+=3)
    {
      if(!(X&0x07)) M=*C++;
      if(M&0x80) { FC=XPal[XFGColor];BC=XPal[XBGColor]; }
      else       { FC=XPal[FGColor];BC=XPal[BGColor]; }
      M<<=1;
      Y=*(G+((int)*T<<3));
      P[0]=Y&0xC0? FC:BC;
      P[1]=Y&0x30? FC:BC;
      P[2]=Y&0x0C? FC:BC;
    }

    P[0]=P[1]=P[2]=P[3]=P[4]=P[5]=P[6]=XPal[BGColor];
  }
}

