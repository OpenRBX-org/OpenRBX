#include "ObjectBrowser.h"

#include "BrowserScrollBar.h"
#include "MainFrm.h"
#include "ReflectionMetadata.h"
#include "Resources.h"
#include "Roblox.h"
#include "SplitterPane.h"
#include "WhidbeyStyle.h"

#include <G3D/format.h>
#include <QApplication>
#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QResizeEvent>
#include <QSplitter>
#include <algorithm>
#include <cstring>
#include <mutex>
#include <string>

static const int splitterBarWidth = 2;
static const int splitterPaneBorder = 4;
static const char* const documentCompleteStatus = "Done";
static const QMargins splitterPaneMargins(2, 2, 2, 2);
static const unsigned objectBrowserImagesId = 196;
static const int imageSize = 16;
static const int initialClassViewWidth = 200;
static const int minimumMemberViewHeight = 50;
static const int reservedDeclarationViewHeight = 150;

enum ObjectBrowserImage
{
	BROWSER_IMAGE_CLASS = 3,
	BROWSER_IMAGE_FUNCTION = 4,
	BROWSER_IMAGE_TRUSTED_FUNCTION = 5,
	BROWSER_IMAGE_PROPERTY = 6,
	BROWSER_IMAGE_SIGNAL = 8,
	BROWSER_IMAGE_ENUM = 9,
	BROWSER_IMAGE_ENUM_ITEM = 10,
};

class DescriptorItem : public QTreeWidgetItem
{
public:
	explicit DescriptorItem(const RBX::Reflection::Descriptor* descriptor) : m_descriptor(descriptor) {}

	const RBX::Reflection::Descriptor* descriptor() const { return m_descriptor; }

private:
	const RBX::Reflection::Descriptor* m_descriptor;
};

static int descriptorRank(const RBX::Reflection::Descriptor* descriptor)
{
	if (dynamic_cast<const RBX::Reflection::EnumDescriptor::Item*>(descriptor)) {
		return 5;
	}

	if (dynamic_cast<const RBX::Reflection::EnumDescriptor*>(descriptor)) {
		return 4;
	}

	if (dynamic_cast<const RBX::Reflection::SignalDescriptor*>(descriptor)) {
		return 3;
	}

	if (dynamic_cast<const RBX::Reflection::PropertyDescriptor*>(descriptor)) {
		return 2;
	}

	if (dynamic_cast<const RBX::Reflection::FunctionDescriptor*>(descriptor)) {
		return 1;
	}

	return dynamic_cast<const RBX::Reflection::ClassDescriptor*>(descriptor) ? 0 : 100;
}

static int compareDescriptors(
	const RBX::Reflection::Descriptor* descriptor1, const RBX::Reflection::Descriptor* descriptor2
)
{
	int rank1 = descriptorRank(descriptor1);
	int rank2 = descriptorRank(descriptor2);
	if (rank1 != rank2) {
		return rank1 - rank2;
	}

	return std::strcmp(descriptor1->name.c_str(), descriptor2->name.c_str());
}

static std::recursive_mutex& browserMutex()
{
	static std::recursive_mutex mutex;

	return mutex;
}

static std::vector<QIcon> loadImageList(unsigned bitmapId)
{
	std::vector<QIcon> images;

	QImage strip(Resources::bitmapPath(bitmapId));
	if (strip.isNull()) {
		return images;
	}

	strip = strip.convertToFormat(QImage::Format_ARGB32);
	QRgb mask = strip.pixel(0, 0);
	for (int y = 0; y < strip.height(); y++) {
		QRgb* line = reinterpret_cast<QRgb*>(strip.scanLine(y));
		for (int x = 0; x < strip.width(); x++) {
			if (line[x] == mask) {
				line[x] = qRgba(0, 0, 0, 0);
			}
		}
	}

	for (int x = 0; x + imageSize <= strip.width(); x += imageSize) {
		images.push_back(QIcon(QPixmap::fromImage(strip.copy(x, 0, imageSize, imageSize))));
	}
	return images;
}

static void appendArgumentName(
	int& count, std::string& text, const RBX::Reflection::SignatureDescriptor::Item& argument
)
{
	if (count > 0) {
		text += ", ";
	}

	text += argument.name->c_str();
	++count;
}

static void appendArgumentDeclaration(
	int& count, std::string& text, const RBX::Reflection::SignatureDescriptor::Item& argument
)
{
	if (count > 0) {
		text += ", ";
	}

	text += argument.type->name.c_str();
	text += " ";
	text += argument.name->c_str();
	if (&argument.defaultValue.type() == argument.type) {
		text += " = ";
		text += argument.defaultValue.get<std::string>();
	}

	++count;
}

CDeclarationView::CDeclarationView(QWidget* parent) : QTextBrowser(parent)
{
	installBrowserScrollBar(this);
	setFont(QApplication::font("QMessageBox"));
	setOpenLinks(false);
	showDeclaration(nullptr, nullptr, nullptr);
}

void CDeclarationView::showDeclaration(
	RBX::Reflection::Metadata::Item* metadata, const char* declaration, const char* owner
)
{
	QString html;

	QString declarationText = QString::fromLocal8Bit(declaration ? declaration : "");
	if (!declarationText.isEmpty()) {
		html += "<div style=\"font-weight: bold\">";
		html += declarationText;
		html += "</div>";
	}

	if (owner) {
		html += "<div>Member of <span style=\"font-weight: bold\">";
		html += QString::fromLocal8Bit(owner);
		html += "</span></div>";
	}

	if (metadata && metadata->preliminary) {
		html += "<p style=\"color: Red\"><b>Preliminary: </b>This item is under development and is likely to change. "
				"Use only for experimental work.</p>";
	}

	if (metadata && metadata->deprecated) {
		html += "<p style=\"color: Red\"><b>Deprecated: </b>This item is deprecated. Do not use it for new work.</p>";
	}

	QString summary = QString::fromLocal8Bit(metadata ? metadata->summary.c_str() : "");
	if (!summary.isEmpty()) {
		html += "<p><span style=\"font-weight: bold\">Summary:</span><br />";
		html += summary;
		html += "</p>";
	}

	setHtml(html);
}

void CDeclarationView::showDescriptor(const RBX::Reflection::Descriptor* descriptor)
{
	if (!descriptor) {
		showDeclaration(nullptr, nullptr, nullptr);
		return;
	}

	std::string declaration;
	RBX::Reflection::Metadata::Item* metadata = nullptr;
	const RBX::Reflection::ClassDescriptor* owner = nullptr;

	if (const RBX::Reflection::FunctionDescriptor* function =
			dynamic_cast<const RBX::Reflection::FunctionDescriptor*>(descriptor)) {
		declaration = function->getSignature().resultType->name.toString();
		declaration += " ";
		declaration += function->name.toString();
		declaration += "(";

		int count = 0;
		for (const RBX::Reflection::SignatureDescriptor::Item& argument : function->getSignature().arguments) {
			appendArgumentDeclaration(count, declaration, argument);
		}
		declaration += ")";

		owner = &function->owner;
		metadata = RBX::Reflection::Metadata::Reflection::singleton()->getFunction(function);
	}
	else if (
		const RBX::Reflection::PropertyDescriptor* property =
			dynamic_cast<const RBX::Reflection::PropertyDescriptor*>(descriptor)
	) {
		declaration = property->type.name.toString();
		declaration += " ";
		declaration += property->name.toString();

		owner = &property->owner;
		metadata = RBX::Reflection::Metadata::Reflection::singleton()->getProperty(property);
	}
	else if (
		const RBX::Reflection::SignalDescriptor* signal =
			dynamic_cast<const RBX::Reflection::SignalDescriptor*>(descriptor)
	) {
		declaration = "event ";
		declaration += signal->name.toString();
		declaration += "(";

		int count = 0;
		for (const RBX::Reflection::SignatureDescriptor::Item& argument : signal->getSignature().arguments) {
			appendArgumentDeclaration(count, declaration, argument);
		}
		declaration += ")";

		owner = &signal->owner;
		metadata = RBX::Reflection::Metadata::Reflection::singleton()->getEvent(signal);
	}
	else if (
		const RBX::Reflection::EnumDescriptor::Item* item =
			dynamic_cast<const RBX::Reflection::EnumDescriptor::Item*>(descriptor)
	) {
		declaration = G3D::format("%d: %s", item->value, item->name.c_str());
	}
	else if (
		const RBX::Reflection::ClassDescriptor* classDescriptor =
			dynamic_cast<const RBX::Reflection::ClassDescriptor*>(descriptor)
	) {
		declaration = classDescriptor->name.toString();
		metadata = RBX::Reflection::Metadata::Reflection::singleton()->getClass(classDescriptor, false);
	}
	else {
		declaration = descriptor->name.toString();
	}

	showDeclaration(metadata, declaration.c_str(), owner ? owner->name.c_str() : nullptr);
}

DescriptorTreeView::DescriptorTreeView(QWidget* parent)
	: TreeWidget(parent), m_images(loadImageList(objectBrowserImagesId))
{
	setColumnCount(1);
	setHeaderHidden(true);
	setRootIsDecorated(false);
	setFrameShape(QFrame::NoFrame);
	setIconSize(QSize(imageSize, imageSize));
}

void DescriptorTreeView::focusInEvent(QFocusEvent* event)
{
	TreeWidget::focusInEvent(event);
	if (selectedItems().isEmpty() && topLevelItemCount() > 0) {
		setCurrentItem(topLevelItem(0));
	}
}

void DescriptorTreeView::addDescriptorItem(
	const QString& text,
	int image,
	const RBX::Reflection::Descriptor* descriptor,
	RBX::Reflection::Metadata::Item* metadata
)
{
	DescriptorItem* item = new DescriptorItem(descriptor);
	item->setText(0, text);
	if (image < static_cast<int>(m_images.size())) {
		item->setIcon(0, m_images[image]);
	}

	if (metadata && (metadata->deprecated || metadata->preliminary)) {
		QFont itemFont = font();
		itemFont.setStrikeOut(metadata->deprecated);
		itemFont.setItalic(metadata->preliminary);
		item->setFont(0, itemFont);
	}

	m_pendingItems.push_back(item);
}

void DescriptorTreeView::insertSortedItems()
{
	std::stable_sort(m_pendingItems.begin(), m_pendingItems.end(), [](QTreeWidgetItem* item1, QTreeWidgetItem* item2) {
		return compareDescriptors(itemDescriptor(item1), itemDescriptor(item2)) < 0;
	});

	for (QTreeWidgetItem* item : m_pendingItems) {
		addTopLevelItem(item);
	}
	m_pendingItems.clear();
}

const RBX::Reflection::Descriptor* DescriptorTreeView::itemDescriptor(QTreeWidgetItem* item)
{
	return item ? static_cast<DescriptorItem*>(item)->descriptor() : nullptr;
}

CMemberTreeView::CMemberTreeView(QWidget* parent) : DescriptorTreeView(parent)
{
	connect(this, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem* current) {
		if (m_declarationView) {
			m_declarationView->showDescriptor(itemDescriptor(current));
		}
	});
}

void CMemberTreeView::addEnumItem(const RBX::Reflection::EnumDescriptor::Item* item)
{
	addDescriptorItem(QString::fromLocal8Bit(item->name.c_str()), BROWSER_IMAGE_ENUM_ITEM, item, nullptr);
}

void CMemberTreeView::addProperty(const RBX::Reflection::PropertyDescriptor* descriptor)
{
	RBX::Reflection::Metadata::Member* metadata =
		RBX::Reflection::Metadata::Reflection::singleton()->getProperty(descriptor);
	if ((!metadata || metadata->browsable) && descriptor->isPublic()) {
		addDescriptorItem(
			QString::fromLocal8Bit(descriptor->name.c_str()), BROWSER_IMAGE_PROPERTY, descriptor, metadata
		);
	}
}

void CMemberTreeView::addSignal(const RBX::Reflection::SignalDescriptor* descriptor)
{
	RBX::Reflection::Metadata::Member* metadata =
		RBX::Reflection::Metadata::Reflection::singleton()->getEvent(descriptor);
	if (!metadata || metadata->browsable) {
		addDescriptorItem(QString::fromLocal8Bit(descriptor->name.c_str()), BROWSER_IMAGE_SIGNAL, descriptor, metadata);
	}
}

void CMemberTreeView::addFunction(const RBX::Reflection::FunctionDescriptor* descriptor)
{
	RBX::Reflection::Metadata::Member* metadata =
		RBX::Reflection::Metadata::Reflection::singleton()->getFunction(descriptor);
	if (!metadata || metadata->browsable) {
		std::string text = descriptor->name.toString();
		text += "(";

		int count = 0;
		for (const RBX::Reflection::SignatureDescriptor::Item& argument : descriptor->getSignature().arguments) {
			appendArgumentName(count, text, argument);
		}
		text += ")";

		int image = descriptor->security == RBX::Reflection::FunctionDescriptor::NeedTrustedCaller
						? BROWSER_IMAGE_TRUSTED_FUNCTION
						: BROWSER_IMAGE_FUNCTION;
		addDescriptorItem(QString::fromLocal8Bit(text.c_str()), image, descriptor, metadata);
	}
}

void CMemberTreeView::showEnum(const RBX::Reflection::EnumDescriptor* descriptor)
{
	clear();

	std::scoped_lock lock(browserMutex());
	std::for_each(descriptor->begin(), descriptor->end(), [this](const RBX::Reflection::EnumDescriptor::Item* item) {
		addEnumItem(item);
	});

	insertSortedItems();
}

void CMemberTreeView::showClass(const RBX::Reflection::ClassDescriptor* descriptor)
{
	clear();

	std::scoped_lock lock(browserMutex());
	std::for_each(
		descriptor
			->RBX::Reflection::MemberDescriptorContainer<RBX::Reflection::PropertyDescriptor>::descriptors_begin(),
		descriptor->RBX::Reflection::MemberDescriptorContainer<RBX::Reflection::PropertyDescriptor>::descriptors_end(),
		[this](const RBX::Reflection::PropertyDescriptor* property) { addProperty(property); }
	);
	std::for_each(
		descriptor
			->RBX::Reflection::MemberDescriptorContainer<RBX::Reflection::FunctionDescriptor>::descriptors_begin(),
		descriptor->RBX::Reflection::MemberDescriptorContainer<RBX::Reflection::FunctionDescriptor>::descriptors_end(),
		[this](const RBX::Reflection::FunctionDescriptor* function) { addFunction(function); }
	);
	std::for_each(
		descriptor->RBX::Reflection::MemberDescriptorContainer<RBX::Reflection::SignalDescriptor>::descriptors_begin(),
		descriptor->RBX::Reflection::MemberDescriptorContainer<RBX::Reflection::SignalDescriptor>::descriptors_end(),
		[this](const RBX::Reflection::SignalDescriptor* signal) { addSignal(signal); }
	);

	insertSortedItems();
}

CClassTreeView::CClassTreeView(QWidget* parent) : DescriptorTreeView(parent)
{
	connect(this, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem* current) {
		const RBX::Reflection::Descriptor* descriptor = itemDescriptor(current);

		if (m_memberView) {
			if (const RBX::Reflection::ClassDescriptor* classDescriptor =
					dynamic_cast<const RBX::Reflection::ClassDescriptor*>(descriptor)) {
				m_memberView->showClass(classDescriptor);
			}
			else if (
				const RBX::Reflection::EnumDescriptor* enumDescriptor =
					dynamic_cast<const RBX::Reflection::EnumDescriptor*>(descriptor)
			) {
				m_memberView->showEnum(enumDescriptor);
			}
		}

		if (m_declarationView) {
			m_declarationView->showDescriptor(descriptor);
		}
	});
}

void CClassTreeView::addEnum(const RBX::Reflection::EnumDescriptor* descriptor)
{
	addDescriptorItem(QString::fromLocal8Bit(descriptor->name.c_str()), BROWSER_IMAGE_ENUM, descriptor, nullptr);
}

void CClassTreeView::addClass(const RBX::Reflection::ClassDescriptor* descriptor)
{
	RBX::Reflection::Metadata::Class* metadata =
		RBX::Reflection::Metadata::Reflection::singleton()->getClass(descriptor, false);
	if (!metadata || metadata->browsable) {
		addDescriptorItem(QString::fromLocal8Bit(descriptor->name.c_str()), BROWSER_IMAGE_CLASS, descriptor, metadata);
	}

	std::for_each(
		descriptor->derivedClasses_begin(),
		descriptor->derivedClasses_end(),
		[this](const RBX::Reflection::ClassDescriptor* derivedClass) { addClass(derivedClass); }
	);
}

void CClassTreeView::initialUpdate()
{
	std::scoped_lock lock(browserMutex());
	addClass(&RBX::Reflection::ClassDescriptor::rootDescriptor());
	std::for_each(
		RBX::Reflection::EnumDescriptor::enumsBegin(),
		RBX::Reflection::EnumDescriptor::enumsEnd(),
		[this](const RBX::Reflection::EnumDescriptor* descriptor) { addEnum(descriptor); }
	);

	insertSortedItems();
}

CObjectBrowser::CObjectBrowser() : m_classViewWidth(initialClassViewWidth)
{
}

CObjectBrowser::~CObjectBrowser() = default;

bool CObjectBrowser::create()
{
	if (!View::create()) {
		return false;
	}

	m_classSplitter = new QSplitter(Qt::Horizontal, this);
	m_classSplitter->setGeometry(0, 0, 0, 0);
	m_classSplitter->setHandleWidth(splitterBarWidth);
	m_classView = new CClassTreeView(nullptr);
	m_classSplitter->addWidget(new SplitterPane(m_classView, splitterPaneMargins, false, m_classSplitter));
	m_memberSplitter = new QSplitter(Qt::Vertical, m_classSplitter);
	m_memberSplitter->setHandleWidth(splitterBarWidth);
	m_memberView = new CMemberTreeView(nullptr);
	m_memberSplitter->addWidget(new SplitterPane(m_memberView, splitterPaneMargins, true, m_memberSplitter));
	m_declarationView = new CDeclarationView(nullptr);
	m_declarationView->setFrameShape(QFrame::NoFrame);
	m_memberSplitter->addWidget(new SplitterPane(m_declarationView, splitterPaneMargins, true, m_memberSplitter));
	m_classSplitter->setStretchFactor(0, 0);
	m_classSplitter->setStretchFactor(1, 1);
	connect(m_classSplitter, &QSplitter::splitterMoved, this, [this]() {
		m_classViewWidth = m_classSplitter->sizes().value(0) - splitterPaneBorder;
	});

	m_classView->setMemberView(m_memberView);
	m_classView->setDeclarationView(m_declarationView);
	m_memberView->setDeclarationView(m_declarationView);

	m_classSplitter->show();

	QResizeEvent event(size(), size());
	resizeEvent(&event);
	return true;
}

void CObjectBrowser::initialUpdate()
{
	View::initialUpdate();
	m_classView->initialUpdate();
	theApp.mainFrame()->setMessageText(QString::fromLatin1(documentCompleteStatus));
}

void CObjectBrowser::paintEvent(QPaintEvent* event)
{
	QPainter painter(this);
	WhidbeyStyle::drawClientEdge(&painter, rect());
}

void CObjectBrowser::resizeEvent(QResizeEvent* event)
{
	View::resizeEvent(event);

	if (m_classSplitter) {
		int width = event->size().width();
		int height = event->size().height();
		int delta = height - m_classSplitter->height();
		int memberViewHeight = m_memberSplitter->sizes().value(0) - splitterPaneBorder;

		int edge = WhidbeyStyle::clientEdgeWidth;
		width -= 2 * edge;
		height -= 2 * edge;
		m_classSplitter->setGeometry(edge, edge, width, height);
		m_classSplitter->setSizes(
			{m_classViewWidth + splitterPaneBorder,
			 std::max(0, width - m_classViewWidth - splitterPaneBorder - m_classSplitter->handleWidth())}
		);

		int top = std::min(height - reservedDeclarationViewHeight, memberViewHeight + delta);
		top = std::max(minimumMemberViewHeight, top);
		m_memberSplitter->setSizes(
			{top + splitterPaneBorder, std::max(0, height - top - splitterPaneBorder - m_memberSplitter->handleWidth())}
		);
	}
}
