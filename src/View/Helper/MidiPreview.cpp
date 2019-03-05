/*
 * MidiPreview.cpp
 *
 *  Created on: 22.07.2018
 *      Author: michi
 */

#include "MidiPreview.h"
#include "../../Session.h"
#include "../../Module/Synth/Synthesizer.h"
#include "../../Module/Midi/MidiSource.h"
#include "../../Device/OutputStream.h"


class MidiPreviewSource : public MidiSource
{
public:
	MidiPreviewSource()
	{
		mode = Mode::WAITING;
		ttl = -1;
		volume = 1.0f;
		debug = false;
	}

	bool debug;
	void o(const string &str)
	{
		if (debug)
			msg_write(str);
	}

	int read(MidiEventBuffer &midi) override
	{
		std::lock_guard<std::mutex> lock(mutex);

		o("mps.read");
		if (mode == Mode::END_OF_STREAM){
			o("  - end of stream");
			return Port::END_OF_STREAM;
		}

		if (mode == Mode::START_NOTES){
			for (int p: pitch_queued)
				midi.add(MidiEvent(0, p, volume));
			pitch_active = pitch_queued;
			set_mode(Mode::ACTIVE_NOTES);
		}else if (mode == Mode::END_NOTES){
			for (int p: pitch_active)
				midi.add(MidiEvent(0, p, 0));
			set_mode(Mode::END_OF_STREAM);
		}else if (mode == Mode::CHANGE_NOTES){
			for (int p: pitch_active)
				midi.add(MidiEvent(0, p, 0));
			for (int p: pitch_queued)
				midi.add(MidiEvent(0, p, volume));
			pitch_active = pitch_queued;
			set_mode(Mode::ACTIVE_NOTES);
		}
		if (mode == Mode::ACTIVE_NOTES){
			ttl -= midi.samples;
			if (ttl < 0)
				set_mode(Mode::END_NOTES);
		}
		return midi.samples;
	}
	int mode;
	enum Mode{
		WAITING,
		START_NOTES,
		CHANGE_NOTES,
		ACTIVE_NOTES,
		END_NOTES,
		END_OF_STREAM
	};
	string mode_str(int mode)
	{
		if (mode == Mode::WAITING)
			return "wait";
		if (mode == Mode::START_NOTES)
			return "start";
		if (mode == Mode::CHANGE_NOTES)
			return "change";
		if (mode == Mode::ACTIVE_NOTES)
			return "active";
		if (mode == Mode::END_NOTES)
			return "end";
		if (mode == Mode::END_OF_STREAM)
			return "eos";
		return "???";
	}
	void set_mode(int new_mode)
	{
		o("    " + mode_str(mode) + " -> " + mode_str(new_mode));
		mode = new_mode;
	}
	int ttl;

	Array<int> pitch_active;
	Array<int> pitch_queued;
	float volume;


	void start(const Array<int> &_pitch, int _ttl)
	{
		std::lock_guard<std::mutex> lock(mutex);
		o("START");

		pitch_queued = _pitch;
		ttl = _ttl;

		if ((mode == Mode::WAITING) or (mode == Mode::END_OF_STREAM)){
			set_mode(Mode::START_NOTES);
		}else if ((mode == Mode::END_NOTES) or (mode == Mode::ACTIVE_NOTES) or (mode == Mode::CHANGE_NOTES)){
			set_mode(Mode::CHANGE_NOTES);
		}
	}
	void end()
	{
		o("END");
		std::lock_guard<std::mutex> lock(mutex);

		if (mode == Mode::START_NOTES){
			// skip queue
			set_mode(Mode::WAITING);
		}else if ((mode == Mode::ACTIVE_NOTES) or (mode == Mode::CHANGE_NOTES)){
			set_mode(Mode::END_NOTES);
		}
	}

private:
	std::mutex mutex;
};

MidiPreview::MidiPreview(Session *s)
{
	session = s;
	source = new MidiPreviewSource;
	stream = nullptr;
	synth = nullptr;
}

MidiPreview::~MidiPreview()
{
	if (stream)
		delete stream;
	if (synth)
		delete synth;
	delete source;
}

void MidiPreview::start(Synthesizer *s, const Array<int> &pitch, float volume, float ttl)
{
	//kill_preview();
	if (!synth){
		synth = (Synthesizer*)s->copy();
//		synth->setInstrument(view->cur_track->instrument);
		synth->plug(0, source, 0);
	}
	if (!stream){
		stream = new OutputStream(session);
		stream->plug(0, synth, 0);
		stream->subscribe(this, [&]{ on_end_of_stream(); }, stream->MESSAGE_PLAY_END_OF_STREAM);
	}
	stream->set_volume(volume);

	source->start(pitch, session->sample_rate() * ttl);
	stream->play();
}

void MidiPreview::end()
{
	source->end();
}

void MidiPreview::on_end_of_stream()
{
	//msg_write("preview: end of stream");
	stream->stop();
	//hui::RunLater(0.001f,  std::bind(&ViewModeMidi::kill_preview, this));
}

void MidiPreview::kill()
{
	//msg_write("kill");
	if (stream){
		stream->stop();
		delete stream;
	}
	stream = nullptr;
	if (synth)
		delete synth;
	synth = nullptr;
}

