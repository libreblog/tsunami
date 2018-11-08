/*
 * MiniBar.cpp
 *
 *  Created on: 23.03.2014
 *      Author: michi
 */

#include "BottomBar.h"
#include "../Helper/CpuDisplay.h"
#include "../../Device/OutputStream.h"
#include "../../Device/DeviceManager.h"
#include "../../Session.h"
#include "../AudioView.h"
#include "MiniBar.h"
#include "../Helper/PeakMeterDisplay.h"

MiniBar::MiniBar(BottomBar *_bottom_bar, Session *_session)
{
	session = _session;
	view = session->view;
	dev_manager = session->device_manager;
	bottom_bar = _bottom_bar;

	from_resource("mini_bar");

	peak_meter = new PeakMeterDisplay(this, "peaks", view->peak_meter);
	set_float("volume", dev_manager->get_output_volume());

	cpu_display = new CpuDisplay(this, "cpu", session);

	event("show_bottom_bar", std::bind(&MiniBar::on_show_bottom_bar, this));
	event("volume", std::bind(&MiniBar::on_volume, this));

	bottom_bar->subscribe(this, std::bind(&MiniBar::on_bottom_bar_update, this));
	dev_manager->subscribe(this, std::bind(&MiniBar::on_volume_change, this));
}

MiniBar::~MiniBar()
{
	dev_manager->unsubscribe(this);
	bottom_bar->unsubscribe(this);
	delete(peak_meter);
	delete(cpu_display);
}

void MiniBar::on_show_bottom_bar()
{
	bottom_bar->_show();
	hide();
}

void MiniBar::on_volume()
{
	dev_manager->set_output_volume(get_float(""));
}

void MiniBar::on_show()
{
	peak_meter->enable(true);
}

void MiniBar::on_hide()
{
	peak_meter->enable(false);
}

void MiniBar::on_bottom_bar_update()
{
	if (bottom_bar->visible)
		hide();
	else
		show();
}

void MiniBar::on_volume_change()
{
	set_float("volume", dev_manager->get_output_volume());
}


