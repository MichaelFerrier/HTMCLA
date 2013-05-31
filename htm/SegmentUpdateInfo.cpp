#include <stdlib.h>
#include "SegmentUpdateInfo.h"
#include "Utils.h"
#include "Cell.h"
#include "Segment.h"
#include "Column.h"
#include "Region.h"

SegmentUpdateInfo::SegmentUpdateInfo(void)
{
}

SegmentUpdateInfo::~SegmentUpdateInfo(void)
{
}

void SegmentUpdateInfo::Retire()
{
	ActiveDistalSynapses.Clear();
	CellsThatWillLearn.Clear();
}

/// Randomly sample m values from the Cell array of length n (m less than n).
/// Runs in O(2m) worst case time.  Result is written to the result
/// array of length m containing the randomly chosen cells.
/// 
/// cells: input Cells to randomly choose from.
/// result: the resulting random subset of Cells.
/// m: the number of random samples to take (m less than equal to result.Length)
void SegmentUpdateInfo::RandomSample(FastList &cells, FastList &result, int numberRandomSamples)
{
	int n = cells.Count();
	Cell *cell;
	for (int i = n - numberRandomSamples; i < n; ++i)
	{
		int pos = rand() % (i + 1);
		cell = (Cell*)(cells.GetByIndex(pos));

		//if(subset ss contains item already) then use item[i] instead
		bool contains = result.IsInList(cell);
		result.InsertAtEnd(contains ? (Cell*)(cells.GetByIndex(i)) : cell);
	}
}

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
void SegmentUpdateInfo::Initialize(Cell *_cell, Segment *_segment, FastList *activeDistalSynapses, bool addNewSynapses, int _creationTimeStep, UpdateType _updateType)
{
	Cell *curCell;

	// BMK Essential for temporal learning. Details specified 
	cell = _cell;
	segment = _segment;
	AddNewSynapses = addNewSynapses;
	CreationTimeStep = _creationTimeStep;
	updateType = _updateType;
	NumPredictionSteps = 1;

	if (activeDistalSynapses != NULL) {
		activeDistalSynapses->CopyContentsTo(ActiveDistalSynapses);
	}
		
	// Once synapses added, store here to visualize later
	// this.AddedSynapses = new List<DistalSynapse>();

	FastList learningCells; //capture learning cells at this time step

	// If adding new synapses, find the current set of learning cells within
	// the Region and select a random subset of them to connect the segment to.
	// Do not add >1 synapse to the same cell on a given segment
	Region *region = cell->GetColumn()->GetRegion();
	if (AddNewSynapses)
	{
		FastList segCells;

		if (segment != NULL)
		{
			// Fill the segCells list with each of the segment's existing synapses' input cells. 
			FastListIter synapses_iter(segment->Synapses);
			for (Synapse *syn = (Synapse*)(synapses_iter.Reset()); syn != NULL; syn = (Synapse*)(synapses_iter.Advance()))
			{
				segCells.InsertAtEnd(((DistalSynapse*)syn)->GetInputSource());
			}
		}

		// Only allow connecting to Columns within prediction radius
		Column *cellColumn = cell->GetColumn();

		int minY, maxY, minX, maxX;

		// Determine bounds of Columns that we may make synapses with, those within the PredictionRadius.
		// If prediction radius is -1, it means 'no restriction'
		if (region->PredictionRadius > -1)
		{
			Area predictionArea = cellColumn->DetermineColumnsWithinHypercolumnRadius(region->PredictionRadius);
			minX = predictionArea.MinX;
			minY = predictionArea.MinY;
			maxX = predictionArea.MaxX;
			maxY = predictionArea.MaxY;
		}
		else
		{
			minY = 0;
			maxY = region->Height - 1;
			minX = 0;
			maxX = region->Width - 1;
		}

		for (int x = minX; x <= maxX; x++)
		{
			for (int y = minY; y <= maxY; y++)
			{
				// Get the current column at position x,y.
				Column *col = region->GetColumn(x, y);

				// NOTE: There is no indication in the Numenta pseudocode that a cell shouldn't be able to have a 
				// distal synapse from another cell in the same column. Therefore the below check is commented out.
				//// Skip cells in our own col (don't connect to ourself)
				//if (col == cell->GetColumn()) {
				//	continue;
				//}

				// Add each of the col's cells that WasLearning, and that is not in the segCells list, to the learningCells list.
				for (int cellIndex = 0; cellIndex < region->GetCellsPerCol(); cellIndex++)
				{
					curCell = col->Cells[cellIndex];

					if (curCell->GetWasLearning() && (!segCells.IsInList(curCell))) {
						learningCells.InsertAtEnd(curCell);
					}
				}
			}
		}

		segCells.Clear();
	}

	// Basic allowed number of new Synapses
	// TODO: Move this up so that if newSynCount is 0, the above compiling of a list of learning cells won't be done.
	int newSynCount = region->NewSynapsesCount;
	if (segment != NULL)
	{
		newSynCount = Max(0, newSynCount - ((activeDistalSynapses == NULL) ? 0 : activeDistalSynapses->Count()));
	}

	int numberLearningCells = learningCells.Count();

	// Clamp at -- of learn cells
	newSynCount = Min(numberLearningCells, newSynCount);

	// Randomly choose synCount learning cells to add connections to
	if ((numberLearningCells > 0) && (newSynCount > 0))
	{
		FastList result;
		RandomSample(learningCells, result, newSynCount);
		result.TransferContentsTo(CellsThatWillLearn);
	}

	learningCells.Clear();
}

/// Create a new segment on the update cell using connections from
/// the set of learning cells for the update info.
Segment *SegmentUpdateInfo::CreateCellSegment(int time)
{
	Segment *newSegment = cell->CreateSegment(CellsThatWillLearn, time);
	//segment.getSynapses(_addedSynapses);//if UI wants to display added synapses
	newSegment->SetNumPredictionSteps(NumPredictionSteps);
	return newSegment;
}

/// Create new synapse connections to the segment to be updated using
/// the set of learning cells in this update info.
void SegmentUpdateInfo::CreateSynapsesToLearningCells(SynapseParameters *params)
{
	segment->CreateSynapsesToLearningCells(CellsThatWillLearn, params);
}