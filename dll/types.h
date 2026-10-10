/*
 * types.h
 */

#ifndef _DLL_TYPES_H_
#define _DLL_TYPES_H_

#include <exec/exec.h>

typedef struct dll_symbol_s dll_symbol_t;
typedef struct dll_message_s dll_message_t;

enum {
  DLL_CPU_NULL = 0,
  DLL_CPU_PPC,
  DLL_CPU_M68K,
  DLL_CPU_X86,
  DLL_CPU_ARM,
  DLL_CPU_LAST
};

enum {
  DLL_OS_NULL = 0,
  DLL_OS_AMIGA_4,
  DLL_OS_AMIGA_3,
  DLL_OS_AROS,
  DLL_OS_MORPHOS,
  DLL_OS_LAST
};

#ifdef __PPC__
#define DLL_CURRENT_CPU DLL_CPU_PPC
#elif __mc68000__
#define DLL_CURRENT_CPU DLL_CPU_M68K
#elif __i386__
#define DLL_CURRENT_CPU DLL_CPU_X86
#elif __arm__
#define DLL_CURRENT_CPU DLL_CPU_ARM
#else
#define DLL_CURRENT_CPU DLL_CPU_NULL
#endif

#ifdef __amigaos4__
#define DLL_CURRENT_OS DLL_OS_AMIGA_4
#elif __AROS__
#define DLL_CURRENT_OS DLL_OS_AROS
#elif __MORPHOS__
#define DLL_CURRENT_OS DLL_OS_MORPHOS
#elif __AMIGA__
#define DLL_CURRENT_OS DLL_OS_AMIGA_3
#else
#define DLL_CURRENT_OS DLL_OS_NULL
#endif

enum {
  DLL_MESSAGE_NULL = 0,
  DLL_MESSAGE_OPEN,
  DLL_MESSAGE_GET_SYMBOLS,
  DLL_MESSAGE_CLOSE,
  DLL_MESSAGE_LAST
};

enum {
  DLL_RESULT_NULL = 0,
  DLL_RESULT_OK,
  DLL_RESULT_FILE_NOT_FOUND,
  DLL_RESULT_NOT_A_DLL_FILE,
  DLL_RESULT_WRONG_SYSTEM,
  DLL_RESULT_OUT_OF_MEMORY,
  DLL_RESULT_SETUP_FAILED,
  DLL_RESULT_SYMBOL_NOT_FOUND,
  DLL_RESULT_UNKNOWN_MESSAGE,
  DLL_RESULT_LAST
};

/* internal message structure */

struct dll_message_s {
  /* mandatory AmigaOS structure */
  struct Message message;

  /* type of message */
  int type;

  /* result code */
  int result;

  /* the message itself */
  union {
    /* DLL_MESSAGE_OPEN */
    struct {
      int cpu;
      int os;
    } open;

    /* DLL_MESSAGE_GET_SYMBOLS */
    struct {
      const dll_symbol_t *symbols;
    } get_symbols;

    /* DLL_MESSAGE_CLOSE */
    struct {
    } close;
  } msg;
};

/* symbol structure */

struct dll_symbol_s {
  const char *name;
  const void *address;
};

/* functions to deal with dll files */

const void *dll_open(const char *name, const char *log);
const void *dll_get_symbol_address(const void *dll, const char *name);
const char *dll_get_error(void);
void dll_close(const void *dll);

/* helpers to create dll files */

#define DLL_SYMBOLS_DECL_BEGIN()       \
  const dll_symbol_t dll_symbols[] = {

#define DLL_SYMBOL_DECL(name)          \
  { #name, &name },

#define DLL_SYMBOLS_DECL_END()         \
    { NULL, NULL }                     \
  };

/* the dll may define these */

int dll_setup(void);
void dll_cleanup(void);

/* the dll must define this */

extern const dll_symbol_t dll_symbols[];

#endif /* _DLL_TYPES_H_ */
