/*
 * MiniConsole.h
 *
 *  Created on: 23.03.2014
 *      Author: michi
 */

#ifndef MINICONSOLE_H_
#define MINICONSOLE_H_

#include "../../lib/hui/hui.h"

class PeakMeter;
class AudioOutput;

class MiniConsole : public HuiPanel
{
public:
	MiniConsole();
	virtual ~MiniConsole();

	AudioOutput *output;
	PeakMeter *peak_meter;
};

#endif /* MINICONSOLE_H_ */
