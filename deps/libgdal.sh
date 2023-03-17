#!/bin/bash

set -eu

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$DIR/libgdal"

GDAL_VERSION=3.6.3
GDAL_VERSION_SUFFIX=
dir_gdal=./gdal
dir_formats_gyp=./gyp-formats
dir_gyp_templates=./gyp-templates

#
# download gdal source
#

rm -rf $dir_gdal
if [[ ! -f gdal-${GDAL_VERSION}${GDAL_VERSION_SUFFIX}.tar.gz ]]; then
	curl -L http://download.osgeo.org/gdal/${GDAL_VERSION}/gdal-${GDAL_VERSION}${GDAL_VERSION_SUFFIX}.tar.gz -o gdal-${GDAL_VERSION}${GDAL_VERSION_SUFFIX}.tar.gz
fi
tar -xzf gdal-${GDAL_VERSION}${GDAL_VERSION_SUFFIX}.tar.gz
mv gdal-${GDAL_VERSION} $dir_gdal
mv $dir_gdal/gcore/gdal_version.h.in $dir_gdal/gcore/gdal_version.h
#
# apply patches
#

for PATCH in patches/*.diff; do
  echo "Applying ${PATCH}"
  patch -p1 < $PATCH
done

#
# create format gyps
#

GDAL_FORMATS="gtiff hfa aigrid aaigrid ceos ceos2 iso8211 xpm
	sdts raw dted mem jdem envisat elas fit vrt usgsdem l1b
	nitf bmp pcidsk airsar rs2 ilwis rmf leveller sgi srtmhgt
	idrisi gsg ers jaxapalsar dimap gff cosar pds adrg
	coasp tsx terragen blx til r northwood saga xyz hf2
	kmlsuperoverlay ctg zmap ngsgeoid iris map
	jpeg openjpeg png mbtiles wms wmts hdf5 grib netcdf wcs
  zlib
	${OPT_GDAL_FORMATS:-}"

OGR_FORMATS="shape vrt avc geojson mem mitab kml gpx
	dxf csv edigeo geoconcept georss gml gmt gpsbabel
	idrisi dgn openfilegdb pds pgdump s57 sdts
	svg sxf ntf wasp sqlite gpkg mvt osm flatgeobuf carto"

mkdir -p $dir_formats_gyp

function wrap_array() {
	local serialized=
	local indent="$2"
	local var=""
	local tabs=""
	for (( c=1; c<=$2; c++)) ; do tabs="\t$tabs"; done
	while read a; do
		if [ -n "$a" ]; then
			serialized=$(wrap_string "$a")
			var="$var$serialized,\n$tabs"
		fi
	done <<< "$1"
	var=$(trim_comma "$var")
	printf "%s" "$var"
}
function wrap_string() {
	local var=$(trim $1)
	printf "%s" "\"$var\""
}
function trim() {
	local var=$@
	var="${var#"${var%%[![:space:]]*}"}"   # remove leading whitespace characters
	var="${var%"${var##*[![:space:]]}"}"   # remove trailing whitespace characters
	printf "%s" "$var"
}
function trim_comma() {
	local var=$@
	var=`echo -e "$var" | perl -p -000 -e 's/[\s\n\,]+$//g'`
	printf "%s" "$var"
}

format_list_gyps=""
format_list_defs=""
format_gyp_template=`cat ${dir_gyp_templates}/libgdal_format.gyp`

function generate_formats() {
	local formats="$1"
	local directory="$2"
	local prefix="${3:-}"

	for fmt in $formats; do
		file_gyp="${dir_formats_gyp}/${prefix}${fmt}.gyp"
		target_name="libgdal_${prefix}${fmt}_frmt"
		format_target_name=$(wrap_string "$target_name")

		dir_format="${directory}/${fmt}"
		relative_dir_format=`find ${dir_format} -type d | awk '{print "." $0}'`
		files_src_all=`find ${dir_format} | egrep '\.(c|cpp)$' | awk '{print "." $0}'`
		files_src=''
		# check to see which .c/.cpp files are mentioned in the makefile
		makefiles=`find ${dir_format} -name CMakeLists.txt`
		for file in $files_src_all; do
			base=`basename $file | cut -f 1 -d '.'`
			if grep -q ${base}.o ${makefiles}; then
				files_src="${files_src}"$'\n'"${file}"
			fi
		done
		# check for additional include directories
		add_includes=`cat ${dir_format}/CMakeLists.txt | grep -oE -- '-I\S+' | sed 's#-I#../gdal/frmts/#g'`
		relative_dir_format="${relative_dir_format}"$'\n'"${add_includes}"

		gyp_template="${format_gyp_template}"

		format_sources=$(wrap_array "$files_src" 4)
		format_include_dirs=$(wrap_array "$relative_dir_format" 4)

		if [[ ! -f "$file_gyp" ]]; then
			echo "Defining target: $format_target_name (\"$file_gyp\")"
			gyp_template=${gyp_template//'$TARGET_NAME'/$format_target_name}
			gyp_template=${gyp_template//'$INCLUDE_DIRS'/$format_include_dirs}
			gyp_template=${gyp_template//'$SOURCES'/$format_sources}
			echo -e "$gyp_template" > $file_gyp
		else
			echo "Skipping: $format_target_name (\"$file_gyp\")"
		fi

		if [[ $directory == *ogrsf* ]]; then
			frmt_upper=`echo $fmt | tr '[:lower:]' '[:upper:]'`
			if [[ ${frmt_upper} == "MITAB" ]]; then
				format_list_defs="$format_list_defs"$'\n'"TAB_ENABLED=1"
			fi
			format_list_defs="$format_list_defs"$'\n'"${frmt_upper}_ENABLED=1"
		else
			format_list_defs="$format_list_defs"$'\n'"FRMT_$fmt=1"
		fi

		format_list_gyps="$format_list_gyps"$'\n'"$file_gyp:$target_name"
	done
}

mkdir -p gdal/gcore/gdal_version_full
cp gdal/gcore/gdal_version.h gdal/gcore/gdal_version_full/

generate_formats "$GDAL_FORMATS" "${dir_gdal}/frmts"
generate_formats "$OGR_FORMATS" "${dir_gdal}/ogr/ogrsf_frmts" "ogr_"

#
# create format list
#

format_list_plain=`echo -e "$GDAL_FORMATS" | perl -p -000 -e 's/\s+/ /g'`
format_list_gyps=$(wrap_array "$format_list_gyps" 3)
format_list_defs=$(wrap_array "$format_list_defs" 3)

formats_gypi=`cat ${dir_gyp_templates}/libgdal_formats.gypi`
formats_gypi=${formats_gypi//'$GDAL_FORMATS'/$format_list_plain}
formats_gypi=${formats_gypi//'$GDAL_FORMAT_GYPS'/$format_list_gyps}
formats_gypi=${formats_gypi//'$GDAL_FORMAT_DEFS'/$format_list_defs}

echo -e "$formats_gypi" > libgdal_formats.gypi
