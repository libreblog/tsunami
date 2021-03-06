/*
 * Song.h
 *
 *  Created on: 22.03.2012
 *      Author: michi
 */

#ifndef SRC_DATA_SONG_H_
#define SRC_DATA_SONG_H_

#include "Data.h"
#include "Curve.h"
#include "../lib/base/base.h"
#include "Rhythm/BarCollection.h"
#include <shared_mutex>

class Data;
class AudioEffect;
class MidiEffect;
class Track;
class TrackLayer;
class Sample;
class Synthesizer;
class Curve;
class SongSelection;
class AudioBuffer;
class BarPattern;
class TrackMarker;
class MidiNoteBuffer;
enum class SampleFormat;
enum class SignalType;

class Tag {
public:
	string key, value;
	Tag(){}
	Tag(const string &_key, const string &_value) {
		key = _key;
		value = _value;
	}
};

class Song : public Data {
public:
	Song(Session *session, int sample_rate);
	virtual ~Song();

	void _cdecl __init__(Session *session, int sample_rate);
	void _cdecl __delete__() override;

	Range _cdecl range();
	Range _cdecl range_with_time();

	static const string MESSAGE_NEW;
	static const string MESSAGE_ADD_TRACK;
	static const string MESSAGE_DELETE_TRACK;
	static const string MESSAGE_ADD_EFFECT;
	static const string MESSAGE_DELETE_EFFECT;
	static const string MESSAGE_ADD_CURVE;
	static const string MESSAGE_DELETE_CURVE;
	static const string MESSAGE_EDIT_CURVE;
	static const string MESSAGE_ADD_SAMPLE;
	static const string MESSAGE_DELETE_SAMPLE;
	static const string MESSAGE_ADD_LAYER;
	static const string MESSAGE_EDIT_LAYER;
	static const string MESSAGE_DELETE_LAYER;
	static const string MESSAGE_CHANGE_CHANNELS;
	static const string MESSAGE_EDIT_BARS;
	static const string MESSAGE_ENABLE_FX;



	class Error : public Exception {
	public:
		explicit Error(const string &message);
	};


	string _cdecl get_time_str(int t);
	string _cdecl get_time_str_fuzzy(int t, float dt);
	string _cdecl get_time_str_long(int t);

	void _cdecl reset() override;
	bool is_empty();

	void _cdecl invalidate_all_peaks();

	Track *_cdecl time_track();
	int _cdecl bar_offset(int index);

	string _cdecl get_tag(const string &key);

	Array<TrackMarker*> get_parts();

	// action
	void _cdecl add_tag(const string &key, const string &value);
	void _cdecl edit_tag(int index, const string &key, const string &value);
	void _cdecl delete_tag(int index);
	void _cdecl change_all_track_volumes(Track *t, float volume);
	void _cdecl set_sample_rate(int sample_rate);
	void _cdecl set_default_format(SampleFormat format);
	void _cdecl set_compression(int compression);
	Track *_cdecl add_track(SignalType type, int index = -1);
	Track *_cdecl add_track_after(SignalType type, Track *insert_after = nullptr);
	void _cdecl delete_track(Track *track);
	Sample *_cdecl create_sample_audio(const string &name, const AudioBuffer &buf);
	Sample *_cdecl create_sample_midi(const string &name, const MidiNoteBuffer &midi);
	void _cdecl add_sample(Sample *s);
	void _cdecl delete_sample(Sample *s);
	void _cdecl edit_sample_name(Sample *s, const string &name);
	void _cdecl sample_replace_buffer(Sample *s, AudioBuffer *buf);
	void _cdecl add_bar(int index, const BarPattern &bar, int mode);
	void _cdecl add_pause(int index, int length, int mode);
	void _cdecl edit_bar(int index, const BarPattern &bar, int mode);
	void _cdecl delete_bar(int index, bool affect_midi);
	void _cdecl delete_time_interval(int index, const Range &range);
	void _cdecl insert_selected_samples(const SongSelection &sel);
	void _cdecl delete_selected_samples(const SongSelection &sel);
	void _cdecl delete_selection(const SongSelection &sel);
	void _cdecl create_samples_from_selection(const SongSelection &sel, bool auto_delete);
	Curve *_cdecl add_curve(const string &name, Array<Curve::Target> &targets);
	void _cdecl delete_curve(Curve *curve);
	void _cdecl edit_curve(Curve *curve, const string &name, float min, float max);
	void _cdecl curve_set_targets(Curve *curve, Array<Curve::Target> &targets);
	void _cdecl curve_add_point(Curve *curve, int pos, float value);
	void _cdecl curve_delete_point(Curve *curve, int index);
	void _cdecl curve_edit_point(Curve *curve, int index, int pos, float value);

	// helper
	Sample* _cdecl get_sample_by_uid(int uid);

// data
	Array<Tag> tags;
	int sample_rate;
	SampleFormat default_format;
	int compression;

	Array<AudioEffect*> __fx;
	Array<Track*> tracks;
	Array<Sample*> samples;
	Array<Curve*> curves;
	BarCollection bars;

	Array<TrackLayer*> layers() const;
};



int get_track_index(Track *t);


#endif /* SRC_DATA_SONG_H_ */
