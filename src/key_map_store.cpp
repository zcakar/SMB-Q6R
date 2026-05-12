#include "key_map_store.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDebug>

namespace smbq6r {

QString KeyMapStore::defaultPath()
{
    const QString dir = QDir::homePath() + QStringLiteral("/.smb-q6r");
    QDir().mkpath(dir);
    return dir + QStringLiteral("/keymap.dat");
}

QVector<int> KeyMapStore::load(const QString& path)
{
    QVector<int> out(kCellCount, -1);

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return out;
    }

    int idx = 0;
    while (!f.atEnd() && idx < kCellCount) {
        const QByteArray line = f.readLine().trimmed();
        bool ok = false;
        const int v = line.toInt(&ok);
        out[idx++] = ok ? v : -1;
    }
    qInfo() << "KeyMapStore: loaded" << path
            << "— mapped" << std::count_if(out.begin(), out.end(),
                                           [](int v){ return v >= 0; })
            << "/ 14";
    return out;
}

bool KeyMapStore::save(const QVector<int>& codes, const QString& path)
{
    const QString tmp = path + QStringLiteral(".tmp");
    {
        QFile f(tmp);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
            qWarning() << "KeyMapStore::save open" << tmp << f.errorString();
            return false;
        }
        for (int i = 0; i < kCellCount; ++i) {
            const int v = i < codes.size() ? codes[i] : -1;
            f.write(QByteArray::number(v) + "\n");
        }
    }
    // QFile::rename does not overwrite — drop the existing target first.
    QFile::remove(path);
    if (!QFile::rename(tmp, path)) {
        qWarning() << "KeyMapStore::save rename" << tmp << "->" << path;
        return false;
    }
    return true;
}

bool KeyMapStore::clear(const QString& path)
{
    if (!QFileInfo::exists(path)) return true;
    return QFile::remove(path);
}

} // namespace smbq6r
