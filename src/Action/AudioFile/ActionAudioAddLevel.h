/*
 * ActionAudioAddLevel.h
 *
 *  Created on: 11.07.2012
 *      Author: michi
 */

#ifndef ACTIONAUDIOADDLEVEL_H_
#define ACTIONAUDIOADDLEVEL_H_

#include "../Action.h"

class ActionAudioAddLevel : public Action
{
public:
	ActionAudioAddLevel();
	virtual ~ActionAudioAddLevel();

	virtual void *execute(Data *d);
	virtual void undo(Data *d);
};

#endif /* ACTIONAUDIOADDLEVEL_H_ */