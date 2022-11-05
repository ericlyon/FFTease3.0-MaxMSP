# FFTease3.0-MaxMSP

http://disis.music.vt.edu/eric/main/fftease/


## Build requirements

- git
- CMake >=3.19
    - https://cmake.org/download/ 
    - or via `brew install cmake` on Mac
- Xcode >= 10 or Visual Studio >= 2017

## Building

1. `cd` into your Max packages directory.

2. Get this repository along with the `max-sdk-base` submodule

```sh
git clone --recurse-submodules https://github.com/ericlyon/FFTease3.0-MaxMSP.git
```

3. Create a build directory where all IDE files and intermediary CMake files will live. Usually this
   is called `build`.

```sh
mkdir build
cd build
```

4. Find which CMake generators are available. CMake is used to generate the projects (e.g. Xcode
   project, VS solution, Makefiles, etc.), so this is where you can pick. 

```sh
cmake --help
```

From here, you'll see some options. On Mac, for example, you will see something like Unix Makefiles,
Ninja, and Xcode as options. On Windows, you'll likely see Visual Studio on this list. This is your
"generator" that you'll specify on the next step. The default generator will have a `*` next to it.

5. Generate the projects

For example, to create Xcode projects for each external, do the following from the `build/`
directory.

```sh
cmake -G "Xcode" ..
```

6. Build the projects (e.g. the externals)

From here, you can open each project (e.g. with Xcode) and build as usual. You can also build all
targets in the project with a single command, again from the `build/` directory

```sh
cmake --build .
```
