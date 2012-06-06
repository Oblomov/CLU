/* vim:ft=c
   This is the OpenCL Utility library. It defines a set of functions and C
   macros to make the host side of OpenCL programming less tedious

   Copyright (C) 2011 Giuseppe Bilotta

   See LEGAL for license information and other legalese
 */

/* Generic functions */

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

static const size_t error_string_count = ARRAY_SIZE(error_strings);
#define unknown_error_index (error_string_count - 1)

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
#define unknown_device_type_index (device_type_string_count - 1)

const char *
cluGetDeviceTypeString(cl_device_type type)
{
	if (type > unknown_device_type_index)
		type = unknown_device_type_index;
	return device_type_string[type];
}

