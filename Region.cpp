#include "Region.h"
#include "NetworkManager.h"
#include <math.h>
#include <crtdbg.h>
#include "Utils.h"
#include "Cell.h"

Region::~Region(void)
{
	// Delete all columns.
	for (int cy = 0; cy < Height; cy++)
	{
		for (int cx = 0; cx < Width; cx++)
		{
			// Delete the current column.
			delete Columns[(cy * Width) + cx];
		}
	}

	// Delete the array of Column pointers.
	delete Columns;

	InputIDs.clear();
	InputList.clear();
}

/// Constructor

/// Initializes a new instance of the Region class.
///
/// colGridSize: Point structure that represents the 
///     region size in columns (ColumnSizeX * ColumnSizeY).
/// pctInputPerCol: The percent of input bits each column has potential
///     synapses for (PctInputCol).
/// pctMinOverlap: The minimum percent of column's proximal synapses
///     that must be active for a column to be even be considered during spatial 
///     pooling.
/// predictionRadius: Furthest number of columns away to allow
///     distal synapses from each column/cell.
/// pctLocalActivity: Approximate percent of Columns within inhibition
///     radius to be winners after spatial inhibition.
/// maxBoost: The maximum allowed Boost value. Setting a max Boost value prevents 
///     competition for activation from continuing indefinitely, and so 
///     facilitates stabilization of representations.
/// boostPeriod: The time period in which Boosting may occur (-1 for no limit).
/// cellsPerCol: The number of context learning cells per column.
/// segActiveThreshold: The minimum number of synapses that must be 
///     active for a segment to fire.
/// newSynapseCount: The number of new distal synapses added if
///     no matching ones found during learning.
/// hardcodedSpatial: If set to true, this Region must have exactly one
///     input, with the same dimensions as this Region. What is active in 
///     that input will directly dictate what columns are activated in this
///     Region, with the spatial pooling and inhibition steps skipped.
/// spatialLearning: If true, spatial learning takes place as normal.
/// temporalLearning: If true, temporal learning takes place as normal.
/// outputColumnActivity: If true, there will be one value in this Region's
///     output representing activity of each column as a whole.
/// outputCellActivity: If true, there will be one value in this Region's
///     output representaing activity of each cell in each column.
///
/// Prior to receiving any inputs, the region is initialized by computing a list of 
/// initial potential synapses for each column. This consists of a random set of 
/// inputs selected from the input space. Each input is represented by a synapse 
/// and assigned a random permanence value. The random permanence values are chosen 
/// with two criteria. First, the values are chosen to be in a small range around 
/// connectedPerm (the minimum permanence value at which a synapse is considered 
/// "connected"). This enables potential synapses to become connected (or 
/// disconnected) after a small number of training iterations. Second, each column 
/// has a natural center over the input region, and the permanence values have a bias
/// towards this center (they have higher values near the center).
/// 
/// In addition to this Uwe added a concept of Input Radius, which is an 
/// additional parameter to control how far away synapse connections can be made 
/// instead of allowing connections anywhere.  The reason for this is that in the
/// case of video images I wanted to experiment with forcing each Column to only
/// learn on a small section of the total input to more effectively learn lines or 
/// corners in a small section without being 'distracted' by learning larger patterns
/// in the overall input space (which hopefully higher hierarchical Regions would 
/// handle more successfully).  Passing in -1 for input radius will mean no 
/// restriction which will more closely follow the Numenta doc if desired.
Region::Region(NetworkManager *manager, QString &_id, Point colGridSize, int hypercolumnDiameter, SynapseParameters proximalSynapseParams, SynapseParameters distalSynapseParams, float pctInputPerCol, float pctMinOverlap, int predictionRadius, InhibitionTypeEnum inhibitionType, int inhibitionRadius, float pctLocalActivity, float boostRate, float maxBoost, int spatialLearningStartTime, int spatialLearningEndTime, int temporalLearningStartTime, int temporalLearningEndTime, int boostingStartTime, int boostingEndTime, int cellsPerCol, int segActiveThreshold, int newSynapseCount, int min_MinOverlapToReuseSegment, int max_MinOverlapToReuseSegment, bool hardcodedSpatial, bool outputColumnActivity, bool outputCellActivity)
	: DataSpace(_id)
{
	//this.Predictions = new BindingList<Prediction>();

	Manager = manager;
	Width = (int)(colGridSize.X);
	Height = (int)(colGridSize.Y);
	HypercolumnDiameter = hypercolumnDiameter;
	ProximalSynapseParams = proximalSynapseParams;
	DistalSynapseParams = distalSynapseParams;
	PredictionRadius = predictionRadius;
	InhibitionType = inhibitionType;
	InhibitionRadius = inhibitionRadius;
	BoostRate = boostRate;
	MaxBoost = maxBoost;
	SpatialLearningStartTime = spatialLearningStartTime;
	SpatialLearningEndTime = spatialLearningEndTime;
	TemporalLearningStartTime = temporalLearningStartTime;
	TemporalLearningEndTime = temporalLearningEndTime;
	BoostingStartTime = boostingStartTime;
	BoostingEndTime = boostingEndTime;
	CellsPerCol = cellsPerCol;
	SegActiveThreshold = segActiveThreshold;
	NewSynapsesCount = newSynapseCount;
	Min_MinOverlapToReuseSegment = min_MinOverlapToReuseSegment;
	Max_MinOverlapToReuseSegment = max_MinOverlapToReuseSegment;
	PctLocalActivity = pctLocalActivity;
	PctInputPerColumn = pctInputPerCol;
	PctMinOverlap = pctMinOverlap;
	HardcodedSpatial = hardcodedSpatial; 
	OutputColumnActivity = outputColumnActivity;
	OutputCellActivity = outputCellActivity;

	// Determine number of output values.
	NumOutputValues = (OutputColumnActivity ? 1 : 0) + (OutputCellActivity ? CellsPerCol : 0);
	
	// Create the columns based on the size of the input data to connect to.
	int minOverlapToReuseSegment;
	Columns = new Column*[Width * Height];
	for (int cy = 0; cy < Height; cy++)
	{
		for (int cx = 0; cx < Width; cx++)
		{
			// Determine the current column's minOverlapToReuseSegment, a random value within this Region's range.
			minOverlapToReuseSegment = rand() % (Max_MinOverlapToReuseSegment - Min_MinOverlapToReuseSegment + 1) + Min_MinOverlapToReuseSegment;

			// Create a column with sourceCoords and GridCoords
			Columns[(cy * Width) + cx] = new Column(this, Point(cx, cy), minOverlapToReuseSegment);
		}
	}
}

/// Methods

// DataSpace Methods

bool Region::GetIsActive(int _x, int _y, int _index)
{
	_ASSERT((_x >= 0) && (_x < Width));
	_ASSERT((_y >= 0) && (_y < Height));
	_ASSERT((_index >= 0) && (_index < NumOutputValues));

	Column *col = Columns[_x + (_y * Width)];

	if (OutputColumnActivity && (_index == (NumOutputValues - 1)))
	{
		// Return value representing activity of the entire column _x, _y.
		// NOTE: If this will ever be used, should it also return true if any cell in the column is in predictive state?
		return col->GetIsActive();
	}
	else
	{
		// Return value representing activity of the column's cell with the given _index (active or predictive state).
		Cell *cell = col->GetCellByIndex(_index);
		return cell->GetIsActive() || cell->GetIsPredicting();
	}
}

// Add an input DataSpace to this Region
void Region::AddInput(DataSpace *_inputDataSpace)
{
	// If HardcodedSpatial is true, then there can be only one input DataSpace, and it must be the same size as this Region, and have only 1 value.
	_ASSERT((HardcodedSpatial == false) || ((InputList.size() == 0) && (_inputDataSpace->GetNumValues() == 1) && (_inputDataSpace->GetSizeX() == Width) && (_inputDataSpace->GetSizeY() == Height)));

	InputList.push_back(_inputDataSpace);
}

bool Region::IsCellActive(int _x, int _y, int _index)
{
	_ASSERT((_x >= 0) && (_x < Width));
	_ASSERT((_y >= 0) && (_y < Height));
	
	Column *col = Columns[_x + (_y * Width)];

	// Return value representing activity of the column's cell with the given _index.
	return col->GetCellByIndex(_index)->GetIsActive();
}

bool Region::IsCellPredicted(int _x, int _y, int _index)
{
	_ASSERT((_x >= 0) && (_x < Width));
	_ASSERT((_y >= 0) && (_y < Height));
	
	Column *col = Columns[_x + (_y * Width)];

	// Return value representing prediction of the column's cell with the given _index.
	return col->GetCellByIndex(_index)->GetIsPredicting();
}

bool Region::IsCellLearning(int _x, int _y, int _index)
{
	_ASSERT((_x >= 0) && (_x < Width));
	_ASSERT((_y >= 0) && (_y < Height));
	
	Column *col = Columns[_x + (_y * Width)];

	// Return value representing prediction of the column's cell with the given _index.
	return col->GetCellByIndex(_index)->GetIsLearning();
}

Cell *Region::GetCell(int _x, int _y, int _index)
{
	_ASSERT((_x >= 0) && (_x < Width));
	_ASSERT((_y >= 0) && (_y < Height));

	Column *col = Columns[_x + (_y * Width)];

	// Return pointer to the given column's cell with the given _index.
	return col->GetCellByIndex(_index);
}

// Called after adding all inputs to this Region.
void Region::Initialize()
{
	if (HardcodedSpatial == false)
	{
		// Create Segments with potential synapses for columns
		for (int i = 0; i < Width * Height; i++)
		{
			Columns[i]->CreateProximalSegments(InputList, InputRadii);
		}

		if (InhibitionType == INHIBITION_TYPE_AUTOMATIC)
		{
			// Initialize InhibitionRadius based on the average receptive field size.
			InhibitionRadius = AverageReceptiveFieldSize();
		}

		// Determine the DesiredLocalActivity value for each Column, based on InhibitionRadius.
		for (int ColIndex = 0; ColIndex < Width * Height; ColIndex++) {
			Columns[ColIndex]->DetermineDesiredLocalActivity();
		}
	}

	InitializeStatisticParameters();
}

/// Performs spatial pooling for the current input in this Region.
///
/// The result will be a subset of Columns being set as active as well
/// as (proximal) synapses in all Columns having updated permanences and boosts, and 
/// the Region will update inhibitionRadius.
/// From the Numenta white paper:
/// Phase 1: 
///     Compute the overlap with the current input for each column. Given an input 
///     vector, the first phase calculates the overlap of each column with that 
///     vector. The overlap for each column is simply the number of connected 
///     synapses with active inputs, multiplied by its boost. If this value is 
///     below minOverlap, we set the overlap score to zero.
/// Phase 2: 
///     Compute the winning columns after inhibition. The second phase calculates
///     which columns remain as winners after the inhibition step. 
///     desiredLocalActivity is a parameter that controls the number of columns 
///     that end up winning. For example, if desiredLocalActivity is 10, a column 
///     will be a winner if its overlap score is greater than the score of the 
///     10'th highest column within its inhibition radius.
/// Phase 3: 
///     Update synapse permanence and internal variables.The third phase performs 
///     learning; it updates the permanence values of all synapses as necessary, 
///     as well as the boost and inhibition radius. The main learning rule is 
///     implemented in lines 20-26. For winning columns, if a synapse is active, 
///     its permanence value is incremented, otherwise it is decremented. Permanence 
///     values are constrained to be between 0 and 1.
///     Lines 28-36 implement boosting. There are two separate boosting mechanisms 
///     in place to help a column learn connections. If a column does not win often 
///     enough (as measured by activeDutyCycle), its overall boost value is 
///     increased (line 30-32). Alternatively, if a column's connected synapses do 
///     not overlap well with any inputs often enough (as measured by 
///     overlapDutyCycle), its permanence values are boosted (line 34-36). 
///     
/// Note: Once learning is turned off, boost(c) is frozen. 
/// Finally at the end of Phase 3 the inhibition radius is recomputed (line 38).
void Region::PerformSpatialPooling()
{
	int ColIndex;
	Column *col;

	if (HardcodedSpatial)
	{
		// This Region should have at most one DataSpace, which should be the same size as this Region and have one value per column.
		DataSpace *inputDataSpace = InputList.front();
		if (inputDataSpace && (inputDataSpace->GetSizeX() == GetSizeX()) && (inputDataSpace->GetSizeY() == GetSizeY()))
		{
			for (ColIndex = 0; ColIndex < Width * Height; ColIndex++)
			{
				col = Columns[ColIndex];
				col->SetIsActive(inputDataSpace->GetIsActive(col->Position.X, col->Position.Y, 0) == 1);
			}
		}
		return;
	}

	// Determine whether spatial learning is currently allowed.
	bool allowSpatialLearning= ((GetSpatialLearningStartTime() == -1) || (GetSpatialLearningStartTime() <= GetStepCounter())) &&
		                         ((GetSpatialLearningEndTime() == -1) || (GetSpatialLearningEndTime() >= GetStepCounter()));

	// Determine whether boosting is currently allowed.
	bool allowBoosting= ((GetBoostingStartTime() == -1) || (GetBoostingStartTime() <= GetStepCounter())) &&
		                  ((GetBoostingEndTime() == -1) || (GetBoostingEndTime() >= GetStepCounter()));

	// Phase 1: Compute Input Overlap
	for (ColIndex = 0; ColIndex < Width * Height; ColIndex++)
	{
		Columns[ColIndex]->ComputeOverlap();
	}

	// Phase 2: Compute active columns (Winners after inhibition)
	for (ColIndex = 0; ColIndex < Width * Height; ColIndex++)
	{
		Columns[ColIndex]->ComputeColumnInhibition();
	}

	// Phase 3: Synapse Learning and Determining Boosting
	for (ColIndex = 0; ColIndex < Width * Height; ColIndex++)
	{
		col = Columns[ColIndex];
		
		if (allowSpatialLearning && col->GetIsActive())
		{
			col->AdaptPermanences();
		}

		col->UpdateDutyCycles();

		if (allowBoosting) 
		{
			col->PerformBoosting();
		}
	}

	if (allowSpatialLearning && (InhibitionType == INHIBITION_TYPE_AUTOMATIC)) 
	{
		// Determine the new InhibitionRadius value based on average receptive field size.
		InhibitionRadius = AverageReceptiveFieldSize();

		// Determine the new DesiredLocalActivity value for each column.
		for (ColIndex = 0; ColIndex < Width * Height; ColIndex++) {
			Columns[ColIndex]->DetermineDesiredLocalActivity();
		}
	}
}

/// Performs temporal pooling based on the current spatial pooler output.
///
/// From the Numenta white paper:
/// The input to this code is activeColumns(t), as computed by the spatial pooler. 
/// The code computes the active and predictive state for each cell at the current 
/// timestep, t. The boolean OR of the active and predictive states for each cell 
/// forms the output of the temporal pooler for the next level.
/// 
/// Phase 1: 
/// Compute the active state, activeState(t), for each cell.
/// The first phase calculates the activeState for each cell that is in a winning 
/// column. For those columns, the code further selects one cell per column as the 
/// learning cell (learnState). The logic is as follows: if the bottom-up input was 
/// predicted by any cell (i.e. its predictiveState output was 1 due to a sequence 
/// segmentUpdateList), then those cells become active (lines 23-27). 
/// If that segmentUpdateList became active from cells chosen with learnState on, 
/// this cell is selected as the learning cell (lines 28-30). If the bottom-up input 
/// was not predicted, then all cells in the column become active (lines 32-34). 
/// In addition, the best matching cell is chosen as the learning cell (lines 36-41) 
/// and a new segmentUpdateList is added to that cell.
/// 
/// Phase 2:
/// Compute the predicted state, predictiveState(t), for each cell.
/// The second phase calculates the predictive state for each cell. A cell will turn 
/// on its predictive state output if one of its segments becomes active, i.e. if 
/// enough of its lateral inputs are currently active due to feed-forward input. 
/// In this case, the cell queues up the following changes: a) reinforcement of the 
/// currently active segmentUpdateList (lines 47-48), and b) reinforcement of a 
/// segmentUpdateList that could have predicted this activation, i.e. a 
/// segmentUpdateList that has a (potentially weak) match to activity during the 
/// previous time step (lines 50-53).
/// Phase 3:
/// Update synapses. The third and last phase actually carries out learning. In this 
/// phase segmentUpdateList updates that have been queued up are actually implemented 
/// once we get feed-forward input and the cell is chosen as a learning cell 
/// (lines 56-57). Otherwise, if the cell ever stops predicting for any reason, we 
/// negatively reinforce the segments (lines 58-60).
void Region::PerformTemporalPooling()
{
	// Phase1
	// 18. for c in activeColumns(t)
	// 19.
	// 20.   buPredicted = false
	// 21.   lcChosen = false
	// 22.   for i = 0 to cellsPerColumn - 1
	// 23.     if predictiveState(c, i, t-1) == true then
	// 24.       s = getActiveSegment(c, i, t-1, activeState)
	// 25.       if s.sequenceSegment == true then
	// 26.         buPredicted = true
	// 27.         activeState(c, i, t) = 1
	// 28.         if segmentActive(s, t-1, learnState) then
	// 29.           lcChosen = true
	// 30.           learnState(c, i, t) = 1
	// 31.
	// 32.   if buPredicted == false then
	// 33.     for i = 0 to cellsPerColumn - 1
	// 34.       activeState(c, i, t) = 1
	// 35.
	// 36.   if lcChosen == false then
	// 37.     i,s = getBestMatchingCell(c, t-1)
	// 38.     learnState(c, i, t) = 1
	// 39.     sUpdate = getSegmentActiveSynapses (c, i, s, t-1, true)
	// 40.     sUpdate.sequenceSegment = true
	// 41.     segmentUpdateList.add(sUpdate)

	bool predicted, learnCellChosen;
	int ColIndex;
	Column *col;
	FastListIter segments_iter;
	Cell *cell, *bestCell;
	Segment *segment, *bestSegment, *seg, *predictiveSegment;
	SegmentUpdateInfo *segmentUpdateInfo, *predictiveSegUpdate;

	// Determine whether temporal learning is currently allowed.
	bool temporalLearning= ((GetTemporalLearningStartTime() == -1) || (GetTemporalLearningStartTime() <= GetStepCounter())) &&
		                     ((GetTemporalLearningEndTime() == -1) || (GetTemporalLearningEndTime() >= GetStepCounter()));

	// Phase 1: Compute cell active states and segment learning updates
	for (ColIndex = 0; ColIndex < Width * Height; ColIndex++)
	{
		col = Columns[ColIndex];

		if (col->IsActive)
		{
			predicted = false;
			learnCellChosen = false;
			
			for (int cellIndex = 0; cellIndex < GetCellsPerCol(); cellIndex++)
			{
				cell = col->Cells[cellIndex];
	
				if (cell->GetWasPredicted())
				{
					segment = cell->GetPreviousActiveSegment();
					if ((segment != NULL) && segment->GetIsSequence())
					{
						predicted = true;
						cell->SetIsActive(true);
						if (TemporalLearning && segment->GetWasActiveFromLearning())
						{
							learnCellChosen = true;
							cell->SetIsLearning(true);
						}
					}
				}
			}

			// TESTING
			if (id == QString("Region1"))
			{
				if ((col->Position.X == 32) && (col->Position.Y == 22)) 
				{
					QString stepString;
					stepString.setNum(GetStepCounter());
					Manager->WriteToLog(QString("Time ") + stepString + QString(": Cell 32,22,0 active.") + (predicted ? QString("Predicted.") : QString("")));
				}
				if ((col->Position.X == 32) && (col->Position.Y == 23)) 
				{
					QString stepString;
					stepString.setNum(GetStepCounter());
					Manager->WriteToLog(QString("Time ") + stepString + QString(": Cell 32,23,0 active.") + (predicted ? QString("Predicted.") : QString("")));
				}
			}

			if (!predicted)
			{
				for (int cellIndex = 0; cellIndex < GetCellsPerCol(); cellIndex++)
				{
					cell = col->Cells[cellIndex];
					cell->SetIsActive(true);
				}
			}

			if (temporalLearning && !learnCellChosen)
			{
				// isSequence=true, previous=true
				col->GetBestMatchingCell(1, true, bestCell, bestSegment);
				_ASSERT(bestCell != NULL);

				bestCell->SetIsLearning(true);

				// segmentToUpdate is added internally to the bestCell's update list.
				// Then set number of prediction steps to 1 (meaning it's a sequence segment)
				segmentUpdateInfo = bestCell->UpdateSegmentActiveSynapses(true, bestSegment, true, UPDATE_DUE_TO_ACTIVE);
				segmentUpdateInfo->SetNumPredictionSteps(1);
			}
		}
	}
	
	// Phase2
	// 42. for c, i in cells
	// 43.   for s in segments(c, i)
	// 44.     if segmentActive(s, t, activeState) then
	// 45.       predictiveState(c, i, t) = 1
	// 46.
	// 47.       activeUpdate = getSegmentActiveSynapses (c, i, s, t, false)
	// 48.       segmentUpdateList.add(activeUpdate)
	// 49.
	// 50.       predSegment = getBestMatchingSegment(c, i, t-1)
	// 51.       predUpdate = getSegmentActiveSynapses(
	// 52.                                   c, i, predSegment, t-1, true)
	// 53.       segmentUpdateList.add(predUpdate)
	for (ColIndex = 0; ColIndex < Width * Height; ColIndex++)
	{
		col = Columns[ColIndex];

		for (int cellIndex = 0; cellIndex < GetCellsPerCol(); cellIndex++)
		{
			cell = col->Cells[cellIndex];
	
			// Process all segments on the cell to cache the activity for later.
			segments_iter.SetList(cell->Segments);
			for (seg = (Segment*)(segments_iter.Reset()); seg != NULL; seg = (Segment*)(segments_iter.Advance()))
			{
				seg->ProcessSegment();
			}

			segments_iter.SetList(cell->Segments);
			for (seg = (Segment*)(segments_iter.Reset()); seg != NULL; seg = (Segment*)(segments_iter.Advance()))
			{
				// Now check for an active segment, we only need one for the cell to predict, but all Segments need to be checked
				// so that a segment update will be created for each active segment, and so that the lowest numPredictionSteps 
				// among active segments is adopted by the cell.
				if (seg->GetIsActive())
				{
					cell->SetIsPredicting(true, seg->GetNumPredictionSteps());

					if (seg->GetIsSequence())
					{
						cell->SetIsSegmentPredicting(true);
					}

					// a) reinforcement of the currently active segments
					if (temporalLearning)
					{
						// Add segment update to this cell
						cell->UpdateSegmentActiveSynapses(false, seg, false, UPDATE_DUE_TO_PREDICTIVE);
					}
				}
			}

			// b) reinforcement of a segment that could have predicted 
			//    this activation, i.e. a segment that has a (potentially weak)
			//    match to activity during the previous time step (lines 50-53).
			// NOTE: The check against MaxTimeSteps is a correctly functioning way of enforcing a maximum number of time steps, 
			// as opposed to the previous way of storing Max(numPredictionSteps, MaxTimeSteps) as a segment's numPredictionSteps,
			// which caused inaccurate numPredictionSteps values to be stored, resulting in duplicate segments being created.
			// Note also that if the system of recording and using an exact number of time steps is abandonded (and replaced with the
			// original sequence/non-sequence system), then all references to MaxTimeSteps can be removed.
			if (temporalLearning && cell->GetIsPredicting() && (cell->GetNumPredictionSteps() != MaxTimeSteps))
			{
				predictiveSegment = cell->GetBestMatchingPreviousSegment();

				// Either update existing or add new segment for this cell considering
				// only segments matching the number of prediction steps of the
				// best currently active segment for this cell.
				predictiveSegUpdate = cell->UpdateSegmentActiveSynapses(true, predictiveSegment, true, UPDATE_DUE_TO_PREDICTIVE);
				if (predictiveSegment == NULL)
				{
					predictiveSegUpdate->NumPredictionSteps = cell->GetNumPredictionSteps() + 1;
				}
			}
		}
	}

	// Phase3
	// 54. for c, i in cells
	// 55.   if learnState(c, i, t) == 1 then
	// 56.     adaptSegments (segmentUpdateList(c, i), true)
	// 57.     segmentUpdateList(c, i).delete()
	// 58.   else if predictiveState(c, i, t) == 0 and predictiveState(c, i, t-1)==1 then
	// 59.     adaptSegments (segmentUpdateList(c,i), false)
	// 60.     segmentUpdateList(c, i).delete()
	if (temporalLearning)
	{
		for (ColIndex = 0; ColIndex < Width * Height; ColIndex++)
		{
			col = Columns[ColIndex];

			for (int cellIndex = 0; cellIndex < GetCellsPerCol(); cellIndex++)
			{
				cell = col->Cells[cellIndex];
	
				if (cell->GetIsLearning())
				{
					cell->ApplySegmentUpdates(StepCounter, AUT_ACTIVE);
				}
				else if ((cell->GetIsPredicting() == false) && cell->GetWasPredicted())
				{
					cell->ApplySegmentUpdates(StepCounter, AUT_INACTIVE);
				}
				else if ((cell->GetIsPredicting() == true) && (cell->GetWasPredicted() == true) && (cell->GetNumPredictionSteps() > 1) && (cell->GetPrevNumPredictionSteps() == 1))
				{
					cell->ApplySegmentUpdates(StepCounter, AUT_LONGER_PREDICTION);
				}
			}
		}
	}
}

/// Get a reference to the Column at the specified column grid coordinate.
///
/// x: the x coordinate component of the column's position.
/// y: the y coordinate component of the column's position.
/// returns: a pointer to the Column at that position.
Column *Region::GetColumn(int x, int y)
{
	return Columns[(y * Width) + x];
}

/// The radius of the average connected receptive field size of all the columns. 
/// 
/// returns: The average connected receptive field size (in hypercolumn grid space).
/// 
/// The connected receptive field size of a column includes only the connected 
/// synapses (those with permanence values >= connectedPerm). This is used to 
/// determine the extent of lateral inhibition between columns.
float Region::AverageReceptiveFieldSize()
{
	float maxDistance, sum = 0.0;
	Column *col;
	FastListIter synapses_iter;

	for (int ColIndex = 0; ColIndex < Width * Height; ColIndex++)
	{
		col = Columns[ColIndex];

		// Initialize maxIstance to 0.
		maxDistance = 0.0f;

		// Iterate through each of the current column's proximal synapses...
		synapses_iter.SetList(col->ProximalSegment->Synapses);
		for (Synapse *syn = (Synapse*)(synapses_iter.Reset()); syn != NULL; syn = (Synapse*)(synapses_iter.Advance()))
		{
			// Skip non-connected synapses.
			if (syn->GetIsConnected() == false) {
				continue;
			}

			// Determine the distance of the further proximal synapse. This will be considered the size of the receptive field.
			maxDistance = Max(maxDistance, ((ProximalSynapse*)syn)->DistanceToInput);
		}

		// Add the current column's receptive field size to the sum.
		sum += maxDistance;
	}

	// Return this Region's average receptive field size.
	return (sum / (float)(Width * Height));
}

/// Return true if the given Column has an overlap value that is at least the
/// k'th largest amongst all neighboring columns within inhibitionRadius hypercolumns around the given Column's hypercolumn.
///
/// This function is effectively determining which columns are to be inhibited 
/// during the spatial pooling procedure of the region.
bool Region::IsWithinKthScore(Column *col, int k)
{
	// Determine the extent, in columns, of the area within the InhibitionRadius hypercolumns of the given col's hypercolumn.
	Area inhibitionArea = col->DetermineColumnsWithinHypercolumnRadius(InhibitionRadius + 0.5f);

	//Loop over all columns that are within inhibitionRadius hypercolumns of the given column's hypercolumn.
	// Count how many neighbor columns have strictly greater overlap than our
	// given column. If this count is <k then we are within the kth score
	int numberColumns = 0;
	for (int x = inhibitionArea.MinX; x <= inhibitionArea.MaxX; x++)
	{
		for (int y = inhibitionArea.MinY; y <= inhibitionArea.MaxY; y++)
		{
			numberColumns += (Columns[(y * Width) + x]->GetOverlap() > col->GetOverlap()) ? 1 : 0;
		}
	}
	return numberColumns < k; //if count is < k, we are within the kth score of all neighbors
}

/// Run one time step iteration for this Region.
///
/// All cells will have their current (last run) state pushed back to be their 
/// new previous state and their new current state reset to no activity.  
/// Then SpatialPooling followed by TemporalPooling is performed for one time step.
void Region::Step()
{
	Column *col;
	Cell *cell;

	for (int ColIndex = 0; ColIndex < Width * Height; ColIndex++)
	{
		col = Columns[ColIndex];

		col->ProximalSegment->NextTimeStep();

		for (int cellIndex = 0; cellIndex < GetCellsPerCol(); cellIndex++)
		{
			cell = col->Cells[cellIndex];
			cell->NextTimeStep();
		}
	}

	// Compute Region statistics
	ComputeBasicStatistics();

	// Perform pooling
	PerformSpatialPooling();
	PerformTemporalPooling();

	// Determine accuracy of patterns matched by proximal segments.
	ComputeColumnAccuracy();
}

/// Statistics

/// Sets statistics values to 0.
void Region::InitializeStatisticParameters()
{
	StepCounter = 0;
}

/// Updates statistics values.
void Region::ComputeBasicStatistics()
{
	StepCounter++;
}

void Region::ComputeColumnAccuracy()
{
	Column *col;
	FastListIter segments_iter;

	if ((GetStepCounter() % 1000) == 1)
	{
		if (GetStepCounter() > 1)
		{
			// Determine avergage missing and extra synapses over the past 1000 time steps.
			float avgMissingSynapses = (float)fd_missingSynapesCount / (float)fd_numActiveCols;
			float avgExtraSynapses = (float)fd_extraSynapsesCount / (float)fd_numActiveCols;

			// Log results.
			QString stepString, avgMissingSynapsesString, avgExtraSynapsesString;
			stepString.setNum(GetStepCounter());
			avgMissingSynapsesString.setNum(avgMissingSynapses);
			avgExtraSynapsesString.setNum(avgExtraSynapses);
			Manager->WriteToLog(QString("Time ") + stepString + QString(" feature accuracy: Missing synapses: ") + avgMissingSynapsesString + QString(", Extra synapses: ") + avgExtraSynapsesString + QString("."));
		}

		fd_numActiveCols = 0;
		fd_missingSynapesCount = 0;
		fd_extraSynapsesCount = 0;
	}

	for (int ColIndex = 0; ColIndex < Width * Height; ColIndex++)
	{
		col = Columns[ColIndex];

		if (col->GetIsActive())
		{
			fd_numActiveCols++;

			FastListIter synapses_iter(col->ProximalSegment->Synapses);
			for (Synapse *syn = (Synapse*)(synapses_iter.Reset()); syn != NULL; syn = (Synapse*)(synapses_iter.Advance()))
			{
				if (syn->GetIsActive() && (syn->GetIsConnected() == false)) {
					fd_missingSynapesCount++;
				}
				else if ((syn->GetIsActive() == false) && (syn->GetIsConnected())) {
					fd_extraSynapsesCount++;
				}
			}
		}
	}
}