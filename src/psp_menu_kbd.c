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

#include "global.h"
#include "psp_sdl.h"
#include "psp_kbd.h"
#include "psp_menu.h"
#include "psp_fmgr.h"
#include "psp_menu_kbd.h"
#include "psp_danzeff.h"

enum {
  MENU_KBD_SKIN,
  MENU_KBD_KBD_SELECT,
  MENU_KBD_UP,
  MENU_KBD_DOWN,
  MENU_KBD_LEFT,
  MENU_KBD_RIGHT,
  MENU_KBD_CROSS,
  MENU_KBD_SQUARE,
  MENU_KBD_TRIANGLE,
  MENU_KBD_CIRCLE,
  MENU_KBD_LTRIGGER,
  MENU_KBD_RTRIGGER,
  MENU_KBD_FIRE,
  MENU_KBD_JOY_UP,
  MENU_KBD_JOY_DOWN,
  MENU_KBD_JOY_LEFT,
  MENU_KBD_JOY_RIGHT,
  MENU_KBD_LOAD,
  MENU_KBD_SAVE,
  MENU_KBD_HOTKEYS,
  MENU_KBD_RESET,
  MENU_KBD_BACK,
  MAX_MENU_KBD_ITEM
};

  static menu_item_t menu_list[] =
  { 
   { "Skin     :" },
   { "Mapping  :" },

   { "Up       :" },
   { "Down     :" },
   { "Left     :" },
   { "Right    :" },
# if defined(DINGUX_MODE)
   { "B        :" },
   { "Y        :" },
   { "X        :" },
   { "A        :" },
# else
   { "X        :" },
   { "A        :" },
   { "Y        :" },
   { "B        :" },
# endif
   { "LTrigger :" },
   { "RTrigger :" },
   { "JoyFire  :" },
   { "JoyUp    :" },
   { "JoyDown  :" },
   { "JoyLeft  :" },
   { "JoyRight :" },

   { "Load Keyboard" },
   { "Save Keyboard" },
   { "Set Hotkeys" },
   { "Reset Keyboard" },
   { "Back to Menu" }
  };

  static int cur_menu_id = MENU_KBD_BACK;

  static int loc_kbd_mapping[ KBD_ALL_BUTTONS ];
  static int loc_kbd_mapping_L[ KBD_ALL_BUTTONS ];
  static int loc_kbd_mapping_R[ KBD_ALL_BUTTONS ];

  int menu_kbd_selected = -1;
  
static int
psp_kbd_menu_id_to_key_id(int menu_id)
{
  int kbd_id = 0;

  switch ( menu_id ) 
  {
    case MENU_KBD_UP        : kbd_id = KBD_UP;        break;
    case MENU_KBD_DOWN      : kbd_id = KBD_DOWN;      break;
    case MENU_KBD_LEFT      : kbd_id = KBD_LEFT;      break;
    case MENU_KBD_RIGHT     : kbd_id = KBD_RIGHT;     break;
    case MENU_KBD_TRIANGLE  : kbd_id = KBD_TRIANGLE;  break;
    case MENU_KBD_CROSS     : kbd_id = KBD_CROSS;     break;
    case MENU_KBD_SQUARE    : kbd_id = KBD_SQUARE;    break;
    case MENU_KBD_CIRCLE    : kbd_id = KBD_CIRCLE;    break;
    case MENU_KBD_LTRIGGER  : kbd_id = KBD_LTRIGGER;  break;
    case MENU_KBD_RTRIGGER  : kbd_id = KBD_RTRIGGER;  break;
    case MENU_KBD_FIRE      : kbd_id = KBD_FIRE;      break;
    case MENU_KBD_JOY_UP    : kbd_id = KBD_JOY_UP;    break;
    case MENU_KBD_JOY_DOWN  : kbd_id = KBD_JOY_DOWN;  break;
    case MENU_KBD_JOY_LEFT  : kbd_id = KBD_JOY_LEFT;  break;
    case MENU_KBD_JOY_RIGHT : kbd_id = KBD_JOY_RIGHT; break;
  }
  return kbd_id;
}

static void 
psp_display_screen_kbd_menu(void)
{
  char buffer[32];
  char *scan;
  int menu_id = 0;
  int kbd_id  = 0;
  int msx_key = 0;
  int color   = 0;
  int x       = 0;
  int y       = 0;
  int y_step  = 0;

  psp_sdl_blit_help();

  x      = 10;
  y      = 20;
  y_step = 10;
  
  for (menu_id = 0; menu_id < MAX_MENU_KBD_ITEM; menu_id++, y += y_step) 
  {
    color = PSP_MENU_TEXT_COLOR;
    if (cur_menu_id == menu_id) color = PSP_MENU_SEL_COLOR;
    // else 
    // if (menu_id == MENU_KBD_KBD_SELECT) color = PSP_MENU_GREEN_COLOR;
    // else                                color = PSP_MENU_TEXT_COLOR;

    psp_sdl_back2_print(x, y, menu_list[menu_id].title, color);

    if (menu_id == MENU_KBD_SKIN) {
      snprintf(buffer, 30, psp_kbd_skin_dir[psp_kbd_skin]);
      scan = strchr(buffer, '/');
      if (scan) *scan = 0;
      psp_sdl_back2_print(80, y, buffer, color);
    } else
    if (menu_id == MENU_KBD_KBD_SELECT) {

      if (menu_kbd_selected == -1) sprintf(buffer, "standard");
      else
      if (menu_kbd_selected == KBD_LTRIGGER_MAPPING) sprintf(buffer, "left");
      else
      if (menu_kbd_selected == KBD_RTRIGGER_MAPPING) sprintf(buffer, "right");

      string_fill_with_space(buffer, 20);
      psp_sdl_back2_print(80, y, buffer, color);

    } else
    if ((menu_id >= MENU_KBD_UP       ) && 
        (menu_id <= MENU_KBD_JOY_RIGHT)) 
    {
      kbd_id  = psp_kbd_menu_id_to_key_id(menu_id);

      if (menu_kbd_selected == KBD_NORMAL_MAPPING  ) msx_key = loc_kbd_mapping[kbd_id];
      else
      if (menu_kbd_selected == KBD_LTRIGGER_MAPPING) msx_key = loc_kbd_mapping_L[kbd_id];
      else
      if (menu_kbd_selected == KBD_RTRIGGER_MAPPING) msx_key = loc_kbd_mapping_R[kbd_id];

      if ((msx_key >= 0) && (msx_key < MSXK_MAX_KEY)) {
        strcpy(buffer, psp_msx_key_info[msx_key].name);
      } else 
      if (msx_key == KBD_UNASSIGNED) {
        sprintf(buffer, "UNASSIGNED");
      } else
      if (msx_key == KBD_LTRIGGER_MAPPING) {
        sprintf(buffer, "L MAPPING");
      } else
      if (msx_key == KBD_RTRIGGER_MAPPING) {
        sprintf(buffer, "R MAPPING");
      } else {
        sprintf(buffer, "KEY %d", msx_key);
      }
      string_fill_with_space(buffer, 12);
      psp_sdl_back2_print(80, y, buffer, color);

      // if (menu_id == MENU_KBD_JOY_RIGHT) {
      //   y += y_step;
      // }
    }
  }

  psp_menu_display_save_name();
}

static void
psp_keyboard_menu_update_lr(void)
{
  int kbd_id;

  for (kbd_id = 0; kbd_id < KBD_ALL_BUTTONS; kbd_id++) {
    if (loc_kbd_mapping[kbd_id] == KBD_LTRIGGER_MAPPING) {
      loc_kbd_mapping_L[kbd_id] = KBD_LTRIGGER_MAPPING;
      loc_kbd_mapping_R[kbd_id] = KBD_LTRIGGER_MAPPING;
    } else 
    if (loc_kbd_mapping[kbd_id] == KBD_RTRIGGER_MAPPING) {
      loc_kbd_mapping_L[kbd_id] = KBD_RTRIGGER_MAPPING;
      loc_kbd_mapping_R[kbd_id] = KBD_RTRIGGER_MAPPING;
    }
  }
}

static void
psp_keyboard_menu_reset_kbd(void)
{
  psp_display_screen_kbd_menu();
  psp_sdl_back2_print(180, 80, "Reset Keyboard !", 
                     PSP_MENU_WARNING_COLOR);
  psp_sdl_flip();
  psp_kbd_default_settings();
  sleep(1);

  memcpy(loc_kbd_mapping, psp_kbd_mapping, sizeof(psp_kbd_mapping));
  memcpy(loc_kbd_mapping_L, psp_kbd_mapping_L, sizeof(psp_kbd_mapping_L));
  memcpy(loc_kbd_mapping_R, psp_kbd_mapping_R, sizeof(psp_kbd_mapping_R));
}

static void
psp_keyboard_menu_hotkeys(void)
{
  psp_display_screen_kbd_menu();
  psp_sdl_back2_print(180, 80, "Set Hotkeys !", 
                     PSP_MENU_WARNING_COLOR);
  psp_sdl_flip();
  psp_kbd_reset_hotkeys();
  sleep(1);

  memcpy(loc_kbd_mapping, psp_kbd_mapping, sizeof(psp_kbd_mapping));
  memcpy(loc_kbd_mapping_L, psp_kbd_mapping_L, sizeof(psp_kbd_mapping_L));
  memcpy(loc_kbd_mapping_R, psp_kbd_mapping_R, sizeof(psp_kbd_mapping_R));
}

static void
psp_keyboard_menu_load()
{
  int ret;

  ret = psp_fmgr_menu(FMGR_FORMAT_KBD, 0);
  if (ret ==  1) /* load OK */
  {
    psp_display_screen_kbd_menu();
    psp_sdl_back2_print(180, 80, "File loaded !", 
                       PSP_MENU_NOTE_COLOR);
    psp_sdl_flip();
    sleep(1);
  }
  else 
  if (ret == -1) /* Load Error */
  {
    psp_display_screen_kbd_menu();
    psp_sdl_back2_print(180, 80, "Can't load file !", 
                       PSP_MENU_WARNING_COLOR);
    psp_sdl_flip();
    sleep(1);
  }

  memcpy(loc_kbd_mapping  , psp_kbd_mapping, sizeof(psp_kbd_mapping));
  memcpy(loc_kbd_mapping_L, psp_kbd_mapping_L, sizeof(psp_kbd_mapping_L));
  memcpy(loc_kbd_mapping_R, psp_kbd_mapping_R, sizeof(psp_kbd_mapping_R));
}

static void
psp_keyboard_menu_mapping(int kbd_id, int step)
{
  int *ploc_kbd_mapping;

  ploc_kbd_mapping = loc_kbd_mapping;
  
  if (menu_kbd_selected == KBD_LTRIGGER_MAPPING) {
    ploc_kbd_mapping = loc_kbd_mapping_L;
  } else
  if (menu_kbd_selected == KBD_RTRIGGER_MAPPING) {
    ploc_kbd_mapping = loc_kbd_mapping_R;
  }

  if (step < 0) ploc_kbd_mapping[kbd_id]--;
  else 
  if (step > 0) ploc_kbd_mapping[kbd_id]++;

  if (ploc_kbd_mapping[kbd_id] <  -3) ploc_kbd_mapping[kbd_id] = MSXK_MAX_KEY-1;
  else
  if (ploc_kbd_mapping[kbd_id] >= MSXK_MAX_KEY) ploc_kbd_mapping[kbd_id] = -3;
}

static void
psp_keyboard_menu_save()
{
  int error;

  psp_keyboard_menu_update_lr();

  memcpy(psp_kbd_mapping  , loc_kbd_mapping  , sizeof(psp_kbd_mapping));
  memcpy(psp_kbd_mapping_L, loc_kbd_mapping_L, sizeof(psp_kbd_mapping_L));
  memcpy(psp_kbd_mapping_R, loc_kbd_mapping_R, sizeof(psp_kbd_mapping_R));

  error = msx_kbd_save();

  if (! error) /* save OK */
  {
    psp_display_screen_kbd_menu();
    psp_sdl_back2_print(180, 80, "File saved !", 
                       PSP_MENU_NOTE_COLOR);
    psp_sdl_flip();
    sleep(1);
  }
  else 
  {
    psp_display_screen_kbd_menu();
    psp_sdl_back2_print(180, 80, "Can't save file !", 
                       PSP_MENU_WARNING_COLOR);
    psp_sdl_flip();
    sleep(1);
  }
}

static void
psp_keyboard_select_change(int step)
{
  if (step > 0) {
    if (menu_kbd_selected == KBD_RTRIGGER_MAPPING) menu_kbd_selected = -1;
    else                                           menu_kbd_selected--;
  } else {
    if (menu_kbd_selected == -1) menu_kbd_selected = KBD_RTRIGGER_MAPPING;
    else                         menu_kbd_selected++;
  }
  psp_keyboard_menu_update_lr();
}

static void
psp_keyboard_menu_skin(int step)
{
  if (step > 0) {
    if (psp_kbd_skin < psp_kbd_last_skin) psp_kbd_skin++;
    else                                  psp_kbd_skin = 0;
  } else {
    if (psp_kbd_skin > 0) psp_kbd_skin--;
    else                  psp_kbd_skin = psp_kbd_last_skin;
  }

  danzeff_change_skin();
}

void 
psp_keyboard_menu(void)
{
  gp2xCtrlData c;
  long new_pad;
  long old_pad;
  int  last_time;
  int  end_menu;
  int  kbd_id;
  int  msx_key;
  int  danzeff_mode;
  int  danzeff_key;

  psp_kbd_wait_no_button();

  old_pad      = 0;
  last_time    = 0;
  end_menu     = 0;
  kbd_id       = 0;

  danzeff_key  = 0;
  danzeff_mode = 0;

  memcpy(loc_kbd_mapping  , psp_kbd_mapping  , sizeof(psp_kbd_mapping));
  memcpy(loc_kbd_mapping_L, psp_kbd_mapping_L, sizeof(psp_kbd_mapping_L));
  memcpy(loc_kbd_mapping_R, psp_kbd_mapping_R, sizeof(psp_kbd_mapping_R));

  while (! end_menu)
  {
    psp_display_screen_kbd_menu();

    if (danzeff_mode) {
      danzeff_moveTo(-10, -50);
      danzeff_render();
    }
    psp_sdl_flip();

    while (1) {
      gp2xCtrlPeekBufferPositive(&c, 1);
      c.Buttons &= PSP_ALL_BUTTON_MASK;

      new_pad = c.Buttons;

      if ((old_pad != new_pad) || ((c.TimeStamp - last_time) > PSP_MENU_MIN_TIME)) {
        last_time = c.TimeStamp;
        old_pad = new_pad;
        break;
      }
    }

    if (danzeff_mode) {

      danzeff_key = danzeff_readInput(c);

      if (danzeff_key > DANZEFF_START) {
        msx_key = msx_get_key_from_ascii(danzeff_key);

        if (msx_key != -1) {
          if ((cur_menu_id >= MENU_KBD_UP       ) && 
              (cur_menu_id <= MENU_KBD_JOY_RIGHT)) 
          {
            kbd_id = psp_kbd_menu_id_to_key_id(cur_menu_id);

            if (menu_kbd_selected == -1) loc_kbd_mapping[kbd_id] = msx_key;
            else
            if (menu_kbd_selected == KBD_LTRIGGER_MAPPING) loc_kbd_mapping_L[kbd_id] = msx_key;
            else
            if (menu_kbd_selected == KBD_RTRIGGER_MAPPING) loc_kbd_mapping_R[kbd_id] = msx_key;
          }
        }

      } else 
      if ((danzeff_key == DANZEFF_START ) || 
          (danzeff_key == DANZEFF_SELECT)) 
      {
        danzeff_mode = 0;
        old_pad = new_pad = 0;

        psp_kbd_wait_no_button();
      }

      if (danzeff_key >= -1) {
        continue;
      }
    }

    if (new_pad & GP2X_CTRL_LTRIGGER) {
      psp_keyboard_select_change(-1);
      psp_kbd_wait_no_button();
    } else
    if (new_pad & GP2X_CTRL_RTRIGGER) {
      psp_keyboard_select_change(+1);
      psp_kbd_wait_no_button();
    } else


    if ((new_pad == GP2X_CTRL_LEFT ) || (new_pad == GP2X_CTRL_RIGHT))
    {
      int step = 1;
      if (new_pad & GP2X_CTRL_LEFT) step = -1;

      if ((cur_menu_id >= MENU_KBD_UP       ) && 
          (cur_menu_id <= MENU_KBD_JOY_RIGHT)) 
      {
        kbd_id = psp_kbd_menu_id_to_key_id(cur_menu_id);
        psp_keyboard_menu_mapping(kbd_id, step); 
      }
      switch (cur_menu_id ) 
      {
        case MENU_KBD_SKIN        : psp_keyboard_menu_skin(step);
        break;
        case MENU_KBD_KBD_SELECT  : psp_keyboard_select_change(step);
        break;
      }
    } else
    if ((new_pad == GP2X_CTRL_CROSS) || (new_pad == GP2X_CTRL_CIRCLE))
    {
      switch (cur_menu_id ) 
      {
        case MENU_KBD_LOAD  : psp_keyboard_menu_load();
                              old_pad = new_pad = 0;
                              menu_kbd_selected = -1;
        break;
        case MENU_KBD_SAVE        : psp_keyboard_menu_save();
        break;
        case MENU_KBD_HOTKEYS     : psp_keyboard_menu_hotkeys();
        break;
        case MENU_KBD_RESET       : psp_keyboard_menu_reset_kbd();
        break;

        case MENU_KBD_BACK        : end_menu = 1;
        break;
      }

    } else
    if(new_pad & GP2X_CTRL_UP) {

      if (cur_menu_id > 0) cur_menu_id--;
      else                 cur_menu_id = MAX_MENU_KBD_ITEM-1;

    } else
    if(new_pad & GP2X_CTRL_DOWN) {

      if (cur_menu_id < (MAX_MENU_KBD_ITEM-1)) cur_menu_id++;
      else                                     cur_menu_id = 0;

    } else  
    if(new_pad & GP2X_CTRL_SQUARE) {
      /* Cancel */
      end_menu = -1;
    } else 
    if ((new_pad & GP2X_CTRL_SELECT) == GP2X_CTRL_SELECT) {
      /* Back to Main Menu */
      end_menu = 1;
    } else
    if ((new_pad & GP2X_CTRL_START) == GP2X_CTRL_START) {
      if ((cur_menu_id < MENU_KBD_UP       ) ||
          (cur_menu_id > MENU_KBD_JOY_RIGHT)) {
        cur_menu_id = MENU_KBD_UP;
      }
      danzeff_mode = 1;
    }
  }
 
  if (end_menu > 0) {
    /* Validate */
    psp_keyboard_menu_update_lr();

    memcpy(psp_kbd_mapping  , loc_kbd_mapping  , sizeof(psp_kbd_mapping));
    memcpy(psp_kbd_mapping_L, loc_kbd_mapping_L, sizeof(psp_kbd_mapping_L));
    memcpy(psp_kbd_mapping_R, loc_kbd_mapping_R, sizeof(psp_kbd_mapping_R));
  }

  psp_kbd_wait_no_button();
}

