/*
   This is the OpenCL Utility library. It defines a set of functions and C
   macros to make the host side of OpenCL programming less tedious

   Copyright (C) 2011 Giuseppe Bilotta

   See LEGAL for license information and other legalese
 */

/* Private macros and internal functions */

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <errno.h>

#include <string.h>
#include <strings.h>

/* number of elements in array */
#define ARRAY_SIZE(ar) sizeof(ar)/sizeof(*ar)

/* allocate num elements of type type */
#define NALLOC(num, type) (type *)calloc(num, sizeof(type));

/* An auxiliary macro that returns what after setting *errcode_ret to error unless errcode_ret is NULL */
#define _RETURN(error, what) \
	do { \
		if (errcode_ret) \
			*errcode_ret = error; \
		return (what); \
	} while(0)
#define RETURN_ON_ERROR if (error) _RETURN(error, NULL);

/* Internal: prepare the cache */
static
cl_int
_cluInitPlatformCache(void);

/* Internal: find platform info by id, assuming the cache is initialized */
static
clu_platform *
_cluGetPlatformByID(cl_platform_id id);

/* Internal: free the resources occupied by a clu_platform */
static
void
_cluReleasePlatformInfo(clu_platform *pinfo);

/* Internal: load the actual platform info */
static
cl_int
_cluGetPlatformInfo(clu_platform *pinfo);

/* Internal: load the actual device info */
static
cl_int
_cluGetDeviceInfo(clu_device *dinfo);

/* Internal: initialize device cache for a platform */
static
cl_int
_cluInitDeviceCache(clu_platform *pinfo, cl_device_id *devs, cl_uint count);

/* Internal: get the device IDs, and cache them if they are all */
static
cl_int
_cluGetDeviceIDs(
	clu_platform *pinfo,
	cl_device_type type,
	cl_device_id **devices,
	cl_uint *num_devices);

/* Internal: reads a file into memory */
static
char*
_cluReadFile(const char *fname, size_t *fsize);
