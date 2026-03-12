#ifdef __STDC__
#include <limits.h>
#else
#include <assert.h>
#endif

int
main()
{
#if defined __stub_getgrgid_r || defined __stub___getgrgid_r
  return 0;
#else
this system have stub
  return 0;
#endif
}
