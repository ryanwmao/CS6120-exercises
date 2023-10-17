# licm

LICM pass
It's for LLVM 17.

Build:

    $ cd l8
    $ mkdir build
    $ cd build
    $ cmake ..
    $ make
    $ cd ..

Run:

    $ clang -fpass-plugin=`echo build/licm/SkeletonPass.*` something.c
