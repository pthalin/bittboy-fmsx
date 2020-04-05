#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int   u32;

#include "gp2x_psp.h"
#include "gp2x_cpu.h"
#include <time.h>

# ifndef CLK_TCK
# define CLK_TCK  CLOCKS_PER_SEC
# endif

//LUDO:
enum {
MSX_RENDER_FAST,
// MSX_RENDER_ZOOMED,
// MSX_RENDER_FULLSCREEN,
// MSX_RENDER_NORMAL,
MSX_RENDER_FIT,
MSX_RENDER_ZOOM,
MSX_RENDER_FULL,
MSX_LAST_RENDER
};

# define MAX_PATH   256
# define MSX_MAX_SAVE_STATE 5

# define MSX_WIDTH    272
# define MSX_HEIGHT   228
# define SCR_WIDTH    MSX_WIDTH
# define SCR_HEIGHT   MSX_HEIGHT
# define SNAP_WIDTH   (MSX_WIDTH/2)
# define SNAP_HEIGHT  (MSX_HEIGHT/2)

#include <SDL.h>

# define MSX_MAX_CHEAT    10

#define MSX_CHEAT_NONE    0
#define MSX_CHEAT_ENABLE  1
#define MSX_CHEAT_DISABLE 2

#define MSX_CHEAT_COMMENT_SIZE 25

#define MSX_MAX_RAM_PAGE  32

  typedef struct MSX_cheat_t {
    unsigned char  type;
    unsigned short addr;
    unsigned char  value;
    char           comment[MSX_CHEAT_COMMENT_SIZE];
  } MSX_cheat_t;

  typedef struct MSX_save_t {

    SDL_Surface    *surface;
    char            used;
    char            thumb;
    time_t          date;

  } MSX_save_t;

  typedef struct MSX_t {

    MSX_save_t msx_save_state[MSX_MAX_SAVE_STATE];
    MSX_cheat_t msx_cheat[MSX_MAX_CHEAT];

    int        comment_present;
    char       msx_save_name[MAX_PATH];
    char       msx_home_dir[MAX_PATH];
    int        msx_speed_limiter;
    int        msx_version;
    int        msx_ram_pages;
    int        msx_ntsc;
    int        psp_screenshot_id;
# if !defined(CAANOO_MODE)
    int        psp_cpu_clock;
    int        msx_current_clock;
# endif
    int        bittboy_ta_func;
    int        bittboy_tb_func;
    int        joy_cursor_keys;
    int        psp_sound_volume;
    int        msx_snd_enable;
    int        msx_view_fps;
    int        msx_current_fps;
    int        msx_render_mode;
    int        msx_vsync;
    int        psp_skip_max_frame;
    int        psp_skip_cur_frame;
    int        msx_use_2413;
    int        msx_use_8950;
    int        msx_uperiod;
    int        msx_auto_fire_period;
    int        msx_auto_fire;
    int        msx_auto_fire_pressed;
    int        msx_megarom_type;

  } MSX_t;

  extern MSX_t MSX;


//END_LUDO:

# endif
