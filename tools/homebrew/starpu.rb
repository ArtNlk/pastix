###
#
#  @file starpu.rb
#  @copyright 2020-2022 Bordeaux INP, CNRS (LaBRI UMR 5800), Inria,
#                       Univ. Bordeaux. All rights reserved.
#
#  @brief Homebrew formula for StarPU
#
#  @version 6.2.1
#  @author Pierre Ramet
#  @date 2021-04-25
#
###
# Documentation: https://docs.brew.sh/Formula-Cookbook
#                https://rubydoc.brew.sh/Formula
# PLEASE REMOVE ALL GENERATED COMMENTS BEFORE SUBMITTING YOUR PULL REQUEST!
class Starpu < Formula
  desc "StarPU is a task programming library for hybrid architectures"
  homepage "https://starpu.gitlabpages.inria.fr/"
  url "https://files.inria.fr/starpu/starpu-1.3.7/starpu-1.3.7.tar.gz"
  sha256 "1d7e01567fbd4a66b7e563626899374735e37883226afb96c8952fea1dab77c2"
  license "GNU GPL v2.1"

  depends_on "autoconf" => :build
  depends_on "automake" => :build
  depends_on "libtool" => :build
  depends_on "pkg-config" => :build
  depends_on "hwloc"
  depends_on "openmpi"

  def install
    system "./autogen.sh" if build.head?
    system "./configure", *std_configure_args
    system "make", "install"
  end

  test do
    (testpath/"test.c").write <<~EOS
      #include <stdio.h>
      #include <stdlib.h>
      #include <starpu.h>


      int main(int argc, char* argv[]) {
        /* initialize StarPU */
        starpu_init(NULL);
        struct starpu_task *task = starpu_task_create();
        task->cl = &cl; /* Pointer to the codelet defined above */
        /* starpu_task_submit will be a blocking call. If unset,
        starpu_task_wait() needs to be called after submitting the task. */
        task->synchronous = 1;
        /* submit the task to StarPU */
        starpu_task_submit(task);
        /* terminate StarPU */
        starpu_shutdown();
          return 0;
      }
    EOS

    system ENV.cc, "test.c", "-I#{include}", "-L#{lib}", "-lstarpu",
                   "-o", "test"
    system "./test"
  end
end
