# Chip-8-Emulator

## How to Build & Run on Windows

## First Clone the Repo and Get the needed submodules for testing
```
git clone https://github.com/FLavenNA/Chip-8-Emulator.git
git submodule update --init --recursive
```

## Release Build
    cmake -S . -B build
## Debug Build
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug

## Compile And Run The Code
    cmake --build build
    ./path_to_your.exe <rom_path>
### For Example
    .\build\Debug\Chip-8-emulator.exe '.\test-roms\chip8-roms\programs\IBM Logo.ch8'

## Keys

- **"Escape"**  : Exit Window
- **"Space"**   : Pause
- **"*"**       : Reset CHIP8 machine for current rom
- **"J"**       : Decrease color lerp rate
- **"K"**       : Increase color lerp rate
- **"O"**       : Decrease volume
- **"P"**       : Increase volume