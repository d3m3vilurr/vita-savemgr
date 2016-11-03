#!/bin/bash
mkdir -p $VITASDK/tmp
mkdir -p $HOME/opt
cd $VITASDK/tmp

if [ ! -f $VITASDK/bin/arm-vita-eabi-gcc ]; then
    mkdir vitatoolchain
    cd vitatoolchain
    TOOLCHAIN=$(curl -s https://api.github.com/repos/frangarcj/buildscripts/releases | grep "browser_download_url" | grep "linux" | head -n 1 | awk '{print $2}' | sed -r 's/"//g')
    curl -s -L -o vitatoolchain.tar.bz2 ${TOOLCHAIN}
    tar xjf vitatoolchain.tar.bz2
    mv vitasdk/* $VITASDK/

    curl https://raw.githubusercontent.com/ooPo/vitatoolchain/master/scripts/004-vita-headers.sh | bash
    curl https://raw.githubusercontent.com/ooPo/vitatoolchain/master/scripts/007-vdpm.sh | bash
    curl https://raw.githubusercontent.com/ooPo/vitatoolchain/master/scripts/008-sdl-vita.sh | bash
    curl https://raw.githubusercontent.com/ooPo/vitatoolchain/master/scripts/009-libftpvita.sh | bash

    cd ..
    rm -rf vitatoolchain
fi

cd ..
rm -rf $VITASDK/tmp
