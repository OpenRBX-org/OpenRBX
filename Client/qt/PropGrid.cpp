#include "PropGrid.h"

#include "ColorSelector.h"
#include "CommandIds.h"
#include "FileDialog.h"
#include "ImageManager.h"
#include "InstanceExplorer.h"
#include "Resources.h"
#include "Roblox.h"
#include "SelectionCaption.h"
#include "SystemColorDialog.h"
#include "SystemMessageBox.h"
#include "WhidbeyStyle.h"
#include "script/Script.h"
#include "util/ContentProvider.h"
#include "v8datamodel/DataModel.h"

#include <QApplication>
#include <QCursor>
#include <QFocusEvent>
#include <QFontMetrics>
#include <QHelpEvent>
#include <QImage>
#include <QKeyEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QScreen>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QStyle>
#include <QStyleOption>
#include <QStyledItemDelegate>
#include <QToolTip>
#include <QWheelEvent>
#include <QWidgetAction>
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <functional>

static const int expandBorder = 14;
static const int toolBarHeight = 25;
static const int captionHeight = 28;
static const int toolBarButtonWidth = 23;
static const int toolBarButtonHeight = 22;
static const int toolBarImageSize = 16;
static const int splitterBorder = 1;
static const int splitterGap = 4;
static const int dropDownItemCount = 10;
static const int lineEditVerticalMargin = 1;
static const int inplaceListTextInset = 2;
static const int inplaceListImageGap = 5;
static const int comboDropDownButtonPart = 1;
static const int comboButtonNormal = 1;
static const int comboButtonHot = 2;
static const int comboButtonPressed = 3;
static const int listBoxBorderVScrollPart = 4;
static const int listBoxBorderNoScrollPart = 3;
static const int listBoxBorderNormal = 1;
static const int listBoxBorderFocused = 2;
static const int listBoxBorderHot = 3;
static const int listBoxBorderDisabled = 4;
static const unsigned soundFilterStringId = 134;
static const unsigned imageFilterStringId = 132;

static const unsigned charBackspace = 8;
static const unsigned charTab = 9;
static const unsigned charReturn = 13;
static const unsigned charEscape = 27;

static const char* const toolBarPixels[16] = {
	"78888888777777777777707777774777",
	"78777778777777777777808777777777",
	"78770778700007777777070777774777",
	"78700078777777777778070877777777",
	"78770778788878877770000077774777",
	"78777778777777777770777077774777",
	"78888888788878877700070007774777",
	"77778777777777777777777777774777",
	"77778777788878877770000077774777",
	"78888888777777777770777077774777",
	"78fffff8788878877777770877444447",
	"78ff0ff8777777777777708777844487",
	"78f000f8777777777777087777744477",
	"78ff0ff8700007777770877077784877",
	"78fffff8777777777770000077774777",
	"78888888788878877777777777777777",
};

static QRect rectFromEdges(int left, int top, int right, int bottom)
{
	return QRect(left, top, right - left, bottom - top);
}

static int rightEdge(const QRect& rect)
{
	return rect.x() + rect.width();
}

static int bottomEdge(const QRect& rect)
{
	return rect.y() + rect.height();
}

static QPoint centerPoint(const QRect& rect)
{
	return QPoint((rect.x() + rightEdge(rect)) / 2, (rect.y() + bottomEdge(rect)) / 2);
}

static void draw3dRect(QPainter& painter, const QRect& rect, const QColor& topLeft, const QColor& bottomRight)
{
	int x = rect.x();
	int y = rect.y();
	int cx = rect.width();
	int cy = rect.height();
	painter.fillRect(x, y, cx - 1, 1, topLeft);
	painter.fillRect(x, y, 1, cy - 1, topLeft);
	painter.fillRect(x + cx - 1, y, 1, cy, bottomRight);
	painter.fillRect(x, y + cy - 1, cx, 1, bottomRight);
}

static void drawClippedText(QPainter& painter, const QRect& rect, int flags, const QString& text)
{
	if (rect.width() <= 0 || rect.height() <= 0) {
		return;
	}

	painter.save();
	painter.setClipRect(rect, Qt::IntersectClip);
	WhidbeyStyle::drawText(&painter, rect, flags | Qt::TextSingleLine, text);
	painter.restore();
}

static void drawFocusRect(QPainter& painter, const QWidget* widget, const QRect& rect, const QColor& background)
{
	QStyleOptionFocusRect option;
	option.initFrom(widget);
	option.rect = rect;
	option.backgroundColor = background;
	widget->style()->drawPrimitive(QStyle::PE_FrameFocusRect, &option, &painter, widget);
}

static QIcon toolBarIcon(int index, const QPalette& palette)
{
	QImage image(16, 16, QImage::Format_ARGB32);
	for (int y = 0; y < 16; y++) {
		for (int x = 0; x < 16; x++) {
			QColor color(0, 0, 0, 0);
			switch (toolBarPixels[y][index * 16 + x]) {
			case '0':
				color = palette.color(QPalette::ButtonText);
				break;
			case '8':
				color = palette.color(QPalette::Dark);
				break;
			case 'f':
				color = palette.color(QPalette::Light);
				break;
			case '4':
				color = QColor(0, 0, 128);
				break;
			}
			image.setPixelColor(x, y, color);
		}
	}
	return QIcon(QPixmap::fromImage(image));
}

class PropertyGridToolBar : public QWidget
{
public:
	struct Button
	{
		QIcon icon;
		QString toolTip;
		std::function<void()> clicked;
		bool checked = false;
	};

	explicit PropertyGridToolBar(QWidget* parent) : QWidget(parent)
	{
		setMouseTracking(true);
		setAttribute(Qt::WA_Hover, true);
	}

	void addButton(const QIcon& icon, const QString& toolTip, std::function<void()> clicked)
	{
		m_buttons.push_back({icon, toolTip, std::move(clicked)});
		update();
	}

	void setChecked(int index, bool checked)
	{
		if (m_buttons[index].checked != checked) {
			m_buttons[index].checked = checked;
			update();
		}
	}

protected:
	bool event(QEvent* event) override
	{
		if (event->type() == QEvent::ToolTip) {
			QHelpEvent* help = static_cast<QHelpEvent*>(event);
			int index = buttonAt(help->pos());
			if (index >= 0) {
				QToolTip::showText(help->globalPos(), m_buttons[index].toolTip, this, buttonRect(index));
			}
			else {
				QToolTip::hideText();
			}
			return true;
		}
		return QWidget::event(event);
	}

	void paintEvent(QPaintEvent* event) override
	{
		static const ButtonFrame checkedFrame = {
			QColor(153, 209, 255),
			QColor(204, 232, 255),
			QColor(237, 239, 241),
			QColor(186, 220, 249),
			QColor(156, 210, 255),
			QColor(170, 217, 255),
			QColor(200, 230, 255),
		};
		static const ButtonFrame hotFrame = {
			QColor(204, 232, 255),
			QColor(229, 243, 255),
			QColor(239, 240, 241),
			QColor(218, 235, 249),
			QColor(205, 232, 255),
			QColor(212, 236, 255),
			QColor(227, 242, 255),
		};

		QPainter painter(this);
		painter.fillRect(rect(), WhidbeyStyle::colors().face);

		for (int index = 0; index < static_cast<int>(m_buttons.size()); index++) {
			const Button& button = m_buttons[index];
			QRect rc = buttonRect(index);
			bool hot = index == m_hot;
			bool down = button.checked || (index == m_pressed && hot);
			if (down || hot) {
				drawButtonFrame(painter, rc, down ? checkedFrame : hotFrame);
			}
			button.icon.paint(
				&painter,
				QRect(
					rc.left() + (rc.width() - toolBarImageSize) / 2 + (down ? 1 : 0),
					rc.top() + (rc.height() - toolBarImageSize) / 2,
					toolBarImageSize,
					toolBarImageSize
				)
			);
		}
	}

	void mouseMoveEvent(QMouseEvent* event) override { setHot(buttonAt(event->position().toPoint())); }

	void leaveEvent(QEvent* event) override { setHot(-1); }

	void mousePressEvent(QMouseEvent* event) override
	{
		if (event->button() == Qt::LeftButton) {
			m_pressed = buttonAt(event->position().toPoint());
			update();
		}
	}

	void mouseReleaseEvent(QMouseEvent* event) override
	{
		if (event->button() != Qt::LeftButton) {
			return;
		}

		int pressed = m_pressed;
		m_pressed = -1;
		update();
		if (pressed >= 0 && pressed == buttonAt(event->position().toPoint())) {
			m_buttons[pressed].clicked();
		}
	}

private:
	struct ButtonFrame
	{
		QColor border;
		QColor fill;
		QColor corner;
		QColor cornerEdge;
		QColor borderEnd;
		QColor cornerInner;
		QColor fillEdge;
	};

	static void drawButtonFrame(QPainter& painter, const QRect& rc, const ButtonFrame& frame)
	{
		painter.fillRect(rc.adjusted(1, 1, -1, -1), frame.fill);
		painter.fillRect(rc.left() + 3, rc.top(), rc.width() - 6, 1, frame.border);
		painter.fillRect(rc.left() + 3, rc.bottom(), rc.width() - 6, 1, frame.border);
		painter.fillRect(rc.left(), rc.top() + 3, 1, rc.height() - 6, frame.border);
		painter.fillRect(rc.right(), rc.top() + 3, 1, rc.height() - 6, frame.border);

		for (int side = 0; side < 4; side++) {
			int x = side & 1 ? rc.right() : rc.left();
			int y = side & 2 ? rc.bottom() : rc.top();
			int dx = side & 1 ? -1 : 1;
			int dy = side & 2 ? -1 : 1;
			painter.fillRect(x, y, 1, 1, frame.corner);
			painter.fillRect(x + dx, y, 1, 1, frame.cornerEdge);
			painter.fillRect(x, y + dy, 1, 1, frame.cornerEdge);
			painter.fillRect(x + 2 * dx, y, 1, 1, frame.borderEnd);
			painter.fillRect(x, y + 2 * dy, 1, 1, frame.borderEnd);
			painter.fillRect(x + dx, y + dy, 1, 1, frame.cornerInner);
			painter.fillRect(x + 2 * dx, y + dy, 1, 1, frame.fillEdge);
			painter.fillRect(x + dx, y + 2 * dy, 1, 1, frame.fillEdge);
		}
	}

	QRect buttonRect(int index) const
	{
		return QRect(index * toolBarButtonWidth, 0, toolBarButtonWidth, toolBarButtonHeight);
	}

	int buttonAt(const QPoint& point) const
	{
		for (int index = 0; index < static_cast<int>(m_buttons.size()); index++) {
			if (buttonRect(index).contains(point)) {
				return index;
			}
		}
		return -1;
	}

	void setHot(int index)
	{
		if (m_hot != index) {
			m_hot = index;
			update();
		}
	}

	std::vector<Button> m_buttons;
	int m_hot = -1;
	int m_pressed = -1;
};

static PropertyGridItem* findItemByCaption(const std::vector<PropertyGridItem*>& items, const QString& caption)
{
	for (PropertyGridItem* item : items) {
		if (item->caption().compare(caption) == 0) {
			return item;
		}

		if (PropertyGridItem* child = findItemByCaption(item->children(), caption)) {
			return child;
		}
	}
	return nullptr;
}

static unsigned characterFromKeyEvent(const QKeyEvent* event)
{
	switch (event->key()) {
	case Qt::Key_Tab:
	case Qt::Key_Backtab:
		return charTab;
	case Qt::Key_Return:
	case Qt::Key_Enter:
		return charReturn;
	case Qt::Key_Escape:
		return charEscape;
	case Qt::Key_Backspace:
		return charBackspace;
	}

	QString text = event->text();
	return text.isEmpty() ? 0 : text.at(0).unicode();
}

PropertyGridConstraint* PropertyGridConstraints::add(const QString& text, qintptr data, int image)
{
	std::unique_ptr<PropertyGridConstraint> constraint = std::make_unique<PropertyGridConstraint>();
	constraint->text = text;
	constraint->data = data;
	constraint->image = image;

	PropertyGridConstraint* result = constraint.get();
	m_constraints.push_back(std::move(constraint));
	m_item->onConstraintsChanged();
	return result;
}

QString PropertyGridConstraints::textAt(int index) const
{
	return index >= 0 && index < count() ? m_constraints[index]->text : QString();
}

int PropertyGridConstraints::find(const QString& text) const
{
	for (int i = 0; i < count(); i++) {
		if (m_constraints[i]->text.compare(text, Qt::CaseInsensitive) == 0) {
			return i;
		}
	}
	return -1;
}

int PropertyGridConstraints::findData(qintptr data) const
{
	for (int i = 0; i < count(); i++) {
		if (m_constraints[i]->data == data) {
			return i;
		}
	}
	return -1;
}

PropertyGridItem::PropertyGridItem(const QString& caption, const QString& value) : m_value(value), m_constraints(this)
{
	qsizetype newline = caption.indexOf('\n');
	m_caption = newline == -1 ? caption : caption.left(newline);
}

PropertyGridItem::~PropertyGridItem()
{
	if (m_view) {
		m_view->itemDestroyed(this);
	}

	for (PropertyGridItem* child : m_children) {
		delete child;
	}
}

void PropertyGridItem::onAddChildItem()
{
	if (!m_category) {
		setFlags(m_flags);
	}
}

void PropertyGridItem::onConstraintsChanged()
{
}

void PropertyGridItem::addInplaceButton(unsigned id)
{
	if (findInplaceButton(id)) {
		return;
	}

	std::unique_ptr<PropertyGridInplaceButton> button = std::make_unique<PropertyGridInplaceButton>();
	button->id = id;
	button->item = this;
	button->width = QApplication::style()->pixelMetric(QStyle::PM_ScrollBarExtent);
	m_inplaceButtons.push_back(std::move(button));
	for (int i = 0; i < static_cast<int>(m_inplaceButtons.size()); i++) {
		m_inplaceButtons[i]->index = i;
	}
}

void PropertyGridItem::removeInplaceButton(unsigned id)
{
	for (auto it = m_inplaceButtons.begin(); it != m_inplaceButtons.end(); ++it) {
		if ((*it)->id == id) {
			if (m_view) {
				m_view->inplaceButtonRemoved(it->get());
			}
			m_inplaceButtons.erase(it);
			break;
		}
	}

	for (int i = 0; i < static_cast<int>(m_inplaceButtons.size()); i++) {
		m_inplaceButtons[i]->index = i;
	}
}

void PropertyGridItem::setFlags(unsigned flags)
{
	m_flags = flags;
	removeInplaceButton(PROPERTY_GRID_EXPAND_BUTTON);
	removeInplaceButton(PROPERTY_GRID_COMBO_BUTTON);

	if (m_flags & PROPERTY_GRID_ITEM_HAS_EXPAND_BUTTON) {
		addInplaceButton(PROPERTY_GRID_EXPAND_BUTTON);
	}
	if (m_flags & PROPERTY_GRID_ITEM_HAS_COMBO_BUTTON) {
		addInplaceButton(PROPERTY_GRID_COMBO_BUTTON);
	}
}

PropertyGridInplaceButton* PropertyGridItem::findInplaceButton(unsigned id) const
{
	for (const std::unique_ptr<PropertyGridInplaceButton>& button : m_inplaceButtons) {
		if (button->id == id) {
			return button.get();
		}
	}
	return nullptr;
}

QRect PropertyGridItem::inplaceButtonRect(const PropertyGridInplaceButton* button) const
{
	QRect rect = itemRect();
	int right = rightEdge(rect);
	for (int i = static_cast<int>(m_inplaceButtons.size()) - 1; i >= 0; i--) {
		int width = m_inplaceButtons[i]->width;
		if (m_inplaceButtons[i].get() == button) {
			return rectFromEdges(right - width, rect.y(), right, bottomEdge(rect) - 1);
		}
		right -= width;
	}
	return QRect();
}

PropertyGridInplaceButton* PropertyGridItem::inplaceButtonAt(QPoint point) const
{
	if (!isInplaceButtonsVisible()) {
		return nullptr;
	}

	for (const std::unique_ptr<PropertyGridInplaceButton>& button : m_inplaceButtons) {
		if (inplaceButtonRect(button.get()).contains(point)) {
			return button.get();
		}
	}
	return nullptr;
}

bool PropertyGridItem::isInplaceButtonsVisible() const
{
	if (!m_view) {
		return false;
	}

	if (!isSelected()) {
		return false;
	}

	return !isReadOnly() && !m_inplaceButtons.empty();
}

PropertyGridItem* PropertyGridItem::addChildItem(PropertyGridItem* item)
{
	return insertChildItem(static_cast<int>(m_children.size()), item);
}

void PropertyGridItem::setIndent(int indent)
{
	m_indent = indent;
	for (PropertyGridItem* child : m_children) {
		child->setIndent(indent + 1);
	}
}

PropertyGridItem* PropertyGridItem::insertChildItem(int index, PropertyGridItem* item)
{
	if (!item) {
		return nullptr;
	}

	if (!m_view) {
		return item;
	}

	if (index < 0 || index > static_cast<int>(m_children.size())) {
		index = static_cast<int>(m_children.size());
	}

	m_children.insert(m_children.begin() + index, item);
	item->m_view = m_view;
	item->m_parent = this;
	item->setIndent(m_indent + 1);
	item->onAddChildItem();

	if (m_visible) {
		if (m_expanded) {
			m_view->setPropertySort(m_view->m_sort, true);
		}
		else if (m_children.size() == 1) {
			m_view->update();
		}
	}
	return item;
}

void PropertyGridItem::collapse()
{
	if (!m_expanded) {
		return;
	}

	if (m_view->m_lockUpdate == 0) {
		if (m_visible) {
			m_view->doCollapse(this);
			m_view->onSelectionChanged();
			m_view->rowsChanged();
		}
	}
	m_expanded = false;
}

void PropertyGridItem::expand()
{
	if (m_expanded) {
		return;
	}

	if (m_view->m_lockUpdate == 0) {
		if (m_visible) {
			m_view->doExpand(this, m_index);
			m_view->refreshIndexes();
			m_view->onSelectionChanged();
			m_view->rowsChanged();
		}
	}
	m_expanded = true;
}

bool PropertyGridItem::hasParent(const PropertyGridItem* parent) const
{
	if (!m_parent) {
		return false;
	}
	if (m_parent == parent) {
		return true;
	}
	return m_parent->hasParent(parent);
}

bool PropertyGridItem::hasVisibleChildren() const
{
	for (PropertyGridItem* child : m_children) {
		if (!child->isHidden()) {
			return true;
		}
	}
	return false;
}

bool PropertyGridItem::isSelected() const
{
	return m_view && m_view->selectedItem() == this;
}

void PropertyGridItem::select()
{
	if (!m_view) {
		return;
	}

	if (m_visible) {
		m_view->setCurrentSelection(m_index);
		m_view->onSelectionChanged();
	}
}

void PropertyGridItem::onDeselect()
{
	if (m_view) {
		m_view->focusInplaceButton(nullptr);
	}
	onValidateEdit();
}

void PropertyGridItem::onValidateEdit()
{
	if (!m_view) {
		return;
	}

	PropertyGridInplaceEdit* edit = m_view->inplaceEdit();
	edit->hideWindow();

	if (edit->gridItem() == this) {
		QString value = edit->text();
		edit->clearItem();

		if (edit->isCancelled()) {
			return;
		}

		if (edit->isEditMode() && onAfterEdit(value)) {
			if (m_value != value && !edit->isCancelled()) {
				onValueChanged(value);
			}
		}
	}
}

void PropertyGridItem::setEditText(const QString& text)
{
	if (!m_view) {
		return;
	}

	PropertyGridInplaceEdit* edit = m_view->inplaceEdit();
	if (edit->gridItem() == this) {
		edit->setText(text);
		edit->setCursorPosition(0);
	}
}

void PropertyGridItem::invalidateView()
{
	if (m_view) {
		m_view->update();
	}
}

void PropertyGridItem::setValue(QString value)
{
	m_value = value;
	setEditText(m_value);
	invalidateView();
}

void PropertyGridItem::onValueChanged(QString value)
{
	if (!m_view) {
		return;
	}

	setValue(value);
	invalidateView();
}

bool PropertyGridItem::onAfterEdit(QString& value)
{
	return m_view != nullptr;
}

void PropertyGridItem::onSelect()
{
	if (!m_view) {
		return;
	}

	PropertyGridInplaceEdit* edit = m_view->inplaceEdit();
	if (m_flags & PROPERTY_GRID_ITEM_HAS_EDIT) {
		edit->setValue(m_value);
		edit->showForItem(this, valueRect());
		edit->setReadOnly(isReadOnly() || m_constraintEdit);
	}
	else {
		edit->hideWindow();
	}
}

void PropertyGridItem::onInplaceButtonDown(PropertyGridInplaceButton* button)
{
	if (isReadOnly()) {
		return;
	}

	if (button->id == PROPERTY_GRID_COMBO_BUTTON && !m_constraints.isEmpty()) {
		m_view->inplaceList()->showForItem(this, itemRect());
		m_view->update();
	}
}

QRect PropertyGridItem::itemRect() const
{
	return m_view->itemRect(m_index);
}

QRect PropertyGridItem::inplaceWindowsRect() const
{
	QRect rect = itemRect();
	int right = rightEdge(rect);
	if (isInplaceButtonsVisible()) {
		for (const std::unique_ptr<PropertyGridInplaceButton>& button : m_inplaceButtons) {
			right -= button->width;
		}
	}
	return rectFromEdges(m_view->dividerPos() + 1, rect.y() + 1, right, bottomEdge(rect) - 1);
}

QRect PropertyGridItem::valueRect() const
{
	return inplaceWindowsRect();
}

bool PropertyGridItem::pointInValueRect(QPoint point) const
{
	return valueRect().contains(point);
}

bool PropertyGridItem::onChar(unsigned character)
{
	if ((m_flags & PROPERTY_GRID_ITEM_HAS_EDIT) && character != charEscape) {
		onSelect();

		PropertyGridInplaceEdit* edit = m_view->inplaceEdit();
		if (edit->isVisible() && edit->gridItem() == this) {
			edit->setFocus();
			edit->selectAll();
			if (character != charTab && edit->gridItem() == this) {
				edit->handleChar(character);
			}
		}
		return true;
	}
	else if (character == charTab) {
		if (!m_inplaceButtons.empty()) {
			m_view->grid()->navigate(PROPERTY_GRID_UI_INPLACE_EDIT, true, this);
		}
	}
	return false;
}

bool PropertyGridItem::onKeyDown(int key)
{
	return false;
}

void PropertyGridItem::onLButtonDblClk(QPoint point)
{
	PropertyGridInplaceButton* button = inplaceButtonAt(point);
	if (button && isInplaceButtonsVisible()) {
		onSelect();
		return;
	}

	if (hasChildren()) {
		if (m_expanded) {
			collapse();
		}
		else {
			expand();
		}
	}
	else {
		onSelect();
		if (m_flags & PROPERTY_GRID_ITEM_HAS_EDIT) {
			PropertyGridInplaceEdit* edit = m_view->inplaceEdit();
			if (edit->isVisible() && edit->gridItem() == this) {
				edit->setFocus();
				if (edit->gridItem() == this && !isReadOnly() && edit->selectConstraint(1, true)) {
					onValidateEdit();
				}
				else {
					edit->selectAll();
				}
			}
		}
		else if (!isReadOnly()) {
			selectNextConstraint();
		}
	}
}

bool PropertyGridItem::selectNextConstraint()
{
	if (m_constraints.isEmpty()) {
		return false;
	}

	int index = m_constraints.find(m_value);
	index += 1;
	if (index >= m_constraints.count()) {
		index = 0;
	}
	if (index < 0) {
		index = m_constraints.count() - 1;
	}

	QString value = m_constraints.textAt(index);
	if (!onAfterEdit(value)) {
		return false;
	}

	m_constraints.setCurrent(index);
	onValueChanged(value);
	return true;
}

bool PropertyGridItem::onLButtonDown(QPoint point)
{
	bool oldSelected = isSelected();
	select();

	PropertyGridInplaceButton* button = inplaceButtonAt(point);
	if (button && isInplaceButtonsVisible()) {
		if (!oldSelected) {
			return true;
		}

		m_view->pressInplaceButton(button);
		return true;
	}

	int offset = (m_indent - (m_category ? 0 : 1)) * expandBorder;
	if (point.x() >= offset && point.x() <= expandBorder + offset) {
		if (m_expanded) {
			collapse();
		}
		else {
			expand();
		}
	}

	if ((m_flags & PROPERTY_GRID_ITEM_HAS_EDIT) && pointInValueRect(point)) {
		PropertyGridInplaceEdit* edit = m_view->inplaceEdit();
		if (edit->isVisible() && edit->gridItem() == this) {
			edit->setFocus(Qt::MouseFocusReason);
			edit->setCursorPosition(edit->cursorPositionAt(edit->mapFrom(m_view, point)));
		}
	}

	if (m_flags == PROPERTY_GRID_ITEM_HAS_COMBO_BUTTON && pointInValueRect(point) && !isReadOnly() &&
		m_inplaceButtons.size() == 1) {
		onInplaceButtonDown(m_inplaceButtons[0].get());
	}
	return true;
}

void PropertyGridItem::setHidden(bool hidden)
{
	if (hidden == m_hidden) {
		return;
	}

	m_hidden = hidden;
	if (m_view) {
		m_view->refresh();
	}
}

void PropertyGridItem::refresh(bool invalidate)
{
	if (m_view) {
		if (isSelected()) {
			onDeselect();
			onSelect();
		}
		if (invalidate) {
			m_view->update();
		}
	}
}

void PropertyGridItem::setReadOnly(bool readOnly)
{
	if (m_readOnly != readOnly) {
		m_readOnly = readOnly;
		refresh(true);
	}
}

QString PropertyGridItem::viewValue() const
{
	return m_value;
}

bool PropertyGridItem::onDrawItemValue(QPainter& painter, QRect valueRect)
{
	return false;
}

void PropertyGridItem::setFocusToInplaceControl()
{
	if (!m_view) {
		return;
	}

	PropertyGridInplaceEdit* edit = m_view->inplaceEdit();
	if (edit->gridItem() == this) {
		edit->setFocus();
		edit->selectAll();
	}
}

PropertyGridItemBool::PropertyGridItemBool(const QString& caption, bool value)
	: PropertyGridItem(caption), m_trueText("True"), m_falseText("False")
{
	m_flags = PROPERTY_GRID_ITEM_HAS_COMBO_BUTTON | PROPERTY_GRID_ITEM_HAS_EDIT;
	m_boolValue = value;
	PropertyGridItem::setValue(value ? m_trueText : m_falseText);
	m_constraints.add(m_trueText);
	m_constraints.add(m_falseText);
	setConstraintEdit(true);
	m_checkBoxStyle = false;
}

void PropertyGridItemBool::setValue(QString value)
{
	setBool(value.compare(m_trueText, Qt::CaseInsensitive) == 0);
}

void PropertyGridItemBool::setBool(bool value)
{
	m_boolValue = value;
	PropertyGridItem::setValue(value ? m_trueText : m_falseText);
}

void PropertyGridItemBool::setCheckBoxStyle(bool checkBoxStyle)
{
	m_checkBoxStyle = checkBoxStyle;
	setFlags(m_checkBoxStyle ? 0 : PROPERTY_GRID_ITEM_HAS_COMBO_BUTTON | PROPERTY_GRID_ITEM_HAS_EDIT);
}

QString PropertyGridItemBool::viewValue() const
{
	return m_checkBoxStyle ? QString() : PropertyGridItem::viewValue();
}

bool PropertyGridItemBool::onKeyDown(int key)
{
	if (m_checkBoxStyle && !isReadOnly() && key == Qt::Key_Space) {
		onLButtonDblClk(centerPoint(valueRect()));
		return true;
	}
	return false;
}

void PropertyGridItemBool::onLButtonDblClk(QPoint point)
{
	if (m_checkBoxStyle && pointInValueRect(point) && !isReadOnly()) {
		onSelect();
		selectNextConstraint();
	}
	else {
		PropertyGridItem::onLButtonDblClk(point);
	}
}

bool PropertyGridItemBool::pointInCheckBoxRect(QPoint point) const
{
	QRect rect = valueRect();
	rect.setWidth(15);
	return rect.contains(point);
}

bool PropertyGridItemBool::onLButtonDown(QPoint point)
{
	if (!PropertyGridItem::onLButtonDown(point)) {
		return false;
	}

	if (m_checkBoxStyle) {
		if (pointInCheckBoxRect(point) && !isReadOnly()) {
			onLButtonDblClk(point);
		}
	}
	return true;
}

bool PropertyGridItemBool::onDrawItemValue(QPainter& painter, QRect valueRect)
{
	if (!m_checkBoxStyle) {
		return false;
	}

	QPoint center = centerPoint(valueRect);
	QStyleOptionButton option;
	option.initFrom(m_view);
	option.styleObject = nullptr;
	option.rect = rectFromEdges(valueRect.x() - 1, center.y() - 6, valueRect.x() + 12, center.y() + 7);
	option.state = (isReadOnly() ? QStyle::State_None : QStyle::State_Enabled) |
				   (getBool() ? QStyle::State_On : QStyle::State_Off);
	m_view->style()->drawPrimitive(QStyle::PE_IndicatorCheckBox, &option, &painter, m_view);
	return true;
}

PropertyGridItemEnum::PropertyGridItemEnum(const QString& caption, int value) : PropertyGridItem(caption)
{
	m_enumValue = value;
	PropertyGridItem::setValue(m_constraints.textAt(m_constraints.findData(value)));
	m_flags = PROPERTY_GRID_ITEM_HAS_COMBO_BUTTON | PROPERTY_GRID_ITEM_HAS_EDIT;
	setConstraintEdit(true);
}

void PropertyGridItemEnum::setValue(QString value)
{
	int index = m_constraints.find(value);
	if (index >= 0) {
		setEnum(m_constraints.at(index));
	}
}

void PropertyGridItemEnum::setEnum(int value)
{
	m_enumValue = value;
	int index = m_constraints.findData(value);
	PropertyGridItem::setValue(m_constraints.textAt(index));
}

void PropertyGridItemEnum::setEnum(const PropertyGridConstraint* constraint)
{
	m_enumValue = static_cast<int>(constraint->data);
	PropertyGridItem::setValue(constraint->text);
}

void PropertyGridItemEnum::onConstraintsChanged()
{
	if (m_value.isEmpty()) {
		int index = m_constraints.findData(m_enumValue);
		if (index != -1) {
			m_value = m_constraints.textAt(index);
		}
	}
}

PropertyGridItemColor::PropertyGridItemColor(const QString& caption, QColor color) : PropertyGridItem(caption)
{
	m_color = color;
	PropertyGridItem::setValue(colorToString(color));
	m_flags = PROPERTY_GRID_ITEM_HAS_EDIT | PROPERTY_GRID_ITEM_HAS_EXPAND_BUTTON;
}

bool PropertyGridItemColor::onDrawItemValue(QPainter& painter, QRect valueRect)
{
	QColor textColor = painter.pen().color();
	QRect sample = rectFromEdges(valueRect.x() - 2, valueRect.y() + 1, valueRect.x() + 18, bottomEdge(valueRect) - 1);

	painter.save();
	painter.setPen(textColor);
	painter.setBrush(m_color);
	painter.drawRect(sample.adjusted(0, 0, -1, -1));
	painter.restore();

	QRect textRect = valueRect;
	textRect.setLeft(textRect.left() + 25);
	drawClippedText(painter, textRect, Qt::AlignLeft | Qt::AlignVCenter, m_value);
	return true;
}

QRect PropertyGridItemColor::valueRect() const
{
	QRect rect = PropertyGridItem::valueRect();
	rect.setLeft(rect.left() + 25);
	return rect;
}

QColor PropertyGridItemColor::stringToColor(const QString& text)
{
	auto component = [&text](int index) {
		QString part = text.section(';', index, index);
		return std::min(std::atoi(part.toLocal8Bit().constData()), 255) & 0xff;
	};
	return QColor(component(0), component(1), component(2));
}

QString PropertyGridItemColor::colorToString(QColor color)
{
	return QString::asprintf("%i; %i; %i", color.red(), color.green(), color.blue());
}

void PropertyGridItemColor::setValue(QString value)
{
	setColor(stringToColor(value));
}

void PropertyGridItemColor::setColor(QColor color)
{
	m_color = color;
	PropertyGridItem::setValue(colorToString(color));
}

void PropertyGridItemColor::onInplaceButtonDown(PropertyGridInplaceButton* button)
{
	PropertyGridView* view = m_view;
	if (std::optional<QColor> color = SystemColorDialog::getColor(m_color, view)) {
		QString value = colorToString(*color);
		if (onAfterEdit(value)) {
			onValueChanged(value);
			view->update();
		}
	}
}

static QString brickColorName(QColor color)
{
	RBX::BrickColor brickColor =
		RBX::BrickColor::closest(G3D::Color3(color.red() / 255.0, color.green() / 255.0, color.blue() / 255.0));
	return QString::fromLocal8Bit(brickColor.name().c_str());
}

static QColor brickColorFromName(const QString& text)
{
	G3D::Color3uint8 color = RBX::BrickColor::parse(text.toLocal8Bit().constData()).color3uint8();
	return QColor(color.r, color.g, color.b);
}

void CPropertyGridItemBrickColor::setColor(QColor color)
{
	m_color = color;
	PropertyGridItem::setValue(brickColorName(color));
}

CPropertyGridItemBrickColor::CPropertyGridItemBrickColor(const QString& caption, QColor color)
	: PropertyGridItemColor(caption, color)
{
	m_flags = PROPERTY_GRID_ITEM_HAS_COMBO_BUTTON;
	m_color = color;
	PropertyGridItem::setValue(brickColorName(color));
}

void CPropertyGridItemBrickColor::setValue(QString value)
{
	if (value != "") {
		setColor(brickColorFromName(value));
	}
	else {
		m_color = QColor(255, 255, 255);
		PropertyGridItem::setValue("");
	}
}

void CPropertyGridItemBrickColor::onInplaceButtonDown(PropertyGridInplaceButton* button)
{
	QRect rect = itemRect();
	QPoint anchor = m_view->mapToGlobal(QPoint(rightEdge(rect), bottomEdge(rect)));

	QMenu popup(m_view);
	ColorSelector* selector = new ColorSelector(&popup);
	QWidgetAction* action = new QWidgetAction(&popup);
	action->setDefaultWidget(selector);
	popup.addAction(action);

	if (value() != "") {
		QColor current = color();
		selector->setColor(
			RBX::BrickColor::closest(
				G3D::Color3(current.red() / 255.0, current.green() / 255.0, current.blue() / 255.0)
			)
		);
	}

	bool selected = false;
	RBX::BrickColor selectedColor;
	selector->onColorSelected([&selected, &selectedColor, &popup](RBX::BrickColor color) {
		selected = true;
		selectedColor = color;
		popup.close();
	});

	popup.exec(QPoint(anchor.x() - popup.sizeHint().width(), anchor.y()));

	if (selected) {
		onValueChanged(QString::fromLocal8Bit(selectedColor.name().c_str()));
	}
}

PropertyGridInplaceEdit::PropertyGridInplaceEdit(PropertyGridView* view) : QLineEdit(view), m_view(view)
{
	setFrame(false);
	setAlignment(Qt::AlignLeft | Qt::AlignTop);
	setTextMargins(1, -lineEditVerticalMargin, 1, 0);
	setFocusPolicy(Qt::ClickFocus);
	hide();
}

void PropertyGridInplaceEdit::showForItem(PropertyGridItem* item, QRect rect)
{
	m_item = item;
	m_editMode = false;
	m_cancel = false;

	setFont(m_view->itemFont(item));
	QPalette colors = palette();
	colors.setColor(QPalette::Base, m_view->grid()->backColor());
	colors.setColor(QPalette::Text, m_view->itemTextColor(item));
	setPalette(colors);

	setText(m_value);
	setCursorPosition(0);
	setGeometry(rect);
	show();
}

void PropertyGridInplaceEdit::hideWindow()
{
	if (!isHidden()) {
		if (hasFocus()) {
			m_view->setFocus();
		}
		hide();
	}
}

void PropertyGridInplaceEdit::destroyItem()
{
	m_item = nullptr;
	m_value.clear();
	hide();
}

bool PropertyGridInplaceEdit::focusNextPrevChild(bool next)
{
	return false;
}

void PropertyGridInplaceEdit::focusInEvent(QFocusEvent* event)
{
	QLineEdit::focusInEvent(event);
	m_view->update();
	m_editMode = true;
}

void PropertyGridInplaceEdit::focusOutEvent(QFocusEvent* event)
{
	QLineEdit::focusOutEvent(event);
	if (event->reason() == Qt::PopupFocusReason) {
		return;
	}

	if (m_item) {
		m_item->onValidateEdit();
		m_view->update();
	}
}

void PropertyGridInplaceEdit::handleChar(unsigned character)
{
	if (character == charTab) {
		return;
	}

	if (character == charEscape || character == charReturn) {
		m_view->setFocus();
		return;
	}

	if (m_item && !m_item->isReadOnly() && m_item->constraintEdit()) {
		PropertyGridConstraints* list = m_item->constraints();
		if (!list->isEmpty()) {
			int index = list->find(text());
			int start = index == -1 ? list->count() - 1 : index;
			QString search = QString(QChar(static_cast<char16_t>(character)));

			do {
				index = index < list->count() - 1 ? index + 1 : 0;
				QString candidate = list->textAt(index);
				if (search.compare(candidate.left(1), Qt::CaseInsensitive) == 0) {
					setText(candidate);
					list->setCurrent(index);
					selectAll();
					return;
				}
			} while (index != start);
			return;
		}
	}

	if (isReadOnly()) {
		return;
	}

	if (character == charBackspace) {
		backspace();
	}
	else if (character >= 32) {
		insert(QString(QChar(static_cast<char16_t>(character))));
	}
}

void PropertyGridInplaceEdit::keyPressEvent(QKeyEvent* event)
{
	event->accept();
	if (!m_item) {
		return;
	}

	int key = event->key();
	bool alt = event->modifiers() & Qt::AltModifier;

	if (alt && (key == Qt::Key_Down || key == Qt::Key_Up)) {
		if (PropertyGridInplaceButton* button = m_item->findInplaceButton(PROPERTY_GRID_COMBO_BUTTON)) {
			m_item->onInplaceButtonDown(button);
		}
		return;
	}

	if (key == Qt::Key_Tab || key == Qt::Key_Backtab) {
		bool forward = key == Qt::Key_Tab && !(event->modifiers() & Qt::ShiftModifier);
		m_view->grid()->navigate(PROPERTY_GRID_UI_INPLACE_EDIT, forward, m_item);
		return;
	}
	else if (key == Qt::Key_Escape) {
		m_cancel = true;
		setText(m_value);
		m_view->setFocus();
		return;
	}
	else if (key == Qt::Key_Return || key == Qt::Key_Enter) {
		m_view->setFocus();
		return;
	}
	else if (key == Qt::Key_F4) {
		if (PropertyGridInplaceButton* button = m_item->findInplaceButton(PROPERTY_GRID_COMBO_BUTTON)) {
			m_item->onInplaceButtonDown(button);
		}
		return;
	}
	else if (key == Qt::Key_Down || key == Qt::Key_Up) {
		if (!m_item->isReadOnly()) {
			if (selectConstraint(key == Qt::Key_Down ? 1 : -1, false)) {
				selectAll();
				return;
			}
		}
	}

	QString typed = event->text();
	bool printable = !typed.isEmpty() && typed.at(0).unicode() >= 32 && typed.at(0).unicode() != 127 &&
					 !(event->modifiers() & Qt::ControlModifier);
	if (printable && !m_item->isReadOnly() && m_item->constraintEdit() && !m_item->constraints()->isEmpty()) {
		handleChar(typed.at(0).unicode());
		return;
	}

	QLineEdit::keyPressEvent(event);
	event->accept();
}

bool PropertyGridInplaceEdit::selectConstraint(int direction, bool circle)
{
	PropertyGridConstraints* list = m_item->constraints();
	if (list->isEmpty()) {
		return false;
	}

	int index = list->find(text());
	if (index == -1) {
		return false;
	}

	index += direction;
	if (index >= list->count()) {
		index = circle ? 0 : list->count() - 1;
	}
	if (index < 0) {
		index = circle ? list->count() - 1 : 0;
	}

	setText(list->textAt(index));
	list->setCurrent(index);
	return true;
}

void PropertyGridInplaceEdit::mouseDoubleClickEvent(QMouseEvent* event)
{
	if (m_item && !m_item->isReadOnly() && selectConstraint(1, true)) {
		m_view->setFocus();
		return;
	}

	QLineEdit::mouseDoubleClickEvent(event);
}

namespace {

class InplaceListDelegate : public QStyledItemDelegate
{
public:
	using QStyledItemDelegate::QStyledItemDelegate;

	void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
	{
		bool selected = option.state & QStyle::State_Selected;
		const QPalette& palette = option.palette;
		painter->fillRect(option.rect, selected ? palette.color(QPalette::Highlight) : palette.color(QPalette::Base));

		QRect text = option.rect.adjusted(inplaceListTextInset, 0, -inplaceListTextInset, 0);
		QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();
		if (!icon.isNull()) {
			QSize size = icon.actualSize(QSize(16, 16));
			int height = std::min(size.height(), option.rect.height());
			icon.paint(painter, QRect(text.left(), centerPoint(text).y() - height / 2, size.width(), height));
			text.setLeft(text.left() + size.width() + inplaceListImageGap);
		}

		painter->setFont(option.font);
		painter->setPen(selected ? palette.color(QPalette::HighlightedText) : palette.color(QPalette::Text));
		WhidbeyStyle::drawText(
			painter, text, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine, index.data().toString()
		);
	}
};

} // namespace

PropertyGridInplaceList::PropertyGridInplaceList(PropertyGridView* view) : QListWidget(view), m_view(view)
{
	setWindowFlags(Qt::Popup | Qt::NoDropShadowWindowHint);
	setItemDelegate(new InplaceListDelegate(this));
	setFrameStyle(QFrame::Box | QFrame::Plain);
	setLineWidth(1);
	setAttribute(Qt::WA_Hover);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setUniformItemSizes(true);
}

void PropertyGridInplaceList::showForItem(PropertyGridItem* item, QRect rect)
{
	if (m_view->inplaceEdit()->hasFocus()) {
		m_view->setFocus();
	}

	m_item = nullptr;
	hide();
	clear();
	m_item = item;

	setFont(m_view->font());
	QFontMetrics metrics(font());
	int thumb = style()->pixelMetric(QStyle::PM_ScrollBarExtent);
	int width = rightEdge(rect) - (m_view->dividerPos() + 1);
	int height = metrics.height() + 3;

	PropertyGridConstraints* list = item->constraints();
	for (int i = 0; i < list->count(); i++) {
		PropertyGridConstraint* constraint = list->at(i);
		QListWidgetItem* row = new QListWidgetItem(constraint->text, this);
		if (constraint->image != -1) {
			QIcon icon = ImageManager::instance().icon(constraint->image);
			if (!icon.isNull()) {
				row->setIcon(icon);
			}
		}

		width = std::max(width, metrics.horizontalAdvance(constraint->text) + thumb * 2);
		height = std::max(height, metrics.height() + 3);
		if (item->value() == constraint->text) {
			setCurrentRow(i);
		}
	}

	for (int i = 0; i < count(); i++) {
		QListWidget::item(i)->setSizeHint(QSize(1, height));
	}

	int top = bottomEdge(rect);
	int bottom = top + height * std::min(dropDownItemCount, count()) + 2;
	int left = rightEdge(rect) - std::min(width, rect.width() - expandBorder);
	QRect popupRect = rectFromEdges(left, top, rightEdge(rect), bottom);
	popupRect.moveTopLeft(m_view->mapToGlobal(popupRect.topLeft()));

	QRect work = m_view->screen()->availableGeometry();
	if (bottomEdge(popupRect) > bottomEdge(work) && popupRect.y() > centerPoint(work).y()) {
		popupRect.translate(0, -popupRect.height() - rect.height() - 1);
	}
	if (popupRect.x() < work.x()) {
		popupRect.translate(work.x() - popupRect.x(), 0);
	}
	if (rightEdge(popupRect) > rightEdge(work)) {
		popupRect.translate(rightEdge(work) - rightEdge(popupRect), 0);
	}

	setGeometry(popupRect);
	show();
	setFocus();
}

bool PropertyGridInplaceList::event(QEvent* event)
{
	switch (event->type()) {
	case QEvent::Paint: {
		QPainter painter(this);
		int part = verticalScrollBar()->isVisible() ? listBoxBorderVScrollPart : listBoxBorderNoScrollPart;
		int state = !isEnabled()   ? listBoxBorderDisabled
					: underMouse() ? listBoxBorderHot
					: hasFocus()   ? listBoxBorderFocused
								   : listBoxBorderNormal;
		if (!WhidbeyStyle::drawThemeBackground(
				&painter, rect(), L"LISTBOX", part, state, palette().color(QPalette::Base)
			)) {
			painter.setPen(palette().color(QPalette::WindowText));
			painter.drawRect(rect().adjusted(0, 0, -1, -1));
		}
		return true;
	}
	case QEvent::MouseButtonPress:
	case QEvent::MouseButtonDblClick:
		if (!rect().contains(static_cast<QMouseEvent*>(event)->position().toPoint())) {
			close();
			return true;
		}
		break;
	case QEvent::HoverEnter:
	case QEvent::HoverLeave:
	case QEvent::FocusIn:
	case QEvent::FocusOut:
		update();
		break;
	default:
		break;
	}
	return QListWidget::event(event);
}

void PropertyGridInplaceList::destroyItem()
{
	m_item = nullptr;
	hide();
}

void PropertyGridInplaceList::hideEvent(QHideEvent* event)
{
	QListWidget::hideEvent(event);
	m_item = nullptr;
	m_view->update();
}

void PropertyGridInplaceList::mouseReleaseEvent(QMouseEvent* event)
{
	QListWidget::mouseReleaseEvent(event);
	if (viewport()->rect().contains(event->position().toPoint())) {
		apply();
	}
	else {
		cancel();
	}
}

void PropertyGridInplaceList::cancel()
{
	m_item = nullptr;
	hide();
	m_view->setFocus();
}

void PropertyGridInplaceList::apply()
{
	int row = currentRow();
	PropertyGridItem* item = m_item;
	QString text = row >= 0 ? QListWidget::item(row)->text() : QString();

	m_item = nullptr;
	hide();
	m_view->setFocus();

	if (row >= 0 && item) {
		item->constraints()->setCurrent(row);
		if (item->onAfterEdit(text) && text != item->value()) {
			item->onValueChanged(text);
		}
	}
}

void PropertyGridInplaceList::keyPressEvent(QKeyEvent* event)
{
	int key = event->key();
	if (event->modifiers() & Qt::AltModifier) {
		if (key == Qt::Key_Down || key == Qt::Key_Up) {
			apply();
			return;
		}
	}

	if (key == Qt::Key_Escape) {
		cancel();
	}
	else if (key == Qt::Key_Return || key == Qt::Key_Enter || key == Qt::Key_F4) {
		apply();
	}
	else {
		QListWidget::keyPressEvent(event);
	}
}

PropertyGridToolTip::PropertyGridToolTip(PropertyGridView* view)
	: QWidget(view, Qt::ToolTip | Qt::FramelessWindowHint), m_view(view)
{
	setAttribute(Qt::WA_TransparentForMouseEvents);
	setAttribute(Qt::WA_ShowWithoutActivating);
}

void PropertyGridToolTip::activate(bool active, PropertyGridItem* item, bool valuePart)
{
	if (active) {
		setFont(m_view->itemFont(item));

		QRect rect = geometry();
		rect.setWidth(QFontMetrics(font()).horizontalAdvance(m_text) + 8);

		QRect work = m_view->screen()->availableGeometry();
		if (rightEdge(rect) > rightEdge(work)) {
			rect.translate(rightEdge(work) - rightEdge(rect), 0);
		}
		if (rect.x() < work.x()) {
			rect.translate(work.x() - rect.x(), 0);
		}

		setGeometry(rect);
		show();
		update();
	}
	else {
		hide();
	}
}

void PropertyGridToolTip::paintEvent(QPaintEvent* event)
{
	QPainter painter(this);
	QColor textColor = palette().color(QPalette::ToolTipText);
	painter.fillRect(rect(), palette().color(QPalette::ToolTipBase));
	draw3dRect(painter, rect(), textColor, textColor);

	painter.setPen(textColor);
	QRect textRect = rect();
	textRect.setLeft(textRect.left() + 4);
	WhidbeyStyle::drawText(&painter, textRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine, m_text);
}

PropertyGridView::PropertyGridView(PropertyGrid* grid) : QWidget(grid), m_grid(grid)
{
	setFocusPolicy(Qt::ClickFocus);
	setMouseTracking(true);
	setFont(grid->normalFont());
	m_itemHeight = QFontMetrics(grid->normalFont()).height() + 4;
	m_inplaceEdit = new PropertyGridInplaceEdit(this);
	m_inplaceList = new PropertyGridInplaceList(this);
}

PropertyGridView::~PropertyGridView()
{
	m_rows.clear();
	m_selected = nullptr;
	clearCategories();
}

PropertyGridItem* PropertyGridView::insertCategory(int index, const QString& caption)
{
	if (index < 0 || index > static_cast<int>(m_categories.size())) {
		index = static_cast<int>(m_categories.size());
	}

	PropertyGridItem* category = new PropertyGridItem(caption);
	setPropertySort(PROPERTY_GRID_SORT_CATEGORIZED);

	category->m_view = this;
	category->m_category = true;
	category->m_flags = 0;
	m_categories.insert(m_categories.begin() + index, category);

	int insertAt = index >= static_cast<int>(m_categories.size()) - 1 ? count() : m_categories[index + 1]->m_index;
	insertItem(category, insertAt);
	refreshIndexes();
	if (m_lockUpdate == 0) {
		rowsChanged();
	}
	return category;
}

PropertyGridItem* PropertyGridView::addCategory(const QString& caption)
{
	return insertCategory(static_cast<int>(m_categories.size()), caption);
}

void PropertyGridView::clearCategories()
{
	std::vector<PropertyGridItem*> categories;
	categories.swap(m_categories);
	for (PropertyGridItem* category : categories) {
		delete category;
	}
}

void PropertyGridView::refresh()
{
	refreshIndexes();
	setPropertySort(m_sort, true);
	m_grid->update();
}

void PropertyGridView::resetContent()
{
	if (m_selected) {
		m_selected->onDeselect();
		m_selected = nullptr;
	}

	for (PropertyGridItem* row : m_rows) {
		row->m_visible = false;
	}

	m_rows.clear();
	m_currentRow = -1;
	m_topIndex = 0;
	m_itemHeight = QFontMetrics(m_grid->normalFont()).height() + 4;
}

void PropertyGridView::setPropertySort(PropertyGridSortOrder sort, bool refresh)
{
	if (m_lockUpdate > 0) {
		m_sort = sort;
		return;
	}

	if (sort == m_sort && !refresh) {
		return;
	}

	PropertyGridItem* topItem = refresh ? item(m_topIndex) : nullptr;
	PropertyGridItem* selected = selectedItem();

	resetContent();

	if (sort == PROPERTY_GRID_SORT_CATEGORIZED) {
		for (PropertyGridItem* category : m_categories) {
			insertItem(category, count());
		}
	}
	else if (sort == PROPERTY_GRID_SORT_ALPHABETICAL || sort == PROPERTY_GRID_SORT_NONE) {
		std::vector<PropertyGridItem*> items;
		for (PropertyGridItem* category : m_categories) {
			if (!category->isHidden()) {
				items.insert(items.end(), category->m_children.begin(), category->m_children.end());
			}
		}

		if (sort != PROPERTY_GRID_SORT_NONE) {
			std::stable_sort(items.begin(), items.end(), [](const PropertyGridItem* a, const PropertyGridItem* b) {
				return a->caption().compare(b->caption()) < 0;
			});
		}

		for (PropertyGridItem* sorted : items) {
			insertItem(sorted, count());
		}
	}

	refreshIndexes();

	if (topItem && topItem->isVisible()) {
		setTopIndex(topItem->m_index);
	}

	if (selected) {
		selected->select();
	}

	onSelectionChanged();

	if (sort != m_sort) {
		m_sort = sort;
		m_grid->onSortChanged();
	}

	rowsChanged();
}

void PropertyGridView::setDividerPos(int divider)
{
	double width = m_grid->viewWindowWidth();
	if (!m_autoDivider) {
		m_divider = divider;
		m_divider = std::max(m_divider, 1.0);
		if (width != 0) {
			m_divider = std::min(m_divider, width);
		}
	}
	else {
		m_divider = width == 0 ? .5 : static_cast<double>(divider) / width;
		m_divider = std::max(m_divider, .1);
		m_divider = std::min(m_divider, .85);
	}
}

int PropertyGridView::dividerPos() const
{
	return static_cast<int>(m_divider * (m_autoDivider ? m_grid->viewWindowWidth() : 1));
}

int PropertyGridView::insertItem(PropertyGridItem* item, int index)
{
	if (m_lockUpdate > 0) {
		return 0;
	}

	if (item->isHidden()) {
		return 0;
	}

	index = std::clamp(index, 0, count());
	m_rows.insert(m_rows.begin() + index, item);
	if (m_currentRow >= index) {
		m_currentRow++;
	}

	item->m_visible = true;

	int inserted = 1;
	if (item->m_expanded) {
		inserted += doExpand(item, index);
	}
	return inserted;
}

int PropertyGridView::doExpand(PropertyGridItem* item, int index)
{
	int start = index;
	for (PropertyGridItem* child : item->m_children) {
		index += insertItem(child, index + 1);
	}
	return index - start;
}

void PropertyGridView::refreshIndexes()
{
	for (int i = 0; i < count(); i++) {
		m_rows[i]->m_index = i;
	}
}

void PropertyGridView::doCollapse(PropertyGridItem* item)
{
	int index = item->m_index + 1;
	while (index < count()) {
		PropertyGridItem* child = m_rows[index];
		if (!child->hasParent(item)) {
			break;
		}

		child->m_visible = false;
		m_rows.erase(m_rows.begin() + index);
		if (m_currentRow == index) {
			m_currentRow = -1;
		}
		else if (m_currentRow > index) {
			m_currentRow--;
		}
	}

	refreshIndexes();
}

PropertyGridItem* PropertyGridView::item(int index) const
{
	if (index < 0 || index >= count()) {
		return nullptr;
	}
	return m_rows[index];
}

void PropertyGridView::switchExpandState(int index)
{
	PropertyGridItem* row = item(index);
	if (!row) {
		return;
	}

	if (row->m_expanded) {
		row->collapse();
	}
	else {
		row->expand();
	}
}

QRect PropertyGridView::itemRect(int index) const
{
	return QRect(0, (index - m_topIndex) * m_itemHeight, width(), m_itemHeight);
}

PropertyGridItem* PropertyGridView::itemFromPoint(QPoint point) const
{
	if (m_itemHeight <= 0 || !rect().contains(point)) {
		return nullptr;
	}
	return item(m_topIndex + point.y() / m_itemHeight);
}

int PropertyGridView::pageSize() const
{
	return m_itemHeight > 0 ? std::max(1, height() / m_itemHeight) : 1;
}

void PropertyGridView::setTopIndex(int index)
{
	index = std::clamp(index, 0, std::max(0, count() - pageSize()));
	if (index != m_topIndex) {
		m_topIndex = index;
		scrollContents();
	}
}

void PropertyGridView::scrollContents()
{
	PropertyGridItem* edited = m_inplaceEdit->gridItem();
	if (edited && !m_inplaceEdit->isHidden()) {
		m_inplaceEdit->setGeometry(edited->valueRect());
	}

	m_grid->reposition();
	update();
}

void PropertyGridView::rowsChanged()
{
	int top = std::clamp(m_topIndex, 0, std::max(0, count() - pageSize()));
	if (top != m_topIndex) {
		m_topIndex = top;
		PropertyGridItem* edited = m_inplaceEdit->gridItem();
		if (edited && !m_inplaceEdit->isHidden()) {
			m_inplaceEdit->setGeometry(edited->valueRect());
		}
	}

	m_grid->reposition();
	update();
}

void PropertyGridView::setCurrentSelection(int index)
{
	m_currentRow = index >= 0 && index < count() ? index : -1;
	if (m_currentRow != -1) {
		int page = pageSize();
		if (m_currentRow < m_topIndex) {
			setTopIndex(m_currentRow);
		}
		else if (m_currentRow >= m_topIndex + page) {
			setTopIndex(m_currentRow - page + 1);
		}
	}
	update();
}

void PropertyGridView::onSelectionChanged()
{
	PropertyGridItem* current = selectedItem();
	if (m_selected) {
		m_selected->onDeselect();
	}
	if (current) {
		current->onSelect();
	}
	m_selected = current;
}

void PropertyGridView::focusInplaceButton(PropertyGridInplaceButton* button)
{
	if (button != m_focusedButton) {
		if (button) {
			setFocus();
		}
		m_focusedButton = button;
		update();
	}
}

void PropertyGridView::pressInplaceButton(PropertyGridInplaceButton* button)
{
	m_pressedButton = button;
	m_hotButton = button;
	setFocus();
	update();
}

void PropertyGridView::inplaceButtonRemoved(PropertyGridInplaceButton* button)
{
	if (m_focusedButton == button) {
		m_focusedButton = nullptr;
	}
	if (m_hotButton == button) {
		m_hotButton = nullptr;
	}
	if (m_pressedButton == button) {
		m_pressedButton = nullptr;
	}
}

void PropertyGridView::itemDestroyed(PropertyGridItem* item)
{
	if (m_selected == item) {
		m_selected = nullptr;
	}

	for (const std::unique_ptr<PropertyGridInplaceButton>& button : item->inplaceButtons()) {
		inplaceButtonRemoved(button.get());
	}

	if (m_inplaceList->gridItem() == item) {
		m_inplaceList->destroyItem();
	}
	if (m_inplaceEdit->gridItem() == item) {
		m_inplaceEdit->destroyItem();
	}
}

bool PropertyGridView::isViewFocused() const
{
	QWidget* focus = QApplication::focusWidget();
	return focus && (focus == this || focus->parentWidget() == this);
}

QFont PropertyGridView::itemFont(const PropertyGridItem* item) const
{
	return item && item->isCategory() ? m_grid->boldFont() : m_grid->normalFont();
}

QColor PropertyGridView::itemTextColor(const PropertyGridItem* item) const
{
	if (!item) {
		return m_grid->foreColor();
	}
	return item->isCategory()   ? m_grid->categoryForeColor()
		   : item->isReadOnly() ? m_grid->readOnlyForeColor()
								: m_grid->foreColor();
}

bool PropertyGridView::hitTestSplitter(QPoint point) const
{
	int divider = dividerPos();
	return point.x() > divider - 4 && point.x() <= divider + 2 && count() > 0;
}

bool PropertyGridView::event(QEvent* event)
{
	if (m_toolTip && m_toolTip->isVisible()) {
		relayToolTipEvent(event->type());
	}
	return QWidget::event(event);
}

bool PropertyGridView::focusNextPrevChild(bool next)
{
	return false;
}

void PropertyGridView::resizeEvent(QResizeEvent* event)
{
	QWidget::resizeEvent(event);
	m_topIndex = std::clamp(m_topIndex, 0, std::max(0, count() - pageSize()));
	onSelectionChanged();
	update();
}

void PropertyGridView::focusInEvent(QFocusEvent* event)
{
	m_focusedButton = nullptr;
	QWidget::focusInEvent(event);
	update();
}

void PropertyGridView::focusOutEvent(QFocusEvent* event)
{
	m_focusedButton = nullptr;
	QWidget::focusOutEvent(event);
	update();
}

void PropertyGridView::leaveEvent(QEvent* event)
{
	if (m_hotButton && !m_pressedButton) {
		m_hotButton = nullptr;
		update();
	}
	QWidget::leaveEvent(event);
}

void PropertyGridView::mousePressEvent(QMouseEvent* event)
{
	if (event->button() != Qt::LeftButton) {
		return;
	}

	QPoint point = event->position().toPoint();
	if (hitTestSplitter(point)) {
		setFocus();
		if (m_selected) {
			m_selected->onValidateEdit();
		}
		m_tracking = true;
		return;
	}

	PropertyGridItem* clicked = itemFromPoint(point);
	if (clicked) {
		setFocus();
		if (itemFromPoint(point) == clicked) {
			clicked->onLButtonDown(point);
		}
	}
}

void PropertyGridView::mouseMoveEvent(QMouseEvent* event)
{
	QPoint point = event->position().toPoint();
	if (m_tracking) {
		setDividerPos(point.x());
		if (m_selected) {
			m_selected->onSelect();
		}
		update();
		return;
	}

	if (m_pressedButton) {
		PropertyGridInplaceButton* hot =
			m_pressedButton->item->inplaceButtonRect(m_pressedButton).contains(point) ? m_pressedButton : nullptr;
		if (m_hotButton != hot) {
			m_hotButton = hot;
			update();
		}
		return;
	}

	PropertyGridItem* hovered = itemFromPoint(point);
	PropertyGridInplaceButton* button = hovered ? hovered->inplaceButtonAt(point) : nullptr;
	if (button != m_hotButton) {
		m_hotButton = button;
		update();
	}

	setCursor(hitTestSplitter(point) ? Qt::SplitHCursor : Qt::ArrowCursor);
	showToolTip(point);
}

void PropertyGridView::mouseReleaseEvent(QMouseEvent* event)
{
	if (event->button() != Qt::LeftButton) {
		return;
	}

	m_tracking = false;

	if (m_pressedButton) {
		PropertyGridInplaceButton* button = m_pressedButton;
		bool click = m_hotButton == button;
		m_pressedButton = nullptr;
		update();
		if (click) {
			button->item->onInplaceButtonDown(button);
		}
	}
}

void PropertyGridView::mouseDoubleClickEvent(QMouseEvent* event)
{
	if (event->button() != Qt::LeftButton) {
		return;
	}

	QPoint point = event->position().toPoint();
	PropertyGridItem* clicked = itemFromPoint(point);
	if (clicked) {
		clicked->onLButtonDblClk(point);
	}
}

void PropertyGridView::wheelEvent(QWheelEvent* event)
{
	if (count() == 0) {
		return;
	}

	int lines = event->angleDelta().y() / 120 * QApplication::wheelScrollLines();
	setTopIndex(m_topIndex - lines);
	event->accept();
}

void PropertyGridView::keyPressEvent(QKeyEvent* event)
{
	event->accept();
	if (event->modifiers() & Qt::AltModifier) {
		onSysKeyDown(event->key());
		return;
	}

	onKeyDown(event->key(), event->modifiers());

	unsigned character = characterFromKeyEvent(event);
	if (character != 0) {
		bool forward = event->key() != Qt::Key_Backtab && !(event->modifiers() & Qt::ShiftModifier);
		onChar(character, forward);
	}
}

void PropertyGridView::onSysKeyDown(int key)
{
	if ((key == Qt::Key_Down || key == Qt::Key_Up) && m_selected) {
		if (PropertyGridInplaceButton* button = m_selected->findInplaceButton(PROPERTY_GRID_COMBO_BUTTON)) {
			m_selected->onInplaceButtonDown(button);
		}
	}
}

void PropertyGridView::onKeyDown(int key, Qt::KeyboardModifiers modifiers)
{
	PropertyGridItem* current = selectedItem();

	if (m_focusedButton && m_focusedButton->item == current) {
		if (key == Qt::Key_Space || key == Qt::Key_Down || key == Qt::Key_Return || key == Qt::Key_Enter ||
			key == Qt::Key_F4) {
			m_focusedButton->item->onInplaceButtonDown(m_focusedButton);
		}
		return;
	}

	if ((modifiers & Qt::ControlModifier) && (key == Qt::Key_Right || key == Qt::Key_Left)) {
		setDividerPos(dividerPos() + (key == Qt::Key_Right ? 3 : -3));
		if (m_selected) {
			m_selected->onSelect();
		}
		update();
		return;
	}

	if (key == Qt::Key_Right) {
		if (current && current->hasChildren() && !current->m_expanded) {
			current->expand();
			return;
		}
	}
	else if (key == Qt::Key_Left) {
		if (current && current->hasChildren() && current->m_expanded) {
			current->collapse();
			return;
		}
	}

	if (key == Qt::Key_Return || key == Qt::Key_Enter) {
		switchExpandState(m_currentRow);
		return;
	}

	if (key == Qt::Key_F4 && m_selected) {
		if (PropertyGridInplaceButton* button = m_selected->findInplaceButton(PROPERTY_GRID_COMBO_BUTTON)) {
			m_selected->onInplaceButtonDown(button);
		}
	}

	if (m_selected && m_selected->onKeyDown(key)) {
		return;
	}

	listKeyDown(key);
}

void PropertyGridView::listKeyDown(int key)
{
	if (count() == 0) {
		return;
	}

	int page = pageSize();
	int target = m_currentRow;
	switch (key) {
	case Qt::Key_Up:
	case Qt::Key_Left:
		target = m_currentRow <= 0 ? 0 : m_currentRow - 1;
		break;
	case Qt::Key_Down:
	case Qt::Key_Right:
		target = m_currentRow < 0 ? 0 : std::min(m_currentRow + 1, count() - 1);
		break;
	case Qt::Key_Home:
		target = 0;
		break;
	case Qt::Key_End:
		target = count() - 1;
		break;
	case Qt::Key_PageUp:
		target = m_currentRow > m_topIndex ? m_topIndex : std::max(0, m_currentRow - (page - 1));
		break;
	case Qt::Key_PageDown:
		target = m_currentRow < m_topIndex + page - 1 ? std::min(m_topIndex + page - 1, count() - 1)
													  : std::min(m_currentRow + (page - 1), count() - 1);
		break;
	default:
		return;
	}

	if (target != m_currentRow) {
		setCurrentSelection(target);
		onSelectionChanged();
	}
}

void PropertyGridView::onChar(unsigned character, bool forward)
{
	if (m_tracking) {
		return;
	}

	if (m_focusedButton && character != charTab) {
		return;
	}

	if (!hasFocus()) {
		return;
	}

	PropertyGridItem* current = selectedItem();

	if (character == '+' && current && current->hasChildren() && !current->m_expanded) {
		current->expand();
		return;
	}

	if (character == '-' && current && current->hasChildren() && current->m_expanded) {
		current->collapse();
		return;
	}

	if (character == charTab) {
		if (m_focusedButton) {
			m_grid->navigate(forward ? PROPERTY_GRID_UI_INPLACE_EDIT : PROPERTY_GRID_UI_VIEW_NEXT, forward, current);
		}
		else {
			m_grid->navigate(PROPERTY_GRID_UI_VIEW, forward, current);
		}
		return;
	}

	if (current && (character != charReturn || !current->hasChildren())) {
		current->onChar(character);
	}
}

void PropertyGridView::paintEvent(QPaintEvent* event)
{
	QPainter painter(this);
	painter.fillRect(rect(), m_grid->backColor());

	for (int row = m_topIndex; row < count(); row++) {
		if (itemRect(row).y() >= height()) {
			break;
		}
		drawItem(painter, row);
	}
}

void PropertyGridView::drawItemValue(QPainter& painter, PropertyGridItem* item, QRect valueRect)
{
	painter.setPen(itemTextColor(item));
	painter.fillRect(valueRect, m_grid->backColor());
	valueRect.setLeft(valueRect.left() + 4);

	if (item->onDrawItemValue(painter, valueRect)) {
		return;
	}

	valueRect.adjust(0, 1, 0, -1);
	drawClippedText(painter, valueRect, Qt::AlignLeft | Qt::AlignVCenter, item->viewValue());
}

void PropertyGridView::drawItemCaption(QPainter& painter, PropertyGridItem* item, QRect captionRect)
{
	QRect textRect = rectFromEdges(
		(item->indent() + (item->isCategory() ? 1 : 0)) * expandBorder + 3,
		captionRect.y(),
		rightEdge(captionRect) - 1,
		bottomEdge(captionRect) - 1
	);
	drawClippedText(painter, textRect, Qt::AlignLeft | Qt::AlignVCenter, item->caption());
}

void PropertyGridView::drawInplaceButtons(QPainter& painter, PropertyGridItem* item)
{
	if (!item->isInplaceButtonsVisible()) {
		return;
	}

	for (const std::unique_ptr<PropertyGridInplaceButton>& button : item->inplaceButtons()) {
		drawInplaceButton(painter, button.get(), item->inplaceButtonRect(button.get()));
	}
}

void PropertyGridView::drawInplaceButton(QPainter& painter, PropertyGridInplaceButton* button, QRect rect)
{
	QPoint center = centerPoint(rect);
	QPoint dots(center.x(), center.y() + 3);
	QColor textColor = palette().color(QPalette::ButtonText);
	bool hot = m_hotButton == button;
	bool pressed = m_pressedButton == button && hot;
	bool focused = m_focusedButton == button;

#ifdef _WIN32
	painter.fillRect(rect, m_grid->backColor());

	QStyleOptionButton option;
	option.initFrom(this);
	option.styleObject = nullptr;
	option.rect = rect;
	option.state = QStyle::State_Enabled | (pressed ? QStyle::State_Sunken : QStyle::State_Raised);
	if (hot) {
		option.state |= QStyle::State_MouseOver;
	}
	if (focused && button->id != PROPERTY_GRID_COMBO_BUTTON) {
		option.features |= QStyleOptionButton::DefaultButton;
	}
	bool themed = button->id == PROPERTY_GRID_COMBO_BUTTON &&
				  WhidbeyStyle::drawThemeBackground(
					  &painter,
					  rect,
					  L"COMBOBOX",
					  comboDropDownButtonPart,
					  pressed ? comboButtonPressed : (hot ? comboButtonHot : comboButtonNormal),
					  m_grid->backColor()
				  );
	if (!themed) {
		style()->drawControl(QStyle::CE_PushButtonBevel, &option, &painter, this);
	}

	if (button->id == PROPERTY_GRID_COMBO_BUTTON && !themed) {
		QStyleOption arrow;
		arrow.initFrom(this);
		arrow.rect = rect;
		arrow.state = QStyle::State_Enabled;
		style()->drawPrimitive(QStyle::PE_IndicatorArrowDown, &arrow, &painter, this);
	}
#else
	QColor face = palette().color(QPalette::Button);
	painter.fillRect(rect, face);
	if (pressed) {
		draw3dRect(painter, rect, palette().color(QPalette::Dark), palette().color(QPalette::Dark));
	}
	else {
		draw3dRect(painter, rect, face, palette().color(QPalette::Shadow));
		draw3dRect(
			painter, rect.adjusted(1, 1, -1, -1), palette().color(QPalette::Base), palette().color(QPalette::Dark)
		);
	}
	if (button->id == PROPERTY_GRID_COMBO_BUTTON) {
		QPoint arrow = centerPoint(rect.adjusted(1, 1, -1, -1));
		const QPoint triangle[] = {
			QPoint(arrow.x() - 3, arrow.y() - 1), QPoint(arrow.x() + 3, arrow.y() - 1), QPoint(arrow.x(), arrow.y() + 2)
		};
		painter.save();
		painter.setPen(textColor);
		painter.setBrush(textColor);
		painter.drawPolygon(triangle, 3);
		painter.restore();
	}
#endif

	if (button->id == PROPERTY_GRID_EXPAND_BUTTON) {
		painter.fillRect(dots.x() - 4, dots.y(), 2, 2, textColor);
		painter.fillRect(dots.x(), dots.y(), 2, 2, textColor);
		painter.fillRect(dots.x() + 4, dots.y(), 2, 2, textColor);
	}

	if (focused) {
		drawFocusRect(painter, this, rect.adjusted(2, 2, -2, -2), palette().color(QPalette::Button));
	}
}

void PropertyGridView::drawExpandButton(QPainter& painter, PropertyGridItem* item, QRect captionRect)
{
	QRect button(expandBorder / 2 - 5, centerPoint(captionRect).y() - 4, 9, 9);
	if (item->indent() > 0) {
		button.translate((item->indent() - (item->isCategory() ? 0 : 1)) * expandBorder, 0);
	}

	QColor buttonColor = palette().color(QPalette::Button);
	QColor frameColor = palette().color(QPalette::Dark);
	int left = button.x();
	int top = button.y();
	int right = rightEdge(button);
	int bottom = bottomEdge(button);

	painter.fillRect(left + 1, top + 1, 7, 7, buttonColor);
	painter.fillRect(left + 1, top, 9 - 2, 1, frameColor);
	painter.fillRect(left + 1, bottom - 1, 9 - 2, 1, frameColor);
	painter.fillRect(left, top + 1, 1, 9 - 2, frameColor);
	painter.fillRect(right - 1, top + 1, 1, 9 - 2, frameColor);
	painter.fillRect(left + 1, top + 1, 9 - 2, 3, QColor(255, 255, 255));
	painter.fillRect(left + 1, top + 4, 9 - 4, 2, QColor(255, 255, 255));
	painter.fillRect(left + 2, top + 4, 9 - 4, 1, QColor(0, 0, 0));

	if (!item->isExpanded()) {
		painter.fillRect(left + 4, top + 2, 1, 9 - 4, QColor(0, 0, 0));
	}
}

void PropertyGridView::drawItem(QPainter& painter, int row)
{
	PropertyGridItem* rowItem = m_rows[row];
	QRect rect = itemRect(row);
	QColor lineColor = m_grid->lineColor();
	bool selected = row == m_currentRow;

	painter.setFont(itemFont(rowItem));
	painter.fillRect(rect, m_grid->backColor());

	QRect captionRect = rectFromEdges(rect.x(), rect.y(), rect.x() + dividerPos(), bottomEdge(rect));

	if (rowItem->isCategory()) {
		painter.fillRect(rect, lineColor);

		WhidbeyStyle::fillHorizontalGradient(
			&painter,
			rectFromEdges(rect.x(), bottomEdge(rect) - 2, rightEdge(rect), bottomEdge(rect) - 1),
			lineColor,
			palette().color(QPalette::Light)
		);

		painter.setPen(itemTextColor(rowItem));
		int textWidth = QFontMetrics(itemFont(rowItem)).horizontalAdvance(rowItem->caption());
		QRect textRect((rowItem->indent() + 1) * expandBorder - 1, rect.y() + 1, textWidth + 4, rect.height() - 3);
		drawClippedText(painter, textRect, Qt::AlignRight | Qt::AlignVCenter, rowItem->caption());

		if (selected && hasFocus()) {
			drawFocusRect(painter, this, textRect, lineColor);
		}
	}
	else {
		QRect valueRect = rectFromEdges(rightEdge(captionRect), rect.y(), rightEdge(rect), bottomEdge(rect) - 1);
		drawItemValue(painter, rowItem, valueRect);

		painter.setPen(itemTextColor(rowItem));
		painter.setFont(itemFont(rowItem));

		bool focused = selected && (isViewFocused() || m_inplaceList->isVisible());
		if (focused) {
			painter.setPen(m_grid->highlightTextColor());
			painter.fillRect(captionRect, m_grid->highlightColor());
		}
		else if (selected) {
			painter.fillRect(captionRect, lineColor);
		}

		for (PropertyGridItem* category = rowItem->parentItem(); category; category = category->parentItem()) {
			if (category->isCategory()) {
				painter.fillRect(
					rect.x() + category->indent() * expandBorder, rect.y(), expandBorder, rect.height(), lineColor
				);
			}
		}

		painter.fillRect(0, bottomEdge(rect) - 1, rightEdge(rect), 1, lineColor);
		painter.fillRect(rightEdge(captionRect), rect.y(), 1, rect.height() - 1, lineColor);

		drawItemCaption(painter, rowItem, captionRect);

		if (selected) {
			drawInplaceButtons(painter, rowItem);
		}
	}

	PropertyGridItem* next = item(row + 1);
	if (next && next->isCategory() && next->isVisible()) {
		painter.fillRect(next->indent() * expandBorder, bottomEdge(rect) - 1, rect.width(), 1, m_grid->faceColor());
	}

	if (rowItem->hasVisibleChildren()) {
		painter.save();
		if (!rowItem->isCategory()) {
			painter.setClipRect(captionRect, Qt::IntersectClip);
		}
		drawExpandButton(painter, rowItem, captionRect);
		painter.restore();
	}
}

void PropertyGridView::showToolTip(QRect bounds, QRect textRect, PropertyGridItem* item, bool valuePart)
{
	m_toolTip->setText(m_toolTipText);
	m_toolTip->setGeometry(textRect);
	m_toolTip->activate(true, item, valuePart);
	m_toolTipBounds = bounds;
}

void PropertyGridView::showToolTip(QPoint point)
{
	if (m_inplaceList->isVisible()) {
		return;
	}

	if (!m_toolTip) {
		m_toolTip = new PropertyGridToolTip(this);
	}

	if (m_hotButton) {
		return;
	}

	PropertyGridItem* hovered = itemFromPoint(point);
	if (!hovered || hovered->isCategory()) {
		return;
	}

	int divider = dividerPos();
	QRect rect = hovered->itemRect();
	QRect captionRect = rectFromEdges(rect.x(), rect.y(), rect.x() + divider, bottomEdge(rect));

	if (captionRect.contains(point)) {
		if (m_toolTipText == hovered->caption()) {
			return;
		}
		m_toolTipText = hovered->caption();

		QRect captionScreen(mapToGlobal(captionRect.topLeft()), captionRect.size());
		QRect textRect = captionScreen;
		textRect.setLeft(textRect.left() + hovered->indent() * expandBorder - 1);
		textRect.setTop(textRect.top() - 1);

		int extent = QFontMetrics(itemFont(hovered)).horizontalAdvance(m_toolTipText);
		if (extent + 3 > textRect.width()) {
			showToolTip(captionScreen, textRect, hovered, false);
		}
	}
	else {
		if (m_toolTipText == hovered->viewValue()) {
			return;
		}
		m_toolTipText = hovered->viewValue();

		if (m_inplaceEdit->hasFocus() && m_inplaceEdit->gridItem() == hovered) {
			return;
		}

		QRect valueRect = rectFromEdges(rect.x() + divider, rect.y(), rightEdge(rect), bottomEdge(rect));
		QRect valueScreen(mapToGlobal(valueRect.topLeft()), valueRect.size());
		QRect textRect = hovered->valueRect().adjusted(-1, -2, 0, 1);
		textRect.moveTopLeft(mapToGlobal(textRect.topLeft()));

		int extent = QFontMetrics(itemFont(hovered)).horizontalAdvance(m_toolTipText);
		if (extent + 3 > textRect.width()) {
			showToolTip(valueScreen, textRect, hovered, true);
		}
	}
}

void PropertyGridView::relayToolTipEvent(QEvent::Type type)
{
	if (!m_toolTipBounds.contains(QCursor::pos()) || m_hotButton) {
		m_toolTipText = "";
		m_toolTip->activate(false, nullptr, false);
	}

	switch (type) {
	case QEvent::Wheel:
		m_toolTipText = "";
		m_toolTip->activate(false, nullptr, false);
		break;
	case QEvent::KeyPress:
	case QEvent::MouseButtonPress:
	case QEvent::MouseButtonRelease:
	case QEvent::MouseButtonDblClick:
	case QEvent::Leave:
		m_toolTip->activate(false, nullptr, false);
		break;
	default:
		break;
	}
}

PropertyGrid::PropertyGrid(QWidget* parent) : QWidget(parent)
{
	m_normalFont = QApplication::font("QAbstractItemView");
	m_normalFont.setWeight(QFont::Normal);
	m_normalFont.setItalic(false);
	m_normalFont.setStyleStrategy(QFont::PreferDefault);
	m_boldFont = m_normalFont;
	m_boldFont.setWeight(QFont::Bold);

	setFocusPolicy(Qt::StrongFocus);

	m_scrollBar = new QScrollBar(Qt::Vertical, this);
	m_scrollBar->hide();
	m_view = new PropertyGridView(this);

	connect(m_scrollBar, &QScrollBar::valueChanged, this, [this](int value) { m_view->setTopIndex(value); });
}

PropertyGrid::~PropertyGrid() = default;

QColor PropertyGrid::backColor() const
{
	return palette().color(QPalette::Base);
}

QColor PropertyGrid::foreColor() const
{
	return palette().color(QPalette::Text);
}

QColor PropertyGrid::readOnlyForeColor() const
{
	return palette().color(QPalette::Disabled, QPalette::Text);
}

QColor PropertyGrid::lineColor() const
{
	return palette().color(QPalette::Button);
}

QColor PropertyGrid::categoryForeColor() const
{
	return palette().color(QPalette::ButtonText);
}

QColor PropertyGrid::highlightColor() const
{
	return palette().color(QPalette::Active, QPalette::Highlight);
}

QColor PropertyGrid::highlightTextColor() const
{
	return palette().color(QPalette::Active, QPalette::HighlightedText);
}

QColor PropertyGrid::faceColor() const
{
	return palette().color(QPalette::Button);
}

void PropertyGrid::paintEvent(QPaintEvent* event)
{
	QPainter painter(this);
	painter.fillRect(rect(), faceColor());
	if (m_viewFrame.width() > 0 && m_viewFrame.height() > 0) {
		QColor frame = WhidbeyStyle::colors().staticFrame;
		draw3dRect(painter, m_viewFrame, frame, frame);
	}
}

void PropertyGrid::resizeEvent(QResizeEvent* event)
{
	QWidget::resizeEvent(event);
	reposition();
}

void PropertyGrid::focusInEvent(QFocusEvent* event)
{
	QWidget::focusInEvent(event);
	if (event->reason() == Qt::BacktabFocusReason) {
		navigate(PROPERTY_GRID_UI_PARENT_NEXT, false, nullptr);
	}
	else if (event->reason() == Qt::TabFocusReason) {
		navigate(PROPERTY_GRID_UI_PARENT_PREVIOUS, true, nullptr);
	}
	else {
		m_view->setFocus();
	}
}

QWidget* PropertyGrid::nextGridTabItem(bool previous) const
{
	QWidget* parent = parentWidget();
	if (!parent) {
		return nullptr;
	}

	std::vector<QWidget*> siblings;
	int index = -1;
	for (QObject* child : parent->children()) {
		QWidget* widget = qobject_cast<QWidget*>(child);
		if (!widget) {
			continue;
		}

		if (widget == this) {
			index = static_cast<int>(siblings.size());
			siblings.push_back(widget);
		}
		else if (
			!widget->isWindow() && widget->isVisibleTo(parent) && widget->isEnabled() &&
			(widget->focusPolicy() & Qt::TabFocus)
		) {
			siblings.push_back(widget);
		}
	}

	int size = static_cast<int>(siblings.size());
	if (index == -1 || size <= 1) {
		return nullptr;
	}

	QWidget* next = siblings[(index + (previous ? size - 1 : 1)) % size];
	return next == this ? nullptr : next;
}

void PropertyGrid::navigate(int element, bool forward, PropertyGridItem* item)
{
	int step = forward ? 1 : -1;
	int ui = element + step;

	if (!item) {
		item = m_view->selectedItem();
	}

	while (ui != element) {
		if (ui == PROPERTY_GRID_UI_VIEW_PREVIOUS) {
			ui += step;
		}

		if (ui == PROPERTY_GRID_UI_VIEW) {
			m_view->setFocus();
			m_view->focusInplaceButton(nullptr);
			return;
		}

		if (ui == PROPERTY_GRID_UI_INPLACE_EDIT) {
			if (item && (item->flags() & PROPERTY_GRID_ITEM_HAS_EDIT)) {
				item->onSelect();
				item->setFocusToInplaceControl();
				return;
			}
			ui += step;
		}

		if (ui == PROPERTY_GRID_UI_INPLACE_CONTROL) {
			ui += step;
		}

		if (ui == PROPERTY_GRID_UI_INPLACE_BUTTON) {
			if (item && item->isInplaceButtonsVisible()) {
				const std::vector<std::unique_ptr<PropertyGridInplaceButton>>& buttons = item->inplaceButtons();
				int count = static_cast<int>(buttons.size());
				PropertyGridInplaceButton* focused = m_view->focusedButton();

				if (focused && focused->item == item) {
					int index = focused->index;
					if (forward && index < count - 1) {
						m_view->focusInplaceButton(buttons[index + 1].get());
						return;
					}
					else if (!forward && index > 0) {
						m_view->focusInplaceButton(buttons[index - 1].get());
						return;
					}
				}
				else {
					m_view->focusInplaceButton(buttons[forward ? 0 : count - 1].get());
					return;
				}
			}
			ui += step;
		}

		if (ui == PROPERTY_GRID_UI_VIEW_NEXT) {
			ui += step;
		}

		if (ui == PROPERTY_GRID_UI_VERB) {
			ui += step;
		}

		if (ui == PROPERTY_GRID_UI_PARENT_NEXT || ui == PROPERTY_GRID_UI_PARENT_PREVIOUS) {
			if (QWidget* next = nextGridTabItem(!forward)) {
				next->setFocus();
				return;
			}
			ui = ui == PROPERTY_GRID_UI_PARENT_NEXT ? PROPERTY_GRID_UI_VIEW_PREVIOUS : PROPERTY_GRID_UI_VERB;
		}

		if (ui < PROPERTY_GRID_UI_PARENT_PREVIOUS || ui > PROPERTY_GRID_UI_PARENT_NEXT) {
			return;
		}
	}
}

void PropertyGrid::reposition()
{
	int cx = width();
	int cy = height();
	QRect viewRect(0, 0, cx, cy);

	if (m_toolBarVisible && m_toolBar) {
		m_toolBar->setGeometry(0, 0, cx, toolBarHeight);
		viewRect.setTop(toolBarHeight);
	}

	m_viewFrame = viewRect;

	QRect inner = viewRect.adjusted(1, 1, -1, -1);
	if (inner.width() < 0) {
		inner.setWidth(0);
	}
	if (inner.height() < 0) {
		inner.setHeight(0);
	}

	int itemHeight = m_view->itemHeight();
	int page = itemHeight > 0 ? std::max(1, inner.height() / itemHeight) : 1;
	int maximum = std::max(0, m_view->count() - page);

	if (maximum > 0) {
		int extent = std::min(style()->pixelMetric(QStyle::PM_ScrollBarExtent, nullptr, m_scrollBar), inner.width());
		m_scrollBar->setGeometry(rightEdge(inner) - extent, inner.y(), extent, inner.height());
		m_scrollBar->show();
		inner.setWidth(inner.width() - extent);
	}
	else {
		m_scrollBar->hide();
	}

	m_view->setGeometry(inner);

	{
		QSignalBlocker blocker(m_scrollBar);
		m_scrollBar->setRange(0, maximum);
		m_scrollBar->setPageStep(page);
		m_scrollBar->setSingleStep(1);
		m_scrollBar->setValue(m_view->topIndex());
	}

	m_view->update();
	update();
}

void PropertyGrid::createToolBar()
{
	m_toolBar = new PropertyGridToolBar(this);
	m_toolBar->addButton(toolBarIcon(0, palette()), "Categorized", [this]() {
		setPropertySort(PROPERTY_GRID_SORT_CATEGORIZED);
		refreshToolBarButtons();
	});
	m_toolBar->addButton(toolBarIcon(1, palette()), "Alphabetical", [this]() {
		setPropertySort(PROPERTY_GRID_SORT_ALPHABETICAL);
		refreshToolBarButtons();
	});
	m_toolBar->show();
}

void PropertyGrid::showToolBar(bool show)
{
	if (show && !m_toolBar) {
		createToolBar();
	}
	else if (m_toolBar) {
		m_toolBar->setVisible(show);
	}

	m_toolBarVisible = show;
	reposition();
	refreshToolBarButtons();
}

void PropertyGrid::refreshToolBarButtons()
{
	if (m_toolBarVisible && m_toolBar) {
		m_toolBar->setChecked(0, m_view->propertySort() == PROPERTY_GRID_SORT_CATEGORIZED);
		m_toolBar->setChecked(1, m_view->propertySort() == PROPERTY_GRID_SORT_ALPHABETICAL);
	}
}

PropertyGridSortOrder PropertyGrid::propertySort() const
{
	return m_view->propertySort();
}

void PropertyGrid::setPropertySort(PropertyGridSortOrder sort)
{
	m_view->setPropertySort(sort);
}

void PropertyGrid::onSortChanged()
{
	refreshToolBarButtons();
}

PropertyGridItem* PropertyGrid::addCategory(const QString& caption)
{
	return m_view->addCategory(caption);
}

PropertyGridItem* PropertyGrid::findItem(const QString& caption) const
{
	return findItemByCaption(m_view->categories(), caption);
}

const std::vector<PropertyGridItem*>& PropertyGrid::categories() const
{
	return m_view->categories();
}

PropertyGridItem* PropertyGrid::selectedItem() const
{
	return m_view->selectedItem();
}

void PropertyGrid::refreshContent()
{
	m_view->setPropertySort(propertySort(), true);
}

void PropertyGrid::resetContent()
{
	m_view->resetContent();
	m_view->m_lockUpdate++;
	m_view->clearCategories();
	m_view->m_lockUpdate--;
}

void PropertyGrid::beginUpdate(PropertyGridUpdateContext& context)
{
	m_view->m_lockUpdate = 1;
	context.propertySort = propertySort();
	resetContent();
}

void PropertyGrid::endUpdate(PropertyGridUpdateContext& context)
{
	m_view->m_lockUpdate = 0;
	m_view->setPropertySort(context.propertySort, true);
	reposition();
}

template <>
bool XItem<bool, PropertyGridItemBool, RBX::Reflection::TypedPropertyDescriptor<bool>>::editorValue()
{
	return getBool();
}

template <>
void XItem<bool, PropertyGridItemBool, RBX::Reflection::TypedPropertyDescriptor<bool>>::setEditorValue(
	const bool& value
)
{
	setBool(value);
}

template <>
void XItem<float, PropertyGridItem, RBX::Reflection::TypedPropertyDescriptor<float>>::setEditorValue(const float& value)
{
	setValue(QString::asprintf("%.3g", value));
}

template <>
void XItem<int, PropertyGridItem, RBX::Reflection::TypedPropertyDescriptor<int>>::setEditorValue(const int& value)
{
	setValue(QString::asprintf("%d", value));
}

template <>
G3D::Color3 XItem<G3D::Color3, PropertyGridItemColor, RBX::Reflection::TypedPropertyDescriptor<G3D::Color3>>::
	editorValue()
{
	QColor current = color();
	return G3D::Color3(current.red() / 255.0, current.green() / 255.0, current.blue() / 255.0);
}

template <>
void XItem<G3D::Color3, PropertyGridItemColor, RBX::Reflection::TypedPropertyDescriptor<G3D::Color3>>::setEditorValue(
	const G3D::Color3& value
)
{
	setColor(QColor(
		static_cast<unsigned char>(value.r * 255.0),
		static_cast<unsigned char>(value.g * 255.0),
		static_cast<unsigned char>(value.b * 255.0)
	));
}

template <>
RBX::BrickColor XItem<
	RBX::BrickColor,
	CPropertyGridItemBrickColor,
	RBX::Reflection::TypedPropertyDescriptor<RBX::BrickColor>>::editorValue()
{
	QColor current = color();
	return RBX::BrickColor::closest(
		G3D::Color3(current.red() / 255.0, current.green() / 255.0, current.blue() / 255.0)
	);
}

template <>
void XItem<RBX::BrickColor, CPropertyGridItemBrickColor, RBX::Reflection::TypedPropertyDescriptor<RBX::BrickColor>>::
	setEditorValue(const RBX::BrickColor& value)
{
	G3D::Color4uint8 current = value.color4uint8();
	setColor(QColor(current.r, current.g, current.b));
}

template <>
void XItem<G3D::Vector3, PropertyGridItem, RBX::Reflection::TypedPropertyDescriptor<G3D::Vector3>>::setEditorValue(
	const G3D::Vector3& value
)
{
	setValue(QString::asprintf("%.3g, %.3g, %.3g", value.x, value.y, value.z));
}

template <>
void XItem<std::string, PropertyGridItem, RBX::Reflection::PropertyDescriptor>::setEditorValue(const std::string& value)
{
	setValue(QString::fromLocal8Bit(value.c_str()));
}

template <>
void XItem<
	RBX::Soundscape::SoundId,
	PropertyGridItem,
	RBX::Reflection::TypedPropertyDescriptor<RBX::Soundscape::SoundId>>::
	setEditorValue(const RBX::Soundscape::SoundId& value)
{
	setValue(QString::fromLocal8Bit(value.c_str()));
}

template <>
void XItem<RBX::TextureId, PropertyGridItem, RBX::Reflection::TypedPropertyDescriptor<RBX::TextureId>>::setEditorValue(
	const RBX::TextureId& value
)
{
	setValue(QString::fromLocal8Bit(value.c_str()));
}

template <>
int XItem<int, PropertyGridItemEnum, RBX::Reflection::EnumPropertyDescriptor>::editorValue()
{
	return getEnum();
}

template <>
void XItem<int, PropertyGridItemEnum, RBX::Reflection::EnumPropertyDescriptor>::setEditorValue(const int& value)
{
	setEnum(value);
}

template <>
std::string XItem<std::string, PropertyGridItem, RBX::Reflection::PropertyDescriptor>::editorValue()
{
	return value().toLocal8Bit().constData();
}

template <>
RBX::Soundscape::SoundId XItem<
	RBX::Soundscape::SoundId,
	PropertyGridItem,
	RBX::Reflection::TypedPropertyDescriptor<RBX::Soundscape::SoundId>>::editorValue()
{
	return value().toLocal8Bit().constData();
}

template <>
RBX::TextureId XItem<RBX::TextureId, PropertyGridItem, RBX::Reflection::TypedPropertyDescriptor<RBX::TextureId>>::
	editorValue()
{
	return value().toLocal8Bit().constData();
}

static std::recursive_mutex& createItemsMutex()
{
	static std::recursive_mutex mutex;
	return mutex;
}

PropertyGridItem* CPropGrid::categoryItem(const RBX::Name& category)
{
	QString caption = QString::fromLocal8Bit(category.toString().c_str());
	PropertyGridItem* item = findItem(caption);
	if (!item) {
		item = addCategory(caption);
		item->expand();
	}
	return item;
}

void CPropGrid::createItems(const RBX::Reflection::ClassDescriptor* classDescriptor)
{
	std::scoped_lock lock(createItemsMutex());
	std::for_each(
		classDescriptor->derivedClasses_begin(),
		classDescriptor->derivedClasses_end(),
		[this](const RBX::Reflection::ClassDescriptor* derivedClass) { createItems(derivedClass); }
	);
	std::for_each(
		classDescriptor
			->RBX::Reflection::MemberDescriptorContainer<RBX::Reflection::PropertyDescriptor>::descriptors_begin(),
		classDescriptor
			->RBX::Reflection::MemberDescriptorContainer<RBX::Reflection::PropertyDescriptor>::descriptors_end(),
		[this, classDescriptor](const RBX::Reflection::PropertyDescriptor* descriptor) {
			createItem(classDescriptor, descriptor);
		}
	);
}

void CPropGrid::postRefreshItems()
{
	std::scoped_lock lock(m_mutex);
	if (!m_refreshPosted) {
		QMetaObject::invokeMethod(this, [this]() { refreshItems(); }, Qt::QueuedConnection);
		m_refreshPosted = true;
	}
}

void CPropGrid::onEvent(const RBX::Instance* source, RBX::PropertyChanged event)
{
	std::scoped_lock lock(m_mutex);
	if (!m_refreshAll) {
		std::map<const RBX::Reflection::PropertyDescriptor*, Item*>::const_iterator iter =
			m_itemsByDescriptor.find(&event.getDescriptor());
		if (iter != m_itemsByDescriptor.end()) {
			m_dirtyItems.insert(iter->second);
		}

		postRefreshItems();
	}
}

void CPropGrid::Item::addClassDescriptor(const RBX::Reflection::ClassDescriptor* classDescriptor)
{
	m_classDescriptors.push_back(classDescriptor);
	std::for_each(
		classDescriptor->derivedClasses_begin(),
		classDescriptor->derivedClasses_end(),
		[this](const RBX::Reflection::ClassDescriptor* derivedClass) { addClassDescriptor(derivedClass); }
	);
}

void CPropGrid::InstanceMap::remove(RBX::Instance* instance)
{
	iterator iter = find(&instance->getDescriptor());
	if (iter != end()) {
		std::vector<std::shared_ptr<RBX::Instance>>::iterator found =
			std::find(iter->second.begin(), iter->second.end(), RBX::shared_from(instance));
		if (found != iter->second.end()) {
			iter->second.erase(found);
		}
	}
}

template <class T>
void CPropGrid::insertItem(T* item)
{
	PropertyGridItem* category = categoryItem(item->getDescriptor()->category);

	int i = 0;
	while (i < static_cast<int>(category->children().size())) {
		PropertyGridItem* child = category->children()[i];
		if (item->caption().compare(child->caption()) < 0) {
			break;
		}
		i++;
	}

	category->insertChildItem(i, item);
	m_itemsByDescriptor[item->getDescriptor()] = item;
	m_items.push_back(item);
}

void CPropGrid::removeInstance(std::shared_ptr<RBX::Instance> instance)
{
	RBX::stopListening<RBX::Instance, RBX::PropertyChanged>(instance, this);
	m_instances.remove(instance.get());
}

void CPropGrid::refreshItems()
{
	if (m_items.empty()) {
		PropertyGridUpdateContext context;
		beginUpdate(context);
		createItems(&RBX::Reflection::ClassDescriptor::rootDescriptor());
		endUpdate(context);
	}

	std::vector<Item*> items;
	std::set<Item*> dirty;
	{
		std::scoped_lock lock(m_mutex);
		if (m_refreshAll) {
			items = m_items;
			m_refreshAll = false;
		}
		else {
			dirty = m_dirtyItems;
			m_dirtyItems.clear();
		}
		m_refreshPosted = false;
	}

	{
		RBX::DataModel::Lock lock(m_dataModel);
		std::for_each(items.begin(), items.end(), [this](Item* item) { item->update(m_instances); });
		std::for_each(dirty.begin(), dirty.end(), [this](Item* item) { item->update(m_instances); });
	}

	for (int i = 0; i < static_cast<int>(categories().size()); i++) {
		PropertyGridItem* category = categories()[i];
		const std::vector<PropertyGridItem*>& children = category->children();
		bool hidden = true;
		for (int j = 0; j < static_cast<int>(children.size()); j++) {
			if (!children[j]->isHidden()) {
				hidden = false;
				break;
			}
		}
		category->setHidden(hidden);
	}

	refreshContent();
}

CPropGrid::Item::Item(const RBX::Reflection::PropertyDescriptor& descriptor, CPropGrid* grid) : m_grid(grid)
{
	addClassDescriptor(&descriptor.owner);
}

template <class T, class Base, class Descriptor>
void XItem<T, Base, Descriptor>::update(const CPropGrid::InstanceMap& instances)
{
	T first;
	bool hasValue = false;
	CPropGrid::InstanceMap::const_iterator objectsEnd = instances.end();
	for (std::vector<const RBX::Reflection::ClassDescriptor*>::const_iterator iter = m_classDescriptors.begin(),
																			  end = m_classDescriptors.end();
		 iter != end;
		 ++iter) {
		const RBX::Reflection::ClassDescriptor* descriptor = *iter;
		CPropGrid::InstanceMap::const_iterator found = instances.find(descriptor);
		if (found != objectsEnd) {
			for (std::vector<std::shared_ptr<RBX::Instance>>::const_iterator instance = found->second.begin(),
																			 instancesEnd = found->second.end();
				 instance != instancesEnd;
				 ++instance) {
				if (!hasValue) {
					first = readValue(instance->get());
					setEditorValue(first);
				}
				else if (!equalValues(first, readValue(instance->get()))) {
					Base::PropertyGridItem::setValue("");
					Base::setHidden(false);
					return;
				}
				hasValue = true;
			}
		}
	}
	Base::setHidden(!hasValue);
}

template <class T, class Base, class Descriptor>
T XItem<T, Base, Descriptor>::readValue(RBX::Instance* instance)
{
	RBX::DataModel::Lock lock(m_grid->dataModel());
	return m_descriptor->getValue(instance);
}

template <>
int XItem<int, PropertyGridItemEnum, RBX::Reflection::EnumPropertyDescriptor>::readValue(RBX::Instance* instance)
{
	RBX::DataModel::Lock lock(m_grid->dataModel());
	return m_descriptor->getEnumValue(instance);
}

template <>
std::string XItem<std::string, PropertyGridItem, RBX::Reflection::PropertyDescriptor>::readValue(
	RBX::Instance* instance
)
{
	RBX::DataModel::Lock lock(m_grid->dataModel());
	return m_descriptor->getStringValue(instance);
}

template <class T, class Base, class Descriptor>
void XItem<T, Base, Descriptor>::commit(const T& value)
{
	try {
		std::unique_ptr<RBX::XmlState> undoState(new RBX::UndoState());
		std::unique_ptr<RBX::XmlState> redoState(new RBX::RedoState());

		CPropGrid::InstanceMap& objects = m_grid->instances();
		CPropGrid::InstanceMap::iterator objectsEnd = objects.end();
		std::vector<const RBX::Reflection::ClassDescriptor*>::const_iterator end = m_classDescriptors.end();
		for (std::vector<const RBX::Reflection::ClassDescriptor*>::const_iterator iter = m_classDescriptors.begin();
			 iter != end;
			 ++iter) {
			const RBX::Reflection::ClassDescriptor* descriptor = *iter;
			CPropGrid::InstanceMap::iterator found = objects.find(descriptor);
			if (found != objectsEnd) {
				for (std::vector<std::shared_ptr<RBX::Instance>>::iterator instance = found->second.begin(),
																		   instancesEnd = found->second.end();
					 instance != instancesEnd;
					 ++instance) {
					undoState->addProperty(RBX::Reflection::Property(*m_descriptor, instance->get()));
					writeValue(instance->get(), value);
					redoState->addProperty(RBX::Reflection::Property(*m_descriptor, instance->get()));
				}
			}
		}

		if (m_grid->dataModel()) {
			RBX::DataModel::Lock lock(m_grid->dataModel());
			m_grid->dataModel()->pushState(m_descriptor->name.toString(), undoState.release(), redoState.release());
			m_grid->dataModel()->setDirty(true);
		}
	}
	catch (const std::exception& e) {
		SystemMessageBox::warning(
			m_grid->window(), Resources::string(appTitleStringId), QString::fromLocal8Bit(e.what())
		);
	}
}

template <>
void XItem<std::string, PropertyGridItem, RBX::Reflection::PropertyDescriptor>::writeValue(
	RBX::Instance* instance, const std::string& value
)
{
	RBX::DataModel::Lock lock(m_grid->dataModel());
	m_descriptor->setStringValue(instance, value);
}

template <>
void XItem<int, PropertyGridItemEnum, RBX::Reflection::EnumPropertyDescriptor>::writeValue(
	RBX::Instance* instance, const int& value
)
{
	RBX::DataModel::Lock lock(m_grid->dataModel());
	m_descriptor->setEnumValue(instance, value);
}

XBoolItem::XBoolItem(const RBX::Reflection::TypedPropertyDescriptor<bool>& descriptor, CPropGrid* grid)
	: XItem<bool, PropertyGridItemBool, RBX::Reflection::TypedPropertyDescriptor<bool>>(descriptor, grid)
{
	setCheckBoxStyle();
}

bool XBoolItem::onDrawItemValue(QPainter& painter, QRect valueRect)
{
	if (!m_checkBoxStyle) {
		return false;
	}

	QPoint center = centerPoint(valueRect);
	QStyleOptionButton option;
	option.initFrom(m_view);
	option.styleObject = nullptr;
	option.rect = rectFromEdges(valueRect.x() - 1, center.y() - 6, valueRect.x() + 12, center.y() + 7);
	option.state = (isReadOnly() ? QStyle::State_None : QStyle::State_Enabled) |
				   (isIndeterminate() ? QStyle::State_NoChange : (getBool() ? QStyle::State_On : QStyle::State_Off));
	m_view->style()->drawPrimitive(QStyle::PE_IndicatorCheckBox, &option, &painter, m_view);
	return true;
}

XBrickColorItem::XBrickColorItem(
	const RBX::Reflection::TypedPropertyDescriptor<RBX::BrickColor>& descriptor, CPropGrid* grid
)
	: XItem<RBX::BrickColor, CPropertyGridItemBrickColor, RBX::Reflection::TypedPropertyDescriptor<RBX::BrickColor>>(
		  descriptor, grid
	  )
{
}

bool XBrickColorItem::onDrawItemValue(QPainter& painter, QRect valueRect)
{
	if (value() == "") {
		return false;
	}

	return CPropertyGridItemBrickColor::onDrawItemValue(painter, valueRect);
}

Vector3Item::Vector3Item(const RBX::Reflection::TypedPropertyDescriptor<G3D::Vector3>& descriptor, CPropGrid* grid)
	: XItem<G3D::Vector3, PropertyGridItem, RBX::Reflection::TypedPropertyDescriptor<G3D::Vector3>>(descriptor, grid)
{
	m_components[0] = m_components[1] = m_components[2] = nullptr;
}

bool Vector3Item::onAfterEdit(QString& value)
{
	G3D::Vector3 vector;
	return RBX::StringConverter<G3D::Vector3>::convertToValue(value.toLocal8Bit().toStdString(), vector);
}

void Vector3Item::update(const CPropGrid::InstanceMap& instances)
{
	XItem<G3D::Vector3, PropertyGridItem, RBX::Reflection::TypedPropertyDescriptor<G3D::Vector3>>::update(instances);
	for (int i = 0; i < 3; i++) {
		if (m_components[i]) {
			m_components[i]->update(instances);
		}
	}
}

Vector3ComponentItem::Vector3ComponentItem(
	const RBX::Reflection::TypedPropertyDescriptor<G3D::Vector3>& descriptor, CPropGrid* grid, int component
)
	: XItem<G3D::Vector3, PropertyGridItem, RBX::Reflection::TypedPropertyDescriptor<G3D::Vector3>>(descriptor, grid),
	  m_component(component)
{
	switch (component) {
	case 0:
		setCaption("x");
		break;
	case 1:
		setCaption("y");
		break;
	case 2:
		setCaption("z");
		break;
	}
}

bool Vector3ComponentItem::equalValues(const G3D::Vector3& first, const G3D::Vector3& second)
{
	return first[m_component] == second[m_component];
}

void Vector3ComponentItem::setEditorValue(const G3D::Vector3& value)
{
	m_vector = value;
	setValue(QString::asprintf("%.3g", value[m_component]));
}

G3D::Vector3 Vector3ComponentItem::editorValue()
{
	G3D::Vector3 result = m_vector;
	if (RBX::StringConverter<float>::convertToValue(value().toLocal8Bit().toStdString(), result[m_component])) {
		return result;
	}
	return m_vector;
}

void Vector3ComponentItem::writeValue(RBX::Instance* instance, const G3D::Vector3& value)
{
	RBX::DataModel::Lock lock(m_grid->dataModel());
	G3D::Vector3 current = m_descriptor->getValue(instance);
	current[m_component] = value[m_component];
	m_descriptor->setValue(instance, current);
}

void Vector3Item::onAddChildItem()
{
	PropertyGridItem::onAddChildItem();
	for (int i = 0; i < 3; i++) {
		m_components[i] = new Vector3ComponentItem(*m_descriptor, m_grid, i);
		addChildItem(m_components[i]);
	}
}

RefItem::RefItem(const RBX::Reflection::RefPropertyDescriptor& descriptor, CPropGrid* grid)
	: XItem<std::string, PropertyGridItem, RBX::Reflection::PropertyDescriptor>(descriptor, grid),
	  m_refDescriptor(&descriptor)
{
	setReadOnly(true);
}

void RefItem::update(const CPropGrid::InstanceMap& instances)
{
	RBX::Instance* first = nullptr;
	bool hasValue = false;
	CPropGrid::InstanceMap::const_iterator objectsEnd = instances.end();
	for (std::vector<const RBX::Reflection::ClassDescriptor*>::const_iterator iter = m_classDescriptors.begin(),
																			  end = m_classDescriptors.end();
		 iter != end;
		 ++iter) {
		const RBX::Reflection::ClassDescriptor* descriptor = *iter;
		CPropGrid::InstanceMap::const_iterator found = instances.find(descriptor);
		if (found != objectsEnd) {
			for (std::vector<std::shared_ptr<RBX::Instance>>::const_iterator instance = found->second.begin(),
																			 instancesEnd = found->second.end();
				 instance != instancesEnd;
				 ++instance) {
				if (!hasValue) {
					first = static_cast<RBX::Instance*>(m_refDescriptor->getRefValue(instance->get()));
					setEditorValue(first ? first->getName() : std::string(""));
				}
				else if (first != m_refDescriptor->getRefValue(instance->get())) {
					PropertyGridItem::setValue("");
					setHidden(false);
					return;
				}
				hasValue = true;
			}
		}
	}
	setHidden(!hasValue);
}

ScriptItem::ScriptItem(const RBX::Reflection::PropertyDescriptor& descriptor, CPropGrid* grid)
	: XItem<std::string, PropertyGridItem, RBX::Reflection::PropertyDescriptor>(descriptor, grid)
{
	setFlags(flags() | PROPERTY_GRID_ITEM_HAS_EXPAND_BUTTON);
	setFlags(flags() & ~PROPERTY_GRID_ITEM_HAS_EDIT);
	PropertyGridItem::setValue("(Script)");
}

void RefItem::onValueChanged(QString value)
{
}

void ScriptItem::onValueChanged(QString value)
{
}

void ScriptItem::update(const CPropGrid::InstanceMap& instances)
{
	for (std::vector<const RBX::Reflection::ClassDescriptor*>::const_iterator iter = m_classDescriptors.begin(),
																			  end = m_classDescriptors.end();
		 iter != end;
		 ++iter) {
		const RBX::Reflection::ClassDescriptor* descriptor = *iter;
		CPropGrid::InstanceMap::const_iterator found = instances.find(descriptor);
		if (found != instances.end() && found->second.size() != 0) {
			setHidden(false);
			return;
		}
	}
	setHidden(true);
}

void ScriptItem::onInplaceButtonDown(PropertyGridInplaceButton* button)
{
	CPropGrid::InstanceMap& objects = m_grid->instances();
	CPropGrid::InstanceMap::iterator objectsEnd = objects.end();
	for (std::vector<const RBX::Reflection::ClassDescriptor*>::const_iterator iter = m_classDescriptors.begin(),
																			  end = m_classDescriptors.end();
		 iter != end;
		 ++iter) {
		const RBX::Reflection::ClassDescriptor* descriptor = *iter;
		CPropGrid::InstanceMap::iterator found = objects.find(descriptor);
		if (found != objectsEnd) {
			for (std::vector<std::shared_ptr<RBX::Instance>>::iterator instance = found->second.begin(),
																	   instancesEnd = found->second.end();
				 instance != instancesEnd;
				 ++instance) {
				if (RBX::Script* script = dynamic_cast<RBX::Script*>(instance->get())) {
					theApp.openScriptDocument(RBX::shared_from(script));
				}
			}
		}
	}
}

EnumItem::EnumItem(const RBX::Reflection::EnumPropertyDescriptor& descriptor, CPropGrid* grid)
	: XItem<int, PropertyGridItemEnum, RBX::Reflection::EnumPropertyDescriptor>(descriptor, grid)
{
	for (std::vector<const RBX::Reflection::EnumDescriptor::Item*>::const_iterator
			 iter = descriptor.enumDescriptor.begin(),
			 end = descriptor.enumDescriptor.end();
		 iter != end;
		 ++iter) {
		constraints()->add(QString::fromLocal8Bit((*iter)->name.c_str()), (*iter)->value);
	}
}

template <class T>
ContentItem<T>::ContentItem(const RBX::Reflection::TypedPropertyDescriptor<T>& descriptor, CPropGrid* grid)
	: XItem<T, PropertyGridItem, RBX::Reflection::TypedPropertyDescriptor<T>>(descriptor, grid)
{
	PropertyGridItem::setFlags(PropertyGridItem::flags() | PROPERTY_GRID_ITEM_HAS_COMBO_BUTTON);
	PropertyGridItem::constraints()->add("Browse...", 0, ID_FILE_OPEN);
	PropertyGridItem::constraints()->add("Clear", 1, ID_EDIT_CLEAR);
}

SoundItem::SoundItem(
	const RBX::Reflection::TypedPropertyDescriptor<RBX::Soundscape::SoundId>& descriptor, CPropGrid* grid
)
	: ContentItem<RBX::Soundscape::SoundId>(descriptor, grid)
{
	PropertyGridItem::setValue("(Sound)");
}

TextureItem::TextureItem(const RBX::Reflection::TypedPropertyDescriptor<RBX::TextureId>& descriptor, CPropGrid* grid)
	: ContentItem<RBX::TextureId>(descriptor, grid)
{
	PropertyGridItem::setValue("(Texture)");
}

void CPropGrid::createItem(
	const RBX::Reflection::ClassDescriptor* classDescriptor, const RBX::Reflection::PropertyDescriptor* descriptor
)
{
	if (descriptor->owner != *classDescriptor || !descriptor->isPublic()) {
		return;
	}

	if (const RBX::Reflection::TypedPropertyDescriptor<bool>* d =
			dynamic_cast<const RBX::Reflection::TypedPropertyDescriptor<bool>*>(descriptor)) {
		insertItem(new XBoolItem(*d, this));
	}
	else if (
		const RBX::Reflection::TypedPropertyDescriptor<float>* d =
			dynamic_cast<const RBX::Reflection::TypedPropertyDescriptor<float>*>(descriptor)
	) {
		insertItem(new XItem<float, PropertyGridItem, RBX::Reflection::TypedPropertyDescriptor<float>>(*d, this));
	}
	else if (
		const RBX::Reflection::TypedPropertyDescriptor<int>* d =
			dynamic_cast<const RBX::Reflection::TypedPropertyDescriptor<int>*>(descriptor)
	) {
		insertItem(new XItem<int, PropertyGridItem, RBX::Reflection::TypedPropertyDescriptor<int>>(*d, this));
	}
	else if (
		const RBX::Reflection::TypedPropertyDescriptor<G3D::Color3>* d =
			dynamic_cast<const RBX::Reflection::TypedPropertyDescriptor<G3D::Color3>*>(descriptor)
	) {
		insertItem(new XItem<G3D::Color3, PropertyGridItemColor, RBX::Reflection::TypedPropertyDescriptor<G3D::Color3>>(
			*d, this
		));
	}
	else if (
		const RBX::Reflection::TypedPropertyDescriptor<RBX::BrickColor>* d =
			dynamic_cast<const RBX::Reflection::TypedPropertyDescriptor<RBX::BrickColor>*>(descriptor)
	) {
		insertItem(new XBrickColorItem(*d, this));
	}
	else if (
		const RBX::Reflection::TypedPropertyDescriptor<G3D::Vector3>* d =
			dynamic_cast<const RBX::Reflection::TypedPropertyDescriptor<G3D::Vector3>*>(descriptor)
	) {
		insertItem(new Vector3Item(*d, this));
	}
	else if (
		const RBX::Reflection::EnumPropertyDescriptor* d =
			dynamic_cast<const RBX::Reflection::EnumPropertyDescriptor*>(descriptor)
	) {
		insertItem(new EnumItem(*d, this));
	}
	else if (
		const RBX::Reflection::RefPropertyDescriptor* d =
			dynamic_cast<const RBX::Reflection::RefPropertyDescriptor*>(descriptor)
	) {
		insertItem(new RefItem(*d, this));
	}
	else if (descriptor == &RBX::Script::prop_EmbeddedSourceCode) {
		insertItem(new ScriptItem(*descriptor, this));
	}
	else if (
		const RBX::Reflection::TypedPropertyDescriptor<RBX::TextureId>* d =
			dynamic_cast<const RBX::Reflection::TypedPropertyDescriptor<RBX::TextureId>*>(descriptor)
	) {
		insertItem(new TextureItem(*d, this));
	}
	else if (
		const RBX::Reflection::TypedPropertyDescriptor<RBX::Soundscape::SoundId>* d =
			dynamic_cast<const RBX::Reflection::TypedPropertyDescriptor<RBX::Soundscape::SoundId>*>(descriptor)
	) {
		insertItem(new SoundItem(*d, this));
	}
	else if (descriptor->hasStringValue()) {
		insertItem(new XItem<std::string, PropertyGridItem, RBX::Reflection::PropertyDescriptor>(*descriptor, this));
	}
}

void CPropGrid::InstanceMap::add(RBX::Instance* instance)
{
	(*this)[&instance->getDescriptor()].push_back(RBX::shared_from(instance));
}

void CPropGrid::addInstance(std::shared_ptr<RBX::Instance> instance)
{
	m_instances.add(instance.get());
	RBX::startListening<RBX::Instance, RBX::PropertyChanged>(instance, this);
}

void CSelectionPropGrid::onEvent(const RBX::Selection* source, RBX::SelectionChanged event)
{
	if (event.addedItem) {
		addInstance(event.addedItem);
	}
	if (event.removedItem) {
		removeInstance(event.removedItem);
	}

	std::scoped_lock lock(m_mutex);
	m_refreshAll = true;
	m_dirtyItems.clear();

	postRefreshItems();
}

void CSelectionPropGrid::setSelection(RBX::Selection* selection)
{
	std::scoped_lock lock(m_mutex);

	if (m_selection) {
		RBX::stopListening<RBX::Selection, RBX::SelectionChanged>(m_selection, this);
		std::for_each(m_selection->begin(), m_selection->end(), [this](const std::shared_ptr<RBX::Instance>& instance) {
			removeInstance(instance);
		});
	}

	m_dirtyItems.clear();
	m_instances.clear();
	m_selection = RBX::shared_from(selection);

	if (m_selection) {
		std::for_each(m_selection->begin(), m_selection->end(), [this](const std::shared_ptr<RBX::Instance>& instance) {
			addInstance(instance);
		});
		RBX::startListening<RBX::Selection, RBX::SelectionChanged>(m_selection, this);
	}

	m_refreshAll = true;

	postRefreshItems();
}

void CDataModelPropGrid::activeDataModelChanged(
	std::shared_ptr<RBX::DataModel> newDataModel, std::shared_ptr<RBX::DataModel> oldDataModel
)
{
	if (m_dataModel) {
		RBX::DataModel::Lock lock(m_dataModel);
		setSelection(nullptr);
	}

	m_dataModel = newDataModel.get();
	if (m_dataModel) {
		RBX::DataModel::Lock lock(m_dataModel);
		setSelection(m_dataModel->create<RBX::Selection>());
	}
}

void SoundItem::browse()
{
	CFileDialog dialog(true, QString(), QString(), 0, Resources::string(soundFilterStringId));
	if (dialog.DoModal() == IDOK) {
		std::string path = dialog.GetPathName().toLocal8Bit().toStdString();
		std::ifstream stream(path, std::ios::in | std::ios::binary);
		RBX::Soundscape::SoundId id =
			RBX::ContentProvider::singleton().registerContent(stream, RBX::Name::declare("audio/x-wav", -1));
		commit(id);
	}
}

void TextureItem::browse()
{
	CFileDialog dialog(true, QString(), QString(), 0, Resources::string(imageFilterStringId));
	if (dialog.DoModal() == IDOK) {
		std::string path = dialog.GetPathName().toLocal8Bit().toStdString();
		std::ifstream stream(path, std::ios::in | std::ios::binary);
		RBX::TextureId id = RBX::ContentProvider::singleton().registerContent(stream, imageMimeType(path.c_str()));
		commit(id);
	}
}

CPropGrid::~CPropGrid() = default;

CPropGrid::CPropGrid(QWidget* parent) : PropertyGrid(parent)
{
}

template class XItem<float, PropertyGridItem, RBX::Reflection::TypedPropertyDescriptor<float>>;
template class XItem<int, PropertyGridItem, RBX::Reflection::TypedPropertyDescriptor<int>>;
template class XItem<bool, PropertyGridItemBool, RBX::Reflection::TypedPropertyDescriptor<bool>>;
template class XItem<G3D::Color3, PropertyGridItemColor, RBX::Reflection::TypedPropertyDescriptor<G3D::Color3>>;
template class XItem<G3D::Vector3, PropertyGridItem, RBX::Reflection::TypedPropertyDescriptor<G3D::Vector3>>;
template class XItem<
	RBX::BrickColor,
	CPropertyGridItemBrickColor,
	RBX::Reflection::TypedPropertyDescriptor<RBX::BrickColor>>;
template class XItem<int, PropertyGridItemEnum, RBX::Reflection::EnumPropertyDescriptor>;
template class XItem<std::string, PropertyGridItem, RBX::Reflection::PropertyDescriptor>;
template class XItem<
	RBX::Soundscape::SoundId,
	PropertyGridItem,
	RBX::Reflection::TypedPropertyDescriptor<RBX::Soundscape::SoundId>>;
template class XItem<RBX::TextureId, PropertyGridItem, RBX::Reflection::TypedPropertyDescriptor<RBX::TextureId>>;

PropertiesPane::PropertiesPane(QWidget* parent) : QWidget(parent)
{
	m_caption = new CSelectionCaption(this);
	m_grid = new CDataModelPropGrid(this);
	m_grid->showToolBar(true);
	m_grid->ActiveDocView::refresh();
	setFocusProxy(m_grid);
}

QRect PropertiesPane::captionFrame() const
{
	return QRect(splitterBorder, splitterBorder, width() - 2 * splitterBorder, captionHeight + 2);
}

QRect PropertiesPane::gridFrame() const
{
	int top = splitterBorder + captionHeight + 2 + splitterGap;
	return QRect(splitterBorder, top, width() - 2 * splitterBorder, std::max(0, height() - top - splitterBorder));
}

void PropertiesPane::paintEvent(QPaintEvent* event)
{
	const WhidbeyColors& c = WhidbeyStyle::colors();
	QPainter painter(this);
	painter.fillRect(rect(), c.face);
	draw3dRect(painter, captionFrame(), c.shadow, c.buttonHighlight);
	draw3dRect(painter, gridFrame(), c.shadow, c.buttonHighlight);
}

void PropertiesPane::resizeEvent(QResizeEvent* event)
{
	QWidget::resizeEvent(event);
	m_caption->setGeometry(captionFrame().adjusted(1, 1, -1, -1));
	m_grid->setGeometry(gridFrame().adjusted(1, 1, -1, -1));
}
