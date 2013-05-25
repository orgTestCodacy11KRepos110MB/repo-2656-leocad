#include "lc_global.h"
#include "lc_glwidget.h"
#include "glwindow.h"
#include "project.h"
#include "lc_library.h"
#include "lc_application.h"

lcGLWidget::lcGLWidget(QWidget *parent, lcGLWidget *share, GLWindow *window, bool view)
	: QGLWidget(parent, share)
{
	mWindow = window;
	mWindow->CreateFromWindow(this);

	mWindow->MakeCurrent();
	mWindow->OnInitialUpdate();

	preferredSize = QSize(0, 0);
	setMouseTracking(true);

	isView = view;
	if (isView)
	{
		setFocusPolicy(Qt::StrongFocus);
		setAcceptDrops(true);
	}
}

lcGLWidget::~lcGLWidget()
{
	if (isView)
		delete mWindow;
}

QSize lcGLWidget::sizeHint() const
{
	if (preferredSize.isEmpty())
		return QGLWidget::sizeHint();
	else
		return preferredSize;
}

void lcGLWidget::initializeGL()
{
}

void lcGLWidget::resizeGL(int width, int height)
{
	mWindow->OnSize(width, height);
}

void lcGLWidget::paintGL()
{
	mWindow->OnDraw();
}

void lcGLWidget::mousePressEvent(QMouseEvent *event)
{
	bool Control = event->modifiers() & Qt::ControlModifier;
	bool Shift = event->modifiers() & Qt::ShiftModifier;
	bool Alt = event->modifiers() & Qt::AltModifier;

	switch (event->button())
	{
	case Qt::LeftButton:
		mWindow->OnLeftButtonDown(event->x(), height() - event->y() - 1, Control, Shift, Alt);
		break;
	case Qt::MidButton:
		mWindow->OnMiddleButtonDown(event->x(), height() - event->y() - 1, Control, Shift, Alt);
		break;
	case Qt::RightButton:
		mWindow->OnRightButtonDown(event->x(), height() - event->y() - 1, Control, Shift, Alt);
		break;
	default:
		break;
	}
}

void lcGLWidget::mouseReleaseEvent(QMouseEvent *event)
{
	bool Control = event->modifiers() & Qt::ControlModifier;
	bool Shift = event->modifiers() & Qt::ShiftModifier;
	bool Alt = event->modifiers() & Qt::AltModifier;

	switch (event->button())
	{
	case Qt::LeftButton:
		mWindow->OnLeftButtonUp(event->x(), height() - event->y() - 1, Control, Shift, Alt);
		break;
	case Qt::MidButton:
		mWindow->OnMiddleButtonUp(event->x(), height() - event->y() - 1, Control, Shift, Alt);
		break;
	case Qt::RightButton:
		mWindow->OnRightButtonUp(event->x(), height() - event->y() - 1, Control, Shift, Alt);
		break;
	default:
		break;
	}
}

void lcGLWidget::mouseMoveEvent(QMouseEvent *event)
{
	bool Control = event->modifiers() & Qt::ControlModifier;
	bool Shift = event->modifiers() & Qt::ShiftModifier;
	bool Alt = event->modifiers() & Qt::AltModifier;

	mWindow->OnMouseMove(event->x(), height() - event->y() - 1, Control, Shift, Alt);
}

void lcGLWidget::dragEnterEvent(QDragEnterEvent *event)
{
	if (isView && event->mimeData()->hasFormat("application/vnd.leocad-part"))
	{
		event->acceptProposedAction();

		QByteArray pieceData = event->mimeData()->data("application/vnd.leocad-part");
		QDataStream dataStream(&pieceData, QIODevice::ReadOnly);
		QString id;

		dataStream >> id;

		lcGetActiveProject()->BeginPieceDrop(lcGetPiecesLibrary()->FindPiece(id.toLocal8Bit().data(), false));
	}
	else
		event->ignore();
}

void lcGLWidget::dragLeaveEvent(QDragLeaveEvent *event)
{
	if (!isView)
		return;

	lcGetActiveProject()->EndPieceDrop(false);

	event->accept();
}

void lcGLWidget::dragMoveEvent(QDragMoveEvent *event)
{
	if (!isView || !event->mimeData()->hasFormat("application/vnd.leocad-part"))
		return;

	lcGetActiveProject()->OnPieceDropMove(event->pos().x(), height() - event->pos().y() - 1);

	event->accept();
}

void lcGLWidget::dropEvent(QDropEvent *event)
{
	if (!isView || !event->mimeData()->hasFormat("application/vnd.leocad-part"))
		return;

	lcGetActiveProject()->EndPieceDrop(true);

	event->accept();
}
