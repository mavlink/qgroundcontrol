/* lvconfig.h
 *
 * This is a generated file.  Please modify 'configure.ac'
 */

#ifndef __LV_CONFIG_H__
#define __LV_CONFIG_H__

#ifndef __cplusplus
# define LV_HAVE_ISO_VARARGS	(1)
#endif

/* gcc-2.95.x supports both gnu style and ISO varargs, but if -ansi
 * is passed ISO vararg support is turned off, and there is no work
 * around to turn it on, so we unconditionally turn it off.
 */
#if __GNUC__ == 2 && __GNUC_MINOR__ == 95
#  undef LV_HAVE_ISO_VARARGS
#endif

#define LV_HAVE_GNUC_VARARGS	(1)

#define VISUAL_BIG_ENDIAN	(0)
#define VISUAL_LITTLE_ENDIAN	(1)

typedef unsigned int visual_size_t;
#define VISUAL_SIZE_T_FORMAT	"u"

#define VISUAL_ARCH_UNKNOWN

#define VISUAL_OS_LINUX

#define VISUAL_HAVE_THREADS

#define VISUAL_THREAD_MODEL_POSIX


#endif /* LV_CONFIG_H */

