/*
   This is the OpenCL Utility library. It defines a set of functions and C
   macros to make the host side of OpenCL programming less tedious

   Copyright (C) 2011 Giuseppe Bilotta

   See LEGAL for license information and other legalese
 */

#include "clu.h"

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <errno.h>

#include <string.h>
#include <strings.h>

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

static
const char *
error_strings[] =
{
	"no error",
	"device not found",
	"device not available",
	"compiler not available",
	"memory object allocation failure",
	"out of resources",
	"out of host memory",
	"profiling information not available",
	"memory copy overlap",
	"image format mismatch",
	"image format not supported",
	"build program failure",
	"map failure",
	"misaligned sub-buffer offset",
	"exec status error for events in wait list",
	NULL, NULL, NULL, NULL, NULL, /* 15 to 19 */
	NULL, NULL, NULL, NULL, NULL, /* 20 to 24 */
	NULL, NULL, NULL, NULL, NULL, /* 24 to 29 */
	"invalid value",
	"invalid device type",
	"invalid platform",
	"invalid device",
	"invalid context",
	"invalid queue properties",
	"invalid command queue",
	"invalid host pointer",
	"invalid memory object",
	"invalid image format descriptor",
	"invalid image size",
	"invalid sampler",
	"invalid binary",
	"invalid build options",
	"invalid program",
	"invalid program executable",
	"invalid kernel name",
	"invalid kernel definition",
	"invalid kernel",
	"invalid argument index",
	"invalid argument value",
	"invalid argument size",
	"invalid kernel arguments",
	"invalid work dimension",
	"invalid workgroup size",
	"invalid work-item size",
	"invalid global offset",
	"invalid event wait list",
	"invalid event",
	"invalid operation",
	"invalid GL object",
	"invalid buffer size",
	"invalid MIP level",
	"invalid global work size",
	"invalid property",
	"unknown error" /* 65 */
};

#define ARRAY_SIZE(ar) sizeof(ar)/sizeof(*ar)

static const size_t error_string_count = ARRAY_SIZE(error_strings);
static const size_t unknown_error_index = error_string_count - 1;

const char *
cluGetErrorString(cl_int error)
{
	size_t index = -error;
	if (index > unknown_error_index)
		index = unknown_error_index;
	if (error_strings[index])
		return error_strings[index];
	else
		return error_strings[unknown_error_index];
}

/* TODO thread safety */
#define EMPTY_CACHE ((cl_uint)-1)
/* number of cached platform */
static cl_uint _num_platforms = EMPTY_CACHE;
/* cached platform information */
static clu_platform *_platforms;

/* Internal: prepare the cache */
static
cl_int
_cluInitPlatformCache(void)
{
	if (_num_platforms == EMPTY_CACHE) {
		cl_platform_id *plats;
		return cluGetPlatformIDs(&plats, &_num_platforms);
	} else
		return CL_SUCCESS;
}

/* Internal: find platform info by id, assuming the cache is initialized */
static
clu_platform *
_cluGetPlatformByID(cl_platform_id id)
{
	for (size_t i=0; i < _num_platforms; ++i)
		if (_platforms[i].id == id)
			return _platforms + i;
	return NULL;
}

cl_int
cluGetPlatformIDs(
	cl_platform_id **platforms,
	cl_uint *num_platforms)
{
	cl_int error = CL_SUCCESS;

	if (!num_platforms)
		return CL_INVALID_VALUE;

	if (_num_platforms < EMPTY_CACHE) {
		*num_platforms = _num_platforms;
	} else {
		error = clGetPlatformIDs(0, NULL, num_platforms);
		if (error == CL_SUCCESS)
			_num_platforms = *num_platforms;
		else
			return error;
	}

	if (platforms == NULL)
		return error;

	if (_num_platforms == 0) {
		*platforms = NULL;
		return error;
	}

	*platforms = NALLOC(_num_platforms, cl_platform_id);
	if (*platforms == NULL)
		return CL_OUT_OF_HOST_MEMORY;

	if (_platforms) {
		for (size_t i=0; i < _num_platforms; ++i)
			(*platforms)[i] = _platforms[i].id;
		return error;
	}

	_platforms = NALLOC(_num_platforms, clu_platform);
	if (_platforms == NULL)
		return CL_OUT_OF_HOST_MEMORY;

	error = clGetPlatformIDs(*num_platforms, *platforms, num_platforms);
	if (error != CL_SUCCESS) {
		free(_platforms);
		_platforms = NULL;
		free(*platforms);
		*platforms = NULL;
		return error;
	}

	for (size_t i=0; i < _num_platforms; ++i)
		_platforms[i].id = (*platforms)[i];

	return error;
}

/* Internal: free the resources occupied by a clu_platform */
static
void
_cluReleasePlatformInfo(clu_platform *pinfo)
{
#define FREE(field) free((void *)pinfo->field); pinfo->field = NULL
	FREE(extensions);
	FREE(vendor);
	FREE(name);
	FREE(version);
	FREE(profile);
#undef FREE
}

/* Internal: load the actual platform info */
static
cl_int
_cluGetPlatformInfo(clu_platform *pinfo)
{
	if (pinfo->has_info)
		return CL_SUCCESS;

	cl_int error = CL_SUCCESS;
	size_t data_size;

#define GET_INFO(def, field) \
	error = clGetPlatformInfo(pinfo->id, CL_PLATFORM_##def, 0, NULL, &data_size); \
	if (error) return error; \
	pinfo->field = (char *)malloc(data_size); \
	if (pinfo->field == NULL) return CL_OUT_OF_HOST_MEMORY; \
	error = clGetPlatformInfo(pinfo->id, CL_PLATFORM_##def, data_size, (void *)pinfo->field, &data_size); \
	if (error) return error;

	GET_INFO(PROFILE, profile);
	GET_INFO(VERSION, version);
	GET_INFO(NAME, name);
	GET_INFO(VENDOR, vendor);
	GET_INFO(EXTENSIONS, extensions);
#undef GET_INFO
	pinfo->has_info = CL_TRUE;

	return error;
}

cl_int
cluGetPlatforms(clu_pfmptr **platforms, cl_uint *num_platforms)
{
	if (!num_platforms)
		return CL_INVALID_VALUE;

	cl_int error = _cluInitPlatformCache();
	if (error) return error;

	for (size_t i=0; i < _num_platforms; ++i) {
		clu_platform *curr = _platforms+i;
		error = _cluGetPlatformInfo(curr);
		if (error) return error;
	}

	*num_platforms = _num_platforms;

	if (!platforms)
		return CL_SUCCESS;

	clu_pfmptr *plats = NALLOC(_num_platforms, clu_pfmptr);
	if (plats == NULL)
		return CL_OUT_OF_HOST_MEMORY;

	for (cl_uint i=0; i < _num_platforms; ++i)
		plats[i] = _platforms +i;

	*platforms = plats;

	return CL_SUCCESS;
}

clu_pfmptr
cluGetPlatformByID(cl_platform_id id, cl_int *errcode_ret)
{
	cl_int error = _cluInitPlatformCache();
	RETURN_ON_ERROR;

	for (size_t i=0; i < _num_platforms; ++i) {
		clu_platform *curr = _platforms+i;
		if (_platforms[i].id == id) {
			error = _cluGetPlatformInfo(curr);
			RETURN_ON_ERROR
			_RETURN(error, _platforms + i);
		}
	}

	_RETURN(CL_INVALID_PLATFORM, NULL);
}

clu_pfmptr
cluGetPlatformByName(const char* name, cl_int *errcode_ret)
{
	cl_int error = _cluInitPlatformCache();
	RETURN_ON_ERROR;

	if (name == NULL || name[0] == '\0') {
		_RETURN(error, _platforms);
	}

	size_t len = strlen(name);

	for (size_t i=0; i < _num_platforms; ++i) {
		clu_platform *curr = _platforms+i;
		/* TODO locale */
		if (strncasecmp(name, curr->name, len) == 0) {
			error = _cluGetPlatformInfo(curr);
			RETURN_ON_ERROR;
			_RETURN(error, curr);
		}
	}

	_RETURN(CL_INVALID_PLATFORM, NULL);
}

clu_pfmptr
cluGetPlatformByNumber(cl_uint num, cl_int *errcode_ret)
{
	cl_int error = _cluInitPlatformCache();
	RETURN_ON_ERROR;

	if (num < _num_platforms) {
		clu_platform *curr = _platforms+num;
		error = _cluGetPlatformInfo(curr);
		RETURN_ON_ERROR;
		_RETURN(error, curr);
	}

	_RETURN(CL_INVALID_VALUE, NULL);
}

clu_pfmptr
cluGetPlatformFromArg(const char* name_or_num, cl_int *errcode_ret)
{
	char *end;
	cl_ulong num = strtoul(name_or_num, &end, 0);
	if (*end != '\0') /* the argument was not a number */
		return cluGetPlatformByName(name_or_num, errcode_ret);
	return cluGetPlatformByNumber(num, errcode_ret);
}

static
const char*
device_type_string[] =
{
	"no device",
	"default",
	"CPU",
	"default CPU",
	"GPU",
	"default GPU",
	"CPU/GPU",
	"default CPU/GPU",
	"accelerator",
	"default accelerator",
	"CPU/accelerator",
	"default CPU/accelerator",
	"GPU/accelerator",
	"default GPU/accelerator",
	"CPU/GPU/accelerator",
	"default CPU/GPU/accelerator",
	"unkown"
};

static const size_t device_type_string_count = ARRAY_SIZE(device_type_string);
static const size_t unknown_device_type_index = device_type_string_count - 1;

const char *
cluGetDeviceTypeString(cl_device_type type)
{
	if (type > unknown_device_type_index)
		type = unknown_device_type_index;
	return device_type_string[type];
}

/* Internal: load the actual device info */
static
cl_int
_cluGetDeviceInfo(clu_device *dinfo)
{
	if (dinfo->has_info)
		return CL_SUCCESS;

	cl_int error = CL_SUCCESS;
	size_t data_size;

#define GET_INFO(def, field) \
	error = clGetDeviceInfo(dinfo->id, CL_DEVICE_##def, \
			sizeof(dinfo->field), &dinfo->field, &data_size); \
	if (error) return error;

#define GET_STRING_(def, field) \
	error = clGetDeviceInfo(dinfo->id, CL_##def, \
			0, NULL, &data_size); \
	if (error) return error; \
	dinfo->field = (char *)malloc(data_size); \
	if (dinfo->field == NULL) return CL_OUT_OF_HOST_MEMORY; \
	error = clGetDeviceInfo(dinfo->id, CL_##def, \
			data_size, (void *)dinfo->field, &data_size); \
	if (error) return error;

#define GET_STRING(def, field) GET_STRING_(DEVICE_##def, field)

	GET_INFO(TYPE, type);
	GET_INFO(VENDOR_ID, vendor_id);
	GET_INFO(MAX_COMPUTE_UNITS, max_compute_units);
	GET_INFO(MAX_WORK_ITEM_DIMENSIONS, max_work_item_dimensions);
	GET_INFO(MAX_WORK_GROUP_SIZE, max_work_group_size);

	dinfo->max_work_item_sizes = NALLOC(dinfo->max_work_item_dimensions, size_t);
	if (dinfo->max_work_item_sizes == NULL) return CL_OUT_OF_HOST_MEMORY;
	data_size = sizeof(size_t)*dinfo->max_work_item_dimensions;
	error = clGetDeviceInfo(dinfo->id, CL_DEVICE_MAX_WORK_ITEM_SIZES,
			data_size, (void*)dinfo->max_work_item_sizes, &data_size);
	if (error) return error;

	GET_INFO(PREFERRED_VECTOR_WIDTH_CHAR, preferred_vector_width_char);
	GET_INFO(PREFERRED_VECTOR_WIDTH_SHORT, preferred_vector_width_short);
	GET_INFO(PREFERRED_VECTOR_WIDTH_INT, preferred_vector_width_int);
	GET_INFO(PREFERRED_VECTOR_WIDTH_LONG, preferred_vector_width_long);
	GET_INFO(PREFERRED_VECTOR_WIDTH_FLOAT, preferred_vector_width_float);
	GET_INFO(PREFERRED_VECTOR_WIDTH_DOUBLE, preferred_vector_width_double);
	GET_INFO(MAX_CLOCK_FREQUENCY, max_clock_frequency);
	GET_INFO(ADDRESS_BITS, address_bits);
	GET_INFO(MAX_READ_IMAGE_ARGS, max_read_image_args);
	GET_INFO(MAX_WRITE_IMAGE_ARGS, max_write_image_args);
	GET_INFO(MAX_MEM_ALLOC_SIZE, max_mem_alloc_size);
	GET_INFO(IMAGE2D_MAX_WIDTH, image2d_max_width);
	GET_INFO(IMAGE2D_MAX_HEIGHT, image2d_max_height);
	GET_INFO(IMAGE3D_MAX_WIDTH, image3d_max_width);
	GET_INFO(IMAGE3D_MAX_HEIGHT, image3d_max_height);
	GET_INFO(IMAGE3D_MAX_DEPTH, image3d_max_depth);
	GET_INFO(IMAGE_SUPPORT, image_support);
	GET_INFO(MAX_PARAMETER_SIZE, max_parameter_size);
	GET_INFO(MAX_SAMPLERS, max_samplers);
	GET_INFO(MEM_BASE_ADDR_ALIGN, mem_base_addr_align);
	GET_INFO(MIN_DATA_TYPE_ALIGN_SIZE, min_data_type_align_size);
	GET_INFO(SINGLE_FP_CONFIG, single_fp_config);
	GET_INFO(GLOBAL_MEM_CACHE_TYPE, global_mem_cache_type);
	GET_INFO(GLOBAL_MEM_CACHELINE_SIZE, global_mem_cacheline_size);
	GET_INFO(GLOBAL_MEM_CACHE_SIZE, global_mem_cache_size);
	GET_INFO(GLOBAL_MEM_SIZE, global_mem_size);
	GET_INFO(MAX_CONSTANT_BUFFER_SIZE, max_constant_buffer_size);
	GET_INFO(MAX_CONSTANT_ARGS, max_constant_args);
	GET_INFO(LOCAL_MEM_TYPE, local_mem_type);
	GET_INFO(LOCAL_MEM_SIZE, local_mem_size);
	GET_INFO(ERROR_CORRECTION_SUPPORT, error_correction_support);
	GET_INFO(PROFILING_TIMER_RESOLUTION, profiling_timer_resolution);
	GET_INFO(ENDIAN_LITTLE, endian_little);
	GET_INFO(AVAILABLE, available);
	GET_INFO(COMPILER_AVAILABLE, compiler_available);
	GET_INFO(EXECUTION_CAPABILITIES, execution_capabilities);
	GET_INFO(QUEUE_PROPERTIES, queue_properties);
	GET_STRING(NAME, name);
	GET_STRING(VENDOR, vendor);
	GET_STRING_(DRIVER_VERSION, driver_version);
	GET_STRING(PROFILE, profile);
	GET_STRING(VERSION, version);
	GET_STRING(EXTENSIONS, extensions);
	GET_INFO(PLATFORM, platform);
	GET_INFO(PREFERRED_VECTOR_WIDTH_HALF, preferred_vector_width_half);
	GET_INFO(HOST_UNIFIED_MEMORY, host_unified_memory);
	GET_INFO(NATIVE_VECTOR_WIDTH_CHAR, native_vector_width_char);
	GET_INFO(NATIVE_VECTOR_WIDTH_SHORT, native_vector_width_short);
	GET_INFO(NATIVE_VECTOR_WIDTH_INT, native_vector_width_int);
	GET_INFO(NATIVE_VECTOR_WIDTH_LONG, native_vector_width_long);
	GET_INFO(NATIVE_VECTOR_WIDTH_FLOAT, native_vector_width_float);
	GET_INFO(NATIVE_VECTOR_WIDTH_DOUBLE, native_vector_width_double);
	GET_INFO(NATIVE_VECTOR_WIDTH_HALF, native_vector_width_half);
	GET_STRING(OPENCL_C_VERSION, opencl_c_version);

#undef GET_INFO
#undef GET_STRING_
#undef GET_STRING
	dinfo->has_info = CL_TRUE;

	return error;
}

static
cl_int _cluInitDeviceCache(clu_platform *, cl_device_id *, cl_uint);

/* Internal: get the device IDs, and cache them if they are all */
static
cl_int
_cluGetDeviceIDs(
	clu_platform *pinfo,
	cl_device_type type,
	cl_device_id **devices,
	cl_uint *num_devices)
{
	cl_uint count;
	cl_device_id *devs;
	cl_int error = CL_SUCCESS;

	error = clGetDeviceIDs(pinfo->id, type, 0, NULL, &count);
	if (error)
		return error;

	devs = NALLOC(count, cl_device_id);
	if (devs == NULL)
		return CL_OUT_OF_HOST_MEMORY;

	error = clGetDeviceIDs(pinfo->id, type, count, devs, &count);
	if (error) {
		free(devs);
		return error;
	}

	*num_devices = count;
	if (devices)
		*devices = devs;

	/* take the chance to init cache */
	if (type == CL_DEVICE_TYPE_ALL && pinfo->devices == NULL) {
		error = _cluInitDeviceCache(pinfo, devs, count);
	}

	return error;
}

/* Internal: initialize device cache for a platform */
static
cl_int
_cluInitDeviceCache(clu_platform *pinfo, cl_device_id *devs, cl_uint count)
{
	if (pinfo->devices)
	       return CL_SUCCESS;

	/* we are being called without the actual initialization list, so
	   call _cluGetDeviceIDs to get all devices, which will call us again
	   with the appropriate data */
	if (!devs)
		return _cluGetDeviceIDs(pinfo, CL_DEVICE_TYPE_ALL,
				NULL, &count);

	pinfo->devices = NALLOC(count, clu_device);
	if (pinfo->devices == NULL) {
		free(devs);
		return CL_OUT_OF_HOST_MEMORY;
	}
	pinfo->num_devices = count;

	for (cl_uint i=0; i < count; ++i) {
		clu_device *dev = (clu_device *)&pinfo->devices[i];
		dev->id = devs[i];
		dev->pinfo = pinfo;
	}
	return CL_SUCCESS;
}


cl_int
cluGetDeviceIDs(
	cl_platform_id platform,
	cl_device_type type,
	cl_device_id **devices,
	cl_uint *num_devices)
{
	cl_int error = _cluInitPlatformCache();

	if (error)
		return error;

	if (!num_devices)
		return CL_INVALID_VALUE;

	clu_platform *pinfo = _cluGetPlatformByID(platform);

	if (!pinfo)
		return CL_INVALID_PLATFORM;

	return _cluGetDeviceIDs(pinfo, type, devices, num_devices);
}

cl_int
cluGetDevices(
	cl_platform_id platform,
	cl_device_type type,
	clu_devptr **devices,
	cl_uint *num_devices)
{
	/* quick way out if the user is only interested in the number of devices */
	if (!devices)
		return clGetDeviceIDs(platform, type, 0, NULL, num_devices);

	/* let's get serious now */
	if (!num_devices)
		return CL_INVALID_VALUE;

	cl_int error = CL_SUCCESS;
	clu_platform *pinfo = (clu_platform *)cluGetPlatformByID(platform, &error);
	if (error) return error;

	error = _cluInitDeviceCache(pinfo, NULL, 0);
	if (error) return error;

	cl_device_id *devids = NULL;
	error = cluGetDeviceIDs(platform, type, &devids, num_devices);
	if (error) {
		free(devids);
		return error;
	}

	clu_devptr *devs = NALLOC(*num_devices, clu_devptr);
	if (!devs) {
		free(devids);
		return error;
	}

	for (cl_uint i=0; i < *num_devices; ++i)
		devs[i] = cluGetDeviceByID(platform, devids[i], &error);

	*devices = devs;
	return error;
}

clu_devptr
cluGetDeviceByID(cl_platform_id platform, cl_device_id id, cl_int *errcode_ret)
{
	cl_int error = CL_SUCCESS;
	clu_pfmptr pinfo = cluGetPlatformByID(platform, &error);
	RETURN_ON_ERROR;

	error = _cluInitDeviceCache((clu_platform *)pinfo, NULL, 0);

	for (cl_uint i=0; i < pinfo->num_devices; ++i) {
		clu_device *dev = (clu_device *)(pinfo->devices + i);
		if (dev->id == id) {
			error = _cluGetDeviceInfo(dev);
			RETURN_ON_ERROR;
			_RETURN(error, dev);
		}
	}

	_RETURN(CL_INVALID_DEVICE, NULL);
}




/* Internal function: reads a file into memory */
static
char*
_cluReadFile(const char *fname, size_t *fsize)
{
	size_t readsize = 0;
	char *buff = NULL;

	*fsize = 0;

	FILE *fd = fopen(fname, "rb");
	if (!fd)
		return NULL;

	fseek(fd, 0, SEEK_END);
	*fsize = ftell(fd);

	buff = (char *)malloc(*fsize);
	if (!buff)
		return NULL;

	rewind(fd);
	readsize = fread(buff, 1, *fsize, fd);
	if (*fsize != readsize) {
		free(buff);
		*fsize = 0;
		return NULL;
	}

	return buff;
}

cl_program
cluLoadProgramFromSourceFiles(
	cl_context context,
	cl_uint count,
	const char * const *filenames,
	cl_int *errcode_ret)
{
	errcode_ret = CL_SUCCESS;
	cl_program prog = NULL;
	char **buffers = NULL;
	size_t *lengths = NULL;

	if (!count) {
		if (errcode_ret)
			*errcode_ret = CL_INVALID_VALUE;
		goto ret;
	}

	buffers = NALLOC(count, char*);
	if (!buffers) {
		if (errcode_ret)
			*errcode_ret = CL_OUT_OF_HOST_MEMORY;
		goto ret;
	}

	lengths = NALLOC(count, size_t);
	if (!lengths) {
		if (errcode_ret)
			*errcode_ret = CL_OUT_OF_HOST_MEMORY;
		goto free_buff;
	}

	for (cl_uint i=0; i < count; ++i) {
		buffers[i] = _cluReadFile(filenames[i], lengths + i);
		if (buffers[i] == NULL) {
			if (errcode_ret) {
				if (errno == ENOMEM)
					*errcode_ret = CL_OUT_OF_HOST_MEMORY;
				else
					*errcode_ret = CL_INVALID_VALUE;
			}
			goto free_buffers;
		}
	}

	prog = clCreateProgramWithSource(context,
			count, (const char **)buffers, lengths, errcode_ret);

free_buffers:
	for (cl_uint i=0; i < count; ++i) {
		free(buffers[i]);
	}
	free(lengths);
free_buff:
	free(buffers);
ret:
	return prog;
}

