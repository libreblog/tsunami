/*
 * ActionAudioAddTrack.cpp
 *
 *  Created on: 24.03.2012
 *      Author: michi
 */

#include "ActionAudioAddTrack.h"
#include "../../Data/AudioFile.h"
#include "../../lib/hui/hui.h"
#include <assert.h>

ActionAudioAddTrack::ActionAudioAddTrack(int _index, int _type)
{
	index = _index;
	type = _type;
}

ActionAudioAddTrack::~ActionAudioAddTrack()
{
}

void ActionAudioAddTrack::undo(Data *d)
{
	AudioFile *a = dynamic_cast<AudioFile*>(d);
	delete(a->track[index]);
	a->track.erase(index);
	a->cur_track = old_cur_track;
}



void *ActionAudioAddTrack::execute(Data *d)
{
	AudioFile *a = dynamic_cast<AudioFile*>(d);
	old_cur_track = a->cur_track;

	assert((index >= 0) && (index <= a->track.num));

	Track *t = new Track;

	t->name = format(_("Spur %d"), a->track.num + 1);
	t->root = a;
	t->parent = -1;
	t->is_selected = true;
	t->type = type;
	t->level.resize(a->level_name.num);

	a->cur_track = index;
	a->track.insert(t, a->cur_track);

	return t;
}


