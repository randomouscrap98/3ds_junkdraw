#!/bin/sh

set -e

echo "NOTE: You may have to run this twice depending on how old everything is"
pacman -Syu --needed vim 3ds-dev 3ds-libpng

echo "Done"
