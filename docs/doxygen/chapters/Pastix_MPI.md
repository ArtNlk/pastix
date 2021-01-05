## Installing PaStiX with MPI support

Here you have the features available in hybrid shared/distributed
memory with MPI between processes, and POSIX threads within a process.

|                         | Seq   | Static | Dyn   | StarPU | PaRSEC |
|-------------------------|-------|--------|-------|--------|--------|
| POTRF (Cholesky)        | FR    | FR     | FR    | FR     | FR     |
| PXTRF (LL^t for complex)| FR    | FR     | FR    | FR     | FR     |
| HETRF (LDL^h)           | FR    | FR     | FR    | FR     | FR     |
| SYTRF (LDL^t)           | FR    | FR     | FR    | FR     | FR     |
| GETRF (LU)              | FR    | FR     | FR    | FR     | FR     |
| TRSM                    | FR    | FR     | FR    | FR     | FR     |
| DIAG                    | FR    | FR     | FR    | FR     | FR     |

**Disclaimer**

Even if PaStiX works in distributed, it is still under development.
You can face either performance issues (without runtimes) or memory
issues (with StarPU or PaRSEC)

**Requirements**

In addition to the requirements of the sequential PaStiX, you'll need:

    * An MPI implementation (MPICH, OpenMPI...)

### Download and install OpenMPI

The Open MPI Project is an open source Message Passing Interface
implementation that is developed and maintained by a consortium
of academic, research, and industry partners.

We recommend a release >= 3.0 available on the [OpenMPI
website](https://www.open-mpi.org/).
A Ubuntu package (version 3.x) is also available on Ubuntu,
and can be download with :

```sh
sudo apt-get install openmpi-bin openmpi-common
```

To verify your downloaded version, you can launch :
```sh
mpiexec --version
```

### Download and install MPICH

MPICH is a high performance and widely portable implementation
of the Message Passing Interface (MPI) standard. MPICH and its
derivatives form the most widely used implementations of MPI in the world

We recommend a release >= 3.0 available on the [MPICH
website](https://www.mpich.org/).
A Ubuntu package (version 3.x) is also available on Ubuntu,
and can be download with :

```sh
sudo apt-get install mpich
```

To verify your downloaded version, you can launch :
```sh
mpiexec --version
```
### Download and install StarPU

Please refer to [_Installing PaStiX with runtime support_](./Pastix_Runtime.md)
for StarPU installation. The only step that will change for MPI support will be
during the configuration :

```sh
mkdir build_MPI
cd build_MPI
../starpu-1.3.7/configure --prefix=${STARPU_DIR} --enable-mpi
make
make install
```

### Download and install PaRSEC

Please refer to [_Installing PaStiX with runtime support_](./Pastix_Runtime.md)
for PaRSEC installation. The only step that will change for MPI support will be
during the configuration :

```sh
mkdir build_MPI
cd build_MPI
cmake .. -DCMAKE_INSTALL_PREFIX=${PARSEC_DIR} -DPARSEC_DIST_WITH_MPI=ON
make
make install
```

### Compile PaStiX with MPI

Now that you have installed MPI, you should be able to
compile PaStiX with MPI support:

Go in your PaStiX source directory and create a new build
directory to build the GPU version:
```sh
mkdir build_MPI
cd build_MPI
cmake .. -DCMAKE_INSTALL_PREFIX=${PASTIX_DIR} -DPASTIX_WITH_MPI=ON \
         -DPASTIX_WITH_STARPU=[ON|OFF] -DPASTIX_WITH_PARSEC=[ON|OFF]
make
make install
```

Note that as before we defined a `PASTIX_DIR` variable to define the
install directory of PaStiX, and you just need to turn ON/OFF the
runtime systems based on the available one(s).

Once the compilation finished, you can setup your environment easily
by sourcing the provided file:
```sh
source $PASTIX_DIR/bin/pastix_env.sh
```
And then, you can run your favorite example:
```sh
mpiexec -n 4 simple -9 10:10:10
```

### How to use pastix with MPI

PaStiX works with all the schedulers whithout runtimes in distributed
memory, but if you want the best performances possible with POSIX threads,
please use the dynamic scheduler (option **-s 4**):
```sh
mpiexec -n 4 simple -s 4 -9 10:10:10
```

In the same way, if you intent to run multiple MPI instances on one node,
make sur that the number of MPI instances per node times the number of
threads per MPI instances is less or equals the number of cores of the node.

If HWLOC gives you trouble in the recognition of the topology, you can
specify the amount of threads per node with the option **-t**.
But don't forget to contact us afterwards!