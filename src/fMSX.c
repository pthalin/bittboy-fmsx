/** fMSX: portable MSX emulator ******************************/
/**                                                         **/
/**                          fMSX.c                         **/
/**                                                         **/
/** This file contains generic main() procedure statrting   **/
/** the emulation.                                          **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1994-2003                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/

#include <unistd.h>
#include "MSX.h"
#include "Help.h"

#include "SDL.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

extern char *Title;      /* Program title                       */
extern int   UseSound;   /* Sound mode (#ifdef SOUND)           */
extern int   UseSHM;     /* Use SHM X extension (#ifdef MITSHM) */
extern int   UseZoom;    /* Zoom factor (#ifdef UNIX)           */
extern int   SaveCPU;    /* Pause when inactive (#ifdef UNIX)   */
extern int   UseStatic;  /* Use static colors (MSDOS & UNIX)    */
extern int   SyncScreen; /* Sync screen updates (#ifdef MSDOS)  */
extern int   FullScreen; /* Use 256x240 screen (#ifdef MSDOS)   */
extern int   SndBufSize; /* Size of audio buffer (#ifdef SDL)   */

/** main() ***************************************************/
/** This is a main() function used in Unix and MSDOS ports. **/
/** It parses command line arguments, sets emulation        **/
/** parameters, and passes control to the emulation itself. **/
/*************************************************************/
int 
SDL_main(int argc,char *argv[])
{
  memset(&MSX, 0, sizeof(MSX_t));

  static char cfg_dir[MAX_PATH];
  snprintf(MSX.msx_home_dir, sizeof(MSX.msx_home_dir), "%s/.bittboy-msx", getenv("HOME")); mkdir(MSX.msx_home_dir, 0777);
  snprintf(cfg_dir, MAX_PATH, "%s/cht",  MSX.msx_home_dir); mkdir(cfg_dir, 0777);
  snprintf(cfg_dir, MAX_PATH, "%s/disk", MSX.msx_home_dir); mkdir(cfg_dir, 0777);
  snprintf(cfg_dir, MAX_PATH, "%s/joy",  MSX.msx_home_dir); mkdir(cfg_dir, 0777);
  snprintf(cfg_dir, MAX_PATH, "%s/kbd",  MSX.msx_home_dir); mkdir(cfg_dir, 0777);
  snprintf(cfg_dir, MAX_PATH, "%s/roms", MSX.msx_home_dir); mkdir(cfg_dir, 0777);
  snprintf(cfg_dir, MAX_PATH, "%s/save", MSX.msx_home_dir); mkdir(cfg_dir, 0777);
  snprintf(cfg_dir, MAX_PATH, "%s/scr",  MSX.msx_home_dir); mkdir(cfg_dir, 0777);
  snprintf(cfg_dir, MAX_PATH, "%s/set",  MSX.msx_home_dir); mkdir(cfg_dir, 0777);
  snprintf(cfg_dir, MAX_PATH, "%s/txt",  MSX.msx_home_dir); mkdir(cfg_dir, 0777);

  psp_sdl_init();

  if(!InitMachine(argc,argv)) {
    return(1);
  }

  psp_sdl_black_screen();

  StartMSX();
  TrashMSX();
  TrashMachine();

  psp_sdl_exit(0);

  return(0);
}
