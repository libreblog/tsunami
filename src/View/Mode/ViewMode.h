/*
 * ViewMode.h
 *
 *  Created on: 12.11.2015
 *      Author: michi
 */

#ifndef SRC_VIEW_MODE_VIEWMODE_H_
#define SRC_VIEW_MODE_VIEWMODE_H_

class AudioView;
class AudioViewTrack;
class Selection;
class Track;
class ViewPort;
class TsunamiWindow;
class Song;
class MidiData;
class Painter;
class rect;
class color;

class ViewMode
{
public:
	ViewMode(AudioView *view);
	virtual ~ViewMode();

	virtual void onLeftButtonDown(){}
	virtual void onLeftButtonUp(){}
	virtual void onLeftDoubleClick(){}
	virtual void onRightButtonDown(){}
	virtual void onRightButtonUp(){}
	virtual void onMouseMove(){}
	virtual void onMouseWheel(){}
	virtual void onKeyDown(int k){}
	virtual void onKeyUp(int k){}
	virtual void updateTrackHeights(){}
	virtual void onCurTrackChange(){}

	virtual Selection getHover();

	virtual void drawGridBars(Painter *c, const rect &r, const color &bg, bool show_time = false){}
	virtual void drawTrackBackground(Painter *c, AudioViewTrack *t){}
	virtual void drawTrackData(Painter *c, AudioViewTrack *t){}
	virtual void drawPost(Painter *c){}
	virtual void drawMidi(Painter *c, AudioViewTrack *t, const MidiData &midi, bool as_reference, int shift){}

	virtual int which_midi_mode(Track *t) = 0;

	AudioView *view;
	ViewPort *cam;
	Selection *selection;
	Selection *hover;
	TsunamiWindow *win;
	Song *song;
};

#endif /* SRC_VIEW_MODE_VIEWMODE_H_ */
