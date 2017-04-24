#!/bin/sh
# Run this to generate all the initial makefiles, etc.

autoreconf -vif -Wall -Wno-portability
./configure $*
