# Game-A-Week Challenge
One small game, written from scratch, every week!

## Build
Make sure all libraries are linked or copied to lib.

1. Make the build directory `mkdir build`
2. Create cmake build files `cmake --preset release`
3. Build `cmake --build build/release`

### Dependencies
* [cglm (v>=0.9.6)](https://github.com/recp/cglm)
* [sdl3 (v>=3.2.4)](https://github.com/libsdl-org/SDL)
* [sdl3_ttf (v>=3.2.0)](https://github.com/libsdl-org/SDL)

#### Note
We are currently using the latest nightly updates for SDL. To this end, it is recommended to clone each repository and build the latest shared libraries locally and sym-link them into lib.

## Debugging

1. Create debug build files `cmake --preset debug`
2. Build `cmake --build build/debug`

## LSP Support
Copy the compile_commands.json file from `build/debug` to the project root. Then, clangd should be able to locate dependencies. 
