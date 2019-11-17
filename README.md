# CodeSim

This repository contains a tool to compute similarity of codes, based on libclang.

# Requirements

* `libclang`
* `clang`

In Ubuntu 18.04 you can run this command:

    sudo apt install clang libclang-dev

# Building the executable

    $ mkdir build
    $ cd build
    $ cmake ..
    $ make

CMake will try to find your llvm location. If that fails on your system, you should manually set it.

# Run the tool

    $ ./codesim [-h|--help] [-v|--verbose] code1.cpp code2.cpp

# Additional information

* https://clang.llvm.org/doxygen/group__CINDEX.html
* https://github.com/Pseudomanifold/libclang-experiments
