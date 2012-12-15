/*
 * ActionTrackDeleteEffect.h
 *
 *  Created on: 15.12.2012
 *      Author: michi
 */

#ifndef ACTIONTRACKDELETEEFFECT_H_
#define ACTIONTRACKDELETEEFFECT_H_

#include "../../Action.h"
#include "../../../Plugins/Effect.h"
class Track;

class ActionTrackDeleteEffect: public Action
{
public:
	ActionTrackDeleteEffect(Track *t, int index);
	virtual ~ActionTrackDeleteEffect();

	virtual void *execute(Data *d);
	virtual void undo(Data *d);

private:
	Effect effect;
	int track_no;
	int index;
};

#endif /* ACTIONTRACKDELETEEFFECT_H_ */