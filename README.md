# Clip-8
Tiny chip-8 emulator written in C, using Raylib for graphics and input

## The what:
A tiny, cross-platform VM that runs chip-8 roms

## The why:
To learn a bit about how emulators and CPUs work, great learning project

## The how:
Don't be shy! take a look at the code!

# HOW TO COMPILE!!!

## Linux
CMake and Make are needed (on distros with apt, run `sudo apt update && sudo apt install cmake make`)
Clone the repo, and then run `cmake ..` from the /build folder. Afterwards, run `make`. Easy!

## Windows
I recommend MSYS2, worked great for me
CMake and Make are needed (run `pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-make` in the MINGW64 terminal)
run this in the MINGW64 terminal from the /build folder: `cmake ..`, then `mingw32-make` 
