/*
 * ConfigurableSelectorDialog.h
 *
 *  Created on: 16.08.2013
 *      Author: michi
 */

#ifndef CONFIGURABLESELECTORDIALOG_H_
#define CONFIGURABLESELECTORDIALOG_H_

#include "../../lib/hui/hui.h"

class Session;
enum class ModuleType;

class ConfigurableSelectorDialog: public hui::Window
{
public:
	ConfigurableSelectorDialog(hui::Window *_parent, ModuleType type, Session *session, const string &old_name = "");
	virtual ~ConfigurableSelectorDialog();

	void onListSelect();
	void onSelect();
	void onClose();
	void onCancel();
	void onOk();

	ModuleType type;
	Session *session;
	struct Label
	{
		string full;
		string name, group;
	};
	Array<Label> labels;
	Set<string> ugroups;

	static Label split_label(const string &s);

	string _return;
};

#endif /* CONFIGURABLESELECTORDIALOG_H_ */
