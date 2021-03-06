/*
 * ViewModeCurve.h
 *
 *  Created on: 14.11.2015
 *      Author: michi
 */

#ifndef SRC_VIEW_MODE_VIEWMODECURVE_H_
#define SRC_VIEW_MODE_VIEWMODECURVE_H_

#include "ViewModeDefault.h"

class Curve;

class ViewModeCurve : public ViewModeDefault
{
public:
	ViewModeCurve(AudioView *view);

	void on_start() override;

	void left_click_handle_void(AudioViewLayer *vlayer) override;
	void on_key_down(int k) override;

	void draw_track_data(Painter *c, AudioViewTrack *t) override;
	void draw_post(Painter *c) override;

	string get_tip() override;

	HoverData get_hover_data(AudioViewLayer *vlayer, float mx, float my) override;


	float value2screen(float value);
	float screen2value(float y);

	Curve *curve;
	AudioViewTrack *cur_track();
	void set_curve(Curve *c);
};

#endif /* SRC_VIEW_MODE_VIEWMODECURVE_H_ */
