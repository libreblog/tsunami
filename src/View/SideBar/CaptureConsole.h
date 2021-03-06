/*
 * CaptureDialog.h
 *
 *  Created on: 27.03.2012
 *      Author: michi
 */

#ifndef SRC_VIEW_SIDEBAR_CAPTURECONSOLE_H_
#define SRC_VIEW_SIDEBAR_CAPTURECONSOLE_H_


#include "SideBar.h"
#include "../../Data/Song.h"
#include "../Helper/PeakMeterDisplay.h"

class CaptureConsoleMode;
class Session;

class AudioInput;
class AudioSucker;
class SignalChain;
class MidiEventBuffer;

class CaptureConsole : public SideBarConsole {
public:
	CaptureConsole(Session *session);
	virtual ~CaptureConsole();


	void on_enter() override;
	void on_leave() override;

	bool allow_close() override;

	void on_start();
	void on_dump();
	void on_pause();
	void on_ok();
	void on_cancel();
	void on_new_version();

	void update_time();

	void on_putput_tick();
	void on_output_end_of_stream();

	bool has_data();

	enum class State {
		EMPTY,
		CAPTURING,
		PAUSED
	} state;

	PeakMeterDisplay *peak_meter;
	CaptureConsoleMode *mode;

	CaptureConsoleMode *mode_audio;
	CaptureConsoleMode *mode_midi;
	CaptureConsoleMode *mode_multi;
	SignalChain *chain;
	int n_sync = 0;


	bool insert_audio(Track *target, AudioBuffer &buf, int delay);
	bool insert_midi(Track *target, const MidiEventBuffer &midi, int delay);
};

#endif /* SRC_VIEW_SIDEBAR_CAPTURECONSOLE_H_ */
