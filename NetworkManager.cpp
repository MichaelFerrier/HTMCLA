#include "NetworkManager.h"
#include "NetworkManager.h"
#include "Synapse.h"
#include "Cell.h"
#include <cstring>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QTextStream>

extern MemManager mem_manager;

NetworkManager::NetworkManager(void)
{
	filename = "";
	time = 0;
	networkLoaded = false;

	// Delete log file if it exists.
	QFile::remove("log.txt");
}

NetworkManager::~NetworkManager(void)
{
	// Clear the network.
	ClearNetwork();
}

void NetworkManager::ClearNetwork()
{
	InputSpace *inputSpace;
	Region *region;

	// First clear all data from the network.
	ClearData();

	// Delete all InputSpaces.
	while (inputSpaces.size() > 0)
	{
		inputSpace = inputSpaces.back();
		inputSpaces.pop_back();
		delete inputSpace;
	}

	// Delete all Regions.
	while (regions.size() > 0)
	{
		region = regions.back();
		regions.pop_back();
		delete region;
	}

	// Initialize members.
	filename = "";
	time = 0;
	networkLoaded = false;

	// Initialize random number generator, to have reproducibe results.
	srand(4242);
}

bool NetworkManager::LoadNetwork(QString &_filename, QXmlStreamReader &_xml, QString &_error_msg)
{
	Region *newRegion;
	InputSpace *newInputSpace;
	Classifier *newClassifier;
	bool result, proximalSynapseParamsFound = false, distalSynapseParamsFound = false;
	SynapseParameters defaultProximalSynapseParams, defaultDistalSynapseParams;

	// Clear the network.
	ClearNetwork();
	
	while(!_xml.atEnd() && !_xml.hasError()) 
	{
		// Read next element.
		QXmlStreamReader::TokenType token = _xml.readNext();

		// If token is just StartDocument, we'll go to next.
		if (token == QXmlStreamReader::StartDocument) {
				continue;
		}

		// If token is StartElement, we'll see if we can read it.
		if (token == QXmlStreamReader::StartElement) 
		{
			// If this is a ProximalSynapseParams element, read in the proximal synapse parameter information.
			if (_xml.name() == "ProximalSynapseParams") 
			{
				// Attempt to parse and store the proximal synapse parameters.
				result = ParseSynapseParams(_xml, defaultProximalSynapseParams, _error_msg);

				if (result == false) 
				{
					ClearNetwork();
					return false;
				}

				proximalSynapseParamsFound = true;
			}

			// If this is a DistalSynapseParams element, read in the distal synapse parameter information.
			if (_xml.name() == "DistalSynapseParams") 
			{
				// Attempt to parse and store the distal synapse parameters.
				result = ParseSynapseParams(_xml, defaultDistalSynapseParams, _error_msg);

				if (result == false) 
				{
					ClearNetwork();
					return false;
				}

				distalSynapseParamsFound = true;
			}

			// If this is a Region element, read in the Region's information.
			else if (_xml.name() == "Region") 
			{
				// Attempt to parse and create the new Region.
				newRegion = ParseRegion(_xml, defaultProximalSynapseParams, defaultDistalSynapseParams, _error_msg);

				if (newRegion == NULL) 
				{
					ClearNetwork();
					return false;
				}

				// Record the Region's index.
				newRegion->SetIndex((int)(regions.size()));

				// Add the new Region to the list of Regions.
				regions.push_back(newRegion);
			}

			// If this is a InputSpace element, read in the InputSpace's information.
			else if (_xml.name() == "InputSpace") 
			{
				// Attempt to parse and create the new InputSpace.
				newInputSpace = ParseInputSpace(_xml, _error_msg);

				if (newInputSpace == NULL) 
				{
					ClearNetwork();
					return false;
				}

				// Record the InputSpace's index.
				newInputSpace->SetIndex((int)(inputSpaces.size()));

				// Add the new InputSpace to the list of InputSpaces.
				inputSpaces.push_back(newInputSpace);
			}

			// If this is a Classifier element, read in the Classifier's information.
			else if (_xml.name() == "Classifier") 
			{
				// Attempt to parse and create the new Classifier.
				newClassifier = ParseClassifier(_xml, _error_msg);

				if (newClassifier == NULL) 
				{
					ClearNetwork();
					return false;
				}

				// Record the InputSpace's index.
				newClassifier->SetIndex((int)(classifiers.size()));

				// Add the new Classifier to the list of Classifiers.
				classifiers.push_back(newClassifier);
			}
		}
	}

	// Error handling.
	if(_xml.hasError()) 
	{
		_error_msg = _xml.errorString();
		ClearNetwork();
		return false;
	}

	// Require that the ProximalSynapseParams section has been found.
	if (proximalSynapseParamsFound == false)
	{
		_error_msg = "ProximalSynapseParams not found in file.";
		ClearNetwork();
		return false;
	}

	// Require that the DistalSynapseParams section has been found.
	if (distalSynapseParamsFound == false)
	{
		_error_msg = "DistalSynapseParams not found in file.";
		ClearNetwork();
		return false;
	}

	// Connect each Region with its input DataSpaces (InputSpaces or Regions).
	DataSpace *dataSpace;
	for (std::vector<Region*>::const_iterator region_iter = regions.begin(), end = regions.end(); region_iter != end; ++region_iter) 
	{
		for (std::vector<QString>::const_iterator input_id_iter = (*region_iter)->InputIDs.begin(), end = (*region_iter)->InputIDs.end(); input_id_iter != end; ++input_id_iter) 
		{
			// Get a pointer to the DataSpace with this Region's current input's ID.
			dataSpace = GetDataSpace(*input_id_iter);

			if (dataSpace == NULL)
			{
				_error_msg = "Region " + (*region_iter)->id + "'s input " + *input_id_iter + " is not a known Region or InputSpace.";
				ClearNetwork();
				return false;
			}

			// Add this DataSpace to this Region's list of inputs.
			(*region_iter)->InputList.push_back(dataSpace);
		}

		// Now that this Region has had its list of input DataSpaces filled in, initialize it.
		(*region_iter)->Initialize();
	}

	// Connect each Classifier with its InputSpace and Region.
	for (std::vector<Classifier*>::const_iterator classifier_iter = classifiers.begin(), end = classifiers.end(); classifier_iter != end; ++classifier_iter) 
	{
		// Get a pointer to the InputSpace with this Classifier's inputspaceID.
		(*classifier_iter)->inputspace = GetInputSpace((*classifier_iter)->inputspaceID);

		// Get a pointer to the Region with this Classifier's regionID.
		(*classifier_iter)->region = GetRegion((*classifier_iter)->regionID);

		if (((*classifier_iter)->inputspace == NULL) || ((*classifier_iter)->region == NULL))
		{
			_error_msg = "Classifier " + (*classifier_iter)->id + " must have both a valid region and inputspace.";
			return false;
		}
	}

	// Removes any device() or data from the XML reader and resets its internal state to the initial state.
	_xml.clear();

	// Record filename.
	filename = _filename;

	// Record that the network has been loaded.
	networkLoaded = true;

	// Success.
	return true;
}

bool NetworkManager::ParseSynapseParams(QXmlStreamReader &_xml, SynapseParameters &_synapseParams, QString &_error_msg)
{
	// Advance to the next element.
	_xml.readNext();
  
	// Loop through the SynapseParam elements (this allows the order to vary).
	// Continue the loop until we hit an EndElement named SynapseParams.
	for (;;)
	{
		// Get the current token name
		QString tokenName = _xml.name().toString().toLower();

		if ((_xml.tokenType() == QXmlStreamReader::EndElement) && ((tokenName == "proximalsynapseparams") || (tokenName == "distalsynapseparams"))) {
			break;
		}

		if (_xml.atEnd()) 
		{
			_error_msg = "Unexpected end of document.";
			return false;
		}

		if (_xml.tokenType() == QXmlStreamReader::StartElement) 
		{
			// InitialPermanence
			if (tokenName == "initialpermanence") 
			{
				_xml.readNext();
				if(_xml.tokenType() == QXmlStreamReader::Characters) {
					_synapseParams.InitialPermanence = _xml.text().toString().toFloat();
				}
			}

			// ConnectedPermanence
			else if (tokenName == "connectedpermanence") 
			{
				_xml.readNext();
				if(_xml.tokenType() == QXmlStreamReader::Characters) {
					_synapseParams.ConnectedPerm = _xml.text().toString().toFloat();
				}
			}

			// PermanenceIncrease
			else if (tokenName == "permanenceincrease") 
			{
				_xml.readNext();
				if(_xml.tokenType() == QXmlStreamReader::Characters) {
					_synapseParams.PermanenceInc = _xml.text().toString().toFloat();
				}
			}

			// PermanenceDecrease
			else if (tokenName == "permanencedecrease") 
			{
				_xml.readNext();
				if(_xml.tokenType() == QXmlStreamReader::Characters) {
					_synapseParams.PermanenceDec = _xml.text().toString().toFloat();
				}
			}
		}

		// Advance to next token...
		_xml.readNext();
	}

	if ((_synapseParams.InitialPermanence <= 0.0f) || (_synapseParams.InitialPermanence > 1.0f))
	{
		_error_msg = QString("InitialPermanence has invalid value of ") + _synapseParams.InitialPermanence + ".";
		return false;
	}

	if ((_synapseParams.ConnectedPerm <= 0.0f) || (_synapseParams.ConnectedPerm > 1.0f))
	{
		_error_msg = QString("ConnectedPermanence has invalid value of ") + _synapseParams.ConnectedPerm + ".";
		return false;
	}

	if ((_synapseParams.PermanenceInc <= 0.0f) || (_synapseParams.PermanenceInc > 1.0f))
	{
		_error_msg = QString("PermanenceIncrease has invalid value of ") + _synapseParams.PermanenceInc + ".";
		return false;
	}

	if ((_synapseParams.PermanenceDec <= 0.0f) || (_synapseParams.PermanenceDec > 1.0f))
	{
		_error_msg = QString("PermanenceDecrease has invalid value of ") + _synapseParams.PermanenceDec + ".";
		return false;
	}
		
	return true;
}

Region *NetworkManager::ParseRegion(QXmlStreamReader &_xml, SynapseParameters _proximalSynapseParams, SynapseParameters _distalSynapseParams, QString &_error_msg)
{
	QString id;
	int sizeX = 0, sizeY = 0, cellsPerColumn = 0, hypercolumnDiameter = 1;
	int spatialLearningStartTime = -1, spatialLearningEndTime = -1;
	int temporalLearningStartTime = -1, temporalLearningEndTime = -1;
	int boostingStartTime, boostingEndTime; 
	int min_MinOverlapToReuseSegment = -1, max_MinOverlapToReuseSegment = -1;
	bool hardcodedSpatial = false, outputColumnActivity = false, outputCellActivity = true; 
	float percentageInputPerCol = 0.0f, percentageMinOverlap = 0.0f, percentageLocalActivity = 0.0f, maxBoost = -1, boostRate = 0.01f;
	int predictionRadius = -1, segmentActivateThreshold = 0, newNumberSynapses = 0;
	InhibitionTypeEnum inhibitionType = INHIBITION_TYPE_AUTOMATIC;
	int inhibitionRadius = -1;
	Region *newRegion = NULL;
	std::vector<QString> input_ids;
	std::vector<int> input_radii;
	bool inputFound = false, result;
	QString temp_string;

	// Look for ID attribute
	QXmlStreamAttributes attributes = _xml.attributes();
	if (attributes.hasAttribute("id")) {
		id = attributes.value("id").toString();
	}

	// Advance to the next element.
	_xml.readNext();
  
	// Loop through this Region's elements (this allows the order to vary).
	// Continue the loop until we hit an EndElement named Region.
	for (;;)
	{
		// Get the current token name
		QString tokenName = _xml.name().toString().toLower();

		if ((_xml.tokenType() == QXmlStreamReader::EndElement) && (tokenName == "region")) {
			break;
		}

		if (_xml.atEnd()) 
		{
			_error_msg = "Unexpected end of document.";
			return NULL;
		}

		if (_xml.tokenType() == QXmlStreamReader::StartElement) 
		{
			// SizeX
			if (tokenName == "sizex") 
			{
				_xml.readNext();
				if(_xml.tokenType() == QXmlStreamReader::Characters) {
					sizeX = _xml.text().toString().toInt();
				}
			}

			// SizeY
			else if (tokenName == "sizey") 
			{
				_xml.readNext();
				if(_xml.tokenType() == QXmlStreamReader::Characters) {
					sizeY = _xml.text().toString().toInt();
				}
			}

			// CellsPerColumn
			else if (tokenName == "cellspercolumn") 
			{
				_xml.readNext();
				if(_xml.tokenType() == QXmlStreamReader::Characters) {
					cellsPerColumn = _xml.text().toString().toInt();
				}
			}

			// HypercolumnDiameter
			else if (tokenName == "hypercolumndiameter") 
			{
				_xml.readNext();
				if(_xml.tokenType() == QXmlStreamReader::Characters) {
					hypercolumnDiameter = _xml.text().toString().toInt();
				}
			}

			// PredictionRadius
			else if (tokenName == "predictionradius") 
			{
				_xml.readNext();
				if(_xml.tokenType() == QXmlStreamReader::Characters) {
					predictionRadius = _xml.text().toString().toInt();
				}
			}

			// SegmentActivateThreshold
			else if (tokenName == "segmentactivatethreshold") 
			{
				_xml.readNext();
				if(_xml.tokenType() == QXmlStreamReader::Characters) {
					segmentActivateThreshold = _xml.text().toString().toInt();
				}
			}

			// Inhibition
			else if (tokenName == "inhibition") 
			{
				attributes = _xml.attributes();

				if (attributes.hasAttribute("type")) 
				{
					if (attributes.value("type").toString().toLower() == "automatic") 
					{
						inhibitionType = INHIBITION_TYPE_AUTOMATIC;
					} 
					else if (attributes.value("type").toString().toLower() == "radius") 
					{
						inhibitionType = INHIBITION_TYPE_RADIUS;
					} 
					else 
					{
						_error_msg = "Region " + id + " has unknown inhibition type " + attributes.value("type").toString() + ".";
						return NULL;
					}
				}

				if (attributes.hasAttribute("radius")) {
					inhibitionRadius = attributes.value("radius").toString().toInt();
				}
			}

			// MinOverlapToReuseSegment
			else if (tokenName == "minoverlaptoreusesegment") 
			{
				attributes = _xml.attributes();

				if (attributes.hasAttribute("min")) {
					min_MinOverlapToReuseSegment = attributes.value("min").toString().toInt();
				}

				if (attributes.hasAttribute("max")) {
					max_MinOverlapToReuseSegment = attributes.value("max").toString().toInt();
				}
			}

			// NewNumberSynapses
			else if (tokenName == "newnumbersynapses") 
			{
				_xml.readNext();
				if(_xml.tokenType() == QXmlStreamReader::Characters) {
					newNumberSynapses = _xml.text().toString().toInt();
				}
			}

			// PercentageInputPerCol
			else if (tokenName == "percentageinputpercol") 
			{
				_xml.readNext();
				if(_xml.tokenType() == QXmlStreamReader::Characters) {
					percentageInputPerCol = _xml.text().toString().toFloat();
				}
			}

			// PercentageMinOverlap
			else if (tokenName == "percentageminoverlap") 
			{
				_xml.readNext();
				if(_xml.tokenType() == QXmlStreamReader::Characters) {
					percentageMinOverlap = _xml.text().toString().toFloat();
				}
			}

			// PercentageLocalActivity
			else if (tokenName == "percentagelocalactivity") 
			{
				_xml.readNext();
				if(_xml.tokenType() == QXmlStreamReader::Characters) {
					percentageLocalActivity = _xml.text().toString().toFloat();
				}
			}

			// Boost
			else if (tokenName == "boost") 
			{
				attributes = _xml.attributes();

				if (attributes.hasAttribute("max")) {
					maxBoost = attributes.value("max").toString().toFloat();
				}

				if (attributes.hasAttribute("rate")) {
					boostRate = attributes.value("rate").toString().toFloat();
				}
			}

			// SpatialLearningPeriod
			else if (tokenName == "spatiallearningperiod") 
			{
				attributes = _xml.attributes();

				if (attributes.hasAttribute("start")) {
					spatialLearningStartTime = attributes.value("start").toString().toInt();
				}

				if (attributes.hasAttribute("end")) {
					spatialLearningEndTime = attributes.value("end").toString().toInt();
				}
			}

			// TemporalLearningPeriod
			else if (tokenName == "temporallearningperiod") 
			{
				attributes = _xml.attributes();

				if (attributes.hasAttribute("start")) {
					temporalLearningStartTime = attributes.value("start").toString().toInt();
				}

				if (attributes.hasAttribute("end")) {
					temporalLearningEndTime = attributes.value("end").toString().toInt();
				}
			}

			// BoostingPeriod
			else if (tokenName == "boostingperiod") 
			{
				attributes = _xml.attributes();

				if (attributes.hasAttribute("start")) {
					boostingStartTime = attributes.value("start").toString().toInt();
				}

				if (attributes.hasAttribute("end")) {
					boostingEndTime = attributes.value("end").toString().toInt();
				}
			}

			// HardcodedSpatial
			else if (tokenName == "hardcodedspatial") 
			{
				_xml.readNext();
				if(_xml.tokenType() == QXmlStreamReader::Characters) {
					hardcodedSpatial = (_xml.text().toString().toLower() == "true");
				}
			}

			// OutputColumnActivity
			else if (tokenName == "outputcolumnactivity") 
			{
				_xml.readNext();
				if(_xml.tokenType() == QXmlStreamReader::Characters) {
					outputColumnActivity = (_xml.text().toString().toLower() == "true");
				}
			}

			// OutputCellActivity
			else if (tokenName == "outputcellactivity") 
			{
				_xml.readNext();
				if(_xml.tokenType() == QXmlStreamReader::Characters) {
					outputCellActivity = (_xml.text().toString().toLower() == "true");
				}
			}

			// ProximalSynapseParams
			else if (tokenName == "proximalsynapseparams") 
			{
				// Attempt to parse and store this Region's proximal synapse parameters.
				result = ParseSynapseParams(_xml, _proximalSynapseParams, _error_msg);

				if (result == false) {
					return false;
				}
			}

			// DistalSynapseParams
			else if (tokenName == "distalsynapseparams") 
			{
				// Attempt to parse and store this Region's distal synapse parameters.
				result = ParseSynapseParams(_xml, _distalSynapseParams, _error_msg);

				if (result == false) {
					return false;
				}
			}

			// Input
			else if (tokenName == "input") 
			{
				attributes = _xml.attributes();

				if (attributes.hasAttribute("id")) 
				{
					input_ids.push_back(attributes.value("id").toString());
					inputFound = true;
				}

				// Determine inputRadius.
				int inputRadius = -1;
				if (attributes.hasAttribute("radius")) 
				{
					inputRadius = attributes.value("radius").toString().toInt();					

					if ((inputRadius < -1) || (inputRadius > INPUTSPACE_MAX_SIZE))
					{
						_error_msg = "Region " + id + " has input with invalid InputRadius " + QString(inputRadius) + ".";
						return NULL;
					}
				}

				// Record inputRadius for this input DataSpace.
				input_radii.push_back(inputRadius);
			}
		}
		
		// Advance to next token...
		_xml.readNext();
	}

	if (id.length() == 0)
	{
		_error_msg = "Region missing ID.";
		return NULL;
	}

	if (!inputFound)
	{
		_error_msg = "Region " + id + " has no Input(s).";
		return NULL;
	}

	if ((sizeX <= 0) || (sizeX > INPUTSPACE_MAX_SIZE))
	{
		_error_msg = "Region " + id + " has invalid SizeX " + QString(sizeX) + ".";
		return NULL;
	}

	if ((sizeY <= 0) || (sizeY > INPUTSPACE_MAX_SIZE))
	{
		_error_msg = "Region " + id + " has invalid SizeY " + QString(sizeY) + ".";
		return NULL;
	}

	if ((cellsPerColumn <= 0) || (cellsPerColumn > INPUTSPACE_MAX_NUM_VALUES))
	{
		_error_msg = "Region " + id + " has invalid CellsPerColumn " + QString(cellsPerColumn) + ".";
		return NULL;
	}

	if ((hypercolumnDiameter <= 0) || (hypercolumnDiameter > Min(sizeX, sizeY)))
	{
		_error_msg = "Region " + id + " has invalid HypercolumnDiameter " + QString(hypercolumnDiameter) + ".";
		return NULL;
	}

	if ((sizeX % hypercolumnDiameter) != 0)
	{
		_error_msg = "Region " + id + " has SizeX " + QString(sizeX) + " that is not evenly divisible by its HypercolumnDiameter " + QString(hypercolumnDiameter) + ".";
		return NULL;
	}

	if ((sizeY % hypercolumnDiameter) != 0)
	{
		_error_msg = "Region " + id + " has SizeY " + QString(sizeY) + " that is not evenly divisible by its HypercolumnDiameter " + QString(hypercolumnDiameter) + ".";
		return NULL;
	}

	if ((predictionRadius < -1) || (predictionRadius > INPUTSPACE_MAX_SIZE))
	{
		_error_msg = "Region " + id + " has invalid PredictionRadius " + QString(predictionRadius) + ".";
		return NULL;
	}

	if ((maxBoost <= 1.0f) && (maxBoost != -1))
	{
		temp_string.setNum(maxBoost);
		_error_msg = "Region " + id + " has invalid Boost max value " + temp_string + ".";
		return NULL;
	}

	if (boostRate <= 0)
	{
		temp_string.setNum(boostRate);
		_error_msg = "Region " + id + " has invalid Boost rate value " + temp_string + ".";
		return NULL;
	}

	if (spatialLearningStartTime < -1)
	{
		_error_msg = "Region " + id + " has invalid SpatialLearningPeriod start time " + QString(spatialLearningStartTime) + ".";
		return NULL;
	}

	if (spatialLearningEndTime < -1)
	{
		_error_msg = "Region " + id + " has invalid SpatialLearningPeriod end time " + QString(spatialLearningEndTime) + ".";
		return NULL;
	}

	if (temporalLearningStartTime < -1)
	{
		_error_msg = "Region " + id + " has invalid TemporalLearningPeriod start time " + QString(temporalLearningStartTime) + ".";
		return NULL;
	}

	if (temporalLearningEndTime < -1)
	{
		_error_msg = "Region " + id + " has invalid TemporalLearningPeriod end time " + QString(temporalLearningEndTime) + ".";
		return NULL;
	}

	if (boostingStartTime < -1)
	{
		_error_msg = "Region " + id + " has invalid BoostingPeriod start time " + QString(boostingStartTime) + ".";
		return NULL;
	}

	if (boostingEndTime < -1)
	{
		_error_msg = "Region " + id + " has invalid BoostingPeriod end time " + QString(boostingEndTime) + ".";
		return NULL;
	}

	if (segmentActivateThreshold <= 0)
	{
		_error_msg = "Region " + id + " has invalid SegmentActivateThreshold " + QString(segmentActivateThreshold) + ".";
		return NULL;
	}

	if (newNumberSynapses <= 0)
	{
		_error_msg = "Region " + id + " has invalid NewNumberSynapses " + QString(newNumberSynapses) + ".";
		return NULL;
	}

	if (min_MinOverlapToReuseSegment < 0)
	{
		_error_msg = "Region " + id + " has invalid MinOverlapToReuseSegment min value " + QString(min_MinOverlapToReuseSegment) + ".";
		return NULL;
	}

	if (max_MinOverlapToReuseSegment < 0)
	{
		_error_msg = "Region " + id + " has invalid MinOverlapToReuseSegment max value " + QString(max_MinOverlapToReuseSegment) + ".";
		return NULL;
	}

	if ((outputColumnActivity == false) && (outputCellActivity == false))
	{
		_error_msg = "Region " + id + " has no output.";
		return NULL;
	}

	if ((percentageInputPerCol <= 0.0f) || (percentageInputPerCol > 100.0f))
	{
		temp_string.setNum(percentageInputPerCol);
		_error_msg = "Region " + id + " has invalid PercentageInputPerCol " + temp_string + ".";
		return NULL;
	}

	if ((percentageMinOverlap <= 0.0f) || (percentageMinOverlap > 100.0f))
	{
		temp_string.setNum(percentageMinOverlap);
		_error_msg = "Region " + id + " has invalid PercentageInputPerCol " + temp_string + ".";
		return NULL;
	}

	if ((percentageLocalActivity <= 0.0f) || (percentageLocalActivity > 100.0f))
	{
		temp_string.setNum(percentageLocalActivity);
		_error_msg = "Region " + id + " has invalid PercentageLocalActivity " + temp_string + ".";
		return NULL;
	}

	if (input_ids.size() == 0)
	{
		_error_msg = "Region " + id + " has no Inputs.";
		return NULL;
	}

	// Create the new Region.
	newRegion = new Region(this, id, Point(sizeX, sizeY), hypercolumnDiameter, _proximalSynapseParams, _distalSynapseParams, percentageInputPerCol / 100.0f, percentageMinOverlap / 100.0f, predictionRadius, inhibitionType, inhibitionRadius, percentageLocalActivity / 100.0f, boostRate, maxBoost, spatialLearningStartTime, spatialLearningEndTime, temporalLearningStartTime, temporalLearningEndTime, boostingStartTime, boostingEndTime, cellsPerColumn, segmentActivateThreshold, newNumberSynapses, min_MinOverlapToReuseSegment, max_MinOverlapToReuseSegment, hardcodedSpatial, outputColumnActivity, outputCellActivity);

	// Record in the new Region its lists of input IDs and radii.
	newRegion->InputIDs = input_ids;
	newRegion->InputRadii = input_radii;

	return newRegion;
}

InputSpace *NetworkManager::ParseInputSpace(QXmlStreamReader &_xml, QString &_error_msg)
{
	QString id;
	int sizeX = 0, sizeY = 0, numValues = 0;
	std::vector<PatternInfo*> patterns;
	PatternInfo *newPattern;
	InputSpace *newInputSpace = NULL;

	// Look for ID attribute
	QXmlStreamAttributes attributes = _xml.attributes();
	if (attributes.hasAttribute("id")) {
		id = attributes.value("id").toString();
	}

	// Advance to the next element.
	_xml.readNext();
  
	// Loop through this InputSpace's elements (this allows the order to vary).
	// Continue the loop until we hit an EndElement named InputSpace.
	for (;;)
	{
		// Get the current token name
		QString tokenName = _xml.name().toString().toLower();

		if ((_xml.tokenType() == QXmlStreamReader::EndElement) && (tokenName == "inputspace")) {
			break;
		}

		if (_xml.atEnd()) 
		{
			_error_msg = "Unexpected end of document.";
			return NULL;
		}

		if (_xml.tokenType() == QXmlStreamReader::StartElement) 
		{
			// SizeX
			if (tokenName == "sizex") 
			{
				_xml.readNext();
				if(_xml.tokenType() == QXmlStreamReader::Characters) {
					sizeX = _xml.text().toString().toInt();
				}
			}

			// SizeY
			else if (tokenName == "sizey") 
			{
				_xml.readNext();
				if(_xml.tokenType() == QXmlStreamReader::Characters) {
					sizeY = _xml.text().toString().toInt();
				}
			}

			// NumValues
			else if (tokenName == "numvalues") 
			{
				_xml.readNext();
				if(_xml.tokenType() == QXmlStreamReader::Characters) {
					numValues = _xml.text().toString().toInt();
				}
			}

			// Pattern
			else if (tokenName == "pattern") 
			{
				if ((sizeX > 0) && (sizeY > 0)) 
				{
					newPattern = ParsePattern(_xml, _error_msg, sizeX, sizeY);

					if (newPattern != NULL) {
						patterns.push_back(newPattern);
					}
				}
			}
		}
		
		// Advance to next token...
		_xml.readNext();
	}

	if (id.length() == 0)
	{
		_error_msg = "InputSpace missing ID.";
		return NULL;
	}

	if ((sizeX <= 0) || (sizeX > INPUTSPACE_MAX_SIZE))
	{
		_error_msg = "InputSpace " + id + " has invalid SizeX " + QString(sizeX) + ".";
		return NULL;
	}

	if ((sizeY <= 0) || (sizeY > INPUTSPACE_MAX_SIZE))
	{
		_error_msg = "InputSpace " + id + " has invalid SizeY " + QString(sizeY) + ".";
		return NULL;
	}

	if ((numValues <= 0) || (numValues > INPUTSPACE_MAX_NUM_VALUES))
	{
		_error_msg = "InputSpace " + id + " has invalid NumValues " + QString(numValues) + ".";
		return NULL;
	}
  
	// Create the new InputSpace
	newInputSpace = new InputSpace(id, sizeX, sizeY, numValues, patterns);

	return newInputSpace;
}

PatternInfo *NetworkManager::ParsePattern(QXmlStreamReader &_xml, QString &_error_msg, int _width, int _height)
{
	PatternType patternType = PATTERN_NONE;
	PatternImageFormat patternImageFormat = PATTERN_IMAGE_FORMAT_UNDEF;
	PatternImageMotion patternImageMotion = PATTERN_IMAGE_MOTION_NONE;
	int startTime = -1, endTime = -1, patternMinTrialDuration = 1, patternMaxTrialDuration = 1, imageWidth, imageHeight;
	QString patternString = "";
	std::vector<int*> bitmaps;
	std::vector<ImageInfo*> images;
	int x, y;
	QString curString;
	float curVal;
	char stringBuffer[STRING_BUFFER_LEN], lineBuffer[LINE_BUFFER_LEN];
	char *linePos;

	// Look for attributes
	QXmlStreamAttributes attributes = _xml.attributes();
				
	if (attributes.hasAttribute("type")) 
	{
		QString typeString = attributes.value("type").toString().toLower();
					
		if (typeString == "stripe") {
			patternType = PATTERN_STRIPE;
		} else if (typeString == "bouncingstripe") {
			patternType = PATTERN_BOUNCING_STRIPE;
		} else if (typeString == "bar") {
			patternType = PATTERN_BAR;
		} else if (typeString == "bouncingbar") {
			patternType = PATTERN_BOUNCING_BAR;
		} else if (typeString == "text") {
			patternType = PATTERN_TEXT;
		} else if (typeString == "bitmap") {
			patternType = PATTERN_BITMAP;
		} else if (typeString == "image") {
			patternType = PATTERN_IMAGE;
		}
	}

	if (attributes.hasAttribute("format")) 
	{
		QString formatString = attributes.value("format").toString().toLower();
					
		if (formatString == "spreadsheet") {
			patternImageFormat = PATTERN_IMAGE_FORMAT_SPREADSHEET;
		} 
	}

	if (attributes.hasAttribute("motion")) 
	{
		QString formatString = attributes.value("motion").toString().toLower();
					
		if (formatString == "across") {
			patternImageMotion = PATTERN_IMAGE_MOTION_ACROSS;
		} 
		else if (formatString == "across2") {
			patternImageMotion = PATTERN_IMAGE_MOTION_ACROSS2;
		} 
	}

	if (attributes.hasAttribute("width")) 
	{
		imageWidth = attributes.value("width").toString().toInt();
	}

	if (attributes.hasAttribute("height")) 
	{
		imageHeight = attributes.value("height").toString().toInt();
	}

	if (attributes.hasAttribute("source")) 
	{
		// Get the source file's filename.
		QString sourceFilename = attributes.value("source").toString();

		// Add the network file's path to the given source file's filename.
		QFile *networkFile = (QFile*)(_xml.device());
		QFileInfo fileInfo(*networkFile);
		sourceFilename = fileInfo.path() + QDir::separator() + sourceFilename;

		// Load source file
		QFile* file = new QFile(sourceFilename);
    
		// If the file failed to open, return error message.
		if (!file->open(QIODevice::ReadOnly | QIODevice::Text)) 
		{
			_error_msg = "Could not open file " + sourceFilename + ".";
			return NULL;
		}

		QTextStream in(file);
    while (!in.atEnd())
    {
      QString line = in.readLine();
			strncpy(lineBuffer, line.toStdString().data(), LINE_BUFFER_LEN);
			linePos = lineBuffer;

			// Create a new ImageInfo for this image.
			ImageInfo *curImageInfo = new ImageInfo();

			// Read the first item on the line, the label, into the stringBuffer.
			ReadItem(linePos, ' ', stringBuffer, STRING_BUFFER_LEN);

			curImageInfo->data = new float[imageWidth * imageHeight];
			curImageInfo->label = stringBuffer;
			curImageInfo->width = imageWidth;
			curImageInfo->height = imageHeight;
			
			int contentX0 = imageWidth -1, contentY0 = imageHeight - 1, contentX1 = 0, contentY1 = 0;

			for (y = 0; y < imageHeight; y++)
			{
				for (x = 0; x < imageWidth; x++)
				{
					// Read the next item on the line into the stringBuffer.
					ReadItem(linePos, ' ', stringBuffer, STRING_BUFFER_LEN);

					const char *valStringStart = strchr(stringBuffer, ':') + 1;
					curVal = atof(valStringStart);

					// Record current pixel value in the image data.
					curImageInfo->data[y * imageWidth + x] = curVal;

					// Keep track of inner content area.
					if (curVal > 0)
					{
						contentX0 = Min(contentX0, x);
						contentY0 = Min(contentY0, y);
						contentX1 = Max(contentX1, x);
						contentY1 = Max(contentY1, y);
					}
				}
			}

			// Record the dimensions of the image's inner content area.
			curImageInfo->contentX = contentX0;
			curImageInfo->contentY = contentY0;
			curImageInfo->contentWidth = contentX1 - contentX0 + 1;
			curImageInfo->contentHeight = contentY1 - contentY0 + 1;

			// Add the curImageInfo to the images array.
			images.push_back(curImageInfo);
    }
					
		// Delete the source file object.
		delete file;
	}

	if (attributes.hasAttribute("start")) 
	{
		startTime = attributes.value("start").toString().toInt();
	}

	if (attributes.hasAttribute("end")) 
	{
		endTime = attributes.value("end").toString().toInt();
	}

	if (attributes.hasAttribute("trial_duration")) 
	{
		int dashPos;
		QString durationString = attributes.value("trial_duration").toString();

		if ((dashPos = durationString.indexOf("-")) != -1)
		{
			patternMinTrialDuration = durationString.section(QChar('-'), 0, 0).toInt();
			patternMaxTrialDuration = durationString.section(QChar('-'), 1, 1).toInt();
		}
		else
		{
			patternMinTrialDuration = patternMaxTrialDuration = durationString.toInt(); 
		}
	}

	if (attributes.hasAttribute("string")) {
		patternString = attributes.value("string").toString();
	}

	if (patternMinTrialDuration <= 0) {
		_error_msg = QString("Minimum trial duration is too low: ").arg(patternMinTrialDuration);
		return NULL;
	}

	if (patternMaxTrialDuration <= 0) {
		_error_msg = QString("Maximum trial duration is too low: ").arg(patternMaxTrialDuration);
		return NULL;
	}

	if (patternMaxTrialDuration < patternMinTrialDuration) {
		_error_msg = QString("Maximum trial duration is less than minimum trial duration.");
		return NULL;
	}

	// Advance to the next element.
	_xml.readNext();
  
	// Loop through this Pattern's elements (this allows the order to vary).
	// Continue the loop until we hit an EndElement named Pattern.
	while (!((_xml.tokenType() == QXmlStreamReader::EndElement) && (_xml.name() == "Pattern"))) 
	{
		if (_xml.tokenType() == QXmlStreamReader::StartElement) 
		{
			// BitMap
			if (_xml.name() == "BitMap") 
			{
				_xml.readNext();
				if(_xml.tokenType() == QXmlStreamReader::Characters) 
				{
					QString bitmap_string = _xml.text().toString();

					// Remove all whitespace from the string.
					bitmap_string = bitmap_string.simplified();
					bitmap_string.replace(" ", "");

					// Create an int array for the current bitmap.
					int *bitmap_array = new int[_width * _height];
					memset(bitmap_array, 0, _width * _height * sizeof(int));
					bitmaps.push_back(bitmap_array);

					for (int i = 0; i < Min(_width * _height, bitmap_string.length()); i++)
					{
						if (bitmap_string[i] != '0') {
							bitmap_array[i] = 1;
						}
					}
				}
			}
		}

		// Advance to next token...
		_xml.readNext();
	}

	return new PatternInfo(patternType, startTime, endTime, patternMinTrialDuration, patternMaxTrialDuration, patternString, patternImageMotion, bitmaps, images);
}

Classifier *NetworkManager::ParseClassifier(QXmlStreamReader &_xml, QString &_error_msg)
{
	QString inputspaceID(""), regionID(""), id("");
	int numitems = -1;
	QStringList labels;

	// Look for attributes
	QXmlStreamAttributes attributes = _xml.attributes();

	if (attributes.hasAttribute("id")) 
	{
		id = attributes.value("id").toString();
	}

	if (attributes.hasAttribute("inputspace")) 
	{
		inputspaceID = attributes.value("inputspace").toString();
	}

	if (attributes.hasAttribute("region")) 
	{
		regionID = attributes.value("region").toString();
	}

	if (attributes.hasAttribute("numitems")) 
	{
		numitems = attributes.value("numitems").toString().toInt();
	}

	if (attributes.hasAttribute("labels")) 
	{
		QString labels_string = attributes.value("labels").toString();
		labels = labels_string.split(",");
	}

	if (id == "") 
	{
		_error_msg = QString("Classifier must have 'id' attribute.");
		return NULL;
	}

	if (regionID == "") 
	{
		_error_msg = QString("Classifier '" + id + "' must have 'region' ID.");
		return NULL;
	}

	if (inputspaceID == "") 
	{
		_error_msg = QString("Classifier '" + id + "' must have 'inputspace' ID.");
		return NULL;
	}

	if (numitems <= 0)
	{
		_error_msg = QString("Classifier '" + id + "' must have 'numitems' greater than 0.");
		return NULL;
	}

	// Advance to the next element.
	_xml.readNext();
  
	// Loop through this Pattern's elements (this allows the order to vary).
	// Continue the loop until we hit an EndElement named Pattern.
	while (!((_xml.tokenType() == QXmlStreamReader::EndElement) && (_xml.name() == "Classifier"))) 
	{
		// Advance to next token...
		_xml.readNext();
	}	

	return new Classifier(id, numitems, regionID, inputspaceID, labels);
}

void NetworkManager::ReadItem(char* &_linePos, char _separator, char *_stringBuffer, int _stringBufferLen)
{
	char *separator_pos = strchr(_linePos, _separator);

	if (separator_pos == NULL)
	{
		_stringBuffer[0] = '\0';
		return;
	}

	// Copy the isolated item into the _stringBuffer.
	strncpy(_stringBuffer, _linePos, Min(separator_pos - _linePos, _stringBufferLen - 2));
	_stringBuffer[Min(separator_pos - _linePos, _stringBufferLen - 1)] = '\0';

	// Advance the _linePos past the isolated item and the separator.
	_linePos = separator_pos + 1;
}

void NetworkManager::ClearData()
{
	Region *region;
	Column *column;
	Cell *cell;

	// Iterate through all regions...
	for (int regionIndex = 0; regionIndex < regions.size(); regionIndex++)
	{
		region = regions[regionIndex];

		// Iterate through each column.
		for (int colIndex = 0; colIndex < (region->GetSizeX() * region->GetSizeY()); colIndex++)
		{
			column = region->Columns[colIndex];

			// Clear the column's proximal segment.
			ClearData_ProximalSegment(column->ProximalSegment);

			// Iterate through all cells...
			for (int cellIndex = 0; cellIndex < region->GetCellsPerCol(); cellIndex++)
			{
				cell = column->GetCellByIndex(cellIndex);

				// Clear all of this cell's distal segments...
				while (cell->Segments.Count() > 0)
				{
					// Clear the column's distal segment.
					ClearData_DistalSegment((Segment*)(cell->Segments.GetFirst()));

					// Remove and release the current distal segment.
					mem_manager.ReleaseObject((Segment*)(cell->Segments.GetFirst()));
					cell->Segments.RemoveFirst();
				}				
			}
		}
	}
}

void NetworkManager::ClearData_ProximalSegment(Segment *_segment)
{
	while (_segment->Synapses.Count() > 0)
	{
		// Remove and release the current proximal synapse.
		mem_manager.ReleaseObject((ProximalSynapse*)(_segment->Synapses.GetFirst()));
		_segment->Synapses.RemoveFirst();
	}
}

void NetworkManager::ClearData_DistalSegment(Segment *_segment)
{
	while (_segment->Synapses.Count() > 0)
	{
		// Remove and release the current distal synapse.
		mem_manager.ReleaseObject((DistalSynapse*)(_segment->Synapses.GetFirst()));
		_segment->Synapses.RemoveFirst();
	}
}

bool NetworkManager::LoadData(QString &_filename, QFile *_file, QString &_error_msg)
{
	int numRegions, width, height, cellsPerCol, numDistalSegments;
	Region *region;
	Column *column;
	Cell *cell;
	Segment *segment;
	bool result;

	// Clear the existing data.
	ClearData();

	QDataStream stream(_file);

	// Read the number of regions.
	stream >> numRegions;

	if (numRegions < 0) 
	{
		_error_msg = QString("Invalid number of Regions.");
		return false;
	}

	// Iterate through all regions...
	for (int regionIndex = 0; regionIndex < Min(numRegions, regions.size()); regionIndex++)
	{
		// Get a pointer to the current Region.
		region = regions[regionIndex];

		stream >> width;
		stream >> height;
		stream >> cellsPerCol;

		if ((width != region->GetSizeX()) || (height != region->GetSizeY()) || (cellsPerCol != region->GetCellsPerCol())) 
		{
			_error_msg = QString("Dimensions of Region do not match network.");
			ClearData();
			return false;
		}

		// Iterate through each column.
		for (int colIndex = 0; colIndex < (region->GetSizeX() * region->GetSizeY()); colIndex++)
		{
			column = region->Columns[colIndex];
					
			// Read the column's data.
			stream >> column->_overlapDutyCycle;
			stream >> column->ActiveDutyCycle;
			stream >> column->FastActiveDutyCycle;
			stream >> column->MinBoost;
			stream >> column->MaxBoost;
			stream >> column->Boost;

			// Read the column's proximal segment data.
			result = LoadData_ProximalSegment(stream, region, column->ProximalSegment, _error_msg);

			if (result == false) 
			{
				ClearData();
				return false;
			}
			
			for (int cellIndex = 0; cellIndex < region->GetCellsPerCol(); cellIndex++)
			{
				cell = column->GetCellByIndex(cellIndex);

				// Read the number of distal segments.
				stream >> numDistalSegments;

				// Iterate through each of this cell's distal segments...
				for (int segIndex = 0; segIndex < numDistalSegments; segIndex++)
				{
					// Create the new distal segment.
					segment = (Segment*)(mem_manager.GetObject(MOT_SEGMENT));
					cell->Segments.InsertAtEnd(segment);

					// Read the current distal segment's data.
					result = LoadData_DistalSegment(stream, region, segment, _error_msg);

					if (result == false) 
					{
						ClearData();
						return false;
					}
				}				
			}
		}
	}

	return true;
}

bool NetworkManager::LoadData_ProximalSegment(QDataStream &_stream, Region *_region, Segment *_segment, QString &_error_msg)
{
	// Read attributes of this segment.
	_stream >> _segment->_numPredictionSteps;
	_stream >> _segment->ConnectedSynapsesCount;
	_stream >> _segment->PrevConnectedSynapsesCount;
	_stream >> _segment->ActiveThreshold;

	// Record the number of synapses.
	int numSynapses;
	_stream >> numSynapses;

	ProximalSynapse *syn;
	float perm;
	DataSpaceType dataSpaceType;
	int dataSpaceIndex;
	DataSpace *dataSpace;
	for (int i = 0; i < numSynapses; i++)
	{
		syn = (ProximalSynapse*)(mem_manager.GetObject(MOT_PROXIMAL_SYNAPSE));
		syn->Initialize(&(_region->DistalSynapseParams));
		_segment->Synapses.InsertAtEnd(syn);

		_stream >> perm;
		syn->SetPermanence(perm);

		_stream >> dataSpaceType;
		_stream >> dataSpaceIndex;

		dataSpace = (dataSpaceType == DATASPACE_TYPE_INPUTSPACE) ? ((DataSpace*)(inputSpaces[dataSpaceIndex])) : ((DataSpace*)(regions[dataSpaceIndex]));

		if (dataSpace == NULL) 
		{
			_error_msg = QString("Proximal synapse connects to Region or InputSpace that does not exist in network.");
			return false;
		}

		// Record this Synapse's input source.
		syn->InputSource = dataSpace;

		// Read this Synapse's input coordinates.
		_stream >> syn->InputPoint.X;
		_stream >> syn->InputPoint.Y;
		_stream >> syn->InputPoint.Index;

		if ((syn->InputPoint.X < 0) || (syn->InputPoint.X >= dataSpace->GetSizeX()) ||
			  (syn->InputPoint.Y < 0) || (syn->InputPoint.Y >= dataSpace->GetSizeY()) ||
				(syn->InputPoint.Index < 0) || (syn->InputPoint.Index >= dataSpace->GetNumValues()))
		{
			_error_msg = QString("Proximal synapse connects to input that is beyond the dimensions of its input Region or InputSpace.");
			return false;
		}

		// Read this Synapse's DistanceToInput.
		_stream >> syn->DistanceToInput;
	}

	return true;
}

bool NetworkManager::LoadData_DistalSegment(QDataStream &_stream, Region *_region, Segment *_segment, QString &_error_msg)
{
	// Read attributes of this segment.
	_stream >> _segment->_numPredictionSteps;
	_stream >> _segment->ConnectedSynapsesCount;
	_stream >> _segment->PrevConnectedSynapsesCount;
	_stream >> _segment->ActiveThreshold;

	// Record the number of synapses.
	int numSynapses;
	_stream >> numSynapses;

	DistalSynapse *syn;
	float perm;
	int inputX, inputY, inputIndex;
	for (int i = 0; i < numSynapses; i++)
	{
		syn = (DistalSynapse*)(mem_manager.GetObject(MOT_DISTAL_SYNAPSE));
		syn->Initialize(&(_region->DistalSynapseParams));
		_segment->Synapses.InsertAtEnd(syn);

		_stream >> perm;
		syn->SetPermanence(perm);

		// Read this Synapse's input coordinates.
		_stream >> inputX;
		_stream >> inputY;
		_stream >> inputIndex;

		if ((inputX < 0) || (inputX >= _region->GetSizeX()) ||
			  (inputY < 0) || (inputY >= _region->GetSizeY()) ||
				(inputIndex < 0) || (inputIndex >= _region->GetNumValues()))
		{
			_error_msg = QString("Distal synapse connects to input that is beyond the dimensions of its Region.");
			return false;
		}

		// Store pointer to this Synapse's input source Cell.
		syn->InputSource = _region->GetColumn(inputX, inputY)->GetCellByIndex(inputIndex);
	}

	return true;
}

bool NetworkManager::SaveData(QString &_filename, QFile *_file, QString &_error_msg)
{
	Region *region;
	Column *column;
	Cell *cell;

	QDataStream stream(_file);

	// Write number of Regions
	stream << (int)(regions.size());

	// Iterate through all regions...
	for (int regionIndex = 0; regionIndex < regions.size(); regionIndex++)
	{
		region = regions[regionIndex];

		// Record this region's dimensions.
		stream << (int)(region->GetSizeX());
		stream << (int)(region->GetSizeY());
		stream << (int)(region->GetCellsPerCol());

		// Iterate through each column.
		for (int colIndex = 0; colIndex < (region->GetSizeX() * region->GetSizeY()); colIndex++)
		{
			column = region->Columns[colIndex];

			// Record the column's data.
			stream << column->GetOverlapDutyCycle();
			stream << column->GetActiveDutyCycle();
			stream << column->GetFastActiveDutyCycle();
			stream << column->GetMinBoost();
			stream << column->GetMaxBoost();
			stream << column->GetBoost();

			// Record the column's proximal segment.
			SaveData_ProximalSegment(stream, column->ProximalSegment);

			for (int cellIndex = 0; cellIndex < region->GetCellsPerCol(); cellIndex++)
			{
				cell = column->GetCellByIndex(cellIndex);

				// Record the number of distal segments.
				stream << cell->Segments.Count();

				// Iterate through each of this cell's distal segments...
				for (int segIndex = 0; segIndex < cell->Segments.Count(); segIndex++)
				{
					// Record the current distal segment.
					SaveData_DistalSegment(stream, (Segment*)(cell->Segments.GetByIndex(segIndex)));
				}				
			}
		}
	}

	return true;
}

bool NetworkManager::SaveData_ProximalSegment(QDataStream &_stream, Segment *_segment)
{
	// Record attributes of this segment.
	_stream << _segment->GetNumPredictionSteps();
	_stream << _segment->GetConnectedSynapseCount();
	_stream << _segment->GetPrevConnectedSynapseCount();
	_stream << _segment->GetActiveThreshold();

	// Record the number of synapses.
	_stream << _segment->Synapses.Count();

	FastListIter synIter(_segment->Synapses);
	ProximalSynapse *syn;
	for (syn = (ProximalSynapse*)(synIter.Reset()); syn != NULL; syn = (ProximalSynapse*)(synIter.Advance()))
	{
		_stream << syn->GetPermanence();
		_stream << syn->InputSource->GetDataSpaceType();
		_stream << syn->InputSource->GetIndex();
		_stream << syn->InputPoint.X;
		_stream << syn->InputPoint.Y;
		_stream << syn->InputPoint.Index;
		_stream << syn->DistanceToInput;

	}

	return true;
}

bool NetworkManager::SaveData_DistalSegment(QDataStream &_stream, Segment *_segment)
{
		// Record attributes of this segment.
	_stream << _segment->GetNumPredictionSteps();
	_stream << _segment->GetConnectedSynapseCount();
	_stream << _segment->GetPrevConnectedSynapseCount();
	_stream << _segment->GetActiveThreshold();

	// Record the number of synapses.
	_stream << _segment->Synapses.Count();

	FastListIter synIter(_segment->Synapses);
	DistalSynapse *syn;
	for (syn = (DistalSynapse*)(synIter.Reset()); syn != NULL; syn = (DistalSynapse*)(synIter.Advance()))
	{
		_stream << syn->GetPermanence();
		_stream << syn->GetInputSource()->GetColumn()->Position.X;
		_stream << syn->GetInputSource()->GetColumn()->Position.Y;
		_stream << syn->GetInputSource()->GetIndex();
	}

	return true;
}

DataSpace *NetworkManager::GetDataSpace(const QString _id)
{
	InputSpace *inputspace = GetInputSpace(_id);

	if (inputspace != NULL) {
		return inputspace;
	}

	Region *region = GetRegion(_id);

	return region;
}

InputSpace *NetworkManager::GetInputSpace(const QString _id)
{
	// Look for a match to the given _id among the InputSpaces.
	for (std::vector<InputSpace*>::const_iterator input_iter = inputSpaces.begin(), end = inputSpaces.end(); input_iter != end; ++input_iter) 
	{
		if ((*input_iter)->GetID() == _id) {
			return (*input_iter);
		}
	}

	return NULL;
}

Region *NetworkManager::GetRegion(const QString _id)
{
	// Look for a match to the given _id among the Regions.
	for (std::vector<Region*>::const_iterator region_iter = regions.begin(), end = regions.end(); region_iter != end; ++region_iter) 
	{
		if ((*region_iter)->GetID() == _id) {
			return (*region_iter);
		}
	}

	return NULL;
}

void NetworkManager::Step()
{
	// Increment time.
	time++;

	// Apply any test patterns to the InputSpaces.
	for (std::vector<InputSpace*>::const_iterator input_iter = inputSpaces.begin(), end = inputSpaces.end(); input_iter != end; ++input_iter) {
		(*input_iter)->ApplyPatterns(time);
	}

	// Run a time step for each Region, in the order they were defined.
	for (std::vector<Region*>::const_iterator region_iter = regions.begin(), end = regions.end(); region_iter != end; ++region_iter) {
		(*region_iter)->Step();
	}
}

void NetworkManager::WriteToLog(QString _text)
{
	QFile file("log.txt");
	if ( file.open(QIODevice::Append) )
	{
			QTextStream stream( &file );
			stream << _text << endl;
	}
}
