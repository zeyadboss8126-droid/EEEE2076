#ifndef VRRENDERTHREAD_H
#define VRRENDERTHREAD_H

#ifndef __APPLE__

#include <QThread>
#include <QMutex>
#include <QAtomicInt>
#include <QList>
#include <vtkSmartPointer.h>

class vtkActor;
class vtkOpenVRRenderer;
class vtkOpenVRRenderWindow;
class vtkOpenVRRenderWindowInteractor;

/**
 * @brief Runs the VTK OpenVR render loop on a dedicated thread.
 *
 * vtkOpenVRRenderWindowInteractor::Start() is a blocking event loop that
 * must not run on the Qt main thread (it would freeze the UI).  This class
 * moves it onto a QThread, exposes a stop() method that safely terminates
 * the loop, and emits vrFinished() so the main window can reset its UI.
 */
class VRRenderThread : public QThread
{
    Q_OBJECT

public:
    explicit VRRenderThread(QObject* parent = nullptr);
    ~VRRenderThread() override;

    /** Add an actor (shallow-copied into the VR scene before the loop starts). */
    void addActor(vtkSmartPointer<vtkActor> actor);

    /** Thread-safe: signal the VR event loop to exit. */
    void stop();

    /** Thread-safe: request a VR re-render on the next frame (after colour/filter change). */
    void scheduleRender();

    /** Thread-safe: enable or disable camera auto-rotation inside VR. */
    void setAnimating(bool on);

    /** Thread-safe: reset the VR camera to fit the scene on the next frame. */
    void resetCamera();

signals:
    /** Emitted when the VR event loop exits (either by stop() or headset disconnect). */
    void vrFinished();

protected:
    void run() override;

private:
    QList<vtkSmartPointer<vtkActor>>                 m_actors;

    // VTK objects — created inside run() on the VR thread
    vtkSmartPointer<vtkOpenVRRenderer>               m_renderer;
    vtkSmartPointer<vtkOpenVRRenderWindow>           m_window;
    vtkSmartPointer<vtkOpenVRRenderWindowInteractor> m_interactor;

    QMutex     m_mutex;

    // Atomic flags written from the Qt main thread, read from the VR thread
    QAtomicInt m_renderNeeded;  /**< Set to 1 to request a VR refresh. */
    QAtomicInt m_animating;     /**< Set to 1 while camera rotation is active. */
    QAtomicInt m_resetCamera;   /**< Set to 1 to trigger a ResetCamera on next frame. */
};

#endif // __APPLE__
#endif // VRRENDERTHREAD_H
