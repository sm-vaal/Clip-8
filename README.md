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

# USAGE INSTRUCTIONS
As simple as it gets. Place your ROMs in the executable's folder. Run the executable, and write the rom name. That's it!

# CONTROLS
Chip-8 uses a 4x4 keypad as inputs, here it's mapped like this:
`
      CHIP 8          KEYBOARD
      1 2 3 C         1 2 3 4
      4 5 6 D   ==>   Q W E R
      7 8 9 E         A S D F
      A 0 B F         Z X C V
`
