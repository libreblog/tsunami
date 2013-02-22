/*
 * AudioInputMidi.h
 *
 *  Created on: 19.02.2013
 *      Author: michi
 */

#ifndef AUDIOINPUTMIDI_H_
#define AUDIOINPUTMIDI_H_

#include "../lib/base/base.h"
#include "../lib/hui/hui.h"
#include "../Data/AudioFile.h"
#include "AudioInputBase.h"

struct _snd_seq;

class AudioInputMidi : public AudioInputBase
{
public:
	AudioInputMidi(MidiData &data);
	virtual ~AudioInputMidi();

	void Init();

	virtual bool Start(int sample_rate);
	virtual void Stop();
	virtual int DoCapturing();

	virtual bool IsCapturing();

	virtual int GetDelay();
	virtual void ResetSync();

	virtual float GetSampleRate();
	virtual BufferBox GetSomeSamples(int num_samples);

private:

	void ClearInput();

	MidiData &data;

	_snd_seq *handle;
	int npfd;
	struct pollfd *pfd;
	int sample_rate;
	int timer;
	double offset;
	bool capturing;

	int tone_start[128];
	float tone_volume[128];
};

#endif /* AUDIOINPUTMIDI_H_ */