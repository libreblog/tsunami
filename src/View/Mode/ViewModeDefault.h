/*
 * ViewModeDefault.h
 *
 *  Created on: 12.11.2015
 *      Author: michi
 */

#ifndef SRC_VIEW_MODE_VIEWMODEDEFAULT_H_
#define SRC_VIEW_MODE_VIEWMODEDEFAULT_H_

#include "ViewMode.h"

class ActionTrackMoveSample;

class ViewModeDefault : public ViewMode
{
public:
	ViewModeDefault(AudioView *view, ViewMode *parent);
	virtual ~ViewModeDefault();

	virtual void onLeftButtonDown();
	virtual void onLeftButtonUp();
	virtual void onLeftDoubleClick();
	virtual void onRightButtonDown();
	virtual void onRightButtonUp();
	virtual void onMouseWheel();
	virtual void onMouseMove();
	virtual void onKeyDown(int k);
	virtual void onKeyUp(int k);
	virtual int getTrackHeightMin(Track *t);
	virtual int getTrackHeightMax(Track *t);

	virtual void drawGridBars(HuiPainter *c, const rect &r, const color &bg, bool show_time);
	virtual void drawTrackBackground(HuiPainter *c, AudioViewTrack *t);
	virtual void drawTrackData(HuiPainter *c, AudioViewTrack *t);

	void selectUnderMouse();
	void setCursorPos(int pos);
	virtual Selection getHover();

	void setBarriers(Selection *s);
	void applyBarriers(int &pos);

	float mouse_possibly_selecting;
	int mouse_possibly_selecting_start;

	ActionTrackMoveSample *cur_action;
};

#endif /* SRC_VIEW_MODE_VIEWMODEDEFAULT_H_ */