#!/bin/bash
echo This script has been designed to execute on Ubuntu-like Linux distribution
echo New C++ compiler supporting C++17 is a requirement too.
echo If you want to install GNU compiler version 10 you can use the following commands, valid for Ubuntu 18.04 or higher:
echo
echo sudo add-apt-repository ppa:ubuntu-toolchain-r/test
echo sudo apt install gcc-10 gcc-10-base gcc-10-doc g++-10 libstdc++-10-dev libstdc++-10-doc 
echo
echo Make sure symlinks point to the new toolchain before proceeding with cmake commands executing the following commands:
ls -la /usr/bin/ | grep -oP "[\S]*(gcc|g\+\+)(-[a-z]+)*[\s]" | xargs bash -c 'for link in ${@:1}; do echo sudo ln -s -f "/usr/bin/${link}-${0}" "/usr/bin/${link}"; done' 10
echo 
echo "Install any other dependencies (root password required)"
sudo apt install build-essential g++ cmake libboost-all-dev jsonlint curl unzip uuid-dev

set -x

echo "Re-build HttpSrv"
ok=0
rm -rf build && mkdir build && cd build && cmake .. && make && ok=1
if [ ok = "0" ]; then
  echo "Build failed " >&2
  exit 1
fi

pkill httpsrv && sleep 5

./httpsrv -vv > ./httpsrv.log & 

ok=1
grep -i error httpsrv.log && ok=0

if [ ok = "0" ]; then
  echo "Can't run httpsrv" >&2
  exit 1
fi

cd ../test

./functional_test.sh
