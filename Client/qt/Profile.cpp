#include "Profile.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <QSettings>
#endif

namespace Profile {

#ifdef _WIN32
static HKEY openSectionKey(const QString& section, bool create)
{
	QString path = QStringLiteral("Software\\ROBLOX Corporation\\Roblox");
	if (!section.isEmpty()) {
		path += QLatin1Char('\\') + section;
	}
	HKEY key = nullptr;
	LONG result = create ? RegCreateKeyExW(
							   HKEY_CURRENT_USER,
							   reinterpret_cast<LPCWSTR>(path.utf16()),
							   0,
							   nullptr,
							   REG_OPTION_NON_VOLATILE,
							   KEY_WRITE | KEY_READ,
							   nullptr,
							   &key,
							   nullptr
						   )
						 : RegOpenKeyExW(HKEY_CURRENT_USER, reinterpret_cast<LPCWSTR>(path.utf16()), 0, KEY_READ, &key);
	return result == ERROR_SUCCESS ? key : nullptr;
}

int GetProfileInt(const QString& section, const QString& entry, int defaultValue)
{
	HKEY key = openSectionKey(section, false);
	if (key == nullptr) {
		return defaultValue;
	}

	DWORD value;
	DWORD type;
	DWORD size = sizeof(value);
	LONG result = RegQueryValueExW(
		key, reinterpret_cast<LPCWSTR>(entry.utf16()), nullptr, &type, reinterpret_cast<LPBYTE>(&value), &size
	);
	RegCloseKey(key);
	if (result != ERROR_SUCCESS || type != REG_DWORD) {
		return defaultValue;
	}
	return static_cast<int>(value);
}

void WriteProfileInt(const QString& section, const QString& entry, int value)
{
	HKEY key = openSectionKey(section, true);
	if (key == nullptr) {
		return;
	}

	DWORD data = static_cast<DWORD>(value);
	RegSetValueExW(
		key, reinterpret_cast<LPCWSTR>(entry.utf16()), 0, REG_DWORD, reinterpret_cast<const BYTE*>(&data), sizeof(data)
	);
	RegCloseKey(key);
}

QString GetProfileString(const QString& section, const QString& entry, const QString& defaultValue)
{
	HKEY key = openSectionKey(section, false);
	if (key == nullptr) {
		return defaultValue;
	}

	DWORD type;
	DWORD size = 0;
	LONG result = RegQueryValueExW(key, reinterpret_cast<LPCWSTR>(entry.utf16()), nullptr, &type, nullptr, &size);
	if (result != ERROR_SUCCESS || type != REG_SZ) {
		RegCloseKey(key);
		return defaultValue;
	}

	QString value(size / sizeof(wchar_t), QChar());
	RegQueryValueExW(
		key, reinterpret_cast<LPCWSTR>(entry.utf16()), nullptr, &type, reinterpret_cast<LPBYTE>(value.data()), &size
	);
	RegCloseKey(key);
	while (!value.isEmpty() && value.back().isNull()) {
		value.chop(1);
	}
	return value;
}

void WriteProfileString(const QString& section, const QString& entry, const QString& value)
{
	HKEY key = openSectionKey(section, true);
	if (key == nullptr) {
		return;
	}

	RegSetValueExW(
		key,
		reinterpret_cast<LPCWSTR>(entry.utf16()),
		0,
		REG_SZ,
		reinterpret_cast<const BYTE*>(value.utf16()),
		static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t))
	);
	RegCloseKey(key);
}

bool GetProfileBinary(const QString& section, const QString& entry, QByteArray& data)
{
	HKEY key = openSectionKey(section, false);
	if (key == nullptr) {
		return false;
	}

	DWORD type;
	DWORD size = 0;
	LONG result = RegQueryValueExW(key, reinterpret_cast<LPCWSTR>(entry.utf16()), nullptr, &type, nullptr, &size);
	if (result != ERROR_SUCCESS || type != REG_BINARY) {
		RegCloseKey(key);
		return false;
	}

	data.resize(size);
	result = RegQueryValueExW(
		key, reinterpret_cast<LPCWSTR>(entry.utf16()), nullptr, &type, reinterpret_cast<LPBYTE>(data.data()), &size
	);
	RegCloseKey(key);
	return result == ERROR_SUCCESS;
}

void WriteProfileBinary(const QString& section, const QString& entry, const QByteArray& data)
{
	HKEY key = openSectionKey(section, true);
	if (key == nullptr) {
		return;
	}

	RegSetValueExW(
		key,
		reinterpret_cast<LPCWSTR>(entry.utf16()),
		0,
		REG_BINARY,
		reinterpret_cast<const BYTE*>(data.constData()),
		static_cast<DWORD>(data.size())
	);
	RegCloseKey(key);
}
#else
static QSettings& settings()
{
	static QSettings settings(QSettings::IniFormat, QSettings::UserScope, "ROBLOX Corporation", "Roblox");
	return settings;
}

static QString settingsKey(const QString& section, const QString& entry)
{
	return section.isEmpty() ? entry : section + '/' + entry;
}

int GetProfileInt(const QString& section, const QString& entry, int defaultValue)
{
	return settings().value(settingsKey(section, entry), defaultValue).toInt();
}

void WriteProfileInt(const QString& section, const QString& entry, int value)
{
	settings().setValue(settingsKey(section, entry), value);
}

QString GetProfileString(const QString& section, const QString& entry, const QString& defaultValue)
{
	return settings().value(settingsKey(section, entry), defaultValue).toString();
}

void WriteProfileString(const QString& section, const QString& entry, const QString& value)
{
	settings().setValue(settingsKey(section, entry), value);
}

bool GetProfileBinary(const QString& section, const QString& entry, QByteArray& data)
{
	QVariant value = settings().value(settingsKey(section, entry));
	if (!value.isValid()) {
		return false;
	}
	data = value.toByteArray();
	return true;
}

void WriteProfileBinary(const QString& section, const QString& entry, const QByteArray& data)
{
	settings().setValue(settingsKey(section, entry), data);
}
#endif

} // namespace Profile
