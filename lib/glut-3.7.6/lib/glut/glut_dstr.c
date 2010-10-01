
/* Copyright (c) Mark J. Kilgard, 1997. */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "glutint.h"

/* glxcaps matches the criteria macros listed in glutint.h, but
   only list the first set (those that correspond to GLX visual
   attributes). */
static int glxcap[NUM_GLXCAPS] =
{
  GLX_RGBA,
  GLX_BUFFER_SIZE,
  GLX_DOUBLEBUFFER,
  GLX_STEREO,
  GLX_AUX_BUFFERS,
  GLX_RED_SIZE,
  GLX_GREEN_SIZE,
  GLX_BLUE_SIZE,
  GLX_ALPHA_SIZE,
  GLX_DEPTH_SIZE,
  GLX_STENCIL_SIZE,
  GLX_ACCUM_RED_SIZE,
  GLX_ACCUM_GREEN_SIZE,
  GLX_ACCUM_BLUE_SIZE,
  GLX_ACCUM_ALPHA_SIZE,
  GLX_LEVEL,
};

#ifdef TEST

#if !defined(_WIN32)
char *__glutProgramName = "dstr";
Display *__glutDisplay;
int __glutScreen;
XVisualInfo *(*__glutDetermineVisualFromString) (char *string, Bool * treatAsSingle,
  Criterion * requiredCriteria, int nRequired, int requiredMask, void **fbc) = NULL;
char *__glutDisplayString = NULL;
#endif
static int verbose = 0;

static char *compstr[] =
{
  "none", "=", "!=", "<=", ">=", ">", "<", "~"
};
static char *capstr[] =
{
  "rgba", "bufsize", "double", "stereo", "auxbufs", "red", "green", "blue", "alpha",
  "depth", "stencil", "acred", "acgreen", "acblue", "acalpha", "level", "xvisual",
  "transparent", "samples", "xstaticgray", "xgrayscale", "xstaticcolor", "xpseudocolor",
  "xtruecolor", "xdirectcolor", "slow", "conformant", "num"
};

static void
printCriteria(Criterion * criteria, int ncriteria)
{
  int i;
  printf("Criteria: %d\n", ncriteria);
  for (i = 0; i < ncriteria; i++) {
    printf("  %s %s %d\n",
      capstr[criteria[i].capability],
      compstr[criteria[i].comparison],
      criteria[i].value);
  }
}

#endif /* TEST */

static int isMesaGLX = -1;

static int
determineMesaGLX(void)
{
#ifdef GLX_VERSION_1_1
  const char *vendor, *version, *ch;

  vendor = glXGetClientString(__glutDisplay, GLX_VENDOR);
  if (!strcmp(vendor, "Brian Paul")) {
    version = glXGetClientString(__glutDisplay, GLX_VERSION);
    for (ch = version; *ch != ' ' && *ch != '\0'; ch++);
    for (; *ch == ' ' && *ch != '\0'; ch++);

#define MESA_NAME "Mesa "  /* Trailing space is intentional. */

    if (!strncmp(MESA_NAME, ch, sizeof(MESA_NAME) - 1)) {
      return 1;
    }
  }
#else
  /* Recent versions for Mesa should support GLX 1.1 and
     therefore glXGetClientString.  If we get into this case,
     we would be compiling against a true OpenGL not supporting
     GLX 1.1, and the resulting compiled library won't work well
     with Mesa then. */
#endif
  return 0;
}

static XVisualInfo **
getMesaVisualList(int *n)
{
  XVisualInfo **vlist, *vinfo;
  int attribs[23];
  int i, x, cnt;

  vlist = (XVisualInfo **) malloc((32 + 16) * sizeof(XVisualInfo *));
  if (!vlist)
    __glutFatalError("out of memory.");

  cnt = 0;
  for (i = 0; i < 32; i++) {
    x = 0;
    attribs[x] = GLX_RGBA;
    x++;
    attribs[x] = GLX_RED_SIZE;
    x++;
    attribs[x] = 1;
    x++;
    attribs[x] = GLX_GREEN_SIZE;
    x++;
    attribs[x] = 1;
    x++;
    attribs[x] = GLX_BLUE_SIZE;
    x++;
    attribs[x] = 1;
    x++;
    if (i & 1) {
      attribs[x] = GLX_DEPTH_SIZE;
      x++;
      attribs[x] = 1;
      x++;
    }
    if (i & 2) {
      attribs[x] = GLX_STENCIL_SIZE;
      x++;
      attribs[x] = 1;
      x++;
    }
    if (i & 4) {
      attribs[x] = GLX_ACCUM_RED_SIZE;
      x++;
      attribs[x] = 1;
      x++;
      attribs[x] = GLX_ACCUM_GREEN_SIZE;
      x++;
      attribs[x] = 1;
      x++;
      attribs[x] = GLX_ACCUM_BLUE_SIZE;
      x++;
      attribs[x] = 1;
      x++;
    }
    if (i & 8) {
      attribs[x] = GLX_ALPHA_SIZE;
      x++;
      attribs[x] = 1;
      x++;
      if (i & 4) {
        attribs[x] = GLX_ACCUM_ALPHA_SIZE;
        x++;
        attribs[x] = 1;
        x++;
      }
    }
    if (i & 16) {
      attribs[x] = GLX_DOUBLEBUFFER;
      x++;
    }
    attribs[x] = None;
    x++;
    assert(x <= sizeof(attribs) / sizeof(attribs[0]));
    vinfo = glXChooseVisual(__glutDisplay, __glutScreen, attribs);
    if (vinfo) {
      vlist[cnt] = vinfo;
      cnt++;
    }
  }
  for (i = 0; i < 16; i++) {
    x = 0;
    if (i & 1) {
      attribs[x] = GLX_DEPTH_SIZE;
      x++;
      attribs[x] = 1;
      x++;
    }
    if (i & 2) {
      attribs[x] = GLX_STENCIL_SIZE;
      x++;
      attribs[x] = 1;
      x++;
    }
    if (i & 4) {
      attribs[x] = GLX_DOUBLEBUFFER;
      x++;
    }
    if (i & 8) {
      attribs[x] = GLX_LEVEL;
      x++;
      attribs[x] = 1;
      x++;
#if defined(GLX_TRANSPARENT_TYPE_EXT) && defined(GLX_TRANSPARENT_INDEX_EXT)
      attribs[x] = GLX_TRANSPARENT_TYPE_EXT;
      x++;
      attribs[x] = GLX_TRANSPARENT_INDEX_EXT;
      x++;
#endif
    }
    attribs[x] = None;
    x++;
    assert(x <= sizeof(attribs) / sizeof(attribs[0]));
    vinfo = glXChooseVisual(__glutDisplay, __glutScreen, attribs);
    if (vinfo) {
      vlist[cnt] = vinfo;
      cnt++;
    }
  }

  *n = cnt;
  return vlist;
}

static FrameBufferMode *
loadVisuals(int *nitems_return)
{
  XVisualInfo *vinfo, **vlist, template;
  FrameBufferMode *fbmodes, *mode;
  int n, i, j, rc, glcapable;
#if defined(GLX_VERSION_1_1) && defined(GLX_SGIS_multisample)
  int multisample;
#endif
#if defined(GLX_VERSION_1_1) && defined(GLX_EXT_visual_info)
  int visual_info;
#endif
#if defined(GLX_VERSION_1_1) && defined(GLX_EXT_visual_rating)
  int visual_rating;
#endif
#if defined(GLX_VERSION_1_1) && defined(GLX_SGIX_fbconfig)
  int fbconfig;
#endif

  isMesaGLX = determineMesaGLX();
  if (isMesaGLX) {
    vlist = getMesaVisualList(&n);
  } else {
#if !defined(_WIN32)
    template.screen = __glutScreen;
    vinfo = XGetVisualInfo(__glutDisplay, VisualScreenMask, &template, &n);
#else
    vinfo = XGetVisualInfo(__glutDisplay, 0, &template, &n);
#endif
    if (vinfo == NULL) {
      *nitems_return = 0;
      return NULL;
    }
    assert(n > 0);

    /* Make an array of XVisualInfo* pointers to help the Mesa
       case because each glXChooseVisual call returns a
       distinct XVisualInfo*, not a handy array like
       XGetVisualInfo.  (Mesa expects us to return the _exact_
       pointer returned by glXChooseVisual so we could not just
       copy the returned structure.) */
    vlist = (XVisualInfo **) malloc(n * sizeof(XVisualInfo *));
    if (!vlist)
      __glutFatalError("out of memory.");
    for (i = 0; i < n; i++) {
      vlist[i] = &vinfo[i];
    }
  }

#if defined(GLX_VERSION_1_1) && defined(GLX_SGIS_multisample)
  multisample = __glutIsSupportedByGLX("GLX_SGIS_multisample");
#endif
#if defined(GLX_VERSION_1_1) && defined(GLX_EXT_visual_info)
  visual_info = __glutIsSupportedByGLX("GLX_EXT_visual_info");
#endif
#if defined(GLX_VERSION_1_1) && defined(GLX_EXT_visual_rating)
  visual_rating = __glutIsSupportedByGLX("GLX_EXT_visual_rating");
#endif
#if defined(GLX_VERSION_1_1) && defined(GLX_SGIX_fbconfig)
  fbconfig = __glutIsSupportedByGLX("GLX_SGIX_fbconfig");
#endif

  fbmodes = (FrameBufferMode *) malloc(n * sizeof(FrameBufferMode));
  if (fbmodes == NULL) {
    *nitems_return = -1;
    return NULL;
  }
  for (i = 0; i < n; i++) {
    mode = &fbmodes[i];
    mode->vi = vlist[i];
#if defined(GLX_VERSION_1_1) && defined(GLX_SGIX_fbconfig)
    mode->fbc = NULL;
#endif
    rc = glXGetConfig(__glutDisplay, vlist[i], GLX_USE_GL, &glcapable);
    if (rc == 0 && glcapable) {
      mode->valid = 1;  /* Assume the best until proven
                           otherwise. */
      for (j = 0; j < NUM_GLXCAPS; j++) {
        rc = glXGetConfig(__glutDisplay, vlist[i], glxcap[j], &mode->cap[j]);
        if (rc != 0) {
          mode->valid = 0;
        }
      }
#if defined(_WIN32)
      mode->cap[XVISUAL] = ChoosePixelFormat(XHDC, vlist[i]);
#else
      mode->cap[XVISUAL] = (int) vlist[i]->visualid;
#endif
      mode->cap[XSTATICGRAY] = 0;
      mode->cap[XGRAYSCALE] = 0;
      mode->cap[XSTATICCOLOR] = 0;
      mode->cap[XPSEUDOCOLOR] = 0;
      mode->cap[XTRUECOLOR] = 0;
      mode->cap[XDIRECTCOLOR] = 0;
#if !defined(_WIN32)
#if defined(__cplusplus) || defined(c_plusplus)
      switch (vlist[i]->c_class) {
#else
      switch (vlist[i]->class) {
#endif
      case StaticGray:
        mode->cap[XSTATICGRAY] = 1;
        break;
      case GrayScale:
        mode->cap[XGRAYSCALE] = 1;
        break;
      case StaticColor:
        mode->cap[XSTATICCOLOR] = 1;
        break;
      case PseudoColor:
        mode->cap[XPSEUDOCOLOR] = 1;
        break;
      case TrueColor:
        mode->cap[XTRUECOLOR] = 1;
        break;
      case DirectColor:
        mode->cap[XDIRECTCOLOR] = 1;
        break;
      }
#endif
#if defined(GLX_VERSION_1_1) && defined(GLX_EXT_visual_rating)
      if (visual_rating) {
        int rating;

/* babcock@cs.montana.edu reported that DEC UNIX (OSF1) V4.0
   564 for Alpha did not properly define GLX_VISUAL_CAVEAT_EXT
   in <GL/glx.h> despite claiming to support
   GLX_EXT_visual_rating. */
#ifndef GLX_VISUAL_CAVEAT_EXT
#define GLX_VISUAL_CAVEAT_EXT 0x20
#endif

        rc = glXGetConfig(__glutDisplay,
	  vlist[i], GLX_VISUAL_CAVEAT_EXT, &rating);
        if (rc != 0) {
          mode->cap[SLOW] = 0;
          mode->cap[CONFORMANT] = 1;
        } else {
          switch (rating) {
          case GLX_SLOW_VISUAL_EXT:
            mode->cap[SLOW] = 1;
            mode->cap[CONFORMANT] = 1;
            break;

/* IRIX 5.3 for the R10K Indigo2 may have shipped without this
   properly defined in /usr/include/GL/glxtokens.h */
#ifndef GLX_NON_CONFORMANT_VISUAL_EXT
#define GLX_NON_CONFORMANT_VISUAL_EXT   0x800D
#endif

          case GLX_NON_CONFORMANT_VISUAL_EXT:
            mode->cap[SLOW] = 0;
            mode->cap[CONFORMANT] = 0;
            break;
          case GLX_NONE_EXT:
          default:     /* XXX Hopefully this is a good default
                           assumption. */
            mode->cap[SLOW] = 0;
            mode->cap[CONFORMANT] = 1;
            break;
          }
        }
      } else {
        mode->cap[TRANSPARENT] = 0;
      }
#else
      mode->cap[SLOW] = 0;
      mode->cap[CONFORMANT] = 1;
#endif
#if defined(GLX_VERSION_1_1) && defined(GLX_EXT_visual_info)
      if (visual_info) {
        int transparent;

/* babcock@cs.montana.edu reported that DEC UNIX (OSF1) V4.0
   564 for Alpha did not properly define
   GLX_TRANSPARENT_TYPE_EXT in <GL/glx.h> despite claiming to
   support GLX_EXT_visual_info. */
#ifndef GLX_TRANSPARENT_TYPE_EXT
#define GLX_TRANSPARENT_TYPE_EXT 0x23
#endif

        rc = glXGetConfig(__glutDisplay,
          vlist[i], GLX_TRANSPARENT_TYPE_EXT, &transparent);
        if (rc != 0) {
          mode->cap[TRANSPARENT] = 0;
        } else {
          mode->cap[TRANSPARENT] = (transparent != GLX_NONE_EXT);
        }
      } else {
        mode->cap[TRANSPARENT] = 0;
      }
#else
      mode->cap[TRANSPARENT] = 0;
#endif
#if defined(GLX_VERSION_1_1) && defined(GLX_SGIS_multisample)
      if (multisample) {
        rc = glXGetConfig(__glutDisplay,
	  vlist[i], GLX_SAMPLES_SGIS, &mode->cap[SAMPLES]);
        if (rc != 0) {
          mode->cap[SAMPLES] = 0;
        }
      } else {
        mode->cap[SAMPLES] = 0;
      }
#else
      mode->cap[SAMPLES] = 0;
#endif
    } else {
#if defined(GLX_VERSION_1_1) && defined(GLX_SGIX_fbconfig)
      if (fbconfig) {
        GLXFBConfigSGIX fbc;
        int fbconfigID, drawType, renderType;

        fbc = glXGetFBConfigFromVisualSGIX(__glutDisplay, vlist[i]);
        if (fbc) {
          rc = glXGetFBConfigAttribSGIX(__glutDisplay, fbc,
	    GLX_FBCONFIG_ID_SGIX, &fbconfigID);
          if ((rc == 0) && (fbconfigID != None)) {
            rc = glXGetFBConfigAttribSGIX(__glutDisplay, fbc,
	      GLX_DRAWABLE_TYPE_SGIX, &drawType);
            if ((rc == 0) && (drawType & GLX_WINDOW_BIT_SGIX)) {
              rc = glXGetFBConfigAttribSGIX(__glutDisplay, fbc,
	        GLX_RENDER_TYPE_SGIX, &renderType);
              if ((rc == 0) && (renderType & GLX_RGBA_BIT_SGIX)) {
                mode->fbc = fbc;
                mode->valid = 1;  /* Assume the best until
                                     proven otherwise. */

		assert(glxcap[0] == GLX_RGBA);
                mode->cap[0] = 1;

                /* Start with "j = 1" to skip the GLX_RGBA attribute. */
                for (j = 1; j < NUM_GLXCAPS; j++) {
                  rc = glXGetFBConfigAttribSGIX(__glutDisplay,
		    fbc, glxcap[j], &mode->cap[j]);
                  if (rc != 0) {
                    mode->valid = 0;
                  }
                }

                mode->cap[XVISUAL] = (int) vlist[i]->visualid;
                mode->cap[XSTATICGRAY] = 0;
                mode->cap[XGRAYSCALE] = 0;
                mode->cap[XSTATICCOLOR] = 0;
                mode->cap[XPSEUDOCOLOR] = 0;
                mode->cap[XTRUECOLOR] = 0;
                mode->cap[XDIRECTCOLOR] = 0;
#if defined(__cplusplus) || defined(c_plusplus)
                switch (vlist[i]->c_class) {
#else
                switch (vlist[i]->class) {
#endif
                case StaticGray:
                  mode->cap[XSTATICGRAY] = 1;
                  break;
                case GrayScale:
                  mode->cap[XGRAYSCALE] = 1;
                  break;
                case StaticColor:
                  mode->cap[XSTATICCOLOR] = 1;
                  break;
                case PseudoColor:
                  mode->cap[XPSEUDOCOLOR] = 1;
                  break;
                case TrueColor:
                  mode->cap[XTRUECOLOR] = 1;
                  break;
                case DirectColor:
                  mode->cap[XDIRECTCOLOR] = 1;
                  break;
                }
#if defined(GLX_VERSION_1_1) && defined(GLX_EXT_visual_rating)
                if (visual_rating) {
                  int rating;

/* babcock@cs.montana.edu reported that DEC UNIX (OSF1) V4.0
   564 for Alpha did not properly define GLX_VISUAL_CAVEAT_EXT
   in <GL/glx.h> despite claiming to support
   GLX_EXT_visual_rating. */
#ifndef GLX_VISUAL_CAVEAT_EXT
#define GLX_VISUAL_CAVEAT_EXT 0x20
#endif

                  rc = glXGetFBConfigAttribSGIX(__glutDisplay,
		    fbc, GLX_VISUAL_CAVEAT_EXT, &rating);
                  if (rc != 0) {
                    mode->cap[SLOW] = 0;
                    mode->cap[CONFORMANT] = 1;
                  } else {
                    switch (rating) {
                    case GLX_SLOW_VISUAL_EXT:
                      mode->cap[SLOW] = 1;
                      mode->cap[CONFORMANT] = 1;
                      break;

/* IRIX 5.3 for the R10K Indigo2 may have shipped without this
   properly defined in /usr/include/GL/glxtokens.h */
#ifndef GLX_NON_CONFORMANT_VISUAL_EXT
#define GLX_NON_CONFORMANT_VISUAL_EXT   0x800D
#endif

                    case GLX_NON_CONFORMANT_VISUAL_EXT:
                      mode->cap[SLOW] = 0;
                      mode->cap[CONFORMANT] = 0;
                      break;
                    case GLX_NONE_EXT:
                    default:  /* XXX Hopefully this is a good
                                  default assumption. */
                      mode->cap[SLOW] = 0;
                      mode->cap[CONFORMANT] = 1;
                      break;
                    }
                  }
                } else {
                  mode->cap[TRANSPARENT] = 0;
                }
#else
                mode->cap[SLOW] = 0;
                mode->cap[CONFORMANT] = 1;
#endif
#if defined(GLX_VERSION_1_1) && defined(GLX_EXT_visual_info)
                if (visual_info) {
                  int transparent;

/* babcock@cs.montana.edu reported that DEC UNIX (OSF1) V4.0
   564 for Alpha did not properly define
   GLX_TRANSPARENT_TYPE_EXT in <GL/glx.h> despite claiming to
   support GLX_EXT_visual_info. */
#ifndef GLX_TRANSPARENT_TYPE_EXT
#define GLX_TRANSPARENT_TYPE_EXT 0x23
#endif

                  rc = glXGetFBConfigAttribSGIX(__glutDisplay,
		    fbc, GLX_TRANSPARENT_TYPE_EXT, &transparent);
                  if (rc != 0) {
                    mode->cap[TRANSPARENT] = 0;
                  } else {
                    mode->cap[TRANSPARENT] = (transparent != GLX_NONE_EXT);
                  }
                } else {
                  mode->cap[TRANSPARENT] = 0;
                }
#else
                mode->cap[TRANSPARENT] = 0;
#endif
#if defined(GLX_VERSION_1_1) && defined(GLX_SGIS_multisample)
                if (multisample) {
                  rc = glXGetFBConfigAttribSGIX(__glutDisplay,
		    fbc, GLX_SAMPLES_SGIS, &mode->cap[SAMPLES]);
                  if (rc != 0) {
                    mode->cap[SAMPLES] = 0;
                  }
                } else {
                  mode->cap[SAMPLES] = 0;
                }
#else
                mode->cap[SAMPLES] = 0;
#endif

              } else {
                /* Fbconfig is not RGBA; GLUT only uses RGBA
                   FBconfigs. */
                /* XXX Code could be exteneded to handle color
                   index FBconfigs, but seems a color index
                   window-renderable FBconfig would also be
                   advertised as an X visual. */
                mode->valid = 0;
              }
            } else {
              /* Fbconfig does not support window rendering;
                 not a valid FBconfig for GLUT windows. */
              mode->valid = 0;
            }
          } else {
            /* FBconfig ID is None (zero); not a valid
               FBconfig. */
            mode->valid = 0;
          }
        } else {
          /* FBconfig ID is None (zero); not a valid FBconfig. */
          mode->valid = 0;
        }
      } else {
        /* No SGIX_fbconfig GLX sever implementation support. */
        mode->valid = 0;
      }
#else
      /* No SGIX_fbconfig GLX extension API support. */
      mode->valid = 0;
#endif
    }
  }

  free(vlist);
  *nitems_return = n;
  return fbmodes;
}

static XVisualInfo *
findMatch(FrameBufferMode * fbmodes, int nfbmodes,
  Criterion * criteria, int ncriteria, void **fbc)
{
  FrameBufferMode *found;
  int *bestScore, *thisScore;
  int i, j, numok, result, worse, better;

  found = NULL;
  numok = 1;            /* "num" capability is indexed from 1,
                           not 0. */

  /* XXX alloca canidate. */
  bestScore = (int *) malloc(ncriteria * sizeof(int));
  if (!bestScore)
    __glutFatalError("out of memory.");
  for (j = 0; j < ncriteria; j++) {
    /* Very negative number. */
    bestScore[j] = -32768;
  }

  /* XXX alloca canidate. */
  thisScore = (int *) malloc(ncriteria * sizeof(int));
  if (!thisScore)
    __glutFatalError("out of memory.");

  for (i = 0; i < nfbmodes; i++) {
    if (fbmodes[i].valid) {
#ifdef TEST
#if !defined(_WIN32)
      if (verbose)
        printf("Visual 0x%x\n", fbmodes[i].vi->visualid);
#endif
#endif

      worse = 0;
      better = 0;

      for (j = 0; j < ncriteria; j++) {
        int cap, cvalue, fbvalue;

        cap = criteria[j].capability;
        cvalue = criteria[j].value;
        if (cap == NUM) {
          fbvalue = numok;
        } else {
          fbvalue = fbmodes[i].cap[cap];
        }
#ifdef TEST
        if (verbose)
          printf("  %s %s %d to %d\n",
            capstr[cap], compstr[criteria[j].comparison], cvalue, fbvalue);
#endif
        switch (criteria[j].comparison) {
        case EQ:
          result = cvalue == fbvalue;
          thisScore[j] = 1;
          break;
        case NEQ:
          result = cvalue != fbvalue;
          thisScore[j] = 1;
          break;
        case LT:
          result = fbvalue < cvalue;
          thisScore[j] = fbvalue - cvalue;
          break;
        case GT:
          result = fbvalue > cvalue;
          thisScore[j] = fbvalue - cvalue;
          break;
        case LTE:
          result = fbvalue <= cvalue;
          thisScore[j] = fbvalue - cvalue;
          break;
        case GTE:
          result = (fbvalue >= cvalue);
          thisScore[j] = fbvalue - cvalue;
          break;
        case MIN:
          result = fbvalue >= cvalue;
          thisScore[j] = cvalue - fbvalue;
          break;
        }

#ifdef TEST
        if (verbose)
          printf("                result=%d   score=%d   bestScore=%d\n", result, thisScore[j], bestScore[j]);
#endif

        if (result) {
          if (better || thisScore[j] > bestScore[j]) {
			better = 1;
          } else if (thisScore[j] == bestScore[j]) {
            /* Keep looking. */
          } else {
            goto nextFBM;
          }
        } else {
			if (cap == NUM) {
            worse = 1;
          } else {
            goto nextFBM;
          }
        }

      }

      if (better && !worse) {
        found = &fbmodes[i];
        for (j = 0; j < ncriteria; j++) {
          bestScore[j] = thisScore[j];
        }
      }
      numok++;

    nextFBM:;

    }
  }
  free(bestScore);
  free(thisScore);
  if (found) {
#if defined(GLX_VERSION_1_1) && defined(GLX_SGIX_fbconfig)
    *fbc = found->fbc;
#endif
	return found->vi;
  } else {
    return NULL;
  }
}

static int
parseCriteria(char *word, Criterion * criterion, int *mask,
  Bool * allowDoubleAsSingle)
{
  char *cstr, *vstr, *response;
  int comparator, value;
  int rgb, rgba, acc, acca, count, i;

  cstr = strpbrk(word, "=><!~");
  if (cstr) {
    switch (cstr[0]) {
    case '=':
      comparator = EQ;
      vstr = &cstr[1];
      break;
    case '~':
      comparator = MIN;
      vstr = &cstr[1];
      break;
    case '>':
      if (cstr[1] == '=') {
        comparator = GTE;
        vstr = &cstr[2];
      } else {
        comparator = GT;
        vstr = &cstr[1];
      }
      break;
    case '<':
      if (cstr[1] == '=') {
        comparator = LTE;
        vstr = &cstr[2];
      } else {
        comparator = LT;
        vstr = &cstr[1];
      }
      break;
    case '!':
      if (cstr[1] == '=') {
        comparator = NEQ;
        vstr = &cstr[2];
      } else {
        return -1;
      }
      break;
    default:
      return -1;
    }
    value = (int) strtol(vstr, &response, 0);
    if (response == vstr) {
      /* Not a valid number. */
      return -1;
    }
    *cstr = '\0';
  } else {
    comparator = NONE;
  }
  switch (word[0]) {
  case 'a':
    if (!strcmp(word, "alpha")) {
      criterion[0].capability = ALPHA_SIZE;
      if (comparator == NONE) {
        criterion[0].comparison = GTE;
        criterion[0].value = 1;
      } else {
        criterion[0].comparison = comparator;
        criterion[0].value = value;
      }
      *mask |= (1 << RGBA);
      *mask |= (1 << ALPHA_SIZE);
      *mask |= (1 << RGBA_MODE);
      return 1;
    }
    acca = !strcmp(word, "acca");
    acc = !strcmp(word, "acc");
    if (acc || acca) {
      criterion[0].capability = ACCUM_RED_SIZE;
      criterion[1].capability = ACCUM_GREEN_SIZE;
      criterion[2].capability = ACCUM_BLUE_SIZE;
      criterion[3].capability = ACCUM_ALPHA_SIZE;
      if (acca) {
        count = 4;
      } else {
        count = 3;
        criterion[3].comparison = MIN;
        criterion[3].value = 0;
      }
      if (comparator == NONE) {
        comparator = GTE;
        value = 8;
      }
      for (i = 0; i < count; i++) {
        criterion[i].comparison = comparator;
        criterion[i].value = value;
      }
      *mask |= (1 << ACCUM_RED_SIZE);
      return 4;
    }
    if (!strcmp(word, "auxbufs")) {
      criterion[0].capability = AUX_BUFFERS;
      if (comparator == NONE) {
        criterion[0].comparison = MIN;
        criterion[0].value = 1;
      } else {
        criterion[0].comparison = comparator;
        criterion[0].value = value;
      }
      *mask |= (1 << AUX_BUFFERS);
      return 1;
    }
    return -1;
  case 'b':
    if (!strcmp(word, "blue")) {
      criterion[0].capability = BLUE_SIZE;
      if (comparator == NONE) {
        criterion[0].comparison = GTE;
        criterion[0].value = 1;
      } else {
        criterion[0].comparison = comparator;
        criterion[0].value = value;
      }
      *mask |= (1 << RGBA);
      *mask |= (1 << RGBA_MODE);
      return 1;
    }
    if (!strcmp(word, "buffer")) {
      criterion[0].capability = BUFFER_SIZE;
      if (comparator == NONE) {
        criterion[0].comparison = GTE;
        criterion[0].value = 1;
      } else {
        criterion[0].comparison = comparator;
        criterion[0].value = value;
      }
      return 1;
    }
    return -1;
  case 'c':
    if (!strcmp(word, "conformant")) {
      criterion[0].capability = CONFORMANT;
      if (comparator == NONE) {
        criterion[0].comparison = EQ;
        criterion[0].value = 1;
      } else {
        criterion[0].comparison = comparator;
        criterion[0].value = value;
      }
      *mask |= (1 << CONFORMANT);
      return 1;
    }
    return -1;
  case 'd':
    if (!strcmp(word, "depth")) {
      criterion[0].capability = DEPTH_SIZE;
      if (comparator == NONE) {
        criterion[0].comparison = GTE;
        criterion[0].value = 12;
      } else {
        criterion[0].comparison = comparator;
        criterion[0].value = value;
      }
      *mask |= (1 << DEPTH_SIZE);
      return 1;
    }
    if (!strcmp(word, "double")) {
      criterion[0].capability = DOUBLEBUFFER;
      if (comparator == NONE) {
        criterion[0].comparison = EQ;
        criterion[0].value = 1;
      } else {
        criterion[0].comparison = comparator;
        criterion[0].value = value;
      }
      *mask |= (1 << DOUBLEBUFFER);
      return 1;
    }
    return -1;
  case 'g':
    if (!strcmp(word, "green")) {
      criterion[0].capability = GREEN_SIZE;
      if (comparator == NONE) {
        criterion[0].comparison = GTE;
        criterion[0].value = 1;
      } else {
        criterion[0].comparison = comparator;
        criterion[0].value = value;
      }
      *mask |= (1 << RGBA);
      *mask |= (1 << RGBA_MODE);
      return 1;
    }
    return -1;
  case 'i':
    if (!strcmp(word, "index")) {
      criterion[0].capability = RGBA;
      criterion[0].comparison = EQ;
      criterion[0].value = 0;
      *mask |= (1 << RGBA);
      *mask |= (1 << CI_MODE);
      criterion[1].capability = BUFFER_SIZE;
      if (comparator == NONE) {
        criterion[1].comparison = GTE;
        criterion[1].value = 1;
      } else {
        criterion[1].comparison = comparator;
        criterion[1].value = value;
      }
      return 2;
    }
    return -1;
  case 'l':
    if (!strcmp(word, "luminance")) {
      criterion[0].capability = RGBA;
      criterion[0].comparison = EQ;
      criterion[0].value = 1;

      criterion[1].capability = RED_SIZE;
      if (comparator == NONE) {
        criterion[1].comparison = GTE;
        criterion[1].value = 1;
      } else {
        criterion[1].comparison = comparator;
        criterion[1].value = value;
      }

      criterion[2].capability = GREEN_SIZE;
      criterion[2].comparison = EQ;
      criterion[2].value = 0;

      criterion[3].capability = BLUE_SIZE;
      criterion[3].comparison = EQ;
      criterion[3].value = 0;

      *mask |= (1 << RGBA);
      *mask |= (1 << RGBA_MODE);
      *mask |= (1 << LUMINANCE_MODE);
      return 4;
    }
    return -1;
  case 'n':
    if (!strcmp(word, "num")) {
      criterion[0].capability = NUM;
      if (comparator == NONE) {
        return -1;
      } else {
        criterion[0].comparison = comparator;
        criterion[0].value = value;
        return 1;
      }
    }
    return -1;
  case 'r':
    if (!strcmp(word, "red")) {
      criterion[0].capability = RED_SIZE;
      if (comparator == NONE) {
        criterion[0].comparison = GTE;
        criterion[0].value = 1;
      } else {
        criterion[0].comparison = comparator;
        criterion[0].value = value;
      }
      *mask |= (1 << RGBA);
      *mask |= (1 << RGBA_MODE);
      return 1;
    }
    rgba = !strcmp(word, "rgba");
    rgb = !strcmp(word, "rgb");
    if (rgb || rgba) {
      criterion[0].capability = RGBA;
      criterion[0].comparison = EQ;
      criterion[0].value = 1;

      criterion[1].capability = RED_SIZE;
      criterion[2].capability = GREEN_SIZE;
      criterion[3].capability = BLUE_SIZE;
      criterion[4].capability = ALPHA_SIZE;
      if (rgba) {
        count = 5;
      } else {
        count = 4;
        criterion[4].comparison = MIN;
        criterion[4].value = 0;
      }
      if (comparator == NONE) {
        comparator = GTE;
        value = 1;
      }
      for (i = 1; i < count; i++) {
        criterion[i].comparison = comparator;
        criterion[i].value = value;
      }
      *mask |= (1 << RGBA);
      *mask |= (1 << RGBA_MODE);
      return 5;
    }
    return -1;
  case 's':
    if (!strcmp(word, "stencil")) {
      criterion[0].capability = STENCIL_SIZE;
      if (comparator == NONE) {
        criterion[0].comparison = MIN;
        criterion[0].value = 1;
      } else {
        criterion[0].comparison = comparator;
        criterion[0].value = value;
      }
      *mask |= (1 << STENCIL_SIZE);
      return 1;
    }
    if (!strcmp(word, "single")) {
      criterion[0].capability = DOUBLEBUFFER;
      if (comparator == NONE) {
        criterion[0].comparison = EQ;
        criterion[0].value = 0;
        *allowDoubleAsSingle = True;
        *mask |= (1 << DOUBLEBUFFER);
        return 1;
      } else {
        return -1;
      }
    }
    if (!strcmp(word, "stereo")) {
      criterion[0].capability = STEREO;
      if (comparator == NONE) {
        criterion[0].comparison = EQ;
        criterion[0].value = 1;
      } else {
        criterion[0].comparison = comparator;
        criterion[0].value = value;
      }
      *mask |= (1 << STEREO);
      return 1;
    }
    if (!strcmp(word, "samples")) {
      criterion[0].capability = SAMPLES;
      if (comparator == NONE) {
        criterion[0].comparison = LTE;
        criterion[0].value = 4;
      } else {
        criterion[0].comparison = comparator;
        criterion[0].value = value;
      }
      *mask |= (1 << SAMPLES);
      return 1;
    }
    if (!strcmp(word, "slow")) {
      criterion[0].capability = SLOW;
      if (comparator == NONE) {
        /* Just "slow" means permit fast visuals, but accept
           slow ones in preference. Presumably the slow ones
           must be higher quality or something else desirable. */
        criterion[0].comparison = GTE;
        criterion[0].value = 0;
      } else {
        criterion[0].comparison = comparator;
        criterion[0].value = value;
      }
      *mask |= (1 << SLOW);
      return 1;
    }
    return -1;
#if defined(_WIN32)
  case 'w':
    if (!strcmp(word, "win32pfd")) {
      criterion[0].capability = XVISUAL;
      if (comparator == NONE) {
        return -1;
      } else {
        criterion[0].comparison = comparator;
        criterion[0].value = value;
        return 1;
      }
    }
    return -1;
#endif
#if !defined(_WIN32)
  case 'x':
    if (!strcmp(word, "xvisual")) {
      if (comparator == NONE) {
        return -1;
      } else {
        criterion[0].capability = XVISUAL;
        criterion[0].comparison = comparator;
        criterion[0].value = value;
        /* Set everything in "mask" so that no default criteria
           get used.  Assume the program really wants the
           xvisual specified. */
        *mask |= ~0;
        return 1;
      }
    }
    /* Be a little over-eager to fill in the comparison and
       value so we won't have to replicate the code after each
       string match. */
    if (comparator == NONE) {
      criterion[0].comparison = EQ;
      criterion[0].value = 1;
    } else {
      criterion[0].comparison = comparator;
      criterion[0].value = value;
    }

    if (!strcmp(word, "xstaticgray")) {
      criterion[0].capability = XSTATICGRAY;
      *mask |= (1 << XSTATICGRAY);  /* Indicates _any_ visual
                                       class selected. */
      return 1;
    }
    if (!strcmp(word, "xgrayscale")) {
      criterion[0].capability = XGRAYSCALE;
      *mask |= (1 << XSTATICGRAY);  /* Indicates _any_ visual
                                       class selected. */
      return 1;
    }
    if (!strcmp(word, "xstaticcolor")) {
      criterion[0].capability = XSTATICCOLOR;
      *mask |= (1 << XSTATICGRAY);  /* Indicates _any_ visual
                                       class selected. */
      return 1;
    }
    if (!strcmp(word, "xpseudocolor")) {
      criterion[0].capability = XPSEUDOCOLOR;
      *mask |= (1 << XSTATICGRAY);  /* Indicates _any_ visual
                                       class selected. */
      return 1;
    }
    if (!strcmp(word, "xtruecolor")) {
      criterion[0].capability = XTRUECOLOR;
      *mask |= (1 << XSTATICGRAY);  /* Indicates _any_ visual
                                       class selected. */
      return 1;
    }
    if (!strcmp(word, "xdirectcolor")) {
      criterion[0].capability = XDIRECTCOLOR;
      *mask |= (1 << XSTATICGRAY);  /* Indicates _any_ visual
                                       class selected. */
      return 1;
    }
    return -1;
#endif
  default:
    return -1;
  }
}

static Criterion *
parseModeString(char *mode, int *ncriteria, Bool * allowDoubleAsSingle,
  Criterion * requiredCriteria, int nRequired, int requiredMask)
{
  Criterion *criteria = NULL;
  int n, mask, parsed, i;
  char *copy, *word;

  *allowDoubleAsSingle = False;
  copy = __glutStrdup(mode);
  /* Attempt to estimate how many criteria entries should be
     needed. */
  n = 0;
  word = strtok(copy, " \t");
  while (word) {
    n++;
    word = strtok(NULL, " \t");
  }
  /* Overestimate by 4 times ("rgba" might add four criteria
     entries) plus add in possible defaults plus space for
     required criteria. */
  criteria = (Criterion *) malloc((4 * n + 30 + nRequired) * sizeof(Criterion));
  if (!criteria) {
    __glutFatalError("out of memory.");
  }

  /* Re-copy the copy of the mode string. */
  strcpy(copy, mode);

  /* First add the required criteria (these match at the
     highest priority). Typically these will be used to force a
     specific level (layer), transparency, and/or visual type. */
  mask = requiredMask;
  for (i = 0; i < nRequired; i++) {
    criteria[i] = requiredCriteria[i];
  }
  n = nRequired;

  word = strtok(copy, " \t");
  while (word) {
    parsed = parseCriteria(word, &criteria[n], &mask, allowDoubleAsSingle);
    if (parsed >= 0) {
      n += parsed;
    } else {
      __glutWarning("Unrecognized display string word: %s (ignoring)\n", word);
    }
    word = strtok(NULL, " \t");
  }

#if defined(GLX_VERSION_1_1) && defined(GLX_SGIS_multisample)
  if (__glutIsSupportedByGLX("GLX_SGIS_multisample")) {
    if (!(mask & (1 << SAMPLES))) {
      criteria[n].capability = SAMPLES;
      criteria[n].comparison = EQ;
      criteria[n].value = 0;
      n++;
    } else {
      /* Multisample visuals are marked nonconformant.  If
         multisampling was requeste and no conformant
         preference was set, assume that we will settle for a
         non-conformant visual to get multisampling. */
      if (!(mask & (1 << CONFORMANT))) {
        criteria[n].capability = CONFORMANT;
        criteria[n].comparison = MIN;
        criteria[n].value = 0;
        n++;
        mask |= (1 << CONFORMANT);
      }
    }
  }
#endif
#if defined(GLX_VERSION_1_1) && defined(GLX_EXT_visual_info)
  if (__glutIsSupportedByGLX("GLX_EXT_visual_info")) {
    if (!(mask & (1 << TRANSPARENT))) {
      criteria[n].capability = TRANSPARENT;
      criteria[n].comparison = EQ;
      criteria[n].value = 0;
      n++;
    }
  }
#endif
#if defined(GLX_VERSION_1_1) && defined(GLX_EXT_visual_rating)
  if (__glutIsSupportedByGLX("GLX_EXT_visual_rating")) {
    if (!(mask & (1 << SLOW))) {
      criteria[n].capability = SLOW;
      criteria[n].comparison = EQ;
      criteria[n].value = 0;
      n++;
    }
    if (!(mask & (1 << CONFORMANT))) {
      criteria[n].capability = CONFORMANT;
      criteria[n].comparison = EQ;
      criteria[n].value = 1;
      n++;
    }
  }
#endif
  if (!(mask & (1 << ACCUM_RED_SIZE))) {
    criteria[n].capability = ACCUM_RED_SIZE;
    criteria[n].comparison = MIN;
    criteria[n].value = 0;
    criteria[n + 1].capability = ACCUM_GREEN_SIZE;
    criteria[n + 1].comparison = MIN;
    criteria[n + 1].value = 0;
    criteria[n + 2].capability = ACCUM_BLUE_SIZE;
    criteria[n + 2].comparison = MIN;
    criteria[n + 2].value = 0;
    criteria[n + 3].capability = ACCUM_ALPHA_SIZE;
    criteria[n + 3].comparison = MIN;
    criteria[n + 3].value = 0;
    n += 4;
  }
  if (!(mask & (1 << AUX_BUFFERS))) {
    criteria[n].capability = AUX_BUFFERS;
    criteria[n].comparison = MIN;
    criteria[n].value = 0;
    n++;
  }
  if (!(mask & (1 << RGBA))) {
    criteria[n].capability = RGBA;
    criteria[n].comparison = EQ;
    criteria[n].value = 1;
    criteria[n + 1].capability = RED_SIZE;
    criteria[n + 1].comparison = GTE;
    criteria[n + 1].value = 1;
    criteria[n + 2].capability = GREEN_SIZE;
    criteria[n + 2].comparison = GTE;
    criteria[n + 2].value = 1;
    criteria[n + 3].capability = BLUE_SIZE;
    criteria[n + 3].comparison = GTE;
    criteria[n + 3].value = 1;
    criteria[n + 4].capability = ALPHA_SIZE;
    criteria[n + 4].comparison = MIN;
    criteria[n + 4].value = 0;
    n += 5;
    mask |= (1 << RGBA_MODE);
  }
#if !defined(_WIN32)
  if (!(mask & (1 << XSTATICGRAY))) {
    assert(isMesaGLX != -1);
    if ((mask & (1 << RGBA_MODE)) && !isMesaGLX) {
      /* Normally, request an RGBA mode visual be TrueColor,
         except in the case of Mesa where we trust Mesa (and
         other code in GLUT) to handle any type of RGBA visual
         reasonably. */
      if (mask & (1 << LUMINANCE_MODE)) {
	/* If RGBA luminance was requested, actually go for
	   a StaticGray visual. */
        criteria[n].capability = XSTATICGRAY;
      } else {
        criteria[n].capability = XTRUECOLOR;
      }
      criteria[n].value = 1;
      criteria[n].comparison = EQ;

      n++;
    }
    if (mask & (1 << CI_MODE)) {
      criteria[n].capability = XPSEUDOCOLOR;
      criteria[n].value = 1;
      criteria[n].comparison = EQ;
      n++;
    }
  }
#endif
  if (!(mask & (1 << STEREO))) {
    criteria[n].capability = STEREO;
    criteria[n].comparison = EQ;
    criteria[n].value = 0;
    n++;
  }
  if (!(mask & (1 << DOUBLEBUFFER))) {
    criteria[n].capability = DOUBLEBUFFER;
    criteria[n].comparison = EQ;
    criteria[n].value = 0;
    *allowDoubleAsSingle = True;
    n++;
  }
  if (!(mask & (1 << DEPTH_SIZE))) {
    criteria[n].capability = DEPTH_SIZE;
    criteria[n].comparison = MIN;
    criteria[n].value = 0;
    n++;
  }
  if (!(mask & (1 << STENCIL_SIZE))) {
    criteria[n].capability = STENCIL_SIZE;
    criteria[n].comparison = MIN;
    criteria[n].value = 0;
    n++;
  }
  if (!(mask & (1 << LEVEL))) {
    criteria[n].capability = LEVEL;
    criteria[n].comparison = EQ;
    criteria[n].value = 0;
    n++;
  }
  if (n) {
    /* Since over-estimated the size needed; squeeze it down to
       reality. */
    criteria = (Criterion *) realloc(criteria, n * sizeof(Criterion));
    if (!criteria) {
      /* Should never happen since should be shrinking down! */
      __glutFatalError("out of memory.");
    }
  } else {
    /* For portability, avoid "realloc(ptr,0)" call. */
    free(criteria);
    criteria = NULL;
  }

  free(copy);
  *ncriteria = n;
  return criteria;
}

static FrameBufferMode *fbmodes = NULL;
static int nfbmodes = 0;

static XVisualInfo *
getVisualInfoFromString(char *string, Bool * treatAsSingle,
  Criterion * requiredCriteria, int nRequired, int requiredMask, void **fbc)
{
  Criterion *criteria;
  XVisualInfo *visinfo;
  Bool allowDoubleAsSingle;
  int ncriteria, i;

  /* In WIN32, after changing display settings, the visuals might change.
  (e.g. if entering game mode with a different bitdepth!)
  Therefore, reload the visuals each time they are queried. */
#ifdef WIN32
  if (fbmodes) {
	  free(fbmodes);
	  fbmodes = NULL;
	  nfbmodes = 0;
  }
#endif
  if (!fbmodes) {
    fbmodes = loadVisuals(&nfbmodes);
  }
  criteria = parseModeString(string, &ncriteria,
    &allowDoubleAsSingle, requiredCriteria, nRequired, requiredMask);
  if (criteria == NULL) {
    __glutWarning("failed to parse mode string");
    return NULL;
  }
#ifdef TEST
  printCriteria(criteria, ncriteria);
#endif
  visinfo = findMatch(fbmodes, nfbmodes, criteria, ncriteria, fbc);
  if (visinfo) {
    *treatAsSingle = 0;
  } else {
    if (allowDoubleAsSingle) {
      /* Rewrite criteria so that we now look for a double
         buffered visual which will then get treated as a
         single buffered visual. */
      for (i = 0; i < ncriteria; i++) {
        if (criteria[i].capability == DOUBLEBUFFER
          && criteria[i].comparison == EQ
          && criteria[i].value == 0) {
          criteria[i].value = 1;
        }
      }
      visinfo = findMatch(fbmodes, nfbmodes, criteria, ncriteria, fbc);
      if (visinfo) {
        *treatAsSingle = 1;
      }
    }
  }
  free(criteria);

  if (visinfo) {
#if defined(_WIN32)
    /* We could have a valid pixel format for drawing to a
       bitmap. However, we don't want to draw into a bitmap, we
       need one that can be used with a window, so make sure
       that this is true. */
    if (!(visinfo->dwFlags & PFD_DRAW_TO_WINDOW))
      return NULL;
#endif
    return visinfo;
  } else {
    return NULL;
  }
}

/* CENTRY */
void APIENTRY
glutInitDisplayString(const char *string)
{
#ifdef _WIN32
  XHDC = GetDC(GetDesktopWindow());
#endif

  __glutDetermineVisualFromString = getVisualInfoFromString;
  if (__glutDisplayString) {
    free(__glutDisplayString);
  }
  if (string) {
    __glutDisplayString = __glutStrdup(string);
    if (!__glutDisplayString)
      __glutFatalError("out of memory.");
  } else {
    __glutDisplayString = NULL;
  }
}
/* ENDCENTRY */

#ifdef TEST

Criterion requiredWindowCriteria[] =
{
  {LEVEL, EQ, 0},
  {TRANSPARENT, EQ, 0}
};
int numRequiredWindowCriteria = sizeof(requiredWindowCriteria) / sizeof(Criterion);
int requiredWindowCriteriaMask = (1 << LEVEL) | (1 << TRANSPARENT);

Criterion requiredOverlayCriteria[] =
{
  {LEVEL, EQ, 1},
  {TRANSPARENT, EQ, 1},
  {XPSEUDOCOLOR, EQ, 1},
  {RGBA, EQ, 0},
  {BUFFER_SIZE, GTE, 1}
};
int numRequiredOverlayCriteria = sizeof(requiredOverlayCriteria) / sizeof(Criterion);
int requiredOverlayCriteriaMask =
(1 << LEVEL) | (1 << TRANSPARENT) | (1 << XSTATICGRAY) | (1 << RGBA) | (1 << CI_MODE);

int
main(int argc, char **argv)
{
  Display *dpy;
  XVisualInfo *vinfo;
  Bool treatAsSingle;
  char *str, buffer[1024];
  int tty = isatty(fileno(stdin));
  int overlay = 0, showconfig = 0;
#if defined(GLX_VERSION_1_1) && defined(GLX_SGIX_fbconfig)
  GLXFBConfigSGIX fbc;
#else
  void *fbc;
#endif

#if !defined(_WIN32)
  dpy = XOpenDisplay(NULL);
  if (dpy == NULL) {
    printf("Could not connect to X server\n");
    exit(1);
  }
  __glutDisplay = dpy;
  __glutScreen = DefaultScreen(__glutDisplay);
#endif
  while (!feof(stdin)) {
    if (tty)
      printf("dstr> ");
    str = gets(buffer);
    if (str) {
      printf("\n");
      if (!strcmp("v", str)) {
        verbose = 1 - verbose;
        printf("verbose = %d\n\n", verbose);
      } else if (!strcmp("s", str)) {
        showconfig = 1 - showconfig;
        printf("showconfig = %d\n\n", showconfig);
      } else if (!strcmp("o", str)) {
        overlay = 1 - overlay;
        printf("overlay = %d\n\n", overlay);
      } else {
        if (overlay) {
          vinfo = getVisualInfoFromString(str, &treatAsSingle,
            requiredOverlayCriteria, numRequiredOverlayCriteria, requiredOverlayCriteriaMask, (void**) &fbc);
        } else {
          vinfo = getVisualInfoFromString(str, &treatAsSingle,
            requiredWindowCriteria, numRequiredWindowCriteria, requiredWindowCriteriaMask, (void**) &fbc);
        }
        if (vinfo) {
          printf("\n");
          if (!tty)
            printf("Display string: %s", str);
#ifdef _WIN32
          printf("Visual = 0x%x\n", 0);
#else
          printf("Visual = 0x%x%s\n", vinfo->visualid, fbc ? " (needs FBC)" : "");
#endif
          if (treatAsSingle) {
            printf("Treat as SINGLE.\n");
          }
          if (showconfig) {
            int glxCapable, bufferSize, level, renderType, doubleBuffer,
              stereo, auxBuffers, redSize, greenSize, blueSize,
              alphaSize, depthSize, stencilSize, acRedSize, acGreenSize,
              acBlueSize, acAlphaSize;

            glXGetConfig(dpy, vinfo, GLX_BUFFER_SIZE, &bufferSize);
            glXGetConfig(dpy, vinfo, GLX_LEVEL, &level);
            glXGetConfig(dpy, vinfo, GLX_RGBA, &renderType);
            glXGetConfig(dpy, vinfo, GLX_DOUBLEBUFFER, &doubleBuffer);
            glXGetConfig(dpy, vinfo, GLX_STEREO, &stereo);
            glXGetConfig(dpy, vinfo, GLX_AUX_BUFFERS, &auxBuffers);
            glXGetConfig(dpy, vinfo, GLX_RED_SIZE, &redSize);
            glXGetConfig(dpy, vinfo, GLX_GREEN_SIZE, &greenSize);
            glXGetConfig(dpy, vinfo, GLX_BLUE_SIZE, &blueSize);
            glXGetConfig(dpy, vinfo, GLX_ALPHA_SIZE, &alphaSize);
            glXGetConfig(dpy, vinfo, GLX_DEPTH_SIZE, &depthSize);
            glXGetConfig(dpy, vinfo, GLX_STENCIL_SIZE, &stencilSize);
            glXGetConfig(dpy, vinfo, GLX_ACCUM_RED_SIZE, &acRedSize);
            glXGetConfig(dpy, vinfo, GLX_ACCUM_GREEN_SIZE, &acGreenSize);
            glXGetConfig(dpy, vinfo, GLX_ACCUM_BLUE_SIZE, &acBlueSize);
            glXGetConfig(dpy, vinfo, GLX_ACCUM_ALPHA_SIZE, &acAlphaSize);
            printf("RGBA = (%d, %d, %d, %d)\n", redSize, greenSize, blueSize, alphaSize);
            printf("acc  = (%d, %d, %d, %d)\n", acRedSize, acGreenSize, acBlueSize, acAlphaSize);
            printf("db   = %d\n", doubleBuffer);
            printf("str  = %d\n", stereo);
            printf("aux  = %d\n", auxBuffers);
            printf("lvl  = %d\n", level);
            printf("buf  = %d\n", bufferSize);
            printf("rgba = %d\n", renderType);
            printf("z    = %d\n", depthSize);
            printf("s    = %d\n", stencilSize);
          }
        } else {
          printf("\n");
          printf("No match.\n");
        }
        printf("\n");
      }
    }
  }
  printf("\n");
  return 0;
}
#endif
