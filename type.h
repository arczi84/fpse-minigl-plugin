#ifndef _FPSE_TYPE_H_
#define _FPSE_TYPE_H_

// Return values (should be applied to the entire code).
#define FPSE_OK         0
#define FPSE_ERR        -1
#define FPSE_WARN       1

#ifndef COMPILER_MAY_ALIAS
  #if __GNUC__ >= 3
    #define COMPILER_MAY_ALIAS __attribute__ ((may_alias))
  #else
    #define COMPILER_MAY_ALIAS
  #endif
#endif

#ifndef EXCLUDE_FPSE_TYPES

typedef signed char          INT8;
typedef signed short int     INT16;
typedef signed long int      INT32;
typedef signed long long int INT64;

typedef unsigned char          UINT8;
typedef unsigned short int     UINT16;
typedef unsigned long int      UINT32;
typedef unsigned long long int UINT64;

#endif // EXCLUDE_FPSE_TYPES

#ifndef REG
  #ifdef __mc68000__
    #define REG(reg,arg) register arg asm(#reg)
  #else
    #define REG(reg,arg) arg
  #endif
#endif

#endif /* _FPSE_TYPE_H_ */
