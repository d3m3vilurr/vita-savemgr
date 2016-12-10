#!/bin/bash
mkdir -p $VITASDK/tmp
mkdir -p $HOME/opt
cd $VITASDK/tmp

if [ ! -f $VITASDK/bin/arm-vita-eabi-gcc ]; then
    mkdir vitatoolchain
    cd vitatoolchain
    curl -s https://api.github.com/repos/vitasdk/autobuilds/releases | grep browser_download_url | grep linux | head -n 1 | cut -d '"' -f 4 | xargs curl -L | tar xj
    mv vitasdk/* $VITASDK/

    curl https://raw.githubusercontent.com/ooPo/vitatoolchain/master/scripts/004-vita-headers.sh | bash
    curl https://raw.githubusercontent.com/ooPo/vitatoolchain/master/scripts/007-vdpm.sh | bash

    cd ..
    rm -rf vitatoolchain
fi

cd ..
rm -rf $VITASDK/tmp
