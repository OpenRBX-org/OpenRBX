#ifndef PROFILE_H
#define PROFILE_H

#include <QByteArray>
#include <QString>

namespace Profile {

int GetProfileInt(const QString& section, const QString& entry, int defaultValue);
void WriteProfileInt(const QString& section, const QString& entry, int value);
QString GetProfileString(const QString& section, const QString& entry, const QString& defaultValue = QString());
void WriteProfileString(const QString& section, const QString& entry, const QString& value);
bool GetProfileBinary(const QString& section, const QString& entry, QByteArray& data);
void WriteProfileBinary(const QString& section, const QString& entry, const QByteArray& data);

} // namespace Profile

#endif // PROFILE_H
