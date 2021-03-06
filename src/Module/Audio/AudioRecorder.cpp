/*
 * AudioRecorder.cpp
 *
 *  Created on: 07.03.2019
 *      Author: michi
 */

#include "AudioRecorder.h"
#include "../Port/Port.h"
#include "../../Data/base.h"


int AudioRecorder::Output::read_audio(AudioBuffer& buf) {
	if (!rec->source)
		return buf.length;

	int r = rec->source->read_audio(buf);

	if (r > 0) {
		if (rec->accumulating) {
			std::lock_guard<std::mutex> lock(rec->mtx_buf);
			rec->buf.append(buf.ref(0, r));
		} else {
			rec->samples_skipped += r;
		}
	}

	return r;
}

AudioRecorder::Output::Output(AudioRecorder *r) : Port(SignalType::AUDIO, "out") {
	rec = r;
}

AudioRecorder::AudioRecorder() :
	Module(ModuleType::PLUMBING, "AudioRecorder")
{
	port_out.add(new Output(this));
	port_in.add({SignalType::AUDIO, &source, "in"});
	source = nullptr;
	accumulating = false;
}

void AudioRecorder::_accumulate(bool enable) {
	accumulating = enable;
}

void AudioRecorder::reset_state() {
	samples_skipped = 0;
}

int AudioRecorder::command(ModuleCommand cmd, int param) {
	if (cmd == ModuleCommand::ACCUMULATION_START) {
		_accumulate(true);
		return 0;
	} else if (cmd == ModuleCommand::ACCUMULATION_STOP) {
		_accumulate(false);
		return 0;
	} else if (cmd == ModuleCommand::ACCUMULATION_CLEAR) {
		samples_skipped += buf.length;
		buf.clear();
		return 0;
	} else if (cmd == ModuleCommand::ACCUMULATION_GET_SIZE) {
		return buf.length;
	}
	return COMMAND_NOT_HANDLED;
}
