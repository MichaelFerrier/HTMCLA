#pragma once
#include <QtWidgets/QFrame>
#include <QtWidgets/QGraphicsView>
#include "NetworkManager.h"
#include <map>

class QSlider;
class QToolButton;
class QLabel;
class QComboBox;
class QMenu;
class View;
class ColumnDisp;
class htm;

enum MouseMode {
	MOUSE_MODE_SELECT = 0,
	MOUSE_MODE_DRAG = 1
};

enum MarkType {
	MARK_ACTIVE,
	MARK_PREDICTED,
	MARK_LEARNING
};

class GraphicsView : public QGraphicsView
{
	Q_OBJECT
public:
    GraphicsView(View *v) : QGraphicsView(), view(v) { }

protected:
    void wheelEvent(QWheelEvent *);
		void mousePressEvent(QMouseEvent *event);
		void mouseDoubleClickEvent(QMouseEvent *event);

private:
    View *view;
};

class View : public QFrame
{
	Q_OBJECT
public:
    explicit View(htm *_win, QWidget *parent = 0);

    QGraphicsView *view() const;

public slots:
    void zoomIn(int level = 1);
    void zoomOut(int level = 1);
		void ShowDataSpace(const QString &_id);

private slots:
    void setupMatrix();

		void ViewMode_Activity();
		void ViewMode_Reconstruction();
		void ViewMode_Prediction();
		void ViewMode_Boost();
		void ViewMode_ConnectionsIn();
		void ViewMode_ConnectionsOut();
		void ViewMode_MarkedCells();

		void View_MarkActiveCells();
		void View_MarkPredictedCells();
		void View_MarkLearningCells();

private:

	QMenu *CreateOptionsMenu();

public:
	void keyPressEvent(QKeyEvent* e);

	void SetMouseMode(MouseMode _mouseMode);
	MouseMode GetMouseMode() {return mouseMode;}

	void SetViewMode(bool _viewActivity, bool _viewReconstruction, bool _viewPrediction, bool _viewBoost, bool _viewConnectionsIn, bool _viewConnectionsOut, bool _viewMarkedCells);
	bool GetViewActivity() {return viewActivity;}
	bool GetViewReconstruction() {return viewReconstruction;}
	bool GetViewBoost() {return viewBoost;}
	bool GetViewPrediction() {return viewPrediction;}
	bool GetViewConnectionsIn() {return viewConnectionsIn;}
	bool GetViewConnectionsOut() {return viewConnectionsOut;}
	bool GetViewMarkedCells() {return viewMarkedCells;}

	void MarkCells(MarkType _type);

	void MousePressed(QMouseEvent *event, bool doubleClick);
	void SetSelected(Region *_region, InputSpace *_input, int _colX, int _colY, int _cellIndex, int _selSegmentIndex);

	void UpdateForNetwork(NetworkManager *_networkManager);
	void UpdateForExecution();

	void GenerateDataImage();
	
	GraphicsView *graphicsView;
	QLabel *label_show;
	QComboBox *showComboBox;
	QToolButton *optionsMenuButton;
	QAction *viewActivityAct, *viewReconstructionAct, *viewPredictionAct, *viewBoostAct, *viewConnectionsInAct, *viewConnectionsOutAct, *viewMarkedCellsAct, *viewDuringRunAct;
	QSlider *zoomSlider;

	htm *win;
	NetworkManager *networkManager;
	DataSpace *dataSpace;
	QGraphicsScene *scene;
	int sceneWidth, sceneHeight;
	ColumnDisp **columnDisps;
	bool addingDataSpaces;
	MouseMode mouseMode;
	bool viewActivity, viewPrediction, viewReconstruction, viewBoost, viewConnectionsIn, viewConnectionsOut, viewMarkedCells;

	Region *selRegion;
	InputSpace *selInput;
	int selColIndex, selColX, selColY, selCellIndex, selSegmentIndex;

	std::map<ColumnDisp*,ColumnDisp*> cols_with_sel_synapses;
	std::map<int, int> marked_cells;
};

