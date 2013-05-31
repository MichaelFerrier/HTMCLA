#pragma once
#include "Utils.h"
#include "MemObject.h"
#include "MemManager.h"
#include "FastList.h"
#include "DistalSynapse.h"
#include "ProximalSynapse.h"

class DataSpace;

/// Represents a single dendrite segment that forms synapses (connections) to other Cells.
/// 
/// Each Segment also maintains a boolean flag indicating 
/// whether the Segment predicts feed-forward input on the next time step.
/// Segments can be either proximal or distal (for spatial pooling or temporal pooling 
/// respectively) however the class object itself does not need to know which
/// it ultimately is as they behave identically.  Segments are considered 'active' 
/// if enough of its existing synapses are connected and individually active.

/// Maximum number of prediction steps to track.
const int MaxTimeSteps = 10;

class Segment :
	public MemObject
{
public:
	Segment(void);
	~Segment(void);

	void Retire();


	/// Properties
	
private:

	bool IsActive, WasActive, IsSequence;
	int ActiveConnectedSynapsesCount, PrevActiveConnectedSynapsesCount;
	int ActiveLearningSynapsesCount, PrevActiveLearningSynapsesCount;
	int InactiveWellConnectedSynapsesCount;
	int CreationTime;

	void SetIsActive(bool value) {IsActive = value;}
	void SetWasActive(bool value) {WasActive = value;}

	void SetActiveThreshold(float value) {ActiveThreshold = value;}
			
public:

	int _numPredictionSteps;
	int ConnectedSynapsesCount, PrevConnectedSynapsesCount;
	float ActiveThreshold;

	MemObjectType GetMemObjectType() {return MOT_SEGMENT;}

	/// Returns true if the number of connected synapses on this 
	/// Segment that are active due to active states at time t is 
	/// greater than activationThreshold.
	bool GetIsActive() {return IsActive;}

	/// Returns true if the number of connected synapses on this segmentUpdateList
	/// that were active due to active states at time t-1 is greater than ActiveThreshold.
	bool GetWasActive() {return WasActive;}

	/// Returns true if the number of connected synapses on this 
	/// Segment that were active due to learning states at time t-1 is 
	/// greater than activationThreshold.   
	bool GetWasActiveFromLearning();

	/// The synapses list.
	FastList Synapses;

	/// The list of all active synapses as computed as of the most recently processed
	/// time step for this segment.
	FastList ActiveSynapses;

	/// The list of all previously active (in t-1) synapses as computed as of the most
	/// recently processed time step for this segment.
	FastList PrevActiveSynapses;

	/// Returns true if the Segment predicts feed-forward input on the next time step.
	bool GetIsSequence() {return IsSequence;}
	void SetIsSequence(bool value) {IsSequence = value;} 

	///Define the number of time steps in the future an activation will occur
	///in if this segment becomes active.
	///
	/// For example if the segment is intended to predict activation in the very next 
	/// time step (t+1) then this value is 1. If the value is 2 this segment is said 
	/// to predict its Cell will activate in 2 time steps (t+2) etc.  By definition if 
	/// a segment is a sequence segment it has a value of 1 for prediction steps.
	int GetNumPredictionSteps()
	{
		return _numPredictionSteps;
	}
		
	void SetNumPredictionSteps(int value)
	{
		_numPredictionSteps = Min(Max(1, value), MaxTimeSteps);
		IsSequence = (_numPredictionSteps == 1);
	}

	/// A threshold number of active synapses between active and non-active Segment state.
	float GetActiveThreshold() {return ActiveThreshold;}

	int GetCreationTime() {return CreationTime;}

	/// Methods

	/// Initializes a new instance of the Segment class.
	/// activeThreshold: A threshold number of active synapses between 
	/// active and non-active Segment state.
	void Initialize(int creationTime, float activeThreshold);

	/// Advance this segment to the next time step.
	///
	/// The current state of this segment (active, number of synapes) will be set as 
	/// the previous state and the current state will be reset to no cell activity by 
	/// default until it can be determined.
	void NextTimeStep();

	/// Process this segment for the current time step.
	///
	/// Processing will determine the set of active synapses on this segment for this time 
	/// step.  From there we will determine if this segment is active if enough active 
	/// synapses are present.  This information is then cached for the remainder of the
	/// Region's processing for the time step.  When a new time step occurs, the
	/// Region will call nextTimeStep() on all cells/segments to cache the 
	/// information as what was previously active.
	void ProcessSegment();

	/// Create a new proximal synapse for this segment attached to the specified 
	/// input cell.
	/// inputSource: the input source of the synapse to create.
	/// initPerm: the initial permanence of the synapse.
	/// Returns the newly created synapse.
	ProximalSynapse *CreateProximalSynapse(SynapseParameters *params, DataSpace *inputSource, DataPoint &inputPoint, float permanence, float distanceToInput);

	/// Create a new synapse for this segment attached to the specified input source.
	/// inputSource: the input source of the synapse to create.
	/// initPerm: the initial permanence of the synapse.
	/// Returns the newly created synapse.
	DistalSynapse *CreateDistalSynapse(SynapseParameters *params, Cell *inputSource, float initPerm);

	/// Create numSynapses new synapses attached to the specified
	/// set of learning cells.
	/// synapseCells: Set of available learning cells to form synapses to.
	/// added: Set will be populated with synapses that were successfully added.
	void CreateSynapsesToLearningCells(FastList &synapseCells, SynapseParameters *params);

	// Return a count of how many synapses on this segment are connected.
	int GetConnectedSynapseCount() {return ConnectedSynapsesCount;}

	// Return a count of how many synapses on this segment were connected in the previous time step.
	int GetPrevConnectedSynapseCount() {return PrevConnectedSynapsesCount;}

	/// Return a count of how many synapses on this segment (whether connected or not) 
	/// are active in the current time step.
	int GetActiveSynapseCount();

	/// Return a count of how many synapses on this segment (whether connected or not) 
	/// were active in the previous time step.
	int GetPrevActiveSynapseCount();

	/// Return a count of how many connected synapses on this segment are active
	/// in the current time step.
	int GetActiveConnectedSynapseCount() {return ActiveConnectedSynapsesCount;}

	/// Return a count of how many connected synapses on this segment 
	/// were active in the previous time step.
	int GetPrevActiveConnectedSynapseCount() {return PrevActiveConnectedSynapsesCount;}

	/// Return a count of how many synapses on this segment are from active learning cells
	/// in the current time step.
	int GetActiveLearningSynapseCount() {return ActiveLearningSynapsesCount;}

	/// Return a count of how many synapses on this segment were from active learning cells
	/// in the previous time step.
	int GetPrevActiveLearningSynapseCount() {return PrevActiveLearningSynapsesCount;}
	
	// Return a count of how many synapses on this segment are connected above InitialPerm, but are not active.
	int GetInactiveWellConnectedSynapsesCount() {return InactiveWellConnectedSynapsesCount;}

	/// Update all permanence values of each synapse based on current activity.
	/// If a synapse is active, increase its permanence, else decrease it.
	void AdaptPermanences();

	/// Update (increase or decrease based on whether the synapse is active)
	/// all permanence values of each of the synapses in the specified set.
	void UpdatePermanences(FastList &activeSynapses);

	/// Decrease the permanences of each of the synapses in the set of
	/// active synapses that happen to be on this segment.
	void DecreasePermanences(FastList &activeSynapses);
};

