#pragma once
#include "MemObject.h"
#include "Segment.h"
#include "SegmentUpdateInfo.h"

class Column;

/// A data structure representing a single context sensitive cell.
class Cell :
	public MemObject
{
public:
	Cell(void);
	~Cell(void);

	MemObjectType GetMemObjectType() {return MOT_CELL;} 
	void Retire();

	/// Properties

private:

	int Index, NumPredictionSteps, PrevNumPredictionSteps, PrevActiveTime;
	bool IsActive, WasActive, IsLearning, WasLearning, _isPredicting;
	bool IsSegmentPredicting, WasSegmentPredicted, WasPredicted;
	Column *column;

	void SetColumn(Column *value) {column = value;}

	void SetNumPredictionSteps(int value) {NumPredictionSteps = value;}

public:

	FastList Segments;
	FastList _segmentUpdates;

	/// Position in Column
	int GetIndex() {return Index;}
	void SetIndex(int value) {Index = value;}

	/// Gets or sets a value indicating whether this Cell is active.
	///   true if this instance is active; otherwise, false.
	bool GetIsActive() {return IsActive;}
	void SetIsActive(bool value);

	/// Gets a value indicating whether this Cell was active.
	///   true if it was active; otherwise, false.
	bool GetWasActive() {return WasActive;}
	void SetWasActive(bool value) {WasActive = value;}

	/// Gets or sets a value indicating whether this Cell is learning.
	/// true if it is learning; otherwise, false.
	bool GetIsLearning() {return IsLearning;}
	void SetIsLearning(bool value) {IsLearning = value;}

	/// Gets a value indicating whether this Cell was learning.
	///   true if it was learning; otherwise, false.
	bool GetWasLearning() {return WasLearning;}
	void SetWasLearning(bool value) {WasLearning = value;}

	bool GetIsPredicting() {return _isPredicting;}
	void SetIsPredicting(bool value, int numPredictionSteps);

	bool GetIsSegmentPredicting() {return IsSegmentPredicting;}
	void SetIsSegmentPredicting(bool value) {IsSegmentPredicting = value;}

	/// Indicates whether this cell was predicted to become active.
	bool GetWasPredicted() {return WasPredicted;}

	int GetNumPredictionSteps() {return NumPredictionSteps;}
	int GetPrevNumPredictionSteps() {return PrevNumPredictionSteps;}

	int GetPrevActiveTme() {return PrevActiveTime;}

	Column *GetColumn() {return column;}

	/// Methods

	/// Initialize a new Cell belonging to the specified Column. The index is an 
	/// integer id to distinguish this Cell from others in the Column.
	void Initialize(Column *col, int intdx);

	/// Advances this cell to the next time step. 
	/// 
	/// The current state of this cell (active, learning, predicting) will be set as the
	/// previous state and the current state will be reset to no cell activity by 
	/// default until it can be determined.
	void NextTimeStep();

	/// Creates a new segment for this Cell.
	/// learningCells: A set of available learning cells to add to the segmentUpdateList.
	/// Returns created segment.
	///
	/// The new segment will initially connect to at most newSynapseCount 
	/// synapses randomly selected from the set of cells that
	/// were in the learning state at t-1 (specified by the learningCells parameter).
	Segment *CreateSegment(FastList &learningCells, int creationTime);

	/// For this cell, return a Segment that was active in the previous
	/// time step. If multiple segments were active, sequence segments are given
	/// preference. Otherwise, segments with most activity are given preference.
	Segment *GetPreviousActiveSegment();

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
	SegmentUpdateInfo *UpdateSegmentActiveSynapses(bool previous, Segment *segment, bool newSynapses, UpdateType updateType);

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
	void ApplySegmentUpdates(int _curTime, ApplyUpdateTrigger _trigger);

	/// For this cell in the previous time step (t-1) find the Segment 
	/// with the largest number of active synapses.
	/// 
	/// However only consider segments that predict activation in the number of 
	///  time steps of the active segment of this cell with the least number of 
	///  steps until activation + 1.  For example if right now this cell is being 
	///  predicted to occur in t+2 at the earliest, then we want to find the best 
	///  segment from last time step that would predict for t+3.
	///  This routine is aggressive in finding the best match. The permanence
	///  value of synapses is allowed to be below connectedPerm.
	///  The number of active synapses is allowed to be below activationThreshold,
	///  but must be above minThreshold. The routine returns that segment.
	/// If no segments are found, then None is returned.
	Segment *GetBestMatchingPreviousSegment();

	/// Gets the best matching Segment.
	/// 
	/// numPredictionSteps: Number of time steps in the future an activation will occur.
	/// previous: Defaults to false.
	/// 
	/// For this cell (at t-1 if previous=True else at t), find the segmentUpdateList (only
	/// consider sequence segments if isSequence is True, otherwise only consider
	/// non-sequence segments) with the largest number of active synapses. 
	/// This routine is aggressive in finding the best match. The permanence 
	/// value of synapses is allowed to be below connectedPerm. 
	/// The number of active synapses is allowed to be below activationThreshold, 
	/// but must be above minThreshold. The routine returns that segment. 
	/// If no segments are found, then None is returned.
	Segment *GetBestMatchingSegment(int numPredictionSteps, bool previous);
};

