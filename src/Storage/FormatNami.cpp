/*
 * FormatNami.cpp
 *
 *  Created on: 24.03.2012
 *      Author: michi
 */

#include "FormatNami.h"
#include "../Tsunami.h"
#include "../Plugins/Effect.h"
#include "../Stuff/Log.h"
#include "../View/Helper/Progress.h"
#include "../Audio/Synth/Synthesizer.h"


const int CHUNK_SIZE = 1 << 16;


FormatNami::FormatNami() :
	Format("nami", FLAG_AUDIO | FLAG_MIDI | FLAG_FX | FLAG_MULTITRACK | FLAG_TAGS | FLAG_SUBS)
{
}

FormatNami::~FormatNami()
{
}


Array<int> ChunkPos;

void BeginChunk(CFile *f, const string &name)
{
	string s = name + "        ";
	f->WriteBuffer(s.data, 8);
	f->WriteInt(0); // temporary
	ChunkPos.add(f->GetPos());
}

void EndChunk(CFile *f)
{
	int pos = ChunkPos.back();
	ChunkPos.pop();

	int pos0 = f->GetPos();
	f->SetPos(pos - 4, true);
	f->WriteInt(pos0 - pos);
	f->SetPos(pos0, true);
}

void WriteTag(CFile *f, Tag *t)
{
	BeginChunk(f, "tag");
	f->WriteStr(t->key);
	f->WriteStr(t->value);
	EndChunk(f);
}

void WriteEffect(CFile *f, Effect *e)
{
	BeginChunk(f, "effect");
	f->WriteStr(e->name);
	f->WriteBool(e->only_on_selection);
	f->WriteInt(e->range.offset);
	f->WriteInt(e->range.num);
	f->WriteStr(e->ConfigToString());
	f->WriteStr("");
	EndChunk(f);
}

void WriteBufferBox(CFile *f, BufferBox *b)
{
	BeginChunk(f, "bufbox");
	f->WriteInt(b->offset);
	f->WriteInt(b->num);
	f->WriteInt(2);
	f->WriteInt(16);

	Array<short> data;
	if (!b->get_16bit_buffer(data))
		tsunami->log->Error(_("Amplitude zu gro&s, Signal &ubersteuert."));
	f->WriteBuffer(data.data, data.num * sizeof(short));
	EndChunk(f);
}

void WriteSample(CFile *f, Sample *s)
{
	BeginChunk(f, "sample");
	f->WriteStr(s->name);
	f->WriteFloat(s->volume);
	f->WriteInt(s->offset);
	f->WriteInt(0); // reserved
	f->WriteInt(0);
	WriteBufferBox(f, &s->buf);
	EndChunk(f);
}

void WriteSampleRef(CFile *f, SampleRef *s)
{
	BeginChunk(f, "samref");

	f->WriteStr(s->origin->name);
	f->WriteInt(s->pos);
	f->WriteInt(get_sample_index(s->origin));
	f->WriteFloat(s->volume);
	f->WriteBool(s->muted);
	f->WriteInt(s->rep_num);
	f->WriteInt(s->rep_delay);
	f->WriteInt(0); // reserved
	f->WriteInt(0);

	EndChunk(f);
}

void WriteBar(CFile *f, BarPattern &b)
{
	BeginChunk(f, "bar");

	f->WriteInt(b.type);
	f->WriteInt(b.length);
	f->WriteInt(b.num_beats);
	f->WriteInt(b.count);
	f->WriteInt(0); // reserved

	EndChunk(f);
}

void WriteMidiNote(CFile *f, MidiNote &n)
{
	BeginChunk(f, "midinote");

	f->WriteInt(n.range.offset);
	f->WriteInt(n.range.num);
	f->WriteInt(n.pitch);
	f->WriteFloat(n.volume);
	f->WriteInt(0); // reserved

	EndChunk(f);
}

void WriteMidi(CFile *f, MidiData &m)
{
	BeginChunk(f, "midi");

	f->WriteStr("");
	f->WriteStr("");
	f->WriteStr("");
	f->WriteInt(0); // reserved

	foreach(MidiNote &n, m)
		WriteMidiNote(f, n);

	EndChunk(f);
}

void WriteSynth(CFile *f, Synthesizer *s)
{
	BeginChunk(f, "synth");

	f->WriteStr(s->name);
	f->WriteStr(s->options_to_string());
	f->WriteStr("");
	f->WriteInt(0); // reserved

	EndChunk(f);
}

void WriteTrackLevel(CFile *f, TrackLevel *l, int level_no)
{
	BeginChunk(f, "level");
	f->WriteInt(level_no);

	foreach(BufferBox &b, l->buffer)
		WriteBufferBox(f, &b);

	EndChunk(f);
}

void WriteTrack(CFile *f, Track *t)
{
	BeginChunk(f, "track");

	f->WriteStr(t->name);
	f->WriteFloat(t->volume);
	f->WriteBool(t->muted);
	f->WriteInt(t->type);
	f->WriteFloat(t->panning);
	f->WriteInt(0); // reserved
	f->WriteInt(0);

	foreach(BarPattern &b, t->bar)
		WriteBar(f, b);

	foreachi(TrackLevel &l, t->level, i)
		WriteTrackLevel(f, &l, i);

	foreach(SampleRef *s, t->sample)
		WriteSampleRef(f, s);

	foreach(Effect *effect, t->fx)
		WriteEffect(f, effect);

	if ((t->type == t->TYPE_TIME) || (t->type == t->TYPE_MIDI))
		WriteSynth(f, t->synth);

	if (t->midi.num > 0)
		WriteMidi(f, t->midi);

	EndChunk(f);
}

void WriteLevelName(CFile *f, Array<string> level_name)
{
	BeginChunk(f, "lvlname");

	f->WriteInt(level_name.num);
	foreach(string &l, level_name)
		f->WriteStr(l);

	EndChunk(f);
}

void FormatNami::SaveAudio(AudioFile *a, const string & filename)
{
	tsunami->progress->Start(_("speichere nami"), 0);
	a->filename = filename;

//	int length = a->GetLength();
//	int min = a->GetMin();
	CFile *f = FileCreate(filename);
	f->SetBinaryMode(true);

	BeginChunk(f, "nami");

	f->WriteInt(a->sample_rate);

	foreach(Tag &tag, a->tag)
		WriteTag(f, &tag);

	WriteLevelName(f, a->level_name);

	foreach(Sample *sample, a->sample)
		WriteSample(f, sample);

	foreachi(Track *track, a->track, i){
		WriteTrack(f, track);
		tsunami->progress->Set(_("speichere nami"), ((float)i + 0.5f) / (float)a->track.num);
	}

	foreach(Effect *effect, a->fx)
		WriteEffect(f, effect);

	EndChunk(f);

	FileClose(f);
	tsunami->progress->End();
}



void FormatNami::SaveBuffer(AudioFile *a, BufferBox *b, const string &filename)
{
}


#if 0
void ReadCompressed(CFile *f, char *data, int size)
{
	memset(data, 0, size);
	int done = 0;
	while(done < size){
		// how many non-zeros?
		int nonzero = f->ReadInt();
		f->ReadBuffer(&data[done], nonzero);
		done += nonzero;

		// how many zeros?
		int zero = f->ReadInt();
		done += zero;
		//printf("%d  %d  %d\n", nonzero, zero, done);
	}
}
#endif

SampleRef *__AddEmptySubTrack(Track *t, const Range &r, const string &name)
{
	BufferBox buf;
	buf.resize(r.length());
	t->root->AddSample(name, buf);
	return t->AddSample(r.start(), t->root->sample.num - 1);
}

typedef void chunk_reader(CFile*, void*);

struct ChunkHandler
{
	string tag;
	chunk_reader *reader;
	void *data;
};

struct ChunkLevelData
{
	ChunkLevelData(){}
	ChunkLevelData(const string &_tag, int _pos)
	{	tag = _tag;	pos = _pos;	}
	int pos;
	string tag;
	Array<ChunkHandler> handler;
};
Array<ChunkLevelData> chunk_data;

void AddChunkHandler(const string &tag, chunk_reader *reader, void *data)
{
	ChunkHandler h;
	h.tag = tag;
	h.reader = reader;
	h.data = data;
	chunk_data.back().handler.add(h);
}

void ReadChunkTag(CFile *f, Array<Tag> *tag)
{
	Tag t;
	t.key = f->ReadStr();
	t.value = f->ReadStr();
	tag->add(t);
}

void ReadChunkLevelName(CFile *f, AudioFile *a)
{
	int num = f->ReadInt();
	a->level_name.clear();
	for (int i=0;i<num;i++)
		a->level_name.add(f->ReadStr());
}

void ReadChunkEffectParamLegacy(CFile *f, Effect *e)
{
	EffectParam p;
	f->ReadStr();
	f->ReadStr();
	p.value = f->ReadStr();
	e->legacy_params.add(p);
}

void ReadChunkEffectLegacy(CFile *f, Array<Effect*> *fx)
{
	Effect *e = CreateEffect(f->ReadStr());
	e->only_on_selection = f->ReadBool();
	e->range.offset = f->ReadInt();
	e->range.num = f->ReadInt();
	fx->add(e);

	AddChunkHandler("fxparam", (chunk_reader*)&ReadChunkEffectParamLegacy, e);
}

void ReadChunkEffect(CFile *f, Array<Effect*> *fx)
{
	Effect *e = CreateEffect(f->ReadStr());
	e->only_on_selection = f->ReadBool();
	e->range.offset = f->ReadInt();
	e->range.num = f->ReadInt();
	string params = f->ReadStr();
	e->ConfigFromString(params);
	f->ReadStr();
	fx->add(e);

	AddChunkHandler("fxparam", (chunk_reader*)&ReadChunkEffectParamLegacy, e);
}

void ReadChunkBufferBox(CFile *f, TrackLevel *l)
{
	BufferBox dummy;
	l->buffer.add(dummy);
	BufferBox *b = &l->buffer.back();
	b->offset = f->ReadInt();
	int num = f->ReadInt();
	b->resize(num);
	f->ReadInt(); // channels (2)
	f->ReadInt(); // bit (16)

	Array<short> data;
	data.resize(num * 2);

	// read chunk'ed
	int offset = 0;
	for (int n=0;n<(num * 4) / CHUNK_SIZE;n++){
		f->ReadBuffer(&data[offset], CHUNK_SIZE);
		tsunami->progress->Set((float)f->GetPos() / (float)f->GetSize());
		offset += CHUNK_SIZE / 2;
	}
	f->ReadBuffer(&data[offset], (num * 4) % CHUNK_SIZE);

	// insert
	for (int i=0;i<num;i++){
		b->r[i] =  (float)data[i * 2    ] / 32768.0f;
		b->l[i] =  (float)data[i * 2 + 1] / 32768.0f;
	}
}


void ReadChunkSampleBufferBox(CFile *f, BufferBox *b)
{
	b->offset = f->ReadInt();
	int num = f->ReadInt();
	b->resize(num);
	f->ReadInt(); // channels (2)
	f->ReadInt(); // bit (16)

	Array<short> data;
	data.resize(num * 2);
	f->ReadBuffer(data.data, num * 4);
	for (int i=0;i<num;i++){
		b->r[i] =  (float)data[i * 2    ] / 32768.0f;
		b->l[i] =  (float)data[i * 2 + 1] / 32768.0f;
	}
}

void ReadChunkSample(CFile *f, AudioFile *a)
{
	Sample *s = new Sample;
	a->sample.add(s);
	s->owner = a;
	s->name = f->ReadStr();
	s->volume = f->ReadFloat();
	s->offset = f->ReadInt();
	f->ReadInt(); // reserved
	f->ReadInt();

	AddChunkHandler("bufbox", (chunk_reader*)&ReadChunkSampleBufferBox, &s->buf);
}

void ReadSampleRef(CFile *f, Track *t)
{
	string name = f->ReadStr();
	int pos = f->ReadInt();
	int index = f->ReadInt();
	SampleRef *s = t->AddSample(pos, index);
	s->volume = f->ReadFloat();
	s->muted = f->ReadBool();
	s->rep_num = f->ReadInt();
	s->rep_delay = f->ReadInt();
	f->ReadInt(); // reserved
	f->ReadInt();
}

void ReadSub(CFile *f, Track *t)
{
	string name = f->ReadStr();
	int pos = f->ReadInt();
	int length = f->ReadInt();
	SampleRef *s = __AddEmptySubTrack(t, Range(pos, length), name);
	s->volume = f->ReadFloat();
	s->muted = f->ReadBool();
	s->rep_num = f->ReadInt();
	s->rep_delay = f->ReadInt();
	f->ReadInt(); // reserved
	f->ReadInt();

	AddChunkHandler("bufbox", (chunk_reader*)&ReadChunkSampleBufferBox, &s->buf);
	tsunami->log->Error("\"sub\" chunk is deprecated!");
}

void ReadChunkBar(CFile *f, Array<BarPattern> *bar)
{
	BarPattern b;
	b.type = f->ReadInt();
	b.length = f->ReadInt();
	b.num_beats = f->ReadInt();
	b.count = f->ReadInt();
	f->ReadInt(); // reserved
	bar->add(b);
}

void ReadChunkMidiNote(CFile *f, Array<MidiNote> *notes)
{
	MidiNote n;
	n.range.offset = f->ReadInt();
	n.range.num = f->ReadInt();
	n.pitch = f->ReadInt();
	n.volume = f->ReadFloat();
	f->ReadInt(); // reserved
	notes->add(n);
}

void ReadChunkMidiData(CFile *f, MidiData *midi)
{
	f->ReadStr();
	f->ReadStr();
	f->ReadStr();
	f->ReadInt(); // reserved

	AddChunkHandler("midinote", (chunk_reader*)&ReadChunkMidiNote, midi);
}

void ReadChunkSynth(CFile *f, Track *t)
{
	delete(t->synth);
	t->synth = CreateSynthesizer(f->ReadStr());
	t->synth->options_from_string(f->ReadStr());
	f->ReadStr();
	f->ReadInt();
}

void ReadChunkTrackLevel(CFile *f, Track *t)
{
	int l = f->ReadInt();
	AddChunkHandler("bufbox", (chunk_reader*)&ReadChunkBufferBox, &t->level[l]);
}

void ReadChunkTrack(CFile *f, AudioFile *a)
{
	Track *t = a->AddTrack(Track::TYPE_AUDIO);
	t->name = f->ReadStr();
	t->volume = f->ReadFloat();
	t->muted = f->ReadBool();
	t->type = f->ReadInt();
	t->panning = f->ReadFloat();
	f->ReadInt(); // reserved
	f->ReadInt();
	tsunami->progress->Set((float)f->GetPos() / (float)f->GetSize());

	AddChunkHandler("level", (chunk_reader*)&ReadChunkTrackLevel, t);
	AddChunkHandler("bufbox", (chunk_reader*)&ReadChunkBufferBox, &t->level[0]);
	AddChunkHandler("samref", (chunk_reader*)&ReadSampleRef, t);
	AddChunkHandler("sub", (chunk_reader*)&ReadSub, t);
	AddChunkHandler("fx", (chunk_reader*)&ReadChunkEffectLegacy, &t->fx);
	AddChunkHandler("effect", (chunk_reader*)&ReadChunkEffect, &t->fx);
	AddChunkHandler("bar", (chunk_reader*)&ReadChunkBar, &t->bar);
	AddChunkHandler("midi", (chunk_reader*)&ReadChunkMidiData, &t->midi);
	AddChunkHandler("synth", (chunk_reader*)&ReadChunkSynth, t);
}

void ReadChunkNami(CFile *f, AudioFile *a)
{
	a->sample_rate = f->ReadInt();

	AddChunkHandler("tag", (chunk_reader*)&ReadChunkTag, &a->tag);
	AddChunkHandler("fx", (chunk_reader*)&ReadChunkEffectLegacy, &a->fx);
	AddChunkHandler("effect", (chunk_reader*)&ReadChunkEffect, &a->fx);
	AddChunkHandler("lvlname", (chunk_reader*)&ReadChunkLevelName, a);
	AddChunkHandler("sample", (chunk_reader*)&ReadChunkSample, a);
	AddChunkHandler("track", (chunk_reader*)&ReadChunkTrack, a);
}

void strip(string &s)
{
	while((s.num > 0) && (s.back() == ' '))
		s.resize(s.num - 1);
}

void ReadChunk(CFile *f)
{
	string cname;
	cname.resize(8);
	f->ReadBuffer(cname.data, 8);
	strip(cname);
	int size = f->ReadInt();
	chunk_data.add(ChunkLevelData(cname, f->GetPos() + size));


	bool handled = false;
	foreach(ChunkHandler &h, chunk_data[chunk_data.num - 2].handler)
		if (cname == h.tag){
			h.reader(f, h.data);
			handled = true;
			break;
		}

	if (handled){

		// read nested chunks
		while (f->GetPos() < chunk_data.back().pos)
			ReadChunk(f);

	}else
		tsunami->log->Error("unknown nami chunk: " + cname + " (within " + chunk_data[chunk_data.num - 2].tag + ")");


	f->SetPos(chunk_data.back().pos, true);
	chunk_data.pop();
}

void load_nami_file_new(CFile *f, AudioFile *a)
{
	chunk_data.clear();
	chunk_data.add(ChunkLevelData("-top level-", 0));
	AddChunkHandler("nami", (chunk_reader*)&ReadChunkNami, a);

	ReadChunk(f);
	chunk_data.clear();
}


void check_empty_subs(AudioFile *a)
{
	/*foreach(Track *t, a->track)
		foreachib(Track *s, t->sub, i)
			if (s->length <= 0){
				tsunami->log->Error("empty sub: " + s->name);
				t->sub.erase(i);
			}*/
}

void update_legacy_fx(Effect *fx)
{
	if (fx->legacy_params.num == 0)
		return;
	string params = "(";
	for (int i=0;i<fx->legacy_params.num;i++){
		if (i > 0)
			params += " ";
		params += fx->legacy_params[i].value;
	}
	params += ")";
	msg_write("legacy params: " + params);
	fx->ConfigFromString(params);
}

void FormatNami::LoadAudio(AudioFile *a, const string & filename)
{
	msg_db_f("load_nami_file", 1);
	tsunami->progress->Set(_("lade nami"), 0);

	// TODO?
	a->tag.clear();

	CFile *f = FileOpen(a->filename);
	f->SetBinaryMode(true);

	load_nami_file_new(f, a);

	FileClose(f);

	// some post processing
	check_empty_subs(a);

	foreach(Effect *fx, a->fx)
		update_legacy_fx(fx);
	foreach(Track *t, a->track)
		foreach(Effect *fx, t->fx)
			update_legacy_fx(fx);

	a->UpdateSelection();
}



void FormatNami::LoadTrack(Track *t, const string &filename, int offset, int level)
{
}

