#! /usr/bin/env bash

if ! command -v brew &> /dev/null
then
	# install Homebrew if not installed yet
	/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install.sh)"
	eval "$(/usr/local/bin/brew shellenv)"
fi

brew update
brew install cmake ninja ccache geographiclib git SDL2 exiv2 expat zlib shapelib pkgconf create-dmg
