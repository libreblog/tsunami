/*
 * ActionTrackInsertSelectedSamples.h
 *
 *  Created on: 11.07.2012
 *      Author: michi
 */

#ifndef ACTIONTRACKINSERTSELECTEDSAMPLE_H_
#define ACTIONTRACKINSERTSELECTEDSAMPLES_H_

#include "../../ActionGroup.h"
class TrackLayer;
class SongSelection;

class ActionTrackInsertSelectedSamples : public ActionGroup
{
public:
	ActionTrackInsertSelectedSamples(const SongSelection &sel, TrackLayer *l);

	void build(Data *d) override;

	const SongSelection &sel;
	TrackLayer *layer;
};

#endif /* ACTIONTRACKINSERTSELECTEDSAMPLES_H_ */
