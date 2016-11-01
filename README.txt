
ICIClient - network client and debugger for isiCPU

All platforms:
run "git submodule update --init --recursive"
before build attempt

  ------ Windows -------
Requirements:
 - The lib files for Winsock32 (WS2_32.lib) and OpenGL (OpenGL32.lib)
 - GLEW static (GL/glew.h, glew32s.lib)
 - SDL2 libraries (SDL2.lib, SDL2.dll) and headers (SDL*.h)
 - Something Microsoft-y to complile this stuff with
 - CMake (optional, but recommended)

How to build it (with CMake):
1. Run cmake and supply a seperate directory for the build
   ( ".\build" is a good choice )
2. click Configure in cmake and fill-in any missing paths (you have everything, right?)
  2a. if you have SDL2DIR in your environment, cmake will use it.
3. Generate and open
4. Build it (Release or Debug)
5. Run!

How to build it (without CMake, probably):
1. Add the paths in the included project files for all those
   troublesome includes/libs as needed.
2. Change the path for SDL2.lib if needed
3. probably also need to add Ws2_32.lib
4. Press build in the IDE - Good luck
5. Run!
6. do not commit the project files again, they aren't supported.

  ------ Linux -------
Requirements:
build evironment is sane
SDL2 installed with headers accessable as <SDL*.h>
GLEW
CMake

How to build it:
1. do this: "mkdir build && cd build && cmake .."
  1a. you can also use interactive/gui versions of CMake
2. check and make sure it is using SDL2 (not SDL 1.x or you will have problems)
3. "make" hope it doesn't spit errors
4. Run!

  ------- OS X -------

* none of this is tested

Requirements:
 - command line build tools
 - SDL2 installed somewhere
 - some compentency with C/C++ code
 - text editor
 - CMake (version 2.8 or newer)

Proceedure:
1. Run cmake on it
2. run make or try building
2a. (maybe) Edit ici.h, replace with an include for GL3 (if it doesn't work)
3. ???
4. Try to run!
5. Contribute

  ------- How to use it ---------
run isiCPU on a server with the -s flag
run ICIClient
select Connect from the menu and enter the IP of the server
click ok, port can be left blank (0) for default.
if it worked, have fun, you can type in the output windows.
if it doesn't work: figure it out or tell me about it.

 ( irc.freenode.net #Yukara-dev )

