def find_enums_name( name, enums ):
    for enum in enums :
        if name == enum["name"] :
            return enum
    return -1

def genIparmCompletion( iparms ) :
    result = "            COMPREPLY=($(compgen -W \""

    isize  = len("            COMPREPLY=($(compgen -W \"")
    indent = ""
    for group in iparms:
        for iparm in group["subgroup"]:
            if iparm['access'] != 'IN' :
                continue
            result += indent + iparm["name"] + " \\\n"
            indent  = " " * isize

    result = result[:len(result) -3] + '''" -- $cur))
            ;;
'''
    return result

def genDparmCompletion( dparms ) :
    result = "            COMPREPLY=($(compgen -W \""

    isize  = len("            COMPREPLY=($(compgen -W \"")
    indent = ""
    for dparm in dparms:
        if dparm['access'] != 'IN' :
            continue
        result += indent + dparm["name"] + " \\\n"
        indent  = " " * isize

    result = result[:len(result) -3] + '''" -- $cur))
            ;;
'''
    return result

def genEnumsCompletion( iparms, enums ) :
    result = ""
    isize = len("            COMPREPLY=($(compgen -W \"")
    for group in iparms:
        for iparm in group["subgroup"]:

            if iparm['access'] != 'IN' :
                continue
            if 'enum' not in iparm :
                continue

            enumname  = iparm['enum']
            result += "        " + iparm["name"] + ''')
            COMPREPLY=($(compgen -W "'''
            values =  find_enums_name( enumname, enums )['values']
            indent = ""
            for value in values :
                name = value["name"]
                result += indent + name.lower() + " \\\n"
                indent  = " " * isize
            result = result[:len(result) -3] + '''" -- $cur))
            ;;
'''
            # -o or --ord
            if iparm["name"] == "iparm_ordering" :
                enumsize = len("pastixorder")
                result += '''        -o|--ord)
            COMPREPLY=($(compgen -W "'''
                indent = ""
                for value in values :
                    name = value["name"]
                    result += indent + name.lower()[enumsize:] + " \\\n"
                    indent  = " " * isize
                result = result[:len(result) -3] + '''" -- $cur))
            ;;
'''
    result += '''
'''
    return result

def genShortCompletion():
    result = '''        -v|--verbose)
            COMPREPLY=($(compgen -W "0 1 2" -- $cur))
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
'''
    return result

def genCompleteCommand():
    result = '''# Add the dynamic completion to the executable
# Make sure the executable is in the PATH variable
cd $BINARY_DIR
for e in $(find . -maxdepth 1 -executable -type f | cut -d "/" -f 2)
do
    complete -F _pastix_completion $e
done
cd -
'''
    return result

def genCompletion( iparms, dparms, enums ) :
    isize = len("    local LONG_OPTIONS=(\"") * " "
    long_opts  = '''--rsa --hb --ijv --mm --spm --lap --xlap --graph \\
'''+ isize +'''--threads --gpus --sched --ord --fact --check --iparm --dparm --verbose --help'''
    short_opts = '''-0 -1 -2 -3 -4 -9 -x -G \\
'''+ isize +''' -t -g -s -o -f -c -i -d -v -h'''

    result = '''#!/usr/bin/env bash

BINARY_DIR=$1

_pastix_completion()
{
    local LONG_OPTIONS=("'''+ long_opts +'''")
    local SHORT_OPTIONS=("'''+ short_opts +'''")

    local i cur=${COMP_WORDS[COMP_CWORD]}

    COMPREPLY=($(compgen -W "${LONG_OPTIONS[@]} ${SHORT_OPTIONS[@]}" -- $cur))

    prev=${COMP_WORDS[COMP_CWORD-1]}
    case $prev in
        -i|--iparm)
''' + genIparmCompletion(iparms) + '''        -d|--dparm)
''' + genDparmCompletion(dparms) + genEnumsCompletion( iparms, enums ) + genShortCompletion() +'''        *)
            # For remaining options with one argument, we don't suggest anything
            ;;
    esac
}

''' + genCompleteCommand()
    return result
