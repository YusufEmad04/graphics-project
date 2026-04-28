// MSVC + UCRT compatibility shim for the prebuilt freeglut static library
// (NuGet 2.8.1.15, built with VS2012). The old lib references __iob_func
// which UCRT removed. We only need this when linking against the static
// freeglut.lib on modern MSVC.
#if defined(_MSC_VER) && _MSC_VER >= 1900
#include <stdio.h>
extern "C" FILE *__cdecl __iob_func(void)
{
    static FILE iob[3];
    iob[0] = *stdin;
    iob[1] = *stdout;
    iob[2] = *stderr;
    return iob;
}
#endif
