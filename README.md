cgc1
====

Conservative Garbage Collector in C++11

Build
====
Use cmake 2.8.1 or greater to generate build.
This currently requires at least clang version 4.0.0 (trunk 283082) on OSX and linux.
For GCC, gcc version 7.1 or later is required with associated libstdc++.
For clang, clang version 4.0.0 or later is required with associated libc++
This should mostly compile with current visual studio but currently segfaults during static initialization.