#ifndef FILEDIALOG_H
#define FILEDIALOG_H

#include "Dialog.h"

#include <QString>

class QWidget;

class CFileDialog
{
public:
	enum Flags
	{
		OverwritePrompt = 0x00000002,
		HideReadOnly = 0x00000004,
		PathMustExist = 0x00000800,
		FileMustExist = 0x00001000,
	};

	CFileDialog(
		bool openFileDialog,
		const QString& defExt = QString(),
		const QString& fileName = QString(),
		unsigned flags = HideReadOnly | OverwritePrompt,
		const QString& filter = QString(),
		QWidget* parent = nullptr
	);

	void setTitle(const QString& title) { m_title = title; }
	void addFlags(unsigned flags) { m_flags |= flags; }
	void setDefExt(const QString& defExt) { m_defExt = defExt; }
	void setFileName(const QString& fileName) { m_fileName = fileName; }
	void setFilter(const QString& filter) { m_filter = filter; }
	void setFilterIndex(int index) { m_filterIndex = index; }
	int DoModal();
	QString GetPathName() const { return m_pathName; }
	QString GetFileExt() const;

private:
	bool m_openFileDialog;
	QString m_defExt;
	QString m_fileName;
	unsigned m_flags;
	QString m_filter;
	QWidget* m_parent;
	QString m_title;
	int m_filterIndex = 0;
	QString m_pathName;
};

#endif // FILEDIALOG_H
