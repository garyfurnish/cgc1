cgc1
====

Conservative Garbage Collector in C++11

Build
====
Use cmake 2.8.1 or greater to generate build.
This currently requires at least clang version 4.0.0 (trunk 283515) on OSX and linux.
For GCC, gcc version 7.0.0 20161009 (experimental) (GCC) or later from trunk is required on linux.
On OSX, a current build of libcxx also installed with clang is required.
This should mostly compile with VS "15" Preview 5 but currently segfaults during static initialization.