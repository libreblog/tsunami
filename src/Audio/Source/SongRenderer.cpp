/*
 * SongRenderer.cpp
 *
 *  Created on: 17.08.2015
 *      Author: michi
 */

#include "../Source/SongRenderer.h"

#include "../Synth/Synthesizer.h"
#include "../../Plugins/Effect.h"
#include "../../Plugins/MidiEffect.h"
#include "../../Plugins/PluginManager.h"
#include "../../Data/Curve.h"
#include "../../Data/SongSelection.h"
#include "../../Midi/MidiSource.h"
#include "../../Tsunami.h"

#include "../../lib/math/math.h"

SongRenderer::SongRenderer(Song *s)
{
	MidiRawData no_midi;
	midi_streamer = new MidiDataStreamer(no_midi);
	song = s;

	preview_effect = NULL;
	allow_loop = false;
	loop_if_allowed = false;
	pos = 0;
	prepare(s->getRange(), false);
}

SongRenderer::~SongRenderer()
{
	delete(midi_streamer);
}

void SongRenderer::__init__(Song *s)
{
	new(this) SongRenderer(s);
}

void SongRenderer::__delete__()
{
	this->SongRenderer::~SongRenderer();
}

bool intersect_sub(SampleRef *s, const Range &r, Range &ir, int &bpos)
{
	// intersected intervall (track-coordinates)
	int i0 = max(s->pos, r.start());
	int i1 = min(s->pos + s->buf->length, r.end());

	// beginning of the intervall (relative to sub)
	ir.offset = i0 - s->pos;
	// ~ (relative to old intervall)
	bpos = i0 - r.start();
	ir.length = i1 - i0;

	return !ir.empty();
}

void SongRenderer::render_audio_track_no_fx(AudioBuffer &buf, Track *t)
{
	// track buffer
	t->readBuffersCol(buf, range_cur.offset);

	// subs
	for (SampleRef *s: t->samples){
		if (s->muted)
			continue;

		Range intersect_range;
		int bpos;
		if (!intersect_sub(s, range_cur, intersect_range, bpos))
			continue;

		bpos = s->pos - range_cur.start();
		buf.add(*s->buf, bpos, s->volume * s->origin->volume, 0);
	}
}

void SongRenderer::render_time_track_no_fx(AudioBuffer &buf, Track *t)
{
	Array<Beat> beats = song->bars.getBeats(range_cur, false, true);

	MidiRawData raw;
	raw.samples = buf.length;

	for (Beat &b: beats)
		raw.addMetronomeClick(b.range.offset - range_cur.offset, b.level, 0.8f);

	midi_streamer->setData(raw);
	t->synth->out->setSource(midi_streamer);
	t->synth->out->read(buf);
}

void SongRenderer::render_midi_track_no_fx(AudioBuffer &buf, Track *t, int ti)
{
	MidiData *m = &t->midi;
	if (ti < midi.num)
		m = &midi[ti];
	// TODO

	MidiRawData raw = midi_notes_to_events(*m);

	MidiRawData events;
	raw.read(events, range_cur);

	midi_streamer->setData(events);
	t->synth->out->setSource(midi_streamer);
	t->synth->out->read(buf);
}

void SongRenderer::render_track_no_fx(AudioBuffer &buf, Track *t, int ti)
{
	if (t->type == Track::TYPE_AUDIO)
		render_audio_track_no_fx(buf, t);
	else if (t->type == Track::TYPE_TIME)
		render_time_track_no_fx(buf, t);
	else if (t->type == Track::TYPE_MIDI)
		render_midi_track_no_fx(buf, t, ti);
}

void SongRenderer::apply_fx(AudioBuffer &buf, Track *t, Array<Effect*> &fx_list)
{
	// apply fx
	for (Effect *fx: fx_list)
		if (fx->enabled)
			fx->process(buf);
}

void SongRenderer::render_track_fx(AudioBuffer &buf, Track *t, int ti)
{
	render_track_no_fx(buf, t, ti);

	Array<Effect*> fx = t->fx;
	if (preview_effect)
		fx.add(preview_effect);
	if (fx.num > 0)
		apply_fx(buf, t, fx);
}

int get_first_usable_track(Song *s, Set<Track*> &allowed)
{
	foreachi(Track *t, s->tracks, i)
		if (!t->muted and (allowed.find(t) >= 0))
			return i;
	return -1;
}

void SongRenderer::render_song_no_fx(AudioBuffer &buf)
{
	// any un-muted track?
	int i0 = get_first_usable_track(song, allowed_tracks);
	if (i0 < 0){
		// no -> return silence
		buf.scale(0);
	}else{

		// first (un-muted) track
		render_track_fx(buf, song->tracks[i0], i0);
		buf.scale(song->tracks[i0]->volume, song->tracks[i0]->panning);

		// other tracks
		AudioBuffer tbuf;
		for (int i=i0+1;i<song->tracks.num;i++){
			if (allowed_tracks.find(song->tracks[i]) < 0)
				continue;
			if (song->tracks[i]->muted)
				continue;
			tbuf.resize(buf.length);
			render_track_fx(tbuf, song->tracks[i], i);
			buf.add(tbuf, 0, song->tracks[i]->volume, song->tracks[i]->panning);
		}

		buf.scale(song->volume);
	}
}

void apply_curves(Song *audio, int pos)
{
	for (Curve *c: audio->curves)
		c->apply(pos);
}

void unapply_curves(Song *audio)
{
	for (Curve *c: audio->curves)
		c->unapply();
}

void SongRenderer::read_basic(AudioBuffer &buf, int pos)
{
	range_cur = Range(pos, buf.length);

	apply_curves(song, pos);

	// render without fx
	render_song_no_fx(buf);

	// apply global fx
	if (song->fx.num > 0)
		apply_fx(buf, NULL, song->fx);

	unapply_curves(song);
}

int SongRenderer::read(AudioBuffer &buf)
{
	int size = min(buf.length, _range.end() - pos);
	if (size <= 0)
		return END_OF_STREAM;

	if (song->curves.num >= 0){
		int chunk = 128;
		for (int d=0; d<size; d+=chunk){
			AudioBuffer tbuf;
			tbuf.set_as_ref(buf, d, min(size - d, chunk));
			read_basic(tbuf, pos + d);
			buf.set(tbuf, d, 1.0f);
		}
	}else
		read_basic(buf, pos);

	buf.offset = pos;
	pos += size;
	if ((pos >= _range.end()) and allow_loop and loop_if_allowed)
		seek(_range.offset);
	return size;
}

void SongRenderer::render(const Range &range, AudioBuffer &buf)
{
	prepare(range, false);
	buf.resize(range.length);
	read(buf);
}

void SongRenderer::allowTracks(const Set<Track*> &_allowed_tracks)
{
	allowed_tracks = _allowed_tracks;
}

void SongRenderer::prepare(const Range &__range, bool _allow_loop)
{
	_range = __range;
	allow_loop = _allow_loop;
	pos = _range.offset;
	midi.clear();
	allowed_tracks.clear();
	for (Track* t: song->tracks)
		allowed_tracks.add(t);

	reset();
}

void SongRenderer::reset()
{
	for (Effect *fx: song->fx)
		fx->resetState();
	foreachi(Track *t, song->tracks, i){
		//midi.add(t, t->midi);
		MidiData _midi = t->midi;
		for (auto c: t->samples)
			if (c->type() == t->TYPE_MIDI)
				_midi.append(*c->midi, c->pos);
		midi.add(_midi);
		t->synth->setSampleRate(song->sample_rate);
		t->synth->setInstrument(t->instrument);
		t->synth->reset();
		for (Effect *fx: t->fx)
			fx->resetState();
		for (MidiEffect *fx: t->midi.fx){
			fx->prepare();
			fx->process(&midi[i]);
		}
	}
	if (preview_effect)
		preview_effect->resetState();
}

int SongRenderer::getSampleRate()
{
	return song->sample_rate;
}

int SongRenderer::getNumSamples()
{
	if (allow_loop and loop_if_allowed)
		return -1;
	return _range.length;
}

void SongRenderer::seek(int _pos)
{
	pos = _pos;
	for (Track *t: song->tracks)
		t->synth->reset();//endAllNotes();
}

