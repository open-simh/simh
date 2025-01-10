#!/bin/sh

install_osx() {
    brew update
    brew install pkg-config pcre libpng libedit sdl2 freetype2 sdl2_ttf \
        vde cmake gnu-getopt coreutils zlib
}

install_macports() {
    sudo port install pkgconfig pcre libpng libedit libsdl2 freetype libsdl2_ttf \
        vde2 cmake util-linux coreutils zlib
}

install_arch_linux() {
    sudo pacman -S --noconfirm pkgconf
    sudo pacman -S --noconfirm pcre libpng libedit
    sudo pacman -S --noconfirm mesa
    sudo pacman -S --noconfirm libsm
    sudo pacman -S --noconfirm cmake

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

install_mingw32() {
    ## Doesn't have libpcap or cmake's extra modules. Not that this
    ## makes much of a difference.
    pacman -S --needed mingw-w64-i686-ninja \
	mingw-w64-i686-cmake \
        mingw-w64-i686-gcc \
	mingw-w64-i686-make \
        mingw-w64-i686-pcre \
	mingw-w64-i686-freetype \
        mingw-w64-i686-SDL2 \
	mingw-w64-i686-SDL2_ttf
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
  osx|macports|linux|mingw32|mingw64|ucrt64|clang64)
    install_"$1"
    ;;
  arch-linux)
    install_arch_linux
    ;;
  *)
    echo "$0: Need an operating system name: osx, arch-linux, linux, mingw64 or ucrt64"
    exit 1
    ;;
esac
