#include "PickerWindow.h"

#include "BrowseTreeModel.h"
#include "CodesysXmlSource.h"
#include "LivePlcSource.h"
#include "SymbolsJsonWriter.h"

#include <QApplication>
#include <QButtonGroup>
#include <QFileDialog>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QSortFilterProxyModel>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTreeView>
#include <QVBoxLayout>
#include <QWidget>
#include <QtConcurrent>

namespace {

// Recursive-match proxy: a row passes when its own Name OR any descendant
// Name matches the regex. Without this a typed substring would hide the
// parent containers and the matching leaves with them.
class TreeFilter : public QSortFilterProxyModel
{
public:
    using QSortFilterProxyModel::QSortFilterProxyModel;
protected:
    bool filterAcceptsRow(int row, const QModelIndex& parent) const override
    {
        const QModelIndex idx = sourceModel()->index(row, 0, parent);
        if (matches(idx)) return true;
        const int kids = sourceModel()->rowCount(idx);
        for (int i = 0; i < kids; ++i)
            if (filterAcceptsRow(i, idx)) return true;
        return false;
    }
private:
    bool matches(const QModelIndex& idx) const
    {
        const QString name = sourceModel()->data(idx, Qt::DisplayRole).toString();
        return name.contains(filterRegularExpression());
    }
};

} // namespace

PickerWindow::PickerWindow(const QString& initialXmlPath,
                           const QString& initialDeviceName,
                           const QString& initialEndpoint,
                           const QString& symbolsOut,
                           QWidget* parent)
    : QMainWindow(parent), symbolsOut_(symbolsOut)
{
    setWindowTitle(QStringLiteral("SMB-Q6R Symbol Picker"));
    resize(1180, 800);

    auto* central = new QWidget(this);
    auto* root    = new QVBoxLayout(central);
    root->setContentsMargins(10, 10, 10, 10);

    // ── Source selector group ──────────────────────────────────────
    auto* sourceBox = new QGroupBox(QStringLiteral("Browse source"), this);
    auto* sourceLay = new QVBoxLayout(sourceBox);

    auto* radioRow = new QHBoxLayout;
    liveRadio_ = new QRadioButton(QStringLiteral("Live PLC (host as OPC UA client)"), this);
    xmlRadio_  = new QRadioButton(QStringLiteral("CodeSys Symbol XML (offline)"), this);
    auto* group = new QButtonGroup(this);
    group->addButton(liveRadio_, 0);
    group->addButton(xmlRadio_,  1);
    radioRow->addWidget(liveRadio_);
    radioRow->addWidget(xmlRadio_);
    radioRow->addStretch(1);
    sourceLay->addLayout(radioRow);

    sourceStack_ = new QStackedWidget(this);
    // page 0 — Live PLC
    {
        auto* page = new QWidget;
        auto* row  = new QHBoxLayout(page);
        row->setContentsMargins(0, 0, 0, 0);
        row->addWidget(new QLabel(QStringLiteral("Endpoint:")));
        liveUrl_ = new QLineEdit(initialEndpoint, page);
        liveUrl_->setPlaceholderText(QStringLiteral("opc.tcp://host:port"));
        row->addWidget(liveUrl_, 1);
        sourceStack_->addWidget(page);
    }
    // page 1 — CodeSys XML
    {
        auto* page = new QWidget;
        auto* col  = new QVBoxLayout(page);
        col->setContentsMargins(0, 0, 0, 0);

        auto* fileRow = new QHBoxLayout;
        fileRow->addWidget(new QLabel(QStringLiteral("XML file:")));
        xmlPath_ = new QLineEdit(initialXmlPath, page);
        xmlPath_->setPlaceholderText(QStringLiteral(
            "Symbol Configuration .xml exported by CodeSys IDE"));
        fileRow->addWidget(xmlPath_, 1);
        xmlBrowse_ = new QPushButton(QStringLiteral("Browse…"), page);
        fileRow->addWidget(xmlBrowse_);
        col->addLayout(fileRow);

        // Device name override — the XML's <ProjectInfo devicename=.../>
        // is frozen at export time; the live PLC may have been renamed
        // since. We let the user override so the synthesised NodeIds
        // match the running server.
        auto* nameRow = new QHBoxLayout;
        nameRow->addWidget(new QLabel(QStringLiteral("Device name override:")));
        xmlDeviceName_ = new QLineEdit(initialDeviceName, page);
        xmlDeviceName_->setPlaceholderText(QStringLiteral(
            "leave blank to use the name embedded in the XML"));
        nameRow->addWidget(xmlDeviceName_, 1);
        col->addLayout(nameRow);

        sourceStack_->addWidget(page);
    }
    sourceLay->addWidget(sourceStack_);

    auto* loadRow = new QHBoxLayout;
    loadBtn_ = new QPushButton(QStringLiteral("Load"), this);
    loadBtn_->setDefault(true);
    loadRow->addStretch(1);
    loadRow->addWidget(loadBtn_);
    sourceLay->addLayout(loadRow);

    root->addWidget(sourceBox);

    // ── Source status / filter ────────────────────────────────────
    sourceLabel_ = new QLabel(QStringLiteral("(no browse loaded yet)"), this);
    sourceLabel_->setStyleSheet(QStringLiteral(
        "color:#475569; font-family:monospace; font-size:10pt;"));
    root->addWidget(sourceLabel_);

    auto* filterRow = new QHBoxLayout;
    filterRow->addWidget(new QLabel(QStringLiteral("Filter:")));
    filterEdit_ = new QLineEdit(this);
    filterEdit_->setPlaceholderText(
        QStringLiteral("substring or regex — matches name; parents stay visible"));
    filterRow->addWidget(filterEdit_, 1);
    root->addLayout(filterRow);

    // ── Tree ──────────────────────────────────────────────────────
    model_ = new BrowseTreeModel(this);
    proxy_ = new TreeFilter(this);
    proxy_->setSourceModel(model_);
    proxy_->setFilterCaseSensitivity(Qt::CaseInsensitive);
    proxy_->setRecursiveFilteringEnabled(true);

    tree_ = new QTreeView(this);
    tree_->setModel(proxy_);
    tree_->setUniformRowHeights(true);
    tree_->setSelectionMode(QAbstractItemView::NoSelection);
    tree_->setAlternatingRowColors(true);
    tree_->header()->setStretchLastSection(false);
    tree_->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    tree_->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    tree_->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    root->addWidget(tree_, 1);

    // ── Footer ────────────────────────────────────────────────────
    auto* footer = new QHBoxLayout;
    statusLabel_ = new QLabel(QStringLiteral("0 leafs selected"), this);
    footer->addWidget(statusLabel_, 1);
    clearBtn_ = new QPushButton(QStringLiteral("Clear"), this);
    saveBtn_  = new QPushButton(QStringLiteral("Save  ▸  symbols.json"), this);
    footer->addWidget(clearBtn_);
    footer->addWidget(saveBtn_);
    root->addLayout(footer);

    setCentralWidget(central);

    // Default to Live mode — it's the safest source since it always
    // reflects the current PLC state, and the user usually has it running.
    liveRadio_->setChecked(true);
    sourceStack_->setCurrentIndex(0);

    connect(group, QOverload<int>::of(&QButtonGroup::idClicked),
            this, [this](int) { onSourceChanged(); });
    connect(xmlBrowse_, &QPushButton::clicked, this, [this]() {
        const QString p = QFileDialog::getOpenFileName(
            this, QStringLiteral("Select CodeSys Symbol Configuration XML"),
            xmlPath_->text(), QStringLiteral("CodeSys XML (*.xml)"));
        if (!p.isEmpty()) xmlPath_->setText(p);
    });
    connect(loadBtn_, &QPushButton::clicked, this, &PickerWindow::onLoadClicked);
    connect(filterEdit_, &QLineEdit::textChanged,
            this, &PickerWindow::onFilterChanged);
    connect(clearBtn_, &QPushButton::clicked, this, &PickerWindow::onClearClicked);
    connect(saveBtn_,  &QPushButton::clicked, this, &PickerWindow::onSaveClicked);
    connect(model_, &BrowseTreeModel::selectionCountChanged,
            this, &PickerWindow::onSelectionCountChanged);

    onSelectionCountChanged(0);
}

PickerWindow::Source PickerWindow::currentSource() const
{
    return xmlRadio_->isChecked() ? Source::CodesysXml : Source::LivePlc;
}

void PickerWindow::onSourceChanged()
{
    sourceStack_->setCurrentIndex(xmlRadio_->isChecked() ? 1 : 0);
}

void PickerWindow::onLoadClicked()
{
    if (currentSource() == Source::CodesysXml) {
        loadFromXml(xmlPath_->text(), xmlDeviceName_->text());
    } else {
        loadFromLive(liveUrl_->text());
    }
}

void PickerWindow::loadFromXml(const QString& path, const QString& deviceName)
{
    if (path.isEmpty()) {
        QMessageBox::information(
            this, QStringLiteral("XML missing"),
            QStringLiteral("Select a CodeSys Symbol Configuration XML file."));
        return;
    }
    // Namespace 4 is what the running PLC reports for CodeSys' Application
    // namespace today (see PLC_INTEGRATION.md). We bake it in here and
    // codegen will rewrite if the live PLC turns out to use a different
    // index. CodeSys XML doesn't carry namespace metadata.
    CodesysXmlSource src(path, deviceName, /*assumedNamespaceIndex=*/4);
    BrowseRoot* r = src.load();
    if (!r) {
        QMessageBox::critical(
            this, QStringLiteral("Load failed"),
            QStringLiteral("Could not parse XML:\n\n%1\n\n%2")
                .arg(path, src.lastError()));
        return;
    }
    sourceLabel_->setText(QStringLiteral(
        "Source: %1   ·   Device prefix: %2   ·   (ns=4 assumed)")
            .arg(path,
                 deviceName.isEmpty() ? QStringLiteral("(from XML)")
                                      : deviceName));
    model_->setRoot(r);
    tree_->expandToDepth(1);
}

void PickerWindow::loadFromLive(const QString& endpoint)
{
    if (endpoint.isEmpty()) {
        QMessageBox::information(
            this, QStringLiteral("Endpoint missing"),
            QStringLiteral("Enter an OPC UA endpoint, e.g. opc.tcp://192.168.0.2:4840"));
        return;
    }
    sourceLabel_->setText(QStringLiteral("Connecting to %1…").arg(endpoint));
    QApplication::processEvents();
    loadBtn_->setEnabled(false);

    auto* watcher = new QFutureWatcher<BrowseRoot*>(this);
    connect(watcher, &QFutureWatcher<BrowseRoot*>::finished, this,
            [this, watcher, endpoint]() {
        loadBtn_->setEnabled(true);
        BrowseRoot* r = watcher->result();
        watcher->deleteLater();
        if (!r) {
            QMessageBox::critical(
                this, QStringLiteral("Connect failed"),
                QStringLiteral("Live browse failed for %1.\n\n"
                               "Check that the PLC is reachable and the\n"
                               "OPC UA endpoint is correct.").arg(endpoint));
            sourceLabel_->setText(QStringLiteral("(load failed)"));
            return;
        }
        sourceLabel_->setText(QStringLiteral("Source: %1   ·   App URI: %2")
                                  .arg(endpoint, r->namespaceUri));
        model_->setRoot(r);
        tree_->expandToDepth(1);
    });

    auto fut = QtConcurrent::run([endpoint]() -> BrowseRoot* {
        LivePlcSource src(endpoint);
        return src.load();
    });
    watcher->setFuture(fut);
}

void PickerWindow::onFilterChanged(const QString& text)
{
    proxy_->setFilterRegularExpression(QRegularExpression(text,
        QRegularExpression::CaseInsensitiveOption));
    if (!text.isEmpty()) tree_->expandAll();
}

void PickerWindow::onClearClicked()
{
    model_->clearSelection();
}

void PickerWindow::onSaveClicked()
{
    const auto leaves = model_->selectedLeaves();
    if (leaves.isEmpty()) {
        QMessageBox::information(
            this, QStringLiteral("Nothing selected"),
            QStringLiteral("Tick at least one leaf node before saving."));
        return;
    }
    SymbolsJsonWriter w;
    QString srcLabel;
    if (currentSource() == Source::CodesysXml) {
        srcLabel = QFileInfo(xmlPath_->text()).absoluteFilePath();
    } else {
        srcLabel = liveUrl_->text();
    }
    if (!w.write(symbolsOut_, model_->browseRoot(), srcLabel, leaves)) {
        QMessageBox::critical(
            this, QStringLiteral("Save failed"),
            QStringLiteral("Could not write symbols manifest:\n\n%1\n\n%2")
                .arg(symbolsOut_, w.lastError()));
        return;
    }
    QMessageBox::information(
        this, QStringLiteral("Saved"),
        QStringLiteral("Wrote %1 symbols to:\n\n%2\n\n"
                       "Run `scripts/regen-symbols.sh` to refresh\n"
                       "src/generated/plc_symbols.h and PlcSymbols.qml.")
            .arg(leaves.size()).arg(symbolsOut_));
}

void PickerWindow::onSelectionCountChanged(int count)
{
    statusLabel_->setText(QStringLiteral("%1 leaf%2 selected   →   %3")
                              .arg(count)
                              .arg(count == 1 ? "" : "s")
                              .arg(symbolsOut_));
}
