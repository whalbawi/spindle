# Spindle
Spindle is a C++ project written in C++14.

## Prerequisites
The project is built using [CMake](https://cmake.org/) and requires version 3.20 or newer to work correctly.

Spindle takes a dependency on [googletest](https://github.com/google/googletest) for unit tests and is included
in the source tree as a `git` submodule. Run the following to check out the module and make it available
to the build system:
```bash
$ cd <path-to-spindle>
$ git submodule update --init
```

## Building
Spindle is tested on Linux and macOS.

The following will generate the build system and then build all targets:
```bash
$ cd <path-to-spindle>
$ cmake -B build
$ cmake --build build
```

## Tests
Start by building the unit tests executable:
```bash
$ cmake --build build --target spindle-tests
```

Now run the unit tests using CMake's `ctest` utility:
```bash
$ ctest --test-dir build -R unit-tests -V
```

Alternatively, run the tests executable directly:
```bash
$ ./build/spindle-tests
```
