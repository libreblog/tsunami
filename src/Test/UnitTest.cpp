/*
 * UnitTest.cpp
 *
 *  Created on: 22.07.2018
 *      Author: michi
 */

#include "UnitTest.h"
#include "../lib/file/msg.h"

UnitTest::UnitTest(const string &_name)
{
	name = _name;
}

UnitTest::~UnitTest()
{
}

void UnitTest::run()
{
	msg_write("== " + name + " ==");
	for (auto &t: tests()){
		msg_write(t.name);
		t.f();
		msg_write("  ok");
	}
}

#include "TestAudioBuffer.h"
#include "TestStreams.h"
#include "TestThreads.h"

void UnitTest::run_all(const string &filter)
{
	Array<UnitTest*> tests;
	tests.add(new TestAudioBuffer);
	tests.add(new TestThreads);
	tests.add(new TestStreams);

	for (auto *t: tests)
		if (filter.num == 0 or filter.find(t->name) >= 0)
			t->run();

	for (auto *t: tests)
		delete t;


}
