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

# ifndef _GP2X_FMGR_H_
# define _GP2X_FMGR_H_

#ifdef __cplusplus
extern "C" {
#endif

# define GP2X_FMGR_MAX_PATH    512
# define GP2X_FMGR_MAX_NAME    256
# define GP2X_FMGR_MAX_ENTRY  2048

# define FMGR_FORMAT_ROM   1
# define FMGR_FORMAT_STATE 2
# define FMGR_FORMAT_KBD   3
# define FMGR_FORMAT_JOY   4
# define FMGR_FORMAT_CHT   5
# define FMGR_FORMAT_DISK  6
# define FMGR_FORMAT_ZIP   7
# define FMGR_FORMAT_SET   8

  extern int psp_fmgr_menu(int format, int drive_id);
  extern int psp_fmgr_get_dir_list(char *basedir, int dirmax, char **dirname);

#ifdef __cplusplus
}
#endif

# endif
