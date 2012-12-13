/*
 * Storage.cpp
 *
 *  Created on: 24.03.2012
 *      Author: michi
 */

#include "Format.h"
#include "FormatWave.h"
#include "FormatOgg.h"
#include "FormatFlac.h"
#include "FormatNami.h"
#include "../Tsunami.h"
#include "../lib/hui/hui.h"

Storage::Storage()
{
	format.add(new FormatNami());
	format.add(new FormatWave());
	format.add(new FormatOgg());
	format.add(new FormatFlac());

	CurrentDirectory = HuiConfigReadStr("CurrentDirectory", "");
}

Storage::~Storage()
{
	HuiConfigWriteStr("CurrentDirectory", CurrentDirectory);

	foreach(Format *f, format)
		delete(f);
	format.clear();
}

bool Storage::Load(AudioFile *a, const string &filename)
{
	msg_db_r("Storage.Load", 1);
	bool ok = false;
	bool found = false;

	CurrentDirectory = filename.dirname();
	string ext = filename.extension();

	foreach(Format *f, format)
		if (f->CanHandle(ext)){
			tsunami->progress->Start(_("lade"), 0);

			a->NotifyBegin();

			a->Reset();
			a->action_manager->Enable(false);
			a->used = true;
			a->filename = filename;

			f->LoadAudio(a, filename);


			a->action_manager->Enable(true);
			tsunami->progress->Set("peaks", 1);
			a->UpdatePeaks(tsunami->view->PeakMode);

			tsunami->progress->End();
			tsunami->ForceRedraw();
			if (a->track.num > 0)
				a->SetCurTrack(a->track[0]);
			else
				a->used = false;
			a->action_manager->Reset();
			a->NotifyEnd();
			ok = a->used;
			found = true;
			break;
		}

	if (!found)
		tsunami->log->Error(_("unbekannte Dateiendung: ") + ext);

	msg_db_l(1);
	return ok;
}

bool Storage::LoadTrack(Track *t, const string &filename)
{
	msg_db_r("Storage.LoadTrack", 1);
	bool ok = false;
	bool found = false;

	CurrentDirectory = filename.dirname();
	string ext = filename.extension();

	foreach(Format *f, format)
		if (f->CanHandle(ext)){
			tsunami->progress->Start(_("lade"), 0);

			AudioFile *a = t->root;
			a->NotifyBegin();

			f->LoadTrack(t, filename);

			tsunami->progress->End();
			tsunami->ForceRedraw();
			a->NotifyEnd();
			found = true;
			break;
		}

	if (!found)
		tsunami->log->Error(_("unbekannte Dateiendung: ") + ext);

	msg_db_l(1);
	return ok;
}

bool Storage::Save(AudioFile *a, const string &filename)
{
	msg_db_r("Storage.Save", 1);
	bool ok = false;
	bool found = false;

	CurrentDirectory = filename.dirname();
	string ext = filename.extension();

	foreach(Format *f, format)
		if (f->CanHandle(ext)){
			if (!f->TestFormatCompatibility(a))
				tsunami->log->Warning(_("Datenverlust!"));

			tsunami->progress->Start(_("speichere"), 0);

			a->filename = filename;

			f->SaveAudio(a, filename);

			a->action_manager->MarkCurrentAsSave();
			tsunami->progress->End();
			tsunami->UpdateMenu();
			ok = true;
			found = true;
			break;
		}

	if (!found)
		tsunami->log->Error(_("unbekannte Dateiendung: ") + ext);

	msg_db_l(1);
	return ok;
}

bool Storage::Export(AudioFile *a, const string &filename)
{
	msg_db_r("Storage.Export", 1);
	bool ok = false;
	bool found = false;

	CurrentDirectory = filename.dirname();
	string ext = filename.extension();

	foreach(Format *f, format)
		if (f->CanHandle(ext)){
			tsunami->progress->Start(_("exportiere"), 0);

			// render audio...
			tsunami->progress->Set(_("rendere Audio"), 0);
			Range r = a->GetRange();
			if (!a->selection.empty())
				r = a->selection;
			BufferBox buf = tsunami->renderer->RenderAudioFile(a, r);

			// save
			f->SaveBuffer(a, &buf, filename);

			tsunami->progress->End();
			ok = true;
			found = true;
			break;
		}

	if (!found)
		tsunami->log->Error(_("unbekannte Dateiendung: ") + ext);

	msg_db_l(1);
	return ok;
}

bool Storage::AskByFlags(CHuiWindow *win, const string &title, bool save, int flags)
{
	string filter, filter_show;
	foreach(Format *f, format)
		if ((f->flags & flags) == flags){
			if (filter != "")
				filter += ";";
			filter += "*." + f->extension;
			if (filter_show != "")
				filter_show += ",";
			filter_show += "*." + f->extension;
		}
	if (save)
		return HuiFileDialogSave(win, title, CurrentDirectory, filter_show, filter);
	else
		return HuiFileDialogOpen(win, title, CurrentDirectory, filter_show, filter);
}

bool Storage::AskOpen(CHuiWindow *win)
{
	return AskByFlags(win, _("Datei &offnen"), false, 0);
}

bool Storage::AskSave(CHuiWindow *win)
{
	return AskByFlags(win, _("Datei speichern"), true, 0);
}

bool Storage::AskOpenImport(CHuiWindow *win)
{
	return AskByFlags(win, _("Datei importieren"), false, Format::FLAG_SINGLE_TRACK);
}

bool Storage::AskSaveExport(CHuiWindow *win)
{
	return AskByFlags(win, _("Datei exportieren"), true, Format::FLAG_SINGLE_TRACK);
}
