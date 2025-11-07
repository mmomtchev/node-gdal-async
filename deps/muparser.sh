#!/bin/bash

set -eu

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$DIR/muparser"

MUPARSER_VERSION=2.3.5
dir_muparser=./muparser

#
# download muparser source
#

rm -rf $dir_muparser
if [[ ! -f muparser-${MUPARSER_VERSION}.tar.gz ]]; then
  curl -L https://github.com/beltoforion/muparser/archive/refs/tags/v${MUPARSER_VERSION}.tar.gz -o muparser-${MUPARSER_VERSION}.tar.gz
fi
tar -xzf muparser-${MUPARSER_VERSION}.tar.gz
mv muparser-${MUPARSER_VERSION} muparser
