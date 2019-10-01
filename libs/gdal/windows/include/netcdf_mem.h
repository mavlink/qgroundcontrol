/*! \file netcdf_mem.h
 *
 * Main header file for in-memory (diskless) functionality.
 *
 * Copyright 2010 University Corporation for Atmospheric
 * Research/Unidata. See COPYRIGHT file for more info.
 *
 * See \ref copyright file for more info.
 *
 */

#ifndef NETCDF_MEM_H
#define NETCDF_MEM_H 1

typedef struct NC_memio {
    size_t size;
    void* memory;
    int flags;
#define NC_MEMIO_LOCKED 1    /* Do not try to realloc or free provided memory */
} NC_memio;

#if defined(__cplusplus)
extern "C" {
#endif

/* Treate a memory block as a file; read-only */
EXTERNL int nc_open_mem(const char* path, int mode, size_t size, void* memory, int* ncidp);

EXTERNL int nc_create_mem(const char* path, int mode, size_t initialsize, int* ncidp);

/* Alternative to nc_open_mem with extended capabilites
   See docs/inmemory.md
 */
EXTERNL int nc_open_memio(const char* path, int mode, NC_memio* info, int* ncidp);

/* Close memory file and return the final memory state */
EXTERNL int nc_close_memio(int ncid, NC_memio* info);

#if defined(__cplusplus)
}
#endif

#endif /* NETCDF_MEM_H */
