#!/usr/bin/env python3
"""
 @file simple_obj.py

 PaStiX simple python example with the solver object

 @copyright 2017-2022 Bordeaux INP, CNRS (LaBRI UMR 5800), Inria,
                      Univ. Bordeaux. All rights reserved.

 @version 6.2.0
 @author Pierre Ramet
 @author Mathieu Faverge
 @author Louis Poirel
 @date 2021-04-07

"""
import pypastix as pastix
import scipy.sparse as sps
import scipy.sparse.linalg
import numpy as np

# Set the problem
n = 1000
A = sps.spdiags([np.ones(n)*i for i in [8., -1., -1., -1., -1.]],
                [0, 1, 3, -1, -3], n, n)
x0 = np.arange(n)

normA = sps.linalg.norm( A )
A = A / normA
b = A.dot(x0)

# Hack to make sure that the mkl is loaded
tmp = np.eye(2).dot(np.ones(2))

# Factorize
solver = pastix.solver(A, verbose=True)

# Solve
x = solver.solve( b )

resid = A.dot( x ) - b
print( "Norm residual: ", np.linalg.norm( resid ) )

# Check the solution
rc = solver.check( x, b, x0, eps=1e-12 )

solver.finalize()

exit(rc)
