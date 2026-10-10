/* Separate -pg diagnostic identities and explicit Amiga output paths. */
#ifdef FPSE_GPROF_SELFTEST
#define FPSE_GPROF_NAME "gprof-selftest"
#define FPSE_GPROF_DATA "PROGDIR:gprof-selftest.gmon"
#define FPSE_GPROF_STATUS "PROGDIR:gprof-selftest.txt"
#elif __GNUC__ == 6 && __GNUC_MINOR__ == 5
#define FPSE_GPROF_NAME "gpuminigl-gprof-gcc65"
#define FPSE_GPROF_DATA "PROGDIR:fpse-minigl-gcc65.gmon"
#define FPSE_GPROF_STATUS "PROGDIR:fpse-minigl-gcc65-gprof.txt"
#else
#define FPSE_GPROF_NAME "gpuminigl-gprof"
#define FPSE_GPROF_DATA "PROGDIR:fpse-minigl.gmon"
#define FPSE_GPROF_STATUS "PROGDIR:fpse-minigl-gprof.txt"
#endif
void fpse_gprof_begin(void);
void fpse_gprof_end(void);
