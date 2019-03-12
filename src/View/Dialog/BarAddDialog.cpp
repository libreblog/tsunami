/*
 * BarAddDialog.cpp
 *
 *  Created on: 30.10.2015
 *      Author: michi
 */

#include "BarAddDialog.h"

#include "../../Data/Rhythm/Bar.h"
#include "../../Data/Song.h"
#include "../../Data/base.h"
#include "../AudioView.h"

BarAddDialog::BarAddDialog(hui::Window *root, Song *s, int _index):
	hui::Dialog("", 100, 100, root, false)
{
	from_resource("bar_add_dialog");
	song = s;
	index = max(_index, 0);


	set_int("count", 1);
	new_bar = new Bar(100, 4, 1);
	new_bar->set_bpm(90, song->sample_rate);

	// get default data from "selected" reference bar
	if (song->bars.num > 0){
		foreachi(Bar *b, song->bars, i)
			if ((i <= index) and (!b->is_pause())){
				*new_bar = *b;
			}
	}
	set_int("beats", new_bar->beats.num);
	set_int("divisor", 0);
	if (new_bar->divisor == 2)
		set_int("divisor", 1);
	if (new_bar->divisor == 4)
		set_int("divisor", 2);
	set_string("pattern", new_bar->pat_str());
	set_float("bpm", new_bar->bpm(song->sample_rate));
	check("shift-data", true);

	event("beats", [=]{ on_beats(); });
	event("complex", [=]{ on_complex(); });
	event("pattern", [=]{ on_pattern(); });
	event("divisor", [=]{ on_divisor(); });
	event("ok", [=]{ on_ok(); });
	event("cancel", [=]{ on_close(); });
	event("hui:close", [=]{ on_close(); });
}

void set_bar_pattern(BarPattern &b, const string &pat)
{
	auto xx = pat.replace(",", " ").replace(":", " ").explode(" ");
	b.beats.clear();
	for (string &x: xx)
		b.beats.add(x._int());
	b.update_total();
}

void BarAddDialog::on_beats()
{
	*new_bar = Bar(100, get_int(""), new_bar->divisor);
	set_string("pattern", new_bar->pat_str());
}

void BarAddDialog::on_divisor()
{
	new_bar->divisor = 1 << get_int("");
}

void BarAddDialog::on_pattern()
{
	set_bar_pattern(*new_bar, get_string("pattern"));
	set_int("beats", new_bar->beats.num);
}

void BarAddDialog::on_complex()
{
	bool complex = is_checked("complex");
	hide_control("beats", complex);
	hide_control("pattern", !complex);
}

void BarAddDialog::on_ok()
{
	int count = get_int("count");
	float bpm = get_float("bpm");
	bool move_data = is_checked("shift-data");
	new_bar->set_bpm(bpm, song->sample_rate);

	song->begin_action_group();

	if (!song->time_track())
		song->add_track(SignalType::BEATS, 0);

	for (int i=0; i<count; i++)
		song->add_bar(index, *new_bar, move_data ? Bar::EditMode::STRETCH : Bar::EditMode::IGNORE);
	song->end_action_group();

	destroy();
}

void BarAddDialog::on_close()
{
	destroy();
}
