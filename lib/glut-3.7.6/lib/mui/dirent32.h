/*

    Win32 lacks unix dirent support.  But, we can fake it.  Many
    thanks to Dave Lubrik (lubrik@jaka.ece.uiuc.edu) who found and
    fixed many bugs in the original code.

 */


#ifndef _WIN32
#include <dirent.h>
#else

#include <windows.h>



struct dirent {
    char           d_name[MAX_PATH]; 
};

typedef struct {
    WIN32_FIND_DATA  wfd;
    HANDLE           hFind;
    struct dirent    de;
} DIR;


static DIR *
opendir(char *pSpec)
{
  DIR *pDir = malloc(sizeof(DIR));
  char pathnamespec[MAX_PATH];
  int l;				/* length of directory specifier */
  char c;				/* last char of directory specifier */

  /* Given a directory pathname in pSpec, add \ (if necessary) and *
     to yield a globbable expression describing all the files in that
     directory */
  strcpy(pathnamespec, pSpec);

  /* Add a \ to separate the directory name from the filename-wildcard
     "*", unless it already ends in a \ (don't create \\ sequences),
     or it is a drivespec (since "C:*" differs in meaning from "C:\*") */
  if (((l = strlen(pSpec)) > 0) && ((c = pSpec[l-1]) != '\\') && (c != ':'))
    strcat(pathnamespec, "\\");

  /* Add the filename wildcard "*" */
  strcat(pathnamespec,"*");

  /* Find files matching that expression (all the files in that
     directory) */
  pDir->hFind = FindFirstFile(pathnamespec, &pDir->wfd);

  return pDir;
}


/* closedir takes a pointer to a DIR structure created by opendir, and
   frees up resources allocated by opendir. Call it when done with a
   directory. */
static void
closedir(DIR * pDir)
{
    FindClose(pDir->hFind);		/* Release system resources */
    free(pDir);				/* release memory */
}

/* readdir is used to iterate through the files in a directory.  It
   takes a pointer to a DIR structure created by opendir, and each
   time it is called it returns the name of another file in the
   directory passed to opendir.  Returns: a pointer to a dirent
   structure, containing the file name.  NULL if there are no more
   files in the directory. */
static struct dirent *
readdir(DIR *pDir)
{
    /* The previous call to opendir or readdir has already found the next
           file (using FindFirstFile or FindNextFile respectively).  Return
           that file name to the caller, and silently find the next one. */

    if (*(pDir->wfd.cFileName)) {	/* If we haven't exhausted the files */
	strcpy(pDir->de.d_name, pDir->wfd.cFileName); /* copy name */
	
	if (!FindNextFile(pDir->hFind, &pDir->wfd)) /* get next */
	    *(pDir->wfd.cFileName) = 0;
	/* if no more, zero next filename, so that next time through,
	   we don't even try. */
	
	return &pDir->de;		/* return dirent struct w/filename */
    }

    return NULL;            /* No more files to find. */
}

#endif
