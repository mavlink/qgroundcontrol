#ifndef __win32_dirent__
#define __win32_dirent__

/* For Win32 that lacks Unix direct support. */

#include <windows.h>
#include <string.h>

struct dirent {
  char d_name[MAX_PATH];
};

typedef struct {
  WIN32_FIND_DATA wfd;
  HANDLE hFind;
  struct dirent de;
} DIR;

static DIR *
opendir(char *pSpec)
{
  DIR *pDir = malloc(sizeof(DIR));
  /* XXX Windows 95 has problems opening up "." though Windows NT does this
     fine?  Open "*" instead of "." to be safe. -mjk */
  pDir->hFind = FindFirstFile(strcmp(pSpec, ".") ? pSpec : "*",
    &pDir->wfd);
  return pDir;
}

static void
closedir(DIR * pDir)
{
  FindClose(pDir);
  free(pDir);
}

static struct dirent *
readdir(DIR *pDir)
{
  if (pDir->hFind) {
    strcpy(pDir->de.d_name, pDir->wfd.cFileName);
    if (!FindNextFile(pDir->hFind, &pDir->wfd))
      pDir->hFind = NULL;
    return &pDir->de;
  }
  return NULL;
}

#define fclose(f)  { if (f!=NULL) fclose(f); }

#endif /* __win32_dirent__ */
