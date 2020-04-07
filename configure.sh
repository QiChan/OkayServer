#!/bin/sh

set -x
set -e
dir=$(pwd)

echo "POKER_ROOT=$(pwd)" >> ~/.bash_profile
echo "export POKER_ROOT" >> ~/.bash_profile
source ~/.bash_profile

if [ ! -e "3rd" ]; then
    git clone http://47.115.171.11:3000/cplusplus/3rd.git
    cd 3rd
    sh build.sh
    cd $dir
fi

if [ ! -e "common" ]; then
    git clone http://47.115.171.11:3000/develop/common.git
fi

if [ ! -e "publish" ]; then
    git clone http://47.115.171.11:3000/cplusplus/publish.git
    cd publish
    sh configure.sh
    cd $dir
fi
