/*
 * HuiToolItemSeparatorGtk.cpp
 *
 *  Created on: 26.06.2013
 *      Author: michi
 */

#include "HuiToolItemSeparator.h"

#ifdef HUI_API_GTK

HuiToolItemSeparator::HuiToolItemSeparator() :
	HuiControl(HUI_KIND_TOOL_SEPARATOR, "")
{
	widget = GTK_WIDGET(gtk_separator_tool_item_new());
}

#endif

