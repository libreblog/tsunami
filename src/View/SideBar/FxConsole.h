/*
 * FxConsole.h
 *
 *  Created on: 20.03.2014
 *      Author: michi
 */

#ifndef FXCONSOLE_H_
#define FXCONSOLE_H_

#include "SideBar.h"
#include "../../Stuff/Observer.h"

class Track;
class Song;
class AudioView;

class FxConsole : public SideBarConsole, public Observer
{
public:
	FxConsole(AudioView *view, Song *audio);
	virtual ~FxConsole();

	void clear();
	void setTrack(Track *t);

	void onAdd();

	void onEditSong();
	void onEditTrack();

	virtual void onUpdate(Observable *o, const string &message);

	string id_inner;

	AudioView *view;
	Track *track;
	Song *song;
	Array<HuiPanel*> panels;
};

#endif /* FXCONSOLE_H_ */