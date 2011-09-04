/* vim:ft=c
   This is the OpenCL Utility library. It defines a set of functions and C
   macros to make the host side of OpenCL programming less tedious

   Copyright (C) 2011 Giuseppe Bilotta

   See LEGAL for license information and other legalese
 */

/* Memory functions */

cl_mem
cluMalloc(
	size_t size,
	cl_mem_flags flags,
	void * host_ptr,
	cl_int *errcode_ret)
{
	cl_int error = CL_SUCCESS;
	cl_context ctx = cluGetCurrentContext(&error);
	RETURN_ON_ERROR;
	return clCreateBuffer(ctx, flags, size, host_ptr, errcode_ret);
}
