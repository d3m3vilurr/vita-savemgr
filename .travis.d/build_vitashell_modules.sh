#!/bin/bash
mkdir -p $VITASDK/tmp
cd $VITASDK/tmp

curl -sL https://github.com/TheOfficialFloW/VitaShell/archive/master.tar.gz | tar xz
cd $VITASDK/tmp/VitaShell*/modules/kernel
cmake .
make
make install
cp kernel.skprx $TRAVIS_BUILD_DIR/sce_sys/kernel.skprx

cd $VITASDK/tmp/VitaShell*/modules/user
cmake .
make
make install
cp user.suprx $TRAVIS_BUILD_DIR/sce_sys/user.suprx

cd $VITASDK
rm -rf $VITASDK/tmp
