#
#  @file update_release.sh
#
#  @copyright 2016-2022 Bordeaux INP, CNRS (LaBRI UMR 5800), Inria,
#                       Univ. Bordeaux. All rights reserved.
#
#  @version 6.2.2
#  @author Mathieu Faverge
#  @date 2021-07-02
#
#!/usr/bin/env sh

tag=v6.2.1
majorversion=6
minorversion=2
microversion=2

version="$majorversion.$minorversion.$microversion"

#for i in $( git diff v6.0.2 --name-only ); do if [ -f $i ]; then sed -i 's/@version [0-9].[0-9].[0-9]/@version 6.2.2/' $i; fi; done
if [ ! -z "$tag" ]
then
    fileslist=$( git diff $tag --name-only )
else
    fileslist=$( git ls-files )
fi

fulllist=$( git ls-files )

#
# Steps to update header information before doing the release
#

#
# 1) Check header files with check_headers.sh
#
#./tools/check_headers.sh

#
# 2) Check that the fortran/python wrappers have been updated (see gen_wrappers.py)
#
#./tools/gen_wrappers.py

#
# 3) First update the date of the files with the following lines
#
for f in $fileslist
do
    if [ ! -f $f ]
    then
        continue;
    fi

    date=$( git log -1 --format=%cd --date=short $f )
    echo $date $f
    sed -i "s/date [-0-9]*\$/date $date/" $f
done

#
# 4) Update the release number
#
for f in $fileslist
do
    if [ ! -f $f ]
    then
        continue;
    fi

    sed -i "s/@version [0-9]\.[0-9]\.[0-9][0-9]*/@version $version/" $f
done

#
# 5) Update manually the version number in CMakeLists.txt
#
sed -i "s/set( PASTIX_VERSION_MAJOR [0-9] )/set( PASTIX_VERSION_MAJOR $majorversion )/" CMakeLists.txt
sed -i "s/set( PASTIX_VERSION_MINOR [0-9] )/set( PASTIX_VERSION_MINOR $minorversion )/" CMakeLists.txt
sed -i "s/set( PASTIX_VERSION_MICRO [0-9][0-9]* )/set( PASTIX_VERSION_MICRO $microversion )/" CMakeLists.txt

#
# 6) If necessary, update the copyright information
#
for f in $fulllist
do
    if [ ! -f $f ]
    then
        continue;
    fi

    year=$( date +%Y )
    sed -i "s/copyright \([0-9]*\)-[0-9]* Bordeaux/copyright \1-$year Bordeaux/" $f
done

#
# 7) Update homebrew formula (only after release)
#
