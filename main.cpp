#include "mainwindow.h"
#include <QApplication>
#include <QSurfaceFormat>

int main(int argc, char *argv[])
{
    // Set OpenGL surface format manually BEFORE QApplication.
    // Do NOT use QVTKOpenGLNativeWidget::defaultFormat() — on Qt6 + VTK 9.x
    // on Windows it internally touches widget machinery before QApplication
    // exists, causing the "Must construct a QApplication before a QWidget" crash.
    QSurfaceFormat fmt;
    fmt.setRenderableType(QSurfaceFormat::OpenGL);
    fmt.setVersion(3, 2);
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    fmt.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    fmt.setRedBufferSize(8);
    fmt.setGreenBufferSize(8);
    fmt.setBlueBufferSize(8);
    fmt.setDepthBufferSize(8);
    fmt.setAlphaBufferSize(8);
    fmt.setStencilBufferSize(0);
    fmt.setStereo(false);
    fmt.setSamples(0);
    QSurfaceFormat::setDefaultFormat(fmt);

    QApplication a(argc, argv);

    MainWindow w;
    w.show();

    return a.exec();
}
