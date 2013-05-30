#pragma once
#include "InputSpace.h"
#include "Utils.h"
#include "FastList.h"
#include <list>

class Region;
class Cell;
class Column;
class Segment;
class Synapse;

// Exponential Moving Average alpha value
const float EmaAlpha = 0.005f;
const float FastEmaAlpha = 0.008f;

// A Column's MinBoost and MaxBoost are up to this far from 1.0 and the given MaxBoost, respectively. 
// This is to avoid ties in overlap values between columns with no boost, or with full boost.
const float BoostVariance = 0.01f;

const float ActiveOverMaxDutyCycle_IncreaseBoostThreshold = 0.01f;
const float ActiveOverMaxDutyCycle_DecreaseBoostThreshold = 0.65f;

/// Represents a single column of cells within an HTM Region. 
class Column
{
public:
	~Column(void);

	/// Fields

private:

	/// The minimum number of inputs that must be active for a column to be 
	/// considered during the inhibition step. Value established by input parameters
	/// and receptive-field size. 
	double _minOverlap;

	/// Properties

	int SumInputVolume;

	// Overlap is stored as a float because it represents not just the proximal segment's number of
	// active connected synapses, but then multiplies that number by the boost factor.
	float Overlap;

public:

	Point Position, HypercolumnPosition;
	Region *region;
	float _overlapDutyCycle;
	float ActiveDutyCycle, FastActiveDutyCycle, maxDutyCycle, MinBoost, MaxBoost, Boost;
	bool IsActive, IsInhibited;
	int prevBoostTime;

	/// This variable determines the desired amount of columns to be activated within a
	/// given spatial pooling inhibition radius.  For example if the InhibitionRadius is
	/// 3 it means we have 7x7 grid of hypercolumns under local consideration.  During inhibition
	/// we need to decide which columns in this local area are to be activated.  If 
	/// DesiredLocalActivity is 5 then we take only the 5 most strongly activated columns
	/// within the 7x7 local grid of hypercolumns.
	int DesiredLocalActivity;

	// This parameter determines whether a segment will be considered a match to activity. It may be considered a match if at least
	// this number of the segment's synapses match the actvity. The segment will then be re-used to represent that activity, with new syanpses
	// added to fill out the pattern. The lower this number, the more patterns will be added to a single segment, which can be very bad because
	// the same cell is thus used to represent an input in diffrent contexts, and also if the ratio of PermanenceInc to PermanenceDec
	// is such that multiple patterns cannot be supported on one synapse, so all but 1 will generally remain disconnected, so predictions are never made.
	int MinOverlapToReuseSegment;

	//float _predictionCounter, _correctPredictionCounter;
	//float _segmentPredictionCounter, _correctSegmentPredictionCounter;

	Point GetPosition() {return Position;}
	void  SetPosition(Point value);

	Point GetHypercolumnPosition() {return HypercolumnPosition;}

	Region *GetRegion() {return region;}
	void SetRegion(Region *value) {region = value;}

	float GetActiveDutyCycle() {return ActiveDutyCycle;}

	float GetFastActiveDutyCycle() {return FastActiveDutyCycle;}

	float GetOverlapDutyCycle() {return _overlapDutyCycle;}

	float GetMaxDutyCycle() {return maxDutyCycle;}

	float GetMinOverlap() {return _minOverlap;}

	int GetMinOverlapToReuseSegment() {return MinOverlapToReuseSegment;}

	/// A proximal dendrite segment forms synapses with feed-forward inputs.
	Segment *ProximalSegment;

	Cell **Cells;

	Cell *GetCellByIndex(int _index) {return Cells[_index];}

	/// Toggle whether or not this Column is currently active.
	bool GetIsActive() {return IsActive;}
	void SetIsActive(bool value) {IsActive = value;}

	/// Boost-Factor
	///
	/// The boost value is a scalar >= 1. 
	/// While activeDutyCyle(c) remains above minDutyCycle(c), the boost value remains at 1. 
	/// The boost increases linearly once the column's activeDutyCycle starts falling
	/// below its minDutyCycle.
	float GetBoost() {return Boost;}
	void  SetBoost(float value) {Boost = value;}

	float GetMinBoost() {return MinBoost;}
	float GetMaxBoost() {return MaxBoost;}

	int GetPrevBoostTime() {return prevBoostTime;}

	/// The spatial pooler overlap with a particular input pattern. Just connected proximal synapses 
	/// count to overlap.
	float GetOverlap() {return Overlap;}
	void SetOverlap(int value) {Overlap = value;}

	/// Results from computeColumnInhibition. 
	/// Only possible if Overlap > MinOverlap but Overlap LT MinLocalActivity.
	bool GetIsInhibited() {return IsInhibited;}
	void SetIsInhibited(bool value) {IsInhibited = value;}

	/// Constructor

	/// Initializes a new Column for the given parent Region at source 
	/// row/column position srcPos and column grid position pos.
	/// 
	/// region: The parent Region this Column belongs to.
	/// pos: A Point(x,y) of this Column's position within the Region's 
	///   column grid.
	Column(Region *_region, Point pos, int minOverlapToReuseSegment);

	/// Methods

	/// For each input DataSpace:
	///   For each (position in inputSpaceRandomPositions): 
	///     Create a new ProximalSynapse corresponding to the random sample's X, Y, and index values.
	///
	/// inputRadius
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
	void CreateProximalSegments(std::vector<DataSpace*> &inputList, std::vector<int> &inputRadii);

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
	void GetBestMatchingCell(int numPredictionSteps, bool previous, Cell* &bestCell, Segment* &bestSegment);

	/// The spatial pooler overlap of this column with a particular input pattern.
	/// The overlap for each column is simply the number of connected synapses with active 
	/// inputs, multiplied by its boost. If this value is below MinOverlap, we set the 
	/// overlap score to zero.
	/// Attention: refactored regarding MinOverlap from column: overlap is now computed as 
	/// the former overlap per area as this will make areas with inequal size comparable
	void ComputeOverlap();

	// Return the Area of Columns that are withi the given hypercolumn _radius of this Column's hypercolumn.
	Area DetermineColumnsWithinHypercolumnRadius(int _radius);
	
	// Determine this Column's DesiredLocalActivity, being PctLocalActivity of the number of columns wthin its Region's InhibitionRadius of hypercolumns.
	void DetermineDesiredLocalActivity();

	/// Determine whether this Column will be active or inhibited.
	void ComputeColumnInhibition();

	/// Update the permanence value of every synapse in this column based on whether active.
	/// This is the main learning rule (for the column's proximal dentrite). 
	/// For winning columns, if a synapse is active, its permanence value is incremented, 
	/// otherwise it is decremented. Permanence values are constrained to be between 0 and 1.
	void AdaptPermanences();

	/// Increase the permanence value of every unconnected synapse in this column by a scale factor.
	void BoostPermanences(float scale);

	// Update running averages of activty and overlap.
	void UpdateDutyCycles();

	/// There are two separate boosting mechanisms in place to help a column learn connections. 
	/// If a column does not win often enough (as measured by activeDutyCycle), its overall 
	/// boost value is increased (line 30-32). 
	/// Alternatively, if a column's connected synapses do not overlap well with any inputs
	/// often enough (as measured by overlapDutyCycle), its permanence values are boosted 
	/// (line 34-36). Note: once learning is turned off, boost(c) is frozen.
	void PerformBoosting();

	/// Returns the maximum active duty cycle of the columns that are within
	/// inhibitionRadius of this column.
	/// Returns: Maximum active duty cycle among neighboring columns.
	float DetermineMaxDutyCycle();

	/// Computes a moving average of how often this column has been active 
	/// after inhibition.
	void UpdateActiveDutyCycle();

	/// Computes a moving average of how often this column has overlap greater than
	/// MinOverlap.
	/// Exponential moving average (EMA):
	/// St = a * Yt + (1-a)*St-1
	void UpdateOverlapDutyCycle();
};