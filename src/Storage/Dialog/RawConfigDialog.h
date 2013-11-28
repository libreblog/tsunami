/*
 * RawConfigDialog.h
 *
 *  Created on: 07.07.2013
 *      Author: michi
 */

#ifndef RAWCONFIGDIALOG_H_
#define RAWCONFIGDIALOG_H_

#include "../../lib/hui/hui.h"
#include "../../Data/BufferBox.h"

struct RawConfigData
{
	SampleFormat format;
	int channels;
	int sample_rate;
	int offset;
};

class RawConfigDialog : public HuiDialog
{
public:
	RawConfigDialog(RawConfigData *data, HuiWindow *parent);
	virtual ~RawConfigDialog();

	void OnClose();
	void OnOk();

	RawConfigData *data;
};

#endif /* RAWCONFIGDIALOG_H_ */