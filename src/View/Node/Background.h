/*
 * Background.h
 *
 *  Created on: 08.06.2019
 *      Author: michi
 */

#ifndef SRC_VIEW_NODE_BACKGROUND_H_
#define SRC_VIEW_NODE_BACKGROUND_H_

#include "ViewNode.h"

class AudioView;
class AudioViewLayer;

class Background : public ViewNodeFree {
public:
	Background(AudioView *view);

	bool on_left_button_down() override;
	bool on_right_button_down() override;
	bool allow_handle_click_when_gaining_focus() override { return false; }

	void draw_layer_separator(Painter *c, AudioViewLayer *l1, AudioViewLayer *l2);

	void draw(Painter *p) override;

	AudioView *view;
	HoverData get_hover_data(float mx, float my) override;
};

#endif /* SRC_VIEW_NODE_BACKGROUND_H_ */
