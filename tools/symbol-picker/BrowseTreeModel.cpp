#include "BrowseTreeModel.h"

#include <QApplication>

BrowseTreeModel::BrowseTreeModel(QObject* parent)
    : QAbstractItemModel(parent) {}

BrowseTreeModel::~BrowseTreeModel()
{
    delete root_;
}

void BrowseTreeModel::setRoot(BrowseRoot* root)
{
    beginResetModel();
    delete root_;
    root_ = root;
    checked_.clear();
    endResetModel();
    emit selectionCountChanged(selectedLeafCount());
}

BrowseNode* BrowseTreeModel::nodeAt(const QModelIndex& idx) const
{
    if (!idx.isValid()) return nullptr;
    return static_cast<BrowseNode*>(idx.internalPointer());
}

QModelIndex BrowseTreeModel::index(int row, int column,
                                   const QModelIndex& parent) const
{
    if (!root_ || row < 0 || column < 0 || column >= ColumnCount)
        return {};
    if (!parent.isValid()) {
        if (row >= root_->roots.size()) return {};
        return createIndex(row, column, root_->roots.at(row));
    }
    BrowseNode* p = nodeAt(parent);
    if (!p || row >= p->children.size()) return {};
    return createIndex(row, column, p->children.at(row));
}

QModelIndex BrowseTreeModel::parent(const QModelIndex& child) const
{
    BrowseNode* n = nodeAt(child);
    if (!n || !n->parent) return {};
    BrowseNode* gp = n->parent->parent;
    const QList<BrowseNode*>& siblings =
        gp ? gp->children : root_->roots;
    const int row = siblings.indexOf(n->parent);
    if (row < 0) return {};
    return createIndex(row, 0, n->parent);
}

int BrowseTreeModel::rowCount(const QModelIndex& parent) const
{
    if (!root_) return 0;
    if (!parent.isValid()) return root_->roots.size();
    BrowseNode* p = nodeAt(parent);
    return p ? p->children.size() : 0;
}

int BrowseTreeModel::columnCount(const QModelIndex&) const
{
    return ColumnCount;
}

QVariant BrowseTreeModel::data(const QModelIndex& idx, int role) const
{
    BrowseNode* n = nodeAt(idx);
    if (!n) return {};

    if (role == Qt::DisplayRole) {
        switch (idx.column()) {
            case ColName:   return n->name;
            case ColNodeId: return n->id;
            case ColClass:  return n->nodeClass;
        }
    }
    if (role == Qt::CheckStateRole && idx.column() == ColName) {
        if (n->children.isEmpty())
            return checked_.contains(n) ? Qt::Checked : Qt::Unchecked;
        return computeParentState(n);
    }
    if (role == Qt::ToolTipRole) {
        return QStringLiteral("%1\n%2  [%3]")
            .arg(n->id, n->name, n->nodeClass);
    }
    return {};
}

Qt::CheckState BrowseTreeModel::computeParentState(BrowseNode* parent) const
{
    bool anyChecked = false;
    bool allChecked = true;
    // Recursive descent so a deeply-nested mix renders correctly. Cost
    // is bounded by the subtree size; only walked on demand during paint.
    QList<BrowseNode*> stack = parent->children;
    while (!stack.isEmpty()) {
        BrowseNode* n = stack.takeLast();
        if (n->children.isEmpty()) {
            if (checked_.contains(n)) anyChecked = true;
            else                       allChecked = false;
        } else {
            stack.append(n->children);
        }
        if (anyChecked && !allChecked) break;  // already PartiallyChecked
    }
    if (!anyChecked) return Qt::Unchecked;
    if (allChecked)  return Qt::Checked;
    return Qt::PartiallyChecked;
}

bool BrowseTreeModel::setData(const QModelIndex& idx, const QVariant& value,
                              int role)
{
    if (role != Qt::CheckStateRole || idx.column() != ColName)
        return false;
    BrowseNode* n = nodeAt(idx);
    if (!n) return false;

    const bool desired = (value.toInt() == Qt::Checked);
    cascadeDown(n, desired);
    propagateUp(n);
    emit selectionCountChanged(selectedLeafCount());
    return true;
}

void BrowseTreeModel::cascadeDown(BrowseNode* n, bool checked)
{
    if (n->children.isEmpty()) {
        const bool was = checked_.contains(n);
        if (was == checked) return;
        if (checked) checked_.insert(n);
        else         checked_.remove(n);
        const QModelIndex idx = indexFor(n);
        emit dataChanged(idx, idx, {Qt::CheckStateRole});
        return;
    }
    for (BrowseNode* c : n->children)
        cascadeDown(c, checked);
    // Parent's visible tri-state is recomputed by views via data() on
    // the next paint pass; signal so the view re-asks.
    const QModelIndex idx = indexFor(n);
    emit dataChanged(idx, idx, {Qt::CheckStateRole});
}

void BrowseTreeModel::propagateUp(BrowseNode* n)
{
    BrowseNode* p = n->parent;
    while (p) {
        const QModelIndex idx = indexFor(p);
        emit dataChanged(idx, idx, {Qt::CheckStateRole});
        p = p->parent;
    }
}

QModelIndex BrowseTreeModel::indexFor(BrowseNode* n, int column) const
{
    if (!n) return {};
    const QList<BrowseNode*>& siblings =
        n->parent ? n->parent->children : root_->roots;
    const int row = siblings.indexOf(n);
    if (row < 0) return {};
    return createIndex(row, column, n);
}

Qt::ItemFlags BrowseTreeModel::flags(const QModelIndex& idx) const
{
    Qt::ItemFlags base = QAbstractItemModel::flags(idx);
    if (idx.column() == ColName)
        base |= Qt::ItemIsUserCheckable | Qt::ItemIsAutoTristate;
    return base;
}

QVariant BrowseTreeModel::headerData(int section, Qt::Orientation o,
                                     int role) const
{
    if (o != Qt::Horizontal || role != Qt::DisplayRole) return {};
    switch (section) {
        case ColName:   return QStringLiteral("Name");
        case ColNodeId: return QStringLiteral("Node ID");
        case ColClass:  return QStringLiteral("Class");
    }
    return {};
}

QList<const BrowseNode*> BrowseTreeModel::selectedLeaves() const
{
    QList<const BrowseNode*> out;
    out.reserve(checked_.size());
    for (BrowseNode* n : checked_) out.append(n);
    // Stable order so symbols.json diffs cleanly across runs.
    std::sort(out.begin(), out.end(),
              [](const BrowseNode* a, const BrowseNode* b) {
                  return a->id < b->id;
              });
    return out;
}

int BrowseTreeModel::selectedLeafCount() const
{
    return checked_.size();
}

void BrowseTreeModel::clearSelection()
{
    if (checked_.isEmpty()) return;
    QSet<BrowseNode*> snapshot = checked_;
    checked_.clear();
    for (BrowseNode* n : snapshot) {
        const QModelIndex idx = indexFor(n);
        emit dataChanged(idx, idx, {Qt::CheckStateRole});
    }
    // Plus a top-level refresh for tri-state parents — cheaper than
    // tracking every affected ancestor individually.
    emit dataChanged(QModelIndex(), QModelIndex(), {Qt::CheckStateRole});
    emit selectionCountChanged(0);
}
