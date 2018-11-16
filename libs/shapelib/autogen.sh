#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir="$(dirname "$(readlink -f $0)")"

(test -f $srcdir/configure.ac) || {
  echo -n "**Error**: Directory "\`$srcdir\'" does not look like the"
  echo " top-level package directory"
  echo
  exit 1
}

(libtool --version) < /dev/null > /dev/null 2>&1 || {
  echo "**Error**: You must have \`libtool' installed."
  echo "You can get it from: ftp://ftp.gnu.org/pub/gnu/"
  echo
  exit 1
}

(autoreconf --version) < /dev/null > /dev/null 2>&1 || {
  echo "**Error**: You must have \`autoreconf' installed."
  echo "Download the appropriate package for your distribution,"
  echo "or get the source tarball at ftp://ftp.gnu.org/pub/gnu/"
  echo
  exit 1
}


(
  cd "$srcdir"
  echo "Running autoreconf..."
  autoreconf -fiv
)


if test x$NOCONFIGURE = x; then
  echo Running $srcdir/configure "$@" ...
  $srcdir/configure "$@" \
  && echo Now type \`make\' to compile. || exit 1
else
  echo Skipping configure process.
fi
