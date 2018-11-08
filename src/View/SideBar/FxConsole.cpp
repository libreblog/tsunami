/*
 * FxConsole.cpp
 *
 *  Created on: 20.03.2014
 *      Author: michi
 */

#include "FxConsole.h"
#include "../AudioView.h"
#include "../../Data/Song.h"
#include "../../Data/Track.h"
#include "../../Module/Audio/AudioEffect.h"
#include "../../Module/ConfigPanel.h"
#include "../../Plugins/PluginManager.h"
#include "../../Session.h"

class SingleFxPanel : public hui::Panel
{
public:
	SingleFxPanel(Session *_session, FxConsole *_console, Track *t, AudioEffect *_fx, int _index)
	{
		session = _session;
		console = _console;
		song = session->song;
		track = t;
		fx = _fx;
		index = _index;

		from_resource("fx_panel");

		set_string("name", fx->module_subtype);

		p = fx->create_panel();
		if (p){
			embed(p, "content", 0, 0);
			p->update();
		}else{
			set_target("content");
			add_label(_("not configurable"), 0, 1, "");
			hide_control("load_favorite", true);
			hide_control("save_favorite", true);
		}

		event("enabled", std::bind(&SingleFxPanel::onEnabled, this));
		event("delete", std::bind(&SingleFxPanel::onDelete, this));
		event("load_favorite", std::bind(&SingleFxPanel::onLoad, this));
		event("save_favorite", std::bind(&SingleFxPanel::onSave, this));
		event("show_large", std::bind(&SingleFxPanel::on_large, this));

		check("enabled", fx->enabled);

		old_param = fx->config_to_string();
		fx->subscribe(this, std::bind(&SingleFxPanel::onfxChange, this), fx->MESSAGE_CHANGE);
		fx->subscribe(this, std::bind(&SingleFxPanel::onfxChangeByAction, this), fx->MESSAGE_CHANGE_BY_ACTION);
	}
	virtual ~SingleFxPanel()
	{
		fx->unsubscribe(this);
	}
	void onLoad()
	{
		string name = session->plugin_manager->select_favorite_name(win, fx, false);
		if (name.num == 0)
			return;
		session->plugin_manager->apply_favorite(fx, name);
		if (track)
			track->edit_effect(fx, old_param);
		else
			song->edit_effect(fx, old_param);
		old_param = fx->config_to_string();
	}
	void onSave()
	{
		string name = session->plugin_manager->select_favorite_name(win, fx, true);
		if (name.num == 0)
			return;
		session->plugin_manager->save_favorite(fx, name);
	}
	void onEnabled()
	{
		if (track)
			track->enable_effect(fx, is_checked(""));
		else
			song->enable_effect(fx, is_checked(""));
	}
	void onDelete()
	{
		hui::RunLater(0, [&]{
			if (track)
				track->delete_effect(fx);
			else
				song->delete_effect(fx);
		});
	}
	void on_large()
	{
		console->set_exclusive(this);

	}
	void onfxChange()
	{
		if (track)
			track->edit_effect(fx, old_param);
		else
			song->edit_effect(fx, old_param);
		check("enabled", fx->enabled);
		if (p)
			p->update();
		old_param = fx->config_to_string();

	}
	void onfxChangeByAction()
	{
		check("enabled", fx->enabled);
		if (p)
			p->update();
		old_param = fx->config_to_string();
	}
	Session *session;
	Song *song;
	Track *track;
	AudioEffect *fx;
	string old_param;
	ConfigPanel *p;
	int index;
	FxConsole *console;
};

FxConsole::FxConsole(Session *session) :
	SideBarConsole(_("Effects"), session)
{
	id_inner = "fx_inner_table";

	from_resource("fx_editor");

	track = nullptr;
	exclusive = nullptr;
	//Enable("add", false);

	if (!view)
		hide_control("edit_track", true);

	event("add", std::bind(&FxConsole::on_add, this));

	event("edit_song", std::bind(&FxConsole::on_edit_song, this));
	event("edit_track", std::bind(&FxConsole::on_edit_track, this));

	if (view)
		view->subscribe(this, std::bind(&FxConsole::on_view_cur_track_change, this), view->MESSAGE_CUR_TRACK_CHANGE);
	song->subscribe(this, std::bind(&FxConsole::on_update, this), song->MESSAGE_NEW);
	song->subscribe(this, std::bind(&FxConsole::on_update, this), song->MESSAGE_ADD_EFFECT);
	song->subscribe(this, std::bind(&FxConsole::on_update, this), song->MESSAGE_DELETE_EFFECT);
}

FxConsole::~FxConsole()
{
	clear();
	if (view)
		view->unsubscribe(this);
	song->unsubscribe(this);
}

void FxConsole::on_enter()
{
	set_exclusive(nullptr);
}

void FxConsole::on_leave()
{
	set_exclusive(nullptr);
}

void FxConsole::on_set_large(bool large)
{
	if (!large){
		set_exclusive(nullptr);
	}
}

void FxConsole::on_add()
{
	string name = session->plugin_manager->choose_module(win, session, ModuleType::AUDIO_EFFECT);
	if (name == "")
		return;
	AudioEffect *effect = CreateAudioEffect(session, name);
	if (track)
		track->add_effect(effect);
	else
		song->add_effect(effect);
}

void FxConsole::on_edit_song()
{
	bar()->open(SideBar::SONG_CONSOLE);
}

void FxConsole::on_edit_track()
{
	bar()->open(SideBar::TRACK_CONSOLE);
}

void FxConsole::clear()
{
	if (track)
		track->unsubscribe(this);
	foreachi(hui::Panel *p, panels, i){
		delete(p);
		remove_control("separator_" + i2s(i));
	}
	panels.clear();
	track = nullptr;
	//Enable("add", false);
}

void FxConsole::set_track(Track *t)
{
	clear();
	set_exclusive(nullptr);
	track = t;
	if (track){
		track->subscribe(this, std::bind(&FxConsole::on_track_delete, this), track->MESSAGE_DELETE);
		track->subscribe(this, std::bind(&FxConsole::on_update, this), track->MESSAGE_ADD_EFFECT);
		track->subscribe(this, std::bind(&FxConsole::on_update, this), track->MESSAGE_DELETE_EFFECT);
	}


	Array<AudioEffect*> fx;
	if (track)
		fx = track->fx;
	else
		fx = song->fx;
	foreachi(AudioEffect *e, fx, i){
		auto *p = new SingleFxPanel(session, this, track, e, i);
		p->hide_control("content", !allow_show(p));
		panels.add(p);
		embed(panels.back(), id_inner, 0, i*2);
		add_separator("!horizontal", 0, i*2 + 1, "separator_" + i2s(i));
	}
	hide_control("comment_no_fx", fx.num > 0);
	//Enable("add", track);
}

void FxConsole::set_exclusive(hui::Panel *ex)
{
	exclusive = ex;
	bar()->set_large(exclusive);
	for (auto *p: panels){
		SingleFxPanel *pp = (SingleFxPanel*)p;
		pp->hide_control("content", !allow_show(pp));
		if (pp->p)
			pp->p->set_large(exclusive == pp);
	}
}

bool FxConsole::allow_show(hui::Panel *p)
{
	if (exclusive)
		return p == exclusive;
	return true;
}

void FxConsole::on_track_delete()
{
	set_track(nullptr);
}
void FxConsole::on_view_cur_track_change()
{
	set_track(view->cur_track());
}

void FxConsole::on_update()
{
	set_track(track);
}

