#!/bin/bash
#

OUT=$1
echo "Generating $OUT..."
shift
/usr/bin/libtool -static -o "$OUT" $*
