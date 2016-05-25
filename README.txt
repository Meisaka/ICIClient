
ICIClient - network client and debugger for isiCPU

All platforms:
run "git submodule update --init --recursive"
before build attempt

  ------ Windows -------
Requirements:
The lib files for Winsock32 (WS2_32.lib) and OpenGL (OpenGL32.lib)
GLEW static (GL/glew.h, glew32s.lib) in include and lib path
SDL2 with headers in include path (SDL2/SDL*.h)
Something Microsofty to complile this stuff with

How to build it:
1. Add the paths in the project for those includes/libs if needed
2. Change the path for SDL2.lib if needed
3. Press build in the IDE - Good luck

  ------ Linux -------
Requirements:
build evironment is sane
SDL2 installed with headers accessable as <SDL2/SDL*.h>
GLEW most likely required.
CMake

How to build it:
1. run "cmake ."
2. run "make" and hope it doesn't spit errors

  ------- OS X -------

* none of this is tested

Requirements:
command line build tools
SDL2 installed somewhere
some compentency with C/C++ code
text editor
CMake

How to attempt to build it:
1. Edit ici.h
 1a. comment out or remove GL/glew include
 1b. replace with an include for GL3
2. Edit CMakeLists.txt
 2a. remove GLEW from libraries line
3. Run cmake on it
4. run make
5. ???
6. Profi...err I mean, if it works maybe send a patch
   unless I get to it.

  ------- How to use it ---------
run isiCPU on a server with the -s flag
run ICIClient
select Connect from the menu and enter the IP of the server
click ok, port can be left blank (0) for default.
if it worked, have fun, you can type in the output windows.
if not, oh well, figure it out and if it looks like a bug tell me about it.

