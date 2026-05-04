#include "ModelPart.h"

// All VTK includes live here — never in the header
#include <vtkSTLReader.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkProperty.h>

#include <vtkTransform.h>
#include <vtkShrinkPolyData.h>
#include <vtkClipPolyData.h>
#include <vtkPlane.h>

ModelPart::ModelPart(const QList<QVariant>& data, ModelPart* parent)
    : m_itemData(data),
    m_parentItem(parent),
    m_colour(255, 255, 255)
{
    if (m_itemData.size() > 0)
        m_name = m_itemData.at(0).toString();

    if (m_itemData.size() > 1)
        m_visible = visible();

    if (m_itemData.size() > 2) {
        QStringList rgb = m_itemData.at(2).toString().split(",");
        if (rgb.size() == 3) {
            m_colour = QColor(rgb[0].toInt(), rgb[1].toInt(), rgb[2].toInt());
        }
    }
}

ModelPart::~ModelPart()
{
    qDeleteAll(m_childItems);
}

void ModelPart::appendChild(ModelPart* item)
{
    item->m_parentItem = this;
    m_childItems.append(item);
}

ModelPart* ModelPart::child(int row)
{
    if (row < 0 || row >= m_childItems.size())
        return nullptr;

    return m_childItems.at(row);
}

int ModelPart::childCount() const
{
    return m_childItems.count();
}

int ModelPart::columnCount() const
{
    return m_itemData.count();
}

QVariant ModelPart::data(int column) const
{
    if (column < 0 || column >= m_itemData.size())
        return {};

    return m_itemData.at(column);
}

void ModelPart::set(int column, const QVariant& value)
{
    if (column < 0 || column >= m_itemData.size())
        return;

    m_itemData.replace(column, value);
}

ModelPart* ModelPart::parentItem()
{
    return m_parentItem;
}

int ModelPart::row() const
{
    if (m_parentItem)
        return m_parentItem->m_childItems.indexOf(const_cast<ModelPart*>(this));

    return 0;
}

void ModelPart::setColour(const unsigned char R, const unsigned char G, const unsigned char B)
{
    m_colour = QColor(R, G, B);

    if (m_itemData.size() >= 3)
        set(2, QString("%1,%2,%3").arg(R).arg(G).arg(B));

    if (actor)
        actor->GetProperty()->SetColor(R / 255.0, G / 255.0, B / 255.0);
}

unsigned char ModelPart::getColourR()
{
    return static_cast<unsigned char>(m_colour.red());
}

unsigned char ModelPart::getColourG()
{
    return static_cast<unsigned char>(m_colour.green());
}

unsigned char ModelPart::getColourB()
{
    return static_cast<unsigned char>(m_colour.blue());
}

void ModelPart::setVisible(bool isVisible)
{
    m_visible = isVisible;

    if (m_itemData.size() >= 2)
        set(1, isVisible ? "true" : "false");

    if (actor)
        actor->SetVisibility(isVisible ? 1 : 0);
}

bool ModelPart::visible()
{
    if (m_itemData.size() < 2)
        return m_visible;

    QString s = data(1).toString().trimmed().toLower();
    return (s == "true" || s == "1" || s == "yes");
}

void ModelPart::loadSTL(QString fileName)
{
    file = vtkSmartPointer<vtkSTLReader>::New();
    file->SetFileName(fileName.toStdString().c_str());
    file->Update();

    mapper = vtkSmartPointer<vtkPolyDataMapper>::New();

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);

    actor->GetProperty()->SetColor(
        m_colour.redF(),
        m_colour.greenF(),
        m_colour.blueF()
    );

    actor->SetVisibility(m_visible ? 1 : 0);

    updatePipeline();

    double bounds[6];
    actor->GetBounds(bounds);

    double centreX = (bounds[0] + bounds[1]) / 2.0;
    double centreY = (bounds[2] + bounds[3]) / 2.0;
    double centreZ = (bounds[4] + bounds[5]) / 2.0;

    double sizeX = bounds[1] - bounds[0];
    double sizeY = bounds[3] - bounds[2];
    double sizeZ = bounds[5] - bounds[4];

    double maxSize = sizeX;
    if (sizeY > maxSize) maxSize = sizeY;
    if (sizeZ > maxSize) maxSize = sizeZ;

    double scale = 1.0;
    if (maxSize > 0.0)
        scale = 100.0 / maxSize;

    vtkSmartPointer<vtkTransform> transform =
        vtkSmartPointer<vtkTransform>::New();

    transform->Translate(-centreX, -centreY, -centreZ);
    transform->RotateX(90.0);
    transform->Scale(scale, scale, scale);

    actor->SetUserTransform(transform);
    actor->SetPosition(0.0, 0.0, 0.0);
}

vtkSmartPointer<vtkActor> ModelPart::getActor()
{
    return actor;
}

QString ModelPart::getName() const
{
    return m_name;
}

QColor ModelPart::getColour() const
{
    return m_colour;
}

bool ModelPart::getVisible() const
{
    return m_visible;
}

void ModelPart::setName(const QString& name)
{
    m_name = name;

    if (m_itemData.size() >= 1)
        set(0, name);
}

void ModelPart::setColour(int r, int g, int b)
{
    m_colour = QColor(r, g, b);

    if (m_itemData.size() >= 3)
        set(2, QString("%1,%2,%3").arg(r).arg(g).arg(b));

    if (actor)
        actor->GetProperty()->SetColor(r / 255.0, g / 255.0, b / 255.0);
}

void ModelPart::setShrinkFilter(bool enabled)
{
    m_shrinkEnabled = enabled;
    updatePipeline();
}

void ModelPart::setClipFilter(bool enabled)
{
    m_clipEnabled = enabled;
    updatePipeline();
}

bool ModelPart::shrinkFilterEnabled() const
{
    return m_shrinkEnabled;
}

bool ModelPart::clipFilterEnabled() const
{
    return m_clipEnabled;
}

void ModelPart::updatePipeline()
{
    if (!file || !mapper)
        return;

    // Chain filters directly without storing a vtkAlgorithmOutput* pointer,
    // which requires the full type definition and causes "use of incomplete type"
    // errors on MSVC. Each filter reads from the previous stage via its own
    // GetOutputPort() call at the point of connection.

    if (m_shrinkEnabled && m_clipEnabled) {
        shrinkFilter = vtkSmartPointer<vtkShrinkPolyData>::New();
        shrinkFilter->SetInputConnection(file->GetOutputPort());
        shrinkFilter->SetShrinkFactor(0.8);
        shrinkFilter->Update();

        clipPlane = vtkSmartPointer<vtkPlane>::New();
        clipPlane->SetOrigin(0.0, 0.0, 0.0);
        clipPlane->SetNormal(1.0, 0.0, 0.0);

        clipFilter = vtkSmartPointer<vtkClipPolyData>::New();
        clipFilter->SetInputConnection(shrinkFilter->GetOutputPort());
        clipFilter->SetClipFunction(clipPlane);
        clipFilter->Update();

        mapper->SetInputConnection(clipFilter->GetOutputPort());
    }
    else if (m_shrinkEnabled) {
        shrinkFilter = vtkSmartPointer<vtkShrinkPolyData>::New();
        shrinkFilter->SetInputConnection(file->GetOutputPort());
        shrinkFilter->SetShrinkFactor(0.8);
        shrinkFilter->Update();

        mapper->SetInputConnection(shrinkFilter->GetOutputPort());
    }
    else if (m_clipEnabled) {
        clipPlane = vtkSmartPointer<vtkPlane>::New();
        clipPlane->SetOrigin(0.0, 0.0, 0.0);
        clipPlane->SetNormal(1.0, 0.0, 0.0);

        clipFilter = vtkSmartPointer<vtkClipPolyData>::New();
        clipFilter->SetInputConnection(file->GetOutputPort());
        clipFilter->SetClipFunction(clipPlane);
        clipFilter->Update();

        mapper->SetInputConnection(clipFilter->GetOutputPort());
    }
    else {
        mapper->SetInputConnection(file->GetOutputPort());
    }

    mapper->Update();
}
