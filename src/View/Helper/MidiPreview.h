/*
 * MidiPreview.h
 *
 *  Created on: 22.07.2018
 *      Author: michi
 */

#ifndef SRC_VIEW_HELPER_MIDIPREVIEW_H_
#define SRC_VIEW_HELPER_MIDIPREVIEW_H_

#include "../../lib/base/base.h"

class AudioOutput;
class Synthesizer;
class MidiPreviewSource;
class Module;
class SignalChain;
class MidiInput;
class Device;
class Session;


class MidiPreview : public VirtualBase {
public:
	MidiPreview(Session *s, Synthesizer *synth);
	virtual ~MidiPreview();

	SignalChain *chain;
	Module *synth;
	Module *joiner;
	Module *out;
	MidiPreviewSource *source;
	Module *recorder;
	Session *session;

	MidiInput *input;


	void _start_input();
	void _stop_input();
	void set_input_device(Device *d);
	Device *input_device;

	void start(const Array<int> &pitch, float volume, float ttl);
	void end();
};

#endif /* SRC_VIEW_HELPER_MIDIPREVIEW_H_ */
