#if !defined (__RSVG_RSVG_H_INSIDE__) && !defined (RSVG_COMPILATION)
#warning "Including <librsvg/librsvg-features.h> directly is deprecated."
#endif

#ifndef LIBRSVG_FEATURES_H
#define LIBRSVG_FEATURES_H

#define LIBRSVG_MAJOR_VERSION (2)
#define LIBRSVG_MINOR_VERSION (40)
#define LIBRSVG_MICRO_VERSION (9)
#define LIBRSVG_VERSION "2.40.9"

#define LIBRSVG_CHECK_VERSION(major,minor,micro) \
  (LIBRSVG_MAJOR_VERSION > (major) || \
   (LIBRSVG_MAJOR_VERSION == (major) && LIBRSVG_MINOR_VERSION > (minor)) || \
   (LIBRSVG_MAJOR_VERSION == (major) && LIBRSVG_MINOR_VERSION == (minor) && LIBRSVG_MICRO_VERSION >= (micro)))

#ifndef __GI_SCANNER__
#define LIBRSVG_HAVE_SVGZ  (TRUE)
#define LIBRSVG_HAVE_CSS   (TRUE)

#define LIBRSVG_CHECK_FEATURE(FEATURE) (defined(LIBRSVG_HAVE_##FEATURE) && LIBRSVG_HAVE_##FEATURE)
#endif

extern const guint librsvg_major_version, librsvg_minor_version, librsvg_micro_version;
extern const char librsvg_version[];

#endif
