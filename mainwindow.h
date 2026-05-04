#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QModelIndex>
#include <QTimer>

#include "ModelPartList.h"
#include "ModelPart.h"

#include <vtkSmartPointer.h>

#ifndef __APPLE__
#include "VRRenderThread.h"
#endif

class vtkGenericOpenGLRenderWindow;
class vtkRenderer;
class QVTKOpenGLNativeWidget;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

/**
 * @brief Main application window.
 *
 * Hosts the VTK 3D viewport embedded in a QVTKOpenGLNativeWidget, a tree
 * view of loaded model parts, and controls for loading STL files, editing
 * part properties, running filters, animating the scene, and launching VR.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /** @brief Construct the main window and initialise the VTK render pipeline.
     *  @param parent Optional parent widget. */
    explicit MainWindow(QWidget* parent = nullptr);

    /** @brief Destructor — releases the UI object. */
    ~MainWindow();

signals:
    /** @brief Emitted to update the status bar.
     *  @param message Text to display.
     *  @param timeout Duration in ms (0 = permanent). */
    void statusUpdateMessage(const QString& message, int timeout);

private slots:
    /** @brief Opens the Part Options dialog for the selected tree item. */
    void on_Button1_clicked();

    /** @brief Removes the selected part's actor from the 3D renderer. */
    void on_Button2_clicked();

    /** @brief Updates the status bar when the user clicks a tree item. */
    void handleTreeClicked(const QModelIndex& index);

    /** @brief Opens a file dialog to load an STL file into the tree. */
    void on_actionOpen_triggered();

    /** @brief Opens a directory picker and loads all STL files, mirroring the folder structure in the tree. */
    void on_actionOpenDirectory_triggered();

    /** @brief Placeholder — save not yet implemented. */
    void on_actionSave_triggered();

    /** @brief Closes the application. */
    void on_actionExit_triggered();

    /** @brief Opens the Part Options dialog for the currently selected tree item. */
    void on_actionItem_Options_triggered();

    /** @brief Starts or stops the VR render session. On macOS shows an info message. */
    void on_ButtonVR_clicked();

    /** @brief Resets the VR camera to fit the scene (no-op if VR is not running). */
    void on_ButtonResetVR_clicked();

    /** @brief Toggles the auto-rotation animation on/off. */
    void on_ButtonAnimate_clicked();

    /** @brief Called by m_animTimer on each tick to advance the camera rotation. */
    void animateStep();

#ifndef __APPLE__
    /** @brief Slot connected to VRRenderThread::vrFinished — resets the VR button label. */
    void onVRFinished();
#endif

private:
    /** @brief Rebuilds the renderer scene from all visible tree items. */
    void updateRender();

    /** @brief Recursively adds visible actors from @p index and its children to the renderer.
     *  @param index Tree model index to start from. */
    void updateRenderFromTree(const QModelIndex& index);

    /** @brief Recursively loads all STL files under @p dirPath into the tree, rooted at @p parentIndex.
     *  @param parentIndex Parent tree index (invalid = tree root).
     *  @param dirPath     Absolute path of the directory to scan. */
    void loadDirectory(QModelIndex parentIndex, const QString& dirPath);

    Ui::MainWindow*    ui       = nullptr; /**< Qt-generated UI accessor. */
    ModelPartList*     partList = nullptr; /**< Tree model backing the QTreeView. */
    QVTKOpenGLNativeWidget*    vtkWidget   = nullptr; /**< Widget that hosts the VTK render window. */

    vtkSmartPointer<vtkGenericOpenGLRenderWindow> renderWindow; /**< Off-screen render window linked to vtkWidget. */
    vtkSmartPointer<vtkRenderer>                  renderer;     /**< Scene renderer — holds actors and lights. */

    QTimer* m_animTimer = nullptr; /**< Timer driving the auto-rotation animation. */

#ifndef __APPLE__
    VRRenderThread* m_vrThread = nullptr; /**< VR render thread (Windows only). */
#endif
};

#endif
