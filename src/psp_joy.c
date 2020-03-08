/*

Copyright 2005-2011 - Ludovic Jacomme - All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are
permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this list of
      conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice, this list
      of conditions and the following disclaimer in the documentation and/or other materials
      provided with the distribution.

THIS SOFTWARE IS PROVIDED BY LUDOVIC JACOMME 'AS IS'' AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL LUDOVIC JACOMME OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those of the
authors and should not be interpreted as representing official policies, either expressed
or implied, of Ludovic Jacomme.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "MSX.h"
#include "psp_joy.h"
#include "psp_kbd.h"

int
psp_joy_load_settings(char *joy_filename)
{
  FILE    *JoyFile;
  int      error = 0;
  
  JoyFile = fopen(joy_filename, "r");
  error   = 1;

  if (JoyFile != (FILE*)0) {
    psp_joy_load_settings_file(JoyFile);
    error = 0;
    fclose(JoyFile);
  }

  return error;
}

void
psp_joy_default_settings()
{
  MSX.msx_auto_fire         = 0;
  MSX.msx_auto_fire_period  = 10;
  MSX.msx_auto_fire_pressed = 0;
  MSX.bittboy_ta_func       = MSXK_F5;
}

int
psp_joy_load_settings_file(FILE *JoyFile)
{
  char     Buffer[512];
  char    *Scan;
  int      Value = 0;
  int      joy_id = 0;

  while (fgets(Buffer,512,JoyFile) != (char *)0) {
      
    Scan = strchr(Buffer,'\n');
    if (Scan) *Scan = '\0';
    /* For this #@$% of windows ! */
    Scan = strchr(Buffer,'\r');
    if (Scan) *Scan = '\0';
    if (Buffer[0] == '#') continue;

    Scan = strchr(Buffer,'=');
    if (! Scan) continue;
    
    *Scan = '\0';
    Value = atoi(Scan + 1);

    if (!strcasecmp(Buffer,"bittboy_ta_func"))      MSX.bittboy_ta_func = Value;
    else
    if (!strcasecmp(Buffer,"msx_auto_fire_period")) MSX.msx_auto_fire_period = Value;
  }

  return 0;
}

int
psp_joy_save_settings(char *joy_filename)
{
  FILE    *JoyFile;
  int      joy_id = 0;
  int      error = 0;

  JoyFile = fopen(joy_filename, "w");
  error   = 1;

  if (JoyFile != (FILE*)0) {

    fprintf( JoyFile, "bittboy_ta_func=%d\n"     , MSX.bittboy_ta_func);
    fprintf( JoyFile, "msx_auto_fire_period=%d\n", MSX.msx_auto_fire_period);

    error = 0;
    fclose(JoyFile);
  }

  return error;
}

