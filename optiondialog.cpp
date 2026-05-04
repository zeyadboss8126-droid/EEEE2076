#include "optiondialog.h"
#include "ui_optiondialog.h"

#include "ModelPart.h"
#include <QColor>
#include <QColorDialog>

OptionDialog::OptionDialog(QWidget* parent)
    : QDialog(parent),
    ui(new Ui::OptionDialog)
{
    ui->setupUi(this);
    connect(ui->btnPickColour, &QPushButton::clicked,
            this,              &OptionDialog::onPickColour);
}

OptionDialog::~OptionDialog()
{
    delete ui;
}

void OptionDialog::setModelPart(ModelPart* part)
{
    m_part = part;
    if (!m_part) return;

    ui->lineEditName->setText(m_part->data(0).toString());

    QString visText = m_part->data(1).toString().trimmed().toLower();
    ui->checkVisible->setChecked(visText == "true" || visText == "1" || visText == "yes");

    QString colourText = m_part->data(2).toString().trimmed();
    int r = 255, g = 255, b = 255;

    const QStringList parts = colourText.split(',', Qt::SkipEmptyParts);
    if (parts.size() == 3) {
        r = parts[0].trimmed().toInt();
        g = parts[1].trimmed().toInt();
        b = parts[2].trimmed().toInt();
    }

    m_colour = QColor(r, g, b);
    ui->spinR->setValue(r);
    ui->spinG->setValue(g);
    ui->spinB->setValue(b);

    // Reflect current colour on the picker button
    ui->btnPickColour->setStyleSheet(
        QString("background-color: rgb(%1,%2,%3);").arg(r).arg(g).arg(b));

    ui->checkShrink->setChecked(m_part->shrinkFilterEnabled());
    ui->checkClip->setChecked(m_part->clipFilterEnabled());
}

void OptionDialog::onPickColour()
{
    QColor chosen = QColorDialog::getColor(m_colour, this, "Choose Part Colour");
    if (!chosen.isValid())
        return;

    m_colour = chosen;
    ui->spinR->setValue(chosen.red());
    ui->spinG->setValue(chosen.green());
    ui->spinB->setValue(chosen.blue());

    ui->btnPickColour->setStyleSheet(
        QString("background-color: rgb(%1,%2,%3);")
            .arg(chosen.red()).arg(chosen.green()).arg(chosen.blue()));
}

void OptionDialog::accept()
{
    if (m_part) {
        m_part->setName(ui->lineEditName->text());
        m_part->setVisible(ui->checkVisible->isChecked());
        m_part->setColour(ui->spinR->value(), ui->spinG->value(), ui->spinB->value());
        m_part->setShrinkFilter(ui->checkShrink->isChecked());
        m_part->setClipFilter(ui->checkClip->isChecked());
    }

    QDialog::accept();
}
