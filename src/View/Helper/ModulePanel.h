/*
 * ModulePanel.h
 *
 *  Created on: Jun 21, 2019
 *      Author: michi
 */

#ifndef SRC_VIEW_HELPER_MODULEPANEL_H_
#define SRC_VIEW_HELPER_MODULEPANEL_H_

#include "../../lib/hui/hui.h"

class Module;
class ConfigPanel;
class Session;


class ModulePanel : public hui::Panel {
public:
	
	enum Mode {
		NONE = 0,
		HEADER = 1,
		FAVOURITES = 2,
		ENABLE = 4,
		DELETE = 8,
		DEFAULT = HEADER | FAVOURITES | ENABLE
	};
	
	ModulePanel(Module *m, Mode mode = Mode::DEFAULT);
	virtual ~ModulePanel();
	void on_load();
	void on_save();
	void on_enabled();
	void on_delete();
	void on_large();
	void on_external();
	void on_change();
	void on_change_by_action();
	
	ModulePanel *copy();
	
	void set_func_enabled(std::function<void(bool)> func_enable);
	void set_func_delete(std::function<void()> func_delete);
	void set_func_edit(std::function<void(const string&)> func_edit);
	
	std::function<void(bool)> func_enable;
	std::function<void(const string&)> func_edit;
	std::function<void()> func_delete;
	Session *session;
	Module *module;
	string old_param;
	ConfigPanel *p;
};



#endif /* SRC_VIEW_HELPER_MODULEPANEL_H_ */
