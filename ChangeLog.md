# pastix-6.2.2

- Integrate SPM 1.1.1 to fix int32 bits allocation on border cases
- graph: Save some memory during the graph allocation by avoiding the value array duplication
- Fix the NNZ output value when using int32
- Integrate patch from 6.3.0 to fix issue with gitlab-runner

# pastix-6.2.1

- MPI/PThreads: Small improvement on communication reactivity (pastix/pastix!278)
- Runtime: Fix issue in distributed when looping over the factorization with runtime systems (pastix/pastix!274)
- MPI/refinement: Fix issu in frobenius merge in distributed (pastix/pastix!276)
- MPI/low-rank: Introduce a first version of the distributed low-rank solver with PThread schedulers (pastix/pastix!270)
- analyze: Enable the generation of distributed simulated traces (pastix/pastix!272)
- LR/Schur: Fix issue when using low-rank and Schur functionnality that was re-ordering the Schur complement (pastix/pastix!273)
- api: Introduction of th generator for the iparm/dparm/enum in order to later extend their documentation (pastix/pastix!263)
- starpu/gpu: start testing the heteroprio scheduler (pastix/pastix!269)
- debug: Fix compilation of many debug functionnalities (pastix/pastix!267)
- doc: update on the tutorials
- spm: update the IO functionalities and integrate the fix on distributed load/save from SPM. (pastix/pastix!265)
- cmake: better protect in source compilation

# pastix-6.2.0

- Update cmake_morse submodule to use modern cmake detection
- Update spm submodule with a full MPI support of the sparse matrices
- headers: Fix issue for C++ inclusion of the spm/pastix headers
- headers: Add const keyword where data is input
- cmake: Add an uninstall rule
- Fix solverstack/pastix#43: issue with B null
- timings: add time for analysis subtasks as well as total time
- MPI: reduction of the communication buffer sizes
- Low-rank: Improve default settings for th low-rank factorization
- Low-rank: add ILU(k) preselection
- cmake: add options to disable low-rank testings depending on tmglib (solverstack/pastix#50)
- examples: Update testings to call pastixInit before spmInit in order to initialize MPI automatically if needed (solverstack/pastix#52)
- EZTrace: Fix issue when using EZTrace in distributed (Require master revision of EZTrace)
- Wrappers: Add MPI support intor Fortran, python and julia wrappers
- hwloc: Remove references to old revision of hwloc (< 1.0.0)
- Remove deprecated bzero
- doc: Add tutorials for compilation and usage of GPU, MPI and runtime versions
- scotch: add deterministic option to fix the random algorithms
- scotch: add multi-threaded support for future scotch 6.2.0
- graph: exploit the spm structure to manipulate the graph in the ordering step
- sopalin: add a IPARM_TRANSPOSE_SOLVE option to solve A^t x = b in order to avoid CSC/CSR conversions
- example: add a simple_dist example to show how to use the solver with a distributed sparse matrix (be careful, for now, the matrix is gathered multiple times and it may create large slow down)
- schur: Fix schur complement factorization with dynamic scheduler
- scheduler: dynamic scheduler is now the default
- Licence: Change licence from Cecill-C to LGPL

# pastix-6.1.0

- Add a new dynamic scheduler supported by the internal threads for numerical factorization
- Add MPI support for the numerical factorization and solve
        - Available for all schedulers: sequential, internal threads (static, dynamic), StarPU, and PaRSEC
        - WARNING: The RHS is not distributed yet, and must be replicated on all nodes
        - WARNING: The low-rank and Schur functionalities are not available in distributed yet
- Enable the use of an external SPM module
- Improve splitting strategy:
        - avoid unnecessary splits when using K-Way
        - reduce the range of possible split to limit the apparition of small blocks
- Change the preselected behavior to be:
        - never compressed in the JustInTime scenario
        - compress the preselected block just before applying the TRSM in the MinimalMemory scenario
        - the behavior can be change through IPARM_COMPRESS_PRESELECT
- Add a cmake summary
- Add coverity scan
- Update README.md and documentation

# pastix-6.0.3

- Update spm module to ada4963
- Update morse_cmake to ade4996
  - CMake: Update cmake_module to integrate the last version of the precision generator
- Change StarPU requirement to >= 1.3
- Refactor and extend the CI/CTests
- Update documentation
- Low-rank:
  - Add a new parameter IPARM_COMPRESS_RELTOL to switch between absolute and relative tolerance
  - Improved stability of low-rank kernels
  - Extend the number of tests
  - Add rotation QR kernels (unstable/work in progress)
  - Enable multiple low-rank factorization in a row
- Supports compilation with mpicc (no distributed solver yet)
- Add separated output directories for future distributed process, or for MPI multiple instances
- Octave: Fix issue with number of threads larger than the number of columns
- Octave: Fix compilation on Windows system
- Documentation: add documentation on process binding
- HwLoc: fix binding when already restricted through batch scheduler and/or MPI
- Fix issue solverstack/pastix#35, make pastix_task_analyze thread safe
- Add support for multi-dof in Fortran
- Fix issue in simulation, and a switch between cost and tree levels
- Refinement: Fix issue with gemv computation and PastixConjTrans
- CMake: Enable a round-robin selection of CMAKE_BUILD_TYPE  depending on the sanitizers provided by the compiler
- Homebrew: update formula

# pastix-6.0.2

- Integrate the clusting strategies developped for low-rank (See https://hal.inria.fr/hal-01961675)
- Restructure the ordering/symbolic factorization code to make sure with exit the ordering step with permutation, partition, and elimination tree.
- Relook the splitting/proportional mapping strategy
- Add new compression kernels: PQRCP, RQRCP, and TQRCP
- Fix inplace compilation (Issue #36)
- Fix issue when StarPU threads where fighting for ressources with PaStiX threads (Add pause/resume calls)
- Handle multi-dof static and/or variable in the analysis steps

# pastix-6.0.1

- Support for HWLOC 2.0.0
- Move the SPM library as a submodule
- Parallel (multithreaded) version of the reordering step
- Parallel (multithreaded) version of the refinement steps
- StarPU version of the solve step
- Fix Python/Fortran interface
- Fix Schur functions
- Fix multi-RHS solve
- Fix for METIS
- Update morse_cmake FindPACKAGE
- Update PaRSEC for release 6.0.1
- Improve PKG-CONFIG
- Improve documentation
- Better handle of disconnected graphs
- Add optimal reordering for grids
- Add more detailed statistics during analysis step
- Add detailed statistics about memory gain for the low-rank solver
- Add a function to compress the solver matrix outside the factorization step
- Add an example to dump the symbol matrix including the ranks of the block
- Add an refinement driver and testings
- Add a subtask_refine which does not perform the vector ordering
- Add a more complex testing based on example proposed by @andrea3.14
- Add an iparm IPARM_APPLYPERM_WS to enable/disable the use of an extra workspace to make the functions bvec_xlapmr thread-safe (by default, it is enabled, if disabled, the functions have no memory overhead but loose the thread-safe property)
- Remove the sparse-kit package to avoid conflict (the driver is replaced by HB)

# pastix-6.0.0

- low-rank compression (See https://hal.inria.fr/hal-01824275)
- static scheduler, PaRSEC and StarPU runtime support
- GPUs (Kepler) and KNL support through runtime systems
