/*
 * AudioSucker.h
 *
 *  Created on: 17.09.2017
 *      Author: michi
 */

#ifndef SRC_AUDIO_AUDIOSUCKER_H_
#define SRC_AUDIO_AUDIOSUCKER_H_

#include "AudioBuffer.h"
#include "../Stuff/Observable.h"

class AudioPort;
class AudioSuckerThread;

class AudioSucker : public Observable<VirtualBase>
{
	friend class AudioSuckerThread;
public:
	AudioSucker(AudioPort *source);
	virtual ~AudioSucker();

	void setSource(AudioPort *s);
	void accumulate(bool enable);
	void resetAccumulation();
	void setBufferSize(int size);

	void start();
	void stop();

	int update();
	void wait();

	static const int DEFAULT_BUFFER_SIZE;
	static const string MESSAGE_UPDATE;

	AudioPort *source;
	AudioBuffer buf;
	bool accumulating;
	bool running;
	int buffer_size;
	float no_data_wait;

	int perf_channel;

	AudioSuckerThread *thread;
};

#endif /* SRC_AUDIO_AUDIOSUCKER_H_ */
