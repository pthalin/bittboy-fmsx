/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                         SDLsnd.c                        **/
/**                                                         **/
/** This file contains standard sound generation routines   **/
/** for the SDL library.                                    **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1996-2000                 **/
/**               Vincent van Dam 2002                      **/
/**    	                                                    **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/

#include "Sound.h"

#include "SDL.h"

#include "fmopl.h"
#include "emu2149.h"
#include "emu2212.h"
#include "emu2413.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "global.h"

#define MSX_CLK 3579545

int UseStereo    = 0;     /* Stereo sound (1=enable)         */
# if 0 //LUDO:
int Use2413      = 0;     /* MSX-MUSIC emulation (1=enable)  */
int Use8950      = 0;     /* MSX-AUDIO emulation (1=enable)  */
# else
# define Use2413    MSX.msx_use_2413
# define Use8950    MSX.msx_use_8950
# endif
int SndBufSize   = 512;   /* Buffer size for audio emulation */

# if 0
float FactorPSG  = 1.00;  /* Volume percentage of PSG        */
float FactorSCC  = 1.00;  /* Volume percentage of SCC        */
float Factor2413 = 1.00;  /* Volume percentage of MSX-MUSIC  */
float Factor8950 = 0.25;  /* Volume percentage of MSX-AUDIO  */
# endif

static int Verbose = 0;   /* Verbose messaging flag          */

static OPLL   *opll;
static FM_OPL *fm_opl;
static PSG    *psg;
static SCC    *scc;


/** DSPLoop() ************************************************/
/** Main loop of the sound server.                          **/
/*************************************************************/
static char sound_buffer[2048];
static void DSPCallBack(void* unused, UINT8 *stream, int len)
{
  register int   J;
  register INT16 P,O,A,S;
  int R1,R2;

  if (MSX.msx_snd_enable) {
    for(J=0;J<len;J+=4)
    {
      P=PSG_calc(psg);
      S=SCC_calc(scc);
      O=Use2413? OPLL_calc(opll): 0;
      A=Use8950? Y8950UpdateOne(fm_opl): 0;
      if (UseStereo) {
        //R1=P+ (A >> 4)+S;
        R1=P+A+S;
        R2=O+S;
      } else {
        //R1=R2=P+O+(A >> 4)+S;
        R1=R2=P+O+A+S;
      }
      R1 = R1 * MSX.psp_sound_volume;
      R2 = R2 * MSX.psp_sound_volume;
      sound_buffer[J+0]=R2&0x00FF;
      sound_buffer[J+1]=R2>>8;
      sound_buffer[J+2]=R1&0x00FF;
      sound_buffer[J+3]=R1>>8;
    }
    long volume = (SDL_MIX_MAXVOLUME * gp2xGetSoundVolume()) / 100;
    SDL_MixAudio((signed char *)stream, (unsigned char *)sound_buffer, len, volume);
  } else {
    memset(stream, 0, len);
  }
}

/** OpenSoundDevice() ****************************************/
/** Open sound device with a given level of sound quality.  **/
/** Returns 0 if failed or sound quality (Mode).            **/
/*************************************************************/
static int OpenSoundDevice(int Rate)
{
  /* Open the audio device */
  SDL_AudioSpec *desired, *obtained;

  /* Allocate a desired SDL_AudioSpec */
  desired=(SDL_AudioSpec*)malloc(sizeof(SDL_AudioSpec));

  /* Allocate space for the obtained SDL_AudioSpec */
  obtained=(SDL_AudioSpec*)malloc(sizeof(SDL_AudioSpec));

  /* set audio parameters */
  desired->freq=Rate;
  desired->format=AUDIO_S16LSB; /* 16-bit signed audio */
  desired->samples=SndBufSize;  /* size audio buffer */
  desired->channels=2;

  /* Our callback function */
  desired->callback=DSPCallBack;
  desired->userdata=NULL;

  /* Open the audio device */
  if(SDL_OpenAudio(desired, obtained)<0) return 0;

  Rate=obtained->freq;

  /* Adjust buffer size to the obtained buffer size */ 
  SndBufSize=obtained->samples;
  if(Verbose) printf("OK\n  Obtained buffer of %d bytes...", SndBufSize);

  gp2xInitSoundVolume();

  //LUDO:
  SDL_Delay(1000);        // Give sound some time to init

  /* Start playing */
  SDL_PauseAudio(0);

  /* Free memory */
  free(obtained);
  free(desired);

  if(Verbose) puts("OK");
  return(Rate);
}

/** InitSound() **********************************************/
/** Initialize DSP. Returns Rate on success, 0 otherwise.   **/
/** Mode is 0 to skip initialization (will be silent).      **/
/*************************************************************/
int InitSound(int Rate,int Verb)
{
  Verbose=Verb;
  if(Rate<=0) return(0);
  if(Rate<8192) Rate=44100;

  /* MSX-MUSIC emulation */
  OPLL_init(MSX_CLK,Rate);
  opll=OPLL_new();
  OPLL_reset(opll);
  OPLL_reset_patch(opll,0);

  /* MSX-AUDIO emulation */
  fm_opl=OPLCreate(OPL_TYPE_Y8950,MSX_CLK,Rate,256);
  OPLResetChip(fm_opl);

  /* PSG/SCC emulation */
  PSG_init(MSX_CLK,Rate);
  psg=PSG_new();
  PSG_reset(psg);
  PSG_setVolumeMode(psg,2);

  SCC_init(MSX_CLK,Rate);
  scc=SCC_new();
  SCC_reset(scc);

  /* Open sound device */
  if(Verbose) printf("OK\n  Opening sound device...");
  if(!(Rate=OpenSoundDevice(Rate))) return(0);

  return Rate;
}

/** ResetSound() *********************************************/
/** Reset sound chips.                                      **/
/*************************************************************/
void ResetSound()
{
  PSG_reset(psg);
  SCC_reset(scc);
  OPLL_reset(opll);
  OPLResetChip(fm_opl);
}

/** TrashSound() *********************************************/
/** Shut DSP down.                                          **/
/*************************************************************/
void TrashSound(void)
{
  SDL_CloseAudio();
  
  /* clean up msxmusic */
  OPLL_close();
  OPLL_delete(opll);
  
  /* clean up msxaudio */
  OPLDestroy(fm_opl);
  
  /* clean up psg/scc */
  PSG_delete(psg);
  SCC_delete(scc);
}

/* wrapper functions to actual sound emulation */
void WriteOPLL (int R,int V) { OPLL_writeReg(opll,R,V); }
void WriteAUDIO(int R,int V) { OPLWrite(fm_opl,R,V); }
void Write2212 (int R,int V) { SCC_write(scc,R,V); }
void WritePSG  (int R,int V) { PSG_writeReg(psg,R,V); }
int  ReadAUDIO (int R)       { return OPLRead(fm_opl,R); }
int  ReadPSG   (int R)       { return PSG_readReg(psg,R); }

void 
audio_pause(void)
{
  SDL_PauseAudio(1);
}

void 
audio_resume(void)
{
  if (MSX.msx_snd_enable) {
    SDL_PauseAudio(0);
  }
}

void 
audio_shutdown(void)
{
  SDL_CloseAudio();
}

