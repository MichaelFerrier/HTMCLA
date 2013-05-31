#include <crtdbg.h>
#include "MemManager.h"

// Object class header files
#include "FastHash.h"
#include "FastList.h"
#include "ProximalSynapse.h"
#include "DistalSynapse.h"
#include "Segment.h"
#include "SegmentUpdateInfo.h"
#include "Cell.h"

bool MemManager::releasing_all = false;

MemManager::MemManager()
{
  int i;
  
	// Initialize the countArray
  for (i = 0; i < NUM_MEM_OBJECT_TYPES; i++) {
    countArray[i] = 0;
  }

  // Initialize the freeCountArray
  for (i = 0; i < NUM_MEM_OBJECT_TYPES; i++) {
    freeCountArray[i] = 0;
  }

  // Initialize the memUseArray
  for (i = 0; i < NUM_MEM_OBJECT_TYPES; i++) {
    memUseArray[i] = 0;
  }
}

MemManager::~MemManager()
{
  Reset();
}

void MemManager::Reset()
{
  // Release all objects of every type
  for (int i = 0; i < NUM_MEM_OBJECT_TYPES; i++) 
  {
		ReleaseAll(i);

    // All objects should have been released!
    _ASSERT(freeCountArray[i] == countArray[i]);    
  }
}

MemObject *MemManager::GetObject(MemObjectType _object_type)
{
  _ASSERT(_object_type >= 0);
  _ASSERT(_object_type < NUM_MEM_OBJECT_TYPES);

  MemObject *curObject;
  
  // If there are no free objects of this type...
  if (freeListArray[_object_type] == NULL) {
    // Create and add a new chunk of objects
    NewChunk(_object_type);
  }

  if ((curObject = freeListArray[_object_type]) != NULL)
  {
		// Remove the object from the free list
		freeListArray[_object_type] = curObject->mem_next;
		curObject->mem_next = NULL;
		freeCountArray[_object_type]--; // Decrement free object count for this type

    // Initialize the object for use/reuse
    curObject->Initialize();

    return curObject;
  }
  else
  {
    // No object is available; return NULL.
    return NULL;
  }

  return NULL;
}

void MemManager::ReleaseObject(MemObject *_object)
{
  _ASSERT(_object != NULL);

	// Do not release this single object if currently in the process of releasing all objects.
	if (releasing_all) {
		return;
	}

  MemObjectType object_type = _object->GetMemObjectType();

  _ASSERT(object_type >= 0);
  _ASSERT(object_type < NUM_MEM_OBJECT_TYPES);
  _ASSERT(_object->mem_next == NULL); // Object should not be linked when released.

	 // Prepare the object to be released
   _object->Retire();

  // Add the given object to the appropriate free list
	_object->mem_next = freeListArray[object_type];
  freeListArray[object_type] = _object;
  freeCountArray[object_type]++; // Increment free object count for this type
}

void MemManager::ReleaseAll(MemObjectType _object_type)
{
  _ASSERT(_object_type >= 0);
  _ASSERT(_object_type < NUM_MEM_OBJECT_TYPES);

	// Record that all objects are in the process of being released
	releasing_all = true;

  // Clear this object type's countArray entry
  countArray[_object_type] = 0;

  // Clear this object type's freeCountArray entry
  freeCountArray[_object_type] = 0;

  // Clear this object type's memUseArray entry
  memUseArray[_object_type] = 0;

  // Empty this object type's free list
  freeListArray[_object_type] = NULL;

  // Delete all of this object type's chunks
	ChunkNode *cur_node;
	while ((cur_node = chunkArray[_object_type]) != NULL)
	{
		// Remove the current ChunkNode from the list
		chunkArray[_object_type] = cur_node->next;

		// Delete the chunk represented by the cur_node
		switch (_object_type)
    {
		case MOT_FAST_HASH_TRAY:
			delete [] (FastHashTray*)(cur_node->chunk);
      break;
		case MOT_FAST_LIST_TRAY:
			delete [] (FastListTray*)(cur_node->chunk);
      break;
		case MOT_PROXIMAL_SYNAPSE:
			delete [] (ProximalSynapse*)(cur_node->chunk);
      break;
		case MOT_DISTAL_SYNAPSE:
			delete [] (DistalSynapse*)(cur_node->chunk);
      break;
		case MOT_SEGMENT:
			delete [] (Segment*)(cur_node->chunk);
      break;
		case MOT_CELL:
			delete [] (Cell*)(cur_node->chunk);
      break;
		case MOT_SEGMENT_UPDATE_INFO:
			delete [] (SegmentUpdateInfo*)(cur_node->chunk);
      break;
		}

		// Delete the current ChunkNode itself
		delete cur_node;
	}

	// Done releasing all objects.
	releasing_all = false;
}

int MemManager::GetMemUse(MemObjectType _object_type)
{
  _ASSERT(_object_type >= 0);
  _ASSERT(_object_type < NUM_MEM_OBJECT_TYPES);

  return memUseArray[_object_type];
}

int MemManager::GetTotalMemUse()
{
  int totalMemUse = 0;

    // Sum up the memory usage of every type of object
  for (int i = 0; i < NUM_MEM_OBJECT_TYPES; i++) {
    totalMemUse += memUseArray[i];
  }

  return totalMemUse;
}

int MemManager::GetObjectCount(MemObjectType _object_type)
{
  _ASSERT(_object_type >= 0);
  _ASSERT(_object_type < NUM_MEM_OBJECT_TYPES);

  return countArray[_object_type];
}

int MemManager::GetTotalObjectCount()
{
  int totalObjectCount = 0;

  // Sum up the memory usage of every type of object
  for (int i = 0; i < NUM_MEM_OBJECT_TYPES; i++) {
    totalObjectCount += countArray[i];
  }

  return totalObjectCount;
}

int MemManager::GetFreeObjectCount(MemObjectType _object_type)
{
  _ASSERT(_object_type >= 0);
  _ASSERT(_object_type < NUM_MEM_OBJECT_TYPES);

  return freeCountArray[_object_type];
}

int MemManager::GetTotalFreeObjectCount()
{
  int totalFreeObjectCount = 0;

  // Sum up the memory usage of every type of object
  for (int i = 0; i < NUM_MEM_OBJECT_TYPES; i++) {
    totalFreeObjectCount += freeCountArray[i];
  }

  return totalFreeObjectCount;
}

void MemManager::NewChunk(MemObjectType _object_type)
{
	// Create new ChunkNode and add it to the appropriate list
	ChunkNode *new_node = new ChunkNode();
	new_node->next = chunkArray[_object_type];
	chunkArray[_object_type] = new_node;

	switch (_object_type)
	{
  case MOT_FAST_HASH_TRAY:
		// Create the new chunk
		new_node->chunk = new FastHashTray[FAST_HASH_TRAY_CHUNK_LENGTH];

		// Add all of the new chunk's objects to the free list
		for (int i = 0; i < FAST_HASH_TRAY_CHUNK_LENGTH; i++)
		{
			((FastHashTray*)(new_node->chunk))[i].mem_next = freeListArray[_object_type];
			freeListArray[_object_type] = &(((FastHashTray*)(new_node->chunk))[i]);
		}
		freeCountArray[_object_type] += FAST_HASH_TRAY_CHUNK_LENGTH;
		countArray[_object_type] += FAST_HASH_TRAY_CHUNK_LENGTH;
		break;
  case MOT_FAST_LIST_TRAY:
		// Create the new chunk
		new_node->chunk = new FastListTray[FAST_LIST_TRAY_CHUNK_LENGTH];

		// Add all of the new chunk's objects to the free list
		for (int i = 0; i < FAST_LIST_TRAY_CHUNK_LENGTH; i++)
		{
			((FastListTray*)(new_node->chunk))[i].mem_next = freeListArray[_object_type];
			freeListArray[_object_type] = &(((FastListTray*)(new_node->chunk))[i]);
		}
		freeCountArray[_object_type] += FAST_LIST_TRAY_CHUNK_LENGTH;
		countArray[_object_type] += FAST_LIST_TRAY_CHUNK_LENGTH;
		break;
	case MOT_PROXIMAL_SYNAPSE:
		// Create the new chunk
		new_node->chunk = new ProximalSynapse[PROXIMAL_SYNAPSE_CHUNK_LENGTH];

		// Add all of the new chunk's objects to the free list
		for (int i = 0; i < PROXIMAL_SYNAPSE_CHUNK_LENGTH; i++)
		{
			((ProximalSynapse*)(new_node->chunk))[i].mem_next = freeListArray[_object_type];
			freeListArray[_object_type] = &(((ProximalSynapse*)(new_node->chunk))[i]);
		}
		freeCountArray[_object_type] += PROXIMAL_SYNAPSE_CHUNK_LENGTH;
		countArray[_object_type] += PROXIMAL_SYNAPSE_CHUNK_LENGTH;
		break;
  case MOT_DISTAL_SYNAPSE:
		// Create the new chunk
		new_node->chunk = new DistalSynapse[DISTAL_SYNAPSE_CHUNK_LENGTH];

		// Add all of the new chunk's objects to the free list
		for (int i = 0; i < DISTAL_SYNAPSE_CHUNK_LENGTH; i++)
		{
			((DistalSynapse*)(new_node->chunk))[i].mem_next = freeListArray[_object_type];
			freeListArray[_object_type] = &(((DistalSynapse*)(new_node->chunk))[i]);
		}
		freeCountArray[_object_type] += DISTAL_SYNAPSE_CHUNK_LENGTH;
		countArray[_object_type] += DISTAL_SYNAPSE_CHUNK_LENGTH;
		break;
	case MOT_SEGMENT:
		// Create the new chunk
		new_node->chunk = new Segment[SEGMENT_CHUNK_LENGTH];

		// Add all of the new chunk's objects to the free list
		for (int i = 0; i < SEGMENT_CHUNK_LENGTH; i++)
		{
			((Segment*)(new_node->chunk))[i].mem_next = freeListArray[_object_type];
			freeListArray[_object_type] = &(((Segment*)(new_node->chunk))[i]);
		}
		freeCountArray[_object_type] += SEGMENT_CHUNK_LENGTH;
		countArray[_object_type] += SEGMENT_CHUNK_LENGTH;
		break;
	case MOT_CELL:
		// Create the new chunk
		new_node->chunk = new Cell[CELL_CHUNK_LENGTH];

		// Add all of the new chunk's objects to the free list
		for (int i = 0; i < CELL_CHUNK_LENGTH; i++)
		{
			((Cell*)(new_node->chunk))[i].mem_next = freeListArray[_object_type];
			freeListArray[_object_type] = &(((Cell*)(new_node->chunk))[i]);
		}
		freeCountArray[_object_type] += CELL_CHUNK_LENGTH;
		countArray[_object_type] += CELL_CHUNK_LENGTH;
		break;
	case MOT_SEGMENT_UPDATE_INFO:
		// Create the new chunk
		new_node->chunk = new SegmentUpdateInfo[SEGMENT_UPDATE_INFO_CHUNK_LENGTH];

		// Add all of the new chunk's objects to the free list
		for (int i = 0; i < SEGMENT_UPDATE_INFO_CHUNK_LENGTH; i++)
		{
			((SegmentUpdateInfo*)(new_node->chunk))[i].mem_next = freeListArray[_object_type];
			freeListArray[_object_type] = &(((SegmentUpdateInfo*)(new_node->chunk))[i]);
		}
		freeCountArray[_object_type] += SEGMENT_UPDATE_INFO_CHUNK_LENGTH;
		countArray[_object_type] += SEGMENT_UPDATE_INFO_CHUNK_LENGTH;
		break;
	}
}
