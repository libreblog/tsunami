/*
 * CaptureConsoleModeMidi.h
 *
 *  Created on: 24.09.2017
 *      Author: michi
 */

#ifndef SRC_VIEW_SIDEBAR_CAPTURECONSOLEMODES_CAPTURECONSOLEMODEMIDI_H_
#define SRC_VIEW_SIDEBAR_CAPTURECONSOLEMODES_CAPTURECONSOLEMODEMIDI_H_

#include "CaptureConsoleMode.h"
#include "../../../lib/base/base.h"

class MidiInput;
class Device;
class Track;
class Synthesizer;
class AudioOutput;
class PeakMeter;

class CaptureConsoleModeMidi : public CaptureConsoleMode {
	MidiInput *input;
	Array<Device*> sources;
	const Track *target;
	Synthesizer *preview_synth;
	PeakMeter *peak_meter;
	AudioOutput *preview_stream;



public:
	CaptureConsoleModeMidi(CaptureConsole *cc);
	void on_source();
	void on_target();
	void set_target(const Track *t);
	void enter() override;
	void leave() override;
	void allow_change_device(bool allow) override;

	void update_device_list();
};



#endif /* SRC_VIEW_SIDEBAR_CAPTURECONSOLEMODES_CAPTURECONSOLEMODEMIDI_H_ */
