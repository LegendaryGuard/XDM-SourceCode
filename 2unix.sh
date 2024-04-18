#!/bin/sh
find ./ -type f \( -iname '*.c*' -o -iname '*.h*' \) -exec dos2unix {} \;