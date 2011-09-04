/* vim:ft=c
   This is the OpenCL Utility library. It defines a set of functions and C
   macros to make the host side of OpenCL programming less tedious

   Copyright (C) 2011 Giuseppe Bilotta

   See LEGAL for license information and other legalese
 */

/* Context and queue management functions */

#include <pthread.h>

/* Internal: management of thread-local queue */
static pthread_mutex_t clu_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_key_t *current_que; /* thread-local current queue */

/* Internal functions */

static
int
_cluInitTLS(void)
{
	int ret = 0;
	pthread_mutex_lock(&clu_mutex);
	if (current_que) goto unlock;

	current_que = NALLOC(1, pthread_key_t);
	if (!current_que) {
		ret = ENOMEM;
		goto unlock;
	}
	ret = pthread_key_create(current_que, NULL);
	if (ret)
		free(current_que);
unlock:
	pthread_mutex_unlock(&clu_mutex);
	return ret;
}

cl_context
cluGetQueueContext(cl_command_queue que, cl_int *errcode_ret)
{
	cl_context ret = NULL;
	cl_int error = CL_INVALID_VALUE;
	if (!que)
		_RETURN(error, ret);
	error = clGetCommandQueueInfo(que, CL_QUEUE_CONTEXT,
		sizeof(ret), &ret, NULL);
	RETURN_ON_ERROR;
	_RETURN(error, ret);
}

cl_device_id
cluGetQueueDevice(cl_command_queue que, cl_int *errcode_ret)
{
	cl_device_id ret = NULL;
	cl_int error = CL_INVALID_VALUE;
	if (!que)
		_RETURN(error, ret);
	error = clGetCommandQueueInfo(que, CL_QUEUE_DEVICE,
		sizeof(ret), &ret, NULL);
	RETURN_ON_ERROR;
	_RETURN(error, ret);
}

cl_context
cluGetCurrentContext(cl_int *errcode_ret)
{
	cl_int error;
	cl_command_queue que = cluGetCurrentQueue(&error);
	RETURN_ON_ERROR;
	return cluGetQueueContext(que, errcode_ret);
}

cl_device_id
cluGetCurrentDevice(cl_int *errcode_ret)
{
	cl_int error;
	cl_command_queue que = cluGetCurrentQueue(&error);
	RETURN_ON_ERROR;
	return cluGetQueueDevice(que, errcode_ret);
}

cl_command_queue
cluGetCurrentQueue(cl_int *errcode_ret)
{
	cl_command_queue ret = pthread_getspecific(*current_que);
	cl_int error = ret ? CL_SUCCESS : CLU_NO_CURRENT_QUEUE;
	_RETURN(error, ret);
}

cl_command_queue
cluSetCurrentQueue(cl_command_queue new_que, cl_int *errcode_ret)
{
	cl_command_queue ret = pthread_getspecific(*current_que);
	int pterr = pthread_setspecific(*current_que, new_que);
	if (!pterr)
		_RETURN(CL_SUCCESS, ret);
	if (pterr == ENOMEM)
		_RETURN(CL_OUT_OF_HOST_MEMORY, ret);
	_RETURN(CL_INVALID_VALUE, ret);
}

cl_context
cluCreateContext(
	cl_platform_id pfm,
	cl_uint num_devs,
	cl_device_id *devs,
	cl_int *errcode_ret)
{
	cl_int error = CL_SUCCESS;

	if (num_devs > 0 && !devs)
		_RETURN(CL_INVALID_VALUE, NULL);

	if (!pfm) {
		if (devs) {
			clu_devptr dev = cluGetDeviceByID(*devs, &error);
			RETURN_ON_ERROR;
			pfm = dev->platform;
		} else {
			error = clGetPlatformIDs(1, &pfm, NULL);
			RETURN_ON_ERROR;
		}
	}

	if (!devs) {
		error = cluGetDeviceIDs(pfm, CL_DEVICE_TYPE_DEFAULT, &devs, &num_devs);
		RETURN_ON_ERROR;
	}

	cl_context_properties ctx_prop[] = {
		CL_CONTEXT_PLATFORM, (cl_context_properties)pfm,
		0
	};

	return clCreateContext(ctx_prop, num_devs, devs,
			NULL, NULL, errcode_ret);
}

cl_command_queue
cluCreateCommandQueue(
	cl_context ctx,
	cl_device_id dev,
	cl_command_queue_properties props,
	cl_int *errcode_ret)
{
	cl_int error = CL_SUCCESS;
	cl_command_queue ret = NULL, old = NULL;

	if (!ctx) {
		ctx = cluGetCurrentContext(&error);
		if (error == CLU_NO_CURRENT_CONTEXT) {
			/* no current context */
			ctx = cluCreateContext(NULL,
					(dev ? 1 : 0),
					(dev ? &dev : NULL), &error);
		}
		RETURN_ON_ERROR;
	}

	if (!dev) {
		/* get the first context device */
		error = clGetContextInfo(ctx, CL_CONTEXT_DEVICES, sizeof(dev), &dev, NULL);
		RETURN_ON_ERROR;
	}

	ret = clCreateCommandQueue(ctx, dev, props, &error);
	RETURN_ON_ERROR;

	old = cluSetCurrentQueue(ret, &error);
	if (error)
		_RETURN(error, old);
	_RETURN(error, ret);
}

