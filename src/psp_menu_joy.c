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
#include "psp_menu.h"
#include "psp_fmgr.h"
#include "psp_menu_kbd.h"
#include "psp_menu_set.h"
#include "psp_kbd.h"

extern SDL_Surface *back_surface;

enum {
  MENU_JOY_TA_FUNC,
 // MENU_JOY_ANALOG,
  MENU_JOY_AUTOFIRE_T,
  MENU_JOY_AUTOFIRE_M,
  MENU_JOY_LOAD,
  MENU_JOY_SAVE,
  MENU_JOY_RESET,
  // MENU_JOY_BACK,
  MAX_MENU_JOY_ITEM
};

  static menu_item_t menu_list[] =
  {
   // { "Swap Analog/Cursor :"},
    { "TA F5/UP           :"},
    { "Auto fire period   :"},
    { "Auto fire mode     :"},

    { "Load joystick"       },
    { "Save joystick"       },
    { "Reset joystick"      },

    // { "Back to Menu"        }
  };

  static int cur_menu_id = 0;

 // static int psp_reverse_analog    = 0;
  static enum msx_keys_emum bittboy_ta_func = MSXK_F5;
  static int msx_auto_fire_period = 0;
  static int msx_auto_fire_mode   = 0;
 

static void
psp_joystick_menu_reset(void);

static void
psp_joystick_menu_autofire(int step)
{
  if (step > 0) {
    if (msx_auto_fire_period < 19) msx_auto_fire_period++;
  } else {
    if (msx_auto_fire_period >  0) msx_auto_fire_period--;
  }
}

static void 
psp_display_screen_joystick_menu(void)
{
  char buffer[64];
  int menu_id = 0;
  int color   = 0;
  int x       = 10;
  int y       = 20;
  int y_step  = 10;

  psp_sdl_blit_help();
  
  for (menu_id = 0; menu_id < MAX_MENU_JOY_ITEM; menu_id++) {
    color = PSP_MENU_TEXT_COLOR;
    if (cur_menu_id == menu_id) color = PSP_MENU_SEL_COLOR;

    psp_sdl_back2_print(x, y, menu_list[menu_id].title, color);

    if (menu_id == MENU_JOY_TA_FUNC) {
      if (bittboy_ta_func == MSXK_F5) strcpy(buffer,"F5");
      else                            strcpy(buffer,"UP");
       string_fill_with_space(buffer, 4);
       psp_sdl_back2_print(140, y, buffer, color);
    } else
    if (menu_id == MENU_JOY_AUTOFIRE_T) {
      sprintf(buffer,"%d", msx_auto_fire_period+1);
      string_fill_with_space(buffer, 7);
      psp_sdl_back2_print(140, y, buffer, color);
    } else
    if (menu_id == MENU_JOY_AUTOFIRE_M) {
      if (msx_auto_fire_mode) strcpy(buffer,"yes");
      else                    strcpy(buffer,"no ");
      string_fill_with_space(buffer, 4);
      psp_sdl_back2_print(140, y, buffer, color);
      y += y_step;
    } else
    if (menu_id == MENU_JOY_RESET) {
      y += y_step;
    }

    y += y_step;
  }

  psp_menu_display_save_name();
}

static void
psp_joystick_menu_init(void)
{
  bittboy_ta_func      = MSX.bittboy_ta_func;
  msx_auto_fire_period = MSX.msx_auto_fire_period;
  msx_auto_fire_mode   = MSX.msx_auto_fire;
}

static void
psp_joystick_menu_validate(void)
{
  /* Validate */
  MSX.bittboy_ta_func  = bittboy_ta_func;
  MSX.msx_auto_fire_period = msx_auto_fire_period;
  if (msx_auto_fire_mode != MSX.msx_auto_fire) {
    kbd_change_auto_fire(msx_auto_fire_mode);
  }
}

static void
psp_joystick_menu_load(int format)
{
  int ret;

  ret = psp_fmgr_menu(format, 0);
  if (ret ==  1) /* load OK */
  {
    psp_display_screen_joystick_menu();
    psp_sdl_back2_print(180,  80, "File loaded !", 
                       PSP_MENU_NOTE_COLOR);
    psp_sdl_flip();
    sleep(1);
    psp_joystick_menu_init();
  }
  else 
  if (ret == -1) /* Load Error */
  {
    psp_display_screen_joystick_menu();
    psp_sdl_back2_print(180,  80, "Can't load file !", 
                       PSP_MENU_WARNING_COLOR);
    psp_sdl_flip();
    sleep(1);
  }
}

static void
psp_joystick_menu_save_config()
{
  int error;

  psp_joystick_menu_validate();
  error = msx_joy_save();

  if (! error) /* save OK */
  {
    psp_display_screen_joystick_menu();
    psp_sdl_back2_print(180, 80, "File saved !", 
                       PSP_MENU_NOTE_COLOR);
    psp_sdl_flip();
    sleep(1);
  }
  else 
  {
    psp_display_screen_joystick_menu();
    psp_sdl_back2_print(180, 80, "Can't save file !", 
                       PSP_MENU_WARNING_COLOR);
    psp_sdl_flip();
    sleep(1);
  }
}

static void
psp_joystick_menu_save()
{
  int error;

  psp_joystick_menu_validate();
  error = msx_joy_save();

  if (! error) /* save OK */
  {
    psp_display_screen_joystick_menu();
    psp_sdl_back2_print(180, 80, "File saved !", 
                       PSP_MENU_NOTE_COLOR);
    psp_sdl_flip();
    sleep(1);
  }
  else 
  {
    psp_display_screen_joystick_menu();
    psp_sdl_back2_print(180, 80, "Can't save file !", 
                       PSP_MENU_WARNING_COLOR);
    psp_sdl_flip();
    sleep(1);
  }
}

static void
psp_joystick_menu_reset(void)
{
  psp_display_screen_joystick_menu();
  psp_sdl_back2_print( 180, 80, "Reset joystick !", 
                     PSP_MENU_WARNING_COLOR);
  psp_sdl_flip();
  psp_joy_default_settings();
  psp_joystick_menu_init();
  sleep(1);
}

int 
psp_joystick_menu(void)
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

  psp_joystick_menu_init();

  while (! end_menu)
  {
    psp_display_screen_joystick_menu();
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
    if ((new_pad == GP2X_CTRL_LEFT ) || (new_pad == GP2X_CTRL_RIGHT))
    {
      int step = 1;
      if (new_pad & GP2X_CTRL_LEFT) step = -1;

      switch (cur_menu_id ) 
      {
        case MENU_JOY_AUTOFIRE_T  : psp_joystick_menu_autofire( step );
        break;   
        case MENU_JOY_TA_FUNC     : if (bittboy_ta_func == MSXK_F5) bittboy_ta_func = MSXK_JOY_UP; else bittboy_ta_func = MSXK_F5;
        break;                           
        case MENU_JOY_AUTOFIRE_M  : msx_auto_fire_mode = ! msx_auto_fire_mode;
        break;              
      }
    } else
    if ((new_pad == GP2X_CTRL_CIRCLE))
    {
      switch (cur_menu_id ) 
      {
        case MENU_JOY_LOAD       : psp_joystick_menu_load(FMGR_FORMAT_JOY);
                                   old_pad = new_pad = 0;
        break;              
        case MENU_JOY_SAVE       : psp_joystick_menu_save();
                                   old_pad = new_pad = 0;
        break;                     
        case MENU_JOY_RESET      : psp_joystick_menu_reset();
        break;                     
        // case MENU_JOY_BACK       : end_menu = 1;
        // break;                     
      }

    } else
    if(new_pad & GP2X_CTRL_UP) {

      if (cur_menu_id > 0) cur_menu_id--;
      else                 cur_menu_id = MAX_MENU_JOY_ITEM-1;

    } else
    if(new_pad & GP2X_CTRL_DOWN) {

      if (cur_menu_id < (MAX_MENU_JOY_ITEM-1)) cur_menu_id++;
      else                                     cur_menu_id = 0;

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
 
  if (end_menu > 0) {
    psp_joystick_menu_validate();
  }

  psp_kbd_wait_no_button();

  psp_sdl_clear_screen( PSP_MENU_BLACK_COLOR ); psp_sdl_flip();
  psp_sdl_clear_screen( PSP_MENU_BLACK_COLOR ); psp_sdl_flip();
  psp_sdl_clear_screen( PSP_MENU_BLACK_COLOR ); psp_sdl_flip();

  return 1;
}
