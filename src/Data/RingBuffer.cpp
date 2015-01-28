/*
 * RingBuffer.cpp
 *
 *  Created on: 26.01.2015
 *      Author: michi
 */

#include "RingBuffer.h"

RingBuffer::RingBuffer(int size)
{
	buf.resize(size);
	read_pos = 0;
	write_pos = 0;
}

RingBuffer::~RingBuffer()
{
}

void RingBuffer::clear()
{
	write_pos = 0;
	read_pos = 0;
}

int RingBuffer::available()
{
	if (write_pos < read_pos)
		return write_pos - read_pos + buf.num;
	return write_pos - read_pos;
}

void RingBuffer::read(BufferBox& b)
{
	b.set(buf, read_pos, 1.0f);
	read_pos += b.num;
	if (read_pos >= buf.num)
		read_pos = 0;
}

void RingBuffer::write(BufferBox& b)
{
	int size_a = min(b.num, buf.num - write_pos);
	buf.set(b, write_pos, 1.0f);

	int size_b = b.num - size_a;
	if (size_b > 0){
		buf.set(b, -size_a, 1.0f);
	}

	write_pos += b.num;
	if (write_pos >= buf.num)
		write_pos -= buf.num;
}

void RingBuffer::peekRef(BufferBox &b, int size)
{
	size = min(size, available());
	size = min(size, buf.num - read_pos);
	b.set_as_ref(buf, read_pos, size);
}

void RingBuffer::readRef(BufferBox &b, int size)
{
	peekRef(b, size);
	read_pos += b.num;
	if (read_pos >= buf.num)
		read_pos -= buf.num;
}
