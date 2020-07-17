CHAINPREFIX=/opt/buildroot-cfw-bittboy/output/host
CROSS_COMPILE=$(CHAINPREFIX)/bin/arm-buildroot-linux-musleabi-
MSX_VERSION=1.4.0

CC = $(CROSS_COMPILE)gcc
CXX = $(CROSS_COMPILE)g++
STRIP = $(CROSS_COMPILE)strip
SYSROOT     := $(shell $(CC) --print-sysroot)
SDL_CFLAGS  := $(shell $(SYSROOT)/usr/bin/sdl-config --cflags)
SDL_LIBS    := $(shell $(SYSROOT)/usr/bin/sdl-config --libs)

OUTPUTNAME = ./bittboy-msx/bittboy-msx
DEFINES = -DHAVE_STDINT_H -DVERSION_BITTBOY 
INCLUDES = -Iinclude $(SDL_CFLAGS)

CFLAGS = $(DEFINES) $(INCLUDES) $(OPT_FLAGS) 
#-DSDL 
CFLAGS += -DFMSX -DUNIX -DNARROW -DSOUND -DBPP16 -DLSB_FIRST
CFLAGS += -DDINGUX_MODE
CFLAGS += -DLINUX_MODE
CFLAGS += -DNO_STDIO_REDIRECT
CFLAGS += -DMSX_VERSION=\"$(MSX_VERSION)\"
CFLAGS += -O2 
CXXFLAGS = $(DEFINES) $(INCLUDES) $(OPT_FLAGS) 
LDFLAGS = -s -Wl,--start-group -lasound -lSDL -lSDL_image -lpng -ljpeg -lSDL_mixer -lmikmod  -lm -lz -Wl,--end-group

# Redream (main engine)
OBJS =  \
 ./src/gp2x_psp.o \
./src/cpudingux.o \
./src/fMSX.o ./src/MSX.o ./src/Patch.o ./src/Sound.o ./src/Disk.o \
./src/Z80.o ./src/I8255.o ./src/AY8910.o ./src/YM2413.o ./src/SCC.o ./src/V9938.o ./src/I8251.o \
./src/SDLgfx.o ./src/SDLsnd.o ./src/SDLfilter.o \
./src/emu2413.o ./src/emu2212.o ./src/emu2149.o \
./src/fmopl.o ./src/ymdeltat.o \
./src/psp_main.o \
./src/psp_sdl.o \
./src/psp_kbd.o \
./src/psp_font.o \
./src/psp_menu.o \
./src/psp_joy.o \
./src/psp_danzeff.o \
./src/psp_menu_set.o \
./src/psp_menu_help.o \
./src/psp_menu_joy.o \
./src/psp_menu_tape.o \
./src/psp_menu_cheat.o \
./src/psp_menu_list.o \
./src/psp_editor.o \
./src/unzip.o \
./src/ioapi.o \
./src/sha1.o \
./src/tape.o \
./src/psp_fmgr.o


.c.o:
	$(CC) $(CFLAGS) -c -o $@ $< 

.cpp.o:
	$(CXX) $(CXXFLAGS) -c -o $@ $< 

all: $(OBJS)
	$(CC) -o $(OUTPUTNAME) $(OBJS) $(CFLAGS) $(LDFLAGS)

clean:
	rm -f $(OBJS) $(OUTPUTNAME)
