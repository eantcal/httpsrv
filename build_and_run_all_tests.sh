#!/bin/bash
echo This script has been designed to execute on Ubuntu-like Linux distribution
echo "Install any needed package (root password required)"
sudo apt install build-essential g++ cmake libboost-all-dev jsonlint curl unzip

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
