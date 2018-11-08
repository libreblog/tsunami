/*
 * AudioEffect.cpp
 *
 *  Created on: 10.12.2012
 *      Author: michi
 */

#include "AudioEffect.h"
#include "../ModuleFactory.h"
#include "../../Session.h"
#include "../../lib/math/math.h"
#include "../../Action/Track/Buffer/ActionTrackEditBuffer.h"
#include "../../Data/base.h"
#include "../../Data/Song.h"
#include "../../Data/TrackLayer.h"


AudioEffect::Output::Output(AudioEffect *_fx) : AudioPort("out")
{
	fx = _fx;
}

int AudioEffect::Output::read(AudioBuffer &buf)
{
	if (!fx->source)
		return buf.length;
	fx->sample_rate = fx->session->sample_rate();
	int samples = fx->source->read(buf);
	if (samples > 0)
		fx->process(buf);
	return samples;
}

void AudioEffect::Output::reset()
{
	fx->reset_state();
	if (fx->source)
		fx->source->reset();
}

int AudioEffect::Output::get_pos(int delta)
{
	if (!fx->source)
		return -1;
	return fx->source->get_pos(delta);
}

AudioEffect::AudioEffect() :
	Module(ModuleType::AUDIO_EFFECT)
{
	source = nullptr;
	out = new Output(this);
	port_out.add(out);
	port_in.add(InPortDescription(SignalType::AUDIO, (Port**)&source, "in"));
	sample_rate = DEFAULT_SAMPLE_RATE;
}

void AudioEffect::__init__()
{
	new(this) AudioEffect;
}

void AudioEffect::__delete__()
{
	this->AudioEffect::~AudioEffect();
}

void AudioEffect::do_process_track(TrackLayer *l, const Range &r)
{
	sample_rate = l->song()->sample_rate;
	AudioBuffer buf;
	l->get_buffers(buf, r);
	ActionTrackEditBuffer *a = new ActionTrackEditBuffer(l, r);
	process(buf);
	session->song->execute(a);
}


AudioEffect *CreateAudioEffect(Session *session, const string &name)
{
	return (AudioEffect*)ModuleFactory::create(session, ModuleType::AUDIO_EFFECT, name);
}
