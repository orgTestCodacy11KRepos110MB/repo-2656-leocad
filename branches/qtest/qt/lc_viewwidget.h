#ifndef LC_VIEWWIDGET_H
#define LC_VIEWWIDGET_H

#include "lc_glwidget.h"

class lcViewWidget : public lcGLWidget
{
public:
	lcViewWidget(QWidget *parent, lcGLWidget *share);
};

#endif // LC_VIEWWIDGET_H