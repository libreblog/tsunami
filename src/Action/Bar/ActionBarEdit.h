/*
 * ActionBarEdit.h
 *
 *  Created on: 15.12.2012
 *      Author: michi
 */

#ifndef ACTIONBAREDIT_H_
#define ACTIONBAREDIT_H_

#include "../ActionGroup.h"

class Song;
class BarPattern;

class ActionBarEdit: public ActionGroup
{
public:
	ActionBarEdit(int index, const BarPattern &bar, int mode);

	void build(Data *d) override;

	int index;
	int length, divisor;
	Array<int> beats;
	int mode;
};

#endif /* ACTIONBAREDIT_H_ */
