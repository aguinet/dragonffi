#!/bin/bash
#

OUT=$1
echo "Generating $OUT..."
shift
(
echo create $OUT
for l in $*; do
  echo addlib $l
done
echo save
echo end
) | ar -M
