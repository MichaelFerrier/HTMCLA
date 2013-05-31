#include <crtdbg.h>
#include "ProximalSynapse.h"
#include "DataSpace.h"


ProximalSynapse::ProximalSynapse(void)
{
}

ProximalSynapse::~ProximalSynapse(void)
{
}

/// Returns true if this ProximalSynapse is active due to the current input.
bool ProximalSynapse::GetIsActive()
{
	return InputSource->GetIsActive(InputPoint.X, InputPoint.Y, InputPoint.Index);
}

/// Methods

/// Initializes a new instance of the ProximalSynapse class and 
/// sets its input source and initial permanance values.
/// inputSource: A DataSource (external data source, or another Region) providing source of the input to this synapse.
/// inputPoint: Coordinates and value index of this synapse's input within the inputSource. 
/// permanence: Initial permanence value.
/// distanceToInput: In the Region's hypercolumn coordinates; used by Region::AverageReceptiveFieldSize().
void ProximalSynapse::Initialize(SynapseParameters *params, DataSpace *inputSource, DataPoint &inputPoint, float permanence, float distanceToInput)
{
	Synapse::Initialize(params);

	InputSource = inputSource;
	InputPoint = inputPoint;
	DistanceToInput = distanceToInput;

	SetPermanence(permanence);
}

// This version of Initialize() is used when loading data.
void ProximalSynapse::Initialize(SynapseParameters *params)
{
	Synapse::Initialize(params);

	InputSource = NULL;
	InputPoint = DataPoint();
	DistanceToInput = 0.0f;

	SetPermanence(0.0f);
}
