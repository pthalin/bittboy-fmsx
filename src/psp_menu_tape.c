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
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <fcntl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL/SDL.h>

#include "MSX.h"
#include "global.h"
#include "psp_sdl.h"
#include "psp_kbd.h"
#include "psp_menu_tape.h"
#include "psp_menu.h"
#include "psp_fmgr.h"
#include "psp_menu_set.h"



extern SDL_Surface *back_surface;

enum {
  MENU_TAPE_BLOAD,
  MENU_TAPE_CLOAD,
  MENU_TAPE_RUN_CAS,
  MAX_MENU_TAPE_ITEM
};

  static menu_item_t menu_list[] =
  {
    { "BLOAD\"CAS:\",R"},
    { "CLOAD"},
    { "RUN\"CAS:\""}
  };

static int cur_menu_id = 0;



static void 
psp_display_screen_tape_menu(void)
{
  char buffer[64];
  int menu_id = 0;
  int color   = 0;
  int x       = 10;
  int y       = 20;
  int y_step  = 10;

  psp_sdl_blit_help();
  
  for (menu_id = 0; menu_id < MAX_MENU_TAPE_ITEM; menu_id++) {
    color = PSP_MENU_TEXT_COLOR;
    if (cur_menu_id == menu_id) color = PSP_MENU_SEL_COLOR;

    psp_sdl_back2_print(x, y, menu_list[menu_id].title, color);

    y += y_step;
  }
  psp_menu_display_save_name();
}


int 
psp_tape_menu(void)
{
  gp2xCtrlData c;
  long        new_pad;
  long        old_pad;
  int         last_time;
  int         end_menu;



  psp_kbd_wait_no_button();

  old_pad   = 0;
  last_time = 0;
  end_menu  = 0;

  while (! end_menu)
  {
    psp_display_screen_tape_menu();
    psp_sdl_flip();

    while (1)
    {
      gp2xCtrlPeekBufferPositive(&c, 1);
      c.Buttons &= PSP_ALL_BUTTON_MASK;

      if (c.Buttons) break;
    }

    new_pad = c.Buttons;

    if ((old_pad != new_pad) || ((c.TimeStamp - last_time) > PSP_MENU_MIN_TIME)) {
      last_time = c.TimeStamp;
      old_pad = new_pad;

    } else continue;

    if ((c.Buttons & GP2X_CTRL_RTRIGGER) == GP2X_CTRL_RTRIGGER) {
      psp_settings_menu_reset();
      end_menu = 1;
    } else
    if ((new_pad == GP2X_CTRL_CIRCLE)) {
      switch (cur_menu_id ) {
        case MENU_TAPE_BLOAD   : psp_kbd_run_command("BLOAD\"CAS:\",R\x17");
			         old_pad = new_pad = 0;
			         end_menu = 1;
        break;   
        case MENU_TAPE_CLOAD   : psp_kbd_run_command("CLOAD\x17");
				 old_pad = new_pad = 0;
                                 end_menu = 1;
        break; 
        case MENU_TAPE_RUN_CAS : psp_kbd_run_command("RUN\"CAS:\"\x17");
                                 old_pad = new_pad = 0;
                                 end_menu = 1;
        break;                                        
      }
    }

    if(new_pad & GP2X_CTRL_UP) {
      if (cur_menu_id > 0) cur_menu_id--;
      else                 cur_menu_id = MAX_MENU_TAPE_ITEM-1;
    } else
    if(new_pad & GP2X_CTRL_DOWN) {
      if (cur_menu_id < (MAX_MENU_TAPE_ITEM-1)) cur_menu_id++;
      else                                      cur_menu_id = 0;
    } else  
    if(new_pad & GP2X_CTRL_SQUARE) {
      /* Cancel */
      end_menu = -1;
    } else 
    if((new_pad & GP2X_CTRL_CROSS) || (new_pad & GP2X_CTRL_SELECT)) {
      /* Back to MSX */
      end_menu = 1;
    }
  }
 

  psp_kbd_wait_no_button();

  psp_sdl_clear_screen( PSP_MENU_BLACK_COLOR ); psp_sdl_flip();
  psp_sdl_clear_screen( PSP_MENU_BLACK_COLOR ); psp_sdl_flip();
  psp_sdl_clear_screen( PSP_MENU_BLACK_COLOR ); psp_sdl_flip();

  return 1;
}
