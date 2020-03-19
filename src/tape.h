
#ifndef __TAPE_H__
#define __TAPE_H__

#ifdef __cplusplus
extern "C" {
#endif

enum cas_type_t {ASCII, BINARY, BASIC, UNKOWN, ERROR};

enum cas_type_t cas_type(char * filename);

#ifdef __cplusplus
}
#endif

#endif
