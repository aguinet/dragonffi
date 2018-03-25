#!/bin/bash
#

set -ex
DIR=$(dirname $(realpath $0))

sudo docker build  -t dffi_multiarch "$DIR"
