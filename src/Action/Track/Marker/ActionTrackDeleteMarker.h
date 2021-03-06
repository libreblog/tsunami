/*
 * ActionTrackDeleteMarker.h
 *
 *  Created on: May 13, 2015
 *      Author: ankele
 */

#ifndef SRC_ACTION_TRACK_MARKER_ACTIONTRACKDELETEMARKER_H_
#define SRC_ACTION_TRACK_MARKER_ACTIONTRACKDELETEMARKER_H_

#include "../../Action.h"
class TrackLayer;
class TrackMarker;

class ActionTrackDeleteMarker: public Action {
public:
	ActionTrackDeleteMarker(TrackLayer *l, int index);
	~ActionTrackDeleteMarker() override;

	void *execute(Data *d) override;
	void undo(Data *d) override;

private:
	int index;
	TrackMarker *marker;
	TrackLayer *layer;
};

#endif /* SRC_ACTION_TRACK_MARKER_ACTIONTRACKDELETEMARKER_H_ */
