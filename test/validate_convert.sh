#!/bin/sh

fltname=$1

rm -f *.sort
for i in `ls -1 convert*.dat`;
do
    sort $i > ${i}.sort
done

for mtxtype in General Symmetric Hermitian
do
    basefile=convert_b0_${mtxtype}_CSC_cycle1.dat.sort

    if [ -f $basefile ]
    then
        echo "-- $mtxtype -- "

        for baseval in 0 1
        do
            for fmttype in CSC CSR IJV
            do
                for cycle in cycle1 cycle2
                do
                    echo -n "---- CSC VS $fmttype $cycle: "
                    diff $basefile convert_b${baseval}_${mtxtype}_${fmttype}_${cycle}.dat.sort | wc -l
                done
            done
            echo -n "---- CSC VS CSC end: "
            diff $basefile convert_b${baseval}_${mtxtype}_CSC_end.dat.sort | wc -l
        done
fi
done

