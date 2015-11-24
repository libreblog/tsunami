/*
 * StorageOperationData.cpp
 *
 *  Created on: 09.06.2015
 *      Author: michi
 */

#include "StorageOperationData.h"
#include "../View/Helper/Progress.h"
#include "../Tsunami.h"
#include "../Stuff/Log.h"
#include "../Data/BufferBox.h"
#include "../Audio/AudioRenderer.h"

StorageOperationData::StorageOperationData(Storage *_storage, Format *_format, Song *s, Track *t, BufferBox *b, const string &_filename, const string &message, HuiWindow *_win)
{
	win = _win;
	storage = _storage;
	format = _format;
	song = s;
	filename = _filename;
	progress = new Progress(message, win);
	buf = b;
	track = t;
	offset = 0;
	level = 0;
	renderer = NULL;
}

StorageOperationData::~StorageOperationData()
{
	delete(progress);
}

void StorageOperationData::info(const string& message)
{
	tsunami->log->info(message);
}

void StorageOperationData::warn(const string& message)
{
	tsunami->log->warn(message);
}

void StorageOperationData::error(const string& message)
{
	tsunami->log->error(message);
}

void StorageOperationData::set(float t)
{
	progress->set(t);
}

void StorageOperationData::set(const string &str, float t)
{
	progress->set(str, t);
}

int StorageOperationData::get_num_samples()
{
	if (buf)
		return buf->num;
	if (renderer)
		return renderer->getNumSamples();
	return 0;
}
