/*
 * ActionSampleAdd.cpp
 *
 *  Created on: 15.07.2013
 *      Author: michi
 */

#include "ActionSampleAdd.h"

#include <assert.h>
#include "../../Data/Song.h"

ActionSampleAdd::ActionSampleAdd(const string &name, const BufferBox &buf, bool auto_delete)
{
	sample = new Sample(Track::TYPE_AUDIO);
	sample->buf = buf;
	sample->buf.offset = 0;
	sample->name = name;
	sample->auto_delete = auto_delete;
	sample->_pointer_ref();
}

ActionSampleAdd::ActionSampleAdd(const string &name, const MidiData &midi, bool auto_delete)
{
	sample = new Sample(Track::TYPE_MIDI);
	sample->midi = midi;
	sample->midi.sort();
	sample->name = name;
	sample->auto_delete = auto_delete;
	sample->_pointer_ref();
}

ActionSampleAdd::~ActionSampleAdd()
{
	sample->_pointer_unref();
}

void *ActionSampleAdd::execute(Data *d)
{
	Song *a = dynamic_cast<Song*>(d);
	sample->set_owner(a);
	a->samples.add(sample);
	a->notify(a->MESSAGE_ADD_SAMPLE);
	return sample;
}

void ActionSampleAdd::undo(Data *d)
{
	Song *a = dynamic_cast<Song*>(d);
	assert(sample->ref_count == 0);
	sample->notify(sample->MESSAGE_DELETE);
	a->samples.pop();
	sample->unset_owner();
	a->notify(a->MESSAGE_DELETE_SAMPLE);
}
