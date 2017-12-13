The GAP 4 package `CddInterface'
==============================

# Install

Make sure you can update "configure" scriptes by installing `autoconf`
    
    sudo apt-get install autoconf
    
or
    
    sudo apt-get install dh-autoreconf
    

## Simple

For a simplyfied installation, try the following two commands in the main CddInterface directory

    gap makedoc.g
    ./install.sh <path-to-gaproot>

If that does not work, try the following

## Advanced

Go inside the cddlib directory and create a directory `build` using the following commands:
    
    cd cddlib
    make clean
    mkdir build

After that, compile cddlib via

    ./bootstrap.sh
    ./configure --prefix=$(pwd)/build
    make
    make install

Cdd should now be installed in the `build` directory. After that, go back to the CddInterface main folder
and install CddInterface with the following commands

    gap makedoc.g
    ./autogen.sh
    ./configure --with-gaproot=path/to/gaproot --with-cddlib=$(pwd)/cddlib/build
    make

After that, you should be able to load CddInterface.
