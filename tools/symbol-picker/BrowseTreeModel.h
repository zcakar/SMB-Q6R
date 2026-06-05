#pragma once

#include <QAbstractItemModel>
#include <QHash>
#include <QList>
#include <QSet>

#include "BrowseJsonLoader.h"

// Qt tree model over a BrowseRoot. Adds tri-state checkbox handling that
// cascades:
//   - checking a parent → all descendants become checked
//   - unchecking a parent → all descendants become unchecked
//   - checking the last unchecked sibling → parent becomes fully checked
//   - any mixed state of siblings → parent shows PartiallyChecked
//
// Selection is stored as a QSet<BrowseNode*> for fast contains/insert.
// The model is non-editable except for the checkbox column.
class BrowseTreeModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    enum Column { ColName = 0, ColNodeId, ColClass, ColumnCount };

    explicit BrowseTreeModel(QObject* parent = nullptr);
    ~BrowseTreeModel() override;

    // Takes ownership of |root|. Replaces any previously-loaded data.
    void setRoot(BrowseRoot* root);
    const BrowseRoot* browseRoot() const { return root_; }

    // Selected leaf Variable nodes — the picker writes these into
    // symbols.json. Internal node selection is not reported; we only
    // emit the leaves a developer would actually want to subscribe to.
    QList<const BrowseNode*> selectedLeaves() const;
    int  selectedLeafCount() const;
    void clearSelection();

    // QAbstractItemModel
    QModelIndex index(int row, int column, const QModelIndex& parent) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    int rowCount(const QModelIndex& parent) const override;
    int columnCount(const QModelIndex& parent) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QVariant headerData(int section, Qt::Orientation o, int role) const override;

signals:
    void selectionCountChanged(int leafCount);

private:
    BrowseNode* nodeAt(const QModelIndex& idx) const;
    Qt::CheckState computeParentState(BrowseNode* parent) const;

    // Cascade-set every descendant of |n| (inclusive) to |state|,
    // emitting dataChanged for each row whose state actually flipped.
    void cascadeDown(BrowseNode* n, bool checked);

    // Walk up from |n|'s parent, recomputing tri-state at each level,
    // emitting dataChanged where the state actually flipped.
    void propagateUp(BrowseNode* n);

    QModelIndex indexFor(BrowseNode* n, int column = ColName) const;

    BrowseRoot*           root_ = nullptr;
    QSet<BrowseNode*>     checked_;
};
