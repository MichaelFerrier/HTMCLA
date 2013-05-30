#include <crtdbg.h>
#include "Synapse.h"
#include "Utils.h"

Synapse::Synapse(void)
{
}

Synapse::~Synapse(void)
{
}

void Synapse::Initialize(SynapseParameters *params)
{
  Params = params;
	IsActive = false;
	WasActive = false;
	WasActiveFromLearning = false;
	IsConnected = false;
	Permanence = 0.0f;
}

/// Decrease the permance value of the synapse
void Synapse::DecreasePermanence()
{
	SetPermanence(Max(0.0f, Permanence - Params->PermanenceDec));
}

/// Decrease the permance value of the synapse
/// amount: Amount to decrease
void Synapse::DecreasePermanence(float amount, float min)
{
	SetPermanence(Max(min, Permanence - amount));
}

/// Decrease the permance value of the synapse, with no lower limit.
void Synapse::DecreasePermanenceNoLimit()
{
	SetPermanence(Permanence - Params->PermanenceDec);
}

/// Limit permanence value to >= 0. This is done after a call to DecreasePermanenceNoLimit.
void Synapse::LimitPermanenceAfterDecrease()
{
	if (Permanence < 0.0f) {
		Permanence = 0.0f;
	}
}

/// Increase the permanence value of the synapse
void Synapse::IncreasePermanence()
{
	SetPermanence(Min(1.0f, Permanence + Params->PermanenceInc));
}

/// Increase the permanence value of the synapse
/// amount: Amount to increase
void Synapse::IncreasePermanence(float amount, float max)
{
	SetPermanence(Min(max, Permanence + amount));
}
