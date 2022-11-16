#!/bin/bash

export HOME=/target
SEP="\n=======================================================\n"
echo -e "${SEP}Preparing source tree${SEP}"

if [ -n "${CCACHE_DIR}" ]; then
  echo -e "${SEP}Checking for ccache"
  if which ccache; then
    if [ -z "${CC}" ]; then
      export CC="ccache gcc"
    fi
    if [ -z "${CXX}" ]; then
      export CXX="ccache g++"
    fi
    echo -e "Using ccache in ${CCACHE_DIR} CC=\"${CC}\" CXX=\"${CXX}\"${SEP}"
  else
    echo -e "Container has no ccache installed${SEP}"
  fi
else
  echo -e "${SEP}Host has no ccache installed${SEP}"
fi
rsync -lr /src/ /target/src --exclude=build --exclude=test/data --exclude node_modules
cd /target/src
rm -rf lib/binding package-lock.json
git checkout test/data
npm install --ignore-scripts

if [ "${GDAL}" == "shared" ]; then
  GDAL_OPT="--shared_gdal"
  GDAL_NAME="system-installed"
else
  GDAL_OPT=""
  GDAL_NAME="bundled"
fi

case ${1} in
  DEV)
    echo -e "${SEP}Testing current version against ${GDAL_NAME} GDAL${SEP}"
    echo npx @mapbox/node-pre-gyp configure ${GDAL_OPT}
    npx @mapbox/node-pre-gyp configure ${GDAL_OPT}
    npx @mapbox/node-pre-gyp build -j max
    npm test
    R=$?
    ;;

  RELEASE)
    echo -e "${SEP}Testing pre-built binary${SEP}"
    npm install --ignore-scripts
    npx @mapbox/node-pre-gyp install
    npx yatag
    npm test
    R=$?
    ;;

  PUBLISH)
    echo -e "${SEP}Publishing pre-built binary${SEP}"
    npx @mapbox/node-pre-gyp configure
    npx @mapbox/node-pre-gyp build -j max
    npm test
    npx @mapbox/node-pre-gyp package
    npx node-pre-gyp-github publish
    R=$?
    ;;

  ASAN)
    echo -e "${SEP}Testing w/ASAN current version against ${GDAL_NAME} GDAL${SEP}"
    echo npx @mapbox/node-pre-gyp configure ${GDAL_OPT} --enable-asan --debug
    npx @mapbox/node-pre-gyp configure ${GDAL_OPT} --enable-asan --debug
    npx @mapbox/node-pre-gyp build -j max
    LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libasan.so.5.0.0 npm test 2> asan.output
    ! egrep -q "node_gdal|GDAL" asan.output
    R=$?
    if [ "$R" -ne 0 ]; then
      echo ${SEP}${SEP}Memory leaks detected${SEP}${SEP}
      cat asan.output
    fi
    ;;
esac

# Launch docker -it ... /bin/bash to get a shell after running the test
if [ ! -z "${2}" ]; then
  shift
  exec $@
fi

exit $R
