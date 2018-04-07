/*
 * AudioJoiner.h
 *
 *  Created on: 02.04.2018
 *      Author: michi
 */

#ifndef SRC_MODULE_AUDIO_AUDIOJOINER_H_
#define SRC_MODULE_AUDIO_AUDIOJOINER_H_

#include "../Port/AudioPort.h"
#include "../Module.h"

class Session;

class AudioJoiner : public Module
{
public:
	AudioJoiner(Session *session);
	virtual ~AudioJoiner();

	class Output : public AudioPort
	{
	public:
		Output(AudioJoiner *j);
		virtual ~Output(){}
		virtual int _cdecl read(AudioBuffer &buf);
		virtual void _cdecl reset();
		virtual int _cdecl get_pos(int delta);
		AudioJoiner *joiner;
	};
	Output *out;

	AudioPort *a, *b;
	void _cdecl set_source_a(AudioPort *a);
	void _cdecl set_source_b(AudioPort *b);
};

#endif /* SRC_MODULE_AUDIO_AUDIOJOINER_H_ */