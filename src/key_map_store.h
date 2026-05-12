#pragma once

#include <QString>
#include <QVector>

namespace smbq6r {

// Persistent storage for the 14-cell matrix-keypad mapping. Saves to
// ~/.smb-q6r/keymap.dat as plain text (one integer per line, -1 = unmapped).
// Format is intentionally trivial so it can be edited by hand if needed.
class KeyMapStore
{
public:
    static constexpr int kCellCount = 14;

    // Returns ~/.smb-q6r/keymap.dat, creating the parent directory if needed.
    static QString defaultPath();

    // Loads 14 codes from disk. Missing/invalid entries become -1.
    // Returns an all-(-1) vector if the file does not exist.
    static QVector<int> load(const QString& path = defaultPath());

    // Atomically writes 14 codes to disk (write to .tmp + rename).
    // Returns true on success.
    static bool save(const QVector<int>& codes,
                     const QString& path = defaultPath());

    // Removes the file. Returns true if removed or already absent.
    static bool clear(const QString& path = defaultPath());
};

} // namespace smbq6r
