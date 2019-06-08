/*
 * ViewMode.h
 *
 *  Created on: 12.11.2015
 *      Author: michi
 */

#ifndef SRC_VIEW_MODE_VIEWMODE_H_
#define SRC_VIEW_MODE_VIEWMODE_H_

#include "../../Stuff/Observable.h"

enum class MidiMode;
class AudioView;
class AudioViewTrack;
class AudioViewLayer;
class Selection;
class SongSelection;
class Track;
class ViewPort;
class TsunamiWindow;
class Song;
class MidiNoteBuffer;
class Painter;
class rect;
class color;
class Range;
class Session;

class ViewMode : public Observable<VirtualBase>
{
public:
	ViewMode(AudioView *view);
	virtual ~ViewMode();

	virtual void on_start(){}
	virtual void on_end(){}

	virtual void on_left_button_down(){}
	virtual void on_left_button_up(){}
	virtual void on_left_double_click(){}
	virtual void on_right_button_down(){}
	virtual void on_right_button_up(){}
	virtual void on_mouse_move(){}
	virtual void on_mouse_wheel(){}
	virtual void on_key_down(int k){}
	virtual void on_key_up(int k){}
	virtual float layer_min_height(AudioViewLayer *l) = 0;
	virtual float layer_suggested_height(AudioViewLayer *l) = 0;
	virtual void on_cur_layer_change(){}

	virtual Selection get_hover();

	virtual void draw_track_background(Painter *c, AudioViewTrack *t){}
	virtual void draw_layer_background(Painter *c, AudioViewLayer *l){}
	virtual void draw_track_data(Painter *c, AudioViewTrack *t){}
	virtual void draw_imploded_track_data(Painter *c, AudioViewTrack *t){}
	virtual void draw_post(Painter *c){}

	virtual MidiMode which_midi_mode(Track *t) = 0;

	SongSelection get_selection(const Range &r);
	virtual SongSelection get_selection_for_range(const Range &r);
	virtual SongSelection get_selection_for_rect(const Range &r, int y0, int y1);
	virtual SongSelection get_selection_for_track_rect(const Range &r, int y0, int y1);
	virtual void start_selection(){}

	virtual Set<Track*> prevent_playback(){ return {}; }

	AudioView *view;
	Session *session;
	ViewPort *cam;
	Selection *hover;
	TsunamiWindow *win;
	Song *song;

	int side_bar_console;
};

#endif /* SRC_VIEW_MODE_VIEWMODE_H_ */
