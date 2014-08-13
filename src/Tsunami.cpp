/*
 * Tsunami.cpp
 *
 *  Created on: 13.08.2014
 *      Author: michi
 */

#include "Tsunami.h"
#include "TsunamiWindow.h"
#include "Storage/Storage.h"
#include "Stuff/Log.h"
#include "Stuff/Clipboard.h"
#include "Audio/AudioOutput.h"
#include "Audio/AudioInput.h"
#include "Audio/AudioRenderer.h"
#include "View/Helper/Progress.h"
#include "Plugins/PluginManager.h"


string AppName = "Tsunami";
string AppVersion = "0.6.3.0";

Tsunami *tsunami = NULL;

Tsunami::Tsunami(Array<string> arg) :
	HuiApplication(arg, "tsunami", "Deutsch", HUI_FLAG_LOAD_RESOURCE)
{
	HuiSetProperty("name", AppName);
	HuiSetProperty("version", AppVersion);
	HuiSetProperty("comment", _("Editor f&ur Audio Dateien"));
	HuiSetProperty("website", "http://michi.is-a-geek.org/software");
	HuiSetProperty("copyright", "© 2007-2014 by MichiSoft TM");
	HuiSetProperty("author", "Michael Ankele <michi@lupina.de>");
}

Tsunami::~Tsunami()
{
	delete(storage);
	delete(output);
	delete(input);
	delete(audio);
	delete(renderer);
	delete(plugin_manager);
}

void Tsunami::onStartup(Array<string> arg)
{
	tsunami = this;

	progress = new Progress;
	log = new Log;

	clipboard = new Clipboard;

	output = new AudioOutput;
	input = new AudioInput;
	renderer = new AudioRenderer;

	audio = new AudioFile;
	audio->NewWithOneTrack(Track::TYPE_AUDIO, DEFAULT_SAMPLE_RATE);

	storage = new Storage;

	// create (link) PluginManager after all other components are ready
	plugin_manager = new PluginManager;
	plugin_manager->LinkAppScriptData();

	log->Info(AppName + " " + AppVersion);
	log->Info(_("  ...keine Sorge, das wird schon!"));

	HandleArguments(arg);
}

bool Tsunami::HandleArguments(Array<string> arg)
{
	if (arg.num <= 1){
		CreateWindow();
		return false;
	}
	if (arg[1] == "--info"){
		if (storage->Load(audio, arg[2])){
			msg_write(format("sample-rate: %d", audio->sample_rate));
			msg_write(format("samples: %d", audio->GetRange().num));
			foreach(Tag &t, audio->tag)
				msg_write("tag: " + t.key + " = " + t.value);
		}
		exit(0);
	}
	CreateWindow();
	return storage->Load(audio, arg[1]);
}


HuiWindow *GlobalMainWin = NULL;

void Tsunami::CreateWindow()
{
	win = new TsunamiWindow;
	plugin_manager->AddPluginsToMenu(win);
	GlobalMainWin = dynamic_cast<HuiWindow*>(tsunami->win);
	Script::LinkExternal("MainWin", &GlobalMainWin);

	win->Show();
	HuiRunLaterM(0.01f, win, &TsunamiWindow::OnViewOptimal);
}

void Tsunami::LoadKeyCodes()
{
}

HuiExecute(Tsunami);
