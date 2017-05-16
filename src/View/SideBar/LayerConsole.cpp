/*
 * LayerConsole.cpp
 *
 *  Created on: 23.05.2015
 *      Author: michi
 */


#include "LayerConsole.h"

#include "../../Tsunami.h"
#include "../../Stuff/Observer.h"
#include "../../View/AudioView.h"
#include "../../Data/Song.h"
#include "../../Stuff/Log.h"


LayerConsole::LayerConsole(Song *s, AudioView *v) :
	SideBarConsole(_("Layers")),
	Observer("LayerConsole")
{
	song = s;
	view = v;

	// dialog
	setBorderWidth(5);
	embedDialog("layer_dialog", 0, 0);
	setDecimals(1);

	loadData();

	eventX("layers", "hui:select", std::bind(&LayerConsole::onSelect, this));
	eventX("layers", "hui:change", std::bind(&LayerConsole::onEdit, this));
	event("add_layer", std::bind(&LayerConsole::onAdd, this));
	event("delete_layer", std::bind(&LayerConsole::onDelete, this));
	event("merge_layer", std::bind(&LayerConsole::onMerge, this));

	event("edit_song", std::bind(&LayerConsole::onEditSong, this));

	subscribe(song);
	subscribe(view, view->MESSAGE_CUR_LAYER_CHANGE);
}

LayerConsole::~LayerConsole()
{
	unsubscribe(song);
	unsubscribe(view);
}

void LayerConsole::loadData()
{
	reset("layers");
	foreachi(string &n, song->layer_names, i)
		addString("layers", i2s(i + 1) + "\\" + n);
	if (song->layer_names.num > 0)
		setInt("layers", view->cur_layer);

	enable("delete_layer", song->layer_names.num > 1);
	enable("merge_layer", view->cur_layer > 0);
}


void LayerConsole::onSelect()
{
	int s = getInt("layers");
	view->setCurLayer(s);
}

void LayerConsole::onEdit()
{
	int r = hui::GetEvent()->row;
	if (r < 0)
		return;
	song->renameLayer(r, getCell("layers", r, 1));
}

void LayerConsole::onAdd()
{
	int cur_layer = view->cur_layer;
	song->addLayer("", cur_layer + 1);
	view->setCurLayer(cur_layer + 1);
}

void LayerConsole::onDelete()
{
	try{
		song->deleteLayer(view->cur_layer);
	}catch(SongException &e){
		tsunami->log->error(e.message);
	}
}

void LayerConsole::onMerge()
{
	int s = getInt("layers");
	if (s >= 1)
		song->mergeLayers(s, s - 1);
}

void LayerConsole::onEditSong()
{
	((SideBar*)parent)->open(SideBar::SONG_CONSOLE);
}

void LayerConsole::onUpdate(Observable *o, const string &message)
{
	loadData();
}