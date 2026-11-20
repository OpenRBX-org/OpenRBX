#ifndef OBJECTBROWSER_H
#define OBJECTBROWSER_H

#include "DocView.h"
#include "TreeWidget.h"
#include "reflection/EnumConverter.h"
#include "reflection/function.h"
#include "reflection/object.h"
#include "reflection/property.h"
#include "reflection/signal.h"

#include <QIcon>
#include <QTextBrowser>
#include <vector>

class QSplitter;

namespace RBX::Reflection::Metadata {
class Item;
}

class CDeclarationView : public QTextBrowser
{
public:
	explicit CDeclarationView(QWidget* parent = nullptr);

	void showDeclaration(RBX::Reflection::Metadata::Item* metadata, const char* declaration, const char* owner);
	void showDescriptor(const RBX::Reflection::Descriptor* descriptor);
};

class DescriptorTreeView : public TreeWidget
{
protected:
	explicit DescriptorTreeView(QWidget* parent);

	void addDescriptorItem(
		const QString& text,
		int image,
		const RBX::Reflection::Descriptor* descriptor,
		RBX::Reflection::Metadata::Item* metadata
	);
	void insertSortedItems();
	void focusInEvent(QFocusEvent* event) override;

	static const RBX::Reflection::Descriptor* itemDescriptor(QTreeWidgetItem* item);

private:
	std::vector<QIcon> m_images;
	std::vector<QTreeWidgetItem*> m_pendingItems;
};

class CMemberTreeView : public DescriptorTreeView
{
public:
	explicit CMemberTreeView(QWidget* parent = nullptr);

	void setDeclarationView(CDeclarationView* declarationView) { m_declarationView = declarationView; }

	void showEnum(const RBX::Reflection::EnumDescriptor* descriptor);
	void showClass(const RBX::Reflection::ClassDescriptor* descriptor);

private:
	void addEnumItem(const RBX::Reflection::EnumDescriptor::Item* item);
	void addProperty(const RBX::Reflection::PropertyDescriptor* descriptor);
	void addFunction(const RBX::Reflection::FunctionDescriptor* descriptor);
	void addSignal(const RBX::Reflection::SignalDescriptor* descriptor);

	CDeclarationView* m_declarationView = nullptr;
};

class CClassTreeView : public DescriptorTreeView
{
public:
	explicit CClassTreeView(QWidget* parent = nullptr);

	void setMemberView(CMemberTreeView* memberView) { m_memberView = memberView; }
	void setDeclarationView(CDeclarationView* declarationView) { m_declarationView = declarationView; }

	void initialUpdate();

private:
	void addClass(const RBX::Reflection::ClassDescriptor* descriptor);
	void addEnum(const RBX::Reflection::EnumDescriptor* descriptor);

	CMemberTreeView* m_memberView = nullptr;
	CDeclarationView* m_declarationView = nullptr;
};

class CObjectBrowser : public View
{
public:
	CObjectBrowser();
	~CObjectBrowser() override;

	bool create() override;
	void initialUpdate() override;

protected:
	void paintEvent(QPaintEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;

private:
	QSplitter* m_classSplitter = nullptr;
	QSplitter* m_memberSplitter = nullptr;
	CClassTreeView* m_classView = nullptr;
	CMemberTreeView* m_memberView = nullptr;
	CDeclarationView* m_declarationView = nullptr;
	int m_classViewWidth;
};

#endif // OBJECTBROWSER_H
