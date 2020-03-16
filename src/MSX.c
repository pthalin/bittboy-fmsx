/** fMSX: portable MSX emulator ******************************/
/**                                                         **/
/**                          MSX.c                          **/
/**                                                         **/
/** This file contains implementation for the MSX-specific  **/
/** hardware: slots, memory mapper, PPIs, VDP, PSG, clock,  **/
/** etc. Initialization code and definitions needed for the **/
/** machine-dependent drivers are also here.                **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1994-2003                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/

#include "MSX.h"
#include "Sound.h"
#include "gp2x_cpu.h"
#include "sha1.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>

#include <zlib.h>
#include "unzip.h"
#include <sys/types.h>
#include <sys/stat.h>
#include "psp_fmgr.h"
#include "psp_menu.h"
#include "psp_joy.h"
#include "psp_kbd.h"

#ifdef __BORLANDC__
#include <dir.h>
#endif

#ifdef __WATCOMC__
#include <direct.h>
#endif

#ifdef UNIX
#include <unistd.h>
#endif
#include <dirent.h>

#ifdef ZLIB
#include <zlib.h>
#endif

#define PRINTOK        if(Verbose) puts("OK")
#define PRINTFAILED    if(Verbose) puts("FAILED")
#define PRINTRESULT(R) if(Verbose) puts((R)? "OK":"FAILED")

#define RGB2INT(R,G,B) ((B)|((int)(G)<<8)|((int)(R)<<16))

/* MSDOS chdir() is broken and has to be replaced :( */
#ifdef MSDOS
#include "LibMSDOS.h"
#define chdir(path) ChangeDir(path)
#endif

#include "psp_sdl.h"
#include "psp_kbd.h"

int skip_disk_rom = 0;

//LUDO:
  MSX_t MSX;


/** User-defined parameters for fMSX *************************/
byte Verbose     = 0;              /* Debug msgs ON/OFF      */
int  VPeriod     = CPU_VPERIOD;    /* CPU cycles per VBlank  */
int  HPeriod     = CPU_HPERIOD;    /* CPU cycles per HBlank  */
byte SaveCMOS    = 0;              /* Save CMOS.ROM on exit  */
byte SaveSRAM    = 0;              /* ~ GMASTER2.RAM on exit */
//LUDO:
byte JoyTypeA    = 1;              /* 0=None,1=Joystick,     */
byte JoyTypeB    = 1;              /* 2=MouseAsJstk,3=Mouse  */
int  ROMTypeA    = -1;             /* MegaROM types          */
int  ROMTypeB    = -1;
int  VRAMPages   = 8;              /* Number of VRAM pages   */
byte AutoFire    = 0;              /* Autofire on [SPACE]    */
byte UseDrums    = 0;              /* Use drms for PSG noise */
byte ExitNow     = 0;              /* 1 = Exit the emulator  */

/** Main hardware: CPU, RAM, VRAM, mappers *******************/
extern Z80 CPU;                           /* Z80 CPU state and regs */

byte *VRAM,*VPAGE;                 /* Video RAM              */

byte *RAM[8];                      /* Main RAM (8x8kB pages) */
byte *EmptyRAM;                    /* Empty RAM page (8kB)   */
byte *SRAM;                        /* SRAM (battery backed)  */
byte *FMPACROM;                    /* ROM for the FMPAC      */
byte *MemMap[4][4][8];   /* Memory maps [PPage][SPage][Addr] */

byte *RAMData;                     /* RAM Mapper contents    */
byte RAMMapper[4];                 /* RAM Mapper state       */
byte RAMMask;                      /* RAM Mapper mask        */

byte *ROMData[2];                  /* ROM Mapper contents    */
byte ROMMapper[2][4];              /* ROM Mappers state      */
byte ROMMask[2];                   /* ROM Mapper masks       */

byte EnWrite[4];                   /* 1 if write enabled     */
byte PSL[4],SSL[4];                /* Lists of current slots */
byte PSLReg,SSLReg;      /* Storage for A8h port and (FFFFh) */

byte RCounter;               /* Counter for Z80's R register */

/** Memory blocks to free in TrashMSX() **********************/
byte *Chunks[256];                 /* Memory blocks to free  */
byte CCount;                       /* Number of memory blcks */

/** Cartridge files used by fMSX *****************************/
char CartA[256]      = "carta.rom";    /* Cartridge A ROM file   */
char CartB[256]      = "cartb.rom";    /* Cartridge B ROM file   */

/** Disk images used by fMSX *********************************/
char DiskA[256]      = "drivea.dsk";   /* Drive A disk image  */
char DiskB[256]      = "driveb.dsk";   /* Drive B disk image  */

char ZipFile[256];    /* Cartridge A ROM file   */
/** Soundtrack logging ***************************************/
char *SndName    = "log.mid";      /* Sound log file         */

/** Emulation state saving ***********************************/
char *StateName  = 0;              /* State file (autogen-d) */

/** Fixed font used by fMSX **********************************/
char *FontName   = "default.fnt";  /* Font file for text     */
byte *FontBuf;                     /* Font for text modes    */
byte UseFont     = 0;              /* Use ext. font when 1   */

/** Printer **************************************************/
char *PrnName    = NULL;           /* Printer redirect. file */
FILE *PrnStream;

/** Cassette tape ********************************************/
char CasName[256]    = "default.cas";  /* Tape image file        */
FILE *CasStream;

/** Serial port **********************************************/
char *ComName    = NULL;           /* Serial redirect. file  */
FILE *ComIStream;
FILE *ComOStream;

/** Kanji font ROM *******************************************/
byte *Kanji;                       /* Kanji ROM 4096x32      */
int  KanLetter;                    /* Current letter index   */
byte KanCount;                     /* Byte count 0..31       */

/** Keyboard and mouse ***************************************/
byte KeyMap[16];                   /* Keyboard map           */
byte Buttons[2];                   /* Mouse button states    */
byte MouseDX[2],MouseDY[2];        /* Mouse offsets          */
byte OldMouseX[2],OldMouseY[2];    /* Old mouse coordinates  */
byte MCount[2];                    /* Mouse nibble counter   */

/** General I/O registers: i8255 *****************************/
I8255 PPI;                         /* i8255 PPI at A8h-ABh   */
byte IOReg;                        /* Storage for AAh port   */

/** Sound hardware: PSG, SCC, OPLL ***************************/
AY8910 PSG;                        /* PSG registers & state  */
YM2413 OPLL;                       /* OPLL registers & state */
SCC  SCChip;                       /* SCC registers & state  */
byte SCCOn[2];                     /* 1 = SCC page active    */

/** Serial I/O hardware: i8251+i8253 *************************/
I8251 SIO;                         /* SIO registers & state  */

/** Real-time clock ******************************************/
byte RTCReg,RTCMode;               /* RTC register numbers   */
byte RTC[4][13];                   /* RTC registers          */

/** Video processor ******************************************/
byte *ChrGen,*ChrTab,*ColTab;      /* VDP tables (screen)    */
byte *SprGen,*SprTab;              /* VDP tables (sprites)   */
int  ChrGenM,ChrTabM,ColTabM;      /* VDP masks (screen)     */
int  SprTabM;                      /* VDP masks (sprites)    */
word VAddr;                        /* VRAM address in VDP    */
byte VKey,PKey,WKey;               /* Status keys for VDP    */
byte FGColor,BGColor;              /* Colors                 */
byte XFGColor,XBGColor;            /* Second set of colors   */
byte ScrMode;                      /* Current screen mode    */
byte VDP[64],VDPStatus[16];        /* VDP registers          */
byte IRQPending;                   /* Pending interrupts     */
int  ScanLine;                     /* Current scanline       */
byte VDPData;                      /* VDP data buffer        */
byte PLatch;                       /* Palette buffer         */
byte ALatch;                       /* Address buffer         */
int  Palette[16];                  /* Current palette        */

/** Places in DiskROM to be patched with ED FE C9 ************/
word DiskPatches[] = { 0x4010,0x4013,0x4016,0x401C,0x401F,0 };

/** Places in BIOS to be patched with ED FE C9 ***************/
word BIOSPatches[] = { 0x00E1,0x00E4,0x00E7,0x00EA,0x00ED,0x00F0,0x00F3,0 };

/** Screen Mode Handlers [number of screens + 1] *************/
void (*RefreshLine[MAXSCREEN+2])(byte Y) =
{
  RefreshLine0,   /* SCR 0:  TEXT 40x24  */
  RefreshLine1,   /* SCR 1:  TEXT 32x24  */
  RefreshLine2,   /* SCR 2:  BLK 256x192 */
  RefreshLine3,   /* SCR 3:  64x48x16    */
  RefreshLine4,   /* SCR 4:  BLK 256x192 */
  RefreshLine5,   /* SCR 5:  256x192x16  */
  RefreshLine6,   /* SCR 6:  512x192x4   */
  RefreshLine7,   /* SCR 7:  512x192x16  */
  RefreshLine8,   /* SCR 8:  256x192x256 */
  0,              /* SCR 9:  NONE        */
  RefreshLine10,  /* SCR 10: YAE 256x192 */
  RefreshLine10,  /* SCR 11: YAE 256x192 */
  RefreshLine12,  /* SCR 12: YJK 256x192 */
  RefreshLineTx80 /* SCR 0:  TEXT 80x24  */
};

/** VDP Address Register Masks *******************************/
struct { byte R2,R3,R4,R5,M2,M3,M4,M5; } MSK[MAXSCREEN+2] =
{
  { 0x7F,0x00,0x3F,0x00,0x00,0x00,0x00,0x00 }, /* SCR 0:  TEXT 40x24  */
  { 0x7F,0xFF,0x3F,0xFF,0x00,0x00,0x00,0x00 }, /* SCR 1:  TEXT 32x24  */
  { 0x7F,0x80,0x3C,0xFF,0x00,0x7F,0x03,0x00 }, /* SCR 2:  BLK 256x192 */
  { 0x7F,0x00,0x3F,0xFF,0x00,0x00,0x00,0x00 }, /* SCR 3:  64x48x16    */
  { 0x7F,0x80,0x3C,0xFC,0x00,0x7F,0x03,0x03 }, /* SCR 4:  BLK 256x192 */
  { 0x60,0x00,0x00,0xFC,0x1F,0x00,0x00,0x03 }, /* SCR 5:  256x192x16  */
  { 0x60,0x00,0x00,0xFC,0x1F,0x00,0x00,0x03 }, /* SCR 6:  512x192x4   */
  { 0x20,0x00,0x00,0xFC,0x1F,0x00,0x00,0x03 }, /* SCR 7:  512x192x16  */
  { 0x20,0x00,0x00,0xFC,0x1F,0x00,0x00,0x03 }, /* SCR 8:  256x192x256 */
  { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 }, /* SCR 9:  NONE        */
  { 0x20,0x00,0x00,0xFC,0x1F,0x00,0x00,0x03 }, /* SCR 10: YAE 256x192 */
  { 0x20,0x00,0x00,0xFC,0x1F,0x00,0x00,0x03 }, /* SCR 11: YAE 256x192 */
  { 0x20,0x00,0x00,0xFC,0x1F,0x00,0x00,0x03 }, /* SCR 12: YJK 256x192 */
  { 0x7C,0xF8,0x3F,0x00,0x03,0x07,0x00,0x00 }  /* SCR 0:  TEXT 80x24  */
};

/** MegaROM Mapper Names *************************************/
char *ROMNames[MAXMAPPERS+1] =
{
  "GENERIC/8kB","GENERIC/16kB","KONAMI5/8kB",
  "KONAMI4/8kB","ASCII/8kB","ASCII/16kB",
  "GMASTER2/SRAM","UNKNOWN"
};

/** Internal Functions ***************************************/
/** These functions are defined and internally used by the  **/
/** code in MSX.c.                                          **/
/*************************************************************/
byte *LoadROM(const char *Name,int Size,byte *Buf);
int  LoadCart(const char *Name,int Slot);
int  GuessROM(const byte *Buf,int Size);
void SetMegaROM(int Slot,byte P0,byte P1,byte P2,byte P3);
void MapROM(word A,byte V);       /* Switch MegaROM banks            */
void SSlot(byte V);               /* Switch secondary slots          */
void VDPOut(byte R,byte V);       /* Write value into a VDP register */
void Printer(byte V);             /* Send a character to a printer   */
void PPIOut(byte New,byte Old);   /* Set PPI bits (key click, etc.)  */
void CheckSprites(void);          /* Check collisions and 5th sprite */
byte RTCIn(byte R);               /* Read RTC registers              */
byte SetScreen(void);             /* Change screen mode              */
word SetIRQ(byte IRQ);            /* Set/Reset IRQ                   */
word StateID(void);               /* Compute emulation state ID      */

# if !defined(CAANOO_MODE)
void
myPowerSetClockFrequency(int cpu_clock)
{
  if (MSX.msx_current_clock != cpu_clock) {
    gp2xPowerSetClockFrequency(cpu_clock);
    MSX.msx_current_clock = cpu_clock;
  }
}
# endif

//LUDO:
void
InitAllMSXVariable(void)
{
  Verbose     = 0;              /* Debug msgs ON/OFF      */
  VPeriod     = CPU_VPERIOD;    /* CPU cycles per VBlank  */
  HPeriod     = CPU_HPERIOD;    /* CPU cycles per HBlank  */
  SaveCMOS    = 0;              /* Save CMOS.ROM on exit  */
  SaveSRAM    = 0;              /* ~ GMASTER2.RAM on exit */
//LUDO:
//  MSX.msx_version  = 2;              /* 0=MSX1,1=MSX2,2=MSX2+  */
  JoyTypeA    = 1;              /* 0=None,1=Joystick,     */
  JoyTypeB    = 1;              /* 2=MouseAsJstk,3=Mouse  */
  ROMTypeA    = MSX.msx_megarom_type; // Sergi
  ROMTypeB    = MSX.msx_megarom_type; // Sergi
  //ROMTypeA    = MAXMAPPERS;     /* MegaROM types          */
  //ROMTypeB    = MAXMAPPERS;
//  MSX.msx_ram_pages    = 16;              /* Number of RAM pages    */
  VRAMPages   = 8;              /* Number of VRAM pages   */
  AutoFire    = 0;              /* Autofire on [SPACE]    */
  UseDrums    = 0;              /* Use drms for PSG noise */
  ExitNow     = 0;              /* 1 = Exit the emulator  */

  memset(&CPU, 0, sizeof(Z80));
  memset(EnWrite,0,4);
  memset(PSL,0,4);
  memset(SSL,0,4);
  PSLReg  = 0;
  SSLReg  = 0;
  CCount = 0;
  UseFont = 0;
  KanLetter = 0;
  KanCount = 0;

  memset(KeyMap,0,16);
  memset(Buttons,0,2);
  memset(MouseDX,0,2);
  memset(MouseDY,0,2);
  memset(OldMouseX,0,2);
  memset(OldMouseY,0,2);
  memset(MCount,0,2);
  memset(&PPI, 0, sizeof(I8255));
  IOReg = 0;
  memset(&PSG,0, sizeof(AY8910));
  memset(&OPLL,0, sizeof(YM2413));
  memset(&SCChip,0, sizeof(SCC));
  memset(SCCOn,0, 2);
  memset(&SIO, 0, sizeof(I8251));
  RTCReg = 0; RTCMode = 0;
  memset(RTC, 0, 4 * 13);
  ChrGenM = ChrTabM = ColTabM = 0;
  SprTabM = 0;
  VAddr = 0;
  VKey = PKey = WKey = 0;
  FGColor = 0; BGColor = 0;
  XFGColor = XBGColor = 0;
  ScrMode  =0;
  memset(VDP, 0, 64);
  memset(VDPStatus, 0, 16);
  IRQPending = 0;
  ScanLine = 0;
  VDPData = 0;
  PLatch = 0;
  ALatch = 0;
  memset(Palette, 0, 16);
  RCounter = 0;
}

/** StartMSX() ***********************************************/
/** Allocate memory, load ROM images, initialize hardware,  **/
/** CPU and start the emulation. This function returns 0 in **/
/** the case of failure.                                    **/
/*************************************************************/
int
InitMSX(void)
{
  /*** MSX versions: ***/
  static const char *Versions[] = { "MSX","MSX2","MSX2+" };

  /*** Joystick types: ***/
  static const char *JoyTypes[] =
  {
    "nothing","normal joystick",
    "mouse in joystick mode","mouse in real mode"
  };

  /*** CMOS ROM default values: ***/
  static const byte RTCInit[4][13]  =
  {
    {  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    {  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    {  0, 0, 0, 0,40,80,15, 4, 4, 0, 0, 0, 0 },
    {  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
  };

  /*** VDP status register states: ***/
  static const byte VDPSInit[16] = { 0x9F,0,0x6C,0,0,0,0,0,0,0,0,0,0,0,0,0 };

  /*** VDP control register states: ***/
  static const byte VDPInit[64]  =
  {
    0x00,0x10,0xFF,0xFF,0xFF,0xFF,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
  };

  /*** Initial palette: ***/
  static const byte PalInit[16][3] =
  {
    {0x00,0x00,0x00},{0x00,0x00,0x00},{0x20,0xC0,0x20},{0x60,0xE0,0x60},
    {0x20,0x20,0xE0},{0x40,0x60,0xE0},{0xA0,0x20,0x20},{0x40,0xC0,0xE0},
    {0xE0,0x20,0x20},{0xE0,0x60,0x60},{0xC0,0xC0,0x20},{0xC0,0xC0,0x80},
    {0x20,0x80,0x20},{0xC0,0x40,0xA0},{0xA0,0xA0,0xA0},{0xE0,0xE0,0xE0}
  };

  int *T,I,J,K;
  byte *P;
  word A;
  FILE *F;

  /* LUDO: REALLY  Zero everyting */
  InitAllMSXVariable();

  CasStream=PrnStream=ComIStream=ComOStream=NULL;
  ROMData[0]=ROMData[1]=NULL;
  FontBuf  = NULL;
  VRAM     = NULL;
  SRAM     = NULL;
  Kanji    = NULL;
  SaveCMOS = 0;
  SaveSRAM = 0;
  ExitNow  = 0;
  CCount   = 0;

  /* Calculate IPeriod from VPeriod/HPeriod */
  if(HPeriod<CPU_HPERIOD) HPeriod=CPU_HPERIOD;
  if(VPeriod<CPU_VPERIOD) VPeriod=CPU_VPERIOD;
  CPU.TrapBadOps = Verbose&0x10;
  CPU.IPeriod    = CPU_H240;
  CPU.IAutoReset = 0;

  /* Check parameters for validity */
  if(MSX.msx_version>2) MSX.msx_version=2;
  if((MSX.msx_ram_pages<(MSX.msx_version? 8:4))||(MSX.msx_ram_pages>256))
    MSX.msx_ram_pages=MSX.msx_version? 8:4;
  if((VRAMPages<(MSX.msx_version? 8:2))||(VRAMPages>8))
    VRAMPages=MSX.msx_version? 8:2;

  /* Number of RAM pages should be power of 2 */
  /* Calculate RAMMask=(2^MSX.msx_ram_pages)-1 */
  for(J=1;J<MSX.msx_ram_pages;J<<=1);
  MSX.msx_ram_pages=J;
  RAMMask=J-1;

  /* Number of VRAM pages should be a power of 2 */
  for(J=1;J<VRAMPages;J<<=1);
  VRAMPages=J;

  /* Initialize ROMMasks to zeroes for now */
  ROMMask[0]=ROMMask[1]=0;

  /* Joystick types are in 0..3 range */
  JoyTypeA&=3;
  JoyTypeB&=3;

  /* Allocate 16kB for the empty space (scratch RAM) */
  if(Verbose) printf("Allocating 16kB for empty space...");
  if (!EmptyRAM) {
    if(!(EmptyRAM=malloc(0x4000))) { PRINTFAILED;return(0); }
  }
  memset(EmptyRAM,NORAM,0x4000);
  Chunks[CCount++]=EmptyRAM;

  /* Reset memory map to the empty space */
  for(I=0;I<4;I++)
    for(J=0;J<4;J++)
      for(K=0;K<8;K++)
        MemMap[I][J][K]=EmptyRAM;

  /* Allocate VRAMPages*16kB for VRAM */
  if(Verbose) printf("OK\nAllocating %dkB for VRAM...",VRAMPages*16);
  if (! VRAM) {
    if(!(VRAM=malloc(VRAMPages*0x4000))) { PRINTFAILED;return(0); }
  } else {
    if(!(VRAM=realloc(VRAM, VRAMPages*0x4000))) { PRINTFAILED;return(0); }
  }
  memset(VRAM,0x00,VRAMPages*0x4000);
  Chunks[CCount++]=VRAM;

  /* Allocate MSX.msx_ram_pages*16kB for RAM */
  if(Verbose) printf("OK\nAllocating %dx16kB RAM pages...",MSX.msx_ram_pages);
  if (! RAMData) {
    if(!(RAMData=malloc(MSX.msx_ram_pages*0x4000))) { PRINTFAILED;return(0); }
  } else {
    if(!(RAMData=realloc(RAMData, MSX.msx_ram_pages*0x4000))) { PRINTFAILED;return(0); }
  }
  memset(RAMData,NORAM,MSX.msx_ram_pages*0x4000);
  Chunks[CCount++]=RAMData;

  if(Verbose) printf("OK\nLoading %s ROMs:\n",Versions[MSX.msx_version]);

  /* Open/load system ROM(s) */
  switch(MSX.msx_version)
  {
    case 0:
      if(Verbose) printf("  Opening msx.rom...");
      P=LoadROM("msx.rom",0x8000,0);
      PRINTRESULT(P);
      if(!P) return(0);
      MemMap[0][0][0]=P;
      MemMap[0][0][1]=P+0x2000;
      MemMap[0][0][2]=P+0x4000;
      MemMap[0][0][3]=P+0x6000;
      break;

    case 1:
      if(Verbose) printf("  Opening msx2.rom...");
      P=LoadROM("msx2.rom",0x8000,0);
      PRINTRESULT(P);
      if(!P) return(0);
      MemMap[0][0][0]=P;
      MemMap[0][0][1]=P+0x2000;
      MemMap[0][0][2]=P+0x4000;
      MemMap[0][0][3]=P+0x6000;
      if(Verbose) printf("  Opening msx2ext.rom...");
      P=LoadROM("msx2ext.rom",0x4000,0);
      PRINTRESULT(P);
      if(!P) return(0);
      MemMap[3][1][0]=P;
      MemMap[3][1][1]=P+0x2000;
      break;

    case 2:
      if(Verbose) printf("  Opening msx2p.rom...");
      P=LoadROM("msx2p.rom",0x8000,0);
      PRINTRESULT(P);
      if(!P) return(0);
      MemMap[0][0][0]=P;
      MemMap[0][0][1]=P+0x2000;
      MemMap[0][0][2]=P+0x4000;
      MemMap[0][0][3]=P+0x6000;
      if(Verbose) printf("  Opening msx2pext.rom...");
      P=LoadROM("msx2pext.rom",0x4000,0);
      PRINTRESULT(P);
      if(!P) return(0);
      MemMap[3][1][0]=P;
      MemMap[3][1][1]=P+0x2000;
      break;
  }

  /* Try loading DiskROM */
  if(skip_disk_rom != 1) /* Skip disk.rom if tape is used */
  { 
    if(P=LoadROM("disk.rom",0x4000,0))
    {
      if(Verbose) puts("  Opening disk.rom...OK");
      MemMap[3][1][2]=P;
      MemMap[3][1][3]=P+0x2000;
    }
  }

  /* Apply patches to BIOS */
  if(Verbose) printf("  Patching BIOS: ");
  for(J=0;BIOSPatches[J];J++)
  {
    if(Verbose) printf("%04X..",BIOSPatches[J]);
    P=MemMap[0][0][0]+BIOSPatches[J];
    P[0]=0xED;P[1]=0xFE;P[2]=0xC9;
  }
  if(Verbose) printf("OK\n");

  /* Apply patches to BDOS */
  if(MemMap[3][1][2]!=EmptyRAM)
  {
    if(Verbose) printf("  Patching BDOS: ");
    for(J=0;DiskPatches[J];J++)
    {
      if(Verbose) printf("%04X..",DiskPatches[J]);
      P=MemMap[3][1][2]+DiskPatches[J]-0x4000;
      P[0]=0xED;P[1]=0xFE;P[2]=0xC9;
    }
    if(Verbose) printf("OK\n");
  }

  if(Verbose) printf("Loading other ROMs: ");

  /* Try loading CMOS memory contents */
  if(LoadROM("cmos.rom",sizeof(RTC),(byte *)RTC))
  { if(Verbose) printf("cmos.rom.."); }
  else memcpy(RTC,RTCInit,sizeof(RTC));

  /* Try loading Kanji alphabet ROM */
  if(Kanji=LoadROM("kanji.rom",0x20000,0))
  { if(Verbose) printf("kanji.rom.."); }

  /* Try loading RS232 support ROM */
  if(P=LoadROM("rs232.rom..",0x4000,0))
  {
    if(Verbose) printf("rs232.rom..");
    MemMap[3][0][2]=P;
    MemMap[3][0][3]=P+0x2000;
  }

  /* Try loading FM-PAC support ROM */
  if(FMPACROM=LoadROM("fmpac.rom",0x4000,0))
  {
    if(Verbose) printf("fmpac.rom..");
    MemMap[3][3][2]=(Use2413?FMPACROM:EmptyRAM);
    MemMap[3][3][3]=(Use2413?FMPACROM+0x2000:EmptyRAM);
  }
  else
  {
    FMPACROM=EmptyRAM;
  }

  if(Verbose) printf("OK\n");

  /* Loading cartridge into slot A... */
  K=LoadCart(CartA,0);
  J=(ROMMask[0]+1)*8192;

  /* Guess mapper is not given */
  if((ROMTypeA==-1)&&(J>0x8000))
  {
    ROMTypeA=GuessROM(ROMData[0],K);
    if(Verbose) printf("  Cartridge A: Guessed %s\n",ROMNames[ROMTypeA]);
  }

  /* For Generic/16kB carts, set ROM pages as 0:1:N-2:N-1 */
  if((ROMTypeA==1)&&(J>0x8000)) SetMegaROM(0,0,1,ROMMask[0]-1,ROMMask[0]);

  /* For ASCII/8kB carts, set ROM pages as 0:0:0:0 */
  if((ROMTypeA==4)&&(J>0x8000)) SetMegaROM(0,0,0,0,0);

  /* For ASCII/16kB carts, set ROM pages as 0:1:0:1 */
  if((ROMTypeA==5)&&(J>0x8000)) SetMegaROM(0,0,1,0,1);

  /* For RTYPE carts, set ROM pages as 2E:2F:0:1 */
  if((ROMTypeA==7)&&(J>0x8000)) SetMegaROM(0,0x2E,0x2F,0,1);

  /* Loading cartridge into slot B... */
  K=LoadCart(CartB,1);
  J=(ROMMask[1]+1)*8192;

  /* Guess mapper is not given */
  if((ROMTypeB==-1)&&(J>0x8000))
  {
    ROMTypeB=GuessROM(ROMData[1],K);
    if(Verbose) printf("  Cartridge B: Guessed %s\n",ROMNames[ROMTypeB]);
  }

  /* For Generic/16kB carts, set ROM pages as 0:1:N-2:N-1 */
  if((ROMTypeB==1)&&(J>0x8000)) SetMegaROM(1,0,1,ROMMask[1]-1,ROMMask[1]);

  /* For ASCII/8kB carts, set ROM pages as 0:0:0:0 */
  if((ROMTypeB==4)&&(J>0x8000)) SetMegaROM(1,0,0,0,0);

  /* For ASCII/16kB carts, set ROM pages as 0:1:0:1 */
  if((ROMTypeB==5)&&(J>0x8000)) SetMegaROM(1,0,1,0,1);

  /* For RTYPE carts, set ROM pages as 2E:2F:0:1 */
  if((ROMTypeA==7)&&(J>0x8000)) SetMegaROM(1,0x2E,0x2F,0,1);

  /* For GameMaster2, allocate and load SRAM */
  if((ROMTypeA==6)||(ROMTypeB==6))
  {
    if(Verbose) printf("  Allocating 16kB for GameMaster2 SRAM...");
    if (!SRAM) {
      if(!(SRAM=malloc(0x4000))) { PRINTFAILED; return 0; }
    }

    /* Erase SRAM and add it to the list of mallocs */
    memset(SRAM,NORAM,0x4000);
    Chunks[CCount++]=SRAM;
    /* Try loading SRAM data form disk */
    if(!(F=fopen("GMASTER2.RAM","rb"))) { PRINTOK; }
    else
    {
      if(Verbose) printf("loading GMASTER2.RAM...");
      if(fread(SRAM,1,0x2000,F)!=0x2000) { PRINTFAILED; }
      else
      {
        /* Mirror GameMaster2 SRAM as needed */
        memcpy(SRAM+0x2000,SRAM+0x1000,0x1000);
        memcpy(SRAM+0x3000,SRAM+0x1000,0x1000);
        memcpy(SRAM+0x1000,SRAM,0x1000);
        PRINTOK;
      }
      /* Done with the file */
      fclose(F);
    }

  }

  /* Load MSX2-dependent cartridge ROMs: MSXDOS2 and PAINTER */
  if(MSX.msx_version>0)
  {
    /* Find an empty slot */
    if(MemMap[1][0][2]==EmptyRAM) J=1;
    else if(MemMap[2][0][2]==EmptyRAM) J=2;
         else J=0;

    /* Try loading MSXDOS2 cartridge, if slot */
    /* found and DiskROM present              */
    if(J&&(MemMap[3][1][2]!=EmptyRAM))
    {
      if(J==2) ROMTypeB=1; else ROMTypeA=1;
      if(LoadCart("msxdos2.rom",J-1))
        SetMegaROM(J-1,0,1,ROMMask[J-1]-1,ROMMask[J-1]);
    }

    /* Find an empty slot */
    if(MemMap[1][0][2]==EmptyRAM) J=1;
    else if(MemMap[2][0][2]==EmptyRAM) J=2;
         else J=0;

    /* Try loading PAINTER ROM if slot found */
    if(J) LoadCart("painter.rom",J-1);
  }

  /* Try loading font */
  if(Verbose) printf("Loading %s font...",FontName);
  FontBuf=LoadROM(FontName,0x800,0);
  PRINTRESULT(FontBuf);

  /* Open stream for a printer */
  if(!PrnName) PrnStream=stdout;
  else
  {
    if(Verbose) printf("Redirecting printer output to %s...",PrnName);
    if(!(PrnStream=fopen(PrnName,"wb"))) PrnStream=stdout;
    PRINTRESULT(PrnStream!=stdout);
  }

  /* Open streams for serial IO */
  if(!ComName) { ComIStream=stdin;ComOStream=stdout; }
  else
  {
    if(Verbose) printf("Redirecting serial I/O to %s...",ComName);
    if(!(ComOStream=ComIStream=fopen(ComName,"r+b")))
    { ComIStream=stdin;ComOStream=stdout; }
    PRINTRESULT(ComOStream!=stdout);
  }

  /* Open disk images */
  if(ChangeDisk(0,DiskA)) {
    if(Verbose) printf("Inserting %s into drive A\n",DiskA);
  }
  if(ChangeDisk(1,DiskB)) {
    if(Verbose) printf("Inserting %s into drive B\n",DiskB);
  }

  /* Open casette image */
  if(CasName)
    if(CasStream=fopen(CasName,"r+b")) {
      if(Verbose) printf("Using %s as a tape\n",CasName);
    }

  if(Verbose)
  {
    printf("Attaching %s to joystick port A\n",JoyTypes[JoyTypeA]);
    printf("Attaching %s to joystick port B\n",JoyTypes[JoyTypeB]);
    printf("Initializing memory mappers...");
  }

  for(J=0;J<4;J++)
  {
    EnWrite[J]=0;                      /* Write protect ON for all slots */
    PSL[J]=SSL[J]=0;                   /* PSL=0:0:0:0, SSL=0:0:0:0       */
    MemMap[3][2][J*2]   = RAMData+(3-J)*0x4000;        /* RAMMap=3:2:1:0 */
    MemMap[3][2][J*2+1] = MemMap[3][2][J*2]+0x2000;
    RAMMapper[J]        = 3-J;
    RAM[J*2]            = MemMap[0][0][J*2];           /* Setting RAM    */
    RAM[J*2+1]          = MemMap[0][0][J*2+1];
  }

  if(Verbose) printf("OK\nInitializing VDP, PSG, OPLL, SCC, and CPU...");

  /* Initialize palette */
  for(J=0;J<16;J++)
  {
    Palette[J]=RGB2INT(PalInit[J][0],PalInit[J][1],PalInit[J][2]);
    SetColor(J,PalInit[J][0],PalInit[J][1],PalInit[J][2]);
  }

  /* Reset mouse coordinates/counters */
  for(J=0;J<2;J++)
    Buttons[J]=MouseDX[J]=MouseDY[J]=OldMouseX[J]=OldMouseY[J]=MCount[J]=0;

  /* Reset serial I/O */
  Reset8251(&SIO,ComIStream,ComOStream);

  /* Reset PPI chips and slot selectors */
  Reset8255(&PPI);
  PPI.Rout[0]=PSLReg=0x00;
  PPI.Rout[2]=IOReg=0x00;
  SSLReg=0x00;

  memcpy(VDP,VDPInit,sizeof(VDP));
  memcpy(VDPStatus,VDPSInit,sizeof(VDPStatus));
  memset(KeyMap,0xFF,16);               /* Keyboard         */
  IRQPending=0x00;                      /* No IRQs pending  */
  SCCOn[0]=SCCOn[1]=0;                  /* SCCs off for now */
  RTCReg=RTCMode=0;                     /* Clock registers  */
  KanCount=0;KanLetter=0;               /* Kanji extension  */
  ChrTab=ColTab=ChrGen=VRAM;            /* VDP tables       */
  SprTab=SprGen=VRAM;
  ChrTabM=ColTabM=ChrGenM=SprTabM=~0;   /* VDP addr. masks  */
  VPAGE=VRAM;                           /* VRAM page        */
  FGColor=BGColor=XFGColor=XBGColor=0;  /* VDP colors       */
  ScrMode=0;                            /* Screen mode      */
  VKey=PKey=1;WKey=0;                   /* VDP keys         */
  VAddr=0x0000;                         /* VRAM access addr */
  ScanLine=0;                           /* Current scanline */
  VDPData=NORAM;                        /* VDP data buffer  */
  UseFont=0;                            /* Extern. font use */

  /* Set "V9958" VDP version for MSX2+ */
  if(MSX.msx_version>=2) VDPStatus[1]|=0x04;

  /* Reset soundchips */
  ResetSound();

  /* Reset CPU */
  ResetZ80();

  /* Done with initialization */
  if(Verbose)
  {
    printf("OK\n  %d CPU cycles per HBlank\n",HPeriod);
    printf("  %d CPU cycles per VBlank\n",VPeriod);
    printf("  %d scanlines\n",VPeriod/HPeriod);
  }

  /* If no state name, generate name automatically */
  if(!StateName)
  {
    /* Find maximal StateName length */
    I=CartA? strlen(CartA):0;
    J=CartB? strlen(CartB):0;
    J=J>I? J:I;
    J=J>4? J:4;
    J+=4;
    /* If memory for StateName gets allocated... */
    if(StateName=malloc(J))
    {
      /* Add to the list of chunks to remove */
      Chunks[CCount++]=StateName;
      /* Copy name */
      strcpy(StateName,CartA? CartA:CartB? CartB:"fMSX");
      /* Find extension */
      P=strrchr(StateName,'.');
      /* Either replace extension with ".sta" or add ".sta" */
      if(P) strcpy(P,".sta"); else strcat(StateName,".sta");
    }
  }

  /* Try loading emulation state */
  if(StateName)
  {
    if(Verbose) printf("Loading state from %s...",StateName);
    J=LoadState(StateName);
    PRINTRESULT(J);
  }

  return(1);
}

int
StartMSX(void)
{
  int A;

  /* Start execution of the code */
  if(Verbose) printf("RUNNING ROM CODE...\n");
  A = RunZ80();

  /* Exiting emulation... */
  if(Verbose) printf("EXITED at PC = %04Xh.\n",A);
  return(1);
}

/** TrashMSX() ***********************************************/
/** Free resources allocated by StartMSX().                 **/
/*************************************************************/
void
TrashMSX(void)
{
  int J;
  FILE *F;

  /* CMOS.ROM and GMASTER2.RAM are saved in the program directory */
  /* Save CMOS RAM, if present */
  if(SaveCMOS)
  {
    if(Verbose) printf("Writing cmos.rom...");
    if(!(F=fopen("cmos.rom","wb"))) SaveCMOS=0;
    else
    {
      if(fwrite(RTC,1,sizeof(RTC),F)!=sizeof(RTC)) SaveCMOS=0;
      fclose(F);
    }
    PRINTRESULT(SaveCMOS);
  }

  /* Save GameMaster2 SRAM, if present */
  if(SaveSRAM)
  {
    if(Verbose) printf("Writing GMASTER.RAM...");
    if(!(F=fopen("GMASTER2.RAM","wb"))) SaveSRAM=0;
    else
    {
      if(fwrite(SRAM,1,0x1000,F)!=0x1000)        SaveSRAM=0;
      if(fwrite(SRAM+0x2000,1,0x1000,F)!=0x1000) SaveSRAM=0;
      fclose(F);
    }
    PRINTRESULT(SaveSRAM);
  }

  /* Change back to working directory */
  /* Shut down sound logging */
  TrashMIDI();

  /* Free alocated memory */
  for(J=0;J<CCount;J++) free(Chunks[J]);

  /* Close all IO streams */
  if(PrnStream&&(PrnStream!=stdout))   fclose(PrnStream);
  if(ComOStream&&(ComOStream!=stdout)) fclose(ComOStream);
  if(ComIStream&&(ComIStream!=stdin))  fclose(ComIStream);
  if(CasStream) fclose(CasStream);

  /* Close disk images, if present */
  for(J=0;J<MAXDRIVES;J++) ChangeDisk(J,0);
}

/** RdZ80() **************************************************/
/** Z80 emulation calls this function to read a byte from   **/
/** address A of Z80 address space. Now moved to Z80.c and  **/
/** made inlined to speed things up.                        **/
/*************************************************************/
#ifndef FMSX
byte RdZ80(word A)
{
  if(A!=0xFFFF) return(RAM[A>>13][A&0x1FFF]);
  else return(PSL[3]==3? ~SSLReg:RAM[7][0x1FFF]);
}
#endif

/** WrZ80() **************************************************/
/** Z80 emulation calls this function to write byte V to    **/
/** address A of Z80 address space.                         **/
/*************************************************************/
void WrZ80(word A,byte V)
{
  if(A!=0xFFFF)
  {
    if(EnWrite[A>>14]) RAM[A>>13][A&0x1FFF]=V;
    else if((A>0x3FFF)&&(A<0xC000)) MapROM(A,V);
  }
  else
  {
    if(PSL[3]==3) SSlot(V);
    else if(EnWrite[3]) RAM[7][0x1FFF]=V;
  }
}

/** InZ80() **************************************************/
/** Z80 emulation calls this function to read a byte from   **/
/** a given I/O port.                                       **/
/*************************************************************/
byte InZ80(word Port)
{
  /* MSX only uses 256 IO ports */
  Port&=0xFF;

  /* Return an appropriate port value */
  switch(Port)
  {

case 0x04: if(Use8950) return 2; else return(NORAM);
case 0x05: if(Use8950) return 0; else return(NORAM);
case 0xC0: if(Use8950) return ReadAUDIO(0); else return(NORAM);
case 0xC1: if(Use8950) return ReadAUDIO(1); else return(NORAM);

case 0x90: return(0xFD);                   /* Printer READY signal */
case 0xB5: return(RTCIn(RTCReg));          /* RTC registers        */

case 0xA8: /* Primary slot state   */
case 0xA9: /* Keyboard port        */
case 0xAA: /* General IO register  */
case 0xAB: /* PPI control register */
  PPI.Rin[1]=KeyMap[PPI.Rout[2]&0x0F];
  return(Read8255(&PPI,Port-0xA8));

case 0xFC: /* Mapper page at 0000h */
case 0xFD: /* Mapper page at 4000h */
case 0xFE: /* Mapper page at 8000h */
case 0xFF: /* Mapper page at C000h */
  return(RAMMapper[Port-0xFC]|~RAMMask);

case 0xD9: /* Kanji support */
  Port=Kanji? Kanji[KanLetter+KanCount]:NORAM;
  KanCount=(KanCount+1)&0x1F;
  return(Port);

case 0x80: /* SIO data */
case 0x81:
case 0x82:
case 0x83:
case 0x84:
case 0x85:
case 0x86:
case 0x87:
  return(NORAM);
  /*return(Rd8251(&SIO,Port&0x07));*/

case 0x98: /* VRAM read port */
  /* Read from VRAM data buffer */
  Port=VDPData;
  /* Reset VAddr latch sequencer */
  VKey=1;
  /* Fill data buffer with a new value */
  VDPData=VPAGE[VAddr];
  /* Increment VRAM address */
  VAddr=(VAddr+1)&0x3FFF;
  /* If rolled over, modify VRAM page# */
  if(!VAddr&&(ScrMode>3))
  {
    VDP[14]=(VDP[14]+1)&(VRAMPages-1);
    VPAGE=VRAM+((int)VDP[14]<<14);
  }
  return(Port);

case 0x99: /* VDP status registers */
  /* Read an appropriate status register */
  Port=VDPStatus[VDP[15]];
  /* Reset VAddr latch sequencer */
  VKey=1;
  /* Update status register's contents */
  switch(VDP[15])
  {
    case 0: VDPStatus[0]&=0x5F;SetIRQ(~INT_IE0);break;
    case 1: VDPStatus[1]&=0xFE;SetIRQ(~INT_IE1);break;
    case 7: VDPStatus[7]=VDP[44]=VDPRead();break;
  }
  /* Return the status register value */
  return(Port);

case 0xA2: /* PSG input port */
  /* PSG[14] returns joystick/mouse data */
  if(PSG.Latch==14)
  {
    int DX,DY,L;

    /* Number of a joystick port */
    Port=PSG.R[15]&0x40? 1:0;
    L=Port? JoyTypeB:JoyTypeA;

    /* If no joystick, return dummy value */
    if(!L) return(0x7F);

    /* @@@ For debugging purposes */
    /*printf("Reading from PSG[14]: MCount[%d]=%d PSG[15]=%02Xh Value=",Port,MCount[Port],PSG.R[15]);*/

    /* Poll mouse position, if needed */
    if((L==2)||(MCount[Port]==1))
    {
      /* Read new mouse coordinates */
      L=Mouse(Port);
      Buttons[Port]=(~L>>12)&0x30;
      DY=(L>>8)&0xFF;
      DX=L&0xFF;

      /* Compute offsets and store coordinates  */
      L=OldMouseX[Port]-DX;OldMouseX[Port]=DX;DX=L;
      L=OldMouseY[Port]-DY;OldMouseY[Port]=DY;DY=L;

      /* Adjust offsets */
      MouseDX[Port]=(DX>127? 127:(DX<-127? -127:DX))&0xFF;
      MouseDY[Port]=(DY>127? 127:(DY<-127? -127:DY))&0xFF;
    }

    /* Determine return value */
    switch(MCount[Port])
    {
      case 0: /* Normal joystick */
        if(PSG.R[15]&(Port? 0x20:0x10)) Port=0x3F;
        else
          if((Port? JoyTypeB:JoyTypeA)<2) Port=~Joystick(Port)&0x3F;
          else Port=Buttons[Port]
                   |(MouseDX[Port]? (MouseDX[Port]<128? 0x08:0x04):0x0C)
                   |(MouseDY[Port]? (MouseDY[Port]<128? 0x02:0x01):0x03);
        break;

      case 1: Port=(MouseDX[Port]>>4)|Buttons[Port];break;
      case 2: Port=(MouseDX[Port]&0x0F)|Buttons[Port];break;
      case 3: Port=(MouseDY[Port]>>4)|Buttons[Port];break;
      case 4: Port=(MouseDY[Port]&0x0F)|Buttons[Port];break;
    }

    /* @@@ For debugging purposes */
    /*printf("%02Xh\n",Port|0x40);*/

    /* 6th bit is always 1 */
    return(Port|0x40);
  }

  /* PSG[15] resets mouse counters */
  if(PSG.Latch==15)
  {
    /* @@@ For debugging purposes */
    /*printf("Reading from PSG[15]\n");*/

    /*MCount[0]=MCount[1]=0;*/
    return(PSG.R[15]&0xF0);
  }

  /* Return PSG[0-13] as they are */
  return ReadPSG(PSG.Latch);
}

  /* Return NORAM for non-existing ports */
  return(NORAM);
}

/** OutZ80() *************************************************/
/** Z80 emulation calls this function to write byte V to a  **/
/** given I/O port.                                         **/
/*************************************************************/
void OutZ80(word Port,byte Value)
{
  register byte I,J,K;

  Port&=0xFF;
  switch(Port)
  {

case 0x7C: OPLL.Latch=Value;return;               /* OPLL Register# */
case 0x7D: WriteOPLL(OPLL.Latch,Value);return;    /* OPLL Data      */
case 0xA0: PSG.Latch=Value;return;                /* PSG Register#  */
case 0xC0: WriteAUDIO(0,Value);return;            /* AUDIO Register#*/
case 0xC1: WriteAUDIO(1,Value);return;            /* AUDIO Data     */
case 0x91: Printer(Value);return;                 /* Printer Data   */
case 0xB4: RTCReg=Value&0x0F;return;              /* RTC Register#  */

case 0xD8: /* Upper bits of Kanji ROM address */
  KanLetter=(KanLetter&0x1F800)|((int)(Value&0x3F)<<5);
  KanCount=0;
  return;

case 0xD9: /* Lower bits of Kanji ROM address */
  KanLetter=(KanLetter&0x007E0)|((int)(Value&0x3F)<<11);
  KanCount=0;
  return;

case 0x80: /* SIO data */
case 0x81:
case 0x82:
case 0x83:
case 0x84:
case 0x85:
case 0x86:
case 0x87:
  return;
  /*Wr8251(&SIO,Port&0x07,Value);
  return;*/

case 0x98: /* VDP Data */
  VKey=1;
  if(WKey)
  {
    /* VDP set for writing */
    VDPData=VPAGE[VAddr]=Value;
    VAddr=(VAddr+1)&0x3FFF;
  }
  else
  {
    /* VDP set for reading */
    VDPData=VPAGE[VAddr];
    VAddr=(VAddr+1)&0x3FFF;
    VPAGE[VAddr]=Value;
  }
  /* If VAddr rolled over, modify VRAM page# */
  if(!VAddr&&(ScrMode>3))
  {
    VDP[14]=(VDP[14]+1)&(VRAMPages-1);
    VPAGE=VRAM+((int)VDP[14]<<14);
  }
  return;

case 0x99: /* VDP Address Latch */
  if(VKey) { ALatch=Value;VKey=0; }
  else
  {
    VKey=1;
    switch(Value&0xC0)
    {
      case 0x80:
        /* Writing into VDP registers */
        VDPOut(Value&0x3F,ALatch);
        break;
      case 0x00:
      case 0x40:
        /* Set the VRAM access address */
        VAddr=(((word)Value<<8)+ALatch)&0x3FFF;
        /* WKey=1 when VDP set for writing into VRAM */
        WKey=Value&0x40;
        /* When set for reading, perform first read */
        if(!WKey)
        {
          VDPData=VPAGE[VAddr];
          VAddr=(VAddr+1)&0x3FFF;
          if(!VAddr&&(ScrMode>3))
          {
            VDP[14]=(VDP[14]+1)&(VRAMPages-1);
            VPAGE=VRAM+((int)VDP[14]<<14);
          }
        }
        break;
    }
  }
  return;

case 0x9A: /* VDP Palette Latch */
  if(PKey) { PLatch=Value;PKey=0; }
  else
  {
    byte R,G,B;
    /* New palette entry written */
    PKey=1;
    J=VDP[16];
    /* Compute new color components */
    R=(PLatch&0x70)*255/112;
    G=(Value&0x07)*255/7;
    B=(PLatch&0x07)*255/7;
    /* Set new color for palette entry J */
    Palette[J]=RGB2INT(R,G,B);
    SetColor(J,R,G,B);
    /* Next palette entry */
    VDP[16]=(J+1)&0x0F;
  }
  return;

case 0x9B: /* VDP Register Access */
  J=VDP[17]&0x3F;
  if(J!=17) VDPOut(J,Value);
  if(!(VDP[17]&0x80)) VDP[17]=(J+1)&0x3F;
  return;

case 0xA1: /* PSG Data */
  /* PSG[15] is responsible for joystick/mouse */
  if(PSG.Latch==15)
  {
    /* @@@ For debugging purposes */
    /*printf("Writing PSG[15] <- %02Xh\n",Value);*/

    /* For mouse, update nibble counter      */
    /* For joystick, set nibble counter to 0 */
    if((Value&0x0C)==0x0C) MCount[1]=0;
    else if((JoyTypeB>2)&&((Value^PSG.R[15])&0x20))
           MCount[1]+=MCount[1]==4? -3:1;

    /* For mouse, update nibble counter      */
    /* For joystick, set nibble counter to 0 */
    if((Value&0x03)==0x03) MCount[0]=0;
    else if((JoyTypeA>2)&&((Value^PSG.R[15])&0x10))
           MCount[0]+=MCount[0]==4? -3:1;

    PSG.R[15]=Value;
    return;
  }

  WritePSG(PSG.Latch,Value);
  return;

case 0xB5: /* RTC Data */
  if(RTCReg<13)
  {
    /* J = register bank# now */
    J=RTCMode&0x03;
    /* Store the value */
    RTC[J][RTCReg]=Value;
    /* If CMOS modified, we need to save it */
    if(J>1) SaveCMOS=1;
    return;
  }
  /* RTC[13] is a register bank# */
  if(RTCReg==13) RTCMode=Value;
  return;

case 0xA8: /* Primary slot state   */
case 0xA9: /* Keyboard port        */
case 0xAA: /* General IO register  */
case 0xAB: /* PPI control register */
  /* Write to PPI */
  Write8255(&PPI,Port-0xA8,Value);
  /* If general I/O register has changed... */
  if(PPI.Rout[2]!=IOReg) { PPIOut(PPI.Rout[2],IOReg);IOReg=PPI.Rout[2]; }
  /* If primary slot state has changed... */
  if(PPI.Rout[0]!=PSLReg)
    for(J=0,PSLReg=Value=PPI.Rout[0];J<4;J++,Value>>=2)
    {
      PSL[J]=Value&3;I=J<<1;
      K=PSL[J]==3? SSL[J]:0;
      EnWrite[J]=(K==2)&&(MemMap[3][2][I]!=EmptyRAM);
      RAM[I]=MemMap[PSL[J]][K][I];
      RAM[I+1]=MemMap[PSL[J]][K][I+1];
    }
  /* Done */
  return;

case 0xFC: /* Mapper page at 0000h */
case 0xFD: /* Mapper page at 4000h */
case 0xFE: /* Mapper page at 8000h */
case 0xFF: /* Mapper page at C000h */
  J=Port-0xFC;
  Value&=RAMMask;
  if(RAMMapper[J]!=Value)
  {
    if(Verbose&0x08) printf("RAM-MAPPER: block %d at %Xh\n",Value,J*0x4000);
    I=J<<1;
    RAMMapper[J]      = Value;
    MemMap[3][2][I]   = RAMData+((int)Value<<14);
    MemMap[3][2][I+1] = MemMap[3][2][I]+0x2000;
    if((PSL[J]==3)&&(SSL[J]==2))
    {
      EnWrite[J] = 1;
      RAM[I]     = MemMap[3][2][I];
      RAM[I+1]   = MemMap[3][2][I+1];
    }
  }
  return;

  }
}

/** MapROM() *************************************************/
/** Switch ROM Mapper pages. This function is supposed to   **/
/** be called when ROM page registers are written to.       **/
/*************************************************************/
void MapROM(register word A,register byte V)
{
  byte I,J,K,*P;

/* @@@ For debugging purposes
printf("(%04Xh) = %02Xh at PC=%04Xh\n",A,V,CPU.PC.W);
*/

  /* J contains 16kB page number 0-3  */
  J=A>>14;

  /* I contains slot number 0/1  */
  if(PSL[J]==1) I=0;
  else if(PSL[J]==2) I=1;
       else return;

  /* K contains MegaROM type */
  K=I? ROMTypeB:ROMTypeA;

  if((A&0xFF00)==0x9800 || (A&0xFF00)==0xB800)
  {
        Write2212(A,V);
        return;
  }

  /* If no cartridge or no mapper, exit */
  if(!ROMData[I]||!ROMMask[I]) return;

  switch(K)
  {
    case 0: /*** Generic 8kB cartridges (Konami, etc.) ***/
      if((A<0x4000)||(A>0xBFFF)) return;
      J=(A-0x4000)>>13;
      /* Turn SCC on/off on writes to 8000h-9FFFh */
      if(J==2) SCCOn[I]=(V==0x3F)? 1:0;
      /* Switch ROM pages */
      V&=ROMMask[I];
      if(V!=ROMMapper[I][J])
      {
        RAM[J+2]=MemMap[I+1][0][J+2]=ROMData[I]+((int)V<<13);
        ROMMapper[I][J]=V;
      }
      break;

    case 1: /*** Generic 16kB cartridges (MSXDOS2, HoleInOneSpecial) ***/
      if((A<0x4000)||(A>0xBFFF)) return;
      J=(A&0x8000)>>14;
      V=(V<<1)&ROMMask[I];
      if(V!=ROMMapper[I][J])
      {
        RAM[J+2]=MemMap[I+1][0][J+2]=ROMData[I]+((int)V<<13);
        RAM[J+3]=MemMap[I+1][0][J+3]=RAM[J+2]+0x2000;
        ROMMapper[I][J]=V;
      }
      break;

    case 2: /*** KONAMI5 8kB cartridges ***/
      if((A<0x5000)||(A>0xB000)||((A&0x1FFF)!=0x1000)) return;
      J=(A-0x5000)>>13;
      /* Turn SCC on/off on writes to 9000h */
      if(J==2) SCCOn[I]=(V==0x3F)? 1:0;
      /* Switch ROM pages */
      V&=ROMMask[I];
      if(V!=ROMMapper[I][J])
      {
        RAM[J+2]=MemMap[I+1][0][J+2]=ROMData[I]+((int)V<<13);
        ROMMapper[I][J]=V;
      }
      break;

    case 3: /*** KONAMI4 8kB cartridges ***/
      /* Page at 4000h is fixed */
      if((A<0x6000)||(A>0xA000)||(A&0x1FFF)) return;
      J=(A-0x4000)>>13;
      V&=ROMMask[I];
      if(V!=ROMMapper[I][J])
      {
        RAM[J+2]=MemMap[I+1][0][J+2]=ROMData[I]+((int)V<<13);
        ROMMapper[I][J]=V;
      }
      break;

    case 4: /*** ASCII 8kB cartridges ***/
      if((A<0x6000)||(A>0x7FFF)) return;
      J=(A&0x1800)>>11;
      V&=ROMMask[I];
      if(V!=ROMMapper[I][J])
      {
        P=ROMData[I]+((int)V<<13);
        MemMap[I+1][0][J+2]=P;
        ROMMapper[I][J]=V;
        /* Only update memory when cartridge's slot selected */
        if(PSL[(J>>1)+1]==I+1) RAM[J+2]=P;
      }
      break;

    case 5: /*** ASCII 16kB cartridges ***/
      if((A<0x6000)||(A>0x7FFF)) return;
      J=(A&0x1000)>>11;
      V=(V<<1)&ROMMask[I];
      if(V!=ROMMapper[I][J])
      {
        P=ROMData[I]+((int)V<<13);
        MemMap[I+1][0][J+2]=P;
        MemMap[I+1][0][J+3]=P+0x2000;
        ROMMapper[I][J]=V;
        /* Only update memory when cartridge's slot selected */
        if(PSL[(J>>1)+1]==I+1)
        {
          RAM[J+2]=P;
          RAM[J+3]=P+0x2000;
        }
      }
      break;

    case 6: /*** GameMaster2+SRAM cartridge ***/
      /* Switch ROM and SRAM pages, page at 4000h is fixed */
      if((A>=0x6000)&&(A<=0xA000)&&!(A&0x1FFF))
      {
        /* If changing SRAM page... */
        if(V&0x10)
        {
          /* Select SRAM page */
          RAM[5]=MemMap[I+1][0][5]=(SRAM? SRAM:EmptyRAM)+(V&0x20? 0x2000:0);
          /* SRAM is now on */
          ROMMapper[I][3]=0xFF;
        }
        else
        {
          /* Figure out which ROM page gets switched */
          J=(A-0x4000)>>13;
          /* Compute new ROM page number */
          V&=ROMMask[I];
          /* If ROM page number has changed... */
          if(V!=ROMMapper[I][J])
          {
            RAM[J+2]=MemMap[I+1][0][J+2]=ROMData[I]+((int)V<<13);
            ROMMapper[I][J]=V;
          }
        }
        /* Done with page switch */
        break;
      }
      /* Write to SRAM */
      if((A>=0xB000)&&(A<0xC000)&&(ROMMapper[I][3]==0xFF))
      {
        RAM[5][(A&0x0FFF)|0x1000]=RAM[5][A&0x0FFF]=V;
        SaveSRAM=1;
        break;
      }
      /* Done */
      return;

    case 7: /*** RTYPE 16kB cartridges ***/
      if((A<0x7000)||(A>0x7FFF)) return;
      V&=(V&0x10) ? 0x17:0x1F;
      if(V!=ROMMapper[I][2])
      {
        P=ROMData[I]+((int)V<<14);
        MemMap[I+1][0][4]=P;
        MemMap[I+1][0][5]=P+0x2000;
        ROMMapper[I][2]=V;
        /* Only update memory when cartridge's slot selected */
        if(PSL[2]==I+1)
        {
          RAM[4]=P;
          RAM[5]=P+0x2000;
        }
      }
      break;


    default: /*** No MegaROM mapper by default ***/
      return;
  }

  if(Verbose&0x08)
  {
    /* K=1 for 16kB pages */
    K=(K==1)||(K==5);

    printf
    (
      "ROM-MAPPER %c: %dkB block %d at %04Xh\n",
      I+'A',8*(K+1),K? (V>>1):V,J*0x2000+0x4000
    );
  }
}

/** SSlot() **************************************************/
/** Switch secondary memory slots. This function is called  **/
/** when value in (FFFFh) changes.                          **/
/*************************************************************/
void SSlot(register byte V)
{
  register byte I,J;

  if(SSLReg!=V)
  {
    SSLReg=V;

    for(J=0;J<4;J++,V>>=2)
    {
      SSL[J]=V&3;
      if(PSL[J]==3)
      {
        I=J<<1;
        EnWrite[J]=(SSL[J]==2)&&(MemMap[3][2][I]!=EmptyRAM);
        RAM[I]=MemMap[3][SSL[J]][I];
        RAM[I+1]=MemMap[3][SSL[J]][I+1];
      }
    }
  }
}

#ifdef ZLIB
#define fopen          gzopen
#define fclose         gzclose
#define fread(B,L,N,F) gzread(F,B,(L)*(N))
#define fseek          gzseek
#define fgetc          gzgetc
#define ftell          gztell
#endif

/** LoadROM() ************************************************/
/** Load a file, allocating memory as needed. Returns addr. **/
/** of the alocated space or 0 if failed.                   **/
/*************************************************************/
byte *LoadROM(const char *Name,int Size,byte *Buf)
{
  FILE *F;
  byte *P;

  /* Can't give address without size! */
  if(Buf&&!Size) return(0);

  /* Open file */
  if(!(F=fopen(Name,"rb"))) return(0);

  /* Determine data size, if wasn't given */
  if(!Size)
  {
    /* Determine size via fseek()/ftell() or by reading */
    /* entire [GZIPped] stream                          */
    if(fseek(F,0,SEEK_END)>=0) Size=ftell(F);
    else while(fgetc(F)>=0) Size++;
    /* Rewind file to the beginning */
    fseek(F,0,SEEK_SET);
  }

  /* Allocate memory */
  P=Buf? Buf:malloc(Size);
  if(!P)
  {
    fclose(F);
    return(0);
  }

  /* Read data */
  if(fread(P,1,Size,F)!=Size)
  {
    if(!Buf) free(P);
    fclose(F);
    return(0);
  }

  /* Add a new chunk to free */
  if(!Buf) Chunks[CCount++]=P;

  /* Done */
  fclose(F);
  return(P);
}

/** LoadCart() ***********************************************/
/** Load a cartridge ROM from .ROM file into given slot.    **/
/*************************************************************/
int LoadCart(const char *Name,int Slot)
{
  int C1,C2,C3,ROM64,LastFirst,FileSize;
  FILE *F;

  /* Check slot #, try to open file */
  if((Slot!=0)&&(Slot!=1)) return(0);

  //LUDO:
  if (ROMData[Slot]) free(ROMData[Slot]);
  ROMData[Slot] = NULL;

  if(!(F=fopen(Name,"rb"))) return(0);
  if(Verbose) printf("Found %s:\n",Name);

  /* Check "AB" signature in a file */
  ROM64=LastFirst=0;
  C1=fgetc(F);
  C2=fgetc(F);

  /* Maybe this is a flat 64kB ROM? */
  if((C1!='A')||(C2!='B'))
    if(fseek(F,0x4000,SEEK_SET)>=0)
    {
      C1=fgetc(F);
      C2=fgetc(F);
      ROM64=(C1=='A')&&(C2=='B');
    }

  /* Maybe it is the last page that contains "AB" signature? */
  if((C1!='A')||(C2!='B'))
    if(fseek(F,-0x4000,SEEK_END)>=0)
    {
      C1=fgetc(F);
      C2=fgetc(F);
      LastFirst=(C1=='A')&&(C2=='B');
    }

  /* If we can't find "AB" signature, drop out */
  if((C1!='A')||(C2!='B'))
  {
    if(Verbose) puts("  Not a valid cartridge ROM");
    fclose(F);
    return(0);
  }

  if(Verbose) printf("  Cartridge %c: ",'A'+Slot);

  /* Determine file length via fseek()/ftell() */
  if(fseek(F,0,SEEK_END)>=0) C1=ftell(F);
  else
  {
    /* Determine file length by reading entire [GZIPped] stream */
    fseek(F,0,SEEK_SET);
    for(C1=0;(C2=fread(EmptyRAM,1,0x4000,F))==0x4000;C1+=C2);
    if(C2>=0) C1+=C2;
  }
  FileSize=C1;

  /* Done with the file */
  fclose(F);

  /* Length must be a multiple of 8kB */
  /* Flat 64kB ROM must be 40..64kB */
  if(C1&0x1FFF)           { PRINTFAILED;return(0); }
  if(ROM64&&(C1<0xA000))  { PRINTFAILED;return(0); }
  if(ROM64&&(C1>0x10000)) { PRINTFAILED;return(0); }

  /* Show ROM type and size */
  if(Verbose)
    printf
    (
      "%dkB %s ROM...",C1/1024,
      !ROM64&&(C1>0x8000)? ROMNames[Slot? ROMTypeB:ROMTypeA]:"NORMAL"
    );

  /* Compute size in 8kB pages */
  C1>>=13;

  /* Calculate 2^n closest to number of pages */
  for(C3=1;C3<C1;C3<<=1);

  /* Assign ROMMask for MegaROMs */
  ROMMask[Slot]=!ROM64&&(C1>4)? C3-1:0x00;

  /* Allocate space for the ROM */
  ROMData[Slot]=malloc(C3*0x2000);
  if(!ROMData[Slot]) { PRINTFAILED;return(0); }
  Chunks[CCount++]=ROMData[Slot];

  /* Try loading ROM */
  if(!LoadROM(Name,C1*0x2000,ROMData[Slot])) { PRINTFAILED;return(0); }

  /* Mirror ROM if it is smaller than 2^n pages */
  if(C1<C3)
    memcpy
    (
      ROMData[Slot]+C1*0x2000,
      ROMData[Slot]+(C1-C3/2)*0x2000,
      (C3-C1)*0x2000
    );

  /* Set memory map depending on the ROM size */
  switch(C1)
  {
    case 1:
      /* 8kB ROMs are mirrored 8 times: 0:0:0:0:0:0:0:0 */
      MemMap[Slot+1][0][0]=ROMData[Slot];
      MemMap[Slot+1][0][1]=ROMData[Slot];
      MemMap[Slot+1][0][2]=ROMData[Slot];
      MemMap[Slot+1][0][3]=ROMData[Slot];
      MemMap[Slot+1][0][4]=ROMData[Slot];
      MemMap[Slot+1][0][5]=ROMData[Slot];
      MemMap[Slot+1][0][6]=ROMData[Slot];
      MemMap[Slot+1][0][7]=ROMData[Slot];
      break;

    case 2:
      /* 16kB ROMs are mirrored 4 times: 0:1:0:1:0:1:0:1 */
      MemMap[Slot+1][0][0]=ROMData[Slot];
      MemMap[Slot+1][0][1]=ROMData[Slot]+0x2000;
      MemMap[Slot+1][0][2]=ROMData[Slot];
      MemMap[Slot+1][0][3]=ROMData[Slot]+0x2000;
      MemMap[Slot+1][0][4]=ROMData[Slot];
      MemMap[Slot+1][0][5]=ROMData[Slot]+0x2000;
      MemMap[Slot+1][0][6]=ROMData[Slot];
      MemMap[Slot+1][0][7]=ROMData[Slot]+0x2000;
      break;

    case 3:
    case 4:
      /* 24kB and 32kB ROMs are mirrored twice: 0:1:0:1:2:3:2:3 */
      MemMap[Slot+1][0][0]=ROMData[Slot];
      MemMap[Slot+1][0][1]=ROMData[Slot]+0x2000;
      MemMap[Slot+1][0][2]=ROMData[Slot];
      MemMap[Slot+1][0][3]=ROMData[Slot]+0x2000;
      MemMap[Slot+1][0][4]=ROMData[Slot]+0x4000;
      MemMap[Slot+1][0][5]=ROMData[Slot]+0x6000;
      MemMap[Slot+1][0][6]=ROMData[Slot]+0x4000;
      MemMap[Slot+1][0][7]=ROMData[Slot]+0x6000;
      break;

    default:
      if(ROM64)
      {
        /* 64kB ROMs are loaded to fill slot: 0:1:2:3:4:5:6:7 */
        MemMap[Slot+1][0][0]=ROMData[Slot];
        MemMap[Slot+1][0][1]=ROMData[Slot]+0x2000;
        MemMap[Slot+1][0][2]=ROMData[Slot]+0x4000;
        MemMap[Slot+1][0][3]=ROMData[Slot]+0x6000;
        MemMap[Slot+1][0][4]=ROMData[Slot]+0x8000;
        MemMap[Slot+1][0][5]=ROMData[Slot]+0xA000;
        MemMap[Slot+1][0][6]=ROMData[Slot]+0xC000;
        MemMap[Slot+1][0][7]=ROMData[Slot]+0xE000;
      }
      else
      {
        /* MegaROMs are switched into 4000h-BFFFh */
        if(!LastFirst) SetMegaROM(Slot,0,1,2,3);
        else SetMegaROM(Slot,C1-2,C1-1,C1-2,C1-1);
      }
      break;
  }

  /* Show starting address */
  if(Verbose)
    printf
    (
      "starts at %04Xh...OK\n",
      MemMap[Slot+1][0][2][2]+256*MemMap[Slot+1][0][2][3]
    );

  /* Done loading cartridge */
  return(FileSize);
}

#ifdef ZLIB
#undef fopen
#undef fclose
#undef fread
#undef fseek
#undef fgetc
#undef ftell
#endif

/** SetIRQ() *************************************************/
/** Set or reset IRQ. Returns IRQ vector assigned to        **/
/** CPU.IRequest. When upper bit of IRQ is 1, IRQ is reset. **/
/*************************************************************/
word SetIRQ(register byte IRQ)
{
  if(IRQ&0x80) IRQPending&=IRQ; else IRQPending|=IRQ;
  CPU.IRequest=IRQPending? INT_IRQ:INT_NONE;
  return(CPU.IRequest);
}

/** SetScreen() **********************************************/
/** Change screen mode. Returns new screen mode.            **/
/*************************************************************/
byte SetScreen(void)
{
  register byte I,J;

  switch(((VDP[0]&0x0E)>>1)|(VDP[1]&0x18))
  {
    case 0x10: J=0;break;
    case 0x00: J=1;break;
    case 0x01: J=2;break;
    case 0x08: J=3;break;
    case 0x02: J=4;break;
    case 0x03: J=5;break;
    case 0x04: J=6;break;
    case 0x05: J=7;break;
    case 0x07: J=8;break;
    case 0x12: J=MAXSCREEN+1;break;
    default:   J=ScrMode;break;
  }

  /* Recompute table addresses */
  I=(J>6)&&(J!=MAXSCREEN+1)? 11:10;
  ChrTab  = VRAM+((int)(VDP[2]&MSK[J].R2)<<I);
  ChrGen  = VRAM+((int)(VDP[4]&MSK[J].R4)<<11);
  ColTab  = VRAM+((int)(VDP[3]&MSK[J].R3)<<6)+((int)VDP[10]<<14);
  SprTab  = VRAM+((int)(VDP[5]&MSK[J].R5)<<7)+((int)VDP[11]<<15);
  SprGen  = VRAM+((int)VDP[6]<<11);
  ChrTabM = ((int)(VDP[2]|~MSK[J].M2)<<I)|((1<<I)-1);
  ChrGenM = ((int)(VDP[4]|~MSK[J].M4)<<11)|0x007FF;
  ColTabM = ((int)(VDP[3]|~MSK[J].M3)<<6)|0x1C03F;
  SprTabM = ((int)(VDP[5]|~MSK[J].M5)<<7)|0x1807F;

  /* Return new screen mode */
  ScrMode=J;
  return(J);
}

/** SetMegaROM() *********************************************/
/** Set MegaROM pages for a given slot. SetMegaROM() always **/
/** assumes 8kB pages.                                      **/
/*************************************************************/
void SetMegaROM(int Slot,byte P0,byte P1,byte P2,byte P3)
{
  P0&=ROMMask[Slot];
  P1&=ROMMask[Slot];
  P2&=ROMMask[Slot];
  P3&=ROMMask[Slot];
  MemMap[Slot+1][0][2]=ROMData[Slot]+P0*0x2000;
  MemMap[Slot+1][0][3]=ROMData[Slot]+P1*0x2000;
  MemMap[Slot+1][0][4]=ROMData[Slot]+P2*0x2000;
  MemMap[Slot+1][0][5]=ROMData[Slot]+P3*0x2000;
  ROMMapper[Slot][0]=P0;
  ROMMapper[Slot][1]=P1;
  ROMMapper[Slot][2]=P2;
  ROMMapper[Slot][3]=P3;
}

/** VDPOut() *************************************************/
/** Write value into a given VDP register.                  **/
/*************************************************************/
void VDPOut(register byte R,register byte V)
{
  register byte J;

  switch(R)
  {
    case  0: /* Reset HBlank interrupt if disabled */
             if((VDPStatus[1]&0x01)&&!(V&0x10))
             {
               VDPStatus[1]&=0xFE;
               SetIRQ(~INT_IE1);
             }
             /* Set screen mode */
             if(VDP[0]!=V) { VDP[0]=V;SetScreen(); }
             break;
    case  1: /* Set/Reset VBlank interrupt if enabled or disabled */
             if(VDPStatus[0]&0x80) SetIRQ(V&0x20? INT_IE0:~INT_IE0);
             /* Set screen mode */
             if(VDP[1]!=V) { VDP[1]=V;SetScreen(); }
             break;
    case  2: J=(ScrMode>6)&&(ScrMode!=MAXSCREEN+1)? 11:10;
             ChrTab  = VRAM+((int)(V&MSK[ScrMode].R2)<<J);
             ChrTabM = ((int)(V|~MSK[ScrMode].M2)<<J)|((1<<J)-1);
             break;
    case  3: ColTab  = VRAM+((int)(V&MSK[ScrMode].R3)<<6)+((int)VDP[10]<<14);
             ColTabM = ((int)(V|~MSK[ScrMode].M3)<<6)|0x1C03F;
             break;
    case  4: ChrGen  = VRAM+((int)(V&MSK[ScrMode].R4)<<11);
             ChrGenM = ((int)(V|~MSK[ScrMode].M4)<<11)|0x007FF;
             break;
    case  5: SprTab  = VRAM+((int)(V&MSK[ScrMode].R5)<<7)+((int)VDP[11]<<15);
             SprTabM = ((int)(V|~MSK[ScrMode].M5)<<7)|0x1807F;
             break;
    case  6: V&=0x3F;SprGen=VRAM+((int)V<<11);break;
    case  7: FGColor=V>>4;BGColor=V&0x0F;break;
    case 10: V&=0x07;
             ColTab=VRAM+((int)(VDP[3]&MSK[ScrMode].R3)<<6)+((int)V<<14);
             break;
    case 11: V&=0x03;
             SprTab=VRAM+((int)(VDP[5]&MSK[ScrMode].R5)<<7)+((int)V<<15);
             break;
    case 14: V&=VRAMPages-1;VPAGE=VRAM+((int)V<<14);
             break;
    case 15: V&=0x0F;break;
    case 16: V&=0x0F;PKey=1;break;
    case 17: V&=0xBF;break;
    case 25: VDP[25]=V;
             SetScreen();
             break;
    case 44: VDPWrite(V);break;
    case 46: VDPDraw(V);break;
  }

  /* Write value into a register */
  VDP[R]=V;
}

/** Printer() ************************************************/
/** Send a character to the printer.                        **/
/*************************************************************/
void Printer(byte V) { fputc(V,PrnStream); }

/** PPIOut() *************************************************/
/** This function is called on each write to PPI to make    **/
/** key click sound, motor relay clicks, and so on.         **/
/*************************************************************/
void PPIOut(register byte New,register byte Old)
{
}

/** RTCIn() **************************************************/
/** Read value from a given RTC register.                   **/
/*************************************************************/
byte RTCIn(register byte R)
{
  static time_t PrevTime;
  static struct tm TM;
  register byte J;
  time_t CurTime;

  /* Only 16 registers/mode */
  R&=0x0F;

  /* Bank mode 0..3 */
  J=RTCMode&0x03;

  if(R>12) J=R==13? RTCMode:NORAM;
  else
    if(J) J=RTC[J][R];
    else
    {
      /* Retrieve system time if any time passed */
      CurTime=time(NULL);
      if(CurTime!=PrevTime)
      {
        TM=*localtime(&CurTime);
        PrevTime=CurTime;
      }

      /* Parse contents of last retrieved TM */
      switch(R)
      {
        case 0:  J=TM.tm_sec%10;break;
        case 1:  J=TM.tm_sec/10;break;
        case 2:  J=TM.tm_min%10;break;
        case 3:  J=TM.tm_min/10;break;
        case 4:  J=TM.tm_hour%10;break;
        case 5:  J=TM.tm_hour/10;break;
        case 6:  J=TM.tm_wday;break;
        case 7:  J=TM.tm_mday%10;break;
        case 8:  J=TM.tm_mday/10;break;
        case 9:  J=(TM.tm_mon+1)%10;break;
        case 10: J=(TM.tm_mon+1)/10;break;
        case 11: J=(TM.tm_year-80)%10;break;
        case 12: J=((TM.tm_year-80)/10)%10;break;
        default: J=0x0F;break;
      }
    }

  /* Four upper bits are always high */
  return(J|0xF0);
}

/** LoopZ80() ************************************************/
/** Refresh screen, check keyboard and sprites. Call this   **/
/** function on each interrupt.                             **/
/*************************************************************/
word LoopZ80()
{
  static byte BFlag=0;
  static byte BCount=0;
  static int  UCount=1;
  static byte ACount=0;
  static byte Drawing=0;
  register int J;

  /* Flip HRefresh bit */
  VDPStatus[2]^=0x20;

  /* If HRefresh is now in progress... */
  if(!(VDPStatus[2]&0x20))
  {
    /* HRefresh takes most of the scanline */
    CPU.IPeriod=!ScrMode||(ScrMode==MAXSCREEN+1)? CPU_H240:CPU_H256;

    /* New scanline */
    ScanLine=ScanLine<(PALVideo? 312:261)? ScanLine+1:0;

    /* If first scanline of the screen... */
    if(!ScanLine)
    {
      /* Drawing now... */
      Drawing=1;

      /* Reset VRefresh bit */
      VDPStatus[2]&=0xBF;

      /* Refresh display */
      if(UCount>=100) { UCount-=100;RefreshScreen(); }
      UCount+= MSX.msx_uperiod;

      /* Blinking for TEXT80 */
      if(BCount) BCount--;
      else
      {
        BFlag=!BFlag;
        if(!VDP[13]) { XFGColor=FGColor;XBGColor=BGColor; }
        else
        {
          BCount=(BFlag? VDP[13]&0x0F:VDP[13]>>4)*10;
          if(BCount)
          {
            if(BFlag) { XFGColor=FGColor;XBGColor=BGColor; }
            else      { XFGColor=VDP[12]>>4;XBGColor=VDP[12]&0x0F; }
          }
        }
      }
    }

    /* Line coincidence is active at 0..255 */
    /* in PAL and 0..234/244 in NTSC        */
    J=PALVideo? 256:ScanLines212? 245:235;

    /* When reaching end of screen, reset line coincidence */
    if(ScanLine==J)
    {
      VDPStatus[1]&=0xFE;
      SetIRQ(~INT_IE1);
    }

    /* When line coincidence is active... */
    if(ScanLine<J)
    {
      /* Line coincidence processing */
      J=(((ScanLine+VScroll)&0xFF)-VDP[19])&0xFF;
      if(J==2)
      {
        /* Set HBlank flag on line coincidence */
        VDPStatus[1]|=0x01;
        /* Generate IE1 interrupt */
        if(VDP[0]&0x10) SetIRQ(INT_IE1);
      }
      else
      {
        /* Reset flag immediately if IE1 interrupt disabled */
        if(!(VDP[0]&0x10)) VDPStatus[1]&=0xFE;
      }
    }

    /* Return whatever interrupt is pending */
    CPU.IRequest=IRQPending? INT_IRQ:INT_NONE;
    return(CPU.IRequest);
  }

  /*********************************/
  /* We come here for HBlanks only */
  /*********************************/

  /* HBlank takes HPeriod-HRefresh */
  CPU.IPeriod=!ScrMode||(ScrMode==MAXSCREEN+1)? CPU_H240:CPU_H256;
  CPU.IPeriod=HPeriod-CPU.IPeriod;

  /* If last scanline of VBlank, see if we need to wait more */
  J=PALVideo? 313:262;
  if(ScanLine>=J-1)
  {
    J*=CPU_HPERIOD;
    if(VPeriod>J) CPU.IPeriod+=VPeriod-J;
  }

  /* If first scanline of the bottom border... */
  if(ScanLine==(ScanLines212? 212:192)) Drawing=0;

  /* If first scanline of VBlank... */
  J=PALVideo? (ScanLines212? 212+42:192+52):(ScanLines212? 212+18:192+28);
  if(!Drawing&&(ScanLine==J))
  {
    /* Set VBlank bit, set VRefresh bit */
    VDPStatus[0]|=0x80;
    VDPStatus[2]|=0x40;

    /* Generate VBlank interrupt */
    if(VDP[1]&0x20) SetIRQ(INT_IE0);
  }

  /* Run V9938 engine */
  LoopVDP();

  /* Refresh scanline, possibly with the overscan */
  if((UCount>=100)&&Drawing&&(ScanLine<256))
  {
    if(!ModeYJK||(ScrMode<7)||(ScrMode>8))
      (RefreshLine[ScrMode])(ScanLine);
    else
      if(ModeYAE) RefreshLine10(ScanLine);
      else RefreshLine12(ScanLine);
  }

  /* Keyboard, sound, and other stuff always runs at line 192    */
  /* This way, it can't be shut off by overscan tricks (Maarten) */
  if(ScanLine==192)
  {
    /* Set the correct value for R register in case we save the state here */
    CPU.R=(CPU.R&0x80)|(RCounter&0x7f);

    /* Check sprites and set Collision, 5Sprites, 5thSprite bits */
    if(!SpritesOFF&&ScrMode&&(ScrMode<MAXSCREEN+1)) CheckSprites();

    /* Check keyboard */
    psp_update_keys();

    /* Exit emulation if requested */
    if(ExitNow) return(INT_QUIT);

    /* Autofire emulation */
    ACount=(ACount+1)&0x07;
    if((ACount>3)&&AutoFire) KeyMap[8]|=0x01;
  }

  /* Return whatever interrupt is pending */
  CPU.IRequest=IRQPending? INT_IRQ:INT_NONE;
  return(CPU.IRequest);
}

/** CheckSprites() *******************************************/
/** Check for sprite collisions and 5th/9th sprite in a     **/
/** row.                                                    **/
/*************************************************************/
void CheckSprites(void)
{
  register word LS,LD;
  register byte DH,DV,*PS,*PD,*T;
  byte I,J,N,M,*S,*D;

  /* Clear 5Sprites, Collision, and 5thSprite bits */
  VDPStatus[0]=(VDPStatus[0]&0x9F)|0x1F;

  for(N=0,S=SprTab;(N<32)&&(S[0]!=208);N++,S+=4);
  M=SolidColor0;

  if(Sprites16x16)
  {
    for(J=0,S=SprTab;J<N;++J,S+=4)
      if((S[3]&0x0F)||M)
        for(I=J+1,D=S+4;I<N;++I,D+=4)
          if((D[3]&0x0F)||M)
          {
            DV=S[0]-D[0];
            if((DV<16)||(DV>240))
	    {
              DH=S[1]-D[1];
              if((DH<16)||(DH>240))
	      {
                PS=SprGen+((int)(S[2]&0xFC)<<3);
                PD=SprGen+((int)(D[2]&0xFC)<<3);
                if(DV<16) PD+=DV; else { DV=256-DV;PS+=DV; }
                if(DH>240) { DH=256-DH;T=PS;PS=PD;PD=T; }
                while(DV<16)
                {
                  LS=((word)*PS<<8)+*(PS+16);
                  LD=((word)*PD<<8)+*(PD+16);
                  if(LD&(LS>>DH)) break;
                  else { DV++;PS++;PD++; }
                }
                if(DV<16) { VDPStatus[0]|=0x20;return; }
              }
            }
          }
  }
  else
  {
    for(J=0,S=SprTab;J<N;++J,S+=4)
      if((S[3]&0x0F)||M)
        for(I=J+1,D=S+4;I<N;++I,D+=4)
          if((D[3]&0x0F)||M)
          {
            DV=S[0]-D[0];
            if((DV<8)||(DV>248))
            {
              DH=S[1]-D[1];
              if((DH<8)||(DH>248))
              {
                PS=SprGen+((int)S[2]<<3);
                PD=SprGen+((int)D[2]<<3);
                if(DV<8) PD+=DV; else { DV=256-DV;PS+=DV; }
                if(DH>248) { DH=256-DH;T=PS;PS=PD;PD=T; }
                while((DV<8)&&!(*PD&(*PS>>DH))) { DV++;PS++;PD++; }
                if(DV<8) { VDPStatus[0]|=0x20;return; }
              }
            }
          }
  }
}

/** GuessROM() ***********************************************/
/** Guess MegaROM mapper of a ROM.                          **/
/*************************************************************/
int GuessROM(const byte *Buf,int Size)
{
  int J,I,N,ROMCount[6];
  char S[256], SHA_BUF[256], TYP_BUF[256], SHA[41];
  FILE *F;
  SHA1Context sha;

  /* Try opening file with SHA1s */
  if(F=fopen("carts.sha","rb"))
  {
    I=-1;
    /* Compute ROM's SHA1 */
    SHA1Reset(&sha);
    SHA1Input(&sha, Buf, Size);

    if (SHA1Result(&sha))
    {
      sprintf(SHA,"%08x%08x%08x%08x%08x",
                      sha.Message_Digest[0],
                      sha.Message_Digest[1],
                      sha.Message_Digest[2],
                      sha.Message_Digest[3],
                      sha.Message_Digest[4]);
      /* Scan file comparing SHA1s */
      while(fgets(S,sizeof(S)-4,F)) {
        if(sscanf(S,"%s %s",SHA_BUF,TYP_BUF)==2) {
          I=strcmp(SHA_BUF,SHA);
          if(!I) break;
        }
      }
    }
    fclose(F);

    if (!I) {
      if (!strcmp(TYP_BUF, "GenericKonami")) return(0); // Generic8
      if (!strcmp(TYP_BUF, "KonamiSCC")) return(2) ;    // Konami5
      if (!strcmp(TYP_BUF, "Konami")) return(3);        // Konami5
      if (!strcmp(TYP_BUF, "ASCII8")) return(4);        // ASCII8
      if (!strcmp(TYP_BUF, "ASCII8SRAM8")) return(4);   // ASCII8
      if (!strcmp(TYP_BUF, "ASCII16")) return(5);       // ASCII16
      if (!strcmp(TYP_BUF, "ASCII16SRAM2")) return(5);  // ASCII16
      if (!strcmp(TYP_BUF, "GameMaster2")) return(6);   // GameMaster2
      if (!strcmp(TYP_BUF, "R-Type")) return(7);        // R-Type
    }
  }

  /* Clear all counters */
  for(J=0;J<6;J++) ROMCount[J]=2;
  ROMCount[0]+=2; /* Mapper #0 is default */
  ROMCount[4]-=2; /* #5 preferred over #4 */
  ROMCount[3]-=2; /* #5 preferred over #3 */
  ROMCount[2]-=2; /* #5 preferred over #2 */

  /* Count occurences of characteristic addresses */
  for(J=2;J<Size-2;J++)
  {
    I=Buf[J]+((int)Buf[J+1]<<8)+((int)Buf[J+2]<<16);
    N=((Buf[J-2]==0x3E) ? 2:1); // 0x3E = LD A,nn
    switch(I)
    {
      case 0x500032: ROMCount[2]+=N;break;
      case 0x900032: ROMCount[2]+=N;break;
      case 0xB00032: ROMCount[2]+=N;break;
      case 0x400032: ROMCount[3]+=N;break;
      case 0x800032: ROMCount[3]+=N;break;
      case 0xA00032: ROMCount[3]+=N;break;
      case 0x680032: ROMCount[4]+=N;break;
      case 0x780032: ROMCount[4]+=N;break;
      case 0x600032: ROMCount[3]+=N;ROMCount[4]+=N;ROMCount[5]+=N;break;
      case 0x700032: ROMCount[2]+=N;ROMCount[4]+=N;ROMCount[5]+=N;break;
      case 0x77FF32: ROMCount[5]+=N;break;
      case 0x67FF32: ROMCount[5]+=N;break; // Sergi
      case 0x77F832: ROMCount[4]+=N;break; // Sergi
      case 0x6FF832: ROMCount[4]+=N;break; // Sergi
      case 0x7FF832: ROMCount[4]+=N;break; // Sergi

    }
  }

  /* Find which mapper type got more hits */
  for(I=0,J=0;J<6;J++)
    if(ROMCount[J]>ROMCount[I]) I=J;

  /* Return the most likely mapper type */
  return(I);
}

#ifdef ZLIB
#define fopen           gzopen
#define fclose          gzclose
#define fread(B,L,N,F)  gzread(F,B,(L)*(N))
#define fwrite(B,L,N,F) gzwrite(F,B,(L)*(N))
#endif

/** SaveState() **********************************************/
/** Save emulation state to a .STA file.                    **/
/*************************************************************/
int SaveState(const char *FileName)
{
  static byte Header[16] = "STE\032\002\0\0\0\0\0\0\0\0\0\0\0";
  unsigned int State[256],J,I;
  FILE *F;

  /* Open state file */
  if(!(F=fopen(FileName,"wb"))) return(0);

  /* Prepare the header */
  J=StateID();
  Header[5] = MSX.msx_ram_pages;
  Header[6] = VRAMPages;
  Header[7] = J&0x00FF;
  Header[8] = J>>8;

  /* Write out the header */
  if(fwrite(Header,1,sizeof(Header),F)!=sizeof(Header))
  { fclose(F);return(0); }

  /* Fill out hardware state */
  J=0;
  State[J++] = VDPData;
  State[J++] = PLatch;
  State[J++] = ALatch;
  State[J++] = VAddr;
  State[J++] = VKey;
  State[J++] = PKey;
  State[J++] = WKey;
  State[J++] = IRQPending;
  State[J++] = ScanLine;
  State[J++] = RTCReg;
  State[J++] = RTCMode;
  State[J++] = KanLetter;
  State[J++] = KanCount;
  State[J++] = IOReg;
  State[J++] = PSLReg;
  State[J++] = SSLReg;
  for(I=0;I<4;I++)
  {
    State[J++] = PSL[I];
    State[J++] = SSL[I];
    State[J++] = EnWrite[I];
    State[J++] = RAMMapper[I];
    State[J++] = ROMMapper[0][I];
    State[J++] = ROMMapper[1][I];
  }

  /* Write out hardware state */
  if(fwrite(&CPU,1,sizeof(CPU),F)!=sizeof(CPU))
  { fclose(F);return(0); }
  if(fwrite(&PPI,1,sizeof(PPI),F)!=sizeof(PPI))
  { fclose(F);return(0); }
  if(fwrite(VDP,1,sizeof(VDP),F)!=sizeof(VDP))
  { fclose(F);return(0); }
  if(fwrite(VDPStatus,1,sizeof(VDPStatus),F)!=sizeof(VDPStatus))
  { fclose(F);return(0); }
  if(fwrite(Palette,1,sizeof(Palette),F)!=sizeof(Palette))
  { fclose(F);return(0); }
  if(fwrite(&PSG,1,sizeof(PSG),F)!=sizeof(PSG))
  { fclose(F);return(0); }
  if(fwrite(&OPLL,1,sizeof(OPLL),F)!=sizeof(OPLL))
  { fclose(F);return(0); }
  if(fwrite(&SCChip,1,sizeof(SCChip),F)!=sizeof(SCChip))
  { fclose(F);return(0); }
  if(fwrite(State,1,sizeof(State),F)!=sizeof(State))
  { fclose(F);return(0); }

  /* Save memory contents */
  if(fwrite(RAMData,1,MSX.msx_ram_pages*0x4000,F)!=MSX.msx_ram_pages*0x4000)
  { fclose(F);return(0); }
  if(fwrite(VRAM,1,VRAMPages*0x4000,F)!=VRAMPages*0x4000)
  { fclose(F);return(0); }

  /* Done */
  fclose(F);
  return(1);
}

/** LoadState() **********************************************/
/** Load emulation state from a .STA file.                  **/
/*************************************************************/
int LoadState(const char *FileName)
{
  unsigned int State[256],J,I;
  byte Header[16];
  FILE *F;

  /* Open state file */
  if(!(F=fopen(FileName,"rb"))) return(0);

  /* Read the header */
  if(fread(Header,1,sizeof(Header),F)!=sizeof(Header))
  { fclose(F);return(0); }

  /* Verify the header */
  if(memcmp(Header,"STE\032\002",5))
  { fclose(F);return(0); }
  if(Header[7]+Header[8]*256!=StateID())
  { fclose(F);return(0); }
  if((Header[5]!=MSX.msx_ram_pages)||(Header[6]!=VRAMPages))
  { fclose(F);return(0); }

  /* Read the hardware state */
  if(fread(&CPU,1,sizeof(CPU),F)!=sizeof(CPU))
  { fclose(F);return(0); }
  if(fread(&PPI,1,sizeof(PPI),F)!=sizeof(PPI))
  { fclose(F);return(0); }
  if(fread(VDP,1,sizeof(VDP),F)!=sizeof(VDP))
  { fclose(F);return(0); }
  if(fread(VDPStatus,1,sizeof(VDPStatus),F)!=sizeof(VDPStatus))
  { fclose(F);return(0); }
  if(fread(Palette,1,sizeof(Palette),F)!=sizeof(Palette))
  { fclose(F);return(0); }
  if(fread(&PSG,1,sizeof(PSG),F)!=sizeof(PSG))
  { fclose(F);return(0); }
  if(fread(&OPLL,1,sizeof(OPLL),F)!=sizeof(OPLL))
  { fclose(F);return(0); }
  if(fread(&SCChip,1,sizeof(SCChip),F)!=sizeof(SCChip))
  { fclose(F);return(0); }
  if(fread(State,1,sizeof(State),F)!=sizeof(State))
  { fclose(F);return(0); }

  /* Load memory contents */
  if(fread(RAMData,1,Header[5]*0x4000,F)!=Header[5]*0x4000)
  { fclose(F);return(0); }
  if(fread(VRAM,1,Header[6]*0x4000,F)!=Header[6]*0x4000)
  { fclose(F);return(0); }

  /* Done with the file */
  fclose(F);

  /* Parse hardware state */
  J=0;
  VDPData    = State[J++];
  PLatch     = State[J++];
  ALatch     = State[J++];
  VAddr      = State[J++];
  VKey       = State[J++];
  PKey       = State[J++];
  WKey       = State[J++];
  IRQPending = State[J++];
  ScanLine   = State[J++];
  RTCReg     = State[J++];
  RTCMode    = State[J++];
  KanLetter  = State[J++];
  KanCount   = State[J++];
  IOReg      = State[J++];
  PSLReg     = State[J++];
  SSLReg     = State[J++];
  for(I=0;I<4;I++)
  {
    PSL[I]          = State[J++];
    SSL[I]          = State[J++];
    EnWrite[I]      = State[J++];
    RAMMapper[I]    = State[J++];
    ROMMapper[0][I] = State[J++];
    ROMMapper[1][I] = State[J++];
  }

  /* Set RAM mapper pages */
  if(RAMMask)
    for(I=0;I<4;I++)
    {
      RAMMapper[I]       &= RAMMask;
      MemMap[3][2][I*2]   = RAMData+RAMMapper[I]*0x4000;
      MemMap[3][2][I*2+1] = MemMap[3][2][I*2]+0x2000;
    }

  /* Set ROM mapper pages */
  if(ROMMask[0])
    SetMegaROM(0,ROMMapper[0][0],ROMMapper[0][1],ROMMapper[0][2],ROMMapper[0][3]);
  if(ROMMask[1])
    SetMegaROM(1,ROMMapper[1][0],ROMMapper[1][1],ROMMapper[1][2],ROMMapper[1][3]);

  /* Set main address space pages */
  for(I=0;I<4;I++)
  {
    RAM[2*I]   = MemMap[PSL[I]][PSL[I]==3? SSL[I]:0][2*I];
    RAM[2*I+1] = MemMap[PSL[I]][PSL[I]==3? SSL[I]:0][2*I+1];
  }

  /* Set palette */
  for(I=0;I<16;I++)
    SetColor(I,(Palette[I]>>16)&0xFF,(Palette[I]>>8)&0xFF,Palette[I]&0xFF);

  /* Set screen mode and VRAM table addresses */
  SetScreen();

  /* Set some other variables */
  VPAGE    = VRAM+((int)VDP[14]<<14);
  FGColor  = VDP[7]>>4;
  BGColor  = VDP[7]&0x0F;
  XFGColor = FGColor;
  XBGColor = BGColor;

  /* Done */
  return(1);
}

#ifdef ZLIB
#undef fopen
#undef fclose
#undef fread
#undef fwrite
#endif

/** StateID() ************************************************/
/** Compute 16bit emulation state ID used to identify .STA  **/
/** files.                                                  **/
/*************************************************************/
word StateID(void)
{
  word ID;
  int J;

  ID=0x0000;

  /* Add up cartridge ROM, BIOS, BASIC, and ExtBIOS bytes */
  if(ROMData[0]) for(J=0;J<(ROMMask[0]+1)*0x2000;J++) ID+=ROMData[0][J];
  if(ROMData[1]) for(J=0;J<(ROMMask[1]+1)*0x2000;J++) ID+=ROMData[1][J];
  if(MemMap[0][0][0]) for(J=0;J<0x8000;J++) ID+=MemMap[0][0][0][J];
  if(MemMap[3][1][0]) for(J=0;J<0x4000;J++) ID+=MemMap[3][1][0][J];

  return(ID);
}

/** ResetMSX() ***********************************************/
/** Reset MSX emulation.                                    **/
/*************************************************************/
void
ResetMSX(int hard)
{

  int J;

  /*** VDP status register states: ***/
  static byte VDPSInit[16] = { 0x9F,0,0x6C,0,0,0,0,0,0,0,0,0,0,0,0,0 };

  /*** VDP control register states: ***/
  static byte VDPInit[64]  = {
    0x00,0x10,0xFF,0xFF,0xFF,0xFF,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
  };

  /*** Initial palette: ***/
  static byte PalInit[16][3] = {
    {0x00,0x00,0x00},{0x00,0x00,0x00},{0x20,0xC0,0x20},{0x60,0xE0,0x60},
    {0x20,0x20,0xE0},{0x40,0x60,0xE0},{0xA0,0x20,0x20},{0x40,0xC0,0xE0},
    {0xE0,0x20,0x20},{0xE0,0x60,0x60},{0xC0,0xC0,0x20},{0xC0,0xC0,0x80},
    {0x20,0x80,0x20},{0xC0,0x40,0xA0},{0xA0,0xA0,0xA0},{0xE0,0xE0,0xE0},
  };

  /* Initialize palette */
  for(J=0;J<16;J++) {
    Palette[J]=RGB2INT(PalInit[J][0],PalInit[J][1],PalInit[J][2]);
    SetColor(J,PalInit[J][0],PalInit[J][1],PalInit[J][2]);
  }

  memcpy(VDP,VDPInit,sizeof(VDP));
  memcpy(VDPStatus,VDPSInit,sizeof(VDPStatus));
  memset(KeyMap,0xFF,16);               /* Keyboard         */
  IRQPending=0x00;                      /* No IRQs pending  */
  SCCOn[0]=SCCOn[1]=0;                  /* SCCs off for now */
  RTCReg=RTCMode=0;                     /* Clock registers  */
  KanCount=0;KanLetter=0;               /* Kanji extension  */
  ChrTab=ColTab=ChrGen=VRAM;            /* VDP tables       */
  SprTab=SprGen=VRAM;
  ChrTabM=ColTabM=ChrGenM=SprTabM=~0;   /* VDP addr. masks  */
  VPAGE=VRAM;                           /* VRAM page        */
  FGColor=BGColor=XFGColor=XBGColor=0;  /* VDP colors       */
  ScrMode=0;                            /* Screen mode      */
  VKey=PKey=1;WKey=0;                   /* VDP keys         */
  VAddr=0x0000;                         /* VRAM access addr */
  ScanLine=0;                           /* Current scanline */
  VDPData=NORAM;                        /* VDP data buffer  */
  UseFont=0;                            /* Extern. font use */

  /* init memory */
  for(J=0;J<4;J++)
  {
    EnWrite[J]=0;                      /* Write protect ON for all slots */
    PSL[J]=SSL[J]=0;                   /* PSL=0:0:0:0, SSL=0:0:0:0       */
    MemMap[3][2][J*2]   = RAMData+(3-J)*0x4000;        /* RAMMap=3:2:1:0 */
    MemMap[3][2][J*2+1] = MemMap[3][2][J*2]+0x2000;
    RAMMapper[J]        = 3-J;
    RAM[J*2]            = MemMap[0][0][J*2];           /* Setting RAM    */
    RAM[J*2+1]          = MemMap[0][0][J*2+1];
  }

  if(hard) memset(RAMData,NORAM,MSX.msx_ram_pages*0x4000);

  MemMap[3][3][2]=(Use2413?FMPACROM:EmptyRAM);
  MemMap[3][3][3]=(((Use2413)&&(FMPACROM!=EmptyRAM))?FMPACROM+0x2000:EmptyRAM);

  /* Set "V9958" VDP version for MSX2+ */
  if(MSX.msx_version>=2) VDPStatus[1]|=0x04;

  /* Reset soundchips */
  ResetSound();

  /* Reset CPU */
  ResetZ80();
}

/** RewindTape() *********************************************/
/** Set tape filepointer to beginning of file.              **/
/*************************************************************/
void RewindTape()
{
  if(CasStream!=NULL) fseek(CasStream,0,SEEK_SET);
}

void
msx_default_settings()
{
  MSX.msx_snd_enable      = 1;
  MSX.msx_use_8950        = 0;
  MSX.msx_uperiod         = 100;
  MSX.msx_use_2413        = 0;
  MSX.msx_render_mode     = MSX_RENDER_FULL; //MSX_RENDER_FAST;
  MSX.bittboy_ta_func     = MSXK_F5;
# if !defined(CAANOO_MODE)
  MSX.psp_cpu_clock       = GP2X_DEF_EMU_CLOCK;
# endif
  MSX.psp_sound_volume    = 1;
  MSX.msx_speed_limiter   = 60;
  MSX.msx_view_fps        = 0;
  MSX.psp_screenshot_id   = 0;
  MSX.msx_version         = 1;
  MSX.msx_ram_pages       = 16;
  MSX.msx_megarom_type    = -1;
  MSX.msx_ntsc            = 1;
  MSX.msx_vsync           = 0;
  MSX.msx_auto_fire = 0;
  MSX.msx_auto_fire_period = 6;
  MSX.msx_auto_fire_pressed = 0;

  if (MSX.msx_ntsc) VPeriod = CPU_V262;
  else              VPeriod = CPU_V313;

  msx_change_render_mode(MSX.msx_render_mode);
# if !defined(CAANOO_MODE)
  myPowerSetClockFrequency(MSX.psp_cpu_clock);
# endif
}

int
loc_msx_save_settings(char *chFileName)
{
  FILE* FileDesc;
  int   error = 0;

  FileDesc = fopen(chFileName, "w");
  if (FileDesc != (FILE *)0 ) {

# if !defined(CAANOO_MODE)
    fprintf(FileDesc, "psp_cpu_clock=%d\n"      , MSX.psp_cpu_clock);
# endif
    fprintf(FileDesc, "psp_skip_max_frame=%d\n" , MSX.psp_skip_max_frame);
    fprintf(FileDesc, "psp_sound_volume=%d\n"   , MSX.psp_sound_volume);
    fprintf(FileDesc, "msx_snd_enable=%d\n"     , MSX.msx_snd_enable);
    fprintf(FileDesc, "msx_use_2413=%d\n"       , MSX.msx_use_2413);
    fprintf(FileDesc, "msx_use_8950=%d\n"       , MSX.msx_use_8950);
    fprintf(FileDesc, "msx_uperiod=%d\n"        , MSX.msx_uperiod);
    fprintf(FileDesc, "msx_render_mode=%d\n"    , MSX.msx_render_mode);
    fprintf(FileDesc, "msx_vsync=%d\n"          , MSX.msx_vsync);
    fprintf(FileDesc, "msx_speed_limiter=%d\n"  , MSX.msx_speed_limiter);
    fprintf(FileDesc, "msx_auto_fire_period=%d\n", MSX.msx_auto_fire_period);
    fprintf(FileDesc, "msx_view_fps=%d\n"       , MSX.msx_view_fps);
    fprintf(FileDesc, "msx_ntsc=%d\n"           , MSX.msx_ntsc);
    fprintf(FileDesc, "msx_version=%d\n"        , MSX.msx_version);
    fprintf(FileDesc, "msx_ram_pages=%d\n"      , MSX.msx_ram_pages);
    fprintf(FileDesc, "msx_megarom_type=%d\n"   , MSX.msx_megarom_type);

    fclose(FileDesc);

  } else {
    error = 1;
  }

  return error;
}

int
msx_save_settings(void)
{
  char  FileName[MAX_PATH+1];
  int   error;

  error = 1;

  snprintf(FileName, MAX_PATH, "%s/set/%s.set", MSX.msx_home_dir, MSX.msx_save_name);
  error = loc_msx_save_settings(FileName);

  return error;
}

static int
loc_msx_load_settings(char *chFileName)
{
  char  Buffer[512];
  char *Scan;
  unsigned int Value;
  FILE* FileDesc;

  int   msx_ram_pages = MSX.msx_ram_pages;
  int   msx_version = MSX.msx_version;

  FileDesc = fopen(chFileName, "r");
  if (FileDesc == (FILE *)0 ) return 0;

  while (fgets(Buffer,512, FileDesc) != (char *)0) {

    Scan = strchr(Buffer,'\n');
    if (Scan) *Scan = '\0';
    /* For this #@$% of windows ! */
    Scan = strchr(Buffer,'\r');
    if (Scan) *Scan = '\0';
    if (Buffer[0] == '#') continue;

    Scan = strchr(Buffer,'=');
    if (! Scan) continue;

    *Scan = '\0';
    Value = atoi(Scan+1);

# if !defined(CAANOO_MODE)
    if (!strcasecmp(Buffer,"psp_cpu_clock"))      MSX.psp_cpu_clock = Value;
    else
# endif
    if (!strcasecmp(Buffer,"bittboy_ta_func")) MSX.bittboy_ta_func = Value;
    else
    if (!strcasecmp(Buffer,"bittboy_tb_func")) MSX.bittboy_tb_func = Value;
    else
    if (!strcasecmp(Buffer,"psp_skip_max_frame")) MSX.psp_skip_max_frame = Value;
    else
    if (!strcasecmp(Buffer,"psp_sound_volume"))  MSX.psp_sound_volume = Value;
    else
    if (!strcasecmp(Buffer,"msx_use_8950"))     MSX.msx_use_8950 = Value;
    else
    if (!strcasecmp(Buffer,"msx_uperiod"))     MSX.msx_uperiod = Value;
    else
    if (!strcasecmp(Buffer,"msx_use_2413"))     MSX.msx_use_2413 = Value;
    else
    if (!strcasecmp(Buffer,"msx_snd_enable"))     MSX.msx_snd_enable = Value;
    else
    if (!strcasecmp(Buffer,"msx_render_mode"))    MSX.msx_render_mode = Value;
    else
    if (!strcasecmp(Buffer,"msx_vsync"))    MSX.msx_vsync = Value;
    else
    if (!strcasecmp(Buffer,"msx_view_fps"))  MSX.msx_view_fps = Value;
    else
    if (!strcasecmp(Buffer,"msx_auto_fire_period")) MSX.msx_auto_fire_period = Value;
    else
    if (!strcasecmp(Buffer,"msx_speed_limiter"))  MSX.msx_speed_limiter = Value;
    else
    if (!strcasecmp(Buffer,"msx_ntsc"))  MSX.msx_ntsc = Value;
    else
    if (!strcasecmp(Buffer,"msx_version"))  msx_version = Value;
    else
    if (!strcasecmp(Buffer,"msx_ram_pages"))  msx_ram_pages = Value;
    else
    if (!strcasecmp(Buffer,"msx_megarom_type"))  MSX.msx_megarom_type = Value;
  }

  fclose(FileDesc);

  if (MSX.msx_ntsc) VPeriod = CPU_V262;
  else              VPeriod = CPU_V313;

  msx_change_render_mode(MSX.msx_render_mode);
# if !defined(CAANOO_MODE)
  myPowerSetClockFrequency(MSX.psp_cpu_clock);
# endif

  if ((msx_version   != MSX.msx_version  ) ||
      (msx_ram_pages != MSX.msx_ram_pages)) {
    MSX.msx_version = msx_version;
    MSX.msx_ram_pages = msx_ram_pages;
    /* Do a full reset */
    msx_eject_rom();
  }

  return 0;
}

int
msx_load_settings()
{
  char  FileName[MAX_PATH+1];
  int   error;

  error = 1;

  snprintf(FileName, MAX_PATH, "%s/set/%s.set", MSX.msx_home_dir, MSX.msx_save_name);
  error = loc_msx_load_settings(FileName);

  return error;
}

int
msx_load_file_settings(char *FileName)
{
  return loc_msx_load_settings(FileName);
}

static int
loc_msx_save_cheat(char *chFileName)
{
  FILE* FileDesc;
  int   cheat_num;
  int   error = 0;

  FileDesc = fopen(chFileName, "w");
  if (FileDesc != (FILE *)0 ) {

    for (cheat_num = 0; cheat_num < MSX_MAX_CHEAT; cheat_num++) {
      MSX_cheat_t* a_cheat = &MSX.msx_cheat[cheat_num];
      if (a_cheat->type != MSX_CHEAT_NONE) {
        fprintf(FileDesc, "%d,%x,%x,%s\n",
                a_cheat->type, a_cheat->addr, a_cheat->value, a_cheat->comment);
      }
    }
    fclose(FileDesc);

  } else {
    error = 1;
  }

  return error;
}

int
msx_save_cheat(void)
{
  char  FileName[MAX_PATH+1];
  int   error;

  error = 1;

  snprintf(FileName, MAX_PATH, "%s/cht/%s.cht", MSX.msx_home_dir, MSX.msx_save_name);
  error = loc_msx_save_cheat(FileName);

  return error;
}

static int
loc_msx_load_cheat(char *chFileName)
{
  char  Buffer[512];
  char *Scan;
  char *Field;
  unsigned int  cheat_addr;
  unsigned int  cheat_value;
  unsigned int  cheat_type;
  char         *cheat_comment;
  int           cheat_num;
  FILE* FileDesc;

  memset(MSX.msx_cheat, 0, sizeof(MSX.msx_cheat));
  cheat_num = 0;

  FileDesc = fopen(chFileName, "r");
  if (FileDesc == (FILE *)0 ) return 0;

  while (fgets(Buffer,512, FileDesc) != (char *)0) {

    Scan = strchr(Buffer,'\n');
    if (Scan) *Scan = '\0';
    /* For this #@$% of windows ! */
    Scan = strchr(Buffer,'\r');
    if (Scan) *Scan = '\0';
    if (Buffer[0] == '#') continue;

    /* %d, %x, %x, %s */
    Field = Buffer;
    Scan = strchr(Field, ',');
    if (! Scan) continue;
    *Scan = 0;
    if (sscanf(Field, "%d", &cheat_type) != 1) continue;
    Field = Scan + 1;
    Scan = strchr(Field, ',');
    if (! Scan) continue;
    *Scan = 0;
    if (sscanf(Field, "%x", &cheat_addr) != 1) continue;
    Field = Scan + 1;
    Scan = strchr(Field, ',');
    if (! Scan) continue;
    *Scan = 0;
    if (sscanf(Field, "%x", &cheat_value) != 1) continue;
    Field = Scan + 1;
    cheat_comment = Field;

    if (cheat_type <= MSX_CHEAT_NONE) continue;

    MSX_cheat_t* a_cheat = &MSX.msx_cheat[cheat_num];

    a_cheat->type  = cheat_type;
    a_cheat->addr  = cheat_addr;
    a_cheat->value = cheat_value;
    strncpy(a_cheat->comment, cheat_comment, sizeof(a_cheat->comment));
    a_cheat->comment[sizeof(a_cheat->comment)-1] = 0;

    if (++cheat_num >= MSX_MAX_CHEAT) break;
  }
  fclose(FileDesc);

  return 0;
}

int
msx_load_cheat()
{
  char  FileName[MAX_PATH+1];
  int   error;

  error = 1;

  snprintf(FileName, MAX_PATH, "%s/cht/%s.cht", MSX.msx_home_dir, MSX.msx_save_name);
  error = loc_msx_load_cheat(FileName);

  return error;
}

int
msx_load_file_cheat(char *FileName)
{
  return loc_msx_load_cheat(FileName);
}

void
msx_apply_cheats()
{
  int cheat_num;
  for (cheat_num = 0; cheat_num < MSX_MAX_CHEAT; cheat_num++) {
    MSX_cheat_t* a_cheat = &MSX.msx_cheat[cheat_num];
    if (a_cheat->type == MSX_CHEAT_ENABLE) {
      RAMData[a_cheat->addr % (MSX.msx_ram_pages * 0x4000)] = a_cheat->value;
    }
  }
}


static int
loc_load_rom(char *TmpName)
{
  int error;
  strcpy(CartA,TmpName);
  ChangeDisk(0, NULL);
  ChangeDisk(1, NULL);
  InitAllMSXVariable();
  error = ! InitMSX();
  return error;
}

static int
loc_load_disk(char *filename, int disk_id)
{
  int error;
  error = ! ChangeDisk(disk_id, filename);
  return error;
}

void
update_save_name(char *Name)
{
  char        TmpFileName[MAX_PATH];
  struct stat aStat;
  int         index;
  char       *SaveName;
  char       *Scan1;
  char       *Scan2;

  SaveName = strrchr(Name,'/');
  if (SaveName != (char *)0) SaveName++;
  else                       SaveName = Name;

  if (!strncasecmp(SaveName, "sav_", 4)) {
    Scan1 = SaveName + 4;
    Scan2 = strrchr(Scan1, '_');
    if (Scan2 && (Scan2[1] >= '0') && (Scan2[1] <= '5')) {
      strncpy(MSX.msx_save_name, Scan1, MAX_PATH);
      MSX.msx_save_name[Scan2 - Scan1] = '\0';
    } else {
      strncpy(MSX.msx_save_name, SaveName, MAX_PATH);
    }
  } else {
    strncpy(MSX.msx_save_name, SaveName, MAX_PATH);
  }

  if (MSX.msx_save_name[0] == '\0') {
    strcpy(MSX.msx_save_name,"carta");
  }

  for (index = 0; index < MSX_MAX_SAVE_STATE; index++) {
    MSX.msx_save_state[index].used  = 0;
    memset(&MSX.msx_save_state[index].date, 0, sizeof(time_t));
    MSX.msx_save_state[index].thumb = 0;

    snprintf(TmpFileName, MAX_PATH, "%s/save/sav_%s_%d.sta", MSX.msx_home_dir, MSX.msx_save_name, index);
    if (! stat(TmpFileName, &aStat)) {
      MSX.msx_save_state[index].used = 1;
      MSX.msx_save_state[index].date = aStat.st_mtime;
      snprintf(TmpFileName, MAX_PATH, "%s/save/sav_%s_%d.png", MSX.msx_home_dir, MSX.msx_save_name, index);
      if (! stat(TmpFileName, &aStat)) {
        if (psp_sdl_load_thumb_png(MSX.msx_save_state[index].surface, TmpFileName)) {
          MSX.msx_save_state[index].thumb = 1;
        }
      }
    }
  }

  MSX.comment_present = 0;
  snprintf(TmpFileName, MAX_PATH, "%s/txt/%s.txt", MSX.msx_home_dir, MSX.msx_save_name);
  if (! stat(TmpFileName, &aStat)) {
    MSX.comment_present = 1;
  }
}

typedef struct thumb_list {
  struct thumb_list *next;
  char              *name;
  char              *thumb;
} thumb_list;

static thumb_list* loc_head_thumb = 0;

static void
loc_del_thumb_list()
{
  while (loc_head_thumb != 0) {
    thumb_list *del_elem = loc_head_thumb;
    loc_head_thumb = loc_head_thumb->next;
    if (del_elem->name) free( del_elem->name );
    if (del_elem->thumb) free( del_elem->thumb );
    free(del_elem);
  }
}

static void
loc_add_thumb_list(char* filename)
{
  thumb_list *new_elem;
  char tmp_filename[MAX_PATH];

  strcpy(tmp_filename, filename);
  char* save_name = tmp_filename;

  /* .png extention */
  char* Scan = strrchr(save_name, '.');
  if ((! Scan) || (strcasecmp(Scan, ".png"))) return;
  *Scan = 0;

  if (strncasecmp(save_name, "sav_", 4)) return;
  save_name += 4;

  Scan = strrchr(save_name, '_');
  if (! Scan) return;
  *Scan = 0;

  /* only one png for a give save name */
  new_elem = loc_head_thumb;
  while (new_elem != 0) {
    if (! strcasecmp(new_elem->name, save_name)) return;
    new_elem = new_elem->next;
  }

  new_elem = (thumb_list *)malloc( sizeof( thumb_list ) );
  new_elem->next = loc_head_thumb;
  loc_head_thumb = new_elem;
  new_elem->name  = strdup( save_name );
  new_elem->thumb = strdup( filename );
}

void
load_thumb_list()
{
  char SaveDirName[MAX_PATH];
  DIR* fd = 0;

  loc_del_thumb_list();

  snprintf(SaveDirName, MAX_PATH, "%s/save", MSX.msx_home_dir);

  fd = opendir(SaveDirName);
  if (!fd) return;

  struct dirent *a_dirent;
  while ((a_dirent = readdir(fd)) != 0) {
    if(a_dirent->d_name[0] == '.') continue;
    if (a_dirent->d_type != DT_DIR)
    {
      loc_add_thumb_list( a_dirent->d_name );
    }
  }
  closedir(fd);
}

int
load_thumb_if_exists(char *Name)
{
  char        FileName[MAX_PATH];
  char        ThumbFileName[MAX_PATH];
  struct stat aStat;
  char       *SaveName;
  char       *Scan;

  strcpy(FileName, Name);
  SaveName = strrchr(FileName,'/');
  if (SaveName != (char *)0) SaveName++;
  else                       SaveName = FileName;

  Scan = strrchr(SaveName,'.');
  if (Scan) *Scan = '\0';

  if (!SaveName[0]) return 0;

  thumb_list *scan_list = loc_head_thumb;
  while (scan_list != 0) {
    if (! strcasecmp( SaveName, scan_list->name)) {
      snprintf(ThumbFileName, MAX_PATH, "%s/save/%s", MSX.msx_home_dir, scan_list->thumb);
      if (! stat(ThumbFileName, &aStat))
      {
        if (psp_sdl_load_thumb_png(save_surface, ThumbFileName)) {
          return 1;
        }
      }
    }
    scan_list = scan_list->next;
  }
  return 0;
}

typedef struct comment_list {
  struct comment_list *next;
  char              *name;
  char              *filename;
} comment_list;

static comment_list* loc_head_comment = 0;

static void
loc_del_comment_list()
{
  while (loc_head_comment != 0) {
    comment_list *del_elem = loc_head_comment;
    loc_head_comment = loc_head_comment->next;
    if (del_elem->name) free( del_elem->name );
    if (del_elem->filename) free( del_elem->filename );
    free(del_elem);
  }
}

static void
loc_add_comment_list(char* filename)
{
  comment_list *new_elem;
  char  tmp_filename[MAX_PATH];

  strcpy(tmp_filename, filename);
  char* save_name = tmp_filename;

  /* .png extention */
  char* Scan = strrchr(save_name, '.');
  if ((! Scan) || (strcasecmp(Scan, ".txt"))) return;
  *Scan = 0;

  /* only one txt for a given save name */
  new_elem = loc_head_comment;
  while (new_elem != 0) {
    if (! strcasecmp(new_elem->name, save_name)) return;
    new_elem = new_elem->next;
  }

  new_elem = (comment_list *)malloc( sizeof( comment_list ) );
  new_elem->next = loc_head_comment;
  loc_head_comment = new_elem;
  new_elem->name  = strdup( save_name );
  new_elem->filename = strdup( filename );
}

void
load_comment_list()
{
  char SaveDirName[MAX_PATH];
  DIR* fd = 0;

  loc_del_comment_list();

  snprintf(SaveDirName, MAX_PATH, "%s/txt", MSX.msx_home_dir);

  fd = opendir(SaveDirName);
  if (!fd) return;

  struct dirent *a_dirent;
  while ((a_dirent = readdir(fd)) != 0) {
    if(a_dirent->d_name[0] == '.') continue;
    if (a_dirent->d_type != DT_DIR)
    {
      loc_add_comment_list( a_dirent->d_name );
    }
  }
  closedir(fd);
}

char*
load_comment_if_exists(char *Name)
{
static char loc_comment_buffer[128];

  char        FileName[MAX_PATH];
  char        TmpFileName[MAX_PATH];
  FILE       *a_file;
  char       *SaveName;
  char       *Scan;

  loc_comment_buffer[0] = 0;

  strcpy(FileName, Name);
  SaveName = strrchr(FileName,'/');
  if (SaveName != (char *)0) SaveName++;
  else                       SaveName = FileName;

  Scan = strrchr(SaveName,'.');
  if (Scan) *Scan = '\0';

  if (!SaveName[0]) return 0;

  comment_list *scan_list = loc_head_comment;
  while (scan_list != 0) {
    if (! strcasecmp( SaveName, scan_list->name)) {
      snprintf(TmpFileName, MAX_PATH, "%s/txt/%s", MSX.msx_home_dir, scan_list->filename);
      a_file = fopen(TmpFileName, "r");
      if (a_file) {
        char* a_scan = 0;
        loc_comment_buffer[0] = 0;
        if (fgets(loc_comment_buffer, 60, a_file) != 0) {
          a_scan = strchr(loc_comment_buffer, '\n');
          if (a_scan) *a_scan = '\0';
          /* For this #@$% of windows ! */
          a_scan = strchr(loc_comment_buffer,'\r');
          if (a_scan) *a_scan = '\0';
          fclose(a_file);
          return loc_comment_buffer;
        }
        fclose(a_file);
        return 0;
      }
    }
    scan_list = scan_list->next;
  }
  return 0;
}


void
reset_save_name()
{
  update_save_name("");
}

void
msx_kbd_load(void)
{
  char        TmpFileName[MAX_PATH + 1];
  struct stat aStat;

  snprintf(TmpFileName, MAX_PATH, "%s/kbd/%s.kbd", MSX.msx_home_dir, MSX.msx_save_name );
  if (! stat(TmpFileName, &aStat)) {
    psp_kbd_load_mapping(TmpFileName);
  }
}

int
msx_kbd_save(void)
{
  char TmpFileName[MAX_PATH + 1];
  snprintf(TmpFileName, MAX_PATH, "%s/kbd/%s.kbd", MSX.msx_home_dir, MSX.msx_save_name );
  return( psp_kbd_save_mapping(TmpFileName) );
}

void
msx_joy_load(void)
{
  char        TmpFileName[MAX_PATH + 1];
  struct stat aStat;

  snprintf(TmpFileName, MAX_PATH, "%s/joy/%s.joy", MSX.msx_home_dir, MSX.msx_save_name );
  if (! stat(TmpFileName, &aStat)) {
    psp_joy_load_settings(TmpFileName);
  }
}

int
msx_joy_save(void)
{
  char TmpFileName[MAX_PATH + 1];
  snprintf(TmpFileName, MAX_PATH, "%s/joy/%s.joy", MSX.msx_home_dir, MSX.msx_save_name );
  return( psp_joy_save_settings(TmpFileName) );
}



void
emulator_reset(void)
{
  ResetMSX(1);
}

int
msx_eject_rom(void)
{
  loc_load_rom("");
  update_save_name("");
  ResetMSX(1);
  return 0;
}

int
msx_load_rom(char *FileName, int zip_format)
{
  // char   SaveName[MAX_PATH+1];
  char   TmpFileName[MAX_PATH + 1];
  FILE*  TmpFile;
  char*  ExtractName;
  // char*  scan;
  char*  zip_buffer;
  int    format;
  // int    error;
  // size_t unzipped_size;

  #define READ_SIZE 8192
  char   SaveName[MAX_PATH+1];
  int    error;
  char* scan;
  size_t unzipped_size;
  int8_t read_buffer[READ_SIZE];
  int8_t filename[512];
  // error = 1;


  error = 1;


  if (zip_format) 
  {
    strncpy(SaveName,FileName,MAX_PATH);
    scan = strrchr(SaveName,'.');
    if (scan) *scan = '\0';
    update_save_name(SaveName);
    
    unzFile *zipfile = unzOpen(FileName);
    unz_global_info global_info;
    if ( unzGetGlobalInfo( zipfile, &global_info ) != UNZ_OK )
    {
      printf( "Could not read file global info\n" );
      unzClose( zipfile );
      return -1;
    }

        /* Get info about current file. */
        unz_file_info file_info;
        if ( unzGetCurrentFileInfo( zipfile, &file_info, filename, 512, NULL, 0, NULL, 0 ) != UNZ_OK )
        {
            printf( "Could not read file info\n" );
            unzClose( zipfile );
            return -1;
        }
        
    /* Entry is a file, so extract it. */
    if ( unzOpenCurrentFile( zipfile ) != UNZ_OK )
    {
      printf( "Could not open file\n" );
      unzClose( zipfile );
      return -1;
    }
    
    FILE *out = fopen("romtmp.rom", "wb" );
    if ( out == NULL )
    {
      printf( "could not open destination file\n" );
      unzCloseCurrentFile( zipfile );
      unzClose( zipfile );
      return -1;
    }
    
    error = UNZ_OK;
    do    
    {
      error = unzReadCurrentFile( zipfile, read_buffer, READ_SIZE );
      if ( error < 0 )
      {
        printf( "Error %d\n", error );
        unzCloseCurrentFile( zipfile );
        unzClose( zipfile );
        return -1;
      }
      if ( error > 0 )
      {
        /* Write data to file. */
        fwrite( read_buffer, error, 1, out );
      }
    } while ( error > 0 );

        fclose( out );
        unzCloseCurrentFile( zipfile );
    snprintf(CartA, sizeof(CartA), "./%s", "romtmp.rom");
    error = loc_load_rom(CartA);
    remove(CartA);
  } 

  // if (zip_format) {

  //   ExtractName = find_possible_filename_in_zip( FileName, "rom.mx1.mx2");

  //   if (ExtractName) {

  //     strncpy(SaveName, FileName, MAX_PATH);
  //     scan = strrchr(SaveName,'.');
  //     if (scan) *scan = '\0';
  //     update_save_name(SaveName);

  //     zip_buffer = extract_file_in_memory( FileName, ExtractName, &unzipped_size);

  //     if (zip_buffer) {
  //       strcpy(TmpFileName, MSX.msx_home_dir);
  //       strcat(TmpFileName, "/unzip.tmp");
  //       if (TmpFile = fopen( TmpFileName, "wb")) {
  //         fwrite(zip_buffer, unzipped_size, 1, TmpFile);
  //         fclose(TmpFile);
  //         error = loc_load_rom(TmpFileName);
  //         remove(TmpFileName);
  //       }
  //       free(zip_buffer);
  //     }
  //   }

  // }

   else {
    strncpy(SaveName,FileName,MAX_PATH);
    scan = strrchr(SaveName,'.');
    if (scan) *scan = '\0';
    update_save_name(SaveName);
    error = loc_load_rom(FileName);
  }

  if (! error ) {
    msx_kbd_load();
    msx_joy_load();
    msx_load_cheat();
    msx_load_settings();
  }

  return error;
}

int
msx_load_disk(char *FileName, int drive_id)
{
  char *scan;
  char  SaveName[MAX_PATH+1];
  int   error;

  error = 1;

  strncpy(SaveName,FileName,MAX_PATH);
  scan = strrchr(SaveName,'.');
  if (scan) *scan = '\0';
  update_save_name(SaveName);
  error = loc_load_disk(FileName, drive_id);

  if (! error ) {
    msx_kbd_load();
    msx_joy_load();
    msx_load_cheat();
    msx_load_settings();
  }

  return error;
}
int
msx_load_tape(char *FileName)
{
  char *scan;
  char  SaveName[MAX_PATH+1];
  int   error;

  error = 1;

  strncpy(SaveName,FileName,MAX_PATH);
  scan = strrchr(SaveName,'.');
  if (scan) *scan = '\0';
  update_save_name(SaveName);

  if (! error ) {
    msx_kbd_load();
    msx_joy_load();
    msx_load_cheat();
    msx_load_settings();
  }

  return error;
}

static int
loc_load_state(char *filename)
{
  int error;
  error = ! LoadState(filename);
  return error;
}

int
msx_load_state(char *FileName)
{
  char  SaveName[MAX_PATH+1];
  int   error;
  char *scan;

  error = 1;

  strncpy(SaveName,FileName,MAX_PATH);
  scan = strrchr(SaveName,'.');
  if (scan) *scan = '\0';
  update_save_name(SaveName);
  error = loc_load_state(FileName);

  if (! error ) {

    msx_kbd_load();
    msx_joy_load();
    msx_load_cheat();
    msx_load_settings();
  }

  return error;
}

static int
loc_msx_save_state(char *filename)
{
  int error;
  error = ! SaveState(filename);
  return error;
}

int
msx_snapshot_save_slot(int save_id)
{
  char        FileName[MAX_PATH+1];
  struct stat aStat;
  int         error;

  error = 1;

  if (save_id < MSX_MAX_SAVE_STATE) {
    snprintf(FileName, MAX_PATH, "%s/save/sav_%s_%d.sta", MSX.msx_home_dir, MSX.msx_save_name, save_id);
    error = loc_msx_save_state(FileName);
    if (! error) {
      if (! stat(FileName, &aStat)) {
        MSX.msx_save_state[save_id].used  = 1;
        MSX.msx_save_state[save_id].thumb = 0;
        MSX.msx_save_state[save_id].date  = aStat.st_mtime;
        snprintf(FileName, MAX_PATH, "%s/save/sav_%s_%d.png", MSX.msx_home_dir, MSX.msx_save_name, save_id);
        if (psp_sdl_save_thumb_png(MSX.msx_save_state[save_id].surface, FileName)) {
          MSX.msx_save_state[save_id].thumb = 1;
        }
      }
    }
  }

  return error;
}

int
msx_snapshot_load_slot(int load_id)
{
  char  FileName[MAX_PATH+1];
  int   error;

  error = 1;

  if (load_id < MSX_MAX_SAVE_STATE) {
    snprintf(FileName, MAX_PATH, "%s/save/sav_%s_%d.sta", MSX.msx_home_dir, MSX.msx_save_name, load_id);
    error = loc_load_state(FileName);
  }
  return error;
}

int
msx_snapshot_del_slot(int save_id)
{
  char        FileName[MAX_PATH+1];
  struct stat aStat;
  int         error;

  error = 1;

  if (save_id < MSX_MAX_SAVE_STATE) {
    snprintf(FileName, MAX_PATH, "%s/save/sav_%s_%d.sta", MSX.msx_home_dir, MSX.msx_save_name, save_id);
    error = remove(FileName);
    if (! error) {
      MSX.msx_save_state[save_id].used  = 0;
      MSX.msx_save_state[save_id].thumb = 0;
      memset(&MSX.msx_save_state[save_id].date, 0, sizeof(time_t));

      /* We keep always thumbnail with id 0, to have something to display in the file requester */
      if (save_id != 0) {
        snprintf(FileName, MAX_PATH, "%s/save/sav_%s_%d.png", MSX.msx_home_dir, MSX.msx_save_name, save_id);
        if (! stat(FileName, &aStat)) {
          remove(FileName);
        }
      }
    }
  }

  return error;
}

void
msx_treat_command_key(int msx_idx)
{
  int new_render;

  switch (msx_idx)
  {
    case MSXK_C_FPS: MSX.msx_view_fps = ! MSX.msx_view_fps;
    break;
    case MSXK_C_RENDER:
      psp_sdl_black_screen();
      new_render = MSX.msx_render_mode + 1;
      if (new_render > MSX_LAST_RENDER) new_render = 0;
      msx_change_render_mode(new_render);
    break;
    case MSXK_C_LOAD: psp_main_menu_load_current();
    break;
    case MSXK_C_SAVE: psp_main_menu_save_current();
    break;
    case MSXK_C_RESET:
       psp_sdl_black_screen();
       emulator_reset();
       reset_save_name();
    break;
    case MSXK_C_AUTOFIRE:
       kbd_change_auto_fire(! MSX.msx_auto_fire);
    break;
    case MSXK_C_DECFIRE:
      if (MSX.msx_auto_fire_period > 0) MSX.msx_auto_fire_period--;
    break;
    case MSXK_C_INCFIRE:
      if (MSX.msx_auto_fire_period < 19) MSX.msx_auto_fire_period++;
    break;
    case MSXK_C_SCREEN: psp_screenshot_mode = 10;
    break;
  }
}
