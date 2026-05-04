#include "mainwindow.h"
#include "ui_mainwindow.h"

#ifndef __APPLE__
// ── VTK OpenVR headers MUST come before QVTKOpenGLNativeWidget ──
// vtkOpenVRRenderWindow pulls in its own gl.h; if Qt's OpenGL bridge
// loads first the duplicate-include guard fires (C1189 / E0035).
#include <vtkOpenVRRenderer.h>
#include <vtkOpenVRRenderWindow.h>
#include <vtkOpenVRRenderWindowInteractor.h>
#endif

// ── Qt-VTK bridge + standard VTK rendering headers ──
#include <QVTKOpenGLNativeWidget.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkLight.h>

// ── Qt headers ──
#include <QMessageBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QStatusBar>
#include <QTreeView>
#include <QVBoxLayout>
#include <QTimer>

#include <functional>

#include "optiondialog.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("McLaren Senna Viewer");

    QVBoxLayout* layout = new QVBoxLayout(ui->vtkPlaceholder);
    layout->setContentsMargins(0, 0, 0, 0);

    vtkWidget = new QVTKOpenGLNativeWidget();
    layout->addWidget(vtkWidget);

    ui->vtkPlaceholder->setLayout(layout);

    renderWindow = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
    vtkWidget->setRenderWindow(renderWindow);

    renderer = vtkSmartPointer<vtkRenderer>::New();
    renderWindow->AddRenderer(renderer);
    renderer->SetBackground(0.1, 0.2, 0.4);

    // Directional key light illuminating the model from above-right
    auto light = vtkSmartPointer<vtkLight>::New();
    light->SetLightTypeToSceneLight();
    light->SetPosition(5.0, 8.0, 5.0);
    light->SetFocalPoint(0.0, 0.0, 0.0);
    light->SetIntensity(1.0);
    light->SetColor(1.0, 1.0, 1.0);
    renderer->AddLight(light);

    // Soft fill light from the opposite side to reduce harsh shadows
    auto fillLight = vtkSmartPointer<vtkLight>::New();
    fillLight->SetLightTypeToSceneLight();
    fillLight->SetPosition(-5.0, 2.0, -3.0);
    fillLight->SetFocalPoint(0.0, 0.0, 0.0);
    fillLight->SetIntensity(0.4);
    renderer->AddLight(fillLight);

    partList = new ModelPartList("Parts List");
    ui->treeView->setModel(partList);

    ui->treeView->setContextMenuPolicy(Qt::ActionsContextMenu);
    ui->treeView->addAction(ui->actionItem_Options);

    connect(ui->treeView, &QTreeView::clicked,
        this, &MainWindow::handleTreeClicked);

    connect(this, &MainWindow::statusUpdateMessage,
        ui->statusbar, &QStatusBar::showMessage);
}

MainWindow::~MainWindow()
{
    if (m_animTimer) {
        m_animTimer->stop();
    }
    delete ui;
}

void MainWindow::handleTreeClicked(const QModelIndex& index)
{
    if (!index.isValid())
        return;

    ModelPart* item = static_cast<ModelPart*>(index.internalPointer());

    if (!item)
        return;

    emit statusUpdateMessage(QString("Selected: %1").arg(item->getName()), 0);
}

void MainWindow::on_Button1_clicked()
{
    on_actionItem_Options_triggered();
}

void MainWindow::on_Button2_clicked()
{
    QModelIndex index = ui->treeView->currentIndex();

    if (!index.isValid()) {
        emit statusUpdateMessage("No item selected", 0);
        return;
    }

    ModelPart* part = static_cast<ModelPart*>(index.internalPointer());

    if (!part) {
        emit statusUpdateMessage("Invalid selection", 0);
        return;
    }

    vtkSmartPointer<vtkActor> actor = part->getActor();

    if (actor != nullptr) {
        renderer->RemoveActor(actor);
        vtkWidget->renderWindow()->Render();
        emit statusUpdateMessage(QString("Removed from render: %1").arg(part->getName()), 0);
    }
    else {
        emit statusUpdateMessage("Selected item has no actor", 0);
    }
}

void MainWindow::on_actionOpen_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        tr("Open STL File"),
        "",
        tr("STL Files (*.stl);;All Files (*)")
    );

    if (fileName.isEmpty())
        return;

    QModelIndex index = ui->treeView->currentIndex();

    QFileInfo info(fileName);
    QList<QVariant> data = { info.fileName(), "true", "255,0,89" };

    QModelIndex newIndex = partList->appendChild(index, data);

    ModelPart* newPart = static_cast<ModelPart*>(newIndex.internalPointer());

    if (newPart) {
        newPart->loadSTL(fileName);
        emit statusUpdateMessage(QString("Loaded: %1").arg(info.fileName()), 0);
    }

    ui->treeView->expandAll();
    updateRender();
}

void MainWindow::on_actionOpenDirectory_triggered()
{
    QString dirPath = QFileDialog::getExistingDirectory(
        this,
        tr("Open Model Directory"),
        "",
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );

    if (dirPath.isEmpty())
        return;

    QModelIndex rootIndex;  // invalid → adds to tree root
    loadDirectory(rootIndex, dirPath);

    ui->treeView->expandAll();
    updateRender();
    emit statusUpdateMessage(
        QString("Loaded directory: %1").arg(QFileInfo(dirPath).fileName()), 0);
}

void MainWindow::loadDirectory(QModelIndex parentIndex, const QString& dirPath)
{
    QDir dir(dirPath);

    // Create a folder node (no STL file, just a tree label)
    QList<QVariant> dirData = { QFileInfo(dirPath).fileName(), "true", "180,180,220" };
    QModelIndex dirIndex = partList->appendChild(parentIndex, dirData);

    // Load every STL file in this directory as a child node
    const QStringList stlFiles = dir.entryList({ "*.stl", "*.STL" }, QDir::Files);
    for (const QString& filename : stlFiles) {
        const QString fullPath = dirPath + "/" + filename;
        QList<QVariant> data = { filename, "true", "255,0,89" };
        QModelIndex newIndex = partList->appendChild(dirIndex, data);
        ModelPart* newPart = static_cast<ModelPart*>(newIndex.internalPointer());
        if (newPart)
            newPart->loadSTL(fullPath);
    }

    // Recurse into subdirectories
    const QStringList subDirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& sub : subDirs)
        loadDirectory(dirIndex, dirPath + "/" + sub);
}

void MainWindow::on_actionSave_triggered()
{
    QMessageBox::information(this, "Save", "Save is not implemented yet.");
}

void MainWindow::on_actionExit_triggered()
{
    close();
}

void MainWindow::on_actionItem_Options_triggered()
{
    QModelIndex index = ui->treeView->currentIndex();

    if (!index.isValid()) {
        emit statusUpdateMessage("No item selected", 0);
        return;
    }

    ModelPart* part = static_cast<ModelPart*>(index.internalPointer());

    if (!part) {
        emit statusUpdateMessage("Invalid selection", 0);
        return;
    }

    OptionDialog dlg(this);
    dlg.setModelPart(part);

    if (dlg.exec() == QDialog::Accepted) {
        partList->dataChanged(index, index);
        updateRender();
        emit statusUpdateMessage(QString("Updated: %1").arg(part->getName()), 0);
    }
}

void MainWindow::updateRender()
{
    renderer->RemoveAllViewProps();

    int rows = partList->rowCount(QModelIndex());

    for (int i = 0; i < rows; ++i) {
        updateRenderFromTree(partList->index(i, 0, QModelIndex()));
    }

    renderer->ResetCamera();

    vtkCamera* camera = renderer->GetActiveCamera();
    if (camera) {
        camera->Azimuth(30);
        camera->Elevation(20);
    }

    renderer->ResetCameraClippingRange();
    renderer->Render();
    vtkWidget->renderWindow()->Render();

#ifndef __APPLE__
    // Push colour/filter changes into the live VR session without a restart
    if (m_vrThread && m_vrThread->isRunning())
        m_vrThread->scheduleRender();
#endif
}

void MainWindow::updateRenderFromTree(const QModelIndex& index)
{
    if (!index.isValid())
        return;

    ModelPart* selectedPart = static_cast<ModelPart*>(index.internalPointer());

    if (selectedPart && selectedPart->visible()) {
        vtkSmartPointer<vtkActor> actor = selectedPart->getActor();

        if (actor != nullptr) {
            renderer->AddActor(actor);
        }
    }

    int rows = partList->rowCount(index);

    for (int i = 0; i < rows; ++i) {
        updateRenderFromTree(partList->index(i, 0, index));
    }
}

void MainWindow::on_ButtonAnimate_clicked()
{
    if (m_animTimer && m_animTimer->isActive()) {
        m_animTimer->stop();
        ui->ButtonAnimate->setText("Animate");
        emit statusUpdateMessage("Animation stopped", 2000);
#ifndef __APPLE__
        if (m_vrThread && m_vrThread->isRunning())
            m_vrThread->setAnimating(false);
#endif
        return;
    }

    if (!m_animTimer) {
        m_animTimer = new QTimer(this);
        connect(m_animTimer, &QTimer::timeout, this, &MainWindow::animateStep);
    }

    m_animTimer->start(16); // ~60 fps
    ui->ButtonAnimate->setText("Stop Animation");
    emit statusUpdateMessage("Animating — click Stop Animation to pause", 0);
#ifndef __APPLE__
    if (m_vrThread && m_vrThread->isRunning())
        m_vrThread->setAnimating(true);
#endif
}

void MainWindow::animateStep()
{
    renderer->GetActiveCamera()->Azimuth(0.5);
    renderer->ResetCameraClippingRange();
    vtkWidget->renderWindow()->Render();
}

void MainWindow::on_ButtonResetVR_clicked()
{
#ifdef __APPLE__
    emit statusUpdateMessage("VR not available on macOS", 2000);
#else
    if (m_vrThread && m_vrThread->isRunning()) {
        m_vrThread->resetCamera();
        emit statusUpdateMessage("VR camera reset", 2000);
    } else {
        emit statusUpdateMessage("VR is not running", 2000);
    }
#endif
}

void MainWindow::on_ButtonVR_clicked()
{
#ifdef __APPLE__
    QMessageBox::information(this, "VR Not Available",
        "VR is not supported on macOS — please run on a Windows machine for VR support.");
#else
    if (m_vrThread && m_vrThread->isRunning()) {
        m_vrThread->stop();
        return;
    }

    if (m_vrThread) {
        delete m_vrThread;
        m_vrThread = nullptr;
    }

    m_vrThread = new VRRenderThread(this);
    connect(m_vrThread, &VRRenderThread::vrFinished,
            this,        &MainWindow::onVRFinished);

    std::function<void(const QModelIndex&)> addActors =
        [&](const QModelIndex& index)
        {
            if (!index.isValid()) return;

            ModelPart* part = static_cast<ModelPart*>(index.internalPointer());

            if (part && part->visible() && part->getActor())
                m_vrThread->addActor(part->getActor());

            int rows = partList->rowCount(index);
            for (int i = 0; i < rows; ++i)
                addActors(partList->index(i, 0, index));
        };

    int topRows = partList->rowCount(QModelIndex());
    for (int i = 0; i < topRows; ++i)
        addActors(partList->index(i, 0, QModelIndex()));

    m_vrThread->start();
    ui->ButtonVR->setText("Stop VR");
    emit statusUpdateMessage("VR started — click Stop VR to end session", 0);
#endif
}

#ifndef __APPLE__
void MainWindow::onVRFinished()
{
    ui->ButtonVR->setText("Start VR");
    emit statusUpdateMessage("VR session ended", 3000);
}
#endif
