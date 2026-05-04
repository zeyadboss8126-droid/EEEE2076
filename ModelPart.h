#ifndef VIEWER_MODELPART_H
#define VIEWER_MODELPART_H

#include <QString>
#include <QList>
#include <QVariant>
#include <QColor>

#include <vtkSmartPointer.h>

// Forward declarations — keep VTK OpenGL headers out of this header
class vtkActor;
class vtkSTLReader;
class vtkPolyDataMapper;
class vtkShrinkPolyData;
class vtkClipPolyData;
class vtkPlane;

/**
 * @brief Represents one node in the CAD model tree.
 *
 * Each ModelPart wraps a single STL file and owns the complete VTK
 * pipeline for that part: vtkSTLReader → optional filters
 * (vtkShrinkPolyData, vtkClipPolyData) → vtkPolyDataMapper → vtkActor.
 *
 * The actor returned by getActor() is added to / removed from the
 * renderer by MainWindow.  For VR, VRRenderThread shallow-copies the
 * actor into a separate VR renderer so that one mapper can serve both.
 */
class ModelPart {
public:
    /** @brief Construct a part from column data.
     *  @param data List of QVariants: [name, visible, "R,G,B"].
     *  @param parent Pointer to the parent node (nullptr for root children). */
    ModelPart(const QList<QVariant>& data, ModelPart* parent = nullptr);

    /** @brief Destructor — recursively deletes all child parts. */
    ~ModelPart();

    /** @brief Append @p item as a child of this node.
     *  @param item Child part to adopt (must not be null). */
    void appendChild(ModelPart* item);

    /** @brief Return the child at @p row, or nullptr if out of range.
     *  @param row Zero-based child index. */
    ModelPart* child(int row);

    /** @brief Return the number of direct children. */
    int childCount() const;

    /** @brief Return the number of data columns (always 3: name, visible, colour). */
    int columnCount() const;

    /** @brief Return the stored value for @p column as a QVariant.
     *  @param column 0 = name, 1 = visible string, 2 = colour string. */
    QVariant data(int column) const;

    /** @brief Overwrite the stored value at @p column.
     *  @param column Column index (0–2).
     *  @param value  New value. */
    void set(int column, const QVariant& value);

    /** @brief Return a pointer to this node's parent, or nullptr for root children. */
    ModelPart* parentItem();

    /** @brief Return the zero-based index of this node within its parent's child list. */
    int row() const;

    // ── Colour ──────────────────────────────────────────────────────────

    /** @brief Set the actor colour using byte components.
     *  @param R Red (0–255).  @param G Green (0–255).  @param B Blue (0–255). */
    void setColour(const unsigned char R, const unsigned char G, const unsigned char B);

    /** @brief Set the actor colour using int components (0–255). */
    void setColour(int r, int g, int b);

    /** @brief Return the red component of the current colour (0–255). */
    unsigned char getColourR();

    /** @brief Return the green component of the current colour (0–255). */
    unsigned char getColourG();

    /** @brief Return the blue component of the current colour (0–255). */
    unsigned char getColourB();

    /** @brief Return the current colour as a QColor. */
    QColor getColour() const;

    // ── Visibility ──────────────────────────────────────────────────────

    /** @brief Show or hide the actor.
     *  @param isVisible true to make the part visible. */
    void setVisible(bool isVisible);

    /** @brief Return true if the part is currently visible.
     *  Reads from the stored column data so the tree view stays in sync. */
    bool visible();

    /** @brief Return the stored visibility flag (does not read column data). */
    bool getVisible() const;

    // ── Geometry ────────────────────────────────────────────────────────

    /** @brief Load an STL file and build the VTK pipeline for this part.
     *  Centres and scales the geometry to fit a 100-unit bounding box.
     *  @param fileName Absolute path to the .stl file. */
    void loadSTL(QString fileName);

    /** @brief Return the vtkActor for this part (nullptr until loadSTL() is called). */
    vtkSmartPointer<vtkActor> getActor();

    // ── Metadata ────────────────────────────────────────────────────────

    /** @brief Return the display name of this part. */
    QString getName() const;

    /** @brief Set the display name.
     *  @param name New name string. */
    void setName(const QString& name);

    // ── Filters ─────────────────────────────────────────────────────────

    /** @brief Enable or disable the vtkShrinkPolyData filter.
     *  Rebuilds the pipeline immediately.
     *  @param enabled true to apply the shrink filter. */
    void setShrinkFilter(bool enabled);

    /** @brief Enable or disable the vtkClipPolyData filter (clips on the YZ plane).
     *  Rebuilds the pipeline immediately.
     *  @param enabled true to apply the clip filter. */
    void setClipFilter(bool enabled);

    /** @brief Return true if the shrink filter is currently active. */
    bool shrinkFilterEnabled() const;

    /** @brief Return true if the clip filter is currently active. */
    bool clipFilterEnabled() const;

private:
    QList<ModelPart*> m_childItems; /**< Ordered list of child nodes. */
    QList<QVariant>   m_itemData;   /**< Column data: [name, visible, "R,G,B"]. */
    ModelPart*        m_parentItem; /**< Parent node pointer (not owned). */

    vtkSmartPointer<vtkSTLReader>       file;         /**< Reads the .stl file. */
    vtkSmartPointer<vtkPolyDataMapper>  mapper;       /**< Converts geometry to render-ready format. */
    vtkSmartPointer<vtkActor>           actor;        /**< Positioned, coloured scene object. */

    vtkSmartPointer<vtkShrinkPolyData>  shrinkFilter; /**< Optional shrink filter in the pipeline. */
    vtkSmartPointer<vtkClipPolyData>    clipFilter;   /**< Optional clip filter in the pipeline. */
    vtkSmartPointer<vtkPlane>           clipPlane;    /**< Clip plane used by clipFilter. */

    QString m_name;              /**< Display name, kept in sync with column 0. */
    QColor  m_colour;            /**< Current part colour. */
    bool    m_visible      = true;  /**< Current visibility state. */
    bool    m_shrinkEnabled = false; /**< Whether the shrink filter is active. */
    bool    m_clipEnabled   = false; /**< Whether the clip filter is active. */

    /** @brief Rebuild the VTK pipeline from the STL reader through active filters to the mapper.
     *  Called automatically by setShrinkFilter() and setClipFilter(). */
    void updatePipeline();
};

#endif
