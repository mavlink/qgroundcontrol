/*
 * Copyright 2012, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ELFUTILS_ERROR_H
#define ELFUTILS_ERROR_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

static inline void __attribute__((noreturn))
error(int status, int errnum, const char *fmt, ...)
{
        va_list lst;
        va_start(lst, fmt);
        vfprintf(stderr, fmt, lst);
        fprintf(stderr, "error %d: %s\n", errnum, strerror(errno));
        va_end(lst);
        exit(status);
}

#endif /* ELFUTILS_ERROR_H */
