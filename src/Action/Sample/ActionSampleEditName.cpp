/*
 * ActionSampleEditName.cpp
 *
 *  Created on: 28.03.2014
 *      Author: michi
 */

#include "ActionSampleEditName.h"

#include "../../Data/Song.h"

ActionSampleEditName::ActionSampleEditName(Sample *s, const string &name)
{
	sample = s;
	new_value = name;
	old_value = s->name;
}

ActionSampleEditName::~ActionSampleEditName()
{
}

void *ActionSampleEditName::execute(Data *d)
{
	sample->name = new_value;
	sample->notify(sample->MESSAGE_CHANGE_BY_ACTION);

	return NULL;
}

void ActionSampleEditName::undo(Data *d)
{
	sample->name = old_value;
	sample->notify(sample->MESSAGE_CHANGE_BY_ACTION);
}


bool ActionSampleEditName::mergable(Action *a)
{
	ActionSampleEditName *aa = dynamic_cast<ActionSampleEditName*>(a);
	if (!aa)
		return false;
	return (aa->sample == sample);
}


