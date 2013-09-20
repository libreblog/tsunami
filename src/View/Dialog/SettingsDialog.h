/*
 * SettingsDialog.h
 *
 *  Created on: 26.03.2012
 *      Author: michi
 */

#ifndef SETTINGSDIALOG_H_
#define SETTINGSDIALOG_H_

#include "../../lib/hui/hui.h"

class Slider;

class SettingsDialog: public HuiWindow
{
public:
	SettingsDialog(HuiWindow *_parent, bool _allow_parent);
	virtual ~SettingsDialog();

	void LoadData();
	void ApplyData();

	void OnLanguage();
	void OnOggBitrate();
	void OnVolume();
	void OnPreviewDevice();
	void OnCaptureDevice();
	void OnCaptureDelay();
	void OnClose();


private:
	struct OggQuality
	{
		OggQuality(){}
		OggQuality(float q, int b) : quality(q), bitrate(b){};
		float quality;
		int bitrate;
	};

	Array<OggQuality> ogg_quality;

	Slider *volume_slider;
};

#endif /* SETTINGSDIALOG_H_ */
