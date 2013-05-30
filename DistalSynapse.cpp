#include <crtdbg.h>
#include "DistalSynapse.h"
#include "Cell.h"

DistalSynapse::DistalSynapse(void)
{
}


DistalSynapse::~DistalSynapse(void)
{
}

/// Returns true if this DistalSynapse is active due to the current input.
bool DistalSynapse::GetIsActive() 
{
	_ASSERT(InputSource != NULL);
	return InputSource->GetIsActive();
}

/// Returns true if this DistalSynapse was active due to the previous input at t-1. 
bool DistalSynapse::GetWasActive()
{
	_ASSERT(InputSource != NULL);
	return InputSource->GetWasActive();
}

/// Returns true if this DistalSynapse was active due to the input
/// previously being in a learning state. 
bool DistalSynapse::GetWasActiveFromLearning()
{
	_ASSERT(InputSource != NULL);
	return (InputSource->GetWasActive()) && (InputSource->GetWasLearning());
}

/// Returns true if this DistalSynapse is active due to the input
/// currently being in a learning state. 
bool DistalSynapse::GetIsActiveFromLearning()
{
	_ASSERT(InputSource != NULL);
	return (InputSource->GetIsActive()) && (InputSource->GetIsLearning());
}

/// Methods

/// Initializes a new instance of the DistalSynapse class and 
/// sets its input source and initial permanance values.
/// inputSrc: An object providing source of the input to this synapse 
/// (a Column's Cell).
/// permanence: Initial permanence value.
void DistalSynapse::Initialize(SynapseParameters *params, Cell *inputSrc, float permanence)
{
	Synapse::Initialize(params);

	InputSource = inputSrc;
	SetPermanence(permanence);
}

// This version of Initialize() is used when loading data from disk.
void DistalSynapse::Initialize(SynapseParameters *params)
{
	Synapse::Initialize(params);

	InputSource = NULL;
	SetPermanence(0.0f);
}
