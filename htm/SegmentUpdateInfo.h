#pragma once
#include "MemObject.h"
#include "FastList.h"

class Cell;
class Segment;
class SynapseParameters;

enum UpdateType 
{
	UPDATE_DUE_TO_ACTIVE,
	UPDATE_DUE_TO_PREDICTIVE
};

enum ApplyUpdateTrigger
{
	AUT_ACTIVE,
	AUT_INACTIVE,
	AUT_LONGER_PREDICTION
};

/// This data structure holds three pieces of information required to update a given 
/// segment: 
/// a) segment reference (None if it's a new segment), 
/// b) a list of existing active synapses, and 
/// c) a flag indicating whether this segment should be marked as a sequence
/// segment (defaults to false).
/// The structure also determines which learning cells (at this time step) are available 
/// to connect (add synapses to) should the segment get updated. If there is a prediction 
/// radius set on the Region, the pool of learning cells is restricted to those within
/// the radius.
class SegmentUpdateInfo :
	public MemObject
{
public:
	SegmentUpdateInfo(void);
	~SegmentUpdateInfo(void);

	MemObjectType GetMemObjectType() {return MOT_SEGMENT_UPDATE_INFO;}

	void Retire();

	/// Properties

	Cell *cell;
	Segment *segment;
	bool AddNewSynapses;
	int NumPredictionSteps, CreationTimeStep;
	UpdateType updateType;
	FastList ActiveDistalSynapses;
	FastList CellsThatWillLearn;

	Cell *GetCell() {return cell;}
	void SetCell(Cell *value) {cell = value;}

	Segment *GetSegment() {return segment;}
	void SetSegment(Segment *value) {segment = value;}

	bool GetAddNewSynapses() {return AddNewSynapses;}
	void SetAddNewSynapses(bool value) {AddNewSynapses = value;}

	int GetNumPredictionSteps() {return NumPredictionSteps;}
	void SetNumPredictionSteps(int value) {NumPredictionSteps = value;}

	int GetCreationTimeStep() {return CreationTimeStep;}

	UpdateType GetUpdateType() {return updateType;}

	/// Methods

	/// Randomly sample m values from the Cell array of length n (m less than n).
	/// Runs in O(2m) worst case time.  Result is written to the result
	/// array of length m containing the randomly chosen cells.
	/// 
	/// cells: input Cells to randomly choose from.
	/// result: the resulting random subset of Cells.
	/// m: the number of random samples to take (m less than equal to result.Length)
	void RandomSample(FastList &cells, FastList &result, int m);

	///Create a new SegmentUpdateInfo that is to modify the state of the Region
	///either by adding a new segment to a cell, new synapses to a segment,
	///or updating permanences of existing synapses on some segment.
	///
	/// cell: cell the cell that is to have a segment added or updated.
	/// segment: the segment that is to be updated (null here means a new
	///  segment is to be created on the parent cell).
	/// activeDistalSynapses: the set of active synapses on the segment 
	///  that are to have their permanences updated. 
	/// addNewSynapses: set to true if new synapses are to be added to the
	///  segment (or if new segment is being created) or false if no new synapses
	///  should be added instead only existing permanences updated. 
	///
	void Initialize(Cell *_cell, Segment *_segment, FastList *activeDistalSynapses, bool addNewSynapses, int _creationTimeStep, UpdateType _updateType);

	/// Create a new segment on the update cell using connections from
	/// the set of learning cells for the update info.
	Segment *CreateCellSegment(int time);

	/// Create new synapse connections to the segment to be updated using
	/// the set of learning cells in this update info.
	void CreateSynapsesToLearningCells(SynapseParameters *params);
};

