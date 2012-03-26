/*
 * AudioView.cpp
 *
 *  Created on: 24.03.2012
 *      Author: michi
 */

#include "AudioView.h"
#include "../Tsunami.h"
#include "../View/Dialog/AudioFileDialog.h"
#include "../View/Dialog/TrackDialog.h"
#include "../View/Dialog/SubDialog.h"

AudioView::AudioView() :
	Observable("AudioView"),
	SUB_FRAME_HEIGHT(20),
	TIME_SCALE_HEIGHT(20),
	BarrierDist(3),
	ScrollSpeed(300),
	ScrollSpeedFast(3000),
	ZoomSpeed(0.1f),
	MouseMinMoveToSelect(5)
{
	ColorBackground = White;
	ColorBackgroundCurWave = color(1, 0.93f, 0.93f, 1);
	ColorBackgroundCurTrack = color(1, 0.85f, 0.85f, 1);
	ColorGrid = color(1, 0.75f, 0.75f, 0.9f);
	ColorSelectionInternal = color(1, 0.7f, 0.7f, 0.9f);
	ColorSelectionBoundary = Blue;
	ColorSelectionBoundaryMO = Red;
	ColorPreviewMarker = color(1,0, 0.8f, 0);
	ColorWave = Gray;
	ColorWaveCur = color(1, 0.3f, 0.3f, 0.3f);
	ColorSub = color(1, 0.6f, 0.6f, 0);
	ColorSubMO = color(1, 0.6f, 0, 0);
	ColorSubNotCur = color(1, 0.4f, 0.4f, 0.4f);

	DetailSteps = HuiConfigReadInt("DetailSteps", 1);
	MouseMinMoveToSelect = HuiConfigReadInt("MouseMinMoveToSelect", 5);
	PreviewSleepTime = HuiConfigReadInt("PreviewSleepTime", 10);


	MousePossiblySelecting = -1;

	tsunami->SetBorderWidth(0);
	tsunami->SetTarget("main_table", 0);
	tsunami->AddDrawingArea("", 0, 0, 0, 0, "area");

	Subscribe(tsunami->audio[0]);
	Subscribe(tsunami->audio[1]);

	// events
	tsunami->EventMX("area", "hui:redraw", this, (void(HuiEventHandler::*)())&AudioView::OnDraw);
	tsunami->EventMX("area", "hui:mouse-move", this, (void(HuiEventHandler::*)())&AudioView::OnMouseMove);
	tsunami->EventMX("area", "hui:left-button-down", this, (void(HuiEventHandler::*)())&AudioView::OnLeftButtonDown);
	tsunami->EventMX("area", "hui:left-double-click", this, (void(HuiEventHandler::*)())&AudioView::OnLeftDoubleClick);
	tsunami->EventMX("area", "hui:left-button-up", this, (void(HuiEventHandler::*)())&AudioView::OnLeftButtonUp);
	tsunami->EventMX("area", "hui:middle-button-down", this, (void(HuiEventHandler::*)())&AudioView::OnMiddleButtonDown);
	tsunami->EventMX("area", "hui:middle-button-up", this, (void(HuiEventHandler::*)())&AudioView::OnMiddleButtonUp);
	tsunami->EventMX("area", "hui:right-button-down", this, (void(HuiEventHandler::*)())&AudioView::OnRightButtonDown);
	tsunami->EventMX("area", "hui:right-button-up", this, (void(HuiEventHandler::*)())&AudioView::OnRightButtonUp);
	//tsunami->EventMX("area", "hui:key-down", this, (void(HuiEventHandler::*)())&AudioView::OnKeyDown);
	tsunami->EventM("hui:key-down", this, (void(HuiEventHandler::*)())&AudioView::OnKeyDown);
	tsunami->EventM("hui:key-up", this, (void(HuiEventHandler::*)())&AudioView::OnKeyUp);
	tsunami->EventMX("area", "hui:mouse-wheel", this, (void(HuiEventHandler::*)())&AudioView::OnMouseWheel);


	HuiAddCommandM("select_none", "", -1, this, (void(HuiEventHandler::*)())&AudioView::OnSelectNone);
	HuiAddCommandM("select_all", "", KEY_A + KEY_CONTROL, this, (void(HuiEventHandler::*)())&AudioView::OnSelectAll);
	HuiAddCommandM("select_nothing", "", -1, this, (void(HuiEventHandler::*)())&AudioView::OnSelectNothing);
	HuiAddCommandM("view_temp_file", "", -1, this, (void(HuiEventHandler::*)())&AudioView::OnViewTempFile);
	HuiAddCommandM("view_mono", "", -1, this, (void(HuiEventHandler::*)())&AudioView::OnViewMono);
	HuiAddCommandM("view_grid", "", -1, this, (void(HuiEventHandler::*)())&AudioView::OnViewGrid);
	HuiAddCommandM("view_optimal", "", -1, this, (void(HuiEventHandler::*)())&AudioView::OnViewOptimal);
	HuiAddCommandM("zoom_in", "", -1, this, (void(HuiEventHandler::*)())&AudioView::OnZoomIn);
	HuiAddCommandM("zoom_out", "", -1, this, (void(HuiEventHandler::*)())&AudioView::OnZoomOut);
	HuiAddCommandM("jump_other_file", "", -1, this, (void(HuiEventHandler::*)())&AudioView::OnJumpOtherFile);

	ForceRedraw();
	UpdateMenu();

	ShowTempFile = false;
	ShowMono = false;
	ShowGrid = true;
}

AudioView::~AudioView()
{
}


void AudioView::SetMouse()
{
	mx = HuiGetEvent()->mx;
	my = HuiGetEvent()->my;
}

void AudioView::ClearMouseOver(AudioFile *a)
{
	a->mo_sel_start = false;
	a->mo_sel_end = false;
	foreach(a->track, t){
		t.is_mouse_over = false;
		foreach(t.sub, s)
			s.is_mouse_over = false;
	}
}

bool AudioView::MouseOverAudio(AudioFile *a)
{
	return ((mx >= a->x) && (mx < a->x + a->width) && (my >= a->y) && (my < a->y + a->height));
}

bool AudioView::MouseOverTrack(Track *t)
{
	return ((mx >= t->x) && (mx < t->x + t->width) && (my >= t->y) && (my < t->y + t->height));
}

int AudioView::MouseOverSub(Track *s)
{
	if ((mx >= s->x) && (mx < s->x + s->width)){
		int offset = s->root->screen2sample(mx) - s->pos;
		if ((my >= s->y) && (my < s->y + SUB_FRAME_HEIGHT))
			return offset;
		if ((my >= s->y + s->height - SUB_FRAME_HEIGHT) && (my < s->y + s->height))
			return offset;
	}
	return -1;
}

void AudioView::SelectionUpdatePos(SelectionType &s)
{
	if (s.audio)
		s.pos = s.audio->screen2sample(mx);
}

AudioView::SelectionType AudioView::GetMouseOver()
{
	msg_db_r("GetMouseOver", 2);

	ClearMouseOver(tsunami->audio[0]);
	ClearMouseOver(tsunami->audio[1]);
	SelectionType s;
	s.type = SEL_TYPE_NONE;
	s.audio = NULL;
	s.track = NULL;
	s.sub = NULL;
	s.sub_offset = 0;

	// audio file?
	for (int i=0;i<2;i++)
		if (MouseOverAudio(tsunami->audio[i])){
			s.audio = tsunami->audio[i];
			s.type = SEL_TYPE_AUDIO;
		}

	// ???
	if (s.audio){
		SelectionUpdatePos(s);
		if (s.audio->selection){
			int ssx = s.audio->sample2screen(s.audio->sel_start_raw);
			if ((mx >= ssx - 5) && (mx <= ssx + 5)){
				s.type = SEL_TYPE_SELECTION_START;
				s.audio->mo_sel_start = true;
				msg_db_l(2);
				return s;
			}
			int sex = s.audio->sample2screen(s.audio->sel_end_raw);
			if ((mx >= sex - 5) && (mx <= sex + 5)){
				s.type = SEL_TYPE_SELECTION_END;
				s.audio->mo_sel_end = true;
				msg_db_l(2);
				return s;
			}
		}
	}

	// track?
	if (s.audio){
		foreach(s.audio->track, t){
			if (MouseOverTrack(&t)){
				s.track = &t;
				s.type = SEL_TYPE_TRACK;
				t.is_mouse_over = true;
			}
		}
	}

	// sub?
	if (s.track){
		// TODO: prefer selected subs
		foreach(s.track->sub, ss){
			int offset = MouseOverSub(&ss);
			if (offset >= 0){
				s.sub = &ss;
				s.type = SEL_TYPE_SUB;
				s.sub_offset = offset;
				ss.is_mouse_over = true;
			}
		}
	}

	msg_db_l(2);
	return s;
}


void AudioView::SelectUnderMouse()
{
	msg_db_r("SelectUnderMouse", 2);
	Selection = GetMouseOver();
	SetCurAudioFile(Selection.audio);
	tsunami->cur_audio = Selection.audio;
	Track *t = Selection.track;
	Track *s = Selection.sub;
	bool control = tsunami->GetKey(KEY_CONTROL);

	// track
	if (Selection.type == SEL_TYPE_TRACK){
		SelectTrack(t, control);
		if (t->is_selected)
			SetCurTrack(tsunami->cur_audio, t);
		if (!control)
			tsunami->cur_audio->UnselectAllSubs();
	}

	// sub
	if (Selection.type == SEL_TYPE_SUB){
		SelectSub(s, control);
		if (s->is_selected)
			SetCurSub(tsunami->cur_audio, s);
	}
	msg_db_l(2);
}

void AudioView::SetBarriers(AudioFile *a, SelectionType *s)
{
	msg_db_r("SetBarriers", 2);
	s->barrier.clear();
	if (s->type == SEL_TYPE_NONE){
		msg_db_l(2);
		return;
	}

	int dpos = 0;
	if (s->type == SEL_TYPE_SUB)
		dpos = s->sub_offset;

	foreach(a->track, t){
		// add subs
		foreach(t.sub, sub){
			s->barrier.add(sub.pos + dpos);
		}

		// time bar...
		foreach(t.bar_col, bc){
			int x0 = bc.pos;
			foreach(bc.bar, b){
				for (int i=0;i<b.num_beats;i++)
					s->barrier.add(x0 + (int)((float)b.length * i / (float)b.num_beats) + dpos);
				x0 += b.length;
			}
		}
	}

	// selection marker
	if (a->selection){
		s->barrier.add(a->sel_start_raw);
		if (MousePossiblySelecting < 0)
			s->barrier.add(a->sel_end_raw);
	}
	msg_db_l(2);
}

void AudioView::ApplyBarriers(int &pos)
{
	msg_db_r("ApplyBarriers", 2);
	AudioFile *a = Selection.audio;
	foreach(Selection.barrier, b){
		int dpos = a->sample2screen(b) - a->sample2screen(pos);
		if (abs(dpos) <= BarrierDist){
			//msg_write(format("barrier:  %d  ->  %d", pos, b));
			pos = b;
		}
	}
	msg_db_l(2);
}

void AudioView::OnMouseMove()
{
	msg_db_r("OnMouseMove", 2);
	SetMouse();
	bool _force_redraw_ = false;

	if (HuiGetEvent()->lbut)
		SelectionUpdatePos(Selection);
	else{
		SelectionType mo = GetMouseOver();
		_force_redraw_ |= (mo.type != mo_old.type) || (mo.sub != mo_old.sub);
		mo_old = mo;
	}

	// mouse over?
	/*if (Selector < 0){
		int mo = MouseOver;
		Track *so = SubMouseOver;
		MouseOver = -1;
		SubMouseOver = NULL;
		int ssx = sample2screen(cur_audio, cur_audio->selection_start);
		int sex = sample2screen(cur_audio, cur_audio->selection_end);
		if ((mx>ssx-5) && (mx<ssx+5))
			MouseOver = MOSelectionStart;
		else if ((mx>sex-5) && (mx<sex+5))
			MouseOver = MOSelectionEnd;
		else{
			if (MouseOverSub(cur_sub, cur_track, cur_audio)){
				MouseOver = MOAdded;
				SubMouseOver = cur_sub;
				added_off = MouseOverSubOffset;
				mo_sub = cur_audio->cur_sub;
			}else{
				foreach(cur_audio->track, t){
					foreachi(t->sub, s, i){
						if (MouseOverSub(s, t, cur_audio)){
							MouseOver = MOAdded;
							SubMouseOver = s;
							added_off = MouseOverSubOffset;
							mo_sub = i;
						}
					}
				}
			}
		}*/
	//}


	// drag & drop
	if (Selection.type == SEL_TYPE_SELECTION_END){
		ApplyBarriers(Selection.pos);
		Selection.audio->sel_end_raw = Selection.pos;
		Selection.audio->selection = true;
		Selection.audio->UpdateSelection();
		//_force_redraw_ = true;
		_force_redraw_ = false;
		int x, w;
		int r = 4;
		if (HuiGetEvent()->dx < 0){
			x = mx - r;
			w = - HuiGetEvent()->dx + 2*r;
		}else{
			x = mx + r;
			w = - HuiGetEvent()->dx - 2*r;
		}
		tsunami->RedrawRect("area", x, Selection.audio->y, w, Selection.audio->height);
	}else if (Selection.type == SEL_TYPE_SUB){
		ApplyBarriers(Selection.pos);
		int dpos = (float)Selection.pos - Selection.sub_offset - Selection.sub->pos;
		foreach(tsunami->cur_audio->track, tt){
			foreach(tt.sub, s){
				if (s.is_selected)
					s.pos += dpos;
			}
		}
		//ChangeTrack(cur_sub);
		_force_redraw_ = true;
	}

	// selection:
	if (!HuiGetEvent()->lbut){
		MousePossiblySelecting = -1;
	}
	if (MousePossiblySelecting >= 0)
		MousePossiblySelecting += abs(HuiGetEvent()->dx);
	if (MousePossiblySelecting > MouseMinMoveToSelect){
		tsunami->cur_audio->sel_start_raw = MousePossiblySelectingStart;
		tsunami->cur_audio->sel_end_raw = Selection.pos;
		tsunami->cur_audio->selection = true;
		tsunami->cur_audio->mo_sel_end = true;
		SetBarriers(tsunami->cur_audio, &Selection);
		tsunami->cur_audio->UpdateSelection();
		Selection.type = SEL_TYPE_SELECTION_END;
		_force_redraw_ = true;
		MousePossiblySelecting = -1;
	}

	if (_force_redraw_)
		ForceRedraw();

	msg_db_l(2);
}



void AudioView::OnLeftButtonDown()
{
	msg_db_r("OnLBD", 2);
	SelectUnderMouse();
	UpdateMenu();

	if (!tsunami->cur_audio->used){
		msg_db_l(2);
		return;
	}

	// selection:
	//   start after lb down and moving
	if ((Selection.type == SEL_TYPE_TRACK) || (Selection.type == SEL_TYPE_AUDIO) || (Selection.type == SEL_TYPE_TIME)){
		MousePossiblySelecting = 0;
		int pos = tsunami->cur_audio->screen2sample(mx);
		MousePossiblySelectingStart = pos;
	}else if (Selection.type == SEL_TYPE_SELECTION_START){
		// switch end / start
		Selection.type = SEL_TYPE_SELECTION_END;
		int t = tsunami->cur_audio->sel_start_raw;
		tsunami->cur_audio->sel_start_raw = tsunami->cur_audio->sel_end_raw;
		tsunami->cur_audio->sel_end_raw = t;
		tsunami->cur_audio->mo_sel_start = false;
		tsunami->cur_audio->mo_sel_end = true;
	}

	SetBarriers(tsunami->cur_audio, &Selection);

	ForceRedraw();
	UpdateMenu();
	msg_db_l(2);
}



void AudioView::OnLeftButtonUp()
{
	msg_db_r("OnLBU", 2);
/*	if (Selection.type == SEL_TYPE_SUB)
		tsunami->cur_audio->history->Change();*/
	// TODO !!!!!!!!
	Selection.type = SEL_TYPE_NONE;
	ForceRedraw();
	UpdateMenu();
	msg_db_l(2);
}



void AudioView::OnMiddleButtonDown()
{
	SelectUnderMouse();

	SelectNone(tsunami->cur_audio);
	UpdateMenu();
}



void AudioView::OnMiddleButtonUp()
{
}



void AudioView::OnRightButtonDown()
{
	SelectUnderMouse();

	// pop up menu...
	UpdateMenu();
}



void AudioView::OnRightButtonUp()
{
}



void AudioView::OnLeftDoubleClick()
{
	SelectUnderMouse();

	if (MousePossiblySelecting < MouseMinMoveToSelect)
		if (tsunami->cur_audio->used){
			if (Selection.type == SEL_TYPE_SUB)
				ExecuteSubDialog(tsunami, Selection.sub);
			else if (Selection.type == SEL_TYPE_TRACK)
				ExecuteTrackDialog(tsunami, Selection.track);
			else
				ExecuteAudioDialog(tsunami, tsunami->cur_audio);
			Selection.type = SEL_TYPE_NONE;
		}
	UpdateMenu();
}



void AudioView::OnCommand(const string & id)
{
}



void AudioView::OnKeyDown()
{
	int k = HuiGetEvent()->key;

// view
	// moving
	float dt = 0.05f;
	if (k == KEY_RIGHT)
		MoveView(tsunami->cur_audio,  ScrollSpeed * dt / tsunami->cur_audio->view_zoom);
	if (k == KEY_LEFT)
		MoveView(tsunami->cur_audio, - ScrollSpeed * dt / tsunami->cur_audio->view_zoom);
	if (k == KEY_NEXT)
		MoveView(tsunami->cur_audio,  ScrollSpeedFast * dt / tsunami->cur_audio->view_zoom);
	if (k == KEY_PRIOR)
		MoveView(tsunami->cur_audio, - ScrollSpeedFast * dt / tsunami->cur_audio->view_zoom);
	// zoom
	if (k == KEY_ADD)
		ZoomAudioFile(tsunami->cur_audio, exp(  ZoomSpeed));
	if (k == KEY_SUBTRACT)
		ZoomAudioFile(tsunami->cur_audio, exp(- ZoomSpeed));
	UpdateMenu();
}



void AudioView::OnKeyUp()
{
}



void AudioView::OnMouseWheel()
{
	ZoomAudioFile(tsunami->cur_audio, exp(ZoomSpeed * HuiGetEvent()->dz));
}


void AudioView::ForceRedraw()
{
	force_redraw = true;
	tsunami->Redraw("area");
}


#define MIN_GRID_DIST	10.0f

float tx[2048], ty[2048];

inline int fill_line_buffer(int width, int di, float pos, float zoom, float f, float hf, float x, float y0, const Array<float> &buf, int offset, float sign)
{
	int nl = 0;
	float dpos = (float)1/zoom/f;
	// pixel position
	// -> buffer position
	float p0 = pos / f;
	for (int i=0;i<width-1;i+=di){

		float p = p0 + dpos * (float)i;
		int ip = (int)p - offset;
		//printf("%f  %f\n", p1, p2);
		if ((ip >= 0) && (ip < buf.num))
		if (((int)(p * f) < offset + buf.num) && (p >= offset)){
			tx[nl] = (float)x+i;
			ty[nl] = y0 + buf[ip] * hf * sign;
//			msg_write(ip);
//			msg_write(f2s(buf[ip], 5));
			nl ++;
		}
		//p += dpos;
	}
	return nl - 1;
}

void AudioView::DrawBuffer(HuiDrawingContext *c, int x, int y, int width, int height, Track *t, int pos, float zoom, const color &col)
{
	msg_db_r("DrawBuffer", 1);
//	int l = 0;
	float f = 1.0f;

	// which level of detail?
	/*if (zoom < 0.8f)
		for (int i=NUM_PEAK_LEVELS-1;i>=0;i--){
			float _f = (float)pow(PeakFactor, (float)i);
			if (_f > 1.0f / zoom){
				l = i;
				f = _f;
			}
		}*/

	// zero heights of both channels
	float y0r = (float)y + (float)height / 4;
	float y0l = (float)y + (float)height * 3 / 4;

	float hf = (float)height / 4;

	if (ShowMono){
		y0r = (float)y + (float)height / 2;
		hf *= 2;
	}

	c->SetColor(col);

	int di = DetailSteps;
	int nl = 0;
	for (int i=0;i<t->buffer.num;i++){
	if (f < MIN_MAX_FACTOR){
		nl = fill_line_buffer(width, di, pos, zoom, f, hf, x, y0r, t->buffer[i].r, t->buffer[i].offset, -1);
		c->DrawLines(tx, ty, nl);
		if (!ShowMono){
			nl = fill_line_buffer(width, di, pos, zoom, f, hf, x, y0l, t->buffer[i].l, t->buffer[i].offset, -1);
			c->DrawLines(tx, ty, nl);
		}
	}else{
		nl = fill_line_buffer(width, di, pos, zoom, f, hf, x, y0r, t->buffer[i].r, t->buffer[i].offset, -1);
		c->DrawLines(tx, ty, nl);
		nl = fill_line_buffer(width, di, pos, zoom, f, hf, x, y0r, t->buffer[i].r, t->buffer[i].offset, +1);
		c->DrawLines(tx, ty, nl);
		if (!ShowMono){
			nl = fill_line_buffer(width, di, pos, zoom, f, hf, x, y0l, t->buffer[i].l, t->buffer[i].offset, -1);
			c->DrawLines(tx, ty, nl);
			nl = fill_line_buffer(width, di, pos, zoom, f, hf, x, y0l, t->buffer[i].l, t->buffer[i].offset, +1);
			c->DrawLines(tx, ty, nl);
		}
	}
	}
	msg_db_l(1);
}

int GetStrWidth(const string &s)
{
	return 80;
}

void AudioView::DrawSubFrame(HuiDrawingContext *c, int x, int y, int width, int height, Track *s, AudioFile *a, const color &col, int delay)
{
	// frame
	int asx = a->sample2screen(s->pos + delay);
	int aex = a->sample2screen(s->pos + s->length + delay);
	clampi(asx, x, x + width);
	clampi(aex, x, x + width);

	if (delay == 0){
		s->x = asx;
		s->width = aex - asx;
		s->y = y;
		s->height = height;
	}


	color col2 = col;
	col2.a *= 0.2f;
	c->SetColor(col2);
	c->DrawRect(asx, y,                             aex - asx, SUB_FRAME_HEIGHT);
	c->DrawRect(asx, y + height - SUB_FRAME_HEIGHT, aex - asx, SUB_FRAME_HEIGHT);

	c->SetColor(col);
	c->DrawLine(asx, y, asx, y + height);
	c->DrawLine(aex, y, aex, y + height);
	c->DrawLine(asx, y, aex, y);
	c->DrawLine(asx, y + height, aex, y + height);
}

void AudioView::DrawSub(HuiDrawingContext *c, int x, int y, int width, int height, Track *s, AudioFile *a)
{
	color col = ColorSub;
	//bool is_cur = ((s == cur_sub) && (t->IsSelected));
	if (!s->is_selected)
		col = ColorSubNotCur;
	if (s->is_mouse_over)
		col = ColorSubMO;
	//col.a = 0.2f;

	DrawSubFrame(c, x, y, width, height, s, a, col, 0);

	color col2 = col;
	col2.a *= 0.5f;
	for (int i=0;i<s->rep_num;i++)
		DrawSubFrame(c, x, y, width, height, s, a, col2, (i + 1) * s->rep_delay);

	// buffer
	if (a != tsunami->cur_audio)
		col = ColorWave;
	DrawBuffer(	c, x, y, width, height,
				s, int(a->view_pos - s->pos), a->view_zoom, col);

	int asx = a->sample2screen(s->pos);
	clampi(asx, x, x + width);
	if (s->is_selected)//((is_cur) || (a->sub_mouse_over == s))
		//NixDrawStr(asx, y + height/2 - 10, s->name);
		c->DrawStr(asx, y + height - SUB_FRAME_HEIGHT, s->name);
}

void AudioView::DrawBarCollection(HuiDrawingContext *c, int x, int y, int width, int height, Track *t, color col, AudioFile *a, int track_no, BarCollection *bc)
{
	int x0 = bc->pos;
	foreachi(bc->bar, bar, i){
		bar.x     = a->sample2screen(x0);
		bar.width = a->sample2screen(x0 + bar.length) - bar.x;
		if ((bar.x >= x) && (bar.x < x + width)){
			c->SetColor(col);
			c->DrawStr(bar.x + 2, y + height/2, i2s(i + 1));
		}
		for (int i=0;i<bar.num_beats;i++){
			color cc = (i == 0) ? Red : col;
			c->SetColor(cc);

			int bx = a->sample2screen(x0 + (int)((float)bar.length * i / bar.num_beats));

			if ((bx >= x) && (bx < x + width))
				c->DrawLine(bx, y, bx, y + height);
		}
		x0 += bar.length;
	}

	if (bc->bar.num > 0){
		bc->x = bc->bar[0].x;
		bc->width = bc->bar.back().x + bc->bar.back().width - bc->x;
	}
}

void AudioView::DrawTrack(HuiDrawingContext *c, int x, int y, int width, int height, Track *t, color col, AudioFile *a, int track_no)
{
	msg_db_r("DrawTrack", 1);
	t->x = x;
	t->width = width;


	//c->SetColor((track_no == a->CurTrack) ? Black : ColorWaveCur);
	c->SetColor(ColorWaveCur);
	c->SetFont("", -1, ((tsunami->cur_audio == a) && (track_no == a->cur_track)), (t->type == Track::TYPE_TIME));
	c->DrawStr(x + 10, y + height / 2 - 10, t->GetNiceName());
	c->SetFont("", -1, false, false);

	DrawBuffer(	c, x,y,width,height,
				t,int(a->view_pos),a->view_zoom,col);

	foreach(t->bar_col, bc)
		DrawBarCollection(c, x, y, width, height, t, col, a, track_no, &bc);

	foreach(t->sub, s)
		DrawSub(c, x, y, width, height, &s, a);
	msg_db_l(1);
}

void AudioView::DrawGrid(HuiDrawingContext *c, int x, int y, int width, int height, AudioFile *a, const color &bg, bool show_time)
{
	if (!ShowGrid)
		return;
	float dl = MIN_GRID_DIST / a->view_zoom; // >= 10 pixel
	float dt = dl / a->sample_rate;
	float exp_s = ceil(log10(dt));
	float exp_s_mod = exp_s - log10(dt);
	dt = pow(10, exp_s);
	dl = dt * a->sample_rate;
//	float dw = dl * a->view_zoom;
	int nx0 = a->screen2sample(x - 1) / dl + 1;
	int nx1 = a->screen2sample(x + width) / dl + 1;
	color c1 = ColorInterpolate(bg, ColorGrid, exp_s_mod);
	color c2 = ColorGrid;
	for (int n=nx0;n<nx1;n++){
		c->SetColor(((n % 10) == 0) ? c2 : c1);
		c->DrawLine(a->sample2screen(n * dl), y, a->sample2screen(n * dl), y + height);
	}
	if (show_time){
		c->SetColor(ColorGrid);
		for (int n=nx0;n<nx1;n++){
			if ((a->sample2screen(dl) - a->sample2screen(0)) > 30){
				if ((((n % 10) % 3) == 0) && ((n % 10) != 9) && ((n % 10) != -9))
					c->DrawStr(a->sample2screen(n * dl) + 2, y, a->get_time_str_fuzzy(n * dl, dt * 3));
			}else{
				if ((n % 10) == 0)
					c->DrawStr(a->sample2screen(n * dl) + 2, y, a->get_time_str_fuzzy(n * dl, dt * 10));
			}
		}
	}
}

void AudioView::OnUpdate(Observable *o)
{
	msg_write("view: " + o->GetName() + " - " + o->GetMessage());
	if (o->GetMessage() == "New")
		OptimizeView(dynamic_cast<AudioFile*>(o));
	else{
		ForceRedraw();
		UpdateMenu();
	}
}

void AudioView::DrawWaveFile(HuiDrawingContext *c, int x, int y, int width, int height, AudioFile *a)
{
	a->x = x;
	a->y = y;
	a->width = width;
	a->height = height;


	foreachi(a->track, t, i){
		t.y = (int)((float)y + TIME_SCALE_HEIGHT + (float)(height - TIME_SCALE_HEIGHT) / (float)a->track.num * i);
		t.height = (int)((float)y + TIME_SCALE_HEIGHT + (float)(height - TIME_SCALE_HEIGHT) / (float)a->track.num * (i + 1)) - t.y;
	}

	// background
	//int trackheight = (a->num_tracks > 0) ? (height / a->num_tracks) : height;
	if ((a == tsunami->cur_audio) && (a->used)){
		c->SetColor(ColorBackgroundCurWave);
		c->DrawRect(x, y, width, TIME_SCALE_HEIGHT);
		DrawGrid(c, x, y, width, TIME_SCALE_HEIGHT, a, ColorBackgroundCurWave, true);
		foreach(a->track, t){
			c->SetColor((t.is_selected) ? ColorBackgroundCurTrack : ColorBackgroundCurWave);
			c->DrawRect(x, t.y, width, t.height);
			DrawGrid(c, x, t.y, width, t.height, a, (t.is_selected) ? ColorBackgroundCurTrack : ColorBackgroundCurWave);
		}
	}else{
		color col = (a == tsunami->cur_audio) ? ColorBackgroundCurWave : ColorBackground;
		c->SetColor(col);
		c->DrawRect(x, y, width, height);
		if (a->used)
			DrawGrid(c, x, y, width, height, a, col, true);
	}

	if (!a->used){
		c->SetColor((a == tsunami->cur_audio) ? ColorWaveCur : ColorWave);
		c->SetFontSize(12);
		c->DrawStr(x + width / 2 - 50, y + height / 2 - 10, _("keine Datei"));
		return;
	}


	// selection
	if (a->selection){
		int sx1 = a->sample2screen(a->sel_start_raw);
		int sx2 = a->sample2screen(a->sel_end_raw);
		int sxx1 = sx1, sxx2 = sx2;
		clampi(sxx1, x, width + x);
		clampi(sxx2, x, width + x);
		bool mo_s = a->mo_sel_start;
		bool mo_e = a->mo_sel_end;
		if (sxx1 > sxx2){
			int t = sxx1;	sxx1 = sxx2;	sxx2 = t;
			//bool bt = mo_s;	mo_s = mo_e;	mo_e = bt; // TODO ???
		}
		foreach(a->track, t)
			if (t.is_selected){
				c->SetColor(ColorSelectionInternal);
				c->DrawRect(sxx1, t.y, sxx2 - sxx1, t.height);
				DrawGrid(c, sxx1, t.y, sxx2 - sxx1, t.height, a, ColorSelectionInternal);
			}
		if ((sx1>=x)&&(sx1<=x+width)){
			color col = mo_s ? ColorSelectionBoundaryMO : ColorSelectionBoundary;
			c->SetColor(col);
			c->DrawLine(sx1, y, sx1, y + height);
			//NixDrawStr(sx1,y,get_time_str(a->SelectionStart,a));
		}
		if ((sx2>=x)&&(sx2<=x+width)){
			color col = mo_e ? ColorSelectionBoundaryMO : ColorSelectionBoundary;
			c->SetColor(col);
			c->DrawLine(sx2, y, sx2, y + height);
			//NixDrawStr(sx2,y+height-TIME_SCALE_HEIGHT,get_time_str(a->SelectionEnd-a->SelectionStart,a));
		}
	}

	//NixDrawStr(x,y,get_time_str((int)a->ViewPos,a));
	// playing position
	if (tsunami->output->IsPlaying()){
		int pos = tsunami->output->GetPos(a);
		int px = a->sample2screen(pos);
		c->SetColor(ColorPreviewMarker);
		c->DrawLine(px, y, px, y + height);
		c->DrawStr(px, y + height / 2, a->get_time_str(pos));
	}

	color col = (a==tsunami->cur_audio) ? ColorWaveCur : ColorWave;
	c->SetColor(col);

	// boundary of wave file
	/*if (a->min != a->max){
		c->DrawLine(sample2screen(a, a->min), y, sample2screen(a, a->min), y + height);
		c->DrawLine(sample2screen(a, a->max), y, sample2screen(a, a->max), y + height);
	}*/


	foreachi(a->track, tt, i)
		DrawTrack(c, x, tt.y, width, tt.height, &tt, col, a, i);

}

int frame=0;

void AudioView::OnDraw()
{
	msg_db_r("OnDraw", 1);
	force_redraw = false;

	HuiDrawingContext *c = tsunami->BeginDraw("area");
	DrawingWidth = c->width;
	c->SetFontSize(10);
	c->SetLineWidth(1.0f);
	c->SetAntialiasing(false);
	//c->SetColor(ColorWaveCur);

	int t0 = max(tsunami->audio[0]->track.num, 1);
	int t1 = max(tsunami->audio[1]->track.num, 1);
	float t = (float)t0 / (float)(t0 + t1);

	if (ShowTempFile){
		DrawWaveFile(c, 0, 0, c->width, c->height * t, tsunami->audio[0]);
		DrawWaveFile(c, 0, c->height * t, c->width, c->height * (1 - t), tsunami->audio[1]);
	}else
		DrawWaveFile(c, 0, 0, c->width, c->height, tsunami->audio[0]);

	//c->DrawStr(100, 100, i2s(frame++));

	c->End();

	msg_db_l(1);
}

void AudioView::OptimizeView(AudioFile *a)
{
	msg_db_r("OptimizeView", 1);
	if (a->width <= 0)
		a->width = DrawingWidth;

	int length = a->GetLength();
	if (length == 0)
		length = 10 * a->sample_rate;
	a->view_zoom = (float)a->width / (float)length;
	a->view_pos = (float)a->GetMin();
	ForceRedraw();
	msg_db_l(1);
}

void AudioView::UpdateMenu()
{
	// edit
	tsunami->Enable("select_all", tsunami->cur_audio->used);
	tsunami->Enable("select_nothing", tsunami->cur_audio->used);
	// view
	tsunami->Check("view_temp_file", ShowTempFile);
	tsunami->Check("view_mono", ShowMono);
	tsunami->Check("view_grid", ShowGrid);
	tsunami->Enable("zoom_in", tsunami->cur_audio->used);
	tsunami->Enable("zoom_out", tsunami->cur_audio->used);
	tsunami->Enable("view_optimal", tsunami->cur_audio->used);
	tsunami->Enable("view_samples", false);//tsunami->cur_audio->used);
}

void AudioView::OnViewTempFile()
{
	ShowTempFile = !ShowTempFile;
	ForceRedraw();
	UpdateMenu();
}

void AudioView::OnViewOptimal()
{
	OptimizeView(tsunami->cur_audio);
}

void AudioView::OnSelectNone()
{
}

void AudioView::OnViewMono()
{
	ShowMono = !ShowMono;
	ForceRedraw();
	UpdateMenu();
}

void AudioView::OnZoomIn()
{
}

void AudioView::OnSelectAll()
{
}

void AudioView::SelectNone(AudioFile *a)
{
	// select all/none
	a->selection = false;
	a->UpdateSelection();
	a->UnselectAllSubs();
	SetCurSub(a, NULL);
}

void AudioView::OnZoomOut()
{
}

void AudioView::OnJumpOtherFile()
{
}

void AudioView::OnViewGrid()
{
	ShowGrid= !ShowGrid;
	ForceRedraw();
	UpdateMenu();
}

void AudioView::OnSelectNothing()
{
}



void AudioView::SelectSub(Track *s, bool diff)
{
	if (!s)
		return;
	if (diff){
		s->is_selected = !s->is_selected;
	}else{
		if (!s->is_selected)
			s->root->UnselectAllSubs();

		// select this sub
		s->is_selected = true;
	}
}

void AudioView::SelectTrack(Track *t, bool diff)
{
	if (!t)
		return;
	if (diff){
		bool is_only_selected = true;
		foreach(t->root->track, tt)
			if ((tt.is_selected) && (&tt != t))
				is_only_selected = false;
		t->is_selected = !t->is_selected || is_only_selected;
	}else{
		if (!t->is_selected){
			// unselect all tracks
			foreach(t->root->track, tt)
				tt.is_selected = false;
		}

		// select this track
		t->is_selected = true;
	}
	t->root->UpdateSelection();
}

void AudioView::SetCurSub(AudioFile *a, Track *s)
{
	msg_db_r("SetCurSub", 2);
	// unset
	foreach(a->track, t)
		t.cur_sub = -1;

	if (s){
		// set
		Track *t = s->GetParent();
		if (t)
			t->cur_sub = get_sub_index(s);
	}
	msg_db_l(2);
}


void AudioView::SetCurTrack(AudioFile *a, Track *t)
{
	a->cur_track = get_track_index(t);
}

void AudioView::SetCurAudioFile(AudioFile *a)
{
	tsunami->cur_audio = a;
	ForceRedraw();
}


void AudioView::ZoomAudioFile(AudioFile *a, float f)
{
	// max zoom: 8 pixel per sample
	// min zoom: whole file on 100 pixel
	int length = a->GetLength();
	if (length == 0)
		length = 10 * a->sample_rate;
	clampf(f, 100.0 / (length * a->view_zoom), 8.0f / a->view_zoom);
	a->view_zoom *= f;
	a->view_pos += float(mx - a->x) / (a->view_zoom / (f - 1));
	ForceRedraw();
}

void AudioView::MoveView(AudioFile *a, float dpos)
{
	a->view_pos += dpos;
	ForceRedraw();
}

void AudioView::ExecuteTrackDialog(CHuiWindow *win, Track *t)
{
	if (!t){
		tsunami->log->Error(_("Keine Spur ausgew&ahlt"));
		return;
	}
	TrackDialog *dlg = new TrackDialog(win, false, t);
	dlg->Update();
	HuiWaitTillWindowClosed(dlg);
}



void AudioView::ExecuteSubDialog(CHuiWindow *win, Track *s)
{
	if (!s){
		tsunami->log->Error(_("Keine Ebene ausgew&ahlt"));
		return;
	}
	SubDialog *dlg = new SubDialog(win, false, s);
	dlg->Update();
	HuiWaitTillWindowClosed(dlg);
}



void AudioView::ExecuteAudioDialog(CHuiWindow *win, AudioFile *a)
{
	if (!a->used){
		tsunami->log->Error(_("Audio-Datei ist leer"));
		return;
	}

	AudioFileDialog *dlg = new AudioFileDialog(win, false, a);
	dlg->Update();
	HuiWaitTillWindowClosed(dlg);
}


