/*
 * DeviceManager.cpp
 *
 *  Created on: 26.03.2012
 *      Author: michi
 */

#include "../Tsunami.h"
#include "../Stuff/Log.h"
#include "Device.h"

#include <pulse/pulseaudio.h>
#include <alsa/asoundlib.h>

#include "DeviceManager.h"
#include "OutputStream.h"

const string DeviceManager::MESSAGE_ADD_DEVICE = "AddDevice";
const string DeviceManager::MESSAGE_REMOVE_DEVICE = "RemoveDevice";


void pa_wait_op(pa_operation *op)
{
	if (!op){
		tsunami->log->error("pa_wait_op:  op=nil");
		return;
	}
	//msg_write("wait op " + p2s(op));
	while (pa_operation_get_state(op) != PA_OPERATION_DONE){
		// PA_OPERATION_RUNNING
		//pa_mainloop_iterate(m, 1, NULL);
		if (pa_operation_get_state(op) == PA_OPERATION_CANCELLED)
			break;
	}
	if (pa_operation_get_state(op) != PA_OPERATION_DONE)
		tsunami->log->error("pa_wait_op: failed");
	pa_operation_unref(op);
	//msg_write(" ok");
}

void pa_subscription_callback(pa_context *c, pa_subscription_event_type_t t, uint32_t idx, void *userdata)
{
	//msg_write(format("event  %d  %d", (t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK), (t & PA_SUBSCRIPTION_EVENT_TYPE_MASK)));

	DeviceManager *out = (DeviceManager*)userdata;

	if (((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_NEW) or ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_REMOVE)){
		//printf("----change   %d\n", idx);

		HuiRunLaterM(0.1f, out, &DeviceManager::update_devices);
	}
}


Array<Device*> str2devs(const string &s, int type)
{
	Array<Device*> devices;
	Array<string> a = s.explode("|");
	foreach(string &b, a)
		devices.add(new Device(type, b));
	return devices;
}

string devs2str(Array<Device*> devices)
{
	string r;
	foreachi(Device *d, devices, i){
		if (i > 0)
			r += "|";
		r += d->to_config();
	}
	return r;
}


DeviceManager::DeviceManager() :
	Observable("AudioOutput")
{
	initialized = false;

	output_devices = str2devs(HuiConfig.getStr("Output.Devices", ""), Device::TYPE_AUDIO_OUTPUT);
	input_devices = str2devs(HuiConfig.getStr("Input.Devices", ""), Device::TYPE_AUDIO_INPUT);
	midi_input_devices = str2devs(HuiConfig.getStr("MidiInput.Devices", ""), Device::TYPE_MIDI_INPUT);

	output_volume = HuiConfig.getFloat("Output.Volume", 1.0f);


	context = NULL;
	handle = NULL;

	// system defaults
	default_devices[Device::TYPE_AUDIO_OUTPUT] = get_device_create(Device::TYPE_AUDIO_OUTPUT, ":default:");
	default_devices[Device::TYPE_AUDIO_OUTPUT]->channels = 2;
	default_devices[Device::TYPE_AUDIO_INPUT] = get_device_create(Device::TYPE_AUDIO_INPUT, ":default:");
	default_devices[Device::TYPE_AUDIO_INPUT]->channels = 2;
	default_devices[Device::TYPE_MIDI_INPUT] = get_device_create(Device::TYPE_MIDI_INPUT, ":default:");

	init();

	hui_rep_id = HuiRunRepeatedM(2.0f, this, &DeviceManager::update_midi_devices);
}

DeviceManager::~DeviceManager()
{
	HuiCancelRunner(hui_rep_id);

	write_config();
	kill();

	foreach(Device *d, output_devices)
		delete(d);
	foreach(Device *d, input_devices)
		delete(d);
	foreach(Device *d, midi_input_devices)
		delete(d);
}

void DeviceManager::remove_device(int type, int index)
{
	Array<Device*> &devices = getDeviceList(type);
	if ((index < 0) or (index >= devices.num))
		return;
	if (devices[index]->present)
		return;
	delete(devices[index]);
	devices.erase(index);

	write_config();
	msg_type = type;
	msg_index = index;
	notify(MESSAGE_REMOVE_DEVICE);
}

void DeviceManager::write_config()
{
	//HuiConfig.setStr("Output.ChosenDevice", chosen_device);
	HuiConfig.setFloat("Output.Volume", output_volume);
	HuiConfig.setStr("Output.Devices", devs2str(output_devices));
	HuiConfig.setStr("Input.Devices", devs2str(input_devices));
	HuiConfig.setStr("MidiInput.Devices", devs2str(midi_input_devices));
}



void pa_sink_info_callback(pa_context *c, const pa_sink_info *i, int eol, void *userdata)
{
	if (eol > 0 or !i or !userdata)
		return;

	//printf("output  %s ||  %s   %d   %d\n", i->name, i->description, i->index, i->channel_map.channels);

	DeviceManager *dm = (DeviceManager*)userdata;
	Device *d = dm->get_device_create(Device::TYPE_AUDIO_OUTPUT, i->name);
	d->name = i->description;
	d->channels = i->channel_map.channels;
	d->present = true;
	dm->setDeviceConfig(d);
}

void pa_source_info_callback(pa_context *c, const pa_source_info *i, int eol, void *userdata)
{
	if (eol > 0 or !i or !userdata)
		return;

	//printf("input  %s ||  %s   %d   %d\n", i->name, i->description, i->index, i->channel_map.channels);

	DeviceManager *dm = (DeviceManager*)userdata;
	Device *d = dm->get_device_create(Device::TYPE_AUDIO_INPUT, i->name);
	d->name = i->description;
	d->channels = i->channel_map.channels;
	d->present = true;
	dm->setDeviceConfig(d);
}

void DeviceManager::update_devices()
{
	foreach(Device *d, output_devices)
		d->present = false;

	pa_operation *op = pa_context_get_sink_info_list(context, pa_sink_info_callback, this);
	if (!testError("pa_context_get_sink_info_list"))
		pa_wait_op(op);

	default_devices[Device::TYPE_AUDIO_OUTPUT]->present = true;


	foreach(Device *d, input_devices)
		d->present = false;

	op = pa_context_get_source_info_list(context, pa_source_info_callback, this);
	if (!testError("pa_context_get_source_info_list"))
		pa_wait_op(op);


	default_devices[Device::TYPE_AUDIO_INPUT]->present = true;


	update_midi_devices();

	notify(MESSAGE_CHANGE);
	write_config();
}

void DeviceManager::update_midi_devices()
{
	foreach(Device *d, midi_input_devices){
		d->present_old = d->present;
		d->present = false;
	}

	snd_seq_client_info_t *cinfo;
	snd_seq_port_info_t *pinfo;

	snd_seq_client_info_alloca(&cinfo);
	snd_seq_port_info_alloca(&pinfo);
	snd_seq_client_info_set_client(cinfo, -1);
	while (snd_seq_query_next_client(handle, cinfo) >= 0){
		snd_seq_port_info_set_client(pinfo, snd_seq_client_info_get_client(cinfo));
		snd_seq_port_info_set_port(pinfo, -1);
		while (snd_seq_query_next_port(handle, pinfo) >= 0){
			if ((snd_seq_port_info_get_capability(pinfo) & SND_SEQ_PORT_CAP_READ) == 0)
				continue;
			if ((snd_seq_port_info_get_capability(pinfo) & SND_SEQ_PORT_CAP_SUBS_READ) == 0)
				continue;
			Device *d = get_device_create(Device::TYPE_MIDI_INPUT, format("%s/%s", snd_seq_client_info_get_name(cinfo), snd_seq_port_info_get_name(pinfo)));
			d->name = d->internal_name;
			d->client = snd_seq_client_info_get_client(cinfo);
			d->port = snd_seq_port_info_get_port(pinfo);
			d->present = true;
		}
	}

	default_devices[Device::TYPE_MIDI_INPUT]->present = true;


	bool changed = false;
	foreach(Device *d, midi_input_devices)
		if (d->present_old != d->present)
			changed = true;
	if (changed)
		notify(MESSAGE_CHANGE);
}


bool pa_wait_context_ready(pa_context *c)
{
	//msg_write("wait stream ready");
	int n = 0;
	while (pa_context_get_state(c) != PA_CONTEXT_READY){
		//pa_mainloop_iterate(m, 1, NULL);
		HuiSleep(0.01f);
		n ++;
		if (n >= 500)
			return false;
		if (pa_context_get_state(c) == PA_CONTEXT_FAILED)
			return false;
	}
	//msg_write("ok");
	return true;
}

void DeviceManager::init()
{
	if (initialized)
		return;
	msg_db_f("Output.init", 1);

	// audio
	pa_threaded_mainloop* m = pa_threaded_mainloop_new();
	if (!m){
		tsunami->log->error("pa_threaded_mainloop_new failed");
		return;
	}

	pa_mainloop_api *mainloop_api = pa_threaded_mainloop_get_api(m);
	if (!m){
		tsunami->log->error("pa_threaded_mainloop_get_api failed");
		return;
	}

	context = pa_context_new(mainloop_api, "tsunami");
	if (testError("pa_context_new"))
		return;

	pa_context_connect(context, NULL, (pa_context_flags_t)0, NULL);
	if (testError("pa_context_connect"))
		return;

	pa_threaded_mainloop_start(m);
	if (testError("pa_threaded_mainloop_start"))
		return;

	if (!pa_wait_context_ready(context)){
		tsunami->log->error("pulse audio context does not turn 'ready'");
		return;
	}

	pa_context_set_subscribe_callback(context, &pa_subscription_callback, this);
	testError("pa_context_set_subscribe_callback");
	pa_context_subscribe(context, (pa_subscription_mask_t)(PA_SUBSCRIPTION_MASK_SINK | PA_SUBSCRIPTION_MASK_SOURCE), NULL, this);
	testError("pa_context_subscribe");



	// midi
	int r = snd_seq_open(&handle, "hw", SND_SEQ_OPEN_DUPLEX, SND_SEQ_NONBLOCK);
	if (r < 0){
		tsunami->log->error(string("Error opening ALSA sequencer: ") + snd_strerror(r));
		return;
	}
	snd_seq_set_client_name(handle, "Tsunami");
	portid = snd_seq_create_simple_port(handle, "Tsunami MIDI in",
				SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE,
				SND_SEQ_PORT_TYPE_APPLICATION);
	if (portid < 0){
		tsunami->log->error(string("Error creating sequencer port: ") + snd_strerror(portid));
		return;
	}

	update_devices();

	initialized = true;
}

void DeviceManager::kill()
{
	if (!initialized)
		return;
	msg_db_f("Output.kill",1);

	foreach(OutputStream *s, streams)
		s->kill();

	// audio
	if (context){
		pa_context_disconnect(context);
		testError("pa_context_disconnect");
	}

	// midi
	if (handle)
		snd_seq_close(handle);

	initialized = false;
}


float DeviceManager::getOutputVolume()
{
	return output_volume;
}

void DeviceManager::setOutputVolume(float _volume)
{
	output_volume = _volume;
	notify(MESSAGE_CHANGE);
}

void DeviceManager::addStream(OutputStream* s)
{
	streams.add(s);
}

void DeviceManager::removeStream(OutputStream* s)
{
	for (int i=streams.num-1; i>=0; i--)
		if (streams[i] == s)
			streams.erase(i);
}

bool DeviceManager::streamExists(OutputStream* s)
{
	for (int i=streams.num-1; i>=0; i--)
		if (streams[i] == s)
			return true;
	return false;
}

Device* DeviceManager::get_device(int type, const string &internal_name)
{
	Array<Device*> &devices = getDeviceList(type);
	foreach(Device *d, devices)
		if (d->internal_name == internal_name)
			return d;
	return NULL;
}

Device* DeviceManager::get_device_create(int type, const string &internal_name)
{
	Array<Device*> &devices = getDeviceList(type);
	foreach(Device *d, devices)
		if (d->internal_name == internal_name)
			return d;
	Device *d = new Device(type, "", internal_name, 0);
	devices.add(d);
	msg_type = type;
	msg_index = devices.num - 1;
	notify(MESSAGE_ADD_DEVICE);
	return d;
}

Array<Device*> &DeviceManager::getDeviceList(int type)
{
	if (type == Device::TYPE_AUDIO_OUTPUT)
		return output_devices;
	if (type == Device::TYPE_AUDIO_INPUT)
		return input_devices;
	if (type == Device::TYPE_MIDI_INPUT)
		return midi_input_devices;
	return empty_device_list;
}

Array<Device*> DeviceManager::getGoodDeviceList(int type)
{
	Array<Device*> &all = getDeviceList(type);
	Array<Device*> list;
	foreach(Device *d, all)
		if (d->visible and d->present)
			list.add(d);
	return list;
}

Device *DeviceManager::chooseDevice(int type)
{
	Array<Device*> &devices = getDeviceList(type);
	foreach(Device *d, devices)
		if (d->present and d->visible)
			return d;
	return NULL;
}

void DeviceManager::setDeviceConfig(Device *d)
{
	/*Device *dd = get_device(d.type, d.internal_name);
	if (dd){
		dd->name = d.name;
		dd->present = d.present;
		dd->visible = d.visible;
		dd->latency = d.latency;
		dd->client = d.client;
		dd->port = d.port;
		dd->channels = d.channels;
	}else{
		getDeviceList(d.type).add(d);
	}*/
	write_config();
	notify(MESSAGE_CHANGE);
}

void DeviceManager::makeDeviceTopPriority(Device *d)
{
	Array<Device*> &devices = getDeviceList(d->type);
	for (int i=0; i<devices.num; i++)
		if (devices[i] == d){
			devices.insert(d, 0);
			devices.erase(i + 1);
			break;
		}
	write_config();
	notify(MESSAGE_CHANGE);
}

bool DeviceManager::testError(const string &msg)
{
	int e = pa_context_errno(context);
	if (e != 0)
		tsunami->log->error(msg + ": " + pa_strerror(e));
	return (e != 0);
}

