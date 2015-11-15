/*
 * CurveConsole.h
 *
 *  Created on: 19.04.2014
 *      Author: michi
 */

#ifndef CURVECONSOLE_H_
#define CURVECONSOLE_H_

#include "SideBar.h"
#include "../../Stuff/Observer.h"
#include "../../lib/math/math.h"

class Song;
class AudioView;
class Curve;

class CurveConsole : public SideBarConsole, public Observer
{
public:
	CurveConsole(AudioView *view, Song *song);
	virtual ~CurveConsole();

	virtual void onUpdate(Observable *o, const string &message);

	void updateList();
	void onAdd();
	void onDelete();
	void onTarget();
	void onListEdit();
	void onListSelect();

	virtual void onEnter();
	virtual void onLeave();

	Song *song;
	AudioView *view;
	Curve *curve;

	string id_list;
};

#endif /* CURVECONSOLE_H_ */
