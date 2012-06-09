/* vim:ft=c
   This is the OpenCL Utility library. It defines a set of functions and C
   macros to make the host side of OpenCL programming less tedious

   Copyright (C) 2011 Giuseppe Bilotta

   See LEGAL for license information and other legalese
 */

/* Platform management functions */

/* TODO thread safety */
#define EMPTY_CACHE ((cl_uint)-1)
/* number of cached platform */
static cl_uint _num_platforms = EMPTY_CACHE;
/* cached platform information */
static clu_platform *_platforms;

/* Internal functions */

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

static
clu_platform *
_cluGetPlatformByID(cl_platform_id id)
{
	for (size_t i=0; i < _num_platforms; ++i)
		if (_platforms[i].id == id)
			return _platforms + i;
	return NULL;
}

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

/* Public functions */

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
		error = _cluGetPlatformInfo(curr);
		RETURN_ON_ERROR;
		/* TODO locale */
		if (strncasecmp(name, curr->name, len) == 0) {
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


