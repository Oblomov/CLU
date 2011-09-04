/*
   This is the OpenCL Utility library. It defines a set of functions and C
   macros to make the host side of OpenCL programming less tedious

   Copyright (C) 2011 Giuseppe Bilotta

   See LEGAL for license information and other legalese
 */

#ifndef CLU_VERSION
#define CLU_VERSION 1
#define CLU_VERSION_STRING 0.0.1

/* OpenCL includes */
#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
  Return a C string describing the given OpenCL error
 */
const char *
cluGetErrorString(cl_int error);

/**
  A data type that holds pointers to all the available platform info.
 */

typedef const struct clu_device* clu_devptr;
typedef const struct clu_platform* clu_pfmptr;

typedef struct clu_platform {
	cl_platform_id	id;
	cl_bool		has_info;
	const char*	profile;
	const char*	version;
	const char*	name;
	const char*	vendor;
	const char*	extensions;
	cl_uint		num_devices;
	clu_devptr	devices;
} clu_platform;


/**
  Point platforms to a list of all available platform IDs,
  and returns their number in num_platforms. If platforms is NULL,
  only returns the number of available platforms. If num_platforms is NULL,
  CL_INVALID_VALUE is returned.
  The caller must take care of freeing the platforms array when done.
 */

cl_int
cluGetPlatformIDs(cl_platform_id **platforms, cl_uint *num_platforms);

/**
  Point platforms to a list of all avaliable platforms,
  and returns their number in num_platforms. If platforms is NULL,
  only returns the number of available platforms. If num_platforms is NULL,
  CL_INVALID_VALUE is returned.
  The caller must take care of freeing the platforms array when done.
 */

cl_int
cluGetPlatforms(clu_pfmptr **platforms, cl_uint *num_platforms);

/**
  Returns a clu_pfmptr pointing to the platform matching the given ID. Error
  conditions are set in errcode_ret.
 */
clu_pfmptr
cluGetPlatformByID(cl_platform_id id, cl_int *errcode_ret);

/**
  Returns a clu_pfmptr pointing to the first platform whose name or vendor
  begins with the given name. If name is NULL or the empty string, the first
  platform is given. Error conditions are set in errcode_ret.
 */
clu_pfmptr
cluGetPlatformByName(const char* name, cl_int *errcode_ret);

/**
  Returns a clu_pfmptr pointing to the (0-based) num-th available platform. If
  num is higher than the number of platform, errcode_ret is set to
  CL_INVALID_VALUE.
 */
clu_pfmptr
cluGetPlatformByNumber(cl_uint num, cl_int *errcode_ret);

/**
  Returns a clu_pfmptr pointing to the platform specified in a command-line argument
  or textual option. If arg looks like a number, get platform by number
  otherwise get platform by name.
 */
clu_pfmptr
cluGetPlatformFromArg(const char* name_or_num, cl_int *errcode_ret);

/**
  A data type that holds all the available device info.
 */

typedef struct clu_device {
	cl_device_id	id;
	clu_pfmptr	pinfo;
	cl_bool		has_info;
	cl_device_type	type;
	cl_uint		vendor_id;
	cl_uint		max_compute_units;
	cl_uint		max_work_item_dimensions;
	size_t		max_work_group_size;
	const size_t*	max_work_item_sizes;
	cl_uint		preferred_vector_width_char;
	cl_uint		preferred_vector_width_short;
	cl_uint		preferred_vector_width_int;
	cl_uint		preferred_vector_width_long;
	cl_uint		preferred_vector_width_float;
	cl_uint		preferred_vector_width_double;
	cl_uint		max_clock_frequency;
	cl_uint		address_bits;
	cl_uint		max_read_image_args;
	cl_uint		max_write_image_args;
	cl_ulong	max_mem_alloc_size;
	size_t		image2d_max_width;
	size_t		image2d_max_height;
	size_t		image3d_max_width;
	size_t		image3d_max_height;
	size_t		image3d_max_depth;
	cl_bool		image_support;
	size_t		max_parameter_size;
	cl_uint		max_samplers;
	cl_uint		mem_base_addr_align;
	cl_uint		min_data_type_align_size;
	cl_device_fp_config single_fp_config;
	cl_device_mem_cache_type global_mem_cache_type;
	cl_uint		global_mem_cacheline_size;
	cl_ulong	global_mem_cache_size;
	cl_ulong	global_mem_size;
	cl_ulong	max_constant_buffer_size;
	cl_uint		max_constant_args;
	cl_device_local_mem_type local_mem_type;
	cl_ulong	local_mem_size;
	cl_bool		error_correction_support;
	size_t		profiling_timer_resolution;
	cl_bool		endian_little;
	cl_bool		available;
	cl_bool		compiler_available;
	cl_device_exec_capabilities execution_capabilities;
	cl_command_queue_properties queue_properties;
	const char*	name;
	const char*	vendor;
	const char*	driver_version;
	const char*	profile;
	const char*	version;
	const char*	extensions;
	cl_platform_id	platform;
	cl_device_fp_config _reserved1; /* double_fp_config */
	cl_device_fp_config _reserved2; /* half_fp_config */
	cl_uint		preferred_vector_width_half;
	cl_bool		host_unified_memory;
	cl_uint		native_vector_width_char;
	cl_uint		native_vector_width_short;
	cl_uint		native_vector_width_int;
	cl_uint		native_vector_width_long;
	cl_uint		native_vector_width_float;
	cl_uint		native_vector_width_double;
	cl_uint		native_vector_width_half;
	const char*	opencl_c_version;
} clu_device;

/**
  Return a C string describing the given OpenCL device type
 */
const char*
cluGetDeviceTypeString(cl_device_type type);

/**
  Point devices to a list of the IDs of all the available devices of the
  given type exposed by the given platform. Their number is returned in
  num_devices. If devices is NULL, only returns the number of available
  devices. If num_devices is NULL, CL_INVALID_VALUE is returned.
  The caller must take care of freeing the devices array when done.
 */

cl_int
cluGetDeviceIDs(
	cl_platform_id platform,
	cl_device_type type,
	cl_device_id **devices,
	cl_uint *num_devices);

/**
  Point devices to a list of all the available devices of the
  given type exposed by the given platform. Their number is returned in
  num_devices. If devices is NULL, only returns the number of available
  devices. If num_devices is NULL, CL_INVALID_VALUE is returned.
  The caller must take care of freeing the devices array when done.
 */

cl_int
cluGetDevices(
	cl_platform_id platform,
	cl_device_type type,
	clu_devptr **devices,
	cl_uint *num_devices);

/**
  Returns a clu_devptr pointing to the device matching the given ID. Error
  conditions are set in errcode_ret.
 */

clu_devptr
cluGetDeviceByID(cl_device_id id, cl_int *errcode_ret);

/**
  Get the context associated with a given command queue
 */

cl_context
cluGetQueueContext(cl_command_queue que, cl_int *errcode_ret);

/**
  Get the device associated with a given command queue
 */

cl_device_id
cluGetQueueDevice(cl_command_queue que, cl_int *errcode_ret);

/**
  The OpenCL Utility library defines a current queue and an associated current
  device and context. If no current queue is defined, the cluGetCurrent*() functions
  will set an appropriate error status (CLU_NO_CURRENT_*), overloading
  CL_INVALID_COMMAND_QUEUE.
 */

#define CLU_NO_CURRENT_CONTEXT CL_INVALID_COMMAND_QUEUE
#define CLU_NO_CURRENT_DEVICE CL_INVALID_COMMAND_QUEUE
#define CLU_NO_CURRENT_QUEUE CL_INVALID_COMMAND_QUEUE

/**
  Get the context of the current command queue
  */
cl_context
cluGetCurrentContext(cl_int *errcode_ret);

/**
  Get the device of the current command queue
  */
cl_device_id
cluGetCurrentDevice(cl_int *errcode_ret);

/**
  Get the current command queue
  */
cl_command_queue
cluGetCurrentQueue(cl_int *errcode_ret);

/**
  Set the current command queue, returning the previous one
  */
cl_command_queue
cluSetCurrentQueue(cl_command_queue new_que, cl_int *errcode_ret);

/**
  Create a context for the specified platform and devices. If the
  platform is NULL, the devices' platform will be used. If the devices
  are NULL, the default platform device will be used. If both are NULL,
  the default device of the default platform will be used.
 */
cl_context
cluCreateContext(
	cl_platform_id pfm,
	cl_uint num_devs,
	cl_device_id *devs,
	cl_int *errcode_ret);

/**
  Create a command queue for the specified devices in the given context. If the
  context is NULL, the current context will be used, or a new one will be
  created. If the device is NULL, the first device in the context will be used.
 */
cl_command_queue
cluCreateCommandQueue(
	cl_context ctx,
	cl_device_id dev,
	cl_command_queue_properties props,
	cl_int *errcode_ret);

/** Create a memory buffer on the current context */
cl_mem
cluMalloc(
	size_t size,
	cl_mem_flags flags,
	void * host_ptr,
	cl_int *errcode_ret);

/** Release a memory buffer */
inline
cl_int
cluFree(cl_mem memobj)
{ return clReleaseMemObject(memobj); }

/**
  Create a program loading given source files.
  The number of source files is given in count, and the file names
  are given in the array of strings filenames.

  Returns a valid non-zero program object and errcode_ret is set to
  CL_SUCCESS if the program object is created successfully. Otherwise,
  it returns a NULL value with one of the following error values
  returned in errcode_ret: 

  * CL_INVALID_CONTEXT if context is not a valid context. 
  * CL_INVALID_VALUE if count is zero or if the files could not be found
    or if there was an error reading them.
  * CL_OUT_OF_HOST_MEMORY if there is a failure to allocate resources
    required to hold the source files content or by the OpenCL
    implementation on the host during program creation.
 */

cl_program
cluLoadProgramFromSourceFiles(
	cl_context context,
	cl_uint count,
	const char * const *filenames,
	cl_int *errcode_ret);

/** A one-file wrapper for cluCreateProgramWithSourceFiles */
inline
cl_program
cluLoadProgramFromSourceFile(
	cl_context context,
	const char *filename,
	cl_int *errcode_ret)
{ return cluLoadProgramFromSourceFiles(context, 1, &filename, errcode_ret); }


#ifdef __cplusplus
}
#endif

#endif // CLU_VERSION
