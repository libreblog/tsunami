/*
 * Curve.h
 *
 *  Created on: 19.04.2014
 *      Author: michi
 */

#ifndef CURVE_H_
#define CURVE_H_

#include "../Stuff/Observable.h"

class AudioFile;
class Track;
class Effect;
class Configurable;
namespace Script
{
	class Type;
};

class Curve : public Observable
{
public:
	Curve();
	virtual ~Curve();

	enum
	{
		TYPE_LINEAR,
		TYPE_LOG,
	};

	struct Target
	{
		float *p;
		string temp_name;
		string temp_name_nice;
		Target();
		Target(float *p);
		Target(float *p, const string &name, const string &name_nice);
		void fromString(const string &str, AudioFile *a);
		string str(AudioFile *a);
		string niceStr(AudioFile *a);

		static Array<Target> enumerate(AudioFile *a);
		static void enumerateTrack(Track *t, Array<Target> &list, const string &prefix, const string &prefix_nice);
		static void enumerateConfigurable(Configurable *c, Array<Target> &list, const string &prefix, const string &prefix_nice);
		static void enumerateType(char *p, Script::Type *t, Array<Target> &list, const string &prefix, const string &prefix_nice);
	};

	string name;
	Array<Target> target;
	Array<float> temp_value;
	int type;

	float min, max;

	struct Point
	{
		int pos;
		float value;
	};

	Array<Point> points;

	void add(int pos, float value);
	float get(int pos);

	void apply(int pos);
	void unapply();

	string getTargets(AudioFile *a);
};

#endif /* CURVE_H_ */