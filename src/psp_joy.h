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

# ifndef _PSP_JOY_H_
# define _PSP_JOY_H_

# ifdef __cplusplus
extern "C" {
# endif

# define JOY_TYPE_NONE          0
# define JOY_TYPE_JOYSTICK      1
# define JOY_TYPE_PADDLE        2
# define JOY_TYPE_DUAL_PADDLE   3
# define JOY_MAX_TYPE           3

# define JOY_UP        0
# define JOY_DOWN      1
# define JOY_LEFT      2
# define JOY_RIGHT     3
# define JOY_FIRE      4
# define JOY_PADDLE1P  5
# define JOY_PADDLE1M  6
# define JOY_PADDLE1F  7
# define JOY_PADDLE2P  8
# define JOY_PADDLE2M  9
# define JOY_PADDLE2F 10

# define JOY_ALL_BUTTONS 11

  extern int psp_joy_mapping[ JOY_ALL_BUTTONS ];

  extern int psp_joy_load_settings(char *kbd_filename);
  extern int psp_joy_save_settings(char *kbd_filename);
  extern void psp_joy_default_settings();
  extern int psp_joy_load_settings_file(FILE *JoyFile);

# ifdef __cplusplus
}
# endif

# endif
