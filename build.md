## Table of Contents

* [Binary packages](https://github.com/cloose/CuteMarkEd/wiki/Build-Instructions#binary-packages)
* [Get the sources](https://github.com/cloose/CuteMarkEd/wiki/Build-Instructions#get-the-sources)
* [Ubuntu](https://github.com/cloose/CuteMarkEd/wiki/Build-Instructions#ubuntu)
    * [Ubuntu 12.04](https://github.com/cloose/CuteMarkEd/wiki/Build-Instructions#ubuntu-1204)
    * [Ubuntu 14.04](https://github.com/cloose/CuteMarkEd/wiki/Build-Instructions#ubuntu-1404)
* [Linux Mint](https://github.com/cloose/CuteMarkEd/wiki/Build-Instructions#linux-mint)
* [Mac OS X](https://github.com/cloose/CuteMarkEd/wiki/Build-Instructions#mac-os-x)

## Binary packages

For some Linux distributions there are already up-to-date binary packages available:
[[Installation on Linux]]

## Get the sources

Clone the CuteMarkEd repository including all submodules

    git clone --recursive https://github.com/cloose/CuteMarkEd.git

or

    git clone https://github.com/cloose/CuteMarkEd.git
    cd CuteMarkEd
    git submodule init 
    git submodule update

## Ubuntu
### Ubuntu 12.04

Tested with Ubuntu 12.04 LTS.
Tested with ArchLinux.

#### Install dependencies

    sudo add-apt-repository -y ppa:canonical-qt5-edgers/qt5-proper # for Qt5
    sudo add-apt-repository -y ppa:ubuntu-sdk-team/ppa # for Qt5
    sudo apt-get update
    sudo apt-get install -qq ubuntu-sdk libc6:i386 libgstreamer-plugins-base0.10 libgstreamer-plugins-base0.10-dev 

#### Generate translations

    lrelease app/translations/cutemarked_de.ts -qm app/translations/cutemarked_de.qm
    lrelease app/translations/cutemarked_cs.ts -qm app/translations/cutemarked_cs.qm

#### Compile

We need to also compile the discount project because Ubuntu has only packages for version 2.1.5 at the moment.

    pushd .
    cd 3rdparty/discount && ./configure.sh --enable-all-features --with-fenced-code && make && sudo make install ;
    popd
    qmake CuteMarkEd.pro
    make

### Ubuntu 14.04

Tested with Ubuntu 14.04 and Mint 17.

#### Install Build-Tools

Checkinstall is optional. Is only needed for building a DEB-Package.

    sudo apt-get install build-essential checkinstall

#### Install dependencies

    sudo apt-get install libqt5webkit5-dev qttools5-dev-tools qt5-default \
                         discount libmarkdown2-dev libhunspell-dev

#### Generate translations

    lrelease app/translations/cutemarked_de.ts -qm app/translations/cutemarked_de.qm
    lrelease app/translations/cutemarked_cs.ts -qm app/translations/cutemarked_cs.qm

#### Compile

    qmake CuteMarkEd.pro
    make

#### Create und install DEB-Package

If you what, you can create a DEB-Package und install it with 

    echo "A Qt-based Markdown editor with live HTML preview and syntax highlighting of markdown document." > description-pak
    sudo checkinstall --requires "libqt5webkit5, libmarkdown2, libhunspell-1.3-0, discount"

After that, create a symbolic link for the programm

    sudo ln -s /usr/lib/x86_64-linux-gnu/qt5/bin/cutemarked /usr/local/bin/

and install the icon

    sudo mkdir -p /usr/local/share/icons
    sudo cp app/icons/scalable/cutemarked.svg /usr/local/share/icons/cutemarked.svg

#### Install Qt5 plugin for fctix

If you use [fctix](https://fcitx-im.org/wiki/Fcitx) as input method framework, please make sure that you also install the Qt5 plugin for it.

    sudo apt-get install fcitx-libs-qt5

## Linux Mint

See the following blog post for instruction how to install CuteMarkEd on Linux Mint 15: http://bakedroy-note.blogspot.com/2013/11/installing-cutemarked-on-linux-mint-15.html?_sm_au_=i6HrqqNN4lf16S7s

### Mac OS X

Tested with Mac  OS X Yosemite.

#### Install dependencies

    brew install discount
    brew install hunspell

#### Generate translations

    lrelease app/translations/cutemarked_de.ts -qm app/translations/cutemarked_de.qm
    lrelease app/translations/cutemarked_cs.ts -qm app/translations/cutemarked_cs.qm

#### Compile

    qmake CuteMarkEd.pro
    make

#### Run

Goto app directory and double click cutemarked.app to run.
