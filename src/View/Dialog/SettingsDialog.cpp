/*
 * SettingsDialog.cpp
 *
 *  Created on: 26.03.2012
 *      Author: michi
 */

#include "SettingsDialog.h"
#include "../../Tsunami.h"
#include "../../Session.h"
#include "../../Device/DeviceManager.h"
#include "../../Device/Stream/AudioOutput.h"
#include "../../TsunamiWindow.h"
#include "../BottomBar/MiniBar.h"
#include "../Helper/CpuDisplay.h"
#include "../../Stuff/Log.h"
#include "../Helper/Slider.h"
#include "../AudioView.h"
#include "../../Module/SignalChain.h"

struct ApiDescription {
	string name;
	DeviceManager::ApiType type;
	int mode;
	bool available;
};
extern ApiDescription api_descriptions[];

SettingsDialog::SettingsDialog(AudioView *_view, hui::Window *_parent) :
		hui::Window("settings_dialog", _parent) {
	view = _view;
	event("language", [=]{ on_language(); });
	event("color_scheme", [=]{ on_color_scheme(); });
	event("ogg_bitrate", [=]{ on_ogg_bitrate(); });
	event("default_artist", [=]{ on_default_artist(); });
	event("scroll_speed", [=]{ on_scroll_speed(); });
	event("cpu_meter", [=]{ on_cpu_meter(); });
	event("antialiasing", [=]{ on_antialiasing(); });
	event("high_details", [=]{ on_high_details(); });
	event("audio_api", [=]{ on_audio_api(); });
	event("midi_api", [=]{ on_midi_api(); });
	event("prebuffer_size", [=]{ on_prebuffer(); });
	event("suck_size", [=]{ on_suck_buffer(); });
	event("quick_export_dir_find", [=]{ on_qed_find(); });
	event("hui:close", [=]{ destroy(); });
	event("close", [=]{ destroy(); });

	set_options("default_artist", "placeholder=" + AppName);

	set_options("quick_export_dir", "placeholder=" + hui::Application::directory);

	ogg_quality.add(OggQuality(0.0f, 64));
	ogg_quality.add(OggQuality(0.1f, 80));
	ogg_quality.add(OggQuality(0.2f, 96));
	ogg_quality.add(OggQuality(0.3f, 112));
	ogg_quality.add(OggQuality(0.4f, 128));
	ogg_quality.add(OggQuality(0.5f, 160));
	ogg_quality.add(OggQuality(0.6f, 192));
	ogg_quality.add(OggQuality(0.7f, 224));
	ogg_quality.add(OggQuality(0.8f, 256));
	ogg_quality.add(OggQuality(0.9f, 320));
	ogg_quality.add(OggQuality(1.0f, 500));

	load_data();

}

void SettingsDialog::load_data() {
	// language
	Array<string> lang = hui::GetLanguages();
	foreachi(string &l, lang, i) {
		add_string("language", l);
		if (l == hui::GetCurLanguage())
			set_int("language", i);
	}

	// color scheme
	foreachi(auto &b, view->color_schemes, i) {
		add_string("color_scheme", b.name);
		if (b.name == view->colors.name)
			set_int("color_scheme", i);
	}

	// ogg quality
	float CurOggQuality = hui::Config.get_float("OggQuality", 0.5f);
	foreachi(OggQuality &q, ogg_quality, i)
		if (CurOggQuality > q.quality - 0.05f)
			set_int("ogg_bitrate", i);
	set_decimals(1);

	set_string("default_artist", hui::Config.get_str("DefaultArtist", ""));

	set_string("quick_export_dir", hui::Config.get_str("QuickExportDir", ""));

	check("cpu_meter", hui::Config.get_bool("CpuDisplay", false));
	check("antialiasing", view->antialiasing);
	check("high_details", view->high_details);
	set_float("scroll_speed", view->mouse_wheel_speed);

	int n_audio = 0, n_midi = 0;
	for (int i=0; i<(int)DeviceManager::ApiType::NUM_APIS; i++) {
		auto &a = api_descriptions[i];
		if (!a.available)
			continue;
		if (a.mode & 1) {
			add_string("audio_api", a.name);
			if (a.type == Session::GLOBAL->device_manager->audio_api)
				set_int("audio_api", n_audio);
			n_audio++;
		}
		if (a.mode & 2) {
			add_string("midi_api", a.name);
			if (a.type == Session::GLOBAL->device_manager->midi_api)
				set_int("midi_api", n_midi);
			n_midi++;
		}
	}
	set_int("prebuffer_size", hui::Config.get_int("Output.BufferSize", AudioOutput::DEFAULT_PREBUFFER_SIZE));
	set_int("suck_size", hui::Config.get_int("SignalChain.BufferSize", SignalChain::DEFAULT_BUFFER_SIZE));
}

void SettingsDialog::applyData() {
}

void SettingsDialog::on_language() {
	Array<string> lang = hui::GetLanguages();
	int l = get_int("");
	hui::SetLanguage(lang[l]);
	hui::Config.set_str("Language", lang[l]);
}

void SettingsDialog::on_color_scheme() {
	int i = get_int("");
	if ((i >= 0) and (i < view->color_schemes.num))
		view->set_color_scheme(view->color_schemes[i].name);
}

void SettingsDialog::on_ogg_bitrate() {
	hui::Config.set_float("OggQuality", ogg_quality[get_int("")].quality);
}

void SettingsDialog::on_default_artist() {
	hui::Config.set_str("DefaultArtist", get_string(""));
}

void SettingsDialog::on_scroll_speed() {
	view->set_mouse_wheel_speed(get_float(""));
}

void SettingsDialog::on_audio_api() {
	int n = get_int("");
	int n_audio = 0;
	for (int i=0; i<(int)DeviceManager::ApiType::NUM_APIS; i++) {
		auto &a = api_descriptions[i];
		if (!a.available)
			continue;
		if (a.mode & 1) {
			if (n_audio == n)
				hui::Config.set_str("AudioApi", a.name);
			n_audio++;
		}
	}
}

void SettingsDialog::on_midi_api() {
	int n = get_int("");
	int n_midi = 0;
	for (int i=0; i<(int)DeviceManager::ApiType::NUM_APIS; i++) {
		auto &a = api_descriptions[i];
		if (!a.available)
			continue;
		if (a.mode & 2) {
			if (n_midi == n)
				hui::Config.set_str("MidiApi", a.name);
			n_midi++;
		}
	}
}

void SettingsDialog::on_prebuffer() {
	int n = get_int("");
	hui::Config.set_int("Output.BufferSize", n);
	view->output_stream->set_prebuffer_size(n);
}

void SettingsDialog::on_suck_buffer() {
	int n = get_int("");
	hui::Config.set_int("SignalChain.BufferSize", n);
	view->signal_chain->set_buffer_size(n);
}

void SettingsDialog::on_cpu_meter() {
	bool show = is_checked("");
	hui::Config.set_bool("CpuDisplay", show);
	view->win->mini_bar->cpu_display->panel->hide_control(view->win->mini_bar->cpu_display->id, !show);
}

void SettingsDialog::on_antialiasing() {
	view->set_antialiasing(is_checked(""));
}

void SettingsDialog::on_high_details() {
	view->set_high_details(is_checked(""));
}

void SettingsDialog::on_qed_find() {
	if (hui::FileDialogDir(this, _("Quick export directory"), "")) {
		hui::Config.set_str("QuickExportDir", hui::Filename);
		set_string("quick_export_dir", hui::Filename);
	}
}
