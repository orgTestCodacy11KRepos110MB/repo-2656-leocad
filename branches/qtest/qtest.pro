QT += core \
    gui \
    opengl
TARGET = qtest
TEMPLATE = app
DEFINES += LC_INSTALL_PREFIX=\\\"/usr/local\\\"
INCLUDEPATH += qt \
    common
CONFIG += precompile_header incremental
PRECOMPILED_HEADER = common/lc_global.h
win32 { 
	QMAKE_CXXFLAGS_WARN_ON += -wd4100
	DEFINES += _CRT_SECURE_NO_WARNINGS _CRT_SECURE_NO_DEPRECATE=1 _CRT_NONSTDC_NO_WARNINGS=1
	INCLUDEPATH += $$[QT_INSTALL_PREFIX]/src/3rdparty/zlib 
	QMAKE_LFLAGS += /INCREMENTAL
	PRECOMPILED_SOURCE = common/lc_global.cpp
} else {
    LIBS += -lz
    QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter
}
RC_FILE = qt/leocad.rc

release: DESTDIR = build/release
debug:   DESTDIR = build/debug

OBJECTS_DIR = $$DESTDIR/.obj
MOC_DIR = $$DESTDIR/.moc
RCC_DIR = $$DESTDIR/.qrc
UI_DIR = $$DESTDIR/.ui

SOURCES += common/view.cpp \
    common/tr.cpp \
    common/texfont.cpp \
    common/terrain.cpp \
    common/str.cpp \
    common/quant.cpp \
    common/project.cpp \
    common/preview.cpp \
    common/pieceinf.cpp \
    common/piece.cpp \
    common/opengl.cpp \
    common/object.cpp \
    common/minifig.cpp \
    common/message.cpp \
    common/mainwnd.cpp \
    common/light.cpp \
    common/lc_zipfile.cpp \
    common/lc_texture.cpp \
    common/lc_mesh.cpp \
    common/lc_library.cpp \
    common/lc_file.cpp \
    common/lc_colors.cpp \
    common/lc_application.cpp \
    common/keyboard.cpp \
    common/image.cpp \
    common/im_png.cpp \
    common/im_jpg.cpp \
    common/im_gif.cpp \
    common/im_bmp.cpp \
    common/group.cpp \
    common/globals.cpp \
    common/debug.cpp \
    common/curve.cpp \
    common/console.cpp \
    common/camera.cpp \
    common/array.cpp \
    qt/lc_qmainwindow.cpp \
    qt/main.cpp \
    qt/linux_gl.cpp \
    qt/glwindow.cpp \
    qt/basewnd.cpp \
    qt/system.cpp \
    qt/qtmain.cpp \
    qt/lc_colorlistwidget.cpp \
    qt/lc_glwidget.cpp \
    qt/lc_qpovraydialog.cpp \
    qt/lc_qarraydialog.cpp \
    qt/lc_qgroupdialog.cpp \
    qt/lc_qaboutdialog.cpp \
    qt/lc_qpartstree.cpp \
    qt/lc_qeditgroupsdialog.cpp \
    qt/lc_qselectdialog.cpp \
    qt/lc_qpropertiesdialog.cpp \
    qt/lc_qhtmldialog.cpp \
    qt/lc_qminifigdialog.cpp \
    qt/lc_qpreferencesdialog.cpp \
    qt/lc_qcategorydialog.cpp \
    qt/lc_qprofile.cpp \
    common/lc_profile.cpp \
    qt/lc_qimagedialog.cpp
HEADERS += common/glwindow.h \
    common/array.h \
    common/view.h \
    common/typedefs.h \
    common/tr.h \
    common/texfont.h \
    common/terrain.h \
    common/system.h \
    common/str.h \
    common/quant.h \
    common/project.h \
    common/preview.h \
    common/pieceinf.h \
    common/piece.h \
    common/opengl.h \
    common/object.h \
    common/minifig.h \
    common/message.h \
    common/mainwnd.h \
    common/light.h \
    common/lc_zipfile.h \
    common/lc_texture.h \
    common/lc_mesh.h \
    common/lc_math.h \
    common/lc_library.h \
    common/lc_global.h \
    common/lc_file.h \
    common/lc_colors.h \
    common/lc_application.h \
    common/keyboard.h \
    common/image.h \
    common/group.h \
    common/globals.h \
    common/defines.h \
    common/debug.h \
    common/curve.h \
    common/console.h \
    common/camera.h \
    common/basewnd.h \
    qt/lc_colorlistwidget.h \
    qt/lc_qmainwindow.h \
    qt/lc_glwidget.h \
    qt/lc_config.h \
    qt/lc_qpovraydialog.h \
    qt/lc_qarraydialog.h \
    qt/lc_qgroupdialog.h \
    qt/lc_qaboutdialog.h \
    qt/lc_qpartstree.h \
    qt/lc_qeditgroupsdialog.h \
    qt/lc_qselectdialog.h \
    qt/lc_qpropertiesdialog.h \
    qt/lc_qhtmldialog.h \
    qt/lc_qminifigdialog.h \
    qt/lc_qpreferencesdialog.h \
    qt/lc_qcategorydialog.h \
    common/lc_profile.h \
    qt/lc_qimagedialog.h
FORMS += \ 
    qt/lc_qpovraydialog.ui \
    qt/lc_qarraydialog.ui \
    qt/lc_qgroupdialog.ui \
    qt/lc_qaboutdialog.ui \
    qt/lc_qeditgroupsdialog.ui \
    qt/lc_qselectdialog.ui \
    qt/lc_qpropertiesdialog.ui \
    qt/lc_qhtmldialog.ui \
    qt/lc_qminifigdialog.ui \
    qt/lc_qpreferencesdialog.ui \
    qt/lc_qcategorydialog.ui \
    qt/lc_qimagedialog.ui
OTHER_FILES += 
RESOURCES += leocad.qrc