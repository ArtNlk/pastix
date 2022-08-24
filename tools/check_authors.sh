#
#  @file check_authors.sh
#
#  @copyright 2016-2022 Bordeaux INP, CNRS (LaBRI UMR 5800), Inria,
#                       Univ. Bordeaux. All rights reserved.
#
#  @version 6.2.0
#  @author Mathieu Faverge
#  @date 2021-04-07
#
# This script check that basic informations is present and correct in
# headers of source files.
#
#!/usr/bin/env sh

list_cleanup()
{
    cfile=$1

    sed -i '/Not Committed Yet/d'                       $cfile
    sed -i 's/BRIDONNEAU Vincent/Vincent Bridonneau/'   $cfile
    sed -i 's/DELARUE Tony/Tony Delarue/'               $cfile
    sed -i 's/Grégoire Pichon/Gregoire Pichon/'         $cfile
    sed -i 's/KORKMAZ Esragul/Esragul Korkmaz/'         $cfile
    sed -i 's/KUHN Matthieu/Matthieu Kuhn/'             $cfile
    sed -i 's/MASLIAH Ian/Ian Masliah/'                 $cfile
    sed -i 's/POIREL Louis/Louis Poirel/'               $cfile
    sed -i 's/PRUVOST Florent/Florent Pruvost/'         $cfile
    sed -i 's/RAMET Pierre/Pierre Ramet/'               $cfile
    sed -i 's/^Grégoire$/Gregoire Pichon/'              $cfile
    sed -i 's/matias hastaran/Matias Hastaran/'         $cfile
    sed -i 's/Hastaran Matias/Matias Hastaran/'         $cfile
    sed -i 's/Mathias Hastaran/Matias Hastaran/'        $cfile
    sed -i 's/tdelarue/Tony Delarue/'                   $cfile
    sed -i 's/François Pellegrini/Francois Pellegrini/' $cfile

    cat $cfile | sort -u > ${cfile}.tmp
    mv ${cfile}.tmp $cfile
}

get_authors_list()
{
    file=$1
    if [ ! -f $file ]
    then
        return;
    fi

    rc=$( grep "@author" $file )
    if [ $? -ne 0 ]
    then
       return
    fi

    error=0
    output="---- $file ----"

    git blame $file | awk -F "[()]" '{ print $2 }' | sed -e 's/^\(.*\w\)\s*[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9] .*$/\1/' | sort -u >> /tmp/full_author_list.txt
}

check_authors_list()
{
    file=$1
    if [ ! -f $file ]
    then
        return;
    fi

    rc=$( grep "@author" $file )
    if [ $? -ne 0 ]
    then
       return
    fi

    error=0
    output="---- $file ----"

    git blame $file | awk -F "[()]" '{ print $2 }' | sed -e 's/^\(.*\w\)\s*[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9] .*$/\1/' | sort -u > /tmp/author_list.txt

    list_cleanup /tmp/author_list.txt

    while read -r author
    do
        grep "@author $author" $file
        rc=$?
        if [ $rc -ne 0 ]
        then
            error=1
            output="${output}\n$author is missing (Automatically added if possible)"

            sed -i "s/^\(.*\)@date/\1@author $author\n\1@date/" $file
        fi
    done < /tmp/author_list.txt

    # Check extra authors
    grep "@author" $file | sed 's/^.*@author //' > /tmp/author_list2.txt
    sed -i '/Mathieu Faverge/d' /tmp/author_list2.txt
    sed -i '/Pierre Ramet/d'    /tmp/author_list2.txt
    sed -i '/Tony Delarue/d'    /tmp/author_list2.txt
    sed -i '/Gregoire Pichon/d' /tmp/author_list2.txt
    sed -i '/Xavier Lacoste/d'  /tmp/author_list2.txt
    sed -i '/Pascal Henon/d'    /tmp/author_list2.txt

    sed -i '/Ahmad Abdelfattah/d'    /tmp/author_list2.txt
    sed -i '/Alfredo Buttari/d'      /tmp/author_list2.txt
    sed -i '/Andrea Piacentini/d'    /tmp/author_list2.txt
    sed -i '/Azzam Haidar/d'         /tmp/author_list2.txt
    sed -i '/David Goudin/d'         /tmp/author_list2.txt
    sed -i '/Dulceneia Becker/d'     /tmp/author_list2.txt
    sed -i '/François Pellegrini/d'  /tmp/author_list2.txt
    sed -i '/Jakub Kurzak/d'         /tmp/author_list2.txt
    sed -i '/Louis Poirel/d'         /tmp/author_list2.txt
    sed -i '/Mark Gates/d'           /tmp/author_list2.txt
    sed -i '/Pierre Lemarinier/d'    /tmp/author_list2.txt
    sed -i '/Piotr Luszczek/d'       /tmp/author_list2.txt
    sed -i '/Stan Tomov/d'           /tmp/author_list2.txt
    sed -i '/Claire Soyez-Martin/d'  /tmp/author_list2.txt

    while read -r author
    do
        rc=$( grep "$author" /tmp/author_list.txt )
        if [ $? -ne 0 ]
        then
            error=1
            output="${output}\n$author is an extra"
            sed -i "/@author $author/d" $file
        fi
    done < /tmp/author_list2.txt

    if [ $error -eq 1 ]
    then
        echo $output
    fi
}

files=$( git ls-files )

rm -f /tmp/full_author_list.txt
for i in $files
do
    get_authors_list $i
done

list_cleanup /tmp/full_author_list.txt
cat /tmp/full_author_list.txt

for i in $files
do
    if [ $i == "tools/check_authors.sh" ]
    then
        continue;
    fi
    if [ $i == "tools/check_header.sh" ]
    then
        continue;
    fi
    check_authors_list $i
done
