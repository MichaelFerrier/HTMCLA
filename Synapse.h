#pragma once
#include "FastList.h"

class SynapseParameters
{
public:
	/// Synapses with permanences above this value are connected.
	float ConnectedPerm;

	/// Initial permanence for distal synapses.
	float InitialPermanence;

	/// Amount permanences of synapses are decremented in learning.
	float PermanenceDec;

	/// Amount permanences of synapses are incremented in learning.
	float PermanenceInc;

	SynapseParameters()
	{
		ConnectedPerm = 0.2f;
		InitialPermanence = ConnectedPerm + 0.1f;
		PermanenceDec = 0.015f;
		PermanenceInc = 0.015f;
	}
};

/// A data structure representing a synapse. Contains a permanence value to
/// indicate connectivity to a target cell.  
class Synapse
	: public MemObject
{
public:
	Synapse(void);
	virtual ~Synapse(void);

	/// Properties

protected:

	bool IsActive, WasActive, WasActiveFromLearning;
	bool IsConnected;
	float Permanence;

public: 

	SynapseParameters *Params;

	virtual MemObjectType GetMemObjectType()=0;

	virtual bool GetIsActive()=0;

	/// Returns true if this Synapse is currently connecting its source
	/// and destination Cells.
	bool GetIsConnected()
	{
		return IsConnected;
	}

	/// A value to indicate connectivity to a target cell.  
	float GetPermanence() {return Permanence;}
	void  SetPermanence(float value) {Permanence = value; IsConnected = (Permanence >= Params->ConnectedPerm);}

	virtual bool GetWasActive() {return false;}

	virtual bool GetWasActiveFromLearning() {return false;}
	virtual bool GetIsActiveFromLearning() {return false;}

	/// Methods

	void Initialize(SynapseParameters *params);

	/// Decrease the permance value of the synapse, limiting to >= 0.
	void DecreasePermanence();

	/// Decrease the permance value of the synapse.
	/// amount: Amount to decrease.
	/// min: Lower limit of permanence value.
	void DecreasePermanence(float amount, float min=0.0f);

	/// Decrease the permance value of the synapse, with no lower limit.
	void DecreasePermanenceNoLimit();

	/// Limit permanence value to >= 0. This is done after a call to DecreasePermanenceNoLimit.
	void LimitPermanenceAfterDecrease();

	/// Increase the permanence value of the synapse
	void IncreasePermanence();

	/// Increase the permanence value of the synapse
	/// amount: Amount to increase
	void IncreasePermanence(float amount, float max=1.0f);
};


