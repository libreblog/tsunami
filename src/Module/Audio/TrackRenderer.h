/*
 * TrackRenderer.h
 *
 *  Created on: 02.09.2018
 *      Author: michi
 */

#ifndef SRC_MODULE_AUDIO_TRACKRENDERER_H_
#define SRC_MODULE_AUDIO_TRACKRENDERER_H_

#include "../../lib/base/base.h"

class Track;
class AudioEffect;
class Synthesizer;
class MidiEventStreamer;
class SongRenderer;
class AudioBuffer;

class TrackRenderer : public VirtualBase
{
	friend SongRenderer;
public:
	TrackRenderer(Track *t, SongRenderer *sr);
	virtual ~TrackRenderer();

	Track *track;
	Array<AudioEffect*> fx;
	Synthesizer *synth;
	MidiEventStreamer* midi_streamer;
	SongRenderer *song_renderer;

	void seek(int pos);

	void render_audio_no_fx(AudioBuffer &buf);
	void render_time_no_fx(AudioBuffer &buf);
	void render_midi_no_fx(AudioBuffer &buf);
	void render_no_fx(AudioBuffer &buf);
	void render_fx(AudioBuffer &buf);

	static void apply_fx(AudioBuffer &buf, Array<AudioEffect*> &fx_list);

	void reset_state();

	void on_track_replace_synth();
	void on_track_add_or_delete_fx();
	void on_track_change_data();
};

#endif /* SRC_MODULE_AUDIO_TRACKRENDERER_H_ */
