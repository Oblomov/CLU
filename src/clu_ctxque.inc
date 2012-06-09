/* vim:ft=c
   This is the OpenCL Utility library. It defines a set of functions and C
   macros to make the host side of OpenCL programming less tedious

   Copyright (C) 2011 Giuseppe Bilotta

   See LEGAL for license information and other legalese
 */

/* Context and queue management functions */

#include <pthread.h>

/* Internal: management of thread-local queue/device/context */
static pthread_mutex_t clu_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_key_t *current_ctx; /* thread-local current context */
static pthread_key_t *current_dev; /* thread-local current device */
static pthread_key_t *current_que; /* thread-local current queue */

/* Internal functions */

static
int
_cluInitTLS(void)
{
	int ret = 0;
	pthread_mutex_lock(&clu_mutex);
	if (current_que) goto unlock;

	current_ctx = NALLOC(1, pthread_key_t);
	if (!current_ctx) {
		ret = ENOMEM;
		goto unlock;
	}
	current_dev = NALLOC(1, pthread_key_t);
	if (!current_dev) {
		ret = ENOMEM;
		goto free_ctx;
	}
	current_que = NALLOC(1, pthread_key_t);
	if (!current_que) {
		ret = ENOMEM;
		goto free_dev;
	}
	ret = pthread_key_create(current_que, NULL);
	if (!ret) goto unlock;
	free(current_que);
free_dev:
	free(current_dev);
free_ctx:
	free(current_ctx);
unlock:
	pthread_mutex_unlock(&clu_mutex);
	return ret;
}

cl_device_id 
cluGetContextDefaultDevice(cl_context ctx, cl_int *errcode_ret) 
{
  cl_int error;
  cl_device_id dev = NULL;
  error = clGetContextInfo(ctx, CL_CONTEXT_DEVICES, sizeof(dev), &dev, NULL);
  RETURN_ON_ERROR;

  _RETURN(error, dev);
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
	cl_context ret = pthread_getspecific(*current_ctx);
	cl_int error = ret ? CL_SUCCESS : CL_INVALID_CONTEXT;
	_RETURN(error, ret);
}

cl_device_id
cluGetCurrentDevice(cl_int *errcode_ret)
{
	cl_device_id ret = pthread_getspecific(*current_dev);
	cl_int error = ret ? CL_SUCCESS : CL_INVALID_DEVICE;
	_RETURN(error, ret);
}

cl_command_queue
cluGetCurrentQueue(cl_int *errcode_ret)
{
	cl_command_queue ret = pthread_getspecific(*current_que);
	cl_int error = ret ? CL_SUCCESS : CL_INVALID_COMMAND_QUEUE;
	_RETURN(error, ret);
}

cl_command_queue
cluSetCurrentQueue(cl_command_queue new_que, cl_int *errcode_ret)
{
	cl_command_queue ret = pthread_getspecific(*current_que);
	cl_int error = CL_SUCCESS;
	int pterr = pthread_setspecific(*current_que, new_que);
	if (!pterr) {
		cl_device_id dev = cluGetQueueDevice(new_que, &error);
		if (error) _RETURN(error, ret);
		pterr = pthread_setspecific(*current_dev, dev);
		if (!pterr) {
			cl_context ctx = cluGetQueueContext(new_que, &error);
			if (error) _RETURN(error, ret);
			pterr = pthread_setspecific(*current_ctx, ctx);
			if (!pterr)
				_RETURN(error, ret);
		}
	}
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


cl_context 
cluCreateContextByType(cl_device_type type,
		       cl_int *errcode_ret)
{
  cl_int error = CL_SUCCESS;
  cl_uint numPlatforms = 0;
  cl_context context = NULL;

  error = clGetPlatformIDs(0, NULL, &numPlatforms);
  RETURN_ON_ERROR;

  if(numPlatforms < 1)
    _RETURN(CL_DEVICE_NOT_FOUND, NULL);
  
  // We have at least one
  cl_platform_id* platforms = NALLOC(numPlatforms, cl_platform_id);
  error = clGetPlatformIDs(numPlatforms, platforms, NULL);
  RETURN_ON_ERROR;
   
  for(int i=0; i<numPlatforms; i++) {
    cl_platform_id platform_id = platforms[i];

    cl_context_properties cps[3] = {
      CL_CONTEXT_PLATFORM, 
      (cl_context_properties)platform_id, 
      0
    };
      
    context = clCreateContextFromType(
				      cps,
				      type,
				      NULL,
				      NULL,
				      &error);
    if(error == CL_SUCCESS) {
      // We have a context, so we are done.
      break; 
    } else if(error == CL_DEVICE_NOT_FOUND || 
	      error == CL_DEVICE_NOT_AVAILABLE || 
	      error == CL_OUT_OF_RESOURCES) {
      // No context yet so make sure context is NULL and try the next platform.
      // These errors probably only effect this one platform.
      context = NULL; 
    } else {
      // There was another error, so probably something else is wrong so return 
      // the error instead of continuing to try other platforms.
      RETURN_ON_ERROR;
    }
  }

  free(platforms);
  
  if( context == NULL ) {
    // If context is still null we have failed.    
    _RETURN(CL_DEVICE_NOT_FOUND, NULL);
  }

  _RETURN(CL_SUCCESS, context);
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
		if (error == CL_INVALID_CONTEXT) {
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

