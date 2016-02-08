/*
 * SongRenderer.cpp
 *
 *  Created on: 17.08.2015
 *      Author: michi
 */

#include "SongRenderer.h"
#include "../Synth/Synthesizer.h"
#include "../../Plugins/Effect.h"
#include "../../Plugins/MidiEffect.h"
#include "../../Plugins/PluginManager.h"
#include "../../Data/Curve.h"
#include "../../Tsunami.h"

#include "../../lib/math/math.h"

SongRenderer::SongRenderer(Song *s)
{
	song = s;
	effect = NULL;
	allow_loop = false;
	loop_if_allowed = false;
	pos = 0;
	prepare(s->getRange(), false);
}

SongRenderer::~SongRenderer()
{
}

void SongRenderer::__init__(Song *s)
{
	new(this) SongRenderer(s);
}

void SongRenderer::__delete__()
{
	midi.clear();
}

bool intersect_sub(SampleRef *s, const Range &r, Range &ir, int &bpos)
{
	// intersected intervall (track-coordinates)
	int i0 = max(s->pos, r.start());
	int i1 = min(s->pos + s->buf->num, r.end());

	// beginning of the intervall (relative to sub)
	ir.offset = i0 - s->pos;
	// ~ (relative to old intervall)
	bpos = i0 - r.start();
	ir.num = i1 - i0;

	return !ir.empty();
}

void SongRenderer::bb_render_audio_track_no_fx(BufferBox &buf, Track *t)
{
	msg_db_f("bb_render_audio_track_no_fx", 1);

	// track buffer
	BufferBox buf0 = t->readBuffersCol(range_cur);
	buf.swap_ref(buf0);

	// subs
	foreach(SampleRef *s, t->samples){
		if (s->muted)
			continue;

		// can be repetitious!
		for (int i=0;i<s->rep_num+1;i++){
			Range rep_range = range_cur;
			rep_range.move(-s->rep_delay * i);
			Range intersect_range;
			int bpos;
			if (!intersect_sub(s, rep_range, intersect_range, bpos))
				continue;

			buf.make_own();
			bpos = s->pos + s->rep_delay * i - range_cur.start();
			buf.add(*s->buf, bpos, s->volume * s->origin->volume, 0);
		}
	}
}

void make_silence(BufferBox &buf, int size)
{
	if (buf.num == 0){
		buf.resize(size);
	}else{
		buf.resize(size);
		memset(buf.r.data, 0, size * sizeof(buf.r[0]));
		memset(buf.l.data, 0, size * sizeof(buf.l[0]));
	}
}

void SongRenderer::bb_render_time_track_no_fx(BufferBox &buf, Track *t)
{
	msg_db_f("bb_render_time_track_no_fx", 1);

	make_silence(buf, range_cur.length());

	Array<Beat> beats = song->bars.getBeats(range_cur);

	MidiRawData raw;
	raw.samples = buf.num;

	foreach(Beat &b, beats)
		raw.addMetronomeClick(b.range.offset - range_cur.offset, (b.beat_no == 0) ? 0 : 1, 0.8f);

	t->synth->feed(raw);
	t->synth->read(buf);
}

void SongRenderer::bb_render_midi_track_no_fx(BufferBox &buf, Track *t, int ti)
{
	msg_db_f("bb_render_midi_track_no_fx", 1);

	make_silence(buf, range_cur.length());

	MidiNoteData *m = &t->midi;
//	if ((ti >= 0) && (ti < midi.num))
//		m = &midi[ti];
	// TODO

	MidiRawData raw = midi_notes_to_events(*m);

	MidiRawData events;
	raw.read(events, range_cur);

	t->synth->feed(events);
	t->synth->read(buf);
}

void SongRenderer::bb_render_track_no_fx(BufferBox &buf, Track *t, int ti)
{
	msg_db_f("bb_render_track_no_fx", 1);

	if (t->type == Track::TYPE_AUDIO)
		bb_render_audio_track_no_fx(buf, t);
	else if (t->type == Track::TYPE_TIME)
		bb_render_time_track_no_fx(buf, t);
	else if (t->type == Track::TYPE_MIDI)
		bb_render_midi_track_no_fx(buf, t, ti);
}

void SongRenderer::make_fake_track(Track *t, BufferBox &buf)
{
	//msg_write("fake track");
	t->song = song;
	t->levels.resize(1);
	t->levels[0].buffers.resize(1);
	t->levels[0].buffers[0].set_as_ref(buf, 0, range_cur.length());
}

void SongRenderer::bb_apply_fx(BufferBox &buf, Track *t, Array<Effect*> &fx_list)
{
	msg_db_f("bb_apply_fx", 1);

	buf.make_own();

	Track fake_track;
	make_fake_track(&fake_track, buf);

	// apply preview plugin?
	if ((t) && (effect))
		effect->apply(buf, &fake_track, false);

	// apply fx
	foreach(Effect *fx, fx_list)
		if (fx->enabled)
			fx->apply(buf, &fake_track, false);
}

void SongRenderer::bb_render_track_fx(BufferBox &buf, Track *t, int ti)
{
	msg_db_f("bb_render_track_fx", 1);

	bb_render_track_no_fx(buf, t, ti);

	if ((t->fx.num > 0) || (effect))
		bb_apply_fx(buf, t, t->fx);
}

int get_first_usable_track(Song *a)
{
	foreachi(Track *t, a->tracks, i)
		if ((!t->muted) && (t->is_selected))
			return i;
	return -1;
}

void SongRenderer::bb_render_song_no_fx(BufferBox &buf)
{
	msg_db_f("bb_render_audio_no_fx", 1);

	// any un-muted track?
	int i0 = get_first_usable_track(song);
	if (i0 < 0){
		// no -> return silence
		buf.resize(range_cur.length());
	}else{

		// first (un-muted) track
		bb_render_track_fx(buf, song->tracks[i0], i0);
		buf.make_own();
		buf.scale(song->tracks[i0]->volume, song->tracks[i0]->panning);

		// other tracks
		for (int i=i0+1;i<song->tracks.num;i++){
			if ((song->tracks[i]->muted) || (!song->tracks[i]->is_selected))
				continue;
			BufferBox tbuf;
			bb_render_track_fx(tbuf, song->tracks[i], i);
			buf.make_own();
			buf.add(tbuf, 0, song->tracks[i]->volume, song->tracks[i]->panning);
		}

		buf.scale(song->volume);
	}
}

void apply_curves(Song *audio, int pos)
{
	foreach(Curve *c, audio->curves)
		c->apply(pos);
}

void unapply_curves(Song *audio)
{
	foreach(Curve *c, audio->curves)
		c->unapply();
}

void SongRenderer::read_basic(BufferBox &buf, int pos, int size)
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

int SongRenderer::read(BufferBox &buf)
{
	msg_db_f("AudioRenderer.read", 1);
	int size = max(min(buf.num, _range.end() - pos), 0);

	if (song->curves.num >= 0){
		buf.resize(size);
		int chunk = 128;
		for (int d=0; d<size; d+=chunk){
			BufferBox tbuf;
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

void SongRenderer::render(const Range &range, BufferBox &buf)
{
	prepare(range, false);
	buf.resize(range.num);
	read(buf);
}

void SongRenderer::prepare(const Range &__range, bool _allow_loop)
{
	msg_db_f("Renderer.Prepare", 2);
	_range = __range;
	allow_loop = _allow_loop;
	pos = _range.offset;
	midi.clear();

	reset();
}

void SongRenderer::reset()
{
	foreach(Effect *fx, song->fx)
		fx->prepare();
	foreachi(Track *t, song->tracks, i){
		//midi.add(t, t->midi);
		midi.add(t->midi);
		t->synth->setSampleRate(song->sample_rate);
		t->synth->reset();
		foreach(Effect *fx, t->fx)
			fx->prepare();
		foreach(MidiEffect *fx, t->midi.fx){
			fx->Prepare();
			tsunami->plugin_manager->context.set(t, 0, _range);
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
	return _range.num;
}

Array<Tag> SongRenderer::getTags()
{
	return song->tags;
}

void SongRenderer::seek(int _pos)
{
	pos = _pos;
	foreach(Track *t, song->tracks)
		t->synth->reset();//endAllNotes();
}
