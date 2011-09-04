/* vim:ft=c
   This is the OpenCL Utility library. It defines a set of functions and C
   macros to make the host side of OpenCL programming less tedious

   Copyright (C) 2011 Giuseppe Bilotta

   See LEGAL for license information and other legalese
 */

/* Device management functions */

/* Internal functions */

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

/* Public functions */

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
