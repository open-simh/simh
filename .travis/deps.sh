#!/bin/sh

install_osx() {
    brew update
    brew install pkg-config
    brew install pcre libpng libedit
    brew install sdl2 freetype2 sdl2_ttf
    brew install vde
    brew install cmake gnu-getopt coreutils
}

install_linux() {
    sudo apt-get update -yqqm
    sudo apt-get install -ym pkg-config
    sudo apt-get install -ym libpcre3-dev libpng-dev libedit-dev
    sudo apt-get install -ym libegl1-mesa-dev libgles2-mesa-dev
    sudo apt-get install -ym libsdl2-dev libfreetype6-dev libsdl2-ttf-dev
    sudo apt-get install -ym libpcap-dev libvdeplug-dev
    sudo apt-get install -ym cmake cmake-data
}

install_mingw64() {
    pacman -S --needed mingw-w64-x86_64-ninja \
	mingw-w64-x86_64-cmake \
	mingw-w64-x86_64-extra-cmake-modules \
        mingw-w64-x86_64-gcc \
	mingw-w64-x86_64-make \
        mingw-w64-x86_64-pcre \
	mingw-w64-x86_64-freetype \
        mingw-w64-x86_64-SDL2 \
	mingw-w64-x86_64-SDL2_ttf \
	mingw-w64-x86_64-libpcap
}

install_ucrt64() {
    pacman -S --needed mingw-w64-ucrt-x86_64-ninja \
	mingw-w64-ucrt-x86_64-cmake \
	mingw-w64-ucrt-x86_64-extra-cmake-modules \
        mingw-w64-ucrt-x86_64-gcc \
	mingw-w64-ucrt-x86_64-make \
        mingw-w64-ucrt-x86_64-pcre \
	mingw-w64-ucrt-x86_64-freetype \
        mingw-w64-ucrt-x86_64-SDL2 \
	mingw-w64-ucrt-x86_64-SDL2_ttf \
	mingw-w64-ucrt-x86_64-libpcap
}

install_clang64() {
    pacman -S --needed mingw-w64-clang-x86_64-ninja \
	mingw-w64-clang-x86_64-cmake \
	mingw-w64-clang-x86_64-extra-cmake-modules \
        mingw-w64-clang-x86_64-clang \
	mingw-w64-clang-x86_64-make \
        mingw-w64-clang-x86_64-pcre \
	mingw-w64-clang-x86_64-freetype \
        mingw-w64-clang-x86_64-SDL2 \
	mingw-w64-clang-x86_64-SDL2_ttf \
	mingw-w64-clang-x86_64-libpcap
}


case "$1" in
  osx|linux|mingw64|ucrt64|clang64)
    install_"$1"
    ;;
  *)
    echo "$0: Need an operating system name: osx, linux, mingw64 or ucrt64"
    exit 1
    ;;
esac
