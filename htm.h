#ifndef HTM_H
#define HTM_H

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QTableWidget>
#include <QtCore/QBasicTimer>
#include <QtGui/QKeyEvent>
#include "ui_htm.h"
#include "View.h"

class NetworkManager;
class QLineEdit;
class QPushButton;

class htm : public QMainWindow
{
	Q_OBJECT

public:
	htm(NetworkManager *_networkManager, QWidget *_parent = 0);
	~htm();

	void keyPressEvent(QKeyEvent* e);
	void timerEvent(QTimerEvent *event);

	void SetSelected(Region *_region, InputSpace *_input, int _colX, int _colY, int _cellIndex, int _segmentIndex);

private slots:
	void loadNetworkFile();
	void loadDataFile();
	void saveDataFile();

	void MouseMode_Select();
	void MouseMode_Drag();

	void ViewMode_UpdateWhileRunning();

	void Pause();
	void Step();
	void Run();

	void SelectSegment(const QModelIndex & current, const QModelIndex & previous);
	void DeselectSegment();

	void CurrentTabChanged(int _currentTabIndex);

private:
	void createMenus();

	void UpdateUIForNetwork();
	void UpdateUIForNetworkExecution();

	void SetMouseMode(MouseMode _mouseMode);

	void UpdateNetworkInfo();
	void UpdateSelectedInfo();

	NetworkManager *networkManager;

	QMenu *fileMenu, *viewMenu, *mouseMenu;
	QAction *loadNetworkAct, *loadDataAct, *saveDataAct;
	QAction *exitAct;
	QAction *viewDuringRunAct;
	QAction *mouseModeSelectAct, *mouseModeDragAct;

	View *view1, *view2;

	QTabWidget *tabWidget;
	QFrame *networkFrame, *selectedFrame, *testFrame;
	QLabel *networkName, *networkTime, *networkInfo, *selectedInfo;
	QLineEdit *stopTime;
	QPushButton *pauseButton, *stepButton, *runButton;
	QTableWidget *segmentsTable;
	QPushButton *deselectSegButton;

	MouseMode mouseMode;

	Region *selRegion;
	InputSpace *selInput;
	int selColX, selColY, selCellIndex, selSegmentIndex;

	bool running;
	bool updateWhileRunning;
	bool networkFrameRequiresUpdate, selectedFrameRequiresUpdate;
	int stopTimeVal;
	QBasicTimer timer;
};

#endif // HTM_H
