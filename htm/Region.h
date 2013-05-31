#pragma once
#include "Column.h"
#include "InputSpace.h"
#include "SegmentUpdateInfo.h"
#include "DataSpace.h"
#include "Synapse.h"
#include <list>

class NetworkManager;
class Cell;

enum InhibitionTypeEnum
{
	INHIBITION_TYPE_AUTOMATIC = 0,
	INHIBITION_TYPE_RADIUS = 1
};

/// Represents an entire region of HTM columns for the CLA.
///
/// Code to represent an entire Hierarchical Temporal Memory (HTM) Region of 
/// Columns that implement Numenta's new Cortical Learning Algorithms (CLA). 
/// The Region is defined by a matrix of columns, each of which contains several cells.  
/// The main idea is that given a matrix of input bits, the Region will first sparsify 
/// the input such that only a few Columns will become 'active'.  As the input matrix 
/// changes over time, different sets of Columns will become active in sequence.  
/// The Cells inside the Columns will attempt to learn these temporal transitions and 
/// eventually the Region will be able to make predictions about what may happen next
/// given what has happened in the past. 
/// For (much) more information, visit www.numenta.com.
/// SpatialPooling snippet from the Numenta docs:
/// The code computes activeColumns(t) = the list of columns that win due to 
/// the bottom-up input at time t. This list is then sent as input to the 
/// temporal pooler routine.
/// Phase 1: compute the overlap with the current input for each column
/// Phase 2: compute the winning columns after inhibition
/// Phase 3: update synapse permanence and internal variables
/// 
/// 1) Start with an input consisting of a fixed number of bits. These bits might 
///    represent sensory data or they might come from another region lower in the 
///    hierarchy.
/// 2) Assign a fixed number of columns to the region receiving this input. Each 
///    column has an associated dendrite segmentUpdateList. Each dendrite 
///    segmentUpdateList has a set of potential synapses representing a subset of the 
///    input bits. Each potential synapse has a permanence value.
///    Based on their permanence values, some of the potential synapses will be valid.
/// 3) For any given input, determine how many valid synapses on each column are 
///    connected to active input bits.
/// 4) The number of active synapses is multiplied by a 'boosting' factor which is 
///    dynamically determined by how often a column is active relative to its neighbors. 
/// 5) The columns with the highest activations after boosting disable all but a fixed 
///    percentage of the columns within an inhibition radius. The inhibition radius is 
///    itself dynamically determined by the spread (or 'fan-out') of input bits. 
///    There is now a sparse set of active columns.
/// 6) For each of the active columns, we adjust the permanence values of all the 
///    potential synapses. The permanence values of synapses aligned with active input 
///    bits are increased. The permanence values of synapses aligned with inactive 
///    input bits are decreased. The changes made to permanence values may change 
///    some synapses from being valid to not valid, and vice-versa.
class Region
	: public DataSpace
{
public:
	~Region(void);

	/// Properties

	NetworkManager *Manager;

	Column **Columns;

	int CellsPerCol;

	int GetCellsPerCol() {return CellsPerCol;}

	int SegActiveThreshold;

	/// If set to true, the Region will assume the Column
	/// grid dimensions exactly matches that of the input data.
	/// 
	/// The hardcoded flag is something Barry added to bypass the spatial 
	/// pooler for exclusively working with the temporal sequences. It is more than
	/// just disabling spatial learning. In this case the Region will assume the 
	/// column grid dimensions exactly matches that of the input data. Then on each
	/// time step no spatial pooling is performed, instead we assume the input data
	/// is itself describing the state of the active columns. Think of hardcoded as
	/// though the spatial pooling has already been performed outside the network and
	/// we are just passing in the results to the columns as-is.
	bool HardcodedSpatial;

	/// If set to true the Region will perform spatial learning
	/// during the spatial pooling phase.  If false the Region will perform inference
	/// only during spatial pooling.  This value may be toggled on/off at any time
	/// during run.  When learning is finished the Region can be frozen in place by
	/// toggling this false (predictions will still occur via previously learned data).
	bool SpatialLearning;

	/// If set to true the Region will perform temporal learning
	/// during the temporal pooling phase.  If false the Region will perform inference
	/// only during temporal pooling.  This value may be toggled on/off at any time
	/// during run.  When learning is finished the Region can be frozen in place by
	/// toggling this false (predictions will still occur via previously learned data).
	bool TemporalLearning;

	// Determines whether InhibitionRadius uses a set given radius, or is automatcially determined based on average receptive field size.
	InhibitionTypeEnum InhibitionType; 

	/// This radius determines how many hypercolumns in a local area are considered during
	/// spatial pooling inhibition.  
	float InhibitionRadius;

	/// Furthest number of columns away (in this Region's Column grid space) to allow new distal 
	/// synapse connections.  If set to 0 then there is no restriction and connections
	/// can form between any two columns in the region.
	int PredictionRadius;

	/// The number of new distal synapses added to segments if no matching ones are found 
	/// during learning.
	int NewSynapsesCount;

	// This parameter determines whether a segment will be considered a match to activity. It may be considered a match if at least
	// this number of the segment's synapses match the actvity. The segment will then be re-used to represent that activity, with new syanpses
	// added to fill out the pattern. The lower this number, the more patterns will be added to a single segment, which can be very bad because
	// the same cell is thus used to represent an input in diffrent contexts, and also if the ratio of PermanenceInc to PermanenceDec
	// is such that multiple patterns cannot be supported on one synapse, so all but 1 will generally remain disconnected, so predictions are never made.
	// These are the minimum and maximum values of a range. Each individual column is given a different MinOverlapToReuseSegment value from within this range.
	int Min_MinOverlapToReuseSegment, Max_MinOverlapToReuseSegment;

	/// Percent of input bits each Column will have potential proximal (spatial) 
	/// synapses for.
	float PctInputPerColumn;

	/// Minimum percent of column's proximal synapses that must be active for column 
	/// to be considered during spatial pooling inhibition.  This value helps set the
	/// minimum column overlap value during region run.
	float PctMinOverlap;

	/// Approximate percent of Columns within average receptive field radius to be 
	/// winners after spatial pooling inhibition.
	float PctLocalActivity;

	// The maximum allowed Boost value. Setting a max Boost value prevents competition
	// for activation from continuing indefinitely, and so facilitates stabilization of representations.
	float MaxBoost;

	// The amount that is added to a Column's Boost value in a single time step, when it is being boosted.
	float BoostRate;

	// Start and end times for spatial learning, temporal learning, and boosting periods. 
	// -1 for start time means start from beginning; -1 for end time means continue through end. 
	// 0 for end time means no learning of that type will take place (since time clock starts at 1).
	int SpatialLearningStartTime, SpatialLearningEndTime, TemporalLearningStartTime, TemporalLearningEndTime, BoostingStartTime, BoostingEndTime;

	// Whether this Region will output the activity of the column as a whole, and of each Cell individually.
	bool OutputColumnActivity, OutputCellActivity;

	// The dimensions of the Region's Column grid, Width x Height.
	int Width, Height;

	// The diameter of a hypercolumn in this Region. Defaults to 1.
	int HypercolumnDiameter;

	// This Region's synapse parameters.
	SynapseParameters ProximalSynapseParams, DistalSynapseParams;

	// The number of output values per column of this Region. If OutputCellActivity is true,
	// then this number will include the number of cells. If OutputColumnActivity is true, then
	// this number will include one value representing the activity of the column as a whole.
	int NumOutputValues;

	// The ID strings of each of this Region's input DataSpaces.
	std::vector<QString> InputIDs;

	// The input radius corresponding to each input DataSpace.
	// Furthest number of hypercolumns away (in the input DataSpace's grid space) to allow proximal 
	// synapse connections from the corresponding input DataSpace. If set to -1 then there is no restriction 
	// and connections can form from any input value.
	std::vector<int> InputRadii;

	// A list of pointers to each of this Region's input DataSpaces.
	std::vector<DataSpace*> InputList;

	/// Statistics

	/// Increments at every time step.
	float StepCounter;

	// For recording feature detector statistics.
	int fd_numActiveCols, fd_missingSynapesCount, fd_extraSynapsesCount;

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
	/// cellsPerCol: The number of context learning cells per column.
	/// segActiveThreshold: The minimum number of synapses that must be 
	///     active for a segment to fire.
	/// newSynapseCount: The number of new distal synapses added if
	///     no matching ones found during learning.
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
	/// handle more successfully).  Passing in -1 for input radius will mean no restriction.
	Region(NetworkManager *manager, QString &_id, Point colGridSize, int hypercolumnDiameter, SynapseParameters proximalSynapseParams, SynapseParameters distalSynapseParams, float pctInputPerCol, float pctMinOverlap, int predictionRadius, InhibitionTypeEnum inhibitionType, int inhibitionRadius, float pctLocalActivity, float boostRate, float maxBoost, int spatialLearningStartTime, int spatialLearningEndTime, int temporalLearningStartTime, int temporalLearningEndTime, int boostingStartTime, int boostingEndTime, int cellsPerCol, int segActiveThreshold, int newSynapseCount, int min_MinOverlapToReuseSegment, int max_MinOverlapToReuseSegment, bool hardcodedSpatial, bool outputColumnActivity, bool outputCellActivity);

	/// Methods

	// DataSpace Methods

	DataSpaceType GetDataSpaceType() {return DATASPACE_TYPE_REGION;}

	int GetSizeX() {return Width;}
	int GetSizeY() {return Height;}
	int GetNumValues() {return NumOutputValues;}
	int GetHypercolumnDiameter() {return HypercolumnDiameter;}

	bool GetIsActive(int _x, int _y, int _index);

	// Add an input DataSpace to this Region
	void AddInput(DataSpace *_inputDataSpace);

	NetworkManager *GetManager() {return Manager;}

	float GetMaxBoost() {return MaxBoost;}
	float GetBoostRate() {return BoostRate;}
	int GetSpatialLearningStartTime() {return SpatialLearningStartTime;}
	int GetSpatialLearningEndTime() {return SpatialLearningEndTime;}
	int GetTemporalLearningStartTime() {return TemporalLearningStartTime;}
	int GetTemporalLearningEndTime() {return TemporalLearningEndTime;}
	int GetBoostingStartTime() {return BoostingStartTime;}
	int GetBoostingEndTime() {return BoostingEndTime;}

	// Determine whether a particular cell in this Region is active, predicted or learning.
	bool IsCellActive(int _x, int _y, int _index);
	bool IsCellPredicted(int _x, int _y, int _index);
	bool IsCellLearning(int _x, int _y, int _index);

	Cell *GetCell(int _x, int _y, int _index);

	int GetStepCounter() {return StepCounter;}
	
	// Called after adding all inputs to this Region.
	void Initialize();

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
	void PerformSpatialPooling();

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
	/// was not predicted, then all cells in the col become active (lines 32-34). 
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
	void PerformTemporalPooling();

	/// Get a pointer to the Column at the specified column grid coordinate.
	///
	/// x: the x coordinate component of the column's position.
	/// y: the y coordinate component of the column's position.
	/// returns: a pointer to the Column at that position.
	Column *GetColumn(int x, int y);

	/// The radius of the average connected receptive field size of all the columns. 
	/// 
	/// returns: The average connected receptive field size (in hypercolumn grid space).
	/// 
	/// The connected receptive field size of a column includes only the connected 
	/// synapses (those with permanence values >= connectedPerm). This is used to 
	/// determine the extent of lateral inhibition between columns.
	float AverageReceptiveFieldSize();

	/// Return true if the given Column has an overlap value that is at least the
	/// k'th largest amongst all neighboring columns within inhibitionRadius.
	///
	/// This function is effectively determining which columns are to be inhibited 
	/// during the spatial pooling procedure of the region.
	bool IsWithinKthScore(Column *col, int k);

	/// Update the values of the inputData for this <see cref="Region"/> by copying row 
	/// references from the specified newInput parameter.
	/// 
	/// inputArray: 2d Array used for next Region time step. 
	/// The newInput array must have the same shape as the original inputData.
	void SetInput(int *inputArray, int width, int height);

	/// Run one time step iteration for this Region.
	///
	/// All cells will have their current (last run) state pushed back to be their 
	/// new previous state and their new current state reset to no activity.  
	/// Then SpatialPooling followed by TemporalPooling is performed for one time step.
	void Step();
	
	/// Statistics

	/// Sets statistics values to 0.
	void InitializeStatisticParameters();

	/// Updates statistics values.
	void ComputeBasicStatistics();
	void ComputeColumnAccuracy();
};

