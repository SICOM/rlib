#!/bin/sh
# Run this to generate all the initial makefiles, etc.

autoreconf -vif -W all -Wno-portability || exit 1
./configure $*
