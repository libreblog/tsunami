/*
 * BottomBar.h
 *
 *  Created on: 23.03.2014
 *      Author: michi
 */

#ifndef SRC_VIEW_BOTTOMBAR_BOTTOMBAR_H_
#define SRC_VIEW_BOTTOMBAR_BOTTOMBAR_H_

#include "../../lib/hui/hui.h"
#include "../../Stuff/Observable.h"

class AudioView;
class Song;
class DeviceManager;
class Track;
class MixingConsole;
class LogConsole;
class DeviceConsole;
class SignalEditor;
class Session;
class BottomBarConsole;
class PluginConsole;

class BottomBar : public Observable<hui::Panel> {
public:
	BottomBar(Session *session);
	virtual ~BottomBar();


	enum {
		MIXING_CONSOLE,
		SIGNAL_EDITOR,
		DEVICE_CONSOLE,
		PLUGIN_CONSOLE,
		LOG_CONSOLE
	};


	class Console : public hui::Panel {
	public:
		Console(const string &_title, Session *_session);
		string title;
		Session *session;
		Song *song;
		AudioView *view;
		BottomBar *bar(){ return dynamic_cast<BottomBar*>(parent); }


		void blink();
		bool notify;
	};

	void on_close();
	void on_choose();

	void _show();
	void _hide();

	void choose(Console *console);
	void open(Console *console);
	void open(int console_index);
	void toggle(int console_index);
	bool is_active(int console_index);
	Console *active_console;
	bool visible;

	MixingConsole *mixing_console;
	SignalEditor *signal_editor;
	DeviceConsole *device_console;
	PluginConsole *plugin_console;
	LogConsole *log_console;

	int index(Console *console);

	Array<Console*> consoles;
	void add_console(Console *c, const string &list_name);
};

#endif /* SRC_VIEW_BOTTOMBAR_BOTTOMBAR_H_ */
