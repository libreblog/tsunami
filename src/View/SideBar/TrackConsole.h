/*
 * TrackConsole.h
 *
 *  Created on: 26.03.2012
 *      Author: michi
 */

#ifndef TRACKCONSOLE_H_
#define TRACKCONSOLE_H_


#include "SideBar.h"
#include "../../Stuff/Observer.h"
class Track;
class Slider;
class BarList;
class AudioView;

class TrackConsole: public SideBarConsole, public Observer
{
public:
	TrackConsole(AudioView *view);
	virtual ~TrackConsole();

	void loadData();
	void applyData();

	void onName();
	void onVolume();
	void onPanning();

	void onEditSong();
	void onEditFx();
	void onEditMidi();
	void onEditMidiFx();
	void onEditSynth();

	void setTrack(Track *t);

	virtual void onUpdate(Observable *o, const string &message);

	AudioView *view;
	Track *track;
	BarList *bar_list;
};

#endif /* TRACKCONSOLE_H_ */
