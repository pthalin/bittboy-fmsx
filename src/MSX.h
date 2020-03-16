/** fMSX: portable MSX emulator ******************************/
/**                                                         **/
/**                           MSX.h                         **/
/**                                                         **/
/** This file contains declarations relevant to the drivers **/
/** and MSX emulation itself. See Z80.h for #defines        **/
/** related to Z80 emulation.                               **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1994-2003                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#ifndef MSX_H
#define MSX_H

#include "Z80.h"            /* Z80 CPU emulation             */
#include "V9938.h"          /* V9938 VDP opcode emulation    */
#include "AY8910.h"         /* AY8910 PSG emulation          */
#include "YM2413.h"         /* YM2413 OPLL emulation         */
#include "SCC.h"            /* Konami SCC chip emulation     */
#include "I8255.h"          /* Intel 8255 PPI emulation      */
#include "I8251.h"          /* Intel 8251 UART emulation     */

#include <stdio.h>
#include "global.h"

#define CPU_CLOCK    3580        /* CPU clock frequency, kHz */
#define VDP_CLOCK    21480       /* VDP clock frequency, kHz */

#define HPERIOD      1368        /* HPeriod, VDP cycles      */
#define VPERIOD_PAL  (HPERIOD*313) /* PAL VPeriod, VDP ccls  */
#define VPERIOD_NTSC (HPERIOD*262) /* NTSC VPeriod, VDP ccls */
#define HREFRESH_240 960         /* 240dot scanline refresh  */
#define HREFRESH_256 1024        /* 256dot scanline refresh  */

#define CPU_VPERIOD  (VPERIOD_NTSC/6)
#define CPU_V262     (VPERIOD_NTSC/6)
#define CPU_V313     (VPERIOD_PAL/6)
#define CPU_HPERIOD  (HPERIOD/6)
#define CPU_H240     (HREFRESH_240/6)
#define CPU_H256     (HREFRESH_256/6)

#define INT_IE0     0x01
#define INT_IE1     0x02
#define INT_IE2     0x04

#define PAGESIZE    0x4000L /* Size of a RAM page            */
#define NORAM       0xFF    /* Byte to be returned from      */
                            /* non-existing pages and ports  */
#define MAXSCREEN   12      /* Highest screen mode supported */
#define MAXSPRITE1  4       /* Sprites/line in SCREEN 1-3    */
#define MAXSPRITE2  8       /* Sprites/line in SCREEN 4-8    */
#define MAXDRIVES   2       /* Number of disk drives         */
#define MAXDISKS    32      /* Number of disks for a drive   */
#define MAXMAPPERS  8       /* Total defined MegaROM mappers */

#define MAXCHANNELS (AY8910_CHANNELS+YM2413_CHANNELS)
  /* Number of sound channels used by the emulation */

/** Following macros can be used in screen drivers ***********/
#define BigSprites    (VDP[1]&0x01)   /* Zoomed sprites      */
#define Sprites16x16  (VDP[1]&0x02)   /* 16x16/8x8 sprites   */
#define ScreenON      (VDP[1]&0x40)   /* Show screen         */
#define SpritesOFF    (VDP[8]&0x02)   /* Don't show sprites  */
#define SolidColor0   (VDP[8]&0x20)   /* Solid/Tran. COLOR 0 */
# if 0 //LUDO:
#define PALVideo      (VDP[9]&0x02)   /* PAL/NTSC video      */
# else
#define PALVideo      (!MSX.msx_ntsc)
# endif
#define FlipEvenOdd   (VDP[9]&0x04)   /* Flip even/odd pages */
#define InterlaceON   (VDP[9]&0x08)   /* Interlaced screen   */
#define ScanLines212  (VDP[9]&0x80)   /* 212/192 scanlines   */
#define HScroll512    (VDP[25]&0x01)  /* HScroll both pages  */
#define MaskEdges     (VDP[25]&0x02)  /* Mask 8-pixel edges  */
#define ModeYJK       (VDP[25]&0x08)  /* YJK screen mode     */
#define ModeYAE       (VDP[25]&0x10)  /* YJK/YAE screen mode */
#define VScroll       VDP[23]
#define HScroll       ((VDP[27]&0x07)|((int)(VDP[26]&0x3F)<<3))
#define VAdjust       (-((signed char)(VDP[18])>>4))
#define HAdjust       (-((signed char)(VDP[18]<<4)>>4))
/*************************************************************/

/** Variables used to control emulator behavior **************/
extern byte Verbose;                  /* Debug msgs ON/OFF   */
extern int  ROMTypeA,ROMTypeB;        /* MegaROM types       */
extern int  VRAMPages;       /* Number of RAM pages */
extern int  VPeriod;                  /* CPU cycles / VBlank */
extern int  HPeriod;                  /* CPU cycles / HBlank */
extern byte JoyTypeA,JoyTypeB;        /* 0=No,1=Jstk,2/3=Mse */
extern byte AutoFire;                 /* Autofire on [SPACE] */
extern byte UseDrums;                 /* Drums for PSG noise */
/*************************************************************/

extern Z80  CPU;                      /* CPU state/registers */
extern byte *VRAM;                    /* Video RAM           */
extern byte VDP[64];                  /* VDP control reg-ers */
extern byte VDPStatus[16];            /* VDP status reg-ers  */
extern byte *ChrGen,*ChrTab,*ColTab;  /* VDP tables (screen) */
extern byte *SprGen,*SprTab;          /* VDP tables (sprites)*/
extern int  ChrGenM,ChrTabM,ColTabM;  /* VDP masks (screen)  */
extern int  SprTabM;                  /* VDP masks (sprites) */
extern byte FGColor,BGColor;          /* Colors              */
extern byte XFGColor,XBGColor;        /* Alternative colors  */
extern byte ScrMode;                  /* Current screen mode */
extern int  ScanLine;                 /* Current scanline    */

extern byte KeyMap[16];               /* Keyboard map        */
extern byte ExitNow;                  /* 1: Exit emulator    */

extern byte PSLReg;                   /* Primary slot reg.   */
extern byte SSLReg;                   /* Secondary slot reg. */
extern byte RCounter;        /* Counter for Z80's R register */

extern char *ProgDir;                 /* Program directory   */
extern char CartA[256];                   /* Cartridge A ROM file*/
extern char CartB[256];                   /* Cartridge B ROM file*/
extern char DiskA[256];                   /* Drive A disk image  */
extern char DiskB[256];                   /* Drive B disk image  */
extern char ZipFile[256];                   /* Cartridge A ROM file*/
extern char *SndName;                 /* Soundtrack log file */
extern char *PrnName;                 /* Printer redir. file */
extern char CasName[256];                 /* Tape image file     */
extern char *ComName;                 /* Serial redir. file  */
extern char *StateName;               /* State save name     */

extern int skip_disk_rom;

extern FILE *CasStream;               /* Cassette I/O stream */

extern char *FontName;                /* Font file for text  */
extern byte *FontBuf;                 /* Font for text modes */
extern byte UseFont;                  /* 1: Use external font*/

# if 0
extern int   Use2413;    /* MSX-MUSIC emulation (1-yes) */
extern int   Use8950;    /* MSX-AUDIO emulation (1-yes) */
# else
# define Use2413    MSX.msx_use_2413
# define Use8950    MSX.msx_use_8950
# endif
extern int   UseStereo;  /* Stereo sound (1-yes)        */
extern int   UseFilter;  /* Filter mode                 */

/** StartMSX() ***********************************************/
/** Allocate memory, load ROM image, initialize hardware,   **/
/** CPU and start the emulation. This function returns 0 in **/
/** the case of failure.                                    **/
/*************************************************************/
int StartMSX(void);

/** TrashMSX() ***********************************************/
/** Free memory allocated by StartMSX().                    **/
/*************************************************************/
void TrashMSX(void);

/** SaveState() **********************************************/
/** Save emulation state to a .STA file.                    **/
/*************************************************************/
int SaveState(const char *FileName);

/** LoadState() **********************************************/
/** Load emulation state from a .STA file.                  **/
/*************************************************************/
int LoadState(const char *FileName);

/** ChangeDisk() *********************************************/
/** Change disk image in a given drive. Closes current disk **/
/** image if Name=0 was given. Returns 1 on success or 0 on **/
/** failure. This function is part of generic disk drivers  **/
/** in Disk.c. It is compiled when DISK is #defined.        **/
/*************************************************************/
byte ChangeDisk(byte ID,char *Name);

/** InitMachine() ********************************************/
/** Allocate resources needed by the machine-dependent code.**/
/************************************ TO BE WRITTEN BY USER **/
int InitMachine(int argc, char* argv[]);

/** TrashMachine() *******************************************/
/** Deallocate all resources taken by InitMachine().        **/
/************************************ TO BE WRITTEN BY USER **/
void TrashMachine(void);

/** Keyboard() ***********************************************/
/** This function is periodically called to poll keyboard.  **/
/************************************ TO BE WRITTEN BY USER **/
void Keyboard(void);

/** Joystick() ***********************************************/
/** Query position of a joystick connected to port N.       **/
/** Returns 0.0.F2.F1.R.L.D.U.                              **/
/************************************ TO BE WRITTEN BY USER **/
byte Joystick(byte N);

/** Mouse() **************************************************/
/** Query coordinates of a mouse connected to port N.       **/
/** Returns F2.F1.Y.Y.Y.Y.Y.Y.Y.Y.X.X.X.X.X.X.X.X.          **/
/************************************ TO BE WRITTEN BY USER **/
int Mouse(byte N);

/** DiskPresent()/DiskRead()/DiskWrite() *********************/
/*** These three functions are called to check for floppyd  **/
/*** disk presence in the "drive", and to read/write given  **/
/*** sector to the disk.                                    **/
/************************************ TO BE WRITTEN BY USER **/
extern byte DiskPresent(byte ID);
extern byte DiskRead(byte ID,byte *Buf,int N);
extern byte DiskWrite(byte ID,byte *Buf,int N);

/** SetColor() ***********************************************/
/** Set color N (0..15) to (R,G,B).                         **/
/************************************ TO BE WRITTEN BY USER **/
void SetColor(byte N,byte R,byte G,byte B);

/** RefreshScreen() ******************************************/
/** Refresh screen. This function is called in the end of   **/
/** refresh cycle to show the entire screen.                **/
/************************************ TO BE WRITTEN BY USER **/
void RefreshScreen(void);

/** RefreshLine#() *******************************************/
/** Refresh line Y (0..191/211), on an appropriate SCREEN#, **/
/** including sprites in this line.                         **/
/************************************ TO BE WRITTEN BY USER **/
void RefreshLineTx80(byte Y);
void RefreshLine0(byte Y);
void RefreshLine1(byte Y);
void RefreshLine2(byte Y);
void RefreshLine3(byte Y);
void RefreshLine4(byte Y);
void RefreshLine5(byte Y);
void RefreshLine6(byte Y);
void RefreshLine7(byte Y);
void RefreshLine8(byte Y);
void RefreshLine10(byte Y);
void RefreshLine12(byte Y);

void HiresRefreshLineTx80(byte Y);
void HiresRefreshLine0(byte Y);
void HiresRefreshLine1(byte Y);
void HiresRefreshLine2(byte Y);
void HiresRefreshLine3(byte Y);
void HiresRefreshLine4(byte Y);
void HiresRefreshLine5(byte Y);
void HiresRefreshLine6(byte Y);
void HiresRefreshLine7(byte Y);
void HiresRefreshLine8(byte Y);
void HiresRefreshLine10(byte Y);
void HiresRefreshLine12(byte Y);

/** SetLowres() **********************************************/
/** Switch to 320x240 graphics driver.                      **/
/*************************************************************/
void SetLowres();

/** ResetMSX() ***********************************************/
/** Reset MSX emulation.                                    **/
/*************************************************************/
void ResetMSX(int hard);

/** RewindTape() *********************************************/
/** Set tape filepointer to beginning of file.              **/
/*************************************************************/
void RewindTape();

/////
int msx_load_file_cheat(char *FileName);
int msx_load_file_settings(char *FileName);
int msx_load_disk(char *FileName, int drive_id);
int msx_load_tape(char *FileName);
int msx_load_rom(char *FileName, int zip_format);
int msx_load_state(char *FileName);
char* load_comment_if_exists(char *Name);
int load_thumb_if_exists(char *Name);
void load_comment_list();
void load_thumb_list();
int msx_save_cheat(void);
int msx_joy_save(void);
void msx_joy_load(void);
void msx_default_settings(void);
int msx_save_settings(void);
# if !defined(CAANOO_MODE)
void myPowerSetClockFrequency(int cpu_clock);
#endif
int msx_eject_rom(void);
int msx_snapshot_del_slot(int save_id);
int msx_snapshot_save_slot(int save_id);
int msx_snapshot_load_slot(int load_id);
void emulator_reset(void);
void msx_treat_command_key(int msx_idx);
void msx_apply_cheats();
int msx_load_cheat();
int InitMSX(void);
void msx_kbd_load(void);
int msx_load_settings(void);
void update_save_name(char *Name);

//Implemented in SDL files
void msx_change_render_mode(int new_render_mode);
void audio_resume(void);
void audio_pause(void);
void msx_save_back_to_blit(void);
void MsxKeyUp(int index, int bit_mask);
void MsxKeyDown(int index, int bit_mask);
void MsxJoyUp(int bit_mask);
void MsxJoyDown(int bit_mask);

#endif /* MSX_H */
