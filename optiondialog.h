#ifndef OPTIONDIALOG_H
#define OPTIONDIALOG_H

#include <QDialog>
#include <QColor>

namespace Ui {
class OptionDialog;
}

class ModelPart;

/**
 * @brief Dialog for editing a single model part's properties.
 *
 * Presents controls for the part name, visibility, colour (with a
 * QColorDialog picker), and VTK filter toggles (shrink / clip).
 * Call setModelPart() before exec() to pre-populate the fields.
 * On acceptance the changes are written back to the ModelPart.
 */
class OptionDialog : public QDialog
{
    Q_OBJECT

public:
    /** @brief Construct the dialog.
     *  @param parent Parent widget (typically MainWindow). */
    explicit OptionDialog(QWidget* parent = nullptr);

    /** @brief Destructor — releases the UI object. */
    ~OptionDialog();

    /** @brief Populate all fields from @p part before calling exec().
     *  @param part Pointer to the ModelPart to edit (must not be null). */
    void setModelPart(ModelPart* part);

protected:
    /** @brief Write all field values back to the ModelPart and close. */
    void accept() override;

private slots:
    /** @brief Open QColorDialog and update R/G/B spinboxes with the chosen colour. */
    void onPickColour();

private:
    Ui::OptionDialog* ui;          /**< Qt-generated UI accessor. */
    ModelPart*        m_part = nullptr; /**< Part currently being edited. */
    QColor            m_colour;         /**< Working colour — updated by the picker. */
};

#endif // OPTIONDIALOG_H
