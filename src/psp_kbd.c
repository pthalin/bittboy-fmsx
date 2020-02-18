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

#include <SDL.h>

#include "MSX.h"
#include "global.h"
#include "psp_kbd.h"
#include "psp_menu.h"
#include "psp_sdl.h"
#include "psp_danzeff.h"

# define KBD_MIN_ANALOG_TIME      150000
# define KBD_MIN_START_TIME       800000
# define KBD_MAX_EVENT_TIME       500000
//# define KBD_MIN_PENDING_TIME     300000
# define KBD_MIN_PENDING_TIME      10000
//# define KBD_MIN_DANZEFF_TIME     150000
# define KBD_MIN_DANZEFF_TIME      10000
# define KBD_MIN_COMMAND_TIME     100000
# define KBD_MIN_BATTCHECK_TIME 90000000 
# define KBD_MIN_AUTOFIRE_TIME   1000000

 static gp2xCtrlData    loc_button_data;
 static unsigned int   loc_last_event_time = 0;
 static long           first_time_stamp = -1;
 static long           first_time_auto_stamp = -1;
 static char           loc_button_press[ KBD_MAX_BUTTONS ]; 
 static char           loc_button_release[ KBD_MAX_BUTTONS ]; 
 static unsigned int   loc_button_mask[ KBD_MAX_BUTTONS ] =
 {
   GP2X_CTRL_UP         , /*  KBD_UP         */
   GP2X_CTRL_RIGHT      , /*  KBD_RIGHT      */
   GP2X_CTRL_DOWN       , /*  KBD_DOWN       */
   GP2X_CTRL_LEFT       , /*  KBD_LEFT       */
   GP2X_CTRL_TRIANGLE   , /*  KBD_TRIANGLE   */
   GP2X_CTRL_CIRCLE     , /*  KBD_CIRCLE     */
   GP2X_CTRL_CROSS      , /*  KBD_CROSS      */
   GP2X_CTRL_SQUARE     , /*  KBD_SQUARE     */
   GP2X_CTRL_SELECT     , /*  KBD_SELECT     */
   GP2X_CTRL_START      , /*  KBD_START      */
   GP2X_CTRL_LTRIGGER   , /*  KBD_LTRIGGER   */
   GP2X_CTRL_RTRIGGER   , /*  KBD_RTRIGGER   */
   GP2X_CTRL_FIRE,        /*  KBD_FIRE       */
 };

 static char loc_button_name[ KBD_ALL_BUTTONS ][20] =
 {
   "UP",
   "RIGHT",
   "DOWN",
   "LEFT",
# if defined(DINGUX_MODE) 
   "X",      // Triangle
   "A",      // Circle
   "B",      // Cross
   "Y",      // Square
# else
   "Y",      // Triangle
   "B",      // Circle
   "X",      // Cross
   "A",      // Square
# endif
   "SELECT",
   "START",
   "LTRIGGER",
   "RTRIGGER",
   "JOY_FIRE",
   "JOY_UP",
   "JOY_RIGHT",
   "JOY_DOWN",
   "JOY_LEFT"
 };

 static char loc_button_name_L[ KBD_ALL_BUTTONS ][20] =
 {
   "L_UP",
   "L_RIGHT",
   "L_DOWN",
   "L_LEFT",
# if defined(DINGUX_MODE) 
   "L_X",      // Triangle
   "L_A",      // Circle
   "L_B",      // Cross
   "L_Y",      // Square
# else
   "L_Y",      // Triangle
   "L_B",      // Circle
   "L_X",      // Cross
   "L_A",      // Square
# endif
   "L_SELECT",
   "L_START",
   "L_LTRIGGER",
   "L_RTRIGGER",
   "L_JOY_FIRE",
   "L_JOY_UP",
   "L_JOY_RIGHT",
   "L_JOY_DOWN",
   "L_JOY_LEFT"
 };
 
  static char loc_button_name_R[ KBD_ALL_BUTTONS ][20] =
 {
   "R_UP",
   "R_RIGHT",
   "R_DOWN",
   "R_LEFT",
# if defined(DINGUX_MODE)
   "R_X",      // Triangle
   "R_A",      // Circle
   "R_B",      // Cross
   "R_Y",      // Square
# else
   "R_Y",      // Triangle
   "R_B",      // Circle
   "R_X",      // Cross
   "R_A",      // Square
# endif
   "R_SELECT",
   "R_START",
   "R_LTRIGGER",
   "R_RTRIGGER",
   "R_JOY_FIRE",
   "R_JOY_UP",
   "R_JOY_RIGHT",
   "R_JOY_DOWN",
   "R_JOY_LEFT"
 };
 
  struct msx_key_trans psp_msx_key_info[MSXK_MAX_KEY]=
  {
    // MSXK            ID MASK  SHIFT  NAME 
    { MSXK_UNDERSCORE, 1, 0x04, 1,     "_" },
    { MSXK_1,          0, 0x02, 0,     "1" },
    { MSXK_2,          0, 0x04, 0,     "2" },
    { MSXK_3,          0, 0x08, 0,     "3" },
    { MSXK_4,          0, 0x10, 0,     "4" },
    { MSXK_5,          0, 0x20, 0,     "5" },
    { MSXK_6,          0, 0x40, 0,     "6" },
    { MSXK_7,          0, 0x80, 0,     "7" },
    { MSXK_8,          1, 0x01, 0,     "8" },
    { MSXK_9,          1, 0x02, 0,     "9" },
    { MSXK_0,          0, 0x01, 0,     "0" },
    { MSXK_SEMICOLON,  1, 0x80, 0,     ";" },
    { MSXK_MINUS    ,  1, 0x04, 0,     "-" },
    { MSXK_DELETE,     7, 0x20, 0,     "DELETE" },
    { MSXK_POUND,      0, 0x10, 1,     "POUND" },
    { MSXK_EXCLAMATN,  0, 0x02, 1,     "!" },
    { MSXK_DBLQUOTE,   2, 0x01, 1,     "\"" },
    { MSXK_HASH,       0, 0x08, 1,     "#" },
    { MSXK_DOLLAR,     0, 0x10, 1,     "$" },
    { MSXK_PERCENT,    1, 0x20, 1,     "%" },
    { MSXK_AMPERSAND,  1, 0x80, 1,     "&" },
    { MSXK_QUOTE,      2, 0x40, 0,     "'" },
    { MSXK_LEFTPAREN,  1, 0x02, 1,     "(" },
    { MSXK_RIGHTPAREN, 0, 0x01, 1,     ")" },
    { MSXK_PLUS,       1, 0x08, 1,     "+" },
    { MSXK_EQUAL,      1, 0x08, 0,     "=" },
    { MSXK_TAB,        7, 0x08, 0,     "TAB  " },
    { MSXK_a,          2, 0x40, 0,     "a" },
    { MSXK_b,          2, 0x80, 0,     "b" },
    { MSXK_c,          3, 0x01, 0,     "c" },
    { MSXK_d,          3, 0x02, 0,     "d" },
    { MSXK_e,          3, 0x04, 0,     "e" },
    { MSXK_f,          3, 0x08, 0,     "f" },
    { MSXK_g,          3, 0x10, 0,     "g" },
    { MSXK_h,          3, 0x20, 0,     "h" },
    { MSXK_i,          3, 0x40, 0,     "i" },
    { MSXK_j,          3, 0x80, 0,     "j" },
    { MSXK_k,          4, 0x01, 0,     "k" },
    { MSXK_l,          4, 0x02, 0,     "l" },
    { MSXK_m,          4, 0x04, 0,     "m" },
    { MSXK_n,          4, 0x08, 0,     "n" },
    { MSXK_o,          4, 0x10, 0,     "o" },
    { MSXK_p,          4, 0x20, 0,     "p" },
    { MSXK_q,          4, 0x40, 0,     "q" },
    { MSXK_r,          4, 0x80, 0,     "r" },
    { MSXK_s,          5, 0x01, 0,     "s" },
    { MSXK_t,          5, 0x02, 0,     "t" },
    { MSXK_u,          5, 0x04, 0,     "u" },
    { MSXK_v,          5, 0x08, 0,     "v" },
    { MSXK_w,          5, 0x10, 0,     "w" },
    { MSXK_x,          5, 0x20, 0,     "x" },
    { MSXK_y,          5, 0x40, 0,     "y" },
    { MSXK_z,          5, 0x80, 0,     "z" },
    { MSXK_A,          2, 0x40, 1,     "A" },
    { MSXK_B,          2, 0x80, 1,     "B" },
    { MSXK_C,          3, 0x01, 1,     "C" },
    { MSXK_D,          3, 0x02, 1,     "D" },
    { MSXK_E,          3, 0x04, 1,     "E" },
    { MSXK_F,          3, 0x08, 1,     "F" },
    { MSXK_G,          3, 0x10, 1,     "G" },
    { MSXK_H,          3, 0x20, 1,     "H" },
    { MSXK_I,          3, 0x40, 1,     "I" },
    { MSXK_J,          3, 0x80, 1,     "J" },
    { MSXK_K,          4, 0x01, 1,     "K" },
    { MSXK_L,          4, 0x02, 1,     "L" },
    { MSXK_M,          4, 0x04, 1,     "M" },
    { MSXK_N,          4, 0x08, 1,     "N" },
    { MSXK_O,          4, 0x10, 1,     "O" },
    { MSXK_P,          4, 0x20, 1,     "P" },
    { MSXK_Q,          4, 0x40, 1,     "Q" },
    { MSXK_R,          4, 0x80, 1,     "R" },
    { MSXK_S,          5, 0x01, 1,     "S" },
    { MSXK_T,          5, 0x02, 1,     "T" },
    { MSXK_U,          5, 0x04, 1,     "U" },
    { MSXK_V,          5, 0x08, 1,     "V" },
    { MSXK_W,          5, 0x10, 1,     "W" },
    { MSXK_X,          5, 0x20, 1,     "X" },
    { MSXK_Y,          5, 0x40, 1,     "Y" },
    { MSXK_Z,          5, 0x80, 1,     "Z" },
    { MSXK_RETURN,     7, 0x80, 0,     "RETURN" },
    { MSXK_CTRL_L,     6, 0x02, 0,     "CTRL_L" },
    { MSXK_CTRL_R,     2, 0x20, 0,     "CTRL_R" },
    { MSXK_SHIFT,      6, 0x01, 0,     "SHIFT" },
    { MSXK_CAPSLOCK,   1, 0x00, 0,     "CAPSLOCK" },
    { MSXK_ESC,        7, 0x04, 0,     "ESC" },
    { MSXK_SPACE,      8, 0x01, 0,     "SPACE" },
    { MSXK_LEFT,       8, 0x10, 0,     "LEFT" },
    { MSXK_UP,         8, 0x20, 0,     "UP" },
    { MSXK_RIGHT,      8, 0x80, 0,     "RIGHT" },
    { MSXK_DOWN,       8, 0x40, 0,     "DOWN" },
    { MSXK_F0,         0, 0x00, 0,     "F0" },
    { MSXK_F1,         6, 0x20, 0,     "F1" },
    { MSXK_F2,         6, 0x40, 0,     "F2" },
    { MSXK_F3,         6, 0x80, 0,     "F3" },
    { MSXK_F4,         7, 0x01, 0,     "F4" },
    { MSXK_F5,         7, 0x02, 0,     "F5" },
    { MSXK_F6,         6, 0x20, 1,     "F6" },
    { MSXK_F7,         6, 0x40, 1,     "F7" },
    { MSXK_F8,         6, 0x80, 1,     "F8" },
    { MSXK_F9,         7, 0x01, 1,     "F9" },
    { MSXK_F10,        7, 0x02, 1,     "F10" },
    { MSXK_AT,         0, 0x04, 1,     "@" },
    { MSXK_COLON,      1, 0x80, 1,     ":" },
    { MSXK_COMMA,      2, 0x04, 0,     "," },
    { MSXK_PERIOD,     2, 0x08, 0,     "." },
    { MSXK_SLASH,      2, 0x10, 0,     "/" },
    { MSXK_ASTERISK,   1, 0x01, 1,     "*" },
    { MSXK_LESS,       2, 0x04, 1,     "<" },
    { MSXK_GREATER,    2, 0x08, 1,     ">" },
    { MSXK_QUESTION,   2, 0x10, 1,     "?" },
    { MSXK_PIPE,       1, 0x10, 1,     "|" },
    { MSXK_RCBRACE,    1, 0x40, 1,     "}" },
    { MSXK_RBRACKET,   1, 0x40, 0,     "]" },
    { MSXK_LBRACKET,   1, 0x20, 0,     "[" },
    { MSXK_LCBRACE,    1, 0x20, 1,     "{" },
    { MSXK_TILDA    ,  2, 0x02, 1,     "~" },
    { MSXK_BACKSLASH,  1, 0x10, 0,     "\\" },
    { MSXK_POWER,      0, 0x40, 1,     "^" },
    { MSXK_SUPPR,      8, 0x08, 0,     "SUPPR" },
    { MSXK_SELECT,     7, 0x40, 0,     "SELECT" },
    { MSXK_GRAPH,      6, 0x04, 0,     "GRAPH" },
    { MSXK_STOP,       7, 0x10, 0,     "STOP"  },
    { MSXK_HOME,       8, 0x02, 0,     "HOME"  },
    { MSXK_JOY_UP,     1, 0x01, 0,     "JOY_UP" },
    { MSXK_JOY_DOWN,   1, 0x02, 0,     "JOY_DOWN" },
    { MSXK_JOY_LEFT,   1, 0x04, 0,     "JOY_LEFT" },
    { MSXK_JOY_RIGHT,  1, 0x08, 0,     "JOY_RIGHT" },
    { MSXK_JOY_FIRE1,  1, 0x10, 0,     "JOY_FIRE1" },
    { MSXK_JOY_FIRE2,  1, 0x20, 0,     "JOY_FIRE2" },
    { MSXK_C_FPS,      0, 0x00, 0,     "C_FPS" },
    { MSXK_C_JOY,      0, 0x00, 0,     "C_JOY" },
    { MSXK_C_RENDER,   0, 0x00, 0,     "C_RENDER" },
    { MSXK_C_LOAD,     0, 0x00, 0,     "C_LOAD" },
    { MSXK_C_SAVE,     0, 0x00, 0,     "C_SAVE" },
    { MSXK_C_RESET,    0, 0x00, 0,     "C_RESET" },
    { MSXK_C_AUTOFIRE, 0, 0x00, 0,     "C_AUTOFIRE" },
    { MSXK_C_INCFIRE,  0, 0x00, 0,     "C_INCFIRE" },
    { MSXK_C_DECFIRE,  0, 0x00, 0,     "C_DECFIRE" },
    { MSXK_C_SCREEN,   0, 0x00, 0,     "C_SCREEN" }
  };

  static int loc_default_mapping[ KBD_ALL_BUTTONS ] = {
    MSXK_UP              , /*  KBD_UP         */
    MSXK_RIGHT           , /*  KBD_RIGHT      */
    MSXK_DOWN            , /*  KBD_DOWN       */
    MSXK_LEFT            , /*  KBD_LEFT       */
    MSXK_JOY_FIRE2       ,    //MSXK_RETURN          , /*  KBD_TRIANGLE   */
    MSXK_JOY_FIRE1       , /*  KBD_CIRCLE     */
    MSXK_SPACE           , /*  KBD_CROSS      */
    MSXK_DELETE          , /*  KBD_SQUARE     */
    -1                   , /*  KBD_SELECT     */
    -1                   , /*  KBD_START      */
    KBD_LTRIGGER_MAPPING , /*  KBD_LTRIGGER   */
    KBD_RTRIGGER_MAPPING , /*  KBD_RTRIGGER   */
    MSXK_JOY_FIRE1       , /*  KBD_JOY_FIRE   */
    MSXK_JOY_UP          , /*  KBD_JOY_UP     */
    MSXK_JOY_RIGHT       , /*  KBD_JOY_RIGHT  */
    MSXK_JOY_DOWN        , /*  KBD_JOY_DOWN   */
    MSXK_JOY_LEFT          /*  KBD_JOY_LEFT   */
  };

  static int loc_default_mapping_L[ KBD_ALL_BUTTONS ] = {
    MSXK_UP          , /*  KBD_UP         */
    MSXK_RIGHT       , /*  KBD_RIGHT      */
    MSXK_DOWN        , /*  KBD_DOWN       */
    MSXK_LEFT        , /*  KBD_LEFT       */
    MSXK_C_LOAD          , /*  KBD_TRIANGLE   */
    MSXK_C_RENDER        , /*  KBD_CIRCLE     */
    MSXK_C_SAVE          , /*  KBD_CROSS      */
    MSXK_C_FPS           , /*  KBD_SQUARE     */
    -1                   , /*  KBD_SELECT     */
    -1                   , /*  KBD_START      */
    KBD_LTRIGGER_MAPPING , /*  KBD_LTRIGGER   */
    KBD_RTRIGGER_MAPPING , /*  KBD_RTRIGGER   */
    MSXK_JOY_FIRE1       , /*  KBD_JOY_FIRE   */
    MSXK_0               , /*  KBD_JOY_UP     */
    MSXK_2               , /*  KBD_JOY_RIGHT  */
    MSXK_1               , /*  KBD_JOY_DOWN   */
    MSXK_3                 /*  KBD_JOY_LEFT   */
  };

  static int loc_default_mapping_R[ KBD_ALL_BUTTONS ] = {
    MSXK_JOY_UP          , /*  KBD_UP         */
    MSXK_C_INCFIRE       , /*  KBD_RIGHT      */
    MSXK_JOY_DOWN        , /*  KBD_DOWN       */
    MSXK_C_DECFIRE       , /*  KBD_LEFT       */
    MSXK_RETURN          , /*  KBD_TRIANGLE   */
    MSXK_JOY_FIRE2       , /*  KBD_CIRCLE     */
    MSXK_C_AUTOFIRE      , /*  KBD_CROSS      */
    MSXK_ESC             , /*  KBD_SQUARE     */
    -1                   , /*  KBD_SELECT     */
    -1                   , /*  KBD_START      */
    KBD_LTRIGGER_MAPPING , /*  KBD_LTRIGGER   */
    KBD_RTRIGGER_MAPPING , /*  KBD_RTRIGGER   */
    MSXK_JOY_FIRE1       , /*  KBD_JOY_FIRE   */
    MSXK_UP              , /*  KBD_JOY_UP     */
    MSXK_RIGHT           , /*  KBD_JOY_RIGHT  */
    MSXK_DOWN            , /*  KBD_JOY_DOWN   */
    MSXK_LEFT              /*  KBD_JOY_LEFT   */
  };

# define KBD_MAX_ENTRIES   116

  int kbd_layout[KBD_MAX_ENTRIES][2] = {
    /* Key            Ascii */
    { MSXK_0,          '0' },
    { MSXK_1,          '1' },
    { MSXK_2,          '2' },
    { MSXK_3,          '3' },
    { MSXK_4,          '4' },
    { MSXK_5,          '5' },
    { MSXK_6,          '6' },
    { MSXK_7,          '7' },
    { MSXK_8,          '8' },
    { MSXK_9,          '9' },
    { MSXK_A,          'A' },
    { MSXK_B,          'B' },
    { MSXK_C,          'C' },
    { MSXK_D,          'D' },
    { MSXK_E,          'E' },
    { MSXK_F,          'F' },
    { MSXK_G,          'G' },
    { MSXK_H,          'H' },
    { MSXK_I,          'I' },
    { MSXK_J,          'J' },
    { MSXK_K,          'K' },
    { MSXK_L,          'L' },
    { MSXK_M,          'M' },
    { MSXK_N,          'N' },
    { MSXK_O,          'O' },
    { MSXK_P,          'P' },
    { MSXK_Q,          'Q' },
    { MSXK_R,          'R' },
    { MSXK_S,          'S' },
    { MSXK_T,          'T' },
    { MSXK_U,          'U' },
    { MSXK_V,          'V' },
    { MSXK_W,          'W' },
    { MSXK_X,          'X' },
    { MSXK_Y,          'Y' },
    { MSXK_Z,          'Z' },
    { MSXK_a,          'a' },
    { MSXK_b,          'b' },
    { MSXK_c,          'c' },
    { MSXK_d,          'd' },
    { MSXK_e,          'e' },
    { MSXK_f,          'f' },
    { MSXK_g,          'g' },
    { MSXK_h,          'h' },
    { MSXK_i,          'i' },
    { MSXK_j,          'j' },
    { MSXK_k,          'k' },
    { MSXK_l,          'l' },
    { MSXK_m,          'm' },
    { MSXK_n,          'n' },
    { MSXK_o,          'o' },
    { MSXK_p,          'p' },
    { MSXK_q,          'q' },
    { MSXK_r,          'r' },
    { MSXK_s,          's' },
    { MSXK_t,          't' },
    { MSXK_u,          'u' },
    { MSXK_v,          'v' },
    { MSXK_w,          'w' },
    { MSXK_x,          'x' },
    { MSXK_y,          'y' },
    { MSXK_z,          'z' },
    { MSXK_DELETE,     DANZEFF_DEL },
    { MSXK_SPACE,      ' '         },
    { MSXK_F0,         DANZEFF_F0  },
    { MSXK_F1,         DANZEFF_F1  },
    { MSXK_F2,         DANZEFF_F2  },
    { MSXK_F3,         DANZEFF_F3  },
    { MSXK_F4,         DANZEFF_F4  },
    { MSXK_F5,         DANZEFF_F5  },
    { MSXK_F6,         DANZEFF_F6  },
    { MSXK_F7,         DANZEFF_F7  },
    { MSXK_F8,         DANZEFF_F8  },
    { MSXK_F9,         DANZEFF_F9  },
    { MSXK_CAPSLOCK,   DANZEFF_CAPSLOCK },
    { MSXK_RETURN,     DANZEFF_RETURN   },
    { MSXK_SHIFT,      DANZEFF_SHIFT    },
    { MSXK_TAB,        DANZEFF_TAB      },
    { MSXK_AMPERSAND,  '&' },
    { MSXK_ASTERISK,   '*' },
    { MSXK_AT,         '@' },
    { MSXK_COLON,      ':' },
    { MSXK_COMMA,      ',' },
    { MSXK_CTRL_L,    DANZEFF_CONTROL  },
    { MSXK_DOWN,       -1  },
    { MSXK_LEFT,       -1  },
    { MSXK_RIGHT,      -1  },
    { MSXK_UP,         -1  },
    { MSXK_DBLQUOTE,   '"' },
    { MSXK_QUOTE,      '\'' },
    { MSXK_DOLLAR,     '$' },
    { MSXK_EQUAL,      '=' },
    { MSXK_ESC,        DANZEFF_ESC   },
    { MSXK_EXCLAMATN,  '!' },
    { MSXK_GREATER,    '>' },
    { MSXK_HASH,       '#' },
    { MSXK_LEFTPAREN,  '(' },
    { MSXK_LESS,       '<' },
    { MSXK_MINUS,      '-' },
    { MSXK_PERCENT,    '%' },
    { MSXK_PERIOD,     '.' },
    { MSXK_PLUS,       '+' },
    { MSXK_QUESTION,   '?' },
    { MSXK_RIGHTPAREN, ')' },
    { MSXK_SEMICOLON,  ';' },
    { MSXK_SLASH,      '/' },
    { MSXK_UNDERSCORE, '_'  },
    { MSXK_PIPE,       '|' },
    { MSXK_RCBRACE,    '}' },
    { MSXK_RBRACKET,   ']' },
    { MSXK_LBRACKET,   '[' },
    { MSXK_LCBRACE,    '{' },
    { MSXK_TILDA    ,  '~' },
    { MSXK_BACKSLASH,  '\\' },
    { MSXK_POWER,      '^' },
    { MSXK_HOME,       DANZEFF_HOME }
  };

 int psp_kbd_mapping[ KBD_ALL_BUTTONS ];
 int psp_kbd_mapping_L[ KBD_ALL_BUTTONS ];
 int psp_kbd_mapping_R[ KBD_ALL_BUTTONS ];
 int psp_kbd_presses[ KBD_ALL_BUTTONS ];
 int kbd_ltrigger_mapping_active;
 int kbd_rtrigger_mapping_active;

 static int danzeff_msx_key     = 0;
 static int danzeff_msx_pending = 0;
 static int danzeff_mode        = 0;


       char command_keys[ 128 ];
 static int command_mode        = 0;
 static int command_index       = 0;
 static int command_size        = 0;
 static int command_msx_pending = 0;
 static int command_msx_key     = 0;

int
msx_key_event(int msx_idx, int press)
{
  int shift    = 0;
  int index    = 0;
  int bit_mask = 0;

  if ((msx_idx < 0) || (msx_idx >= MSXK_MAX_KEY)) {
    return -1;
  }

  if ((msx_idx >= MSXK_C_FPS) &&
      (msx_idx <= MSXK_C_SCREEN)) {
    if (press) {
      msx_treat_command_key(msx_idx);
    }
    return 0;
  }

  if ((msx_idx >= MSXK_JOY_UP) &&
      (msx_idx <= MSXK_JOY_FIRE2)) {
    bit_mask = psp_msx_key_info[msx_idx].bit_mask;
    if (press) {
      MsxJoyUp( bit_mask );
    } else {
      MsxJoyDown( bit_mask );
    }
  } else {
    shift    = psp_msx_key_info[msx_idx].shift;
    index    = psp_msx_key_info[msx_idx].index;
    bit_mask = psp_msx_key_info[msx_idx].bit_mask;

    if (press) {
      if (shift) MsxKeyDown(6,1);
      MsxKeyDown(index, bit_mask);
    } else {
      MsxKeyUp(index, bit_mask);
      if (shift) MsxKeyUp(6,1);
    }

  }
  return 0;
}
int 
msx_kbd_reset()
{
  return 0;
}

int
msx_get_key_from_ascii(int key_ascii)
{
  int index;
  for (index = 0; index < KBD_MAX_ENTRIES; index++) {
   if (kbd_layout[index][1] == key_ascii) return kbd_layout[index][0];
  }
  return -1;
}

void
psp_kbd_run_command(char *Command)
{
  strncpy(command_keys, Command, 128);
  command_size  = strlen(Command);
  command_index = 0;

  command_msx_key     = 0;
  command_msx_pending = 0;
  command_mode        = 1;
}

void
psp_kbd_default_settings()
{
  memcpy(psp_kbd_mapping, loc_default_mapping, sizeof(loc_default_mapping));
  memcpy(psp_kbd_mapping_L, loc_default_mapping_L, sizeof(loc_default_mapping_L));
  memcpy(psp_kbd_mapping_R, loc_default_mapping_R, sizeof(loc_default_mapping_R));
}

int
psp_kbd_reset_hotkeys(void)
{
  int index;
  int key_id;
  for (index = 0; index < KBD_ALL_BUTTONS; index++) {
    key_id = loc_default_mapping[index];
    if ((key_id >= MSXK_C_FPS) && (key_id <= MSXK_C_SCREEN)) {
      psp_kbd_mapping[index] = key_id;
    }
    key_id = loc_default_mapping_L[index];
    if ((key_id >= MSXK_C_FPS) && (key_id <= MSXK_C_SCREEN)) {
      psp_kbd_mapping_L[index] = key_id;
    }
    key_id = loc_default_mapping_R[index];
    if ((key_id >= MSXK_C_FPS) && (key_id <= MSXK_C_SCREEN)) {
      psp_kbd_mapping_R[index] = key_id;
    }
  }
  return 0;
}

int
psp_kbd_load_mapping_file(FILE *KbdFile)
{
  char     Buffer[512];
  char    *Scan;
  int      tmp_mapping[KBD_ALL_BUTTONS];
  int      tmp_mapping_L[KBD_ALL_BUTTONS];
  int      tmp_mapping_R[KBD_ALL_BUTTONS];
  int      msx_key_id = 0;
  int      kbd_id = 0;

  memcpy(tmp_mapping, loc_default_mapping, sizeof(loc_default_mapping));
  memcpy(tmp_mapping_L, loc_default_mapping_L, sizeof(loc_default_mapping_R));
  memcpy(tmp_mapping_R, loc_default_mapping_R, sizeof(loc_default_mapping_R));

  while (fgets(Buffer,512,KbdFile) != (char *)0) {
      
      Scan = strchr(Buffer,'\n');
      if (Scan) *Scan = '\0';
      /* For this #@$% of windows ! */
      Scan = strchr(Buffer,'\r');
      if (Scan) *Scan = '\0';
      if (Buffer[0] == '#') continue;

      Scan = strchr(Buffer,'=');
      if (! Scan) continue;
    
      *Scan = '\0';
      msx_key_id = atoi(Scan + 1);

      for (kbd_id = 0; kbd_id < KBD_ALL_BUTTONS; kbd_id++) {
        if (!strcasecmp(Buffer,loc_button_name[kbd_id])) {
          tmp_mapping[kbd_id] = msx_key_id;
          //break;
        }
      }
      for (kbd_id = 0; kbd_id < KBD_ALL_BUTTONS; kbd_id++) {
        if (!strcasecmp(Buffer,loc_button_name_L[kbd_id])) {
          tmp_mapping_L[kbd_id] = msx_key_id;
          //break;
        }
      }
      for (kbd_id = 0; kbd_id < KBD_ALL_BUTTONS; kbd_id++) {
        if (!strcasecmp(Buffer,loc_button_name_R[kbd_id])) {
          tmp_mapping_R[kbd_id] = msx_key_id;
          //break;
        }
      }
  }

  memcpy(psp_kbd_mapping, tmp_mapping, sizeof(psp_kbd_mapping));
  memcpy(psp_kbd_mapping_L, tmp_mapping_L, sizeof(psp_kbd_mapping_L));
  memcpy(psp_kbd_mapping_R, tmp_mapping_R, sizeof(psp_kbd_mapping_R));
  
  return 0;
}

int
psp_kbd_load_mapping(char *kbd_filename)
{
  FILE    *KbdFile;
  int      error = 0;

  KbdFile = fopen(kbd_filename, "r");
  error   = 1;

  if (KbdFile != (FILE*)0) {
    psp_kbd_load_mapping_file(KbdFile);
    error = 0;
    fclose(KbdFile);
  }

  kbd_ltrigger_mapping_active = 0;
  kbd_rtrigger_mapping_active = 0;
    
  return error;
}

int
psp_kbd_save_mapping(char *kbd_filename)
{
  FILE    *KbdFile;
  int      kbd_id = 0;
  int      error = 0;

  KbdFile = fopen(kbd_filename, "w");
  error   = 1;

  if (KbdFile != (FILE*)0) {

    for (kbd_id = 0; kbd_id < KBD_ALL_BUTTONS; kbd_id++)
    {
      fprintf(KbdFile, "%s=%d\n", loc_button_name[kbd_id], psp_kbd_mapping[kbd_id]);
    }
    for (kbd_id = 0; kbd_id < KBD_ALL_BUTTONS; kbd_id++)
    {
      fprintf(KbdFile, "%s=%d\n", loc_button_name_L[kbd_id], psp_kbd_mapping_L[kbd_id]);
    }
    for (kbd_id = 0; kbd_id < KBD_ALL_BUTTONS; kbd_id++)
    {
      fprintf(KbdFile, "%s=%d\n", loc_button_name_R[kbd_id], psp_kbd_mapping_R[kbd_id]);
    }
    error = 0;
    fclose(KbdFile);
  }

  return error;
}

int
psp_kbd_enter_command()
{
  gp2xCtrlData  c;

  unsigned int command_key = 0;
  int          msx_key     = 0;

  gp2xCtrlPeekBufferPositive(&c, 1);

  if (command_msx_pending) 
  {
    if ((c.TimeStamp - loc_last_event_time) > KBD_MIN_COMMAND_TIME) {
      loc_last_event_time = c.TimeStamp;
      command_msx_pending = 0;
      msx_key_event(command_msx_key, 0);
    }

    return 0;
  }

  if ((c.TimeStamp - loc_last_event_time) > KBD_MIN_COMMAND_TIME) {
    loc_last_event_time = c.TimeStamp;

    if (command_index >= command_size) {

      command_mode  = 0;
      command_index = 0;
      command_size  = 0;

      command_msx_pending = 0;
      command_msx_key     = 0;

      return 0;
    }
  
    command_key = command_keys[command_index++];
    msx_key = msx_get_key_from_ascii(command_key);

    if (msx_key != -1) {
      command_msx_key     = msx_key;
      command_msx_pending = 1;
      msx_key_event(command_msx_key, 1);
    }

    return 1;
  }

  return 0;
}

int 
psp_kbd_is_danzeff_mode()
{
  return danzeff_mode;
}

int
psp_kbd_enter_danzeff()
{
  unsigned int danzeff_key = 0;
  int          msx_key     = 0;

  gp2xCtrlData  c;

  if (! danzeff_mode) {
    psp_init_keyboard();
    danzeff_mode = 1;
  }

  gp2xCtrlPeekBufferPositive(&c, 1);

  if (danzeff_msx_pending) 
  {
    if ((c.TimeStamp - loc_last_event_time) > KBD_MIN_PENDING_TIME) {
      loc_last_event_time = c.TimeStamp;
      danzeff_msx_pending = 0;
      msx_key_event(danzeff_msx_key, 0);
    }
    return 0;
  }

  if ((c.TimeStamp - loc_last_event_time) > KBD_MIN_DANZEFF_TIME) {
    loc_last_event_time = c.TimeStamp;
  
    gp2xCtrlPeekBufferPositive(&c, 1);
    danzeff_key = danzeff_readInput(c);
  }

  if (danzeff_key > DANZEFF_START) {
    msx_key = msx_get_key_from_ascii(danzeff_key);

    if (msx_key != -1) {
      danzeff_msx_key     = msx_key;
      danzeff_msx_pending = 1;
      msx_key_event(danzeff_msx_key, 1);
    }

    return 0;

  } else if (danzeff_key == DANZEFF_START) {
    danzeff_mode       = 0;
    danzeff_msx_pending = 0;
    danzeff_msx_key     = 0;

    psp_kbd_wait_no_button();

  } else if (danzeff_key == DANZEFF_SELECT) {
    danzeff_mode       = 0;
    danzeff_msx_pending = 0;
    danzeff_msx_key     = 0;
    psp_main_menu();
    psp_init_keyboard();

    psp_kbd_wait_no_button();
  }

  return 0;
}

int
msx_decode_key(int psp_b, int button_pressed)
{
  int wake = 0;
  int reverse_cursor = ! MSX.psp_reverse_analog;

  if (reverse_cursor) {
    if ((psp_b >= KBD_JOY_UP  ) &&
        (psp_b <= KBD_JOY_LEFT)) {
      psp_b = psp_b - KBD_JOY_UP + KBD_UP;
    } else
    if ((psp_b >= KBD_UP  ) &&
        (psp_b <= KBD_LEFT)) {
      psp_b = psp_b - KBD_UP + KBD_JOY_UP;
    }
  }

  if (psp_b == KBD_START) {
     if (button_pressed) psp_kbd_enter_danzeff();
  } else
  if (psp_b == KBD_SELECT) {
    if (button_pressed) {
      psp_main_menu();
      psp_init_keyboard();
    }
  } else {
 
    if (psp_kbd_mapping[psp_b] >= 0) {
      wake = 1;
      if (button_pressed) {
        // Determine which buton to press first (ie which mapping is currently active)
        if (kbd_ltrigger_mapping_active) {
          // Use ltrigger mapping
          psp_kbd_presses[psp_b] = psp_kbd_mapping_L[psp_b];
          msx_key_event(psp_kbd_presses[psp_b], button_pressed);
        } else
        if (kbd_rtrigger_mapping_active) {
          // Use rtrigger mapping
          psp_kbd_presses[psp_b] = psp_kbd_mapping_R[psp_b];
          msx_key_event(psp_kbd_presses[psp_b], button_pressed);
        } else {
          // Use standard mapping
          psp_kbd_presses[psp_b] = psp_kbd_mapping[psp_b];
          msx_key_event(psp_kbd_presses[psp_b], button_pressed);
        }
      } else {
          // Determine which button to release (ie what was pressed before)
          msx_key_event(psp_kbd_presses[psp_b], button_pressed);
      }

    } else {
      if (psp_kbd_mapping[psp_b] == KBD_LTRIGGER_MAPPING) {
        kbd_ltrigger_mapping_active = button_pressed;
        kbd_rtrigger_mapping_active = 0;
      } else
      if (psp_kbd_mapping[psp_b] == KBD_RTRIGGER_MAPPING) {
        kbd_rtrigger_mapping_active = button_pressed;
        kbd_ltrigger_mapping_active = 0;
      }
    }
  }
  return 0;
}

void
kbd_change_auto_fire(int auto_fire)
{
  MSX.msx_auto_fire = auto_fire;
  if (MSX.msx_auto_fire_pressed) {
    msx_key_event(MSXK_JOY_FIRE1, 0);
    MSX.msx_auto_fire_pressed = 0;
  }
}

static int 
kbd_reset_button_status(void)
{
  int b = 0;
  /* Reset Button status */
  for (b = 0; b < KBD_MAX_BUTTONS; b++) {
    loc_button_press[b]   = 0;
    loc_button_release[b] = 0;
  }
  psp_init_keyboard();
  return 0;
}

int
kbd_scan_keyboard(void)
{
  gp2xCtrlData c;
  long        delta_stamp;
  int         event;
  int         b;

  event = 0;
  gp2xCtrlPeekBufferPositive( &c, 1 );

  if (MSX.msx_auto_fire) {
    delta_stamp = c.TimeStamp - first_time_auto_stamp;
    if ((delta_stamp < 0) || 
        (delta_stamp > (KBD_MIN_AUTOFIRE_TIME / (1 + MSX.msx_auto_fire_period)))) {
      first_time_auto_stamp = c.TimeStamp;

      msx_key_event(MSXK_JOY_FIRE1, MSX.msx_auto_fire_pressed);
      MSX.msx_auto_fire_pressed = ! MSX.msx_auto_fire_pressed;
    }
  }

  for (b = 0; b < KBD_MAX_BUTTONS; b++) 
  {
    if (c.Buttons & loc_button_mask[b]) {
      if (!(loc_button_data.Buttons & loc_button_mask[b])) {
        loc_button_press[b] = 1;
        event = 1;
      }
    } else {
      if (loc_button_data.Buttons & loc_button_mask[b]) {
        loc_button_release[b] = 1;
        loc_button_press[b] = 0;
        event = 1;
      }
    }
  }
  memcpy(&loc_button_data,&c,sizeof(gp2xCtrlData));

  return event;
}

void
kbd_wait_start(void)
{
  while (1)
  {
    gp2xCtrlData c;
    gp2xCtrlReadBufferPositive(&c, 1);
    if (c.Buttons & GP2X_CTRL_START) break;
  }
  psp_kbd_wait_no_button();
}

void
psp_init_keyboard(void)
{
  msx_kbd_reset();
  kbd_ltrigger_mapping_active = 0;
  kbd_rtrigger_mapping_active = 0;
}

void
psp_kbd_wait_no_button(void)
{
  gp2xCtrlData c;

  do {
   gp2xCtrlPeekBufferPositive(&c, 1);
  } while (c.Buttons != 0);
} 

void
psp_kbd_wait_button(void)
{
  gp2xCtrlData c;

  do {
   gp2xCtrlReadBufferPositive(&c, 1);
  } while (c.Buttons == 0);
} 

int
psp_update_keys(void)
{
  int         b;

  static char first_time = 1;
  static int release_pending = 0;

  if (first_time) {

    gp2xCtrlData c;
    gp2xCtrlPeekBufferPositive(&c, 1);

    if (first_time_stamp == -1) first_time_stamp = c.TimeStamp;

    first_time      = 0;
    release_pending = 0;

    for (b = 0; b < KBD_MAX_BUTTONS; b++) {
      loc_button_release[b] = 0;
      loc_button_press[b] = 0;
    }
    gp2xCtrlPeekBufferPositive(&loc_button_data, 1);

    psp_main_menu();
    psp_init_keyboard();

    return 0;
  }

  if (command_mode) {
    return psp_kbd_enter_command();
  }

  if (danzeff_mode) {
    return psp_kbd_enter_danzeff();
  }

  if (release_pending)
  {
    release_pending = 0;
    for (b = 0; b < KBD_MAX_BUTTONS; b++) {
      if (loc_button_release[b]) {
        loc_button_release[b] = 0;
        msx_decode_key(b, 0);
      }
    }
  }

  kbd_scan_keyboard();

  /* check press event */
  for (b = 0; b < KBD_MAX_BUTTONS; b++) {
    if (loc_button_press[b]) {
      loc_button_press[b] = 0;
      release_pending     = 0;
      msx_decode_key(b, 1);
    }
  }
  /* check release event */
  for (b = 0; b < KBD_MAX_BUTTONS; b++) {
    if (loc_button_release[b]) {
      release_pending = 1;
      break;
    }
  }

  return 0;
}
