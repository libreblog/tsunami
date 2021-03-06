/*
 * PeakMeter.cpp
 *
 *  Created on: 01.04.2018
 *      Author: michi
 */

#include "PeakMeter.h"

#include "../../Data/Audio/AudioBuffer.h"
#include "../../Data/Audio/RingBuffer.h"
#include "../../Plugins/FastFourierTransform.h"
#include "../../Session.h"
#include "../../lib/hui/hui.h"


const int PeakMeter::SPECTRUM_SIZE = 30;
const float PeakMeter::FREQ_MIN = 40.0f;
const float PeakMeter::FREQ_MAX = 4000.0f;

void PeakMeterData::reset() {
	peak = 0;
	super_peak = super_peak_t = 0;
	spec.clear();
	spec.resize(PeakMeter::SPECTRUM_SIZE);
}

float PeakMeterData::get_sp() {
	return max(super_peak * (1 - (float)pow(super_peak_t, 3)*0.2f), 0.0001f);
}

void PeakMeterData::update(Array<float> &buf, float dt) {
	peak = 0;
	for (int i=0; i<buf.num; i++) {
		if (fabs(buf[i]) > peak)
			peak = fabs(buf[i]);
	}
	if (peak > get_sp()) {
		super_peak = peak;
		super_peak_t = 0;
	} else {
		super_peak_t += dt;
	}
}

PeakMeter::PeakMeter() {
	module_subtype = "PeakMeter";
	spectrum_requests = 0;
	l.reset();
	r.reset();
}

PeakMeter::~PeakMeter() {
}

inline float nice_peak(float p) {
	return min((float)pow(p, 0.8f), 1.0f);
}

void PeakMeter::find_peaks(AudioBuffer &buf) {
	float dt = (float)buf.length / (float)session->sample_rate();
	l.update(buf.c[0], dt);
	r.update(buf.c[1], dt);
}

void PeakMeter::clear_data() {
	l.reset();
	r.reset();
}

inline float PeakMeter::i_to_freq(int i) {
	return FREQ_MIN * exp( (float)i / (float)SPECTRUM_SIZE * log(FREQ_MAX / FREQ_MIN));
}

void PeakMeter::request_spectrum() {
	spectrum_requests ++;
}

void PeakMeter::unrequest_spectrum() {
	spectrum_requests --;
}

void PeakMeter::find_spectrum(AudioBuffer &buf) {
	Array<complex> cr, cl;
	cr.resize(buf.length / 2 + 1);
	cl.resize(buf.length / 2 + 1);
	FastFourierTransform::fft_r2c(buf.c[0], cl);
	FastFourierTransform::fft_r2c(buf.c[1], cr);
	r.spec.resize(SPECTRUM_SIZE);
	l.spec.resize(SPECTRUM_SIZE);
	float sample_rate = (float)session->sample_rate();
	for (int i=0;i<SPECTRUM_SIZE;i++) {
		float f0 = i_to_freq(i);
		float f1 = i_to_freq(i + 1);
		int n0 = f0 * buf.length / sample_rate;
		int n1 = max((int)(f1 * buf.length / sample_rate), n0 + 1);
		float s = 0;
		for (int n=n0;n<n1;n++)
			if (n < cr.num) {
				s = max(s, cr[n].abs_sqr());
				s = max(s, cl[n].abs_sqr());
			}
		r.spec[i] = l.spec[i] = sqrt(sqrt(s) / (float)SPECTRUM_SIZE / pi / 2);
	}
}

void PeakMeter::process(AudioBuffer& buf) {
	clear_data();
	find_peaks(buf);
	if (spectrum_requests > 0)
		find_spectrum(buf);
	notify();
}

void PeakMeter::reset_state() {
	clear_data();
	notify();
}
