#!/bin/sh

find . -name Makefile -delete
find . -name Makefile.in -delete

autoreconf -f -i

if [ -z "$NOCONFIGURE" ]; then
    ./configure "$@"
fi
