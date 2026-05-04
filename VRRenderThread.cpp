#ifndef __APPLE__

#include "VRRenderThread.h"

// VTK VR headers — must come before any Qt OpenGL headers
#include <vtkOpenVRRenderer.h>
#include <vtkOpenVRRenderWindow.h>
#include <vtkOpenVRRenderWindowInteractor.h>

// VTK rendering
#include <vtkLight.h>
#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkCommand.h>
#include <vtkPlaneSource.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkTransform.h>
#include <vtkEventData.h>

// VTK data
#include <vtkCellData.h>
#include <vtkUnsignedCharArray.h>

#include <cmath>
#include <algorithm>
#include <limits>

// ── GrabCallback ──────────────────────────────────────────────────────────────

// HOW THE BINDINGS WORK (from vtk_openvr_actions.json + binding files):
//   Right grip button → /actions/vtk/in/RightGripAction → Select3DEvent
//   Left  grip button → /actions/vtk/in/LeftGripAction  → Select3DEvent
//
// GRAB LOGIC:
//   Select3DEvent carries a vtkEventDataDevice3D with Press or Release action.
//     Press:   find the nearest actor within kGrabRadius and lock on to it.
//     Release: drop the held actor so it stays where it was placed.
//   Move3DEvent: while holding, translate the actor by the controller delta.
//               Only the controller that grabbed the object drives the movement.
//
class GrabCallback : public vtkCommand
{
public:
    static GrabCallback* New() { return new GrabCallback; }

    vtkOpenVRRenderWindowInteractor*  interactor = nullptr;
    QList<vtkSmartPointer<vtkActor>>* vrActors   = nullptr;

    vtkActor* grabbedActor  = nullptr;
    int       grabbedPtrIdx = -1;
    double    lastPos[3]       = { 0.0, 0.0, 0.0 };
    double    storedPos[2][3]  = { {0,0,0}, {0,0,0} }; // last known pos per controller

    static constexpr double kGrabRadius = 300.0;

    static double dist3(const double a[3], const double b[3])
    {
        double dx = a[0]-b[0], dy = a[1]-b[1], dz = a[2]-b[2];
        return std::sqrt(dx*dx + dy*dy + dz*dz);
    }

    void Execute(vtkObject*, unsigned long eventId, void* callData) override
    {
        if (!interactor || !vrActors) return;
        int ptrIdx = interactor->GetPointerIndex();
        if (ptrIdx < 0 || ptrIdx > 1) return;

        if (eventId == vtkCommand::Move3DEvent)
        {
            // Always update stored position so Select3DEvent can use it
            double* raw = interactor->GetWorldEventPosition(ptrIdx);
            if (raw) {
                storedPos[ptrIdx][0] = raw[0];
                storedPos[ptrIdx][1] = raw[1];
                storedPos[ptrIdx][2] = raw[2];
            }

            if (!grabbedActor || ptrIdx != grabbedPtrIdx) return;

            double* pos = storedPos[ptrIdx];
            grabbedActor->AddPosition(pos[0]-lastPos[0], pos[1]-lastPos[1], pos[2]-lastPos[2]);
            lastPos[0] = pos[0]; lastPos[1] = pos[1]; lastPos[2] = pos[2];
        }
        else if (eventId == vtkCommand::Select3DEvent)
        {
            // Distinguish press from release via event data
            vtkEventDataDevice3D* edd = callData
                ? reinterpret_cast<vtkEventData*>(callData)->GetAsEventDataDevice3D()
                : nullptr;

            bool pressed  = !edd || edd->GetAction() == vtkEventDataAction::Press;
            bool released =  edd && edd->GetAction() == vtkEventDataAction::Release;

            if (released && grabbedActor && ptrIdx == grabbedPtrIdx)
            {
                grabbedActor  = nullptr;
                grabbedPtrIdx = -1;
            }
            else if (pressed && !grabbedActor)
            {
                // Use last known position from Move3DEvent (reliable)
                double* pos = storedPos[ptrIdx];

                double    bestDist = kGrabRadius;
                vtkActor* best     = nullptr;
                for (auto& a : *vrActors) {
                    double* b = a->GetBounds();
                    double c[3] = { (b[0]+b[1])*0.5, (b[2]+b[3])*0.5, (b[4]+b[5])*0.5 };
                    double d = dist3(pos, c);
                    if (d < bestDist) { bestDist = d; best = a.Get(); }
                }
                if (best) {
                    grabbedActor  = best;
                    grabbedPtrIdx = ptrIdx;
                    lastPos[0] = pos[0]; lastPos[1] = pos[1]; lastPos[2] = pos[2];
                }
            }
        }
    }
};

// ── Per-frame callback ────────────────────────────────────────────────────────
class VRFrameCallback : public vtkCommand
{
public:
    static VRFrameCallback* New() { return new VRFrameCallback; }

    vtkOpenVRRenderer* renderer     = nullptr;
    QAtomicInt*        renderNeeded = nullptr;
    QAtomicInt*        animating    = nullptr;
    QAtomicInt*        resetCam     = nullptr;

    void Execute(vtkObject*, unsigned long, void*) override
    {
        if (!renderer) return;

        if (resetCam && resetCam->loadAcquire()) {
            resetCam->storeRelaxed(0);
            renderer->ResetCamera();
        }

        if (animating && animating->loadAcquire())
            renderer->GetActiveCamera()->Azimuth(0.3);

        if (renderNeeded && renderNeeded->loadAcquire())
            renderNeeded->storeRelaxed(0);
    }
};


// ── VRRenderThread ────────────────────────────────────────────────────────────

VRRenderThread::VRRenderThread(QObject* parent)
    : QThread(parent)
{}

VRRenderThread::~VRRenderThread()
{
    stop();
    wait();
}

void VRRenderThread::addActor(vtkSmartPointer<vtkActor> actor)
{
    m_actors.append(actor);
}

void VRRenderThread::stop()
{
    QMutexLocker lock(&m_mutex);
    if (m_interactor)
        m_interactor->TerminateApp();
}

void VRRenderThread::scheduleRender()
{
    m_renderNeeded.storeRelease(1);
}

void VRRenderThread::setAnimating(bool on)
{
    m_animating.storeRelease(on ? 1 : 0);
}

void VRRenderThread::resetCamera()
{
    m_resetCamera.storeRelease(1);
    m_renderNeeded.storeRelease(1);
}

void VRRenderThread::run()
{
    {
        QMutexLocker lock(&m_mutex);
        m_renderer   = vtkSmartPointer<vtkOpenVRRenderer>::New();
        m_window     = vtkSmartPointer<vtkOpenVRRenderWindow>::New();
        m_interactor = vtkSmartPointer<vtkOpenVRRenderWindowInteractor>::New();
    }

    m_window->AddRenderer(m_renderer);
    m_interactor->SetRenderWindow(m_window);
    m_window->SetInteractor(m_interactor);

    m_interactor->SetActionManifestDirectory("./");

    // 50 VTK units = 1 physical metre
    m_window->SetPhysicalScale(50.0);

    // Sky gradient: deep-blue zenith → lighter blue horizon
    m_renderer->GradientBackgroundOn();
    m_renderer->SetBackground(0.10, 0.20, 0.50);
    m_renderer->SetBackground2(0.01, 0.02, 0.08);

    // Key light
    auto keyLight = vtkSmartPointer<vtkLight>::New();
    keyLight->SetLightTypeToSceneLight();
    keyLight->SetPosition(5.0, 8.0, 5.0);
    keyLight->SetFocalPoint(0.0, 0.0, 0.0);
    keyLight->SetIntensity(1.0);
    keyLight->SetColor(1.0, 1.0, 1.0);
    m_renderer->AddLight(keyLight);

    // Fill light
    auto fillLight = vtkSmartPointer<vtkLight>::New();
    fillLight->SetLightTypeToSceneLight();
    fillLight->SetPosition(-5.0, 2.0, -3.0);
    fillLight->SetFocalPoint(0.0, 0.0, 0.0);
    fillLight->SetIntensity(0.4);
    m_renderer->AddLight(fillLight);

    // Build independent VR actors and keep a list for proximity grab.
    // We store them here so GrabCallback can reference them.
    QList<vtkSmartPointer<vtkActor>> vrActorList;

    // Pass 1: create actors with raw geometry (no per-part transform yet)
    for (auto& srcActor : m_actors) {
        vtkPolyDataMapper* srcMapper =
            vtkPolyDataMapper::SafeDownCast(srcActor->GetMapper());
        if (!srcMapper) continue;

        auto vrMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        if (srcMapper->GetNumberOfInputConnections(0) > 0)
            vrMapper->SetInputConnection(srcMapper->GetInputConnection(0, 0));
        else if (srcMapper->GetInput())
            vrMapper->SetInputData(srcMapper->GetInput());
        else
            continue;

        vrMapper->Update();

        auto vrActor = vtkSmartPointer<vtkActor>::New();
        vrActor->SetMapper(vrMapper);
        vrActor->SetProperty(srcActor->GetProperty());
        vrActor->SetVisibility(srcActor->GetVisibility());
        m_renderer->AddActor(vrActor);
        vrActorList.append(vrActor);
    }

    // Pass 2: compute combined raw bounds across all parts
    constexpr double kInf = std::numeric_limits<double>::max();
    double gb[6] = { kInf, -kInf, kInf, -kInf, kInf, -kInf };
    for (auto& a : vrActorList) {
        double* b = a->GetBounds();
        if (b[0] > b[1]) continue;
        gb[0] = std::min(gb[0], b[0]); gb[1] = std::max(gb[1], b[1]);
        gb[2] = std::min(gb[2], b[2]); gb[3] = std::max(gb[3], b[3]);
        gb[4] = std::min(gb[4], b[4]); gb[5] = std::max(gb[5], b[5]);
    }
    double cx = (gb[0]+gb[1])*0.5, cy = (gb[2]+gb[3])*0.5, cz = (gb[4]+gb[5])*0.5;
    double dx = gb[1]-gb[0], dy = gb[3]-gb[2], dz = gb[5]-gb[4];
    double maxDim = dx > dy ? (dx > dz ? dx : dz) : (dy > dz ? dy : dz);
    double scale  = (maxDim > 0.0) ? 100.0 / maxDim : 1.0;

    // One shared transform: centre the whole assembly, orient upright, normalise size
    auto globalT = vtkSmartPointer<vtkTransform>::New();
    globalT->Translate(-cx, -cy, -cz);
    globalT->RotateX(90.0);
    globalT->Scale(scale, scale, scale);

    // Pass 3: apply the same transform + VR placement to every actor
    for (auto& a : vrActorList) {
        a->SetUserTransform(globalT.GetPointer());
        a->SetPosition(0.0, 13.0, -150.0);
    }

    auto grabCb = vtkSmartPointer<GrabCallback>::New();
    grabCb->interactor = m_interactor;
    grabCb->vrActors   = &vrActorList;
    m_interactor->AddObserver(vtkCommand::Select3DEvent, grabCb);
    m_interactor->AddObserver(vtkCommand::Move3DEvent,   grabCb);

    // Checkered floor — 8 m × 8 m room at PhysicalScale=50
    {
        auto floorSource = vtkSmartPointer<vtkPlaneSource>::New();
        floorSource->SetOrigin(-400.0, 0.0, -400.0);
        floorSource->SetPoint1( 400.0, 0.0, -400.0);
        floorSource->SetPoint2(-400.0, 0.0,  400.0);
        floorSource->SetXResolution(16);
        floorSource->SetYResolution(16);
        floorSource->Update();

        auto cellColors = vtkSmartPointer<vtkUnsignedCharArray>::New();
        cellColors->SetNumberOfComponents(3);
        cellColors->SetName("Colors");
        for (int row = 0; row < 16; ++row) {
            for (int col = 0; col < 16; ++col) {
                unsigned char v = ((row + col) % 2 == 0) ? 190 : 55;
                cellColors->InsertNextTuple3(v, v, v);
            }
        }
        floorSource->GetOutput()->GetCellData()->SetScalars(cellColors);

        auto floorMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        floorMapper->SetInputData(floorSource->GetOutput());
        floorMapper->SetScalarModeToUseCellData();
        floorMapper->SetColorModeToDirectScalars();

        auto floorActor = vtkSmartPointer<vtkActor>::New();
        floorActor->SetMapper(floorMapper);
        floorActor->GetProperty()->SetOpacity(0.9);
        floorActor->GetProperty()->SetAmbient(0.4);
        m_renderer->AddActor(floorActor);
    }

    // Per-frame callback: animation + camera reset + render flag
    auto frameCb = vtkSmartPointer<VRFrameCallback>::New();
    frameCb->renderer     = m_renderer;
    frameCb->renderNeeded = &m_renderNeeded;
    frameCb->animating    = &m_animating;
    frameCb->resetCam     = &m_resetCamera;
    m_renderer->AddObserver(vtkCommand::StartEvent, frameCb);

    m_window->Initialize();
    m_interactor->Start();

    emit vrFinished();
}

#endif // __APPLE__
//kk
