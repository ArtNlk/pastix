/* Need to conserve it MURGE compliant */
enum ERR_NUMBERS {
  NO_ERR             = 0,
  UNKNOWN_ERR        = 1,
  ALLOC_ERR          = 2,
  ASSERT_ERR         = 3,
  NOTIMPLEMENTED_ERR = 4,
  OUTOFMEMORY_ERR    = 5,
  THREAD_ERR         = 6,
  INTERNAL_ERR       = 7,
  BADPARAMETER_ERR   = 8,
  FILE_ERR           = 9,
  BAD_DEFINE_ERR     = 10,
  INTEGER_TYPE_ERR   = 11,
  IO_ERR             = 12,
  MATRIX_ERR         = 13,
  FLOAT_TYPE_ERR     = 14,
  STEP_ORDER_ERR     = 15,
  MPI_ERR            = 16
};

/* _POS_ 4 */
enum API_FACT {
  API_FACT_LLT  = PastixFactLLT,
  API_FACT_LDLT = PastixFactLDLT,
  API_FACT_LU   = PastixFactLU,
  API_FACT_LDLH = PastixFactLDLH
};

/* _POS_ 8 */
#define API_RAFF_GMRES    API_REFINE_GMRES
#define API_RAFF_GRAD     API_REFINE_GRAD
#define API_RAFF_PIVOT    API_REFINE_PIVOT
#define API_RAFF_BICGSTAB API_REFINE_BICGSTAB

/* _POS_ 61 */
enum API_FLOAT {
  API_REALSINGLE    = PastixFloat,
  API_REALDOUBLE    = PastixDouble,
  API_COMPLEXSINGLE = PastixComplex32,
  API_COMPLEXDOUBLE = PastixComplex64
};

/* The following parameters were modified (typo) in refinement */
/* IPARM_ONLY_REFINE */
/* IPARM_MURGE_MAY_REFINE */
/* DPARM_REFINE_TIME */
/* API_TASK_REFINE */
