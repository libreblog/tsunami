/*
 * FormatM4a.h
 *
 *  Created on: 21.09.2014
 *      Author: michi
 */

#ifndef FORMATM4A_H_
#define FORMATM4A_H_

#include "Format.h"

class FormatM4a: public Format
{
public:
	FormatM4a();
	virtual ~FormatM4a();

	void loadTrack(StorageOperationData *od);
	void saveBuffer(StorageOperationData *od);

	void loadAudio(StorageOperationData *od);
	void saveAudio(StorageOperationData *od);
};

#endif /* FORMATMP3_H_ */