#!/bin/bash
mkdir -p $VITASDK/tmp
cd $VITASDK/tmp

curl -sL https://github.com/TheOfficialFloW/VitaShell/archive/master.tar.gz | tar xz
cd $VITASDK/tmp/VitaShell*/modules/kernel
cmake .
make
make install

cd $VITASDK/tmp/VitaShell*/modules/user
cmake .
make
make install

cd $VITASDK
rm -rf $VITASDK/tmp
