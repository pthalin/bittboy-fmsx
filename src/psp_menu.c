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
#include "psp_menu_joy.h"
#include "psp_menu_set.h"
#include "psp_menu_tape.h"
#include "psp_menu_cheat.h"
#include "psp_menu_help.h"
#include "psp_editor.h"
#include "gp2x_psp.h"

extern SDL_Surface *back_surface;

enum {
  // MENU_VOLUME,

  MENU_LOAD_SLOT,
  MENU_SAVE_SLOT,
  MENU_DEL_SLOT,

  MENU_SCREENSHOT,
  // MENU_HELP,

  MENU_LOAD_ROM,
  MENU_LOAD_DISK0,
  MENU_LOAD_DISK1,
  MENU_EJECT_ROM,

  // MENU_EDITOR,
  MENU_CHEATS,
  // MENU_KEYBOARD,
  // MENU_JOYSTICK,
  MENU_SETTINGS,
  MENU_TAPE,


  MENU_RESET,
  // MENU_BACK,

  MENU_EXIT,

  MAX_MENU_ITEM
};

  static menu_item_t menu_list[] =
  {
    // { "Volume          :" },

    { "Load state" },
    { "Save state" },
    { "Delete state" },

    { "Save Screenshot" },
    // { "Help" },

    { "Load Rom" },
    { "Load Disk 0" },
    { "Load Disk 1" },
    { "Eject ROM" },

    // { "Comments" },
    { "Cheats" },
    // { "Keyboard" },
    // { "Joystick" },
    { "Settings"  },
    { "Tape" },

    { "Reset" },
    // { "Back to MSX" },

    { "Exit" }
  };

  static int cur_menu_id = 0;
  static int cur_slot    = 0;

void
psp_menu_display_save_name()
{
  char buffer[128];
  int Length;

  snprintf(buffer, 50, "%s", MSX.msx_save_name);
  Length = strlen(buffer);
  psp_sdl_back2_print(10, 5, buffer, PSP_MENU_TEXT2_COLOR);
}

void
string_fill_with_space(char *buffer, int size)
{
  int length = strlen(buffer);
  int index;

  for (index = length; index < size; index++) {
    buffer[index] = ' ';
  }
  buffer[size] = 0;
}

void
psp_display_screen_menu(void)
{
  char buffer[64];
  int menu_id = 0;
  int slot_id = 0;
  int color   = 0;
  int x       = 10;
  int y       = 20;
  int y_step  = 10;
  int x_step  = 30; /* dc 20130702 */

  psp_sdl_blit_background();

  for (menu_id = 0; menu_id < MAX_MENU_ITEM; menu_id++, y += y_step) {
    color = PSP_MENU_TEXT_COLOR;
    if (cur_menu_id == menu_id) {
      color = PSP_MENU_SEL_COLOR;
      if (cur_menu_id == MENU_LOAD_SLOT) color = PSP_MENU_SEL2_COLOR;
      else if (cur_menu_id == MENU_DEL_SLOT || cur_menu_id == MENU_EXIT) color = PSP_MENU_WARNING_COLOR;
    }
    psp_sdl_back2_print(x, y, menu_list[menu_id].title, color);

    switch (menu_id) {
        case MENU_SCREENSHOT:
            sprintf(buffer,": %d", MSX.psp_screenshot_id);
            string_fill_with_space(buffer, 4);
            psp_sdl_back2_print(140, y, buffer, color);
        case MENU_DEL_SLOT:
        case MENU_EJECT_ROM:
        case MENU_SETTINGS:
        case MENU_TAPE:
        case MENU_RESET:
          y += y_step;
          break;
    }
  }

 if (cur_menu_id <= MENU_DEL_SLOT) {
    y      = 20 + cur_menu_id * y_step; /* dc 20130702 */
    x      = 142; /* dc 20130702 */

    for (slot_id = 0; slot_id < MSX_MAX_SAVE_STATE; slot_id++) {
      if (slot_id == cur_slot) {
        color = PSP_MENU_SEL2_COLOR;
        switch (cur_menu_id) {
            case MENU_SAVE_SLOT:
              color = PSP_MENU_SEL_COLOR;
              break;
            case MENU_DEL_SLOT:
              color = PSP_MENU_WARNING_COLOR;
              break;
        }
      }
      else color = PSP_MENU_TEXT_COLOR;

      if (MSX.msx_save_state[slot_id].used) {
        sprintf(buffer, "[x]");
      } else {
        sprintf(buffer, "[ ]");
      }
      // string_fill_with_space(buffer, 32); /* dc 20130702 */
      // psp_sdl_back2_print(100, y, buffer, color); /* dc 20130702 */
      psp_sdl_back2_print(x, y, buffer, color);

      // y += y_step; /* dc 20130702 */
      x += x_step;
    }

    y += 1.5*y_step;
    x = 140;

    if (MSX.msx_save_state[cur_slot].thumb) {
      psp_sdl_blit_thumb(x,y, MSX.msx_save_state[cur_slot].surface);
    // } else {
      // psp_sdl_blit_thumb(170,115, thumb_surface);
    }
  }
  psp_menu_display_save_name();
}

static void
psp_main_menu_reset(void)
{
  /* Reset ! */
  psp_display_screen_menu();
  psp_sdl_back2_print( 120, 100, "Reset MSX !", PSP_MENU_WARNING_COLOR);
  psp_sdl_flip();
  emulator_reset();
  sleep(1);
}

static int
psp_main_menu_load(int format, int drive_id)
{
  int ret;

  ret = psp_fmgr_menu(format, drive_id);
  if (ret ==  1) /* load OK */
  {
    psp_display_screen_menu();
    psp_sdl_back2_print( 120, 100, "File loaded !",
                       PSP_MENU_NOTE_COLOR);
    psp_sdl_flip();
    sleep(1);
    return 1;
  }
  else
  if (ret == -1) /* Load Error */
  {
    psp_display_screen_menu();
    psp_sdl_back2_print( 120, 100, "Can't load file !",
                       PSP_MENU_WARNING_COLOR);
    psp_sdl_flip();
    sleep(1);
  }
  return 0;
}

/* used by hotkeys */
int
psp_main_menu_load_current()
{
  if (MSX.msx_save_state[cur_slot].used) {
    return msx_snapshot_load_slot(cur_slot);
  }
  return -1;
}

int
psp_main_menu_save_current()
{
  msx_save_back_to_blit();
  return msx_snapshot_save_slot(cur_slot);
}

static int
psp_main_menu_load_slot()
{
  int error;

  if (! MSX.msx_save_state[cur_slot].used) {

    psp_display_screen_menu();
    psp_sdl_back2_print( 120, 100, "Slot is empty !",
                       PSP_MENU_NOTE_COLOR);
    psp_sdl_flip();
    sleep(1);

    return 0;
  }

  error = msx_snapshot_load_slot(cur_slot);

  if (! error) /* save OK */
  {
    psp_display_screen_menu();
    psp_sdl_back2_print( 120, 100, "File loaded !",
                       PSP_MENU_NOTE_COLOR);
    psp_sdl_flip();
    sleep(1);

    return 1;
  }
  else
  {
    psp_display_screen_menu();
    psp_sdl_back2_print( 120, 100, "Can't load file !",
                       PSP_MENU_WARNING_COLOR);
    psp_sdl_flip();
    sleep(1);
  }
  return 0;
}

static void
psp_main_menu_save_slot()
{
  int error;
  error = msx_snapshot_save_slot(cur_slot);

  if (! error) /* save OK */
  {
    psp_display_screen_menu();
    psp_sdl_back2_print( 120, 100, "File saved !",
                       PSP_MENU_NOTE_COLOR);
    psp_sdl_flip();
    sleep(1);
  }
  else
  {
    psp_display_screen_menu();
    psp_sdl_back2_print( 120, 100, "Can't save file !",
                       PSP_MENU_WARNING_COLOR);
    psp_sdl_flip();
    sleep(1);
  }
}

static void
psp_main_menu_del_slot()
{
  int error;

  if (! MSX.msx_save_state[cur_slot].used) {

    psp_display_screen_menu();
    psp_sdl_back2_print( 120, 100, "Slot is empty !",
                       PSP_MENU_NOTE_COLOR);
    psp_sdl_flip();
    sleep(1);

    return;
  }

  error = msx_snapshot_del_slot(cur_slot);

  if (! error) /* save OK */
  {
    psp_display_screen_menu();
    psp_sdl_back2_print( 120, 100, "File deleted !",
                       PSP_MENU_NOTE_COLOR);
    psp_sdl_flip();
    sleep(1);
  }
  else
  {
    psp_display_screen_menu();
    psp_sdl_back2_print( 120, 100, "Can't delete file !",
                       PSP_MENU_WARNING_COLOR);
    psp_sdl_flip();
    sleep(1);
  }
}

static void
psp_main_menu_eject_rom()
{
  int error;

  error = msx_eject_rom();

  psp_display_screen_menu();
  psp_sdl_back2_print( 120, 100, "Rom ejected !",
                     PSP_MENU_NOTE_COLOR);
  psp_sdl_flip();
  sleep(1);
}

static void
psp_main_menu_cur_slot(int step)
{
  if (step == 1) {
    cur_slot++; if (cur_slot >= MSX_MAX_SAVE_STATE) cur_slot = 0;
  } else if (step == -1) {
    cur_slot--; if (cur_slot < 0) cur_slot = MSX_MAX_SAVE_STATE - 1;
  }
}

static void
psp_main_menu_screenshot(void)
{
  psp_screenshot_mode = 10;
}

static void
psp_main_menu_volume(int step)
{
  if (step == 1) {
    gp2xIncreaseVolume();
  } else if (step == -1) {
    gp2xDecreaseVolume();
  }
}

int
psp_main_menu_exit(void)
{
  gp2xCtrlData c;

  // psp_display_screen_menu();
  // psp_sdl_back2_print( 120, 100, "press B to confirm !", PSP_MENU_WARNING_COLOR);
  // psp_sdl_flip();

  // psp_kbd_wait_no_button();

  // do
  // {
  //   gp2xCtrlReadBufferPositive(&c, 1);
  //   c.Buttons &= PSP_ALL_BUTTON_MASK;

  //   if (c.Buttons & GP2X_CTRL_CROSS) {
      // psp_sdl_clear_screen(0); psp_sdl_flip();
      // psp_sdl_clear_screen(0); psp_sdl_flip();
      // psp_sdl_clear_screen(0); psp_sdl_flip();
      psp_sdl_exit(0);
  //   }

  // } while (c.Buttons == 0);

  // psp_kbd_wait_no_button();

  return 0;
}

void
psp_main_menu_editor()
{
  char TmpFileName[MAX_PATH];
  sprintf(TmpFileName, "%s/txt/%s.txt", MSX.msx_home_dir, MSX.msx_save_name);
  psp_editor_menu( TmpFileName );
}

extern int loaded_msx;
extern char ZipFile[256];


int
psp_main_menu(void)
{
	gp2xCtrlData c;
	long        new_pad;
	long        old_pad;
	int         last_time;
	int         end_menu;
	int error;

	audio_pause();

	msx_save_back_to_blit();

	psp_kbd_wait_no_button();

	old_pad   = 0;
	last_time = 0;
	end_menu  = 0;
	skip_disk_rom = 0;

	error = 0;
	
	switch (loaded_msx)
	{
		case 1:
			/* For some reasons, not setting ZIP support to 1 even for uncompressed roms
			does not work ?
			*/
			error = msx_load_rom(CartA, 0);
			loaded_msx = 0;
			end_menu = 1;
		        break;
		case 2:
			error = msx_load_disk(DiskA,0);
			loaded_msx = 0;
			end_menu = 1;
		        break;
		case 3:
			error = msx_load_rom(ZipFile, 1);
			loaded_msx = 0;
			end_menu = 1;
		        break;
                case 4: error = msx_load_tape(CasName);
			loaded_msx = 0;
			end_menu = 1;
		        break;
                default:
			break;
	}


	while (! end_menu)
	{
		psp_display_screen_menu();
		psp_sdl_flip();

		end_menu = 0;

		while (1)
		{
			gp2xCtrlReadBufferPositive(&c, 1);
			c.Buttons &= PSP_ALL_BUTTON_MASK;
			if (c.Buttons) break;
		}

		new_pad = c.Buttons;

		if ((old_pad != new_pad) || ((c.TimeStamp - last_time) > PSP_MENU_MIN_TIME)) {
		  last_time = c.TimeStamp;
		  old_pad = new_pad;

		} else continue;

		if ((c.Buttons & GP2X_CTRL_LTRIGGER) == GP2X_CTRL_LTRIGGER) {
		  psp_settings_menu();
		  old_pad = new_pad = 0;
		} else
		if ((c.Buttons & GP2X_CTRL_RTRIGGER) == GP2X_CTRL_RTRIGGER) {
		  psp_main_menu_reset();
		  end_menu = 1;
		} else
     if ((new_pad == GP2X_CTRL_LEFT ) || (new_pad == GP2X_CTRL_RIGHT))
    {
      int step = 1;
      if (new_pad & GP2X_CTRL_LEFT) step = -1;

      switch (cur_menu_id ) 
      {
        case MENU_LOAD_SLOT : psp_main_menu_cur_slot(step);
        break;
        case MENU_SAVE_SLOT : psp_main_menu_cur_slot(step);
        break;
        case MENU_DEL_SLOT  : psp_main_menu_cur_slot(step);
        break;
        // case MENU_VOLUME     : psp_main_menu_volume(step);
                               // old_pad = new_pad = 0;
        // break;              
      }
    } else 
    if ((new_pad == GP2X_CTRL_CIRCLE))
    {
	  switch (cur_menu_id )
		  {
        case MENU_LOAD_SLOT : if (psp_main_menu_load_slot()) {
                                end_menu = 1;
                              }
        break;
        case MENU_SAVE_SLOT : psp_main_menu_save_slot();
        break;
        case MENU_DEL_SLOT  : psp_main_menu_del_slot();
        break;


			case MENU_LOAD_ROM   : if (psp_main_menu_load(FMGR_FORMAT_ROM, 0)) end_menu = 1;
								   old_pad = new_pad = 0;
			break;
			case MENU_LOAD_DISK0 : if (psp_main_menu_load(FMGR_FORMAT_DISK, 0)) end_menu = 1;
								   old_pad = new_pad = 0;
			break;

			case MENU_LOAD_DISK1 : if (psp_main_menu_load(FMGR_FORMAT_DISK, 1)) end_menu = 1;
								   old_pad = new_pad = 0;
			break;

			// case MENU_EDITOR    : psp_main_menu_editor();
			// 					  old_pad = new_pad = 0;
			// break;

			case MENU_CHEATS    : psp_cheat_menu();
								  old_pad = new_pad = 0;
			break;

			// case MENU_KEYBOARD   : psp_keyboard_menu();
			// 					   old_pad = new_pad = 0;
			// break;
			// case MENU_JOYSTICK   : psp_joystick_menu();
			// 					   old_pad = new_pad = 0;
			// break;
			case MENU_SETTINGS   : psp_settings_menu();
								   old_pad = new_pad = 0;
			break;

			case MENU_EJECT_ROM  : psp_main_menu_eject_rom();
								   old_pad = new_pad = 0;
			break;

			case MENU_SCREENSHOT : psp_main_menu_screenshot();
								   end_menu = 1;
			break;
			// case MENU_VOLUME     : psp_main_menu_volume(step);
			// 					   old_pad = new_pad = 0;
			// break;

			case MENU_RESET     : psp_main_menu_reset();
								  end_menu = 1;
			break;

			// case MENU_BACK      : end_menu = 1;
			// break;
                        case MENU_TAPE      : psp_tape_menu();
					      end_menu = 1;
                        break;

			case MENU_EXIT      : psp_main_menu_exit();
			break;

			// case MENU_HELP      : psp_help_menu();
			// 					  old_pad = new_pad = 0;
			// break;
		  }

		} else
		if(new_pad & GP2X_CTRL_UP) {

		  if (cur_menu_id > 0) cur_menu_id--;
		  else                 cur_menu_id = MAX_MENU_ITEM-1;

		} else
		if(new_pad & GP2X_CTRL_DOWN) {

		  if (cur_menu_id < (MAX_MENU_ITEM-1)) cur_menu_id++;
		  else                                 cur_menu_id = 0;

		} else
		if(new_pad & GP2X_CTRL_SQUARE) {
		  end_menu = -1;
		} else
		if((new_pad & GP2X_CTRL_CROSS) || (new_pad & GP2X_CTRL_SELECT)) {
		  end_menu = 1;
		}
  }

	psp_kbd_wait_no_button();

  psp_sdl_clear_screen( PSP_MENU_BLACK_COLOR ); psp_sdl_flip();
  psp_sdl_clear_screen( PSP_MENU_BLACK_COLOR ); psp_sdl_flip();
  psp_sdl_clear_screen( PSP_MENU_BLACK_COLOR ); psp_sdl_flip();
	psp_sdl_clear_blit(0);
	audio_resume();

	return 1;
}

