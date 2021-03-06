/*
 * ActionTrackAddMarker.h
 *
 *  Created on: May 13, 2015
 *      Author: ankele
 */

#ifndef SRC_ACTION_TRACK_MARKER_ACTIONTRACKADDMARKER_H_
#define SRC_ACTION_TRACK_MARKER_ACTIONTRACKADDMARKER_H_

#include "../../Action.h"

class TrackLayer;
class TrackMarker;
class Range;

class ActionTrackAddMarker: public Action {
public:
	ActionTrackAddMarker(TrackLayer *l, TrackMarker *marker);
	~ActionTrackAddMarker() override;

	void *execute(Data *d) override;
	void undo(Data *d) override;

private:
	TrackMarker *marker;
	TrackLayer *layer;
};

#endif /* SRC_ACTION_TRACK_MARKER_ACTIONTRACKADDMARKER_H_ */
