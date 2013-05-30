#pragma once
#include <QtCore/QStringList>
#include "DataSpace.h"

class InputSpace;
class Region;

class Classifier
	: public DataSpace
{
public:
	Classifier(QString &_id, int _numitems, QString _regionID, QString _inputspaceID, QStringList &_labels);
	~Classifier(void) {};

	// Properties

	QString regionID, inputspaceID;
	InputSpace *inputspace;
	Region *region;
	int numItems;
	QStringList labels;

	// Methods

	DataSpaceType GetDataSpaceType() {return DATASPACE_TYPE_CLASSIFIER;}

	int GetSizeX();
	int GetSizeY();
	int GetNumValues();
	int GetHypercolumnDiameter();

	bool GetIsActive(int _x, int _y, int _index);

	void SetInputSpace(InputSpace *_inputspace) {inputspace = _inputspace;}
	void SetRegion(Region *_region) {region = _region;}
};

