/*
 * PauseEditDialog.cpp
 *
 *  Created on: 30.10.2015
 *      Author: michi
 */

#include "PauseEditDialog.h"
#include "../../Data/Song.h"

PauseEditDialog::PauseEditDialog(HuiWindow *root, Song *_song, int _index, bool _apply_to_midi):
	HuiDialog("", 100, 100, root, false)
{
	fromResource("pause_edit_dialog");
	song = _song;
	index = _index;
	apply_to_midi = _apply_to_midi;

	BarPattern &b = song->bars[index];
	setFloat("duration", (float)b.length / (float)song->sample_rate);

	event("ok", this, &PauseEditDialog::onOk);
	event("cancel", this, &PauseEditDialog::onClose);
	event("hui:close", this, &PauseEditDialog::onClose);
}

void PauseEditDialog::onOk()
{
	float duration = getFloat("duration");
	BarPattern b = song->bars[index];
	b.length = (float)song->sample_rate * duration;
	song->editBar(index, b, apply_to_midi);

	delete(this);
}

void PauseEditDialog::onClose()
{
	delete(this);
}