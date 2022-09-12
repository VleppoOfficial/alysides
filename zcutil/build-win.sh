#!/bin/bash
export HOST=x86_64-w64-mingw32
CXX=x86_64-w64-mingw32-g++-posix
CC=x86_64-w64-mingw32-gcc-posix
PREFIX="$(pwd)/depends/$HOST"

set -eu -o pipefail

UTIL_DIR="$(dirname "$(readlink -f "$0")")"
BASE_DIR="$(dirname "$(readlink -f "$UTIL_DIR")")"
PREFIX="$BASE_DIR/depends/$HOST"

# disable for code audit
# If --enable-websockets is the next argument, enable websockets support for nspv clients:
WEBSOCKETS_ARG=''
# if [ "x${1:-}" = 'x--enable-websockets' ]
# then
# WEBSOCKETS_ARG='--enable-websockets=yes'
# shift
# fi

# make dependences
cd depends/ && make HOST=$HOST V=1 NO_QT=1
cd ../

cd $BASE_DIR/depends
make HOST=$HOST NO_QT=1 "$@"
cd $BASE_DIR

./autogen.sh
CONFIG_SITE=$BASE_DIR/depends/$HOST/share/config.site CXXFLAGS="-DPTW32_STATIC_LIB -DCURL_STATICLIB -DCURVE_ALT_BN128 -fopenmp -pthread" ./configure --prefix=$PREFIX --host=$HOST --enable-static --disable-shared "$WEBSOCKETS_ARG" \
  --with-custom-bin=yes CUSTOM_BIN_NAME=alysides CUSTOM_BRAND_NAME=Alysides \
  CUSTOM_SERVER_ARGS="'-ac_name=VLCGLB1 -ac_supply=498000000 -ac_reward=1000000000 -ac_blocktime=120 -ac_cc=1 -ac_staked=100 -ac_halving=27600 -ac_decay=85000000 -ac_end=331200 -ac_public=1 -addnode=143.110.242.177 -addnode=143.110.254.96 -addnode=139.59.110.85 -addnode=139.59.110.86 -nspv_msg=1'" \
  CUSTOM_CLIENT_ARGS='-ac_name=VLCGLB1'
sed -i 's/-lboost_system-mt /-lboost_system-mt-s /' configure 
  
cd src/
# note: to build alysidesd, alysides-cli it should not exist 'komodod.exe komodo-cli.exe' param here:
CC="${CC} -g " CXX="${CXX} -g " make V=1    
