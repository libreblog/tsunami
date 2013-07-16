/*
 * AudioOutput.h
 *
 *  Created on: 26.03.2012
 *      Author: michi
 */

#ifndef AUDIOOUTPUT_H_
#define AUDIOOUTPUT_H_

#include "../lib/base/base.h"
#include "../lib/hui/hui.h"
#include "../Data/AudioFile.h"
#include "../View/Helper/PeakMeter.h"

struct ALCdevice_struct;
struct ALCcontext_struct;

class AudioOutput : public HuiEventHandler, public PeakMeterSource
{
public:
	AudioOutput();
	virtual ~AudioOutput();

	Array<string> Device;
	string ChosenDevice;
	void SetDevice(const string &device);

	void Init();
	void Kill();

	void Stop();
	void Play(AudioFile *a, bool allow_loop);
	void PlayGenerated(void *func, int sample_rate);
	void Pause();
	void Update();

	bool IsPlaying();
	bool IsPaused();
	AudioFile *GetAudio(){	return audio;	}
	Range GetRange(){	return range;	}
	int GetPos();
	void SetRangeStart(int pos);
	void SetRangeEnd(int pos);
	void Seek(int pos);

	float GetSampleRate();
	BufferBox GetSomeSamples(int num_samples);

	float GetVolume();
	void SetVolume(float _volume);

	bool GetLoop(){	return loop;	}
	void SetLoop(bool _loop){	loop = _loop;	}

	void SetBufferSize(int _size){	buffer_size = _size;	}

private:
	bool TestError(const string &msg);
	bool stream(int buf);

	void stop_play();
	void start_play(int pos);


	float volume;
	bool playing;
	bool allow_loop;
	bool loop;
	int sample_rate;
	int buffer_size;

	AudioFile *audio;
	Range range;
	int stream_offset_next;
	BufferBox box[2];

	typedef void generate_func_t(BufferBox &);
	generate_func_t *generate_func;

	Array<short> data;
	int data_samples;

	ALCcontext_struct *al_context;
	ALCdevice_struct *al_dev;
	int buffer[2];
	int cur_buffer_no;
	unsigned int source;

	bool al_initialized;
	int al_last_error;
};

#endif /* AUDIOOUTPUT_H_ */
