#include <crtdbg.h>
#include <stdlib.h>
#include <random>
#include <limits.h>
#include "Cell.h"
#include "Segment.h"
#include "Synapse.h"
#include "Region.h"
#include "InputSpace.h"
#include "Utils.h"

#include "Column.h"

extern MemManager mem_manager;

Column::~Column(void)
{
	// Release proximal segment.
	mem_manager.ReleaseObject(ProximalSegment);

	// Release all Cells.
	for (int i = 0; i < region->GetCellsPerCol(); i++) {
		mem_manager.ReleaseObject(Cells[i]);
	}

	delete [] Cells;
}

// Methods

/// Initializes a new Column for the given parent Region at source 
/// row/column position srcPos and column grid position pos.
/// 
/// region: The parent Region this Column belongs to.
/// srcPos: A Point (srcX,srcY) of this Column's 'center' position in 
///   terms of the proximal-synapse input space.
/// pos: A Point(x,y) of this Column's position within the Region's 
///   column grid.
Column::Column(Region *_region, Point pos, int minOverlapToReuseSegment)
{
	region = _region;

	IsActive = false;
	IsInhibited = false;
	ActiveDutyCycle = 1;
	FastActiveDutyCycle = 1;
	_overlapDutyCycle = 1.0f;
	maxDutyCycle = 0.0f;
	Overlap = 0;
	prevBoostTime = 0;
	DesiredLocalActivity = 0;
	MinOverlapToReuseSegment = minOverlapToReuseSegment;

	// Determine initial random low Boost value, just to break ties between columns with the same amount of overlap.
	// The initial Boost value is set to be the same as this Column's MinBoost value.
	Boost = MinBoost = 1.0f + ((float)rand() / (float)RAND_MAX) * BoostVariance;

	// Determine this Column's MaxBoost value, with random variation to avoid ties between fully boosted Columns.
	MaxBoost = (region->GetMaxBoost() == -1) ? -1 : region->GetMaxBoost() - ((float)rand() / (float)RAND_MAX) * BoostVariance;

	// Create each of this Column's Cells.
	Cells = new Cell*[region->CellsPerCol];
	Cell *newCell;
	for (int i = 0; i < region->GetCellsPerCol(); i++)
	{
		newCell = (Cell*)(mem_manager.GetObject(MOT_CELL));
		newCell->Initialize(this, i);
		Cells[i] = newCell;
	}

	// The list of potential proximal synapses and their permanence values.
	ProximalSegment = (Segment*)(mem_manager.GetObject(MOT_SEGMENT));
	ProximalSegment->Initialize(0, (float)(region->SegActiveThreshold));

	// Record Position and determine HypercolumnPosition.
	SetPosition(pos);
}

void  Column::SetPosition(Point value) 
{
	Position = value; 
	HypercolumnPosition = Point((int)(value.X / region->GetHypercolumnDiameter()), (int)(value.Y / region->GetHypercolumnDiameter()));
}

/// For each input DataSpace:
///   For each (position in inputSpaceRandomPositions): 
///     Create a new ProximalSynapse corresponding to the random sample's X, Y, and index values.
///
/// inputRadii
/// One value per input DataSpace.
/// Limit a section of the total input to more effectively learn lines or corners in a
/// small section without being 'distracted' by learning larger patterns in the overall
/// input space (which hopefully higher hierarchical Regions would handle more 
/// successfully). Passing in -1 for input radius will mean no restriction.
///   
/// Prior to receiving any inputs, the region is initialized by computing a list of 
/// initial potential synapses for each column. This consists of a random set of inputs
/// selected from the input space. Each input is represented by a synapse and assigned
/// a random permanence value. The random permanence values are chosen with two 
/// criteria. First, the values are chosen to be in a small range around connectedPerm
/// (the minimum permanence value at which a synapse is considered "connected"). This 
/// enables potential synapses to become connected (or disconnected) after a small 
/// number of training 
/// iterations. Second, each column has a natural center over the input region, and 
/// the permanence values have a bias towards this center (they have higher values
/// near the center).
/// 
/// The concept of Input Radius is an additional parameter to control how 
/// far away synapse connections can be made instead of allowing connections anywhere.  
/// The reason for this is that in the case of video images I wanted to experiment 
/// with forcing each Column to only learn on a small section of the total input to 
/// more effectively learn lines or corners in a small section
void Column::CreateProximalSegments(std::vector<DataSpace*> &inputList, std::vector<int> &inputRadii)
{
	DataSpace *curInput;
	Area inputAreaHcols, inputAreaCols;
	int curInputVolume, synapsesPerSegment;
	int inputRadius;
	float dX, dY, distanceToInput_SrcSpace, distanceToInput_DstSpace;
	float srcHcolX, srcHcolY;

	// Initialize values.
	SumInputVolume = 0;
	_minOverlap = 0;

	// Determine this Column's hypercolumn coordinates.
	int destHcolX = (int)(Position.X / region->GetHypercolumnDiameter()); 
	int destHcolY = (int)(Position.Y / region->GetHypercolumnDiameter()); 

	// Determine the center of this Column's hypercolumn, in this Column's Region's space.
	float inputCenterX = (((float)destHcolX) + 0.5f) / (float)((region->GetSizeX()) / (region->GetHypercolumnDiameter()));
	float inputCenterY = (((float)destHcolY) + 0.5f) / (float)((region->GetSizeY()) / (region->GetHypercolumnDiameter()));

	// Iterate through each input DataSpace, creating synapses for each one, in each column's proximal segment.
	for (int inputIndex = 0; inputIndex < inputList.size(); inputIndex++)
	{
		// Get a pointer to the current input DataSpace.
		curInput = inputList[inputIndex];

		// Get the input radius corresponding to the current input space.
		inputRadius = inputRadii[inputIndex];

		// Determine the center of the receptive field, in the input space's hypercolumn coordinates.
		srcHcolX = Min((int)(inputCenterX * (float)(curInput->GetSizeX() / curInput->GetHypercolumnDiameter())), (curInput->GetSizeX() / curInput->GetHypercolumnDiameter() - 1));
		srcHcolY = Min((int)(inputCenterY * (float)(curInput->GetSizeY() / curInput->GetHypercolumnDiameter())), (curInput->GetSizeY() / curInput->GetHypercolumnDiameter() - 1));

		if (inputRadius == -1) 
		{
			// The input area will be the full input space.
			inputAreaHcols = Area(0, 0, curInput->GetSizeX() / curInput->GetHypercolumnDiameter() - 1, curInput->GetSizeY() / curInput->GetHypercolumnDiameter() - 1);
		}	
		else 
		{
			// Determine the input area in hypercolumns.
			inputAreaHcols = Area(Max(0, srcHcolX - inputRadius),
														Max(0, srcHcolY - inputRadius),
														Min(curInput->GetSizeX() / curInput->GetHypercolumnDiameter() - 1, srcHcolX + inputRadius), 
														Min(curInput->GetSizeY() / curInput->GetHypercolumnDiameter() - 1, srcHcolY + inputRadius));
		}

		// Determine the input area in columns.
		inputAreaCols = Area(inputAreaHcols.MinX * curInput->GetHypercolumnDiameter(), 
												 inputAreaHcols.MinY * curInput->GetHypercolumnDiameter(),
												 (inputAreaHcols.MaxX + 1) * curInput->GetHypercolumnDiameter() - 1,
												 (inputAreaHcols.MaxY + 1) * curInput->GetHypercolumnDiameter() - 1);

		// Compute volume (in input values) of current input area.
		curInputVolume = inputAreaCols.GetArea() * curInput->GetNumValues();

		// Add the current input volume to the SumInputVolume (used by ComputeOverlap()).
		SumInputVolume += curInputVolume;

		// Proximal synapses per Column (input segment) for the current input.
		synapsesPerSegment = (int)(curInputVolume * region->PctInputPerColumn + 0.5f);

		// The minimum number of inputs that must be active for a column to be 
		// considered during the inhibition step. Sum for all input DataSpaces.
		_minOverlap +=	(int)ceil((float)synapsesPerSegment * region->PctMinOverlap);

		WeightedDataPoint *InputSpaceArray = new WeightedDataPoint[curInputVolume];
		float weight, sumWeight = 0.0f;

		// Fill the InputSpaceArray with coords of each data point in the input space, 
		// and a weight value inversely proportial to distance from the center of the field.
		int pos = 0;
		for (int hy = inputAreaHcols.MinY; hy <= inputAreaHcols.MaxY; hy++)
		{
			for (int hx = inputAreaHcols.MinX; hx <= inputAreaHcols.MaxX; hx++)
			{
				// Determine the distance of the current input hypercolumn from the center of the field, in the source input space's coordinates.
				dX = srcHcolX - hx;
				dY = srcHcolY - hy;
				distanceToInput_SrcSpace = sqrt(dX * dX + dY * dY);

				// Determine the distance of the current input hypercolumn from the center of the field, in the destination region's coordinates.
				dX *= (float)(region->GetSizeX() / region->GetHypercolumnDiameter()) / (float)(curInput->GetSizeX() / curInput->GetHypercolumnDiameter());
				dY *= (float)(region->GetSizeY() / region->GetHypercolumnDiameter()) / (float)(curInput->GetSizeY() / curInput->GetHypercolumnDiameter());
				distanceToInput_DstSpace = sqrt(dX * dX + dY * dY);

				//// Determine this input hypercolumn's weight based on its distance from the input hypercolumn at the center of the receptive field.
				//// There will be zero probability of synapses to inputs beyond a distance of inputRadius.
				////weight = pow(1.0 - Min(1.0, distanceToInput_InputSpace / (double)(inputRadius)), 0.5);

				// Each hypercolumn with distance from center of receptive field less than inputRadius +1 has weight.
				weight = (distanceToInput_SrcSpace < (inputRadius + 1)) ? 1 : 0;

				for (int y = 0; y < curInput->GetHypercolumnDiameter(); y++)
				{
					for (int x = 0; x < curInput->GetHypercolumnDiameter(); x++)
					{
						for (int valueIndex = 0; valueIndex < curInput->GetNumValues(); valueIndex++)
						{
							InputSpaceArray[pos].X = (curInput->GetHypercolumnDiameter() * hx) + x;
							InputSpaceArray[pos].Y = (curInput->GetHypercolumnDiameter() * hy) + y;
							InputSpaceArray[pos].Index = valueIndex;
							InputSpaceArray[pos].Distance = distanceToInput_DstSpace;
							InputSpaceArray[pos].Weight = weight;
							sumWeight += weight;
							pos++;
						}
					}
				}
			}
		}
		_ASSERT(pos == curInputVolume);

		// Initialize gaussian distribution random number generator. 
		std::default_random_engine generator;
		std::normal_distribution<double> gausianNormalDistribution(region->ProximalSynapseParams.ConnectedPerm, region->ProximalSynapseParams.PermanenceInc); // Center value and standard deviation.

		// Generate synapsesPerSegment samples, moving thier WeightedDataPoint records to the beginning of the InputSpaceArray.
		WeightedDataPoint tempPoint;
		float curSample, curSampleSumWeight;
		int curSamplePos, numSamples = 0;
		double permanence;
		while (numSamples < synapsesPerSegment)
		{
			// Determine a sample within the range of the sum weight of all points that have not yet been selected as samples.
			curSample = ((float)rand() / (float)RAND_MAX) * sumWeight;
			
			// Iterate through all remaining points that have not yet been selected as samples...
			curSampleSumWeight = 0.0f;
			for (curSamplePos = numSamples; curSamplePos < curInputVolume; curSamplePos++)
			{
				curSampleSumWeight += InputSpaceArray[curSamplePos].Weight;

				// If the random sample targets the current point, exit the loop.
				if (curSampleSumWeight >= curSample) {
					break;
				}
			}

			// Subtract the weight of the sampled point from the sumWeight of all points that haven't yet been selected as samples.
			sumWeight -= InputSpaceArray[curSamplePos].Weight;

			// Determine the permanence value for the new Syanpse.
			permanence = gausianNormalDistribution(generator);

			// Create the proximal synapse for the current sample.
			ProximalSegment->CreateProximalSynapse(&(region->ProximalSynapseParams), curInput, InputSpaceArray[curSamplePos], permanence, InputSpaceArray[curSamplePos].Distance);

			if (curSamplePos != numSamples)
			{
				// Swap the Point values at the curSamplePos position with that at the numSamples position.
				// This way, all samples are moved to the beginning of the InputSpaceArray, below numSamples.
				tempPoint = InputSpaceArray[numSamples];
				InputSpaceArray[numSamples] = InputSpaceArray[curSamplePos];
				InputSpaceArray[curSamplePos] = tempPoint;
			}

			// Increment numSamples.
			numSamples++;
		}

		/*
		// OLD SYSTEM from OpenHTM -- varies initial permanence based on distance to input, rather than probability of connection.
		// Create a proximal synapse on the proximalSegment for each sample. 
		// Permanence value is based on Gaussian distribution around the ConnectedPerm value, biased by distance from this Column.
		WeightedDataPoint inputPoint;
		for (int i = 0; i < numSamples; i++)
		{
			// Get the current sample DataPoint.
			inputPoint = InputSpaceArray[i];

			double permanence = gausianNormalDistribution(generator);
			
			// Distance from this column to the input bit, in the input space's coordinates.
			dX = (Position.X * XSpace) - inputPoint.X;
			dY = (Position.Y * YSpace) - inputPoint.Y;
			distanceToInput_InputSpace = sqrt(dX * dX + dY * dY);

			// Distance from this column to the input bit, in this Region's coordinates (used by Region::AverageReceptiveFieldSize()).
			dX = Position.X - (inputPoint.X / XSpace);
			dY = Position.Y - (inputPoint.Y / YSpace);
			distanceToInput_RegionSpace = sqrt(dX * dX + dY * dY);

			// Original
			//double localityBias = RadiusBiasPeak / 2.0f * exp(pow(distanceToInput_InputSpace / (longerSide * RadiusBiasStandardDeviation), 2) / -2);
			//double permanenceBias = Min(1.0f, permanence * localityBias);

			// My version
			double localityBias = pow(1.0 - Min(1.0, distanceToInput_InputSpace / (double)localityRadius), 0.001);
			double permanenceBias = permanence * localityBias;
			
			// Create the proximal synapse for the current sample.
			ProximalSegment->CreateProximalSynapse(&(region->ProximalSynapseParams), curInput, inputPoint, permanenceBias, distanceToInput_RegionSpace);
		}
		*/

		delete [] InputSpaceArray;
	}

	// Overlap must be at least 1.
	if (_minOverlap <= 0) {
		_minOverlap = 1;
	}
}

///  For this column, return the cell with the best matching Segment 
///  (at time t-1 if prevous=True else at time t). Only consider segments that are 
///  predicting cell activation to occur in exactly numPredictionSteps many 
///  time steps from now. If no cell has a matching segment, then return the 
///  cell with the fewest number of segments.
/// 
///  numPredictionSteps: only consider segments that are predicting
///  cell activation to occur in exactly this many time steps from now.
///  if true only consider active segments from t-1 else 
///  consider active segments right now.
///  Returns: Tuple containing the best cell and its best Segment. (may be None).
void Column::GetBestMatchingCell(int numPredictionSteps, bool previous, Cell* &bestCell, Segment* &bestSegment)
{
	// Initialize return values
	bestCell = NULL;
	bestSegment = NULL;

	int bestCount = 0;
	Segment *seg;
	Cell *cell;

	for (int cellIndex = 0; cellIndex < region->GetCellsPerCol(); cellIndex++)
	{
		cell = Cells[cellIndex];

		seg = cell->GetBestMatchingSegment(numPredictionSteps, previous);

		if (seg != NULL)
		{
			int synCount = 0;

			if (previous)
			{
				synCount = seg->GetPrevActiveSynapseCount();
			}
			else
			{
				synCount = seg->GetActiveSynapseCount();
			}

			if (synCount > bestCount)
			{
				bestCell = cell;
				bestSegment = seg;
				bestCount = synCount;
			}
		}
	}

	int fewestNumSegments = INT_MAX;
	int sameNumSegmentsCount = 0;

	// If there are no active sequences, return the cell with the fewest number of segments.
	// If there are multiple cells with the same fewest number of segments, choose one at random.
	// This is necessary as described here: http://sourceforge.net/p/openhtm/discussion/htm/thread/ccedad1f/
	if (bestCell == NULL)
	{
		for (int cellIndex = 0; cellIndex < region->GetCellsPerCol(); cellIndex++)
		{
			cell = Cells[cellIndex];			int numSegments = cell->Segments.Count();

			// Keep count of how many cells have this same (fewest) number of segments.
			if (numSegments < fewestNumSegments) {
				sameNumSegmentsCount = 1;
			}	else if (numSegments == fewestNumSegments) {
				sameNumSegmentsCount++;
			}

			// If the current cell has the fewest number of segments seen yet, record that it is the new bestCell.
			// If the current cell has the same number of segments as previously found cell(s) with the fewest 
			// number of segments, then decide randomly whether to keep the previous pick or select the current
			// cell as the bestCell instead. The probability of selecting this cell is base don the number of cells
			// found so far with this fewest number of segments; for the 2nd cell there is 1/2 chance, for the
			// 3rd cell there is 1/3 chance, etc. The result is correctly that if there are e.g. 10 cells with the 
			// same fewest number of segments, each one will have a 1/10 chance of being selected.
			if ((numSegments < fewestNumSegments) ||
				  ((numSegments == fewestNumSegments) && ((rand() % sameNumSegmentsCount) == 0)))
			{
				fewestNumSegments = numSegments;
				bestCell = cell;
			}
		}

		//leave bestSegment null to indicate a new segment is to be added.
	}
}

/// The spatial pooler overlap of this column with a particular input pattern.
/// The overlap for each column is simply the number of connected synapses with active 
/// inputs, multiplied by its boost. If this value is below MinOverlap, we set the 
/// overlap score to zero.
/// Attention: refactored regarding MinOverlap from column: overlap is now computed as 
/// the former overlap per area as this will make areas with inequal size comparable
/// (Columns near the edges of the Region will have smaller input areas).
void Column::ComputeOverlap()
{
	// Calculate number of active synapses on the proximal segment
	ProximalSegment->ProcessSegment();

	// Find "overlap", that is the current number of active and connected synapses
	float overlap = ProximalSegment->GetActiveConnectedSynapseCount();

	if (overlap < _minOverlap)
	{
		overlap = 0;
	}
	else
	{
		// Set Overlap to be the number of active connected synapses, multiplid by the Boost factor, and mutiplied by the ratio of the 
		// active connected synapses count over the active connected synapses count plus the inactive well-connected (>=InitialPerm) synpses count.
		// This last term penalizes a match if it includes many strongly connected synapses that are not active,
		// so that patterns with greater numbers of connected syanpses do not gain an advantage in representing all possible subpatterns.
		// It only cares about strongly connected synapses, because weakly connected synapses can be the result of little or no learning, and we don't
		// want to penalize matches that haven't had a chance to be refined by learning yet.
		overlap = overlap * ((float)(ProximalSegment->GetActiveConnectedSynapseCount()) / (float)(ProximalSegment->GetActiveConnectedSynapseCount() + ProximalSegment->GetInactiveWellConnectedSynapsesCount())) * Boost;
	}

	// Record the determined number as this Column's Overlap.
	Overlap = overlap;
}

// Return the Area of Columns that are withi the given hypercolumn _radius of this Column's hypercolumn.
Area Column::DetermineColumnsWithinHypercolumnRadius(int _radius)
{
	return Area(Max(0, ((Position.X / region->GetHypercolumnDiameter()) - _radius) * region->GetHypercolumnDiameter()),
							Max(0, ((Position.Y / region->GetHypercolumnDiameter()) - _radius) * region->GetHypercolumnDiameter()),
							Min(region->Width - 1, ((Position.X / region->GetHypercolumnDiameter()) + _radius + 1) * region->GetHypercolumnDiameter() - 1),
							Min(region->Height - 1, ((Position.Y / region->GetHypercolumnDiameter()) + _radius + 1) * region->GetHypercolumnDiameter() - 1));
}

// Determine this Column's DesiredLocalActivity, being PctLocalActivity of the number of columns wthin its Region's InhibitionRadius of hypercolumns.
void Column::DetermineDesiredLocalActivity()
{
	// Determine the extent, in columns, of the area within the InhibitionRadius of hypercolumns.
	Area inhibitionArea = DetermineColumnsWithinHypercolumnRadius(region->InhibitionRadius + 0.5f);

	// The DesiredLocalActivity is the PctLocalActvity multiplied by the area over which inhibition will take place.
	DesiredLocalActivity = (int)(inhibitionArea.GetArea() * region->PctLocalActivity + 0.5f);
}

/// Compute whether or not this Column will be active after the effects of local
/// inhibition are applied.  A Column must have overlap greater than 0 and have its
/// overlap value be within the k'th largest of its neighbors 
/// (where k = desiredLocalActivity).
void Column::ComputeColumnInhibition()
{
	IsActive = false;
	IsInhibited = false;

	if (Overlap > 0)
	{
		if (region->IsWithinKthScore(this, DesiredLocalActivity))
		{
			IsActive = true;
		}
		else
		{
			IsInhibited = true;
		}
	}
}

/// Update the permanence value of every synapse in this column based on whether active.
/// This is the main learning rule (for the column's proximal dentrite). 
/// For winning columns, if a synapse is active, its permanence value is incremented, 
/// otherwise it is decremented. Permanence values are constrained to be between 0 and 1.
void Column::AdaptPermanences()
{
	ProximalSegment->AdaptPermanences();
}

/// Increase the permanence value of every unconnected proximal synapse in this column by the amount given.
void Column::BoostPermanences(float amount)
{
	FastListIter synapses_iter(ProximalSegment->Synapses);
	for (Synapse *syn = (Synapse*)(synapses_iter.Reset()); syn != NULL; syn = (Synapse*)(synapses_iter.Advance()))
	{
		if (syn->GetPermanence() < region->ProximalSynapseParams.ConnectedPerm) {
			syn->IncreasePermanence(amount, region->ProximalSynapseParams.ConnectedPerm);
		}
		else if (syn->GetPermanence() > region->ProximalSynapseParams.ConnectedPerm) {
			syn->DecreasePermanence(amount, region->ProximalSynapseParams.ConnectedPerm);
		}
	}
}


// Update running averages of activity and overlap.
void Column::UpdateDutyCycles()
{
	maxDutyCycle = DetermineMaxDutyCycle();
	
	UpdateActiveDutyCycle();

	UpdateOverlapDutyCycle();
}

/// There are two separate boosting mechanisms in place to help a column learn connections. 
/// If a column does not win often enough (as measured by activeDutyCycle), its overall 
/// boost value is increased (line 30-32). 
/// Alternatively, if a column's connected synapses do not overlap well with any inputs
/// often enough (as measured by overlapDutyCycle), its permanence values are boosted 
/// (line 34-36). Note: once learning is turned off, boost(c) is frozen.
/// Note: the below implementation differs significantly from the Numenta white paper and from the OpenHTM
/// implementation. The changes were made is order to enforce pattern separation, and to optimize the 
/// creation of well fitting feature detectors for vision applications. Eventually, it may be better
/// to replace the proximal synapse learning system with something like the XCAL learning model.
void Column::PerformBoosting()
{
	// If this Column's ActiveDutyCycle is less than a small fraction of its MaxDutyCycle (the max ActiveDutyCycle 
	// of the Columns within its InhibitionRadius), then increase its Boost.
	if (ActiveDutyCycle < (maxDutyCycle * ActiveOverMaxDutyCycle_IncreaseBoostThreshold))
	{
		// If this Column hasn't yet reached the specified MaxBoost...
		if ((GetMaxBoost() == -1) || (Boost < GetMaxBoost()))
		{
			// If this Column's Boost was not increased during the previous time step, meaning that a new period of boosting is beginning...
			if (prevBoostTime < (region->GetStepCounter() - 1))
			{
				// Set the permanence value of each of this Column's connected proximal synapses to exactly ConnectedPerm. This will make it easy
				// for synapses from inactive inputs to become disconnected the next time this Column is activated, allowing this Column to come to 
				// represent a smaller subpattern of what it currently represents.
				FastListIter synapses_iter(ProximalSegment->Synapses);
				for (Synapse *syn = (Synapse*)(synapses_iter.Reset()); syn != NULL; syn = (Synapse*)(synapses_iter.Advance()))
				{
					if (syn->GetPermanence() > region->ProximalSynapseParams.ConnectedPerm) {
						syn->SetPermanence(region->ProximalSynapseParams.ConnectedPerm);
					}
				}
			}

			// Linearly increase Boost.
			Boost = Boost + region->GetBoostRate();

			// Limit Boost to MaxBoost (if MaxBoost != -1, which would mean no limit).
			if (GetMaxBoost() != -1) {
				Boost = Min(Boost, GetMaxBoost());
			}

			// Record when this Boost increase took place.
			prevBoostTime = region->GetStepCounter();
		}
		else
		{
			// Boost the Column's synapse permanences. Unconnected synapses are gradually boosted toward ConnectedPerm. This allows
			// the Column to potentially come to represent a new pattern.
			BoostPermanences(region->GetBoostRate());
		}
	}
	else if ((Boost > GetMinBoost()) && (ActiveDutyCycle > (maxDutyCycle * ActiveOverMaxDutyCycle_DecreaseBoostThreshold)) && (FastActiveDutyCycle > (maxDutyCycle * ActiveOverMaxDutyCycle_DecreaseBoostThreshold)))
	{
		// Linearly decrease Boost.
		Boost = Max(Boost - region->GetBoostRate(), GetMinBoost());
	}
}

/// Returns the maximum active duty cycle of the columns that are within
/// inhibitionRadius hypercolumns of this column.
/// Returns: Maximum active duty cycle among neighboring columns.
float Column::DetermineMaxDutyCycle()
{
	// The below code fails, not sure why. Commented out until I have time to investigate.
	//// If this Column is not the upper-left COlumn in its hypercolumn, return the maxDutyCycle of that upper-left Column. 
	//// All Columns within the same hypercolumns have the same inhbition area and so the same maxDutyCycle.
	//if (((Position.X % region->GetHypercolumnDiameter()) != 0) || ((Position.Y % region->GetHypercolumnDiameter()) != 0)) {
	//	return region->GetColumn(HypercolumnPosition.X * region->GetHypercolumnDiameter(), HypercolumnPosition.Y * region->GetHypercolumnDiameter())->GetMaxDutyCycle();
	//}

	// Find extents of neighboring columns within inhibitionRadius.
	// InhibitionRadius is specified in hypercolumns.
	Area inhibitionArea = DetermineColumnsWithinHypercolumnRadius(region->InhibitionRadius + 0.5f);

	// Find the maxDutyCycle of all columns within inhibitionRadius
	Column *col;
	float maxDuty = 0.0f;
	for (int x = inhibitionArea.MinX; x <= inhibitionArea.MaxX; x++)
	{
		for (int y = inhibitionArea.MinY; y <= inhibitionArea.MaxY; y++)
		{
			col = region->GetColumn(x, y);
			if (col->GetActiveDutyCycle() > maxDuty)
			{
				maxDuty = col->GetActiveDutyCycle();
			}
		}
	}

	return maxDuty;
}

/// Computes a moving average of how often this column has been active 
/// after inhibition.
void Column::UpdateActiveDutyCycle()
{
	float newCycle;

	// Update ActiveDutyCycle
	newCycle = (1.0f - EmaAlpha) * ActiveDutyCycle;
	if (IsActive)
	{
		newCycle += EmaAlpha;
	}
	ActiveDutyCycle = newCycle;

	// Update FastActiveDutyCycle
	newCycle = (1.0f - FastEmaAlpha) * FastActiveDutyCycle;
	if (IsActive)
	{
		newCycle += FastEmaAlpha;
	}
	FastActiveDutyCycle = newCycle;
}

/// Computes a moving average of how often this column has overlap greater than
/// MinOverlap.
/// Exponential moving average (EMA):
/// St = a * Yt + (1-a)*St-1
void Column::UpdateOverlapDutyCycle()
{
	float newCycle = (1.0f - EmaAlpha) * _overlapDutyCycle;

	// CHANGED: Overlap is divided by Boost before being compared with _minOverlap, because _minOverlap doesn't have Boost factored into it. This makes them comparable.
	if ((Overlap / Boost) >= _minOverlap) // Note: Numenta docs indicate >, but given its function in boosting of trying to raise overlap to a number that allows activation (ie., minOverlap), >= makes more sense to me. 
	{
		newCycle += EmaAlpha;
	}
	_overlapDutyCycle = newCycle;
}
