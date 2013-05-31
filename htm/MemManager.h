#pragma once

#include "MemObject.h"
#include "MemObjectType.h"

class MemManager
{
public:
  MemManager();
  ~MemManager();

  void Reset();

  MemObject *GetObject(MemObjectType _object_type);
  void ReleaseObject(MemObject *_object);

  void ReleaseAll(MemObjectType _object_type);

  int GetMemUse(MemObjectType _object_type);
  int GetTotalMemUse();

  int GetObjectCount(MemObjectType _object_type);
  int GetTotalObjectCount();

  int GetFreeObjectCount(MemObjectType _object_type);
  int GetTotalFreeObjectCount();

private:

  void NewChunk(MemObjectType _object_type);

private:

	class ChunkNode
	{
	public:
		ChunkNode() : chunk(NULL), next(NULL) {};
		MemObject *chunk;
		ChunkNode *next;
	};

  // Object memory use arrays
  int countArray[NUM_MEM_OBJECT_TYPES];
  int freeCountArray[NUM_MEM_OBJECT_TYPES];
  int memUseArray[NUM_MEM_OBJECT_TYPES];

  // Object free lists
  MemObject* freeListArray[NUM_MEM_OBJECT_TYPES];

  // Object chunk arrays
  ChunkNode* chunkArray[NUM_MEM_OBJECT_TYPES];

	static bool releasing_all;
};

