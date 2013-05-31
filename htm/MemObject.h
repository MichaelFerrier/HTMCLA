#pragma once

#include "MemObjectType.h"

class MemObject
{
  public:
    virtual MemObjectType GetMemObjectType()=0;
    virtual void Initialize() {}; // Prepare the object to be used/reused
		virtual void Retire() {}; // Prepare the object to be released

	public:
		MemObject *mem_next;
};

