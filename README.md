# Welcome to Bittboy MSX

## Original Authors
		Original fMSX            Marat Fayzullin (1994-2001).
		fMSX-SDL port            Vincent van Dam (2001).
		YM2413/PSG/SCC emulation Mitsutaka Okazaki (2001).
		Y8950 emulation          Tatsuyuki Satoh (1999/2000).
		Dingux port version      Ludovic.Jacomme also known as Zx-81 (zx81.zx81@gmail.com)
		Bittboy version          pthalin (https://github.com/pthalin)


## INTRODUCTION

fMSX emulates MSX, MSX2, and MSX2+ 8bit home computers. It runs MSX/MSX2/MSX2+
software on many different platforms including Windows, MacOS, Unix, MSDOS, 
AmigaOS, etc. See http://fms.komkon.org/fMSX/ for further informations.

Dingux MSX is a port on Dingoo/Dingux of my previous GP2X-Wiz port version.

The Dingux part of this package is under freeBSD license, read LICENSE.txt file 
for more information about it. Original fMSX code is under 
"Marat Fayzullin's license" (see http://fms.komkon.org/fMSX/ for details).

Bittboy-MSX is an adaptation of Dingux-MSX for Bittboy.

## INSTALLATION

Unzip the zip file, and copy the content of the directory local to your
SD card.

Put your rom image files on "roms" sub-directory.

For any comments or questions on this version, please visit 
https://github.com/pthalin

## CONTROL

### Main menu
		R      Main options menu

#### In the main menu
		A        Select
		TA/TB    Go Back
		SELECT   Reset the emulator

### Change Mapping
		START    Open/close the Virtual Keyboard Mapping	
		SELECT   Press for Alt. Mapping

#### Normal Mapping
		Bittboy    MSX 
		A          Fire A
		B          Fire B
		TA         F5 
		TB         F1 
		Up         Up
		Down       Down
		Left       Left
		Right      Right

#### Alt. Mapping
		Bittboy    MSX 
		A          Hotkey show FPS
		B          Return
		TA         Hotkey auto-fire enable/disable
		TB         Escape
		Up         Up
  		Down       Down
  		Left       Left
  		Right      Right

  Press Menu      to enter in emulator main menu.
  Press Select    open/close the virtual keyboard


#### Virtual Keyboard Mapping

Use digital pad to choose one of the 9 squares, and
use A, B, TA, TB to choose one of the 4 letters of the
highlighted square.

Use SELECT to switch between four different virtual keyboard panels.

## LOADING MSX ROM FILES

If you want to load rom image in your emulator, you have to put your rom
file (with .zip, .rom, .mx1 or .mx2 file extension) on your SD card
in the 'roms' directory.

Then, while inside DinguxMSX emulator, just press SELECT to enter in 
the emulator main menu, and then using the file selector choose one 
rom file to load in your emulator.

Back to the emulator window, the rom should stard automatically.

To eject a ROM choose "Eject Rom" inside the emulator menu.

You can use the virtual keyboard in the file requester menu to choose the
first letter of the game you search (it might be useful when you have tons of
games in the same folder). 

Entering several time the same letter let you choose sequentially files
beginning with the given letter. You can use the Run key of the virtual
keyboard to launch the rom.

You may use the Trigger key to swap between the two virtual keyboard panels
(numbers & letters)


## LOADING DISK FILES

If you want to load disk image in the virtual drive A or B of your emulator,
you have to put your disk file (with .dsk file extension, or gzipped disk file
with .dsz file extension) on your SD card in the 'disk' directory. 

Gzipped disk files are not writable.

You proceed as previously described for ROM files, and your disk is then
inserted in the drive 'A' of your emulator.

To start your disk, you have to choose the "Reset MSX" option and back to 
the emulator window, the MSX should reboot and stard your disk automatically.

You must eject any present rom before starting a disk, or only the 
ROM will start instead of your disk image.


<!-- ## COMMENTS

You can write your own comments for games using the "Comment" menu.  The first
line of your comments would then be displayed in the file requester menu while
selecting the given file name (snapshot, disk, keyboard, settings).
 -->

## LOADING KEY MAPPING FILES

For given games, the default keyboard mapping between Dingux Keys and
MSX keys, is not suitable, and the game can't be played on DinguxMSX.

To overcome the issue, you can write your own mapping file. Using notepad for
example you can edit a file with the .kbd extension and put it in the kbd 
directory.

For the exact syntax of those mapping files, have a look on sample files already
presents in the kbd directory (default.kbd etc...).

After writting such keyboard mapping file, you can load them using the main menu
inside the emulator.

If the keyboard filename is the same as the rom filename (.zip etc...)
then when you load this rom, the corresponding keyboard file is automatically 
loaded !

You can now use the Keyboard menu and edit, load and save your
keyboard mapping files inside the emulator. The Save option save the .kbd
file in the kbd directory using the "Game Name" as filename. The game name
is displayed on the right corner in the emulator menu.

If you have saved the state of a game, then a thumbnail image will
be displayed in the file requester while selecting any file (rom, disk,
keyboard, settings) with game name, to help you to recognize that game later.


## CHEAT CODE (.CHT)

You can use cheat codes with Dingux-MSX You can add your own cheat codes in the
cheat.txt file and then import them in the cheat menu.

All cheat codes you have specified for a game can be save in a CHT file 
in 'cht' folder.  Those cheat codes would then be automatically loaded
when you start the game.

The CHT file format is the following :

```
#
# Enable, Address, Value, Comment
#
1,36f,3,Cheat comment
```

Using the Cheat menu you can search for modified bytes in RAM between
current time and the last time you saved the RAM. It might be very usefull to
find "poke" address by yourself, monitoring for example life numbers.

To find a new "poke address" you can proceed as follow :

Let's say you're playing "1942" and you want to find the memory
address where "number lives" is stored.

* Start a new game in 1942
* Enter in the cheat menu. 
* Choose Save Ram to save initial state of the memory. 
* Specify the number of lives you want to find in
	"Scan Old Value" field.
	(for 1942 the initial lives number is 4)
* Go back to the game and loose 1 life.
* Enter in the cheat menu.
* Specify the number of lives you want to find in
	"Scan New Value" field.
	(for 1942 the lives number is now 3)
* In Add Cheat you have many matching Addresses
* Then go back to the game and loose more lives
	let's say up to 0.
* Enter in the cheat menu.
* Specify the number of lives you want to find in
	"Scan New Value" field.
	(for 1942 the lives number is now 0)
 In Add Cheat you have now only one matching Address
 Specify the Poke value you want (for example 4) 
	and add the new cheat with this address / value.
* Try this cheat while restarting a new game and see 
	if the life number is 4 or not. You will see that 
	the good address is 2D2F.

The cheat is now activated in the cheat list 
and you can save it using the "Save cheat" menu.

Let's enjoy 1942 with infinite life !!


## SETTINGS

You can modify several settings value in the settings menu of this emulator.
The following parameters are available :

```
Sound enable       : enable or disable the sound
Music FM-PAC       : emulate specific FM-PAC hardware (slow)
Music Module       : emulate specific music hardware (slow)
Sound volume       : Boost sound volume
Video Mode         : NTSC or PAL video standard
Speed limiter      : limit the speed to a given fps value
Skip frame         : to skip frame and increase emulator speed
Display fps        : display real time fps value 
MSX version        : MSX version 1, 2 or 2+
MSX Ram size       : memory size of MSX
Render mode        : many render modes are available with different 
			geometry that should covered all games requirements
V-Sync             : vertical synchronisation (slow down the emulator)
Clock frequency    : Clock frequency, by default the value is set to
			200Mhz, and should be enough for most of all games.
```


## JOYSTICK SETTINGS

You can modify several joystick settings value in the settings menu of this emulator.
The following parameters are available :

```
Auto fire period   : auto fire period
Auto fire mode     : auto fire mode active or not
```


## COMPILATION

It has been developped under Linux using gcc with BITTBOY Build root.

https://github.com/pthalin/buildroot

Build witha the Makefile in the src archive.
 
All tests have been done using a Bittboy.


