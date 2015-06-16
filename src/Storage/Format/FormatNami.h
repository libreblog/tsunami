/*
 * FormatNami.h
 *
 *  Created on: 24.03.2012
 *      Author: michi
 */

#ifndef FORMATNAMI_H_
#define FORMATNAMI_H_

#include "Format.h"

class FormatNami : public Format
{
public:
	FormatNami();
	virtual ~FormatNami();

	void loadTrack(StorageOperationData *od);
	void saveBuffer(StorageOperationData *od);

	void loadAudio(StorageOperationData *od);
	void saveAudio(StorageOperationData *od);

	string compress_buffer(BufferBox &b);

	Array<int> ChunkPos;
	void make_consistent(AudioFile *a);

	void BeginChunk(const string &name);
	void EndChunk();
	void WriteTag(Tag *t);
	void WriteEffect(Effect *e);
	void WriteBufferBox(BufferBox *b);
	void WriteSample(Sample *s);
	void WriteSampleRef(SampleRef *s);
	void WriteBar(BarPattern &b);
	void WriteMarker(TrackMarker &m);
	void WriteMidiEvent(MidiEvent &e);
	void WriteMidiEffect(MidiEffect *e);
	void WriteMidi(MidiData &m);
	void WriteSynth(Synthesizer *s);
	void WriteTrackLevel(TrackLevel *l, int level_no);
	void WriteTrack(Track *t);
	void WriteLevelName();
	void WriteFormat();

	File *f;
	AudioFile *audio;
};

#endif /* FORMATNAMI_H_ */