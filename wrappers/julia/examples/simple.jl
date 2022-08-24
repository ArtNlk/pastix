#!/usr/bin/env julia
#=
 @file simple.jl

 PaStiX simple example

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

if PaStiX.pastix_mpi_enabled
    using MPI
    MPI.Init()
end

#
# Two solutions to select the outpu file to pass to output functions
#
my_stdout = Libc.FILE( Libc.RawFD(1), "w" ) # Select the stdout or stderr through 1 or 2
#my_stdout = Ptr{Cvoid}(0)                   # Give a null pointer to use the default

A    = spm.spmatrix_t(zero)
Aptr = pointer_from_objref(A)

# Load a sparse matrix from HB driver

spm.spmReadDriver( spm.SpmDriverLaplacian, "10:10:10:2.:1.", Aptr )
spm.spmPrintInfo( Aptr, my_stdout )

# Scale A for low-rank: A / ||A||_f
norm = spm.spmNorm( spm.SpmFrobeniusNorm, Aptr )
spm.spmScalMatrix( 1. / norm, Aptr )

# Generate b and x0 vector such that A * x0 = b
nrhs = 10
n    = A.nexp
X    = zeros(Cdouble, (n, nrhs))
B    = zeros(Cdouble, (n, nrhs))
X0   = zeros(Cdouble, (n, nrhs))

spm.spmGenRHS( spm.SpmRhsRndX, nrhs, Aptr, X0, n, B, n )

# Initialize parameters to default values
iparm = zeros( PaStiX.Pastix_int_t, PaStiX.iparm_size )
dparm = zeros( Cdouble, PaStiX.dparm_size )
PaStiX.pastixInitParam( iparm, dparm )

iparm[PaStiX.iparm_scheduler] = PaStiX.schedsequential #-s 0

# Startup PaStiX
#pastix_data = Ref{Ptr{Cvoid}}()
pastix_data = zeros(Int64,1)
pastix_data_ptr = pointer_from_objref(pastix_data)

PaStiX.pastixInit( pastix_data_ptr, 0, iparm, dparm )

# Perform analyze
PaStiX.pastix_task_analyze( pastix_data, Aptr )

# Perform numerical factorization
PaStiX.pastix_task_numfact( pastix_data, Aptr )

# Perform solve
X = copy(B)
PaStiX.pastix_task_solve( pastix_data, nrhs, X, n )

# Refine the solution
PaStiX.pastix_task_refine( pastix_data, n, nrhs, B, n, X, n )

# Check solution
eps = dparm[PaStiX.dparm_epsilon_refinement]
spm.spmCheckAxb( eps, nrhs, Aptr, X0, n, B, n, X, n )

PaStiX.pastixFinalize( pastix_data_ptr )
spm.spmExit( Aptr )
