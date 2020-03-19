/*
  This file is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version. 

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.

  Some parts are based on cas tools by Vincent van Dam (https://github.com/joyrex2001)  

  Authour: pthalin (https://github.com/pthalin/)

*/

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "tape.h"

char CAS_HEADER[8]  = { 0x1F,0xA6,0xDE,0xBA,0xCC,0x13,0x7D,0x74 };
char CAS_ASCII[10]  = { 0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA };
char CAS_BINARY[10] = { 0xD0,0xD0,0xD0,0xD0,0xD0,0xD0,0xD0,0xD0,0xD0,0xD0 };
char CAS_BASIC[10]  = { 0xD3,0xD3,0xD3,0xD3,0xD3,0xD3,0xD3,0xD3,0xD3,0xD3 };

enum cas_type_t cas_type(char * filename)
{
  FILE *ifile;
  char buffer[10];
  long position = 0;
  enum cas_type_t type = ERROR;
 
  if ( (ifile = fopen(filename,"rb")) == NULL) {
   return (type);
  }

  while (fread(buffer, 1, 8, ifile) == 8) {   
    if (!memcmp(buffer, CAS_HEADER, 8)) { 
      if (fread(buffer, 1, 10, ifile) == 10) {
	if (!memcmp(buffer, CAS_ASCII, 10)) {
          type = ASCII;
	  break;
	} 
	else if (!memcmp(buffer, CAS_BINARY, 10)) {  
	  type = BINARY;
	  break;
	}
	else if (!memcmp(buffer, CAS_BASIC, 10)) { 
          type = BASIC;
	  break;
	}
	else  {
	  type = UNKOWN;
	  break;	
        }
      }
    }	
    fseek(ifile, ++position, SEEK_SET);   
  }   
  fclose(ifile);
 
  return(type);
}
