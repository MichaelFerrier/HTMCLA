#include "Cell.h"
#include "Column.h"
#include "Region.h"
#include "SegmentUpdateInfo.h"

extern MemManager mem_manager;

Cell::Cell(void)
{
}

Cell::~Cell(void)
{
}

void Cell::Retire()
{
	// Release all Segments.
	Segment *segment;
	while ((segment = (Segment*)(Segments.RemoveFirst())) != NULL) {
		mem_manager.ReleaseObject(segment);
	}

	// Release all SegmentsUpdateInfo objects.
	SegmentUpdateInfo *segmentUpdateInfo;
	while ((segmentUpdateInfo = (SegmentUpdateInfo*)(_segmentUpdates.RemoveFirst())) != NULL) {
		mem_manager.ReleaseObject(segmentUpdateInfo);
	}
}

void Cell::SetIsActive(bool value) 
{
	IsActive = value;

	// Record this cells's latest time being active.
	if (IsActive) {
		PrevActiveTime = column->region->GetStepCounter();
	}
}

void Cell::SetIsPredicting(bool value, int numPredictionSteps)
{
	if (value && !_isPredicting)
	{
		// This call is for the first segment found active, to make this cell predictive. Adopt that segment's NumPredictionSteps.
		NumPredictionSteps = numPredictionSteps;
	}
	else
	{
		// This call is not for the firt active segment found; use the lower NumPredictionSteps between the segment's given value and the stored value.
		NumPredictionSteps = Min(NumPredictionSteps, numPredictionSteps);
	}

	_isPredicting = value;
}

/// Methods

/// Initialize a new Cell belonging to the specified Column. The index is an 
/// integer id to distinguish this Cell from others in the Column.
/// col
/// intdx
void Cell::Initialize(Column *col, int index)
{
	column = col;
	Index = index;
	IsActive = false;
	WasActive = false;
	_isPredicting = false;
	WasPredicted = false;
	IsLearning = false;
	WasLearning = false;
	NumPredictionSteps = 0;
	PrevNumPredictionSteps = 0;
	PrevActiveTime = -1;
}

/// Advances this cell to the next time step. 
/// 
/// The current state of this cell (active, learning, predicting) will be set as the
/// previous state and the current state will be reset to no cell activity by 
/// default until it can be determined.
void Cell::NextTimeStep()
{
	WasPredicted = _isPredicting;
	WasSegmentPredicted = IsSegmentPredicting;
	WasActive = IsActive;
	WasLearning = IsLearning;
	_isPredicting = false;
	IsSegmentPredicting = false;
	IsActive = false;
	IsLearning = false;
	PrevNumPredictionSteps = NumPredictionSteps;
	NumPredictionSteps = 0;

	FastListIter segments_iter(Segments);
	for (Segment *seg = (Segment*)(segments_iter.Reset()); seg != NULL; seg = (Segment*)(segments_iter.Advance()))
	{
		seg->NextTimeStep();
	}
}

/// Creates a new segment for this Cell.
/// learningCells: A set of available learning cells to add to the segmentUpdateList.
/// Returns created segment.
///
/// The new segment will initially connect to at most newSynapseCount 
/// synapses randomly selected from the set of cells that
/// were in the learning state at t-1 (specified by the learningCells parameter).
Segment *Cell::CreateSegment(FastList &learningCells, int creationTime)
{
	Segment *newSegment = (Segment*)(mem_manager.GetObject(MOT_SEGMENT));
	newSegment->Initialize(creationTime, (float)(column->region->SegActiveThreshold));
	newSegment->CreateSynapsesToLearningCells(learningCells, &(column->region->DistalSynapseParams));
	Segments.InsertAtEnd(newSegment);
	return newSegment;
}

/// For this cell, return a Segment that was active in the previous
/// time step. If multiple segments were active, sequence segments are given
/// preference. Otherwise, segments with most activity are given preference.
Segment *Cell::GetPreviousActiveSegment()
{
	Segment *bestSegment = NULL;
	bool foundSequence = false;
	int mostSyns = 0;
	int activeSyns;

	FastListIter segments_iter(Segments);
	for (Segment *seg = (Segment*)(segments_iter.Reset()); seg != NULL; seg = (Segment*)(segments_iter.Advance()))
	{
		activeSyns = seg->GetPrevActiveConnectedSynapseCount();
		if (activeSyns >= seg->GetActiveThreshold())
		{
			//if segment is active, check for sequence segment and compare
			//active synapses
			if (seg->GetIsSequence())
			{
				foundSequence = true;
				if (activeSyns > mostSyns)
				{
					mostSyns = activeSyns;
					bestSegment = seg;
				}
			}
			else if ((!foundSequence) && (activeSyns > mostSyns))
			{
				mostSyns = activeSyns;
				bestSegment = seg;
			}
		}
	}

	return bestSegment;
}

/// Add a new SegmentUpdateInfo object to this Cell containing proposed changes to the 
/// specified segment. 
///
/// If the segment is NULL, then a new segment is to be added, otherwise
/// the specified segment is updated.  If the segment exists, find all active
/// synapses for the segment (either at t or t-1 based on the 'previous' parameter)
/// and mark them as needing to be updated.  If newSynapses is true, then
/// Region.newSynapseCount - len(activeSynapses) new synapses are added to the
/// segment to be updated.  The (new) synapses are randomly chosen from the set
/// of current learning cells (within Region.predictionRadius if set).
///
/// These segment updates are only applied when the applySegmentUpdates
/// method is later called on this Cell.
SegmentUpdateInfo *Cell::UpdateSegmentActiveSynapses(bool previous, Segment *segment, bool newSynapses, UpdateType updateType)
{
	FastList *activeSyns = NULL;
	if (segment != NULL)
	{
		activeSyns = previous ? &(segment->PrevActiveSynapses) : &(segment->ActiveSynapses);
	}

	SegmentUpdateInfo *segmentUpdate = (SegmentUpdateInfo*)(mem_manager.GetObject(MOT_SEGMENT_UPDATE_INFO));
	segmentUpdate->Initialize(this, segment, activeSyns, newSynapses, column->region->GetStepCounter(), updateType);
	_segmentUpdates.InsertAtEnd(segmentUpdate);
	return segmentUpdate;
}

/// This function reinforces each segment in this Cell's SegmentUpdateInfo.
///
/// Using the segmentUpdateInfo, the following changes are
/// performed. If positiveReinforcement is true then synapses on the active
/// list get their permanence counts incremented by permanenceInc. All other
/// synapses get their permanence counts decremented by permanenceDec. If
/// positiveReinforcement is false, then synapses on the active list get
/// their permanence counts decremented by permanenceDec. After this step,
/// any synapses in segmentUpdate that do yet exist get added with a permanence
/// count of initialPerm. These new synapses are randomly chosen from the
/// set of all cells that have learnState output = 1 at time step t.
///</remarks>
void Cell::ApplySegmentUpdates(int _curTime, ApplyUpdateTrigger _trigger)
{
	Segment *segment;
	SegmentUpdateInfo *segInfo;
	Synapse *syn;
	FastList modifiedSegments;
	FastListIter synapses_iter, seg_update_iter;
	bool apply_update;

	// Iterate through all segment updates, skipping those not to be applied now, and removing those that are applied.
	seg_update_iter.SetList(_segmentUpdates);
	for (segInfo = (SegmentUpdateInfo*)(seg_update_iter.Reset()); segInfo != NULL; segInfo = (SegmentUpdateInfo*)(seg_update_iter.Get()))
	{
		// Do not apply the current segment update if it was created at the current time step, and was created as a result of the cell being predictive.
		// If this is the case, then this segment update can only be evaluated at a later time step, and so should remain in the queue for now.
		apply_update = !((segInfo->GetCreationTimeStep() == _curTime) && (segInfo->GetUpdateType() == UPDATE_DUE_TO_PREDICTIVE));

		// Do not apply the current segment update if its numPredictionSteps > 1, and if we are applying updates due to the cell still being predictive,
		// but with a greater numPredictionSteps. Unless the segment being updated predicted activation in 1 prediction step, it cannot be proven
		// incorrect, and so shouldn't be processed yet. This is because a "prediction step" can take more than one time step.
		if ((_trigger == AUT_LONGER_PREDICTION) && (segInfo->GetNumPredictionSteps() > 1)) {
			apply_update = false;
		}

		if (apply_update)
		{
			segment = segInfo->GetSegment();

			if (segment != NULL)
			{
				if (_trigger == AUT_ACTIVE)
				{
					// The cell has become actve; positively reinforce the segment.
					segment->UpdatePermanences(segInfo->ActiveDistalSynapses);
				}
				else
				{
					// The cell has become inactive of is predictive but with a longer prediction. Negatively reinforce the segment.
					segment->DecreasePermanences(segInfo->ActiveDistalSynapses);
				}

				// Record that this segment has been modified, so it can later be checked for synapses that should be removed.
				if (modifiedSegments.IsInList(segment) == false) {
					modifiedSegments.InsertAtEnd(segment);
				}
			}

			// Add new synapses (and new segment if necessary)
			if (segInfo->GetAddNewSynapses() && (_trigger == AUT_ACTIVE))
			{
				if (segment == NULL)
				{
					if (segInfo->CellsThatWillLearn.Count() > 0) //only add if learning cells available
					{
						segment = segInfo->CreateCellSegment(column->region->GetStepCounter());
					}
				}
				else if (segInfo->CellsThatWillLearn.Count() > 0)
				{
					//add new synapses to existing segment
					segInfo->CreateSynapsesToLearningCells(&(column->region->DistalSynapseParams));
				}
			}

			// Remove and release the current SegmentUpdateInfo.
			seg_update_iter.Remove();
			mem_manager.ReleaseObject(segInfo);
		}
		else
		{
			// Leave the current segment upate in the queue, to be processed at a later time step. Advance to the next one.
			seg_update_iter.Advance();
		}
	}

	// Only process modified segments if all segment updates have been processed, to avoid deleting segments or synapses that
	// are referred to by still-existing segment updates.
	if (_segmentUpdates.Count() == 0)
	{
		// All segment updates have been processed, so there are none left that may have references to this cell's 
		// synapses or segments. Therefore we can iterate through all segments modified above, to prune unneeded synpses and segments.
		while (modifiedSegments.Count() > 0)
		{
			// Get pointer to the current modified segment, and remove it from the modifiedSegments list.
			segment = (Segment*)(modifiedSegments.GetFirst());
			modifiedSegments.RemoveFirst();

			// Remove from the current modified segment any synapses that have reached permanence of 0.
			synapses_iter.SetList(segment->Synapses);
			for (syn = (Synapse*)(synapses_iter.Reset()); syn != NULL;)
			{
				if (syn->GetPermanence() == 0.0f)
				{
					// Remove and release the current synapse, whose permanence has reached 0.
					synapses_iter.Remove();
					mem_manager.ReleaseObject(syn);

					// Removing the synapse has advanced the iterator to the next synapse. Get a pointer to it.
					syn = (Synapse*)(synapses_iter.Get());
				}
				else
				{
					// Advance to the next synapse.
					syn = (Synapse*)(synapses_iter.Advance());
				}
			}

			// If this modified segment now has no synapses, remove the segment from this cell.
			if (segment->Synapses.Count() == 0)
			{
				Segments.Remove(segment, false);
				mem_manager.ReleaseObject(segment);
			}
		}
	}
	else
	{
		// Not all of the segment updates have been processed, so synapses and segments cannot be pruned. 
		// (because they may be referred to by segment updates that still exist). So just clear the list of modified segments.
		modifiedSegments.Clear();
	}
}

/// For this cell in the previous time step (t-1) find the Segment 
/// with the largest number of active synapses.
/// 
/// However only consider segments that predict activation in the number of 
///  time steps of the active segment of this cell with the least number of 
///  steps until activation, + 1.  For example if right now this cell is being 
///  predicted to occur in t+2 at the earliest, then we want to find the best 
///  segment from last time step that would predict for t+3.
///  This routine is aggressive in finding the best match. The permanence
///  value of synapses is allowed to be below connectedPerm.
///  The number of active synapses is allowed to be below activationThreshold,
///  but must be above minThreshold. The routine returns that segment.
/// If no segments are found, then None is returned.
Segment *Cell::GetBestMatchingPreviousSegment()
{
	return GetBestMatchingSegment(NumPredictionSteps + 1, true);
}

/// Gets the best matching Segment.
/// 
/// numPredictionSteps: Number of time steps in the future an activation will occur.
/// previous: If true, returns for previous time step. Defaults to false.
/// 
/// For this cell (at t-1 if previous=True else at t), find the Segment (only
/// consider sequence segments if isSequence is True, otherwise only consider
/// non-sequence segments) with the largest number of active synapses. 
/// This routine is aggressive in finding the best match. The permanence 
/// value of synapses is allowed to be below connectedPerm. 
/// The number of active synapses is allowed to be below activationThreshold, 
/// but must be above minThreshold. The routine returns that segment. 
/// If no segments are found, then None is returned.
Segment *Cell::GetBestMatchingSegment(int numPredictionSteps, bool previous)
{
	Segment *bestSegment = NULL;
	int bestSynapseCount = column->GetMinOverlapToReuseSegment();
	int synCount;

	FastListIter segments_iter(Segments);
	for (Segment *seg = (Segment*)(segments_iter.Reset()); seg != NULL; seg = (Segment*)(segments_iter.Advance()))
	{
		if (seg->GetNumPredictionSteps() != numPredictionSteps) {
			continue;
		}

		synCount = 0;
		if (previous)
		{
			// Get all the active previously synapses, no matter connection value
			synCount = seg->GetPrevActiveSynapseCount();
		}
		else
		{
			// Get all the momentarily active previously synapses, no matter connection value
			synCount = seg->GetActiveSynapseCount();
		}

		if (synCount >= bestSynapseCount)
		{
			bestSynapseCount = synCount;
			bestSegment = seg;
		}
	}

	return bestSegment;
}
