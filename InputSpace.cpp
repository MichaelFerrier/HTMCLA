#include "InputSpace.h"
#include "Utils.h"
#include <string.h>
#include <crtdbg.h>

InputSpace::InputSpace(QString &_id, int _sizeX, int _sizeY, int _numValues, std::vector<PatternInfo*> &_patterns)
	: DataSpace(_id), image(NULL), patterns(_patterns)
{
	sizeX = _sizeX;
	sizeY = _sizeY;
	numValues = _numValues;

	rowSize = sizeX * numValues;

	// Create data array.
	data = new int[_sizeX * _sizeY * _numValues];

	// Create image processing buffer.
	buffer = new int[_sizeX * _sizeY];

	// Initialize all values to 0.
	DeactivateAll();
}

InputSpace::~InputSpace(void)
{
	// Delete data array.
	delete [] data;

	// Delete image processing buffer.
	delete [] buffer;

	// Delete all PatternInfo objects in patterns array.
	PatternInfo *curPattern;
	while (patterns.size() > 0)
	{
		curPattern = patterns.back();
		patterns.pop_back();
		delete curPattern;
	}
}

int InputSpace::GetSizeX()
{
	return sizeX;
}

int InputSpace::GetSizeY()
{
	return sizeY;
}

int InputSpace::GetNumValues()
{
	return numValues;
}

int InputSpace::GetHypercolumnDiameter() 
{
	return 1;
}

bool InputSpace::GetIsActive(int _x, int _y, int _index)
{
	_ASSERT((_x >= 0) && (_x < sizeX));
	_ASSERT((_y >= 0) && (_y < sizeY));
	_ASSERT((_index >= 0) && (_index < numValues));

	return (data[(_y * rowSize) + (_x * numValues) + _index] != 0);
}

void InputSpace::SetIsActive(int _x, int _y, int _index, bool _active)
{
	_ASSERT((_x >= 0) && (_x < sizeX));
	_ASSERT((_y >= 0) && (_y < sizeY));
	_ASSERT((_index >= 0) && (_index < numValues));

	data[(_y * rowSize) + (_x * numValues) + _index] = (_active ? 1 : 0);
}

void InputSpace::DeactivateAll()
{
	// Reset all values in data array to 0.
	memset(data, 0, sizeY * rowSize * sizeof(int));
}

void InputSpace::ApplyPatterns(int _time)
{
	PatternInfo *curPattern;

	for (int i = 0; i < patterns.size(); i++)
	{
		curPattern = patterns[i];

		if (((curPattern->startTime == -1) || (curPattern->startTime <= _time)) && ((curPattern->endTime == -1) || (curPattern->endTime >= _time)))
		{
			ApplyPattern(curPattern, _time);
			break;
		}
	}
}

void InputSpace::ApplyPattern(PatternInfo *_pattern, int _time)
{
	// If there is no pattern to apply, do nothing.
	if (_pattern->type == PATTERN_NONE) {
		return;
	}

	// Advance to next trial if necessary.
	if (_time >= _pattern->nextTrialStartTime)
	{
		_pattern->trialCount++;
		_pattern->curTrialStartTime = _time;
		
		if (_pattern->minTrialDuration == _pattern->maxTrialDuration) {
			_pattern->nextTrialStartTime = _time + _pattern->minTrialDuration;
		} else {
			_pattern->nextTrialStartTime = _time + _pattern->minTrialDuration + (rand() % (_pattern->maxTrialDuration - _pattern->minTrialDuration + 1));
		}
	}

	int x, y, x1, y1, i, numSteps, step;
	float scale;
	int* bitmap;
	ImageInfo *imageInfo;
	
	switch (_pattern->type)
	{
	case PATTERN_STRIPE:
		// Apply stripe test pattern.

		// If this isn't the start of a new trial, no need to update activity.
		if (_time != _pattern->curTrialStartTime) {
			break;
		}

		// Start by clearing all activity.
		DeactivateAll();

		numSteps = (sizeX + sizeY - 1);
		step = (_pattern->trialCount - 1) % numSteps;

		for (x = 0; x < sizeX; x++)
		{
			y = step - x;

			if (y < 0) break;
			if (y >= sizeY) continue;

			for (i = 0; i < numValues; i++) {
				SetIsActive(x, y, i, true);
			}
		}
		break;

	case PATTERN_BOUNCING_STRIPE:
		// Apply stripe test pattern.

		// If this isn't the start of a new trial, no need to update activity.
		if (_time != _pattern->curTrialStartTime) {
			break;
		}

		// Start by clearing all activity.
		DeactivateAll();

		numSteps = (sizeX + sizeY - 2);
		step = (_pattern->trialCount - 1) % (numSteps * 2);

		// Reverse direction of stripe if appropriate.
		if (step >= numSteps) {
			step = numSteps - (step - numSteps);
		}

		for (x = 0; x < sizeX; x++)
		{
			y = step - x;

			if (y < 0) break;
			if (y >= sizeY) continue;

			for (i = 0; i < numValues; i++) {
				SetIsActive(x, y, i, true);
			}
		}
		break;
	
	case PATTERN_BAR:
		// Apply bar test pattern.

		// If this isn't the start of a new trial, no need to update activity.
		if (_time != _pattern->curTrialStartTime) {
			break;
		}

		// Start by clearing all activity.
		DeactivateAll();

		numSteps = sizeX;
		step = (_pattern->trialCount - 1) % numSteps;

		for (y = 0; y < sizeY; y++)
		{
			for (i = 0; i < numValues; i++) {
				SetIsActive(step, y, i, true);
			}
		}
		break;

	case PATTERN_BOUNCING_BAR:
		// Apply bouncing bar test pattern.
	
		// If this isn't the start of a new trial, no need to update activity.
		if (_time != _pattern->curTrialStartTime) {
			break;
		}

		// Start by clearing all activity.
		DeactivateAll();

		numSteps = sizeX - 1;
		step = (_pattern->trialCount - 1) % (numSteps * 2);

		// Reverse direction of bar if appropriate.
		if (step >= numSteps) {
			step = numSteps - (step - numSteps);
		}

		for (y = 0; y < sizeY; y++)
		{
			for (i = 0; i < numValues; i++) {
				SetIsActive(step, y, i, true);
			}
		}
		break;

	case PATTERN_TEXT:
		// Apply text test pattern.

		// If this isn't the start of a new trial, no need to update activity.
		if (_time != _pattern->curTrialStartTime) {
			break;
		}

		// Start by clearing all activity.
		DeactivateAll();

		numSteps = _pattern->string.length();
		step = (numSteps > 0) ? ((_pattern->trialCount - 1) % numSteps) : 0;
		scale = 0.8 * ((float)sizeY / 8.0f);

		// Initialize image and painter if not yet done.
		if (image == NULL) 
		{
			image = new QImage(sizeX, sizeY, QImage::Format_Mono);
			painter = new QPainter(image);
			painter->setPen(QColor(255,255,255));
			painter->setFont(QFont("Times", 10, QFont::Bold));
			painter->scale(scale, scale);
		}

		// Draw the text to the image.
		image->fill(0);
		painter->drawText(QRect(0, 0, (float)sizeX / scale, (float)sizeY / scale), Qt::AlignCenter, _pattern->string.mid(step, 1)); 

		for (y = 0; y < sizeY; y++)
		{
			for (x = 0; x < sizeX; x++)
			{
				if (QColor(image->pixel(x,y)).lightness() >= 128) 
				{
					for (i = 0; i < numValues; i++) {
						SetIsActive(x, y, i, true);
					}
				}				
			}
		}
		break;

	case PATTERN_BITMAP:
		// Apply bitmap test pattern.

		// If this isn't the start of a new trial, no need to update activity.
		if (_time != _pattern->curTrialStartTime) {
			break;
		}

		// Start by clearing all activity.
		DeactivateAll();

		// If there are no bitmaps given for this TestPattern, do nothing.
		if (_pattern->bitmaps.size() == 0) {
			break;
		}

		numSteps = (int)(_pattern->bitmaps.size());
		step = (numSteps > 0) ? ((_pattern->trialCount - 1) % numSteps) : 0;
		
		// Get a pointer to the current bitmap array.
		bitmap = _pattern->bitmaps[step];

		for (y = 0; y < sizeY; y++)
		{
			for (x = 0; x < sizeX; x++)
			{
				if (bitmap[y * sizeX + x] != 0) 
				{
					for (i = 0; i < numValues; i++) {
						SetIsActive(x, y, i, true);
					}
				}				
			}
		}
		break;
	case PATTERN_IMAGE:
		// Apply image test pattern.

		// If there are no images given for this Pattern, do nothing.
		if (_pattern->images.size() == 0) {
			break;
		}

		numSteps = (int)(_pattern->images.size());
		step = (numSteps > 0) ? ((_pattern->trialCount - 1) % numSteps) : 0;
		
		// Get a pointer to the current ImageInfo.
		imageInfo = _pattern->images[step];

		// If this is the start of a new trial...
		if (_time == _pattern->curTrialStartTime) 
		{
			if (_pattern->imageMotion == PATTERN_IMAGE_MOTION_ACROSS)
			{
				// Determine start and end coordinates.
				if ((rand() % 2) == 0)
				{
					// Horizontal movement.

					if ((rand() % 2) == 0)  
					{
						// Left to right
						_pattern->startX = 0;
						_pattern->endX = sizeX - imageInfo->contentWidth;
					} 
					else 
					{
						// Right to left
						_pattern->startX = sizeX - imageInfo->contentWidth;
						_pattern->endX = 0;
					}

					// Choose start and end Y positions.
					_pattern->startY = (sizeY <= imageInfo->contentHeight) ? 0 : (rand() % (sizeY - imageInfo->contentHeight));
					_pattern->endY = (sizeY <= imageInfo->contentHeight) ? 0 : (rand() % (sizeY - imageInfo->contentHeight));
				}
				else
				{
					// Vertical movement.

					if ((rand() % 2) == 0)  
					{
						// Top to bottom
						_pattern->startY = 0;
						_pattern->endY = sizeY - imageInfo->contentHeight;
					} 
					else 
					{
						// Bottom to top
						_pattern->startY = sizeY - imageInfo->contentHeight;
						_pattern->endY = 0;
					}

					// Choose start and end X positions.
					_pattern->startX = (sizeX <= imageInfo->contentWidth) ? 0 : (rand() % (sizeX - imageInfo->contentWidth));
					_pattern->endX = (sizeX <= imageInfo->contentWidth) ? 0 : (rand() % (sizeX - imageInfo->contentWidth));
				}
			}
			else if (_pattern->imageMotion == PATTERN_IMAGE_MOTION_ACROSS2)
			{
				// Determine start and end coordinates.
				if ((rand() % 2) == 0)
				{
					// Horizontal movement.

					if ((rand() % 2) == 0)  
					{
						// Left to right
						_pattern->startX = 0;
						_pattern->endX = sizeX - imageInfo->contentWidth;
					} 
					else 
					{
						// Right to left
						_pattern->startX = sizeX - imageInfo->contentWidth;
						_pattern->endX = 0;
					}

					// Choose start and end Y position.
					_pattern->startY = _pattern->endY = (sizeY <= imageInfo->contentHeight) ? 0 : (rand() % (sizeY - imageInfo->contentHeight));
				}
				else
				{
					// Vertical movement.

					if ((rand() % 2) == 0)  
					{
						// Top to bottom
						_pattern->startY = 0;
						_pattern->endY = sizeY - imageInfo->contentHeight;
					} 
					else 
					{
						// Bottom to top
						_pattern->startY = sizeY - imageInfo->contentHeight;
						_pattern->endY = 0;
					}

					// Choose start and end X position.
					_pattern->startX = _pattern->endX = (sizeX <= imageInfo->contentWidth) ? 0 : (rand() % (sizeX - imageInfo->contentWidth));
				}
			}
			else
			{
				// Image doesn't move.
				_pattern->startX = _pattern->startY = _pattern->endX = _pattern->endY = 0;
			}
		}

		// Start by clearing all activity.
		DeactivateAll();

		// Clear the image processing buffer.
		memset(buffer, 0, sizeX * sizeY * sizeof(int));

		// Determine current image position.
		int posX, posY;
		if (_pattern->imageMotion == PATTERN_IMAGE_MOTION_ACROSS)
		{
			posX = (int)(((float)(_pattern->endX - _pattern->startX)) / ((float)(_pattern->nextTrialStartTime - _pattern->curTrialStartTime - 1)) * ((float)(_time - _pattern->curTrialStartTime)) + _pattern->startX + 0.5f);
			posY = (int)(((float)(_pattern->endY - _pattern->startY)) / ((float)(_pattern->nextTrialStartTime - _pattern->curTrialStartTime - 1)) * ((float)(_time - _pattern->curTrialStartTime)) + _pattern->startY + 0.5f);
		}
		else if (_pattern->imageMotion == PATTERN_IMAGE_MOTION_ACROSS2)
		{
			if (_pattern->endX > _pattern->startX)
			{
				posY = _pattern->startY;
				posX = _pattern->startX + (_time - _pattern->curTrialStartTime);
				if (posX > _pattern->endX) posX = _pattern->endX - (posX - _pattern->endX);
			}
			else if (_pattern->endX < _pattern->startX)
			{
				posY = _pattern->startY;
				posX = _pattern->startX - (_time - _pattern->curTrialStartTime);
				if (posX < _pattern->endX) posX = _pattern->endX + (_pattern->endX - posX);
			}
			if (_pattern->endY > _pattern->startY)
			{
				posX = _pattern->startX;
				posY = _pattern->startY + (_time - _pattern->curTrialStartTime);
				if (posY > _pattern->endY) posY = _pattern->endY - (posY - _pattern->endY);
			}
			else if (_pattern->endY < _pattern->startY)
			{
				posX = _pattern->startX;
				posY = _pattern->startY - (_time - _pattern->curTrialStartTime);
				if (posY < _pattern->endY) posY = _pattern->endY + (_pattern->endY - posY);
			}
		}
		else
		{
			posX = _pattern->startX;
			posY = _pattern->startY;
		}

		int destX, destY, srcX, srcY;

		for (y = 0; y < imageInfo->contentHeight; y++)
		{
			destY = posY + y;

			if (destY < 0) continue;
			if (destY >= sizeY) break;

			srcY = imageInfo->contentY + y;

			for (x = 0; x < imageInfo->contentWidth; x++)
			{
				destX = posX + x;

				if (destX < 0) continue;
				if (destX >= sizeX) break;

				srcX = imageInfo->contentX + x;	

				if (imageInfo->data[srcY * imageInfo->width + srcX] > 0) 
				{
					buffer[destX + (destY * sizeX)] = 1;
				}				
			}
		}

		bool cur_on, center_on;
		int surround_on_count, surround_off_count;

		for (x = 0; x < sizeX; x++)
		{
			for (y = 0; y < sizeY; y++)
			{
				surround_on_count = 0;
				surround_off_count = 0;

				for (x1 = Max(0, x - 1); x1 < Min(sizeX, x + 2); x1++)
				{
					for (y1 = Max(0, y - 1); y1 < Min(sizeY, y + 2); y1++)
					{
						cur_on = (buffer[x1 + (y1 * sizeX)] == 1);

						if ((x1 == x) && (y1 == y))
						{
							center_on = cur_on;
						}
						else
						{
							if (cur_on) {
								surround_on_count++;
							} else {
								surround_off_count++;
							}
						}
					}
				}

				// On-center, off-surround in value 0.
				if (center_on && (surround_off_count >= 2)) {
					SetIsActive(x, y, 0, true);
				}

				// Off-center, on-surround in value 1.
				if ((GetNumValues() > 1) && (center_on == false) && (surround_on_count >= 2)) {
					SetIsActive(x, y, 1, true);
				}
			}
		}

		break;
	}
}