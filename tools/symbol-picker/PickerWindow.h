#pragma once

#include <QMainWindow>
#include <QString>

class QLabel;
class QLineEdit;
class QPushButton;
class QRadioButton;
class QSortFilterProxyModel;
class QStackedWidget;
class QTreeView;

class BrowseTreeModel;

// The picker can populate its tree from two sources:
//   * a previously-fetched plc-browse.json sidecar (offline, fast)
//   * a direct open62541 connection to the PLC (live, ~5 s the first time)
// Source selection sits at the top of the window; switching sources just
// hides one widget panel and shows the other. The actual load is wired
// to a single "Load" button so the user always confirms before we go on
// the wire.
class PickerWindow : public QMainWindow
{
    Q_OBJECT
public:
    PickerWindow(const QString& initialXmlPath,
                 const QString& initialDeviceName,
                 const QString& initialEndpoint,
                 const QString& symbolsOut,
                 QWidget* parent = nullptr);

private slots:
    void onSourceChanged();
    void onLoadClicked();
    void onFilterChanged(const QString& text);
    void onClearClicked();
    void onSaveClicked();
    void onSelectionCountChanged(int count);

private:
    enum class Source { CodesysXml, LivePlc };
    Source currentSource() const;

    void loadFromXml(const QString& path, const QString& deviceName);
    void loadFromLive(const QString& endpoint);

    QString          symbolsOut_;
    BrowseTreeModel* model_       = nullptr;
    QSortFilterProxyModel* proxy_ = nullptr;

    // Source selector row
    QRadioButton*    xmlRadio_      = nullptr;
    QRadioButton*    liveRadio_     = nullptr;
    QStackedWidget*  sourceStack_   = nullptr;
    QLineEdit*       xmlPath_       = nullptr;
    QPushButton*     xmlBrowse_     = nullptr;
    QLineEdit*       xmlDeviceName_ = nullptr;
    QLineEdit*       liveUrl_       = nullptr;
    QPushButton*     loadBtn_       = nullptr;

    QLabel*          sourceLabel_   = nullptr;
    QLineEdit*       filterEdit_    = nullptr;
    QTreeView*       tree_          = nullptr;
    QLabel*          statusLabel_   = nullptr;
    QPushButton*     clearBtn_      = nullptr;
    QPushButton*     saveBtn_       = nullptr;
};
