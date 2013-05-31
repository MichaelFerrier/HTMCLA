#include "Segment.h"

extern MemManager mem_manager;

Segment::Segment(void)
{
}

Segment::~Segment(void)
{
}

void Segment::Retire()
{
	// Release all Synapses.
	Synapse *curSynapse;
	while ((curSynapse = (Synapse*)(Synapses.RemoveFirst())) != NULL) {
		mem_manager.ReleaseObject(curSynapse);
	}

	ActiveSynapses.Clear();
	PrevActiveSynapses.Clear();
}

/// Returns true if the number of connected synapses on this 
/// Segment that were active due to learning states at time t-1 is 
/// greater than activationThreshold.   
bool Segment::GetWasActiveFromLearning()
{
	int numberSynapsesWasActive = 0;
	FastListIter synapses_iter(Synapses);
	Synapse *syn;

	for (syn = (Synapse*)(synapses_iter.Reset()); syn != NULL; syn = (Synapse*)(synapses_iter.Advance()))
	{
		numberSynapsesWasActive += syn->GetWasActiveFromLearning() ? 1 : 0;
	}
	return numberSynapsesWasActive >= ActiveThreshold;
}

/// Methods

/// Initializes a new instance of the Segment class.
/// activeThreshold: A threshold number of active synapses between 
/// active and non-active Segment state.
void Segment::Initialize(int creationTime, float activeThreshold)
{
	IsSequence = false;
	_numPredictionSteps = -1;
	ActiveThreshold = activeThreshold;
	ConnectedSynapsesCount = 0;
	ActiveConnectedSynapsesCount = 0;
	PrevActiveConnectedSynapsesCount = 0;
	ActiveLearningSynapsesCount = 0;
	PrevActiveLearningSynapsesCount = 0;
	InactiveWellConnectedSynapsesCount = 0;
	IsActive = false;
	WasActive = false;
	CreationTime = creationTime;
}

/// Advance this segment to the next time step.
///
/// The current state of this segment (active, number of synapes) will be set as 
/// the previous state and the current state will be reset to no cell activity by 
/// default until it can be determined.
void Segment::NextTimeStep()
{
	WasActive = IsActive;
	IsActive = false;
	PrevConnectedSynapsesCount = ConnectedSynapsesCount;
	PrevActiveConnectedSynapsesCount = ActiveConnectedSynapsesCount;
	ActiveConnectedSynapsesCount = 0;
	PrevActiveLearningSynapsesCount = ActiveLearningSynapsesCount;
	ActiveLearningSynapsesCount = 0;
	PrevActiveSynapses.Clear();
	ActiveSynapses.TransferContentsTo(PrevActiveSynapses);
	InactiveWellConnectedSynapsesCount = 0;
}

/// Process this segment for the current time step.
///
/// Processing will determine the set of active synapses on this segment for this time 
/// step.  It will also keep count of how many of those active synapses are also connected.
/// From there we will determine if this segment is active if enough active connected
/// synapses are present.  This information is then cached for the remainder of the
/// Region's processing for the time step.  When a new time step occurs, the
/// Region will call nextTimeStep() on all cells/segments to cache the 
/// information as which synapses were previously active.
void Segment::ProcessSegment()
{
	ConnectedSynapsesCount = 0;
	ActiveConnectedSynapsesCount = 0;
	ActiveLearningSynapsesCount = 0;
	InactiveWellConnectedSynapsesCount = 0;

	ActiveSynapses.Clear();

	FastListIter synapses_iter(Synapses);
	for (Synapse *syn = (Synapse*)(synapses_iter.Reset()); syn != NULL; syn = (Synapse*)(synapses_iter.Advance()))
	{
		if (syn->GetIsActive())
		{
			ActiveSynapses.InsertAtEnd(syn);

			if (syn->GetIsConnected()) {
				ActiveConnectedSynapsesCount++;
			}

			if (syn->GetIsActiveFromLearning()) {
				ActiveLearningSynapsesCount++;
			}
		}
		else
		{
			if (syn->GetPermanence() > syn->Params->InitialPermanence) {
				InactiveWellConnectedSynapsesCount++;
			}
		}

		if (syn->GetIsConnected()) {
			ConnectedSynapsesCount++;
		}
	}

	IsActive = (ActiveConnectedSynapsesCount >= ActiveThreshold);
}

/// Create a new proximal synapse for this segment attached to the specified 
/// input cell.
/// inputSource: the input source of the synapse to create.
/// initPerm: the initial permanence of the synapse.
/// Returns the newly created synapse.
ProximalSynapse *Segment::CreateProximalSynapse(SynapseParameters *params, DataSpace *inputSource, DataPoint &inputPoint, float permanence, float distanceToInput)
{
	ProximalSynapse *newSyn = (ProximalSynapse*)(mem_manager.GetObject(MOT_PROXIMAL_SYNAPSE));
	newSyn->Initialize(params, inputSource, inputPoint, permanence, distanceToInput);
	Synapses.InsertAtEnd(newSyn);
	return newSyn;
}

/// Create a new synapse for this segment attached to the specified input source.
/// inputSource: the input source of the synapse to create.
/// initPerm: the initial permanence of the synapse.
/// Returns the newly created synapse.
DistalSynapse *Segment::CreateDistalSynapse(SynapseParameters *params, Cell *inputSource, float initPerm)
{
	DistalSynapse *newSyn = (DistalSynapse*)(mem_manager.GetObject(MOT_DISTAL_SYNAPSE));
	newSyn->Initialize(params, inputSource, initPerm);
	Synapses.InsertAtEnd(newSyn);
	return newSyn;
}

/// Create numSynapses new synapses attached to the specified
/// set of learning cells.
/// synapseCells: Set of available learning cells to form synapses to.
/// added: Set will be populated with synapses that were successfully added.
void Segment::CreateSynapsesToLearningCells(FastList &synapseCells, SynapseParameters *params)
{
	DistalSynapse *newSyn;

	// Assume that cells were previously checked to prevent adding
	// synapses to the same cell more than once per segment.
	FastListIter cells_iter(synapseCells);
	for (Cell *cell = (Cell*)(cells_iter.Reset()); cell != NULL; cell = (Cell*)(cells_iter.Advance()))
	{
		newSyn = CreateDistalSynapse(params, cell, params->InitialPermanence);
	}
}

/// Return a count of how many synapses on this segment (whether connected or not) 
/// are active in the current time step.
int Segment::GetActiveSynapseCount()
{
	return ActiveSynapses.Count();
}

/// Return a count of how many synapses on this segment (whether connected or not) 
/// were active in the previous time step.
int Segment::GetPrevActiveSynapseCount()
{
	return PrevActiveSynapses.Count();
}

/// Update all permanence values of each synapse based on current activity.
/// If a synapse is active, increase its permanence, else decrease it.
void Segment::AdaptPermanences()
{
	FastListIter synapses_iter(Synapses);
	for (Synapse *syn = (Synapse*)(synapses_iter.Reset()); syn != NULL; syn = (Synapse*)(synapses_iter.Advance()))
	{
		if (syn->GetIsActive())
		{
			syn->IncreasePermanence();
		}
		else
		{
			syn->DecreasePermanence();
		}
	}
}

/// Update (increase or decrease based on whether the synapse is active)
/// all permanence values of each of the synapses in the specified set.
void Segment::UpdatePermanences(FastList &activeSynapses)
{
	Synapse *syn;
	FastListIter synapses_iter;

	// Decrease all synapses first...
	synapses_iter.SetList(Synapses);
	for (syn = (Synapse*)(synapses_iter.Reset()); syn != NULL; syn = (Synapse*)(synapses_iter.Advance()))
	{
		syn->DecreasePermanenceNoLimit();
	}

	// Then for each active synapse, undo its decrement and add an increment.
	synapses_iter.SetList(activeSynapses);
	for (syn = (Synapse*)(synapses_iter.Reset()); syn != NULL; syn = (Synapse*)(synapses_iter.Advance()))
	{
		syn->IncreasePermanence(syn->Params->PermanenceDec + syn->Params->PermanenceInc);
	}

	// No make sure that all synapse permanence values are >= 0, since the decrement was done without enforcing a limit 
	// (so as to avoid incorrect amount of increment).
	synapses_iter.SetList(Synapses);
	for (syn = (Synapse*)(synapses_iter.Reset()); syn != NULL; syn = (Synapse*)(synapses_iter.Advance()))
	{
		syn->LimitPermanenceAfterDecrease();
	}
}

/// Decrease the permanences of each of the synapses in the set of
/// active synapses that happen to be on this segment.
void Segment::DecreasePermanences(FastList &activeSynapses)
{
	// Decrease the permanence of each synapse on this segment.
	FastListIter synapses_iter(activeSynapses);
	for (Synapse *syn = (Synapse*)(synapses_iter.Reset()); syn != NULL; syn = (Synapse*)(synapses_iter.Advance()))
	{
		syn->DecreasePermanence();
	}
}