#include "lc_global.h"
#include "lc_application.h"
#include <QApplication>
#include "lc_qmainwindow.h"

int main(int argc, char *argv[])
{
	QCoreApplication::setOrganizationDomain("leocad.org");
	QCoreApplication::setOrganizationName("LeoCAD");
	QCoreApplication::setApplicationName("LeoCAD");
	QCoreApplication::setApplicationVersion(LC_VERSION_TEXT);

	g_App = new lcApplication();
//    main_window = new MainWnd();

#ifdef WIN32
	char libPath[LC_MAXPATH], *ptr;
	strcpy(libPath, argv[0]);
	ptr = strrchr(libPath,'\\');
	if (ptr)
		*(++ptr) = 0;
#else
	const char* libPath = LC_INSTALL_PREFIX"/share/leocad/";
#endif

	if (!g_App->Initialize(argc, argv, libPath, ""))
		return 1;

	QApplication a(argc, argv);
	lcQMainWindow w;
	w.show();

	return a.exec();
}
