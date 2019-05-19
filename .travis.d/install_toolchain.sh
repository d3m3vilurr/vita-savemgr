#!/bin/bash
mkdir -p $VITASDK/tmp
mkdir -p $HOME/opt
cd $VITASDK/tmp

get_download_link () {
  curl -sL https://github.com/vitasdk/vita-headers/raw/master/.travis.d/last_built_toolchain.py | python - $@
}

if [ ! -f $VITASDK/bin/arm-vita-eabi-gcc ]; then
    mkdir vitatoolchain
    cd vitatoolchain
    curl -L $(get_download_link master linux) | tar xj
    mv vitasdk/* $VITASDK/

    curl https://raw.githubusercontent.com/ooPo/vitatoolchain/master/scripts/007-vdpm.sh | bash

    cd ..
    rm -rf vitatoolchain
fi

cd ..
rm -rf $VITASDK/tmp
