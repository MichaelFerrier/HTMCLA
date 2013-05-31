#include "htm.h"
#include "NetworkManager.h"
#include "Cell.h"
#include "Segment.h"
#include <ctime>
#include <QtWidgets/QAction>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QHBoxLayout>
#include <QtCore/QXmlStreamReader>
#include <QtCore/QTextStream>
#include <QtCore/QTime>
#include <QtWidgets/QFrame.h>
#include <QtWidgets/QPushButton.h>
#include <QtWidgets/QTabWidget.h>
#include <QtWidgets/QLabel.h>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QDesktopWidget>

htm::htm(NetworkManager *_networkManager, QWidget *_parent)
	: QMainWindow(_parent), networkManager(_networkManager)
{
	mouseMode = MOUSE_MODE_SELECT;

	selRegion = NULL;
	selInput = NULL;
	selColX = -1;
	selColY = -1;
	selCellIndex = -1;
	selSegmentIndex = -1;

	running = false;
	updateWhileRunning = false;
	networkFrameRequiresUpdate = false;
	selectedFrameRequiresUpdate = false;
	stopTimeVal = 0;

	// Create the menu bar.
	createMenus();
	
	// Create the horizontal splitter for the views.
	QSplitter *hSplitter = new QSplitter;
  hSplitter->setOrientation(Qt::Horizontal);

	// Create the first view.
  view1 = new View(this);
  hSplitter->addWidget(view1);

	// Create the second view.
  view2= new View(this);
  hSplitter->addWidget(view2);

	// Create the right-hand panel.
	QFrame *panel = new QFrame();
	panel->setMaximumWidth(300);
	QVBoxLayout *panelLayout = new QVBoxLayout;
	panelLayout->setContentsMargins(0,0,0,0);
	panel->setLayout(panelLayout);

	// "Network" Frame

	// Create the network frame.
	networkFrame = new QFrame();
	QVBoxLayout *networkFrameLayout = new QVBoxLayout();
	networkFrameLayout->setAlignment(Qt::AlignTop);
	networkFrame->setLayout(networkFrameLayout);

	// Create the network name label
	networkName = new QLabel();
	networkFrameLayout->addWidget(networkName);

	// Create the controls frame.
	QFrame *controlsFrame = new QFrame();
	controlsFrame->setFrameShadow(QFrame::Sunken);
	controlsFrame->setFrameShape(QFrame::WinPanel);
	//controlsFrame->setFixedHeight(50);
	networkFrameLayout->addWidget(controlsFrame);
	QVBoxLayout *controlsFrameLayout = new QVBoxLayout();
	controlsFrame->setLayout(controlsFrameLayout);

	// Create the control buttons frame
	QFrame *controlButtonsFrame = new QFrame();
	controlsFrameLayout->addWidget(controlButtonsFrame);
	QHBoxLayout *controlsLayout = new QHBoxLayout();
	controlsLayout->setMargin(0);
	controlButtonsFrame->setLayout(controlsLayout);

	// Pause simulation button
  pauseButton = new QPushButton("||");
	controlsLayout->addWidget(pauseButton);
	connect(pauseButton, SIGNAL(clicked()), this, SLOT(Pause()));

	// Step simulation button
  stepButton = new QPushButton(">");
	controlsLayout->addWidget(stepButton);
	connect(stepButton, SIGNAL(clicked()), this, SLOT(Step()));

	// Run simulation button
  runButton = new QPushButton(">>");
	controlsLayout->addWidget(runButton);
	connect(runButton, SIGNAL(clicked()), this, SLOT(Run()));

	// Create the network stop time frame
	QFrame *timeFrame = new QFrame();
	controlsFrameLayout->addWidget(timeFrame);
	QHBoxLayout *timeLayout = new QHBoxLayout();
	timeLayout->setMargin(0);
	timeFrame->setLayout(timeLayout);

	// Create the network time label
	networkTime = new QLabel();
	timeLayout->addWidget(networkTime);

	// Add stretch to layout
	timeLayout->addStretch();

	// Create the network stop time label
	QLabel *stopTimeLabel = new QLabel("Stop at:");
	timeLayout->addWidget(stopTimeLabel);

	// Create the stop time QLineEdit
	stopTime = new QLineEdit();
	stopTime->setMaximumWidth(50);
	timeLayout->addWidget(stopTime);

	// Create the network info label
	networkInfo = new QLabel();
	networkFrameLayout->addWidget(networkInfo);

	// "Selected" Frame
	
	// Create the selected frame.
	selectedFrame = new QFrame();
	QVBoxLayout *selectedFrameLayout = new QVBoxLayout();
	selectedFrameLayout->setAlignment(Qt::AlignTop);
	selectedFrame->setLayout(selectedFrameLayout);

	// Create the selected info label
	selectedInfo = new QLabel();
	selectedInfo->setMinimumHeight(300);
	selectedInfo->setAlignment(Qt::AlignTop);
	selectedFrameLayout->addWidget(selectedInfo);
	
	// Create the segmentsTable
	segmentsTable = new QTableWidget(0, 6);
	segmentsTable->setHorizontalHeaderLabels(QStringList() << "Seq" << "Steps" << "Syns" << "Actv" << "Prev" << "Created");
	segmentsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
	segmentsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
	segmentsTable->setSelectionMode(QAbstractItemView::SingleSelection);
	connect(segmentsTable->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)), this, SLOT(SelectSegment(QModelIndex,QModelIndex)));
	selectedFrameLayout->addWidget(segmentsTable);
	segmentsTable->setColumnWidth(0, 30);
	segmentsTable->setColumnWidth(1, 40);
	segmentsTable->setColumnWidth(2, 35);
	segmentsTable->setColumnWidth(3, 50);
	segmentsTable->setColumnWidth(4, 50);
	segmentsTable->setColumnWidth(5, 50);
	
	// Deselect segment
  deselectSegButton = new QPushButton("Display All Segments");
	selectedFrameLayout->addWidget(deselectSegButton);
	connect(deselectSegButton, SIGNAL(clicked()), this, SLOT(DeselectSegment()));

	// Create the tabbed widget for the frames in the right-hand panel.
	tabWidget = new QTabWidget();
	tabWidget->addTab(networkFrame, tr("Network"));
	tabWidget->addTab(selectedFrame, tr("Selected"));
	connect(tabWidget, SIGNAL(currentChanged(int)), this, SLOT(CurrentTabChanged(int)));
	panelLayout->addWidget(tabWidget);
		
	// Create a frame to contain the entire layout 
	QFrame *frame = new QFrame();
  QHBoxLayout *layout = new QHBoxLayout;
	layout->setContentsMargins(0,0,0,0);
	layout->setSpacing(2);
	frame->setLayout(layout);

	// Add the splitter and right-hand panel to the main frame.
	layout->addWidget(hSplitter);
	layout->addWidget(panel);

	// Make the main frame the central widget of the window.
	setCentralWidget(frame);

	// Set the window title.
	setWindowTitle(tr("HTM"));

	// Set initial window size.
	QDesktopWidget desktop;
	resize(desktop.screenGeometry().width() * 0.75, desktop.screenGeometry().height() * 0.75);

	// Initialize the UI for the (empty) network.
	UpdateUIForNetwork();
}

htm::~htm()
{
}

void htm::keyPressEvent(QKeyEvent* e)
{
	if (e->key() == Qt::Key_F1)
	{
		Pause();
	}
	else if (e->key() == Qt::Key_F2)
	{
		Step();
	}
	else if (e->key() == Qt::Key_F3)
	{
		Run();
	}
	if (e->key() == Qt::Key_S) 
	{
		if (mouseModeSelectAct->isChecked() == false)
		{
			mouseModeSelectAct->setChecked(true);
			SetMouseMode(MOUSE_MODE_SELECT);
		}
	}
	else if (e->key() == Qt::Key_D) 
	{
		if (mouseModeDragAct->isChecked() == false)
		{
			mouseModeDragAct->setChecked(true);
			SetMouseMode(MOUSE_MODE_DRAG);
		}
	}
	else 
	{
		e->ignore();
	}
};

void htm::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == timer.timerId()) 
	{
		QTime start_time;
		start_time.start();

			// Get the latest value for the stop time.
		if (stopTime->isModified()) 
		{
			stopTimeVal = stopTime->text().toInt();
			stopTime->setModified(false);
		}

		// Run as many time steps as will fit within a small limited time period.
		do
		{
			// Execute one step for the network.
			networkManager->Step();

			// If the current time is the stop time, pause and exit loop.
			if (networkManager->GetTime() == stopTimeVal) 
			{
				Pause();
				break;
			}

			// If the UI is to be updated while running, run only one step at a time.
			if (updateWhileRunning) {
				break;
			}
		} 
		while ((start_time.elapsed()) < 500);

		// Update the UI
		UpdateUIForNetworkExecution();
	} 
	else 
	{
		QWidget::timerEvent(event);
	}
}

void htm::SetSelected(Region *_region, InputSpace *_input, int _colX, int _colY, int _cellIndex, int _segmentIndex)
{
	selRegion = _region;
	selInput = _input;
	selColX = _colX;
	selColY = _colY;
	selCellIndex = _cellIndex;
	selSegmentIndex = _segmentIndex;

	// Let the Views know what has been selected.
	view1->SetSelected(_region, _input, _colX, _colY, _cellIndex, selSegmentIndex);
	view2->SetSelected(_region, _input, _colX, _colY, _cellIndex, selSegmentIndex);

	// Update info about selected item.
	UpdateSelectedInfo();
}

void htm::createMenus()
{
	// File menu

	fileMenu = menuBar()->addMenu(tr("&File"));
    
	loadNetworkAct = new QAction(tr("&Load Network..."), this);
	fileMenu->addAction(loadNetworkAct);
	connect(loadNetworkAct, SIGNAL(triggered()), this, SLOT(loadNetworkFile()));

	loadDataAct = new QAction(tr("&Load Data..."), this);
	fileMenu->addAction(loadDataAct);
	connect(loadDataAct, SIGNAL(triggered()), this, SLOT(loadDataFile()));

	saveDataAct = new QAction(tr("&Save Data..."), this);
	fileMenu->addAction(saveDataAct);
	connect(saveDataAct, SIGNAL(triggered()), this, SLOT(saveDataFile()));

	fileMenu->addSeparator();

	exitAct = new QAction(tr("E&xit"), this);
	fileMenu->addAction(exitAct);
	connect(exitAct, SIGNAL(triggered()), this, SLOT(close()));

	// View menu

	viewMenu = menuBar()->addMenu(tr("&View"));

	viewDuringRunAct = new QAction(tr("&Update while running"), this);
	viewMenu->addAction(viewDuringRunAct);
  viewDuringRunAct->setCheckable(true);
	viewDuringRunAct->setChecked(updateWhileRunning);
	connect(viewDuringRunAct, SIGNAL(triggered()), this, SLOT(ViewMode_UpdateWhileRunning()));

	// Mouse menu

	mouseMenu = menuBar()->addMenu(tr("&Mouse"));

	QActionGroup *mouseActionGroup = new QActionGroup(mouseMenu);

	mouseModeSelectAct = new QAction(tr("&Select (s)"), this);
	mouseMenu->addAction(mouseModeSelectAct);
  mouseModeSelectAct->setCheckable(true);
	mouseActionGroup->addAction(mouseModeSelectAct);
	connect(mouseModeSelectAct, SIGNAL(triggered()), this, SLOT(MouseMode_Select()));

	mouseModeDragAct = new QAction(tr("&Drag (d)"), this);
	mouseMenu->addAction(mouseModeDragAct);
  mouseModeDragAct->setCheckable(true);
	mouseActionGroup->addAction(mouseModeDragAct);
	connect(mouseModeDragAct, SIGNAL(triggered()), this, SLOT(MouseMode_Drag()));

	mouseModeSelectAct->setChecked(true);
	mouseModeDragAct->setChecked(false);
}

void htm::loadNetworkFile()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Load the network architecture"), tr(""), tr("XML File (*.xml)"));
	
	if (!fileName.isEmpty()) 
	{
		// Load network
		QFile* file = new QFile(fileName);
    
		// If the file failed to open, display message.
		if (!file->open(QIODevice::ReadOnly | QIODevice::Text)) 
		{
			QMessageBox::critical(this, "Error loading network.", QString("Couldn't open ") + fileName, QMessageBox::Ok);
			return;
		}
    
		// Load the XML stream from the file.
    QXmlStreamReader xml(file);

		// Parse the XML file
		QString error_msg;
		bool result = networkManager->LoadNetwork(QFileInfo(*file).fileName(), xml, error_msg);

		if (result == false) 
		{
			QMessageBox::critical(this,	"Error loading network.", error_msg, QMessageBox::Ok);
			return;
		}

		// Initialize stop time.
		stopTimeVal = 0;
		stopTime->setText("0");

		// Update the network controls
		pauseButton->setEnabled(false);
		stepButton->setEnabled(true);
		runButton->setEnabled(true);

		// Update the UI to reflect the network that has been loaded.
		UpdateUIForNetwork();
	}
}

void htm::loadDataFile()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Load the network data"), tr(""), tr("CLA Data (*.clad)"));
	
	if (!fileName.isEmpty()) 
	{
		// Load data
		QFile* file = new QFile(fileName);
    
		// If the file failed to open, display message.
		if (!file->open(QIODevice::ReadOnly)) 
		{
			QMessageBox::critical(this, "Error loading data.", QString("Couldn't open ") + fileName, QMessageBox::Ok);
			return;
		}
    
		// Parse the data file
		QString error_msg;
		bool result = networkManager->LoadData(QFileInfo(*file).fileName(), file, error_msg);

		if (result == false) 
		{
			QMessageBox::critical(this,	"Error loading data.", error_msg, QMessageBox::Ok);
			return;
		}

		// Update the UI to reflect the network's data that has been loaded.
		UpdateUIForNetwork();
	}
}

void htm::saveDataFile()
{
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save the network data"), tr(""), tr("CLA Data (*.clad)"));
	
	if (!fileName.isEmpty()) 
	{
		// Load data
		QFile* file = new QFile(fileName);
    
		// If the file failed to open, display message.
		if (!file->open(QIODevice::WriteOnly)) 
		{
			QMessageBox::critical(this, "Error saving data.", QString("Couldn't open ") + fileName, QMessageBox::Ok);
			return;
		}

		// Save the data file
		QString error_msg;
		bool result = networkManager->SaveData(QFileInfo(*file).fileName(), file, error_msg);
		
		if (result == false) 
		{
			QMessageBox::critical(this,	"Error saving data.", error_msg, QMessageBox::Ok);
			return;
		}
	}
}

void htm::MouseMode_Select()
{
	SetMouseMode(MOUSE_MODE_SELECT);
}

void htm::MouseMode_Drag()
{
	SetMouseMode(MOUSE_MODE_DRAG);
}

void htm::ViewMode_UpdateWhileRunning()
{
	updateWhileRunning = viewDuringRunAct->isChecked();
}

void htm::Pause()
{
	if (!running) {
		return;
	}

	running = false;

	pauseButton->setDisabled(true);
	stepButton->setDisabled(false);
	runButton->setDisabled(false);

	// Stop timer.
	timer.stop();

	// If the UI hasn't been updated fully while running, update fully now.
	if (!updateWhileRunning) {
		UpdateUIForNetworkExecution();
	}
}

void htm::Step()
{
	if (running) {
		return;
	}

	// Have the network manager take one step.
	networkManager->Step();

	// Update UI
	UpdateUIForNetworkExecution();
}

void htm::Run()
{
	if (running) {
		return;
	}

	running = true;

	pauseButton->setDisabled(false);
	stepButton->setDisabled(true);
	runButton->setDisabled(true);

	// Start timer.
	timer.start(0, this);
}

void htm::SelectSegment(const QModelIndex & current, const QModelIndex & previous)
{
	selSegmentIndex = current.row();

	// Let the Views know what has been selected.
	view1->SetSelected(selRegion, selInput, selColX, selColY, selCellIndex, selSegmentIndex);
	view2->SetSelected(selRegion, selInput, selColX, selColY, selCellIndex, selSegmentIndex);
}

void htm::DeselectSegment()
{
	segmentsTable->clearSelection();
	segmentsTable->selectionModel()->clearCurrentIndex();
}

void htm::CurrentTabChanged(int _currentTabIndex)
{
	if (networkFrameRequiresUpdate)
	{
		UpdateNetworkInfo();
		networkFrameRequiresUpdate = false;
	}

	if (selectedFrameRequiresUpdate)
	{
		UpdateSelectedInfo();
		selectedFrameRequiresUpdate = false;
	}
}

void htm::UpdateUIForNetwork()
{
	// Update the network UI
	UpdateNetworkInfo();

	view1->UpdateForNetwork(networkManager);
	view2->UpdateForNetwork(networkManager);

	// Show the first DataSpace in view1.
	if (view1->showComboBox->count() > 0) {
		view1->showComboBox->setCurrentIndex(0);
	}

	// Show the second DataSpace in view1.
	if (view2->showComboBox->count() > 1) {
		view2->showComboBox->setCurrentIndex(1);
	}

	// Update information frames.
	UpdateSelectedInfo();
}

void htm::UpdateUIForNetworkExecution()
{
	// Update the network UI
	UpdateNetworkInfo();

	// Do not update the rest of the UI if currently running, and the UI isn't to be updated while running.
	if (running && !updateWhileRunning) {
		return;
	}

	// Update the selected item info.
	UpdateSelectedInfo();

	// Update the views for execution.
	view1->UpdateForExecution();
	view2->UpdateForExecution();
}

void htm::SetMouseMode(MouseMode _mouseMode)
{
	mouseMode = _mouseMode;

	view1->SetMouseMode(_mouseMode);
	view2->SetMouseMode(_mouseMode);
}

void htm::UpdateNetworkInfo()
{
	// Do not update the network frame if it is not currently visible.
	if (tabWidget->currentWidget() != networkFrame)
	{
		networkFrameRequiresUpdate = true;
		return;
	}

	networkTime->setText(QString("Time: %1").arg(networkManager->GetTime()));

	// If currently running, don't update the rest of the network UI.
	if (running) {
		return;
	}

	networkName->setText(QString("File: ") + networkManager->GetFilename());
}

void htm::UpdateSelectedInfo()
{
	// Do not update the selected frame if it is not currently visible.
	if (tabWidget->currentWidget() != selectedFrame)
	{
		selectedFrameRequiresUpdate = true;
		return;
	}

	Cell *selCell = NULL;
	QString infoString;
	QTextStream info(&infoString);
	int numSegments = 0;

	if (selInput != NULL)
	{
		info << "<b><u>InputSpace:</b> " << selInput->GetID() << "</u><br>";
		info << "Dimensions: " << selInput->GetSizeX() << " x " << selInput->GetSizeY() << "<br>";
		info << "<br>";
		info << "<b><u>Column:</b> " << selColX << "," << selColY << "</u><br>";
		info << "<br>";

		if (selCellIndex != -1)
		{
			info << "<b><u>Cell:</b> " << selCellIndex << "</u><br>";
			info << (selInput->GetIsActive(selColX, selColY, selCellIndex) ? "Active" : "Inactive") << "<br>";
		}
	}
	else if (selRegion != NULL)
	{
		Column *selCol = selRegion->GetColumn(selColX, selColY);

		info << "<b><u>Region:</b> " << selRegion->GetID() << "</u><br>";
		info << "Dimensions: " << selRegion->GetSizeX() << " x " << selRegion->GetSizeY() << "<br>";
		info << "Time: " << networkManager->GetTime() << "<br>";
		info << "<br>";
		info << "<b><u>Column:</b> " << selColX << "," << selColY << "</u><br>";
		info << "Overlap: " << selCol->GetOverlap() << "<br>";
		info << "Active: " << (selCol->GetIsActive() ? "Yes" : "No") << "<br>";
		info << "Inhibited: " << (selCol->GetIsInhibited() ? "Yes" : "No") << "<br>";
		info << "Active Duty Cycle: " << selCol->GetActiveDutyCycle() << "<br>";
		info << "Fast Active Duty Cycle: " << selCol->GetFastActiveDutyCycle() << "<br>";
		info << "Overlap Duty Cycle: " << selCol->GetOverlapDutyCycle() << "<br>";
		info << "Max Duty Cycle: " << selCol->GetMaxDutyCycle() << "<br>";
		info << "Min Duty Cycle: " << (selCol->GetMaxDutyCycle() * 0.01f) << "<br>";
		info << "Min Overlap: " << selCol->GetMinOverlap() << "<br>";
		info << "Boost: " << selCol->GetBoost() << "<br>";
		info << "Prev Boost Time: " << selCol->prevBoostTime << "<br>";
		info << "DesiredLocalActivity: " << selCol->DesiredLocalActivity << "<br>";
		info << "<br>";

		if (selCellIndex != -1)
		{
			selCell = selCol->GetCellByIndex(selCellIndex);

			info << "<b><u>Cell:</b> " << selCellIndex << "</u><br>";
			info << "Active: " << (selCell->GetIsActive() ? "Yes" : "No") << "<br>";
			info << "Predicted: " << (selCell->GetIsPredicting() ? "Yes" : "No") << "<br>";
			info << "Num Prediction Steps: " << selCell->GetNumPredictionSteps() << "<br>"; 
			info << "Learning: " << (selCell->GetIsLearning() ? "Yes" : "No") << "<br>";
			info << "Prev Active: " << (selCell->GetWasActive() ? "Yes" : "No") << "<br>";
			info << "Prev Predicted: " << (selCell->GetWasPredicted() ? "Yes" : "No") << "<br>";
			info << "Prev Learning: " << (selCell->GetWasLearning() ? "Yes" : "No") << "<br>";
			info << "Prev Active Time: " << selCell->GetPrevActiveTme() << "<br>\n";
			info << "Num Distal Segments: " << selCell->Segments.Count() << "<br>\n";
			
			segmentsTable->setVisible(false);
			segmentsTable->setUpdatesEnabled(false);

			QTableWidgetItem *item;
			FastListIter segments_iter(selCell->Segments);
			for (Segment *seg = (Segment*)(segments_iter.Reset()); seg != NULL; seg = (Segment*)(segments_iter.Advance()))
			{
				//Seq   Steps   Syns    Actv   Prev Actv  Created

				// Add a new row to the segmentsTable if necessary.
				if (segmentsTable->rowCount() <= numSegments) {
					segmentsTable->insertRow(numSegments);
				}

				item = new QTableWidgetItem(seg->GetIsSequence() ? tr("Yes") : tr("No"));
				segmentsTable->setItem(numSegments, 0, item);

				item = new QTableWidgetItem(tr("%1").arg(seg->GetNumPredictionSteps()));
				segmentsTable->setItem(numSegments, 1, item);

				item = new QTableWidgetItem(tr("%1").arg(seg->Synapses.Count()));
				segmentsTable->setItem(numSegments, 2, item);

				item = new QTableWidgetItem(tr("%1 (%2)").arg(seg->GetIsActive() ? "Y" : "N").arg(seg->ActiveSynapses.Count()));
				segmentsTable->setItem(numSegments, 3, item);

				item = new QTableWidgetItem(tr("%1 (%2)").arg(seg->GetWasActive() ? "Y" : "N").arg(seg->PrevActiveSynapses.Count()));
				segmentsTable->setItem(numSegments, 4, item);

				item = new QTableWidgetItem(tr("%1").arg(seg->GetCreationTime()));
				segmentsTable->setItem(numSegments, 5, item);

				numSegments++;
			}

			segmentsTable->setUpdatesEnabled(true);
			segmentsTable->setVisible(true);
		}
	}
	else
	{
		info << "<b>No selection.</b><br>";
	}

	int i;

	for (i = 0; i < numSegments; i++) {
		segmentsTable->showRow(i);
	}

	for (i = numSegments; i < segmentsTable->rowCount(); i++) {
		segmentsTable->hideRow(i);
	}

	// Only make the segments table visible if there is a selected cell.
	segmentsTable->setVisible(selCell != NULL);
	deselectSegButton->setVisible(selCell != NULL);

	selectedInfo->setText(infoString);
}
