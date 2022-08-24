"""
 @file gen_api_header.py

 Declaration of the api.h file to be generated automatically.

 @copyright 2021-2022 Bordeaux INP, CNRS (LaBRI UMR 5800), Inria,
                      Univ. Bordeaux. All rights reserved.

 @version 6.2.0
 @author Tony Delarue
 @date 2021-04-07

"""
import time

const_str = ''' * This file is generated automatically. If you want to modify it, modify
 * ${PASTIX_HOME}/tools/gen_param/pastix_[iparm/dparm/enums].py and run
 * ${PASTIX_HOME}/tools/gen_param/gen_parm_files.py ${PASTIX_HOME}.
 *
 * @copyright 2004-'''+ time.strftime( "%Y" ) +''' Bordeaux INP, CNRS (LaBRI UMR 5800), Inria,
 *                      Univ. Bordeaux. All rights reserved.
 *
 * @version 6.2.1
 * @author Xavier Lacoste
 * @author Pierre Ramet
 * @author Mathieu Faverge
 * @author Esragul Korkmaz
 * @author Gregoire Pichon
 * @author Tony Delarue
 * @date '''+ time.strftime( "%Y-%m-%d" )

pastix_enums_begin = '''/**
 *
 * @file pastix/api.h
 *
 * PaStiX API enums parameters.
 *
''' + const_str + '''
 *
 * @addtogroup pastix_api
 * @{
 *
 **/
#ifndef _pastix_api_h_
#define _pastix_api_h_

BEGIN_C_DECLS

'''

close_bracket = '''/**
 * @}
 */
'''

pastix_enums_end = '''
END_C_DECLS

#endif /* _pastix_api_h_ */

''' + close_bracket

blas_str = '''/**
 *
 * @name Constants compatible with CBLAS & LAPACK & PLASMA
 * @{
 *    The naming and numbering of the following constants is consistent with:
 *
 *       - CBLAS from Netlib (http://www.netlib.org/blas/blast-forum/cblas.tgz)
 *       - C Interface to LAPACK from Netlib (http://www.netlib.org/lapack/lapwrapc/)
 *       - Plasma (http://icl.cs.utk.edu/plasma/index.html)
 *
 */
'''

def get_param_max_str_size( params ) :
    """
    Get the max size of all names, brief description and default value
    (Output purpose).

    @in  params : The params dictionnary
    @out max name, brief and default value size
    """
    maxname   = max( list( map(lambda x : len(x['name']),    params) ) )
    maxbrief  = max( list( map(lambda x : len(x['brief']),   params) ) )
    maxdefval = max( list( map(lambda x : len(x['default']), params) ) )

    return maxname, maxbrief, maxdefval

def gen_param( params, maxName, maxBrief, maxDefVal ) :
    """
    Generate a string that corresponds to the declaration of the param

    @in  params : The params dictionnary
    @in  max*   : Size of the max string for this field
    @out The declaration string for the parameters in params
    """
    result = ""
    for param in params :
        # Get the fields
        name    = param['name'].upper() + ","
        brief   = param['brief']
        default = param['default']
        inout   = param['access']

        # Align them correctly
        name    += " " * ( maxName   - len(name) + 1 )
        brief   += " " * ( maxBrief  - len(brief)    )
        default += " " * ( maxDefVal - len(default) + 1  )
        inout   += " " * ( 3         - len(inout)    )

        # Add them in the result
        result += "    " + name +" /**< "+ brief +" Default: "+ default + inout +" */\n"
    return result

#
# IPARM/DPARM declaration
#
decl_str_begin = '''/**
 * @brief {} parameters
 */
typedef enum pastix_{}_e {}
'''
decl_str_end = '''    {}_SIZE
{} pastix_{}_t;

'''
def genIparmDeclaration( iparms ) :
    """
    Generate a string that corresponds to the declaration of the pastix_iparm_t

    @in  params : The iparm groups array
    @out The declaration string for pastix_iparm_t
    """
    # Get the global max size
    maxName   = 0
    maxBrief  = 0
    maxDefVal = 0
    for group in iparms :
        tmpname, tmpbrief, tmpdefval = get_param_max_str_size( group["subgroup"] )
        if tmpname > maxName :
            maxName = tmpname
        if tmpbrief > maxBrief :
            maxBrief = tmpbrief
        if tmpdefval > maxDefVal :
            maxDefVal = tmpdefval

    result = decl_str_begin.format( "Integer", "iparm", "{" )
    for group in iparms :
        if group["brief"] != "None" :
            result += "    /* "+ group["brief"] +" */\n"
        result += gen_param( group["subgroup"], maxName, maxBrief, maxDefVal )
        result += "\n"
    result += decl_str_end.format( "IPARM", "}", "iparm" )

    return result

def genDparmDeclaration( dparms ) :
    """
    Generate a string that corresponds to the declaration of the pastix_dparm_t

    @in  dparms : The dparm array
    @out The declaration string for pastix_dparm_t
    """
    maxName, maxBrief, maxDefVal = get_param_max_str_size( dparms )

    result  = decl_str_begin.format( "Float", "dparm", "{" )
    result += gen_param( dparms, maxName, maxBrief, maxDefVal )
    result += decl_str_end.format( "DPARM", "}", "dparm" )

    return result

def gen_enum_documentation( enum ):
    """
    Generate a string that corresponds to the documentation before
    the declaration of enum

    @in  enum : The current enum dictionnary to declare
    @out the enum documentation string for the pastix_[enum]_t
    """
    result = "/**\n"\
             " * @brief " + enum['doc']['brief'] + "\n"

    if 'details' in enum['doc'] :
        details = enum['doc']['details'].split("\n")
        details = details[:len(details) - 1]

        for line in details:
            if line == '' :
                result += " *\n"
                continue
            result += " * " + line + "\n"
    result += " */\n"

    return result

def gen_enum_declaration( enum ) :
    """
    Generate a string that corresponds to the declaration of an enum

    @in  enum : The current enum dictionnary to declare
    @out the enum declaration string for the pastix_[enum]_t
    """
    name = enum['name']
    # Write documentation
    result = gen_enum_documentation( enum )

    # End comment, begin enum declaration
    result += "typedef enum pastix_"+ name + "_e {\n"\

    values = enum['values']
    last   = len(values) - 1

    nameMaxSize  = max(list( map(lambda x : len(x['name']), values) ))
    briefMaxSize = 0
    valueMaxSize = 0
    if 'brief' in values[0] :
        briefMaxSize = max(list( map(lambda x : len(x['brief']), values) ))
    if 'value' in values[0] :
        valueMaxSize = max(list( map(lambda x : len(str(x['value'])), values) ))

    for value in values :
        # Name
        result += "    " + value['name']

        # If a value is given, put it in the file
        if 'value' in value :
            result += " " * (nameMaxSize - len(value['name']) )
            result += " = "+ str(value['value'])

        if value != enum['values'][ last ] :
            result += ","
        else :
            if 'brief' in value :
                result += " "

        # If a brief comment is given, put it in the file
        if 'brief' in value :
            if 'value' in value :
                result += " " * (valueMaxSize - len(str(value['value'])) ) + " /**< "
            else :
                result += " " * (nameMaxSize  - len(    value['name'])   ) + " /**< "
            result += value['brief'] + " " * (briefMaxSize - len(value['brief']) ) + " */"
        result += "\n"

    # End of the enum declaration
    result += "} pastix_" + name + "_t;\n\n"
    return result

def gen_coeftype( enum ) :
    """
    pastix_coeftype_t is not an enum but a define, we have to generate it differently

    @in  enum : The enum containing pastix_coeftype_t.
    @out The string that declare the pastix_coeftype_t in api.h
    """
    result = gen_enum_documentation( enum )
    result += "#define pastix_coeftype_t spm_coeftype_t\n"

    maxsize = max( list( map( lambda x : len(x["name"]) + 1, enum["values"] ) ) )

    for value in enum["values"] :
        result += "#define "+ value["name"] + ( maxsize - len(value["name"]) ) * " " + str(value['value']) +"\n"
    result += close_bracket + "\n"
    return result

def genEnumsDeclaration( enums ) :
    """
    Generate a string that corresponds to the declaration of all the enums of PaStix,
    right after the declaration of pastix_iparm_t and pastix_dparm_t.

    @in  enums : The enums array.
    @out The string that declare the enums in api.h
    """
    headerFile  = ""

    for enum in enums :
        name = enum["name"]
        if name == "coeftype" :
            headerFile += gen_coeftype( enum )
            continue

        # Begin BLAS-like enums
        if name == "layout" :
            headerFile += blas_str + "\n"

        headerFile += gen_enum_declaration( enum )

    # Close BLAS-like enums bracket
    headerFile += close_bracket
    return headerFile

def genApiHeaderFile( iparms, dparms, enums ) :
    """
    Generate a string that corresponds to the api.h file

    @in  iparms : The iparm groups array.
    @in  dparms : The dparms array.
    @in  enums  : The enums array.
    @out The string that declare the enums in api.h
    """
    apiHeader  = pastix_enums_begin
    apiHeader += genIparmDeclaration(iparms)
    apiHeader += genDparmDeclaration(dparms)
    apiHeader += genEnumsDeclaration(enums)
    apiHeader += pastix_enums_end

    return apiHeader
