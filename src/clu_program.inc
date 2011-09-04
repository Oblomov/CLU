/* vim:ft=c
   This is the OpenCL Utility library. It defines a set of functions and C
   macros to make the host side of OpenCL programming less tedious

   Copyright (C) 2011 Giuseppe Bilotta

   See LEGAL for license information and other legalese
 */

/* Program functions */

/* Internal functions */

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

/* Public functions */

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

