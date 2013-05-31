#pragma once
#include "DataSpace.h"
#include <QtGui/QImage>
#include <QtGui/QPainter>

enum PatternType
{
	PATTERN_NONE,
	PATTERN_STRIPE,
	PATTERN_BOUNCING_STRIPE,
	PATTERN_BAR,
	PATTERN_BOUNCING_BAR,
	PATTERN_TEXT,
	PATTERN_BITMAP,
	PATTERN_IMAGE
};

enum PatternImageFormat
{
	PATTERN_IMAGE_FORMAT_UNDEF,
	PATTERN_IMAGE_FORMAT_SPREADSHEET
};

enum PatternImageMotion
{
	PATTERN_IMAGE_MOTION_NONE,
	PATTERN_IMAGE_MOTION_ACROSS,
	PATTERN_IMAGE_MOTION_ACROSS2
};

class ImageInfo
{
public:
	QString label;
	int width, height, contentX, contentY, contentWidth, contentHeight;
	float *data;
};

class PatternInfo
{
public:
	PatternInfo() : type(PATTERN_NONE), startTime(-1), endTime(-1), minTrialDuration(1), maxTrialDuration(1), string(""), imageMotion(PATTERN_IMAGE_MOTION_NONE), trialCount(0), curTrialStartTime(-1), nextTrialStartTime(-1) {};
	PatternInfo(PatternType _type, int _startTime, int _endTime, int _minTrialDuration, int _maxTrialDuration, QString &_string, PatternImageMotion _imageMotion, std::vector<int*> &_bitmaps, std::vector<ImageInfo*> &_images) : type(_type), startTime(_startTime), endTime(_endTime), minTrialDuration(_minTrialDuration), maxTrialDuration(_maxTrialDuration), string(_string), imageMotion(_imageMotion), trialCount(0), curTrialStartTime(-1), nextTrialStartTime(-1), bitmaps(_bitmaps), images(_images) {};
	PatternInfo(PatternInfo &_original) {type = _original.type; startTime = _original.startTime, endTime = _original.endTime; minTrialDuration = _original.minTrialDuration; maxTrialDuration = _original.maxTrialDuration; string = _original.string; imageMotion = _original.imageMotion; trialCount = 0; curTrialStartTime = -1; nextTrialStartTime = -1;}

	PatternType type;
	PatternImageFormat imageFormat;
	PatternImageMotion imageMotion;
	int startTime, endTime;
	int minTrialDuration, maxTrialDuration;
	int startX, startY, endX, endY;
	QString string;
	std::vector<int*> bitmaps;
	std::vector<ImageInfo*> images;
	int *buffer;

	int trialCount, curTrialStartTime, nextTrialStartTime;
};

class InputSpace
	: public DataSpace
{
public:
	InputSpace(QString &_id, int _sizeX, int _sizeY, int _numValues, std::vector<PatternInfo*> &_patterns);
	~InputSpace(void);

	// Properties

	int sizeX, sizeY, numValues, testPatterns;
	int rowSize;
	int *data, *buffer;

	std::vector<PatternInfo*> patterns;

	QImage *image;
	QPainter *painter;

	// Methods

	DataSpaceType GetDataSpaceType() {return DATASPACE_TYPE_INPUTSPACE;}

	int GetSizeX();
	int GetSizeY();
	int GetNumValues();
	int GetHypercolumnDiameter();

	bool GetIsActive(int _x, int _y, int _index);
	void SetIsActive(int _x, int _y, int _index, bool _active);
	void DeactivateAll();

	void ApplyPatterns(int _time);
	void ApplyPattern(PatternInfo *_pattern, int _time);
};

