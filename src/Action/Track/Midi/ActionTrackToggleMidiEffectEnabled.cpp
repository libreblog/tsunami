/*
 * ActionTrackToggleMidiEffectEnabled.cpp
 *
 *  Created on: 23.09.2014
 *      Author: michi
 */

#include "ActionTrackToggleMidiEffectEnabled.h"
#include "../../../Data/Song.h"
#include "../../../Module/Midi/MidiEffect.h"

ActionTrackToggleMidiEffectEnabled::ActionTrackToggleMidiEffectEnabled(MidiEffect *_fx) {
	fx = _fx;
}

void *ActionTrackToggleMidiEffectEnabled::execute(Data *d) {
	fx->enabled = !fx->enabled;
	fx->Observable::notify(fx->MESSAGE_CHANGE_BY_ACTION);
	d->notify(Song::MESSAGE_ENABLE_FX);

	return nullptr;
}

void ActionTrackToggleMidiEffectEnabled::undo(Data *d) {
	execute(d);
}


