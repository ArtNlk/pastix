#
# @file pastix_completion.bash
#
# This file is generated automatically. If you want to modify it, modify
# ${PASTIX_HOME}/tools/gen_param/pastix_[iparm/dparm/enums].py and run
# ${PASTIX_HOME}/tools/gen_param/gen_parm_files.py ${PASTIX_HOME}.
#
# @copyright 2021-2021 Bordeaux INP, CNRS (LaBRI UMR 5800), Inria,
#                      Univ. Bordeaux. All rights reserved.
#
# @version 6.2.1
# @author Mathieu Faverge
# @author Tony Delarue
# @date 2021-10-13
#
#!/usr/bin/env bash

BINARY_DIR=$1

_pastix_completion()
{
    local LONG_OPTIONS=("--rsa --hb --ijv --mm --spm --lap --xlap --graph \
                         --threads --gpus --sched --ord --fact --check --iparm --dparm --verbose --help")
    local SHORT_OPTIONS=("-0 -1 -2 -3 -4 -9 -x -G \
                          -t -g -s -o -f -c -i -d -v -h")

    local i cur=${COMP_WORDS[COMP_CWORD]}

    COMPREPLY=($(compgen -W "${LONG_OPTIONS[@]} ${SHORT_OPTIONS[@]}" -- $cur))

    prev=${COMP_WORDS[COMP_CWORD-1]}
    case $prev in
        -i|--iparm)
            COMPREPLY=($(compgen -W "iparm_verbose \
                                     iparm_io_strategy \
                                     iparm_produce_stats \
                                     iparm_trace \
                                     iparm_mc64 \
                                     iparm_ordering \
                                     iparm_ordering_default \
                                     iparm_scotch_mt \
                                     iparm_scotch_switch_level \
                                     iparm_scotch_cmin \
                                     iparm_scotch_cmax \
                                     iparm_scotch_frat \
                                     iparm_metis_ctype \
                                     iparm_metis_rtype \
                                     iparm_metis_no2hop \
                                     iparm_metis_nseps \
                                     iparm_metis_niter \
                                     iparm_metis_ufactor \
                                     iparm_metis_compress \
                                     iparm_metis_ccorder \
                                     iparm_metis_pfactor \
                                     iparm_metis_seed \
                                     iparm_metis_dbglvl \
                                     iparm_amalgamation_lvlblas \
                                     iparm_amalgamation_lvlcblk \
                                     iparm_reordering_split \
                                     iparm_reordering_stop \
                                     iparm_splitting_strategy \
                                     iparm_splitting_levels_projections \
                                     iparm_splitting_levels_kway \
                                     iparm_splitting_projections_depth \
                                     iparm_splitting_projections_distance \
                                     iparm_splitting_projections_width \
                                     iparm_min_blocksize \
                                     iparm_max_blocksize \
                                     iparm_tasks2d_level \
                                     iparm_tasks2d_width \
                                     iparm_allcand \
                                     iparm_incomplete \
                                     iparm_level_of_fill \
                                     iparm_factorization \
                                     iparm_free_cscuser \
                                     iparm_schur_fact_mode \
                                     iparm_transpose_solve \
                                     iparm_schur_solv_mode \
                                     iparm_applyperm_ws \
                                     iparm_refinement \
                                     iparm_itermax \
                                     iparm_gmres_im \
                                     iparm_scheduler \
                                     iparm_thread_nbr \
                                     iparm_autosplit_comm \
                                     iparm_gpu_nbr \
                                     iparm_gpu_memory_percentage \
                                     iparm_gpu_memory_block_size \
                                     iparm_compress_min_width \
                                     iparm_compress_min_height \
                                     iparm_compress_when \
                                     iparm_compress_method \
                                     iparm_compress_ortho \
                                     iparm_compress_reltol \
                                     iparm_compress_preselect \
                                     iparm_compress_iluk" -- $cur))
            ;;

        -d|--dparm)
            COMPREPLY=($(compgen -W "dparm_epsilon_refinement \
                                     dparm_epsilon_magn_ctrl \
                                     dparm_compress_tolerance \
                                     dparm_compress_min_ratio" -- $cur))
            ;;

        iparm_verbose)
            COMPREPLY=($(compgen -W "pastixverbosenot \
                                     pastixverboseno \
                                     pastixverboseyes" -- $cur))
            ;;
        iparm_io_strategy)
            COMPREPLY=($(compgen -W "pastixiono \
                                     pastixioload \
                                     pastixiosave \
                                     pastixioloadgraph \
                                     pastixiosavegraph \
                                     pastixioloadcsc \
                                     pastixiosavecsc" -- $cur))
            ;;
        iparm_trace)
            COMPREPLY=($(compgen -W "pastixtracenumfact \
                                     pastixtracesolve" -- $cur))
            ;;
        iparm_ordering)
            COMPREPLY=($(compgen -W "pastixorderscotch \
                                     pastixordermetis \
                                     pastixorderpersonal \
                                     pastixorderptscotch \
                                     pastixorderparmetis" -- $cur))
            ;;
        -o|--ord)
            COMPREPLY=($(compgen -W "scotch \
                                     metis \
                                     personal \
                                     ptscotch \
                                     parmetis" -- $cur))
            ;;
        iparm_splitting_strategy)
            COMPREPLY=($(compgen -W "pastixsplitnot \
                                     pastixsplitkway \
                                     pastixsplitkwayprojections" -- $cur))
            ;;
        iparm_factorization)
            COMPREPLY=($(compgen -W "pastixfactpotrf \
                                     pastixfactsytrf \
                                     pastixfactgetrf \
                                     pastixfactpxtrf \
                                     pastixfacthetrf \
                                     pastixfactllh \
                                     pastixfactldlt \
                                     pastixfactlu \
                                     pastixfactllt \
                                     pastixfactldlh" -- $cur))
            ;;
        iparm_schur_fact_mode)
            COMPREPLY=($(compgen -W "pastixfactmodelocal \
                                     pastixfactmodeschur \
                                     pastixfactmodeboth" -- $cur))
            ;;
        iparm_transpose_solve)
            COMPREPLY=($(compgen -W "pastixnotrans \
                                     pastixtrans \
                                     pastixconjtrans" -- $cur))
            ;;
        iparm_schur_solv_mode)
            COMPREPLY=($(compgen -W "pastixsolvmodelocal \
                                     pastixsolvmodeinterface \
                                     pastixsolvmodeschur" -- $cur))
            ;;
        iparm_refinement)
            COMPREPLY=($(compgen -W "pastixrefinegmres \
                                     pastixrefinecg \
                                     pastixrefinesr \
                                     pastixrefinebicgstab" -- $cur))
            ;;
        iparm_scheduler)
            COMPREPLY=($(compgen -W "pastixschedsequential \
                                     pastixschedstatic \
                                     pastixschedparsec \
                                     pastixschedstarpu \
                                     pastixscheddynamic" -- $cur))
            ;;
        iparm_compress_when)
            COMPREPLY=($(compgen -W "pastixcompressnever \
                                     pastixcompresswhenbegin \
                                     pastixcompresswhenend \
                                     pastixcompresswhenduring" -- $cur))
            ;;
        iparm_compress_method)
            COMPREPLY=($(compgen -W "pastixcompressmethodsvd \
                                     pastixcompressmethodpqrcp \
                                     pastixcompressmethodrqrcp \
                                     pastixcompressmethodtqrcp \
                                     pastixcompressmethodrqrrt \
                                     pastixcompressmethodnbr" -- $cur))
            ;;
        iparm_compress_ortho)
            COMPREPLY=($(compgen -W "pastixcompressorthocgs \
                                     pastixcompressorthoqr \
                                     pastixcompressorthopartialqr" -- $cur))
            ;;
        iparm_start_task)
            COMPREPLY=($(compgen -W "pastixtaskinit \
                                     pastixtaskordering \
                                     pastixtasksymbfact \
                                     pastixtaskanalyze \
                                     pastixtasknumfact \
                                     pastixtasksolve \
                                     pastixtaskrefine \
                                     pastixtaskclean" -- $cur))
            ;;
        iparm_end_task)
            COMPREPLY=($(compgen -W "pastixtaskinit \
                                     pastixtaskordering \
                                     pastixtasksymbfact \
                                     pastixtaskanalyze \
                                     pastixtasknumfact \
                                     pastixtasksolve \
                                     pastixtaskrefine \
                                     pastixtaskclean" -- $cur))
            ;;
        iparm_float)
            COMPREPLY=($(compgen -W "pastixpattern \
                                     pastixfloat \
                                     pastixdouble \
                                     pastixcomplex32 \
                                     pastixcomplex64" -- $cur))
            ;;

        -v|--verbose)
            COMPREPLY=($(compgen -W "0 1 2 3" -- $cur))
            ;;
        -f|--fact)
            COMPREPLY=($(compgen -W "0 1 2 3 4" -- $cur))
            ;;
        -s|--sched)
            COMPREPLY=($(compgen -W "0 1 2 3 4" -- $cur))
            ;;
        -c|--check)
            COMPREPLY=($(compgen -W "0 1 2 3 4 5 6" -- $cur))
            ;;

        # For remaining options, we don't suggest anything
        -0|-1|-2|-3|-4|-9|-x|-G|-t|-g|-h| \
        --rsa|--hb|--ijv|--mm|--spm|--lap| \
        --xlap|--graph|--threads|--gpus|--help| \
        iparm_*|dparm_*)
            COMPREPLY=($(compgen -W "" -- $cur))
            ;;

    esac
}

# Add the dynamic completion to the executable
for exec in $(find $BINARY_DIR -maxdepth 1 -executable -type f)
do
    e=$(basename $exec)
    complete -F _pastix_completion $e
done
