#include "FileDialog.h"

#include "Dialog.h"
#include "MainFrm.h"
#include "Roblox.h"

#include <QFileInfo>
#include <QStringList>
#include <QWidget>

#ifdef _WIN32
#include "NativeWindow.h"

#define NOMINMAX
#include <commdlg.h>
#include <vector>
#include <windows.h>
#else
#include <QDir>
#include <QFileDialog>
#endif

CFileDialog::CFileDialog(
	bool openFileDialog,
	const QString& defExt,
	const QString& fileName,
	unsigned flags,
	const QString& filter,
	QWidget* parent
)
	: m_openFileDialog(openFileDialog), m_defExt(defExt), m_fileName(fileName), m_flags(flags), m_filter(filter),
	  m_parent(parent)
{
}

QString CFileDialog::GetFileExt() const
{
	return QFileInfo(m_pathName).suffix();
}

#ifdef _WIN32

static UINT_PTR CALLBACK fileDialogHook(HWND window, UINT message, WPARAM, LPARAM lParam)
{
	if (message == WM_INITDIALOG) {
		return TRUE;
	}
	if (message == WM_NOTIFY && reinterpret_cast<OFNOTIFYW*>(lParam)->hdr.code == CDN_INITDONE) {
		centerWindowOverOwner(GetParent(window));
	}
	return 0;
}

static HWND safeOwner(QWidget* parent)
{
	HWND window = parent ? reinterpret_cast<HWND>(parent->window()->winId())
						 : reinterpret_cast<HWND>(theApp.mainFrame()->winId());
	while (window && (GetWindowLongW(window, GWL_STYLE) & WS_CHILD)) {
		window = GetParent(window);
	}
	if (!parent && window) {
		window = GetLastActivePopup(window);
	}
	return window;
}

int CFileDialog::DoModal()
{
	std::wstring filter = m_filter.toStdWString();
	for (wchar_t& character : filter) {
		if (character == L'|') {
			character = L'\0';
		}
	}
	std::wstring defExt = m_defExt.toStdWString();
	std::wstring title = m_title.toStdWString();
	std::vector<wchar_t> file(MAX_PATH, L'\0');
	std::vector<wchar_t> fileTitle(64, L'\0');
	std::wstring initial = m_fileName.toStdWString();
	wcsncpy_s(file.data(), file.size(), initial.c_str(), _TRUNCATE);

	OPENFILENAMEW ofn = {};
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = safeOwner(m_parent);
	ofn.lpstrFile = file.data();
	ofn.nMaxFile = static_cast<DWORD>(file.size());
	ofn.lpstrDefExt = m_defExt.isEmpty() ? nullptr : defExt.c_str();
	ofn.lpstrFileTitle = fileTitle.data();
	ofn.nMaxFileTitle = static_cast<DWORD>(fileTitle.size());
	ofn.Flags = m_flags | OFN_ENABLEHOOK | OFN_EXPLORER;
	ofn.lpfnHook = fileDialogHook;
	ofn.lpstrFilter = m_filter.isEmpty() ? nullptr : filter.c_str();
	ofn.nFilterIndex = static_cast<DWORD>(m_filterIndex);
	ofn.lpstrTitle = m_title.isEmpty() ? nullptr : title.c_str();

	BOOL result = m_openFileDialog ? GetOpenFileNameW(&ofn) : GetSaveFileNameW(&ofn);
	if (!result) {
		return IDCANCEL;
	}
	m_pathName = QString::fromWCharArray(file.data());
	return IDOK;
}

#else

int CFileDialog::DoModal()
{
	QStringList parts = m_filter.split(QLatin1Char('|'));
	QStringList filters;
	for (qsizetype i = 0; i + 1 < parts.size() && !parts[i].isEmpty(); i += 2) {
		QString patterns = QString(parts[i + 1]).replace(QLatin1Char(';'), QLatin1Char(' '));
		filters.append(
			parts[i].contains(QLatin1Char('(')) ? parts[i] : QStringLiteral("%1 (%2)").arg(parts[i], patterns)
		);
	}
	QString selectedFilter =
		m_filterIndex > 0 && m_filterIndex <= filters.size() ? filters[m_filterIndex - 1] : QString();
	QWidget* parent = m_parent ? m_parent->window() : theApp.mainFrame();
	QString title =
		m_title.isEmpty() ? (m_openFileDialog ? QStringLiteral("Open") : QStringLiteral("Save As")) : m_title;
	QFileDialog::Options options;
	if (!(m_flags & OverwritePrompt)) {
		options |= QFileDialog::DontConfirmOverwrite;
	}
	QString path = m_openFileDialog
					   ? QFileDialog::getOpenFileName(
							 parent, title, m_fileName, filters.join(QStringLiteral(";;")), &selectedFilter, options
						 )
					   : QFileDialog::getSaveFileName(
							 parent, title, m_fileName, filters.join(QStringLiteral(";;")), &selectedFilter, options
						 );
	if (path.isEmpty()) {
		return IDCANCEL;
	}
	if (!m_defExt.isEmpty() && QFileInfo(path).suffix().isEmpty()) {
		path += QLatin1Char('.') + m_defExt;
	}
	m_pathName = QDir::toNativeSeparators(path);
	return IDOK;
}

#endif
