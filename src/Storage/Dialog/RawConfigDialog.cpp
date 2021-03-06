/*
 * RawConfigDialog.cpp
 *
 *  Created on: 07.07.2013
 *      Author: michi
 */

#include "RawConfigDialog.h"
#include "../StorageOperationData.h"
#include "../../Session.h"
#include "../../Data/Track.h"
#include "../../Data/base.h"

RawConfigDialog::RawConfigDialog(StorageOperationData *_od, hui::Window *parent) :
	hui::Window("raw_config_dialog", parent)
{
	od = _od;
	ok = false;

	for (int i=1; i<(int)SampleFormat::NUM_SAMPLE_FORMATS; i++)
		add_string("format", format_name((SampleFormat)i));
	set_int("format", od->parameters["format"]._int() - 1);
	if (od->parameters["channels"] == "2")
		check("channels:stereo", true);
	else
		check("channels:mono", true);
	set_int("sample_rate", od->parameters["samplerate"]._int());
	set_int("offset", od->parameters["offset"]._int());

	event("hui:close", [=]{ on_close(); });
	event("close", [=]{ on_close(); });
	event("cancel", [=]{ on_close(); });
	event("ok", [=]{ on_ok(); });
}

void RawConfigDialog::on_close() {
	destroy();
}

void RawConfigDialog::on_ok() {
	od->parameters.set("format", i2s(get_int("format") + 1));
	if (is_checked("channels:stereo"))
		od->parameters.set("channels", "2");
	else
		od->parameters.set("channels", "1");
	od->parameters.set("samplerate", get_string("sample_rate"));
	od->parameters.set("offset", get_string("offset"));
	ok = true;
	destroy();
}
