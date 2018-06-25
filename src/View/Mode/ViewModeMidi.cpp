/*
 * ViewModeMidi.cpp
 *
 *  Created on: 12.11.2015
 *      Author: michi
 */

#include "ViewModeMidi.h"

#include "../../Module/Audio/SongRenderer.h"
#include "../../Module/Synth/Synthesizer.h"
#include "../../Module/Port/MidiPort.h"
#include "../../Device/OutputStream.h"
#include "../../Data/Midi/Clef.h"
#include "../../Data/SongSelection.h"
#include "../../TsunamiWindow.h"
#include "../../Session.h"
#include "../AudioView.h"
#include "../AudioViewTrack.h"

void align_to_beats(Song *s, Range &r, int beat_partition);

const int PITCH_SHOW_COUNT = 30;

class MidiPreviewSource : public MidiPort
{
public:
	MidiPreviewSource()
	{
		mode = Mode::WAITING;
		ttl = -1;
		volume = 1.0f;
	}
	virtual int _cdecl read(MidiEventBuffer &midi)
	{
		//printf("mps.read\n");
		if (mode == Mode::END_OF_STREAM){
			//printf("  - end\n");
			return END_OF_STREAM;
		}

		if (mode == Mode::START_NOTES){
			for (int p: pitch)
				midi.add(MidiEvent(0, p, volume));
			mode = Mode::ACTIVE_NOTES;
		}else if (mode == Mode::END_NOTES){
			for (int p: pitch)
				midi.add(MidiEvent(0, p, 0));
			mode = Mode::END_OF_STREAM;
		}
		if (mode == Mode::ACTIVE_NOTES){
			ttl -= midi.samples;
			if (ttl < 0)
				end();
		}
		return midi.samples;
	}

	void start(const Array<int> &_pitch, int _ttl)
	{
		if ((mode != Mode::WAITING) and (mode != Mode::END_OF_STREAM))
			return;
		pitch = _pitch;
		ttl = _ttl;
		mode = Mode::START_NOTES;
	}
	void end()
	{
		if (mode == Mode::START_NOTES){
			mode = Mode::WAITING;
		}else if (mode == Mode::ACTIVE_NOTES){
			mode = Mode::END_NOTES;
		}
	}

private:
	int mode;
	enum Mode{
		WAITING,
		START_NOTES,
		ACTIVE_NOTES,
		END_NOTES,
		END_OF_STREAM
	};
	int ttl;

	Array<int> pitch;
	float volume;
};

ViewModeMidi::ViewModeMidi(AudioView *view) :
	ViewModeDefault(view)
{
	cur_track = NULL;
	beat_partition = 4;
	win->setInt("beat_partition", beat_partition);
	mode_wanted = AudioView::MidiMode::CLASSICAL;
	creation_mode = CreationMode::NOTE;
	midi_interval = 3;
	chord_type = 0;
	chord_inversion = 0;
	modifier = Modifier::NONE;

	moving = false;
	string_no = 0;
	octave = 3;

	scroll_offset = 0;
	scroll_bar = rect(0, 0, 0, 0);

	preview_source = new MidiPreviewSource;

	preview_synth = NULL;
	preview_stream = NULL;
}

ViewModeMidi::~ViewModeMidi()
{
	kill_preview();
	delete preview_source;
}

void ViewModeMidi::setMode(int _mode)
{
	mode_wanted = _mode;
	view->forceRedraw();
}

void ViewModeMidi::setCreationMode(int _mode)
{
	creation_mode = _mode;
	//view->forceRedraw();
}


void ViewModeMidi::startMidiPreview(const Array<int> &pitch, float ttl)
{
	//kill_preview();
	if (!preview_synth){
		preview_synth = (Synthesizer*)view->cur_track->synth->copy();
		preview_synth->setInstrument(view->cur_track->instrument);
		preview_synth->set_source(preview_source);
	}
	if (!preview_stream){
		preview_stream = new OutputStream(view->session, preview_synth->out);
		preview_stream->set_buffer_size(2048);
		preview_stream->subscribe(this, std::bind(&ViewModeMidi::onEndOfStream, this), preview_stream->MESSAGE_PLAY_END_OF_STREAM);
	}
	preview_stream->set_volume(view->cur_track->volume);

	preview_source->start(pitch, view->session->sample_rate() * ttl);
	preview_stream->play();
}

void ViewModeMidi::onEndOfStream()
{
	//msg_write("view: end of stream");
	preview_stream->stop();
	//hui::RunLater(0.001f,  std::bind(&ViewModeMidi::kill_preview, this));
}

void ViewModeMidi::kill_preview()
{
	//msg_write("kill");
	if (preview_stream){
		preview_stream->stop();
		delete preview_stream;
	}
	preview_stream = NULL;
	if (preview_synth)
		delete preview_synth;
	preview_synth = NULL;
}

void ViewModeMidi::onLeftButtonDown()
{
	ViewModeDefault::onLeftButtonDown();
	int mode = which_midi_mode(cur_track->track);

	if (creation_mode == CreationMode::SELECT){
		setCursorPos(hover->pos, true);
		view->msp.start(hover->pos, hover->y0);

	}else{
		//view->msp.start(hover->pos, hover->y0);
		view->hide_selection = true;
	}

	if (hover->type == Selection::Type::MIDI_NOTE){
		if (win->getKey(hui::KEY_CONTROL)){
			view->sel.set(hover->note, !view->sel.has(hover->note));
		}else{
			if (!view->sel.has(hover->note)){
				view->sel.clear_data();
				view->sel.add(hover->note);
			}
		}
		// start moving
		moving = true;
	}else if (hover->type == Selection::Type::CLEF_POSITION){
		/*if (creation_mode != CreationMode::SELECT){
			view->msp.stop();
		}*/
		view->msp.start_pos = hover->pos; // TODO ...bad
		if (mode == AudioView::MidiMode::TAB){
			string_no = clampi(hover->clef_position, 0, cur_track->track->instrument.string_pitch.num - 1);
		}
	}else if (hover->type == Selection::Type::MIDI_PITCH){
		view->msp.start_pos = hover->pos; // TODO ...bad
		if (mode == AudioView::MidiMode::TAB){
		}else{ // CLASSICAL/LINEAR
			// create new note
			startMidiPreview(getCreationPitch(hover->pitch), 1.0f);
		}
	}else if (hover->type == Selection::Type::SCROLL){
		scroll_offset = view->my - scroll_bar.y1;
		view->msp.stop();
	}
}

void ViewModeMidi::onLeftButtonUp()
{
	ViewModeDefault::onLeftButtonUp();
	view->hide_selection = false;

	int mode = which_midi_mode(cur_track->track);
	if ((mode == AudioView::MidiMode::CLASSICAL) or (mode == AudioView::MidiMode::LINEAR)){
		if (hover->type == Selection::Type::MIDI_PITCH){
			auto notes = getCreationNotes(hover, view->msp.start_pos);
			view->cur_track->addMidiNotes(notes);
			notes.clear(); // all notes owned by track now

			preview_source->end();
		}
	}
	moving = false;
}

void ViewModeMidi::onMouseMove()
{
	ViewModeDefault::onMouseMove();
	auto e = hui::GetEvent();

	// drag & drop
	if (moving){
		/**hover = getHover();
		if ((hover->type == Selection::Type::MIDI_NOTE) and (hover->track == view->cur_track)){
			hover->track->deleteMidiNote(hover->index);
			hover->clear();
		}*/
	}

	if (hover->type == Selection::Type::MIDI_PITCH){
		// creating notes
		//view->forceRedraw();
	}else if (hover->type == Selection::Type::SCROLL){
		if (e->lbut){
			int _pitch_max = (cur_track->area.y2 + scroll_offset - view->my) / cur_track->area.height() * (MAX_PITCH - 1.0f);
			cur_track->setPitchMinMax(_pitch_max - PITCH_SHOW_COUNT, _pitch_max);
		}
	}
}

void ViewModeMidi::onKeyDown(int k)
{
	int mode = which_midi_mode(cur_track->track);
	if (mode == AudioView::MidiMode::CLASSICAL){
		if (k == hui::KEY_1){
			modifier = Modifier::NONE;
			view->notify(view->MESSAGE_SETTINGS_CHANGE);
		}else if (k == hui::KEY_2){
			modifier = Modifier::SHARP;
			view->notify(view->MESSAGE_SETTINGS_CHANGE);
		}else if (k == hui::KEY_3){
			modifier = Modifier::FLAT;
			view->notify(view->MESSAGE_SETTINGS_CHANGE);
		}else if (k == hui::KEY_4){
			modifier = Modifier::NATURAL;
			view->notify(view->MESSAGE_SETTINGS_CHANGE);
		}


		if ((k >= hui::KEY_A) and (k <= hui::KEY_G)){
			Range r = getMidiEditRange();
			int number = (k - hui::KEY_A);
			int rel[7] = {9,11,0,2,4,5,7};
			int pitch = pitch_from_octave_and_rel(rel[number], octave);
			cur_track->track->addMidiNote(new MidiNote(r, pitch, 1.0f));
			setCursorPos(r.end() + 1, true);
			//view->updateSelection();
			startMidiPreview(pitch, 0.1f);

		}
		if (k == hui::KEY_UP){
			octave = min(octave + 1, 7);
			view->forceRedraw();
		}
		if (k == hui::KEY_DOWN){
			octave = max(octave - 1, 0);
			view->forceRedraw();
		}
	}else if (mode == AudioView::MidiMode::TAB){

		if (((k >= hui::KEY_0) and (k <= hui::KEY_9)) or ((k >= hui::KEY_A) and (k <= hui::KEY_F))){
			Range r = getMidiEditRange();
			int number = (k - hui::KEY_0);
			if (k >= hui::KEY_A)
				number = 10 + (k - hui::KEY_A);
			int pitch = cur_track->track->instrument.string_pitch[string_no] + number;
			MidiNote *n = new MidiNote(r, pitch, 1.0f);
			n->stringno = string_no;
			cur_track->track->addMidiNote(n);
			setCursorPos(r.end() + 1, true);
			//view->updateSelection();
			startMidiPreview(pitch, 0.1f);

		}
		if (k == hui::KEY_UP){
			string_no = min(string_no + 1, cur_track->track->instrument.string_pitch.num - 1);
			view->forceRedraw();
		}
		if (k == hui::KEY_DOWN){
			string_no = max(string_no - 1, 0);
			view->forceRedraw();
		}
	}

	//if (k == hui::KEY_ESCAPE)
		//tsunami->side_bar->open(SideBar::MIDI_EDITOR_CONSOLE);
		//view->setMode(view->mode_default);

	ViewModeDefault::onKeyDown(k);
}

void ViewModeMidi::updateTrackHeights()
{
	for (AudioViewTrack *t: view->vtrack){
		t->height_min = view->TIME_SCALE_HEIGHT;
		if (t->track->type == Track::Type::AUDIO){
			t->height_wish = view->MAX_TRACK_CHANNEL_HEIGHT;
		}else if (t->track->type == Track::Type::MIDI){
			if (t->track == view->cur_track){
				if (view->midi_view_mode == view->MidiMode::CLASSICAL)
					t->height_wish = view->MAX_TRACK_CHANNEL_HEIGHT * 4;
				else
					t->height_wish = 5000;
			}else{
				t->height_wish = view->MAX_TRACK_CHANNEL_HEIGHT;
			}
		}else{
			t->height_wish = view->TIME_SCALE_HEIGHT * 2;
		}
	}
}

void ViewModeMidi::onCurTrackChange()
{
	view->thm.dirty = true;
}


Range get_allowed_midi_range(Track *t, Array<int> pitch, int start)
{
	Range allowed = Range::ALL;
	for (MidiNote *n: t->midi){
		for (int p: pitch)
			if (n->pitch == p){
				if (n->range.is_inside(start))
					return Range::EMPTY;
			}
	}

	MidiEventBuffer midi = midi_notes_to_events(t->midi);
	for (MidiEvent &e: midi)
		for (int p: pitch)
			if (e.pitch == p){
				if ((e.pos >= start) and (e.pos < allowed.end()))
					allowed.set_end(e.pos);
				if ((e.pos < start) and (e.pos >= allowed.start()))
					allowed.set_start(e.pos);
			}
	return allowed;
}

Array<int> ViewModeMidi::getCreationPitch(int base_pitch)
{
	Array<int> pitch;
	if (creation_mode == CreationMode::NOTE){
		pitch.add(base_pitch);
	}else if (creation_mode == CreationMode::INTERVAL){
		pitch.add(base_pitch);
		if (midi_interval != 0)
			pitch.add(base_pitch + midi_interval);
	}else if (creation_mode == CreationMode::CHORD){
		pitch = chord_notes(chord_type, chord_inversion, base_pitch);
	}
	return pitch;
}

MidiNoteBuffer ViewModeMidi::getCreationNotes(Selection *sel, int pos0)
{
	int start = min(pos0, sel->pos);
	int end = max(pos0, sel->pos);
	Range r = Range(start, end - start);

	// align to beats
	if (song->bars.num > 0)
		align_to_beats(song, r, beat_partition);

	Array<int> pitch = getCreationPitch(sel->pitch);

	// collision?
	Range allowed = get_allowed_midi_range(view->cur_track, pitch, pos0);

	// create notes
	MidiNoteBuffer notes;
	if (allowed.empty())
		return notes;
	for (int p: pitch)
		notes.add(new MidiNote(r and allowed, p, 1));
	if (notes.num > 0){
		notes[0]->clef_position = sel->clef_position;
		notes[0]->modifier = sel->modifier;
	}
	return notes;
}

void ViewModeMidi::setBeatPartition(int partition)
{
	beat_partition = partition;
	view->forceRedraw();
}

void ViewModeMidi::drawTrackBackground(Painter *c, AudioViewTrack *t)
{
	t->drawBlankBackground(c);

	color cc = t->getBackgroundColor();
	if (song->bars.num > 0)
		t->drawGridBars(c, cc, (t->track->type == Track::Type::TIME), beat_partition);
	else
		view->drawGridTime(c, t->area, cc, false);

	if (t->track->type == Track::Type::MIDI){
		int mode = which_midi_mode(t->track);
		if (t->track == view->cur_track){
			if (mode == AudioView::MidiMode::LINEAR)
				drawTrackPitchGrid(c, t);
		}

		if (mode == AudioView::MidiMode::CLASSICAL){
			const Clef& clef = t->track->instrument.get_clef();
			t->drawMidiClefClassical(c, clef, view->midi_scale);
		}else if (mode == AudioView::MidiMode::TAB){
			t->drawMidiClefTab(c);
		}
	}


}

void ViewModeMidi::drawTrackPitchGrid(Painter *c, AudioViewTrack *t)
{
	cur_track = t;

	// pitch grid
	c->setColor(color(0.25f, 0, 0, 0));
	for (int i=t->pitch_min; i<t->pitch_max; i++){
		float y0 = t->pitch2y_linear(i + 1);
		float y1 = t->pitch2y_linear(i);
		if (!view->midi_scale.contains(i)){
			c->setColor(color(0.2f, 0, 0, 0));
			c->drawRect(t->area.x1, y0, t->area.width(), y1 - y0);
		}
	}


	// pitch names
	color cc = view->colors.text;
	cc.a = 0.4f;
	Array<SampleRef*> *p = NULL;
	if ((t->track->synth) and (t->track->synth->module_subtype == "Sample")){
		auto *c = t->track->synth->get_config();
		p = (Array<SampleRef*> *)&c[1];
	}
	bool is_drum = (t->track->instrument.type == Instrument::Type::DRUMS);
	for (int i=cur_track->pitch_min; i<cur_track->pitch_max; i++){
		c->setColor(cc);
		if (((hover->type == Selection::Type::MIDI_PITCH) or (hover->type == Selection::Type::MIDI_NOTE)) and (i == hover->pitch))
			c->setColor(view->colors.text);

		string name = pitch_name(i);
		if (is_drum){
			name = drum_pitch_name(i);
		}else if (p){
			if (i < p->num)
				if ((*p)[i])
					name = (*p)[i]->origin->name;
		}
		c->drawStr(20, t->area.y1 + t->area.height() * (cur_track->pitch_max - i - 1) / PITCH_SHOW_COUNT, name);
	}
}

inline bool hover_note_classical(const MidiNote &n, Selection &s, ViewModeMidi *vmm)
{
	if (n.clef_position != s.clef_position)
		return false;
	return n.range.is_inside(s.pos);
}

inline bool hover_note_tab(const MidiNote &n, Selection &s, ViewModeMidi *vmm)
{
	if (n.stringno != s.clef_position)
		return false;
	return n.range.is_inside(s.pos);
}

inline bool hover_note_linear(const MidiNote &n, Selection &s, ViewModeMidi *vmm)
{
	if (n.pitch != s.pitch)
		return false;
	return n.range.is_inside(s.pos);
}

Selection ViewModeMidi::getHover()
{
	Selection s = ViewModeDefault::getHover();
	if (s.type != s.Type::TRACK)
		return s;

	int mx = view->mx;
	int my = view->my;

	// midi
	if ((s.track) and (s.track->type == Track::Type::MIDI) and (s.track == view->cur_track)){
		int mode = which_midi_mode(s.track);

		// scroll bar
		if ((mode == AudioView::MidiMode::LINEAR) and (scroll_bar.inside(view->mx, view->my))){
			s.type = Selection::Type::SCROLL;
			return s;
		}

		/*if (creation_mode != CreationMode::SELECT)*/{
			if ((mode == AudioView::MidiMode::CLASSICAL)){
				s.pitch = cur_track->y2pitch_classical(my, modifier);
				s.clef_position = cur_track->screen_to_clef_pos(my);
				s.modifier = modifier;
				s.type = Selection::Type::MIDI_PITCH;
				s.index = randi(100000); // quick'n'dirty fix to force view update every time the mouse moves

				foreachi(MidiNote *n, s.track->midi, i)
					if (hover_note_classical(*n, s, this)){
						s.note = n;
						s.index = i;
						s.type = Selection::Type::MIDI_NOTE;
						return s;
					}
			}else if ((mode == AudioView::MidiMode::TAB)){
				//s.pitch = cur_track->y2pitch_classical(my, modifier);
				s.clef_position = cur_track->screen_to_string(my);
				s.modifier = modifier;
				s.type = Selection::Type::CLEF_POSITION;
				s.index = randi(100000); // quick'n'dirty fix to force view update every time the mouse moves

				foreachi(MidiNote *n, s.track->midi, i)
					if (hover_note_tab(*n, s, this)){
						s.note = n;
						s.index = i;
						s.type = Selection::Type::MIDI_NOTE;
						return s;
					}
			}else if (mode == AudioView::MidiMode::LINEAR){
				s.pitch = cur_track->y2pitch_linear(my);
				s.clef_position = cur_track->y2clef_linear(my, s.modifier);
				//s.modifier = modifier;
				s.type = Selection::Type::MIDI_PITCH;
				s.index = randi(100000); // quick'n'dirty fix to force view update every time the mouse moves

				foreachi(MidiNote *n, s.track->midi, i)
					if (hover_note_linear(*n, s, this)){
						s.note = n;
						s.index = i;
						s.type = Selection::Type::MIDI_NOTE;
						return s;
					}
			}
		}
		/*if (creation_mode == CreationMode::SELECT){
			if ((s.type == Selection::Type::MIDI_PITCH) or (s.type == Selection::Type::CLEF_POSITION)){
				s.type = Selection::Type::TRACK;
			}
		}*/
	}

	return s;
}

void ViewModeMidi::drawTrackData(Painter *c, AudioViewTrack *t)
{
	// midi
	if ((view->cur_track == t->track) and (t->track->type == Track::Type::MIDI)){
		// we're editing this track...
		cur_track = t;

		for (int n: t->reference_tracks)
			if ((n >= 0) and (n < song->tracks.num) and (song->tracks[n] != t->track))
				drawMidi(c, t, song->tracks[n]->midi, true, 0);

		drawMidi(c, t, t->track->midi, false, 0);

		int mode = which_midi_mode(t->track);

		if ((mode == view->MidiMode::CLASSICAL) or (mode == view->MidiMode::LINEAR)){

			// current creation
			if ((hui::GetEvent()->lbut) and (hover->type == Selection::Type::MIDI_PITCH)){
				auto notes = getCreationNotes(hover, view->msp.start_pos);
				drawMidi(c, t, notes, false, 0);
				//c->setFontSize(view->FONT_SIZE);
			}


			// creation preview
			if ((!hui::GetEvent()->lbut) and (hover->type == Selection::Type::MIDI_PITCH)){
				auto notes = getCreationNotes(hover, hover->pos);
				drawMidi(c, t, notes, false, 0);
			}
		}


		if (mode == view->MidiMode::CLASSICAL){

		}else if (mode == view->MidiMode::LINEAR){

			// scrollbar
			if (hover->type == Selection::Type::SCROLL)
				c->setColor(view->colors.text);
			else
				c->setColor(view->colors.text_soft1);
			scroll_bar = rect(t->area.x2 - 40, t->area.x2 - 20, t->area.y2 - t->area.height() * cur_track->pitch_max / (MAX_PITCH - 1.0f), t->area.y2 - t->area.height() * cur_track->pitch_min / (MAX_PITCH - 1.0f));
			c->drawRect(scroll_bar);
		}
	}else{

		// not editing -> just draw
		if (t->track->type == Track::Type::MIDI)
			drawMidi(c, t, t->track->midi, false, 0);
	}

	// audio buffer
	t->drawTrackBuffers(c, view->cam.pos);

	// samples
	for (SampleRef *s: t->track->samples)
		t->drawSample(c, s);

	// marker
	t->marker_areas.resize(t->track->markers.num);
	t->marker_label_areas.resize(t->track->markers.num);
	foreachi(TrackMarker *m, t->track->markers, i)
		t->drawMarker(c, m, i, (view->hover.type == Selection::Type::MARKER) and (view->hover.track == t->track) and (view->hover.index == i));
}

int ViewModeMidi::which_midi_mode(Track *t)
{
	if ((view->cur_track == t) and (t->type == Track::Type::MIDI)){
		if (mode_wanted == view->MidiMode::TAB){
			if (t->instrument.string_pitch.num > 0)
				return view->MidiMode::TAB;
			return view->MidiMode::CLASSICAL;
		}
		return mode_wanted;
	}
	return ViewModeDefault::which_midi_mode(t);
}

void ViewModeMidi::drawPost(Painter *c)
{
	ViewModeDefault::drawPost(c);

	int mode = which_midi_mode(cur_track->track);
	if ((mode != AudioView::MidiMode::CLASSICAL) and (mode != AudioView::MidiMode::TAB))
		return;
	Range r = getMidiEditRange();
	int x1 = view->cam.sample2screen(r.start());
	int x2 = view->cam.sample2screen(r.end());

	c->setColor(view->colors.selection_internal);
	if (mode == AudioView::MidiMode::TAB){
		int y = cur_track->string_to_screen(string_no);
		int y1 = y - cur_track->clef_dy/2;
		int y2 = y + cur_track->clef_dy/2;
		c->drawRect(x1,  y1,  x2 - x1,  y2 - y1);
	}else if (mode == AudioView::MidiMode::CLASSICAL){
		int p1 = pitch_from_octave_and_rel(0, octave);
		int p2 = pitch_from_octave_and_rel(0, octave+1);
		int y1 = cur_track->pitch2y_classical(p2);
		int y2 = cur_track->pitch2y_classical(p1);
		c->drawRect(x1,  y1,  x2 - x1,  y2 - y1);
	}
}

Range ViewModeMidi::getMidiEditRange()
{
	int a = song->bars.getPrevSubBeat(view->sel.range.offset+1, beat_partition);
	int b = song->bars.getNextSubBeat(view->sel.range.end()-1, beat_partition);
	if (a == b)
		b = song->bars.getNextSubBeat(b, beat_partition);
	return Range(a, b - a);
}

void ViewModeMidi::startSelection()
{
	setBarriers(*hover);
	hover->range.set_start(view->msp.start_pos);
	hover->range.set_end(hover->pos);
	if (hover->type == Selection::Type::TIME){
		hover->type = Selection::Type::SELECTION_END;
		view->selection_mode = view->SelectionMode::TIME;
	}else{
		hover->y0 = view->msp.start_y;
		hover->y1 = view->my;
		view->selection_mode = view->SelectionMode::RECT;
	}
	view->setSelection(getSelection(hover->range));
}
