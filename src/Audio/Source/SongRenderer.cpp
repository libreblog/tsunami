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
	midi_streamer = new MidiDataSource(no_midi);
	song = s;

	effect = NULL;
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

void SongRenderer::bb_render_audio_track_no_fx(AudioBuffer &buf, Track *t)
{
	// track buffer
	AudioBuffer buf0 = t->readBuffersCol(range_cur);
	buf.swap_ref(buf0);

	// subs
	for (SampleRef *s: t->samples){
		if (s->muted)
			continue;

		Range intersect_range;
		int bpos;
		if (!intersect_sub(s, range_cur, intersect_range, bpos))
			continue;

		buf.make_own();
		bpos = s->pos - range_cur.start();
		buf.add(*s->buf, bpos, s->volume * s->origin->volume, 0);
	}
}

void make_silence(AudioBuffer &buf, int size)
{
	if (buf.length == 0){
		buf.resize(size);
	}else{
		buf.resize(size);
		memset(buf.c[0].data, 0, size * sizeof(buf.c[0][0]));
		memset(buf.c[1].data, 0, size * sizeof(buf.c[1][0]));
	}
}

void SongRenderer::bb_render_time_track_no_fx(AudioBuffer &buf, Track *t)
{
	make_silence(buf, range_cur.length);

	Array<Beat> beats = song->bars.getBeats(range_cur, false, true);

	MidiRawData raw;
	raw.samples = buf.length;

	for (Beat &b: beats)
		raw.addMetronomeClick(b.range.offset - range_cur.offset, b.level, 0.8f);

	midi_streamer->setData(raw);
	t->synth->read(buf, midi_streamer);
}

void SongRenderer::bb_render_midi_track_no_fx(AudioBuffer &buf, Track *t, int ti)
{
	make_silence(buf, range_cur.length);

	MidiData *m = &t->midi;
//	if ((ti >= 0) and (ti < midi.num))
//		m = &midi[ti];
	// TODO

	MidiRawData raw = midi_notes_to_events(*m);

	MidiRawData events;
	raw.read(events, range_cur);

	midi_streamer->setData(events);
	t->synth->read(buf, midi_streamer);
}

void SongRenderer::bb_render_track_no_fx(AudioBuffer &buf, Track *t, int ti)
{
	if (t->type == Track::TYPE_AUDIO)
		bb_render_audio_track_no_fx(buf, t);
	else if (t->type == Track::TYPE_TIME)
		bb_render_time_track_no_fx(buf, t);
	else if (t->type == Track::TYPE_MIDI)
		bb_render_midi_track_no_fx(buf, t, ti);
}

void SongRenderer::make_fake_track(Track *t, AudioBuffer &buf)
{
	//msg_write("fake track");
	t->song = song;
	t->layers.resize(1);
	t->layers[0].buffers.resize(1);
	t->layers[0].buffers[0].set_as_ref(buf, 0, range_cur.length);
}

void SongRenderer::bb_apply_fx(AudioBuffer &buf, Track *t, Array<Effect*> &fx_list)
{
	buf.make_own();

	Track fake_track = Track(Track::TYPE_AUDIO, NULL);
	make_fake_track(&fake_track, buf);

	// apply preview plugin?
	if (t and effect)
		effect->apply(buf, &fake_track, false);

	// apply fx
	for (Effect *fx: fx_list)
		if (fx->enabled)
			fx->apply(buf, &fake_track, false);
}

void SongRenderer::bb_render_track_fx(AudioBuffer &buf, Track *t, int ti)
{
	bb_render_track_no_fx(buf, t, ti);

	if ((t->fx.num > 0) or effect)
		bb_apply_fx(buf, t, t->fx);
}

int get_first_usable_track(Song *s)
{
	foreachi(Track *t, s->tracks, i)
		if (!t->muted)
			return i;
	return -1;
}

void SongRenderer::bb_render_song_no_fx(AudioBuffer &buf)
{
	// any un-muted track?
	int i0 = get_first_usable_track(song);
	if (i0 < 0){
		// no -> return silence
		buf.resize(range_cur.length);
	}else{

		// first (un-muted) track
		bb_render_track_fx(buf, song->tracks[i0], i0);
		buf.make_own();
		buf.scale(song->tracks[i0]->volume, song->tracks[i0]->panning);

		// other tracks
		for (int i=i0+1;i<song->tracks.num;i++){
			if (song->tracks[i]->muted)
				continue;
			AudioBuffer tbuf;
			bb_render_track_fx(tbuf, song->tracks[i], i);
			buf.make_own();
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

void SongRenderer::read_basic(AudioBuffer &buf, int pos, int size)
{
	range_cur = Range(pos, size);

	apply_curves(song, pos);

	// render without fx
	bb_render_song_no_fx(buf);

	// apply global fx
	if (song->fx.num > 0)
		bb_apply_fx(buf, NULL, song->fx);

	unapply_curves(song);
}

int SongRenderer::read(AudioBuffer &buf)
{
	int size = max(min(buf.length, _range.end() - pos), 0);

	if (song->curves.num >= 0){
		buf.resize(size);
		int chunk = 128;
		for (int d=0; d<size; d+=chunk){
			AudioBuffer tbuf;
			read_basic(tbuf, pos + d, min(size - d, chunk));
			buf.set(tbuf, d, 1.0f);
		}
	}else
		read_basic(buf, pos, size);

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

void SongRenderer::prepare(const Range &__range, bool _allow_loop)
{
	_range = __range;
	allow_loop = _allow_loop;
	pos = _range.offset;
	midi.clear();

	reset();
}

void SongRenderer::reset()
{
	for (Effect *fx: song->fx)
		fx->prepare();
	foreachi(Track *t, song->tracks, i){
		//midi.add(t, t->midi);
		midi.add(t->midi);
		t->synth->setSampleRate(song->sample_rate);
		t->synth->setInstrument(t->instrument);
		t->synth->reset();
		for (Effect *fx: t->fx)
			fx->prepare();
		for (MidiEffect *fx: t->midi.fx){
			fx->prepare();
			fx->process(&midi[i]);
		}
	}
	if (effect)
		effect->prepare();
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

Array<Tag> SongRenderer::getTags()
{
	return song->tags;
}

void SongRenderer::seek(int _pos)
{
	pos = _pos;
	for (Track *t: song->tracks)
		t->synth->reset();//endAllNotes();
}
