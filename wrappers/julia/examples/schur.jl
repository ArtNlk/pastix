#!/usr/bin/env julia
#=
 @file schur.jl

 PaStiX Schur example

 @copyright 2019-2022 Bordeaux INP, CNRS (LaBRI UMR 5800), Inria,
                      Univ. Bordeaux. All rights reserved.

 @version 6.2.0
 @author Mathieu Faverge
 @author Selmane Lebdaoui
 @date 2020-07-16
=#

using Pkg
Pkg.activate("../../spm/wrappers/julia/spm")
Pkg.activate("PaStiX")
using CBinding
using PaStiX
using spm
using LinearAlgebra

if PaStiX.pastix_mpi_enabled
    using MPI
    MPI.Init()
end

#
# Two solutions to select the outpu file to pass to output functions
#
my_stdout = Libc.FILE( Libc.RawFD(1), "w" ) # Select the stdout or stderr through 1 or 2
#my_stdout = Ptr{Cvoid}(0)                   # Give a null pointer to use the default

# Load a sparse matrix from RSA driver
A    = spm.spmatrix_t(zero)
Aptr = pointer_from_objref(A)

spm.spmReadDriver( spm.SpmDriverLaplacian, "10:10:10:2.:1.", Aptr )

# Generate b and x0 vector such that A * x0 = b
nrhs = 1
n    = A.gN # Let's consider a matrix with single dof such that n == nexp, and gN == gNexp
X    = zeros(Cdouble, (n, nrhs))
B    = zeros(Cdouble, (n, nrhs))
X0   = zeros(Cdouble, (n, nrhs))

spm.spmGenRHS( spm.SpmRhsRndX, nrhs, Aptr, X0, n, B, n )
X = copy(B)

spm.spmPrintInfo( Aptr, my_stdout )

# Initialize parameters to default values
iparm = zeros( PaStiX.Pastix_int_t, PaStiX.iparm_size )
dparm = zeros( Cdouble, PaStiX.dparm_size )
PaStiX.pastixInitParam( iparm, dparm )

# Startup PaStiX
pastix_data = zeros(Int64,1)
pastix_data_ptr = pointer_from_objref(pastix_data)
PaStiX.pastixInit( pastix_data_ptr, Cint(0), iparm, dparm )

factotype = PaStiX.factlu
iparm[PaStiX.iparm_factorization]   = factotype
iparm[PaStiX.iparm_scheduler]       = PaStiX.schedsequential
iparm[PaStiX.iparm_schur_solv_mode] = PaStiX.solvmodeinterface

# Initialize the Schur list as the first third of the elements
Abase = spm.spmFindBase(Aptr)
nschur = min( Int(floor( n / 3 )), 5000 )

schurlist = PaStiX.Pastix_int_t[ (i + Abase - 1) for i in 1:nschur ] # -1 cause of (1:nschur) = [[ 1, nschur ]]
PaStiX.pastixSetSchurUnknownList( pastix_data, nschur, schurlist )

# Perform analyze
PaStiX.pastix_task_analyze( pastix_data, Aptr )

# Perform numerical factorization
PaStiX.pastix_task_numfact( pastix_data, Aptr )

# Get the Schur complement
S  = zeros( Float64, nschur, nschur ) #column major !! by default in julia
rc = PaStiX.pastixGetSchur( pastix_data, S, nschur )

@assert(rc == 0)

# Store both sides for linalg
if factotype != PaStiX.factlu
    S += LinearAlgebra.transpose(LinearAlgebra.tril(S,-1))
end

# 1- Apply P to b
PaStiX.pastix_subtask_applyorder( pastix_data, A.flttype, spm.SpmDirForward,
                                  n, nrhs, X, n )

if factotype == PaStiX.factlu
    # 2- Forward solve on the non Schur complement part of the system
    PaStiX.pastix_subtask_trsm( pastix_data, A.flttype, PaStiX.left, PaStiX.lower,
                                PaStiX.notrans, PaStiX.unit, nrhs, X, n )

    # 3- Solve the Schur complement part
    X[n-nschur+1:n] = S \ X[n-nschur+1:n]

    # 4- Backward solve on the non Schur complement part of the system
    PaStiX.pastix_subtask_trsm( pastix_data, A.flttype, PaStiX.left, PaStiX.upper,
                                PaStiX.notrans,  PaStiX.nonunit, nrhs, X, n )
else
    # 2- Forward solve on the non Schur complement part of the system
    PaStiX.pastix_subtask_trsm( pastix_data, A.flttype, PaStiX.left, PaStiX.lower,
                                PaStiX.notrans, PaStiX.nonunit, nrhs, X, n )

    # 3- Solve the Schur complement part
    Xs = view( X, n-nschur+1:n, nrhs ) # Use a view to avoid the copy
    LinearAlgebra.LAPACK.posv!( 'L', S, Xs )

    # 4- Backward solve on the non Schur complement part of the system
    PaStiX.pastix_subtask_trsm( pastix_data, A.flttype, PaStiX.left, PaStiX.lower,
                                PaStiX.conjtrans,  PaStiX.nonunit, nrhs, X, n )

end

# 5- Apply P^t to x
PaStiX.pastix_subtask_applyorder( pastix_data, A.flttype, spm.SpmDirBackward, n, nrhs, X, n )

# Check solution
eps = dparm[PaStiX.dparm_epsilon_refinement]
spm.spmCheckAxb( 1.0e-15, nrhs, Aptr, X0, n, B, n, X, n )

PaStiX.pastixFinalize( pastix_data_ptr )
spm.spmExit( Aptr )

