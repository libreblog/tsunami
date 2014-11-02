/*
 * AudioView.h
 *
 *  Created on: 24.03.2012
 *      Author: michi
 */

#ifndef AUDIOVIEW_H_
#define AUDIOVIEW_H_

#include "../lib/hui/hui.h"
#include "../Data/AudioFile.h"
#include "../Stuff/Observer.h"

class ActionTrackMoveSample;
class AudioOutput;
class AudioStream;
class AudioInput;
class AudioRenderer;
class TsunamiWindow;
class AudioViewTrack;
class SynthesizerRenderer;

class AudioView : public Observer, public Observable
{
public:
	AudioView(TsunamiWindow *parent, AudioFile *audio, AudioOutput *output, AudioInput *input, AudioRenderer *renderer);
	virtual ~AudioView();

	void checkConsistency();
	void forceRedraw();

	void onDraw();
	void onMouseMove();
	void onLeftButtonDown();
	void onLeftButtonUp();
	void onMiddleButtonDown();
	void onMiddleButtonUp();
	void onRightButtonDown();
	void onRightButtonUp();
	void onLeftDoubleClick();
	void onMouseWheel();
	void onKeyDown();
	void onKeyUp();
	void onCommand(const string &id);

	void onUpdate(Observable *o, const string &message);
	static const string MESSAGE_CUR_TRACK_CHANGE;
	static const string MESSAGE_CUR_SAMPLE_CHANGE;
	static const string MESSAGE_CUR_LEVEL_CHANGE;
	static const string MESSAGE_SELECTION_CHANGE;
	static const string MESSAGE_SETTINGS_CHANGE;
	static const string MESSAGE_VIEW_CHANGE;

	void setShowMono(bool mono);
	void setPeaksMode(int mode);
	void zoomIn();
	void zoomOut();
	void makeSampleVisible(int sample);

	void drawGridTime(HuiPainter *c, const rect &r, const color &bg, bool show_time = false);
	void drawGridBars(HuiPainter *c, const rect &r, const color &bg, bool show_time = false);
	void drawTimeLine(HuiPainter *c, int pos, int type, color &col, bool show_time = false);
	void drawSelection(HuiPainter *c, const rect &r);
	void drawBackground(HuiPainter *c, const rect &r);
	void drawAudioFile(HuiPainter *c, const rect &r);

	void optimizeView();
	void updateMenu();


	color ColorBackground;
	color ColorBackgroundCurWave;
	color ColorBackgroundCurTrack;
	color ColorGrid;
	color ColorSelectionInternal;
	color ColorSelectionBoundary;
	color ColorSelectionBoundaryMO;
	color ColorPreviewMarker;
	color ColorCaptureMarker;
	color ColorWave;
	color ColorWaveCur;
	color ColorSub;
	color ColorSubMO;
	color ColorSubNotCur;
	static const int SUB_FRAME_HEIGHT;
	static const int TIME_SCALE_HEIGHT;
	static const float LINE_WIDTH;
	static const int FONT_SIZE;
	static const int MAX_TRACK_CHANNEL_HEIGHT;

	enum
	{
		SEL_TYPE_NONE,
		SEL_TYPE_SELECTION_START,
		SEL_TYPE_SELECTION_END,
		SEL_TYPE_PLAYBACK,
		SEL_TYPE_TIME,
		SEL_TYPE_TRACK,
		SEL_TYPE_MUTE,
		SEL_TYPE_SOLO,
		SEL_TYPE_SAMPLE,
		SEL_TYPE_MIDI_NOTE,
		SEL_TYPE_MIDI_PITCH,
		SEL_TYPE_BOTTOM_BUTTON,
	};

	struct SelectionType
	{
		int type;
		AudioViewTrack *vtrack;
		Track *track;
		SampleRef *sample;
		int pos;
		int sample_offset;
		Array<int> barrier;
		Track *show_track_controls;
		int pitch, note;

		SelectionType();
	};

	SelectionType hover, selection;
	Range sel_range;
	Range sel_raw;

	ActionTrackMoveSample *cur_action;

	int mouse_possibly_selecting, mouse_possibly_selecting_start;
	const int BarrierDist;
	float ScrollSpeed;
	float ScrollSpeedFast;
	float ZoomSpeed;

	int mx,my;
	int mx0,my0;

	void selectNone();
	void selectAll();
	void updateSelection();
	Range getPlaybackSelection();

	void setMouse();
	bool mouseOverTrack(AudioViewTrack *t);
	int mouseOverSample(SampleRef *s);
	void selectionUpdatePos(SelectionType &s);
	SelectionType getMouseOver();
	void selectUnderMouse();
	void setBarriers(SelectionType *s);
	void applyBarriers(int &pos);
	void setCursorPos(int pos);

	void selectSample(SampleRef *s, bool diff);
	void selectTrack(Track *t, bool diff);

	double screen2sample(double x);
	double sample2screen(double s);
	double dsample2screen(double ds);
	int y2pitch(int y);
	float pitch2y(int p);

	Array<MidiNote> getSelectedNotes();

	void zoom(float f);
	void move(float dpos);


	bool force_redraw;

	bool show_mono;
	int detail_steps;
	int mouse_min_move_to_select;
	int preview_sleep_time;
	bool antialiasing;

	int peak_mode;

	rect drawing_rect;
	rect bottom_button_rect;

	bool editingMidi();
	int pitch_min, pitch_max;
	int beat_partition;
	int midi_mode;
	int midi_scale;
	int chord_type;
	int chord_inversion;
	bool is_sharp(int pitch);

	enum
	{
		MIDI_MODE_SELECT,
		MIDI_MODE_NOTE,
		MIDI_MODE_CHORD
	};

	TsunamiWindow *win;

	AudioFile *audio;

	AudioStream *stream;
	AudioStream *midi_preview_stream;
	SynthesizerRenderer *midi_preview_renderer;
	AudioInput *input;
	AudioRenderer *renderer;

	void setCurSample(SampleRef *s);
	void setCurTrack(Track *t);
	void setCurLevel(int l);
	Track *cur_track;
	SampleRef *cur_sample;
	int cur_level;

	double view_pos;
	double view_zoom;

	rect area;
	struct TrackHeightManager
	{
		float t;
		bool dirty;
		bool animating;
		rect render_area;
		HuiTimer timer;
		Track *midi_track;

		bool check(AudioFile *a);
		void update(AudioView *v, AudioFile *a, const rect &r);
		void plan(AudioView *v, AudioFile *a, const rect &r);
	};
	TrackHeightManager thm;

	Array<AudioViewTrack*> vtrack;
	void updateTracks();

	int prefered_buffer_level;
	double buffer_zoom_factor;
	void updateBufferZoom();

	Image image_muted, image_unmuted, image_solo;
	Image image_track_audio, image_track_time, image_track_midi;

	HuiMenu *menu_track;
	HuiMenu *menu_sub;
	HuiMenu *menu_audio;
};

#endif /* AUDIOVIEW_H_ */
