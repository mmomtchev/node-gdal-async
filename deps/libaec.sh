#!/bin/bash

set -eu

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$DIR/libaec"

AEC_VERSION=1.1.3
dir_aec=./libaec

#
# download aec source
#

rm -rf $dir_aec
if [[ ! -f libaec-${AEC_VERSION}.tar.gz ]]; then
	curl -L https://github.com/MathisRosenhauer/libaec/releases/download/v${AEC_VERSION}/libaec-${AEC_VERSION}.tar.gz -o libaec-${AEC_VERSION}.tar.gz
fi
tar -xzf libaec-${AEC_VERSION}.tar.gz
mv libaec-${AEC_VERSION} $dir_aec
