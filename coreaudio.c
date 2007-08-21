/*
 * coreaudio -- Python interface to OS X CoreAudio
 *
 * Author: Lars Immisch (lars@ibp.de)
 *
 * License: Python Software Foundation License
 *
 */

#include "Python.h"
#include <structmember.h>
#include <CoreServices/CoreServices.h>
#include <stdio.h>
#include <unistd.h>
#include <AudioUnit/AudioUnit.h>
#include <stdio.h>

#if PY_VERSION_HEX < 0x02050000 && !defined(PY_SSIZE_T_MIN)
typedef int Py_ssize_t;
#define PY_SSIZE_T_MAX INT_MAX
#define PY_SSIZE_T_MIN INT_MIN
#endif

#define FOURCC_ARGS(x)  (x & 0xff000000) >> 24, (x & 0xff0000) >> 16, \
		(x & 0xff00) >> 8, (x) & 0xff

PyDoc_STRVAR(coreaudio_module_doc,
			 "This modules provides support for the CoreAudio API.\n"
			 "Available types are: Component, ComponentDescription and "
			 "AudioStreamBasicDescType.\n"
	);

static PyObject *CoreAudioError;

typedef struct {
	PyObject_HEAD;
	ComponentDescription desc;
} component_desc_t;

static PyTypeObject ComponentDescriptionType;

static PyObject *
component_desc_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
	int res;
	component_desc_t *self;
	OSType cotype = kAudioUnitType_Output;
	OSType subtype = kAudioUnitSubType_DefaultOutput;
	OSType manufacturer = kAudioUnitManufacturer_Apple;
	unsigned long flags = 0;
	unsigned long mask = 0;

	if (!PyArg_ParseTuple(args,"|kkkkk:ComponentDescription", &cotype, 
						  &subtype, &manufacturer, &flags, &mask)) 
		return NULL;

	if (!(self = (component_desc_t *)PyObject_New(component_desc_t, 
												  &ComponentDescriptionType))) 
		return NULL;

	self->desc.componentType = cotype;
	self->desc.componentSubType = subtype;
	self->desc.componentManufacturer = manufacturer;
	self->desc.componentFlags = flags;
	self->desc.componentFlagsMask = mask;
	
	return (PyObject *)self;
}

static void
component_desc_dealloc(component_desc_t *obj)
{
    PyObject_Free(obj);
}

static PyObject *
component_desc_repr(component_desc_t *obj)
{
    return PyString_FromFormat("ComponentDescription('%c%c%c%c', "
							   "'%c%c%c%c', '%c%c%c%c', 0x%x, 0x%x)",
                               FOURCC_ARGS(obj->desc.componentType), 
							   FOURCC_ARGS(obj->desc.componentSubType),
							   FOURCC_ARGS(obj->desc.componentManufacturer),
							   obj->desc.componentFlags, 
							   obj->desc.componentFlagsMask);
}

static PyMemberDef component_desc_members[] = {
    {"componentType", T_ULONG, 
	 offsetof(component_desc_t, desc.componentType), 0, ""},
    {"componentSubType", T_ULONG, 
	 offsetof(component_desc_t, desc.componentSubType), 0, ""},
    {"componentManufacturer", T_ULONG, 
	 offsetof(component_desc_t, desc.componentManufacturer), 0, ""},
    {"componentFlags", T_ULONG, 
	 offsetof(component_desc_t, desc.componentFlags), 0, ""},
    {"componentFlagsMask", T_ULONG, 
	 offsetof(component_desc_t, desc.componentFlagsMask), 0, ""},
    {NULL}  /* Sentinel */
};

static PyTypeObject ComponentDescriptionType = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,                              /*ob_size*/
	"coreaudio.ComponentDescription",      /*tp_name*/
	sizeof(component_desc_t),       /*tp_basicsize*/
	0,                              /*tp_itemsize*/
	/* methods */    
	(destructor)component_desc_dealloc, /*tp_dealloc*/
	0,                              /*print*/
    0,                              /*tp_getattr*/
	0,                              /*tp_setattr*/
	0,                              /*tp_compare*/ 
	(reprfunc)component_desc_repr,  /*tp_repr*/
	0,                              /*tp_as_number*/
	0,                              /*tp_as_sequence*/
	0,                              /*tp_as_mapping*/
	0,                              /*tp_hash*/
	0,                              /*tp_call*/
	0,                              /*tp_str*/
	0,                              /*tp_getattro*/
	0,                              /*tp_setattro*/
	0,                              /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,             /*tp_flags*/
	"CoreFoundation ComponentDescription",  /*tp_doc*/
};

typedef struct {
	PyObject_HEAD;
	Component component;
} component_t;

static PyTypeObject ComponentType;

static PyObject *
component_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
	component_t *self;

	if (!PyArg_ParseTuple(args, ":Component"))
		return NULL;

	if (!(self = (component_t *)PyObject_New(component_t, &ComponentType))) 
		return NULL;

	self->component = NULL;

	return (PyObject *)self;
}

static void
component_dealloc(component_t *obj)
{
    PyObject_Free(obj);
}

static PyTypeObject ComponentType = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,                              /*ob_size*/
	"coreaudio.Component",          /*tp_name*/
	sizeof(component_t),            /*tp_basicsize*/
	0,                              /*tp_itemsize*/
	/* methods */    
	(destructor)component_dealloc,  /*tp_dealloc*/
	0,                              /*print*/
    0,                              /*tp_getattr*/
	0,                              /*tp_setattr*/
	0,                              /*tp_compare*/ 
	0,                              /*tp_repr*/
	0,                              /*tp_as_number*/
	0,                              /*tp_as_sequence*/
	0,                              /*tp_as_mapping*/
	0,                              /*tp_hash*/
	0,                              /*tp_call*/
	0,                              /*tp_str*/
	0,                              /*tp_getattro*/
	0,                              /*tp_setattro*/
	0,                              /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,             /*tp_flags*/
	"CoreFoundation Component",     /*tp_doc*/
};

typedef struct {
	PyObject_HEAD;
	AudioStreamBasicDescription bdesc;
} audio_stream_basic_desc_t;

static PyTypeObject AudioStreamBasicDescType;

static PyObject *
audio_stream_basic_desc_new(PyTypeObject *type, PyObject *args, 
							PyObject *kwds) {
	audio_stream_basic_desc_t *self;
	Float64 sampleRate;
	UInt32 formatID, formatFlags, bytesPerPacket, framesPerPacket,
		bytesPerFrame, channelsPerFrame, bitsPerChannel;

	if (!PyArg_ParseTuple(args, "dkkkkkkk:AudioStreamBasicDescription", 
						  &sampleRate, &formatID, 
						  &formatFlags, &bytesPerPacket, &framesPerPacket,
						  &bytesPerFrame, &channelsPerFrame, &bitsPerChannel))
		return NULL;

	if (!(self = (audio_stream_basic_desc_t *)
		  PyObject_New(audio_stream_basic_desc_t, &AudioStreamBasicDescType))) 
		return NULL;

	self->bdesc.mSampleRate = sampleRate;
	self->bdesc.mFormatID = formatID;
	self->bdesc.mFormatFlags = formatFlags;
	self->bdesc.mBytesPerPacket = bytesPerPacket; 
	self->bdesc.mFramesPerPacket = framesPerPacket;	
	self->bdesc.mBytesPerFrame = bytesPerFrame;		
	self->bdesc.mChannelsPerFrame = channelsPerFrame;
	self->bdesc.mBitsPerChannel = bitsPerChannel;	

	return (PyObject *)self;
}

static void
audio_stream_basic_desc_dealloc(audio_stream_basic_desc_t *obj)
{
    PyObject_Free(obj);
}

static PyMemberDef audio_stream_basic_desc_members[] = {
    {"mSampleRate", T_DOUBLE, 
	 offsetof(audio_stream_basic_desc_t, bdesc.mSampleRate), 0, ""},
    {"mFormatID", T_ULONG, 
	 offsetof(audio_stream_basic_desc_t, bdesc.mFormatID), 0, ""},
    {"mFormatFlags", T_ULONG, 
	 offsetof(audio_stream_basic_desc_t, bdesc.mFormatFlags), 0, ""},
    {"mBytesPerPacket", T_ULONG, 
	 offsetof(audio_stream_basic_desc_t, bdesc.mBytesPerPacket), 0, ""},
    {"mFramesPerPacket", T_ULONG, 
	 offsetof(audio_stream_basic_desc_t, bdesc.mFramesPerPacket), 0, ""},
    {"mBytesPerFrame", T_ULONG, 
	 offsetof(audio_stream_basic_desc_t, bdesc.mBytesPerFrame), 0, ""},
    {"mChannelsPerFrame", T_ULONG, 
	 offsetof(audio_stream_basic_desc_t, bdesc.mChannelsPerFrame), 0, ""},
    {"mBitsPerChannel", T_ULONG, 
	 offsetof(audio_stream_basic_desc_t, bdesc.mBitsPerChannel), 0, ""},
    {NULL}  /* Sentinel */
};

static PyTypeObject AudioStreamBasicDescType = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,                              /*ob_size*/
	"coreaudio.AudioStreamBasicDescription", /*tp_name*/
	sizeof(audio_stream_basic_desc_t), /*tp_basicsize*/
	0,                              /*tp_itemsize*/
	/* methods */    
	(destructor)audio_stream_basic_desc_dealloc, /*tp_dealloc*/
	0,                              /*print*/
    0,                              /*tp_getattr*/
	0,                              /*tp_setattr*/
	0,                              /*tp_compare*/ 
	0,                              /*tp_repr*/
	0,                              /*tp_as_number*/
	0,                              /*tp_as_sequence*/
	0,                              /*tp_as_mapping*/
	0,                              /*tp_hash*/
	0,                              /*tp_call*/
	0,                              /*tp_str*/
	0,                              /*tp_getattro*/
	0,                              /*tp_setattro*/
	0,                              /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,             /*tp_flags*/
	"AudioUnit AudioStreamBasicDescription",  /*tp_doc*/
};

typedef struct {
	PyObject_HEAD;
	AudioUnit instance;
	PyObject *render_callback;
	PyObject *user_data;
} audio_unit_t;

static PyTypeObject AudioUnitType;

static PyObject *
audio_unit_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
	audio_unit_t *self;

	if (!PyArg_ParseTuple(args, ":AudioUnit"))
		return NULL;

	if (!(self = (audio_unit_t *)PyObject_New(
			  audio_unit_t, &AudioUnitType))) 
		return NULL;

	self->instance = NULL;
	self->render_callback = NULL;
	self->user_data = NULL;

	return (PyObject *)self;
}

static void
audio_unit_dealloc(audio_unit_t *obj)
{
	if (obj->render_callback)
		Py_DECREF(obj->render_callback);

	if (obj->user_data)
		Py_DECREF(obj->user_data);

	if (obj->instance)
	{
		AudioUnitUninitialize(obj->instance);
		CloseComponent(obj->instance);
	}

    PyObject_Free(obj);
}

static PyObject *
audio_unit_setstreamformat(audio_unit_t *self, PyObject *args)
{
	OSErr rc;
	audio_stream_basic_desc_t *bdesc;

	if (!PyArg_ParseTuple(args, "O!:SetStreamFormat", 
						  &AudioStreamBasicDescType, &bdesc))
		return NULL;

	rc = AudioUnitSetProperty(self->instance, 
							  kAudioUnitProperty_StreamFormat,
							  kAudioUnitScope_Input,
							  0,
							  &bdesc->bdesc,
							  sizeof(AudioStreamBasicDescription));

	if (rc != noErr)
	{
		PyErr_Format(
			CoreAudioError, "AudioUnitSetProperty(StreamFormat) failed: %4.4s", 
			(char*)&rc);
		return NULL;
	}
	
	Py_INCREF(Py_None);
	return Py_None;
}

static OSStatus
audio_unit_render_callback(void *inRefCon, 
						   AudioUnitRenderActionFlags *ioActionFlags, 
						   const AudioTimeStamp *inTimeStamp, 
						   UInt32 inBusNumber, 
						   UInt32 inNumberFrames, 
						   AudioBufferList *ioData)
{
	int i;
	PyObject *o;
	audio_unit_t *self = (audio_unit_t*)inRefCon;
	PyGILState_STATE gil = PyGILState_Ensure();

	PyObject *args = Py_BuildValue(
		"(k{sdsKsdsKsk}iiiO)", 
		*ioActionFlags,
		"mSampleTime", inTimeStamp->mSampleTime,
		"mHostTime", inTimeStamp->mHostTime,
		"mRateScalar", inTimeStamp->mRateScalar,
		"mWordClockTime", inTimeStamp->mWordClockTime,
		"mFlags", inTimeStamp->mFlags,
		inBusNumber,
		inNumberFrames,
		ioData->mNumberBuffers,
		self->user_data);
	
	PyObject *result = PyObject_CallObject(self->render_callback, args);
	if (!result)
		goto py_error;

	if (!PyTuple_Check(result))
	{
		fprintf(stderr, "render callback must return a tuple\n");
		goto error;
	}

	o = PyTuple_GetItem(result, 0);
	if (!o)
		goto py_error;

	if (o != Py_None)
	{
		if (!PyInt_Check(o))
		{
			fprintf(
				stderr, "render callback must return a tuple (int, buffers)\n");
			goto error;
		}

		*ioActionFlags = PyInt_AsUnsignedLongMask(o);
	}

	for (i = 1; i < PyTuple_Size(result); ++i)
	{
		char *buffer;
		Py_ssize_t len;

		o = PyTuple_GetItem(result, i);
		if (!PyString_Check(o))
		{
			fprintf(
				stderr, "render callback must return a tuple (int, buffers)\n");
			goto error;
		}
		if (PyString_AsStringAndSize(o, &buffer, &len) < 0)
			goto py_error;

		if (len == 0)
		{
			PyGILState_Release(gil);
			// No data: stop audio output
			AudioOutputUnitStop(self->instance);

			return 0;
		}

		if (len != ioData->mBuffers[i-1].mDataByteSize)
		{
			fprintf(stderr, "render_callback: buffer %d size mismatch: "
					"expected %d bytes, got %d\n", i-1, 
					ioData->mBuffers[i-1].mDataByteSize, len);
			goto error;
		}

		memcpy(ioData->mBuffers[i-1].mData, buffer, len);
	}

	PyGILState_Release(gil);

	return 0;

py_error:
	// Todo: store the exception and traceback somewhere
	PyErr_Print();

error:
	PyGILState_Release(gil);
	// Stop output - nothing good could come out of continued operation
	AudioOutputUnitStop(self->instance);

	return -1;
}

static PyObject *
audio_unit_setrendercallback(audio_unit_t *self, PyObject *args)
{
	OSErr rc;
	PyObject *callback;
	PyObject *user_data = Py_None;
    AURenderCallbackStruct input;

	if (!PyArg_ParseTuple(args, "O|O:SetRenderCallback", 
						  &callback, &user_data))
		return NULL;

	// If a callback or user data was previously set, decrement the refcount
	if (self->render_callback)
		Py_DECREF(self->render_callback);

	if (self->user_data)
		Py_DECREF(self->user_data);
	
	// Keep a reference
	Py_INCREF(callback);
	Py_INCREF(user_data);

	self->render_callback = callback;
	self->user_data = user_data;

	if (callback != Py_None)
	{
		input.inputProc = audio_unit_render_callback;
		input.inputProcRefCon = self;
	}
	else
	{
		input.inputProc = NULL;
		input.inputProcRefCon = NULL;
	}

	rc = AudioUnitSetProperty (self->instance, 
							   kAudioUnitProperty_SetRenderCallback, 
							   kAudioUnitScope_Input,
							   0, 
							   &input, 
							   sizeof(input));

	if (rc != noErr)
	{
		self->render_callback = NULL;
		self->user_data = NULL;

		Py_DECREF(callback);
		Py_DECREF(user_data);

		PyErr_Format(
			CoreAudioError, "AudioUnitSetProperty(RenderCallback) failed: "
			"%c%c%c%c", FOURCC_ARGS(rc));
		return NULL;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
audio_unit_initialize(audio_unit_t *self, PyObject *args)
{
	OSErr rc;

	if (!PyArg_ParseTuple(args, ":Initialize"))
		return NULL;

	rc = AudioUnitInitialize(self->instance);
	if (rc != noErr)
	{
		PyErr_Format(CoreAudioError, "AudioUnitInitialize failed: %4.4s", 
					 (char*)&rc);
		return NULL;
	}
	
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
audio_unit_uninitialize(audio_unit_t *self, PyObject *args)
{
	OSErr rc;

	if (!PyArg_ParseTuple(args, ":Uninitialize"))
		return NULL;

	rc = AudioUnitUninitialize(self->instance);
	if (rc != noErr)
	{
		PyErr_Format(CoreAudioError, "AudioUnitUninitialize failed: %4.4s", 
					 (char*)&rc);
		return NULL;
	}
	
	self->instance = NULL;

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
audio_unit_start(audio_unit_t *self, PyObject *args)
{
	OSErr rc;

	if (!PyArg_ParseTuple(args, ":Start"))
		return NULL;

	if (!self->instance)
	{
		PyErr_Format(CoreAudioError, 
					 "AudioOutputUnitStart failed: NULL AudioUnit"); 
		return NULL;
	}

	rc = AudioOutputUnitStart(self->instance);
	if (rc != noErr)
	{
		PyErr_Format(CoreAudioError, "AudioOutputUnitStart failed: %4.4s", 
					 (char*)&rc);
		return NULL;
	}
	
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
audio_unit_stop(audio_unit_t *self, PyObject *args)
{
	OSErr rc;

	if (!PyArg_ParseTuple(args, ":Stop"))
		return NULL;

	if (!self->instance)
	{
		PyErr_Format(CoreAudioError, 
					 "AudioOutputUnitStop failed: NULL AudioUnit"); 
		return NULL;
	}

	rc = AudioOutputUnitStop(self->instance);
	if (rc != noErr)
	{
		PyErr_Format(CoreAudioError, "AudioOutputUnitStop failed: %4.4s", 
					 (char*)&rc);
		return NULL;
	}
	
	Py_INCREF(Py_None);
	return Py_None;
}

/* AudioUnit Object Bureaucracy */

static PyMethodDef audio_unit_methods[] = {
	{"Initialize", (PyCFunction)audio_unit_initialize, 
	 METH_VARARGS},
	{"Uninitialize", (PyCFunction)audio_unit_uninitialize, 
	 METH_VARARGS},
	{"Start", (PyCFunction)audio_unit_start, 
	 METH_VARARGS},
	{"Stop", (PyCFunction)audio_unit_stop, 
	 METH_VARARGS},
	{"SetStreamFormat", (PyCFunction)audio_unit_setstreamformat, 
	 METH_VARARGS},
	{"SetRenderCallback", (PyCFunction)audio_unit_setrendercallback, 
	 METH_VARARGS},
	{NULL, NULL}
};

static PyObject *
audio_unit_getattr(audio_unit_t *self, char *name) {
	return Py_FindMethod(audio_unit_methods, (PyObject *)self, name);
}

static PyTypeObject AudioUnitType = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,                              /*ob_size*/
	"coreaudio.AudioUnit",          /*tp_name*/
	sizeof(audio_unit_t),           /*tp_basicsize*/
	0,                              /*tp_itemsize*/
	/* methods */    
	(destructor)audio_unit_dealloc, /*tp_dealloc*/
	0,                              /*print*/
    (getattrfunc)audio_unit_getattr, /*tp_getattr*/
	0,                              /*tp_setattr*/
	0,                              /*tp_compare*/ 
	0,                              /*tp_repr*/
	0,                              /*tp_as_number*/
	0,                              /*tp_as_sequence*/
	0,                              /*tp_as_mapping*/
	0,                              /*tp_hash*/
	0,                              /*tp_call*/
	0,                              /*tp_str*/
	0,                              /*tp_getattro*/
	0,                              /*tp_setattro*/
	0,                              /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,             /*tp_flags*/
	"CoreFoundation AudioUnit",     /*tp_doc*/
};

static PyObject *
coreaudio_findnextcomponent(PyObject *self, PyObject *args) {
	component_t *component;
	component_desc_t *componentDescription;
	Component c; 
	component_t *retval;

	if (!PyArg_ParseTuple(args,"OO!:FindNextComponent", &component, 
						  &ComponentDescriptionType, &componentDescription)) 
						  return NULL;

	if (!PyArg_ParseTuple(args,"OO:FindNextComponent", &component, 
						  &componentDescription)) 
		return NULL;

	if ((PyObject*)component == Py_None)
		c = FindNextComponent(NULL, &componentDescription->desc);
	else
		c = FindNextComponent(component->component, 
							  &componentDescription->desc);

	if (!c)
	{
		Py_INCREF(Py_None);
		return Py_None;
	}

	if (!(retval = (component_t *)PyObject_New(component_t, &ComponentType))) 
		return NULL;

	retval->component = c;

	return (PyObject*)retval;
}

static PyObject *
coreaudio_openacomponent(PyObject *self, PyObject *args) {
	component_t *component;
	AudioUnit au;
	audio_unit_t *retval;
	OSErr rc;
	
	if (!PyArg_ParseTuple(args,"O!:OpenAComponent", &ComponentType, &component))
	   return NULL;

	rc = OpenAComponent(component->component, &au);
	if (rc != noErr)
	{
		PyErr_Format(CoreAudioError, "OpenAComponent failed: %4.4s", 
					 (char*)&rc);
		return NULL;
	}

	if (!(retval = (audio_unit_t *)PyObject_New(audio_unit_t, &AudioUnitType))) 
		return NULL;

	retval->instance = au;
	retval->render_callback = NULL;
	retval->user_data = NULL;

	return (PyObject*)retval;
}

static PyMethodDef coreaudio_methods[] = {
	{"FindNextComponent", (PyCFunction)coreaudio_findnextcomponent, 
	 METH_VARARGS},
	{"OpenAComponent", (PyCFunction)coreaudio_openacomponent, 
	 METH_VARARGS},
	{NULL, NULL}
};

#define _EXPORT_INT(mod, name) \
	if (PyModule_AddIntConstant(mod, #name, (long) name) == -1) return;

void initcoreaudio(void) {
	PyObject *m;

	PyEval_InitThreads();

	ComponentDescriptionType.tp_members = component_desc_members;
	ComponentDescriptionType.tp_new = component_desc_new;
	ComponentType.tp_new = component_new;
	AudioUnitType.tp_new = audio_unit_new;
	AudioStreamBasicDescType.tp_members = audio_stream_basic_desc_members;
	AudioStreamBasicDescType.tp_new = audio_stream_basic_desc_new;

	m = Py_InitModule3("coreaudio", coreaudio_methods, coreaudio_module_doc);
	
	CoreAudioError = PyErr_NewException("coreaudio.AudioError", NULL, NULL);
	if (CoreAudioError) {
		/* Each call to PyModule_AddObject decrefs it; compensate: */
		
		Py_INCREF(&ComponentType);
		PyModule_AddObject(m, "Component", (PyObject *)&ComponentType);

		Py_INCREF(&ComponentDescriptionType);
		PyModule_AddObject(m, "ComponentDescription", 
						   (PyObject *)&ComponentDescriptionType);

		Py_INCREF(&AudioStreamBasicDescType);
		PyModule_AddObject(m, "AudioStreamBasicDescription", 
						   (PyObject *)&AudioStreamBasicDescType);

		Py_INCREF(&AudioUnitType);
		PyModule_AddObject(m, "AudioUnit", (PyObject *)&AudioUnitType);
   	}
	
	_EXPORT_INT(m, kAudioUnitType_Output);
	_EXPORT_INT(m, kAudioUnitSubType_HALOutput);
	_EXPORT_INT(m, kAudioUnitSubType_DefaultOutput);
	_EXPORT_INT(m, kAudioUnitSubType_SystemOutput);
	_EXPORT_INT(m, kAudioUnitSubType_GenericOutput);

	_EXPORT_INT(m, kAudioUnitType_MusicDevice);
	_EXPORT_INT(m, kAudioUnitSubType_DLSSynth);

	_EXPORT_INT(m, kAudioUnitType_MusicEffect);

	_EXPORT_INT(m, kAudioUnitType_FormatConverter);
	_EXPORT_INT(m, kAudioUnitSubType_AUConverter);
	_EXPORT_INT(m, kAudioUnitSubType_Varispeed);
	_EXPORT_INT(m, kAudioUnitSubType_DeferredRenderer);
	_EXPORT_INT(m, kAudioUnitSubType_TimePitch);
	_EXPORT_INT(m, kAudioUnitSubType_Splitter);
	_EXPORT_INT(m, kAudioUnitSubType_Merger);
	
	_EXPORT_INT(m, kAudioUnitType_Effect);
	_EXPORT_INT(m, kAudioUnitSubType_Delay);
	_EXPORT_INT(m, kAudioUnitSubType_LowPassFilter);
	_EXPORT_INT(m, kAudioUnitSubType_HighPassFilter);
	_EXPORT_INT(m, kAudioUnitSubType_BandPassFilter);
	_EXPORT_INT(m, kAudioUnitSubType_HighShelfFilter);
	_EXPORT_INT(m, kAudioUnitSubType_LowShelfFilter);
	_EXPORT_INT(m, kAudioUnitSubType_ParametricEQ);
	_EXPORT_INT(m, kAudioUnitSubType_GraphicEQ);
	_EXPORT_INT(m, kAudioUnitSubType_PeakLimiter);
	_EXPORT_INT(m, kAudioUnitSubType_DynamicsProcessor);
	_EXPORT_INT(m, kAudioUnitSubType_MultiBandCompressor);
	_EXPORT_INT(m, kAudioUnitSubType_MatrixReverb);
	_EXPORT_INT(m, kAudioUnitSubType_SampleDelay);
	_EXPORT_INT(m, kAudioUnitSubType_Pitch);
	_EXPORT_INT(m, kAudioUnitSubType_AUFilter);
	_EXPORT_INT(m, kAudioUnitSubType_NetSend);
	
	_EXPORT_INT(m, kAudioUnitType_Mixer);
	_EXPORT_INT(m, kAudioUnitSubType_StereoMixer);
	_EXPORT_INT(m, kAudioUnitSubType_3DMixer);
	_EXPORT_INT(m, kAudioUnitSubType_MatrixMixer);

	_EXPORT_INT(m, kAudioUnitType_Panner);

	_EXPORT_INT(m, kAudioUnitType_OfflineEffect);

	_EXPORT_INT(m, kAudioUnitType_Generator);
	_EXPORT_INT(m, kAudioUnitSubType_ScheduledSoundPlayer);
	_EXPORT_INT(m, kAudioUnitSubType_AudioFilePlayer);
	_EXPORT_INT(m, kAudioUnitSubType_NetReceive);

	_EXPORT_INT(m, kAudioUnitManufacturer_Apple);

	_EXPORT_INT(m, kAudioFormatLinearPCM);
    _EXPORT_INT(m, kAudioFormatAC3);
    _EXPORT_INT(m, kAudioFormat60958AC3);
    _EXPORT_INT(m, kAudioFormatAppleIMA4);
    _EXPORT_INT(m, kAudioFormatMPEG4AAC);
    _EXPORT_INT(m, kAudioFormatMPEG4CELP);
    _EXPORT_INT(m, kAudioFormatMPEG4HVXC);
    _EXPORT_INT(m, kAudioFormatMPEG4TwinVQ);
    _EXPORT_INT(m, kAudioFormatMACE3);
    _EXPORT_INT(m, kAudioFormatMACE6);
    _EXPORT_INT(m, kAudioFormatULaw);
    _EXPORT_INT(m, kAudioFormatALaw);
    _EXPORT_INT(m, kAudioFormatQDesign);
    _EXPORT_INT(m, kAudioFormatQDesign2);
    _EXPORT_INT(m, kAudioFormatQUALCOMM);
    _EXPORT_INT(m, kAudioFormatMPEGLayer1);
    _EXPORT_INT(m, kAudioFormatMPEGLayer2);
    _EXPORT_INT(m, kAudioFormatMPEGLayer3);
    _EXPORT_INT(m, kAudioFormatDVAudio);
    _EXPORT_INT(m, kAudioFormatVariableDurationDVAudio);
    _EXPORT_INT(m, kAudioFormatTimeCode);
    _EXPORT_INT(m, kAudioFormatMIDIStream);
    _EXPORT_INT(m, kAudioFormatParameterValueStream);
    _EXPORT_INT(m, kAudioFormatAppleLossless);

    _EXPORT_INT(m, kAudioFormatFlagIsFloat);
    _EXPORT_INT(m, kAudioFormatFlagIsBigEndian);
    _EXPORT_INT(m, kAudioFormatFlagsNativeEndian);
    _EXPORT_INT(m, kAudioFormatFlagIsSignedInteger);
    _EXPORT_INT(m, kAudioFormatFlagIsPacked);
    _EXPORT_INT(m, kAudioFormatFlagIsAlignedHigh);
    _EXPORT_INT(m, kAudioFormatFlagIsNonInterleaved);
    _EXPORT_INT(m, kAudioFormatFlagIsNonMixable);
    _EXPORT_INT(m, kAudioFormatFlagsAreAllClear);
}
