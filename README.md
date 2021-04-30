# wasm-fluid-sim

Try a demo in the browser [here](https://www.studiostudios.net/wasm-fluid-sim/main_sim_sdl.html)!

This is a C++ implementation of the particle-based fluid simulation method described in [this paper](http://www.ligum.umontreal.ca/Clavet-2005-PVFS/pvfs.pdf). This project was done as an exercise to compare the performance with my [javascript implementation of the algorithm](https://github.com/abobco/webgl-fluid-sim), and to learn about more about writing web applications in C++. 

The project also includes a native application for x64 windows, which can be run with:

```
# From the root folder of the project:
cd build
./Release/main_sim_sdl.exe
```

## Building from source

This project uses [CMake](https://cmake.org/download/) for the build process, so install that if you don't have it already.

### Windows (MSVC)
```
# From the root folder of the project:
cd build
rm CMakeCache.txt
cmake ..
cmake --build . --config Release
```

### WebAssembly (Emscripten)
First, install the Emscripten compiler from [here](https://emscripten.org/docs/getting_started/downloads.html) if you haven't already.
Then, from your preferred terminal:
```
# From the root folder of the project:
cd build_wasm
rm CMakeCache.txt
emcmake cmake .
cmake --build . --config Release
```
