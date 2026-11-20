#include "PropertySheet.h"

#include "Dialog.h"
#include "NativeWindow.h"

#include <QEventLoop>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QStyleOptionTab>
#include <QStyleOptionTabWidgetFrame>
#include <QTabBar>
#include <algorithm>
#include <functional>

static const int sheetMarginX = 4;
static const int sheetMarginY = 4;
static const int buttonWidth = 50;
static const int buttonHeight = 14;
static const int buttonGap = 4;
static const int pageInset = 4;
static const int tabHeaderHeight = 22;
static const int sheetFontPoints = 8;
static const int tabTextPadding = 12;
static const int tabStripInset = 2;
static const int selectedLabelShift = 2;
static const int labelInsetX = 8;
static const int labelInsetY = 2;

class PropertySheetTabs : public QWidget
{
public:
	explicit PropertySheetTabs(QWidget* parent) : QWidget(parent) { setFocusPolicy(Qt::TabFocus); }

	void addTab(const QString& text)
	{
		m_texts.push_back(text);
		update();
	}

	int currentIndex() const { return m_current; }

	void setCurrentIndex(int index)
	{
		if (index == m_current || index < 0 || index >= static_cast<int>(m_texts.size())) {
			return;
		}
		m_current = index;
		update();
		if (currentChanged) {
			currentChanged(index);
		}
	}

	std::function<void(int)> currentChanged;

protected:
	QRect tabRect(int index) const
	{
		QFont captionFont = font();
		captionFont.setKerning(false);
		QFontMetrics metrics(captionFont);
		int left = tabStripInset;
		for (int i = 0; i < index; i++) {
			left += metrics.horizontalAdvance(captionText(i)) + tabTextPadding;
		}
		return QRect(left, 0, metrics.horizontalAdvance(captionText(index)) + tabTextPadding, height());
	}

	QString captionText(int index) const
	{
		QString text = m_texts[index];
		text.replace(QLatin1String("&&"), QString(QChar::Null));
		text.remove(QLatin1Char('&'));
		text.replace(QChar(QChar::Null), QLatin1Char('&'));
		return text;
	}

	void paintEvent(QPaintEvent* event) override
	{
		QPainter painter(this);
		for (int pass = 0; pass < 2; pass++) {
			for (int i = 0; i < static_cast<int>(m_texts.size()); i++) {
				bool selected = i == m_current;
				if (selected != (pass == 1)) {
					continue;
				}
				QStyleOptionTab tab;
				tab.initFrom(this);
				tab.styleObject = nullptr;
				tab.rect = tabRect(i);
				tab.shape = QTabBar::RoundedNorth;
				tab.position = QStyleOptionTab::Middle;
				tab.state = QStyle::State_Enabled | (selected ? QStyle::State_Selected : QStyle::State_None);
				if (!selected && i == 0) {
					tab.position = QStyleOptionTab::Beginning;
					tab.rect.adjust(-style()->pixelMetric(QStyle::PM_TabBarTabOverlap, &tab, this), 0, 0, 0);
				}
				else if (!selected && i == static_cast<int>(m_texts.size()) - 1) {
					tab.position = QStyleOptionTab::End;
				}
				style()->drawControl(QStyle::CE_TabBarTabShape, &tab, &painter, this);

				QRect label = tabRect(i).adjusted(labelInsetX, labelInsetY, 0, labelInsetY);
				if (selected) {
					label.translate(selectedLabelShift, -selectedLabelShift);
				}
				style()->drawItemText(
					&painter,
					label,
					Qt::AlignLeft | Qt::AlignVCenter | Qt::TextHideMnemonic,
					palette(),
					true,
					m_texts[i],
					QPalette::WindowText
				);
			}
		}
	}

	void mousePressEvent(QMouseEvent* event) override
	{
		for (int i = 0; i < static_cast<int>(m_texts.size()); i++) {
			if (tabRect(i).contains(event->position().toPoint())) {
				setCurrentIndex(i);
				return;
			}
		}
	}

	void keyPressEvent(QKeyEvent* event) override
	{
		int count = static_cast<int>(m_texts.size());
		if (event->key() == Qt::Key_Right) {
			setCurrentIndex((m_current + 1) % count);
		}
		else if (event->key() == Qt::Key_Left) {
			setCurrentIndex((m_current + count - 1) % count);
		}
		else {
			QWidget::keyPressEvent(event);
		}
	}

private:
	std::vector<QString> m_texts;
	int m_current = 0;
};

CPropertySheet::CPropertySheet(const QString& caption, QWidget* parent)
	: QDialog(
		  parent,
		  Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint |
			  Qt::WindowCloseButtonHint
	  ),
	  m_tabs(new PropertySheetTabs(this))
{
	setWindowTitle(caption);
	QFont font(QStringLiteral("MS Shell Dlg"));
	font.setPixelSize((sheetFontPoints * logicalDpiY() + 36) / 72);
	font.setKerning(false);
	setFont(font);

	m_tabs->currentChanged = [this](int index) {
		for (int i = 0; i < static_cast<int>(m_pages.size()); i++) {
			m_pages[i]->setVisible(i == index);
		}
	};

	m_ok = new QPushButton(QStringLiteral("OK"), this);
	m_cancel = new QPushButton(QStringLiteral("Cancel"), this);
	m_apply = new QPushButton(QStringLiteral("&Apply"), this);
	m_help = new QPushButton(QStringLiteral("Help"), this);
	for (QPushButton* button : {m_ok, m_cancel, m_apply, m_help}) {
		button->setAutoDefault(false);
		button->setFont(font);
	}
	m_ok->setDefault(true);
	connect(m_ok, &QPushButton::clicked, this, &QDialog::accept);
	connect(m_cancel, &QPushButton::clicked, this, &QDialog::reject);
}

void CPropertySheet::AddPage(CDialog* page)
{
	page->setParent(this, Qt::Widget);
	m_pages.push_back(page);
	m_tabs->addTab(page->caption());
	layoutPages();
}

int CPropertySheet::RunModeless()
{
	layoutPages();
	QEventLoop loop;
	connect(this, &QDialog::finished, &loop, &QEventLoop::exit);
	show();
	return loop.exec();
}

QRect CPropertySheet::tabRect() const
{
	QSize page;
	for (CDialog* dialog : m_pages) {
		page = page.expandedTo(dialog->size());
	}
	QRect margin = CDialog::MapDialogRect(sheetMarginX, sheetMarginY, 0, 0);
	return QRect(margin.x(), margin.y(), page.width() + 2 * pageInset, page.height() + tabHeaderHeight + pageInset);
}

void CPropertySheet::layoutPages()
{
	QRect tab = tabRect();
	QRect pageRect(
		tab.x() + pageInset,
		tab.y() + tabHeaderHeight,
		tab.width() - 2 * pageInset,
		tab.height() - tabHeaderHeight - pageInset
	);
	for (CDialog* page : m_pages) {
		page->setGeometry(pageRect);
		page->setVisible(page == m_pages[std::max(0, m_tabs->currentIndex())]);
	}
	m_tabs->setGeometry(tab.x(), tab.y(), tab.width(), tabHeaderHeight - tabStripInset);

	QRect button = CDialog::MapDialogRect(0, 0, buttonWidth, buttonHeight);
	QRect gap = CDialog::MapDialogRect(buttonGap, buttonGap, 0, 0);
	int top = tab.bottom() + 1 + gap.x();
	int right = tab.right() + 1;
	std::vector<QPushButton*> visible;
	for (QPushButton* candidate : {m_ok, m_cancel, m_apply, m_help}) {
		if (!candidate->isHidden()) {
			visible.push_back(candidate);
		}
	}
	int left =
		right - static_cast<int>(visible.size()) * button.width() - (static_cast<int>(visible.size()) - 1) * gap.x();
	for (QPushButton* candidate : visible) {
		candidate->setGeometry(left, top, button.width(), button.height());
		left += button.width() + gap.x();
	}

	QRect margin = CDialog::MapDialogRect(sheetMarginX, sheetMarginY, 0, 0);
	setFixedSize(right + margin.x(), top + button.height() + margin.y());
}

void CPropertySheet::showEvent(QShowEvent* event)
{
	QDialog::showEvent(event);
	removeWindowIcon(this);
	centerWindowOverOwner(this);
	int current = std::max(0, m_tabs->currentIndex());
	if (current < static_cast<int>(m_pages.size())) {
		CDialog* page = m_pages[current];
		for (QWidget* child = page->nextInFocusChain(); child != page && page->isAncestorOf(child);
			 child = child->nextInFocusChain()) {
			if (child->isVisible() && child->isEnabled() && (child->focusPolicy() & Qt::TabFocus)) {
				child->setFocus(Qt::TabFocusReason);
				break;
			}
		}
	}
}

void CPropertySheet::paintEvent(QPaintEvent* event)
{
	QPainter painter(this);
	QStyleOptionTabWidgetFrame frame;
	frame.initFrom(this);
	QRect tab = tabRect();
	frame.rect = QRect(tab.x(), tab.y() + tabHeaderHeight - 2, tab.width(), tab.height() - tabHeaderHeight + 2);
	frame.shape = QTabBar::RoundedNorth;
	frame.tabBarSize = QSize();
	style()->drawPrimitive(QStyle::PE_FrameTabWidget, &frame, &painter, nullptr);
}
