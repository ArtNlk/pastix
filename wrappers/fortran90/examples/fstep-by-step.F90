!
! @file fstep-by-step.F90
!
! Fortran 90 example using a matrix read with the spm driver.
!
! @copyright 2017-2023 Bordeaux INP, CNRS (LaBRI UMR 5800), Inria,
!                      Univ. Bordeaux. All rights reserved.
!
! @version 6.3.0
! @author Mathieu Faverge
! @date 2023-01-17
!
program fstep_by_step
  use iso_c_binding
  use spmf
  use pastixf
  ! use mpi_f08
  implicit none

  type(pastix_data_t),        pointer                      :: pastix_data
  type(pastix_order_t),       pointer                      :: order => null()
  type(spmatrix_t),           pointer                      :: spm
  type(spmatrix_t),           pointer                      :: spm2
  integer(kind=pastix_int_t), target                       :: iparm(iparm_size)
  real(kind=c_double),        target                       :: dparm(dparm_size)
  integer(c_int)                                           :: info
  integer(kind=pastix_int_t)                               :: nrhs
  real(kind=c_double), dimension(:,:), allocatable, target :: x0, x, b
  integer(kind=pastix_int_t), dimension(:), pointer        :: permtab
  integer                                                  :: i, j, nfact, nsolv

  nfact = 2
  nsolv = 3

  ! 1- Initialize the parameters and the solver
  call pastixInitParam( iparm, dparm )
  call pastixInit( pastix_data, MPI_COMM_WORLD, iparm, dparm )

  !
  ! Initialize the problem
  !   1- The matrix
  allocate( spm )
  call spmReadDriver( SpmDriverLaplacian, "d:10:10:10:2.", spm, info )

  allocate( spm2 )
  call spmCheckAndCorrect( spm, spm2, info )
  if ( info .ne. 0 ) then
     call spmExit( spm )
     spm = spm2
  end if
  deallocate( spm2 )

  call spmPrintInfo( spm )

  !   2- The right hand side
  nrhs = 10
  allocate(x0(spm%nexp,nrhs))
  allocate(x( spm%nexp,nrhs))
  allocate(b( spm%nexp,nrhs))

  ! 2- Perform ordering, symbolic factorization, and analyze steps
  call pastix_subtask_order( pastix_data, spm, order, info )
  call pastix_subtask_symbfact( pastix_data, info )
  call pastix_subtask_reordering( pastix_data, info )
  call pastix_subtask_blend( pastix_data, info )

  ! If needed, get the generated ordering
  call pastixOrderGet( pastix_data, order )

  ! Convert the permtab to Fortran array
  call pastixOrderGetArray( order, permtab=permtab )
  print *, permtab(1:10)

  ! 3- Factorize nfact times the matrix
  do i=0,nfact
     ! Perform the numerical factorization
     call pastix_subtask_spm2bcsc( pastix_data, spm, info )
     call pastix_subtask_bcsc2ctab( pastix_data, info )
     call pastix_subtask_sopalin( pastix_data, info )

     ! Perform nsolv solve steps
     do j=0,nsolv

        call spmGenRHS( SpmRhsRndX, nrhs, spm, x0, spm%nexp, b, spm%nexp, info )
        x = b

        ! 4- Solve the problem
        call pastix_task_solve( pastix_data, spm%nexp, nrhs, x, spm%nexp, info )

        ! 5- Refine the solution
        call pastix_task_refine( pastix_data, spm%nexp, nrhs, b, spm%nexp, x, spm%nexp, info )

        ! Check the solution
        call spmCheckAxb( dparm(DPARM_EPSILON_REFINEMENT), nrhs, spm, x0, spm%nexp, b, spm%nexp, x, spm%nexp, info )

     end do
  end do

  ! 6- Destroy the C data structure
  call pastixFinalize( pastix_data )

  call spmExit( spm )
  deallocate( spm )
  deallocate(x0)
  deallocate(x)
  deallocate(b)

end program fstep_by_step
