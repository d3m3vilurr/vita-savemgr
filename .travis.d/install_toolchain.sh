#!/bin/bash
mkdir -p $VITASDK/tmp
mkdir -p $HOME/opt
cd $VITASDK/tmp

if [ ! -f $HOME/opt/lib/libjansson.so ]; then
    git clone --depth=50 https://github.com/akheron/jansson.git

    mkdir -p jansson/build
    cd jansson/build

    cmake -DCMAKE_INSTALL_PREFIX=$HOME/opt -DJANSSON_BUILD_SHARED_LIBS=ON -DJANSSON_BUILD_DOCS=OFF .. > /dev/null
    cmake --build . > /dev/null
    make install > /dev/null || exit 1

    cd ../..
    rm -rf jansson
fi

if [ ! -f $HOME/opt/lib/libzip.so ]; then
    git clone --depth=50 https://github.com/nih-at/libzip.git

    mkdir -p libzip/build
    cd libzip/build

    cmake -DCMAKE_INSTALL_PREFIX=$HOME/opt .. > /dev/null
    cmake --build . > /dev/null
    make all install > /dev/null

    cd ../..
    rm -rf libzip
fi

if [ ! -f $VITASDK/bin/arm-vita-eabi-gcc ]; then
    git clone https://github.com/ooPo/vitatoolchain.git

    mkdir -p vitatoolchain/build
    cd vitatoolchain/build

    bash ../scripts/001-binutils.sh > /dev/null || exit 1
    bash ../scripts/002-libelf.sh > /dev/null || exit 1
    bash ../scripts/003-vita-toolchain.sh > /dev/null || exit 1
    bash ../scripts/004-vita-headers.sh > /dev/null || exit 1
    bash ../scripts/005-gcc.sh > /dev/null || exit 1
    #bash ../scripts/006-gdb.sh > /dev/null || exit 1
    bash ../scripts/007-vdpm.sh > /dev/null || exit 1
    bash ../scripts/008-sdl-vita.sh > /dev/null || exit 1
    bash ../scripts/009-libftpvita.sh > /dev/null || exit 1

    cd ../..
    rm -rf vitatoolchain/build
fi
