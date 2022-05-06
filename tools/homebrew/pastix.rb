###
#
#  @file pastix.rb
#  @copyright 2013-2022 Bordeaux INP, CNRS (LaBRI UMR 5800), Inria,
#                       Univ. Bordeaux. All rights reserved.
#
#  @brief Homebrew formula for PaStiX 6.*
#
#  @version 6.2.1
#  @author Pierre Ramet
#  @author Mathieu Faverge
#  @date 2021-04-25
#
###
class Pastix < Formula
  desc "Parallel solver for sparse linear systems based on direct methods"
  homepage "https://gitlab.inria.fr/solverstack/pastix"
  url "https://gitlab.inria.fr/solverstack/pastix//uploads/5c09cf4e0c120c45e27bc20d24e7f521/pastix-6.2.0.tar.gz"
  sha256 "dbf2f252c8162cbde2c52730325b9ae6d8410772be5b4e93c368d9b8d2d79257"
  head "https://gitlab.inria.fr/solverstack/pastix.git"
  license "LGPL"

  bottle :disable, "needs to be rebuilt"

  depends_on "openblas"
  depends_on "openmpi"
  depends_on "hwloc"               # Could be optinal but strongly recommanded
  depends_on "scotch"              # Could be optinal but strongly recommanded
  depends_on "metis"  => :optional # Enable METIS ordering
  depends_on "starpu" => :optional # Enable STARPU runtime
  depends_on "gcc"    => :build    # GNU Fortran is now provided as part of GCC
  depends_on "cmake"  => :build

  def install
    args = ["-DCMAKE_INSTALL_PREFIX=#{prefix}",
            "-DBUILD_SHARED_LIBS=ON",
            "-DBUILD_DOCUMENTATION=OFF",
            "-DPASTIX_INT64=OFF",
            "-DPASTIX_ORDERING_SCOTCH=ON",
            "-DPASTIX_WITH_FORTRAN=ON",
            "-DPASTIX_WITH_MPI=ON",
            "-DPASTIX_WITH_CUDA=OFF",
            "-DPASTIX_WITH_PARSEC=OFF"]
    args += ["-DPASTIX_ORDERING_METIS=ON"] if build.with? "metis"
    args += ["-DPASTIX_WITH_STARPU=ON"] if build.with? "starpu"
    mkdir "build" do
      system "cmake", "..", *args
      system "make"
      system "make", "install"
    end
    pkgshare.install "example" # Contains all test programs.
  end

  def caveats; <<~EOS
    Set the PYTHONPATH environment variable:
      export PYTHONPATH=#{prefix}/lib/python:$PYTHONPATH
    Try python example with:
      python #{prefix}/examples/python/simple.py
    Or simple example with:
      #{prefix}/examples/simple -9 10:10:10
    EOS
  end

  test do
    system "#{prefix}/examples/simple", "-9", "10:10:10"
    ohai "All test output is in #{HOMEBREW_LOGS}/pastix. Please check."
  end
end
