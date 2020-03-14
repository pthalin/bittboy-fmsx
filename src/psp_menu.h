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

# ifndef _PSP_MENU_H_
# define _PSP_MENU_H_

#ifdef __cplusplus
extern "C" {
#endif

# define PSP_MENU_BORDER_COLOR     psp_sdl_rgb(0x80,0x80,0xF0)
# define PSP_MENU_WARNING_COLOR    psp_sdl_rgb(0xFF,0x00,0x00)
# define PSP_MENU_NOTE_COLOR       psp_sdl_rgb(0xFF,0xFF,0x00)
# define PSP_MENU_BACKGROUND_COLOR psp_sdl_rgb(0x00,0x00,0x00)
# define PSP_MENU_BLACK_COLOR      psp_sdl_rgb(0x00,0x00,0x00)
# define PSP_MENU_AUTHOR_COLOR     psp_sdl_rgb(0x00,0x00,0xFF)
# define PSP_MENU_BLUE_COLOR       psp_sdl_rgb(0x00,0x00,0xFF)
# define PSP_MENU_GREEN_COLOR      psp_sdl_rgb(0x00,0xFF,0x00)
# define PSP_MENU_RED_COLOR        psp_sdl_rgb(0xFF,0x00,0x00)

# define PSP_MENU_TEXT_COLOR       psp_sdl_rgb(0x80,0x80,0xFF)
# define PSP_MENU_TEXT2_COLOR      psp_sdl_rgb(0xff,0xff,0xff)
# define PSP_MENU_SEL3_COLOR       psp_sdl_rgb(0x00,0xff,0xff)
# define PSP_MENU_SEL2_COLOR       psp_sdl_rgb(0x00,0xff,0x00)
# define PSP_MENU_SEL_COLOR        psp_sdl_rgb(0xFF,0xFF,0x00)

# define PSP_MENU_MIN_TIME         150000

  typedef struct menu_item_t {
    char *title;
  } menu_item_t;


   extern int psp_main_menu(void);
#ifdef __cplusplus
}
#endif
/////
void string_fill_with_space(char *buffer, int size);
void psp_display_screen_menu(void);
void psp_menu_display_save_name();

# endif
