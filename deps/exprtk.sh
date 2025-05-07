#!/bin/bash

set -eu

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$DIR/exprtk"

dir_exprtk=./exprtk

#
# download exprtk headers
#

rm -rf $dir_exprtk

curl -LO -fsS https://github.com/ArashPartow/exprtk/raw/refs/heads/release/exprtk.hpp -o $dir_exprtk/exprtk.hpp
