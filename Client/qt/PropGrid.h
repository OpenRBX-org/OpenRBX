#ifndef PROPGRID_H
#define PROPGRID_H

#include "ActiveDocView.h"
#include "util/Events.h"
#include "util/Sound.h"
#include "util/TextureId.h"
#include "util/Utilities.h"
#include "v8datamodel/BrickColor.h"
#include "v8datamodel/Selection.h"
#include "v8tree/Instance.h"

#include <G3D/Color3.h>
#include <G3D/Vector3.h>
#include <QColor>
#include <QEvent>
#include <QFont>
#include <QLineEdit>
#include <QListWidget>
#include <QString>
#include <QWidget>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <vector>

class CSelectionCaption;
class PropertyGrid;
class PropertyGridItem;
class PropertyGridToolBar;
class PropertyGridView;
class QPainter;
class QScrollBar;

enum PropertyGridItemFlags
{
	PROPERTY_GRID_ITEM_HAS_EDIT = 1,
	PROPERTY_GRID_ITEM_HAS_EXPAND_BUTTON = 2,
	PROPERTY_GRID_ITEM_HAS_COMBO_BUTTON = 4,
};

enum PropertyGridSortOrder
{
	PROPERTY_GRID_SORT_CATEGORIZED,
	PROPERTY_GRID_SORT_ALPHABETICAL,
	PROPERTY_GRID_SORT_NONE,
};

enum PropertyGridNavigation
{
	PROPERTY_GRID_UI_PARENT_PREVIOUS = -2,
	PROPERTY_GRID_UI_VIEW_PREVIOUS = -1,
	PROPERTY_GRID_UI_VIEW = 0,
	PROPERTY_GRID_UI_INPLACE_EDIT = 1,
	PROPERTY_GRID_UI_INPLACE_CONTROL = 2,
	PROPERTY_GRID_UI_INPLACE_BUTTON = 3,
	PROPERTY_GRID_UI_VIEW_NEXT = 4,
	PROPERTY_GRID_UI_VERB = 5,
	PROPERTY_GRID_UI_PARENT_NEXT = 6,
};

enum PropertyGridInplaceButtonId
{
	PROPERTY_GRID_EXPAND_BUTTON = 100,
	PROPERTY_GRID_COMBO_BUTTON = 101,
};

struct PropertyGridConstraint
{
	QString text;
	qintptr data = 0;
	int index = 0;
	int image = -1;
};

class PropertyGridConstraints
{
public:
	explicit PropertyGridConstraints(PropertyGridItem* item) : m_item(item) {}

	PropertyGridConstraint* add(const QString& text, qintptr data = 0, int image = -1);
	int count() const { return static_cast<int>(m_constraints.size()); }
	bool isEmpty() const { return m_constraints.empty(); }
	QString textAt(int index) const;
	PropertyGridConstraint* at(int index) const { return m_constraints[index].get(); }
	int find(const QString& text) const;
	int findData(qintptr data) const;
	void setCurrent(int current) { m_current = current; }
	int current() const { return m_current; }

private:
	PropertyGridItem* m_item;
	std::vector<std::unique_ptr<PropertyGridConstraint>> m_constraints;
	int m_current = -1;
};

struct PropertyGridInplaceButton
{
	unsigned id = 0;
	PropertyGridItem* item = nullptr;
	int index = -1;
	int width = 0;
};

class PropertyGridItem
{
public:
	explicit PropertyGridItem(const QString& caption, const QString& value = QString());
	PropertyGridItem(const PropertyGridItem&) = delete;
	PropertyGridItem& operator=(const PropertyGridItem&) = delete;
	virtual ~PropertyGridItem();

	const QString& caption() const { return m_caption; }
	void setCaption(const QString& caption) { m_caption = caption; }
	const QString& value() const { return m_value; }
	bool isReadOnly() const { return m_readOnly; }
	void setReadOnly(bool readOnly);
	bool isHidden() const { return m_hidden; }
	void setHidden(bool hidden);
	bool isCategory() const { return m_category; }
	bool isVisible() const { return m_visible; }
	bool isExpanded() const { return m_expanded; }
	bool hasChildren() const { return !m_children.empty(); }
	bool hasVisibleChildren() const;
	bool hasParent(const PropertyGridItem* parent) const;
	bool isSelected() const;
	int indent() const { return m_indent; }
	PropertyGridItem* parentItem() const { return m_parent; }
	const std::vector<PropertyGridItem*>& children() const { return m_children; }

	unsigned flags() const { return m_flags; }
	void setFlags(unsigned flags);
	PropertyGridConstraints* constraints() { return &m_constraints; }
	bool constraintEdit() const { return m_constraintEdit; }
	void setConstraintEdit(bool constraintEdit) { m_constraintEdit = constraintEdit; }

	const std::vector<std::unique_ptr<PropertyGridInplaceButton>>& inplaceButtons() const { return m_inplaceButtons; }
	PropertyGridInplaceButton* findInplaceButton(unsigned id) const;
	PropertyGridInplaceButton* inplaceButtonAt(QPoint point) const;
	QRect inplaceButtonRect(const PropertyGridInplaceButton* button) const;
	bool isInplaceButtonsVisible() const;

	PropertyGridItem* addChildItem(PropertyGridItem* item);
	PropertyGridItem* insertChildItem(int index, PropertyGridItem* item);
	void expand();
	void collapse();
	void select();
	void refresh(bool invalidate = true);

	QRect itemRect() const;
	QRect inplaceWindowsRect() const;
	virtual QRect valueRect() const;
	bool pointInValueRect(QPoint point) const;

	virtual void setValue(QString value);
	virtual void onValueChanged(QString value);
	virtual QString viewValue() const;
	virtual bool onDrawItemValue(QPainter& painter, QRect valueRect);
	virtual void onInplaceButtonDown(PropertyGridInplaceButton* button);
	virtual bool onAfterEdit(QString& value);
	virtual bool onKeyDown(int key);
	virtual bool onChar(unsigned character);
	virtual bool onLButtonDown(QPoint point);
	virtual void onLButtonDblClk(QPoint point);
	virtual void onSelect();
	virtual void onDeselect();
	void onValidateEdit();
	void setFocusToInplaceControl();
	bool selectNextConstraint();

protected:
	friend class PropertyGrid;
	friend class PropertyGridConstraints;
	friend class PropertyGridView;

	virtual void onAddChildItem();
	virtual void onConstraintsChanged();

	void setEditText(const QString& text);
	void setIndent(int indent);
	void addInplaceButton(unsigned id);
	void removeInplaceButton(unsigned id);
	void invalidateView();

	QString m_caption;
	QString m_value;
	PropertyGridView* m_view = nullptr;
	PropertyGridItem* m_parent = nullptr;
	std::vector<PropertyGridItem*> m_children;
	PropertyGridConstraints m_constraints;
	std::vector<std::unique_ptr<PropertyGridInplaceButton>> m_inplaceButtons;
	unsigned m_flags = PROPERTY_GRID_ITEM_HAS_EDIT;
	int m_index = -1;
	int m_indent = 0;
	bool m_hidden = false;
	bool m_expanded = false;
	bool m_visible = false;
	bool m_readOnly = false;
	bool m_category = false;
	bool m_constraintEdit = false;
};

class PropertyGridItemBool : public PropertyGridItem
{
public:
	explicit PropertyGridItemBool(const QString& caption, bool value = false);

	void setValue(QString value) override;
	virtual void setBool(bool value);
	bool getBool() const { return m_boolValue; }
	void setCheckBoxStyle(bool checkBoxStyle = true);

	QString viewValue() const override;
	bool onKeyDown(int key) override;
	void onLButtonDblClk(QPoint point) override;
	bool onLButtonDown(QPoint point) override;
	bool onDrawItemValue(QPainter& painter, QRect valueRect) override;

protected:
	bool pointInCheckBoxRect(QPoint point) const;

	bool m_boolValue = false;
	bool m_checkBoxStyle = false;
	QString m_trueText;
	QString m_falseText;
};

class PropertyGridItemEnum : public PropertyGridItem
{
public:
	explicit PropertyGridItemEnum(const QString& caption, int value = 0);

	void setValue(QString value) override;
	virtual void setEnum(int value);
	void setEnum(const PropertyGridConstraint* constraint);
	int getEnum() const { return m_enumValue; }

protected:
	void onConstraintsChanged() override;

	int m_enumValue = 0;
};

class PropertyGridItemColor : public PropertyGridItem
{
public:
	explicit PropertyGridItemColor(const QString& caption, QColor color = QColor(0, 0, 0));

	void setValue(QString value) override;
	virtual void setColor(QColor color);
	QColor color() const { return m_color; }

	QRect valueRect() const override;
	bool onDrawItemValue(QPainter& painter, QRect valueRect) override;
	void onInplaceButtonDown(PropertyGridInplaceButton* button) override;

	static QColor stringToColor(const QString& text);
	static QString colorToString(QColor color);

protected:
	QColor m_color;
};

class CPropertyGridItemBrickColor : public PropertyGridItemColor
{
public:
	explicit CPropertyGridItemBrickColor(const QString& caption, QColor color = QColor(0, 0, 0));

	void setValue(QString value) override;
	void setColor(QColor color) override;
	void onInplaceButtonDown(PropertyGridInplaceButton* button) override;
};

class PropertyGridInplaceEdit : public QLineEdit
{
public:
	explicit PropertyGridInplaceEdit(PropertyGridView* view);

	void setValue(const QString& value) { m_value = value; }
	void showForItem(PropertyGridItem* item, QRect rect);
	void hideWindow();
	void destroyItem();
	void clearItem() { m_item = nullptr; }
	PropertyGridItem* gridItem() const { return m_item; }
	bool isCancelled() const { return m_cancel; }
	bool isEditMode() const { return m_editMode; }
	bool selectConstraint(int direction, bool circle);
	void handleChar(unsigned character);

protected:
	bool focusNextPrevChild(bool next) override;
	void focusInEvent(QFocusEvent* event) override;
	void focusOutEvent(QFocusEvent* event) override;
	void keyPressEvent(QKeyEvent* event) override;
	void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
	PropertyGridView* m_view;
	PropertyGridItem* m_item = nullptr;
	QString m_value;
	bool m_cancel = false;
	bool m_editMode = false;
};

class PropertyGridInplaceList : public QListWidget
{
public:
	explicit PropertyGridInplaceList(PropertyGridView* view);

	void showForItem(PropertyGridItem* item, QRect rect);
	void destroyItem();
	PropertyGridItem* gridItem() const { return m_item; }

protected:
	bool event(QEvent* event) override;
	void hideEvent(QHideEvent* event) override;
	void keyPressEvent(QKeyEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;

private:
	void apply();
	void cancel();

	PropertyGridView* m_view;
	PropertyGridItem* m_item = nullptr;
};

class PropertyGridToolTip : public QWidget
{
public:
	explicit PropertyGridToolTip(PropertyGridView* view);

	void setText(const QString& text) { m_text = text; }
	void activate(bool active, PropertyGridItem* item, bool valuePart);

protected:
	void paintEvent(QPaintEvent* event) override;

private:
	PropertyGridView* m_view;
	QString m_text;
};

class PropertyGridView : public QWidget
{
public:
	explicit PropertyGridView(PropertyGrid* grid);
	~PropertyGridView() override;

	PropertyGrid* grid() const { return m_grid; }
	PropertyGridInplaceEdit* inplaceEdit() const { return m_inplaceEdit; }
	PropertyGridInplaceList* inplaceList() const { return m_inplaceList; }
	const std::vector<PropertyGridItem*>& categories() const { return m_categories; }

	PropertyGridItem* addCategory(const QString& caption);
	PropertyGridItem* insertCategory(int index, const QString& caption);
	void clearCategories();

	PropertyGridSortOrder propertySort() const { return m_sort; }
	void setPropertySort(PropertyGridSortOrder sort, bool refresh = false);
	void refresh();
	void resetContent();

	int dividerPos() const;
	void setDividerPos(int divider);

	int count() const { return static_cast<int>(m_rows.size()); }
	PropertyGridItem* item(int index) const;
	PropertyGridItem* selectedItem() const { return item(m_currentRow); }
	PropertyGridItem* itemFromPoint(QPoint point) const;
	QRect itemRect(int index) const;
	void setCurrentSelection(int index);
	int topIndex() const { return m_topIndex; }
	void setTopIndex(int index);
	int itemHeight() const { return m_itemHeight; }
	int pageSize() const;

	void onSelectionChanged();
	void focusInplaceButton(PropertyGridInplaceButton* button);
	PropertyGridInplaceButton* focusedButton() const { return m_focusedButton; }
	void pressInplaceButton(PropertyGridInplaceButton* button);
	void inplaceButtonRemoved(PropertyGridInplaceButton* button);
	void itemDestroyed(PropertyGridItem* item);
	bool isViewFocused() const;

	QFont itemFont(const PropertyGridItem* item) const;
	QColor itemTextColor(const PropertyGridItem* item) const;

protected:
	bool event(QEvent* event) override;
	bool focusNextPrevChild(bool next) override;
	void paintEvent(QPaintEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void mouseDoubleClickEvent(QMouseEvent* event) override;
	void wheelEvent(QWheelEvent* event) override;
	void keyPressEvent(QKeyEvent* event) override;
	void focusInEvent(QFocusEvent* event) override;
	void focusOutEvent(QFocusEvent* event) override;
	void leaveEvent(QEvent* event) override;

private:
	friend class PropertyGrid;
	friend class PropertyGridItem;

	int insertItem(PropertyGridItem* item, int index);
	int doExpand(PropertyGridItem* item, int index);
	void doCollapse(PropertyGridItem* item);
	void refreshIndexes();
	void switchExpandState(int index);
	void rowsChanged();
	void scrollContents();
	bool hitTestSplitter(QPoint point) const;

	void onKeyDown(int key, Qt::KeyboardModifiers modifiers);
	void onSysKeyDown(int key);
	void onChar(unsigned character, bool forward);
	void listKeyDown(int key);

	void drawItem(QPainter& painter, int row);
	void drawItemValue(QPainter& painter, PropertyGridItem* item, QRect valueRect);
	void drawItemCaption(QPainter& painter, PropertyGridItem* item, QRect captionRect);
	void drawInplaceButtons(QPainter& painter, PropertyGridItem* item);
	void drawInplaceButton(QPainter& painter, PropertyGridInplaceButton* button, QRect rect);
	void drawExpandButton(QPainter& painter, PropertyGridItem* item, QRect captionRect);

	void showToolTip(QPoint point);
	void showToolTip(QRect bounds, QRect textRect, PropertyGridItem* item, bool valuePart);
	void relayToolTipEvent(QEvent::Type type);

	PropertyGrid* m_grid;
	PropertyGridInplaceEdit* m_inplaceEdit;
	PropertyGridInplaceList* m_inplaceList;
	PropertyGridToolTip* m_toolTip = nullptr;
	std::vector<PropertyGridItem*> m_categories;
	std::vector<PropertyGridItem*> m_rows;
	PropertyGridItem* m_selected = nullptr;
	PropertyGridInplaceButton* m_focusedButton = nullptr;
	PropertyGridInplaceButton* m_hotButton = nullptr;
	PropertyGridInplaceButton* m_pressedButton = nullptr;
	PropertyGridSortOrder m_sort = PROPERTY_GRID_SORT_CATEGORIZED;
	QString m_toolTipText;
	QRect m_toolTipBounds;
	double m_divider = .5;
	int m_currentRow = -1;
	int m_topIndex = 0;
	int m_itemHeight = 0;
	int m_lockUpdate = 0;
	bool m_autoDivider = true;
	bool m_tracking = false;
};

struct PropertyGridUpdateContext
{
	PropertyGridSortOrder propertySort = PROPERTY_GRID_SORT_CATEGORIZED;
};

class PropertyGrid : public QWidget
{
public:
	explicit PropertyGrid(QWidget* parent = nullptr);
	~PropertyGrid() override;

	void showToolBar(bool show);
	PropertyGridSortOrder propertySort() const;
	void setPropertySort(PropertyGridSortOrder sort);

	PropertyGridItem* addCategory(const QString& caption);
	PropertyGridItem* findItem(const QString& caption) const;
	const std::vector<PropertyGridItem*>& categories() const;
	PropertyGridItem* selectedItem() const;

	void refreshContent();
	void resetContent();
	void beginUpdate(PropertyGridUpdateContext& context);
	void endUpdate(PropertyGridUpdateContext& context);

	void navigate(int element, bool forward, PropertyGridItem* item);
	void onSortChanged();
	int viewWindowWidth() const { return m_viewFrame.width(); }

	QFont normalFont() const { return m_normalFont; }
	QFont boldFont() const { return m_boldFont; }
	QColor backColor() const;
	QColor foreColor() const;
	QColor readOnlyForeColor() const;
	QColor lineColor() const;
	QColor categoryForeColor() const;
	QColor highlightColor() const;
	QColor highlightTextColor() const;
	QColor faceColor() const;
	void reposition();

protected:
	void paintEvent(QPaintEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;
	void focusInEvent(QFocusEvent* event) override;

private:
	void createToolBar();
	void refreshToolBarButtons();
	QWidget* nextGridTabItem(bool previous) const;

	PropertyGridView* m_view;
	QScrollBar* m_scrollBar;
	PropertyGridToolBar* m_toolBar = nullptr;
	QFont m_normalFont;
	QFont m_boldFont;
	QRect m_viewFrame;
	bool m_toolBarVisible = false;
};

class CPropGrid : public PropertyGrid, public RBX::Listener<RBX::Instance, RBX::PropertyChanged>
{
public:
	class InstanceMap
		: public std::map<const RBX::Reflection::ClassDescriptor*, std::vector<std::shared_ptr<RBX::Instance>>>
	{
	public:
		void remove(RBX::Instance* instance);
		void add(RBX::Instance* instance);
	};

	class Item
	{
	public:
		Item(const RBX::Reflection::PropertyDescriptor& descriptor, CPropGrid* grid);
		virtual ~Item() = default;

		virtual void update(const InstanceMap& instances) = 0;

	protected:
		void addClassDescriptor(const RBX::Reflection::ClassDescriptor* classDescriptor);

		std::vector<const RBX::Reflection::ClassDescriptor*> m_classDescriptors;
		CPropGrid* m_grid;
	};

	explicit CPropGrid(QWidget* parent = nullptr);
	~CPropGrid() override;

	virtual void createItem(
		const RBX::Reflection::ClassDescriptor* classDescriptor, const RBX::Reflection::PropertyDescriptor* descriptor
	);
	virtual RBX::Selection* selection() = 0;

	RBX::DataModel* dataModel() const { return m_dataModel; }
	InstanceMap& instances() { return m_instances; }

protected:
	void onEvent(const RBX::Instance* source, RBX::PropertyChanged event) override;

	void createItems(const RBX::Reflection::ClassDescriptor* classDescriptor);
	void removeInstance(std::shared_ptr<RBX::Instance> instance);
	void addInstance(std::shared_ptr<RBX::Instance> instance);
	void postRefreshItems();
	void refreshItems();

private:
	PropertyGridItem* categoryItem(const RBX::Name& category);

	template <class T>
	void insertItem(T* item);

protected:
	std::recursive_mutex m_mutex;
	RBX::DataModel* m_dataModel = nullptr;
	std::vector<Item*> m_items;
	std::map<const RBX::Reflection::PropertyDescriptor*, Item*> m_itemsByDescriptor;
	bool m_refreshPosted = false;
	bool m_refreshAll = false;
	std::set<Item*> m_dirtyItems;
	InstanceMap m_instances;
};

template <class T, class Base, class Descriptor>
class XItem : public Base, public CPropGrid::Item
{
public:
	XItem(const Descriptor& descriptor, CPropGrid* grid)
		: Base(QString::fromLocal8Bit(descriptor.name.c_str())), Item(descriptor, grid), m_descriptor(&descriptor)
	{
		Base::m_hidden = true;
		Base::setReadOnly(descriptor.isReadOnly());
	}

	void onValueChanged(QString value) override
	{
		Base::onValueChanged(value);
		commit(editorValue());
	}

	void update(const CPropGrid::InstanceMap& instances) override;

	virtual T editorValue()
	{
		T result;
		if (RBX::StringConverter<T>::convertToValue(Base::value().toLocal8Bit().toStdString(), result)) {
			return result;
		}
		return T();
	}
	virtual void setEditorValue(const T& value);
	virtual bool equalValues(const T& first, const T& second) { return first == second; }
	virtual void writeValue(RBX::Instance* instance, const T& value)
	{
		RBX::DataModel::Lock lock(m_grid->dataModel());
		m_descriptor->setValue(instance, value);
	}
	virtual bool isIndeterminate() { return Base::value().isEmpty(); }

	const Descriptor* getDescriptor() const { return m_descriptor; }

protected:
	T readValue(RBX::Instance* instance);
	void commit(const T& value);

	const Descriptor* m_descriptor;
};

template <>
bool XItem<bool, PropertyGridItemBool, RBX::Reflection::TypedPropertyDescriptor<bool>>::editorValue();
template <>
void XItem<bool, PropertyGridItemBool, RBX::Reflection::TypedPropertyDescriptor<bool>>::setEditorValue(
	const bool& value
);
template <>
void XItem<float, PropertyGridItem, RBX::Reflection::TypedPropertyDescriptor<float>>::setEditorValue(
	const float& value
);
template <>
void XItem<int, PropertyGridItem, RBX::Reflection::TypedPropertyDescriptor<int>>::setEditorValue(const int& value);
template <>
G3D::Color3 XItem<G3D::Color3, PropertyGridItemColor, RBX::Reflection::TypedPropertyDescriptor<G3D::Color3>>::
	editorValue();
template <>
void XItem<G3D::Color3, PropertyGridItemColor, RBX::Reflection::TypedPropertyDescriptor<G3D::Color3>>::setEditorValue(
	const G3D::Color3& value
);
template <>
RBX::BrickColor XItem<
	RBX::BrickColor,
	CPropertyGridItemBrickColor,
	RBX::Reflection::TypedPropertyDescriptor<RBX::BrickColor>>::editorValue();
template <>
void XItem<RBX::BrickColor, CPropertyGridItemBrickColor, RBX::Reflection::TypedPropertyDescriptor<RBX::BrickColor>>::
	setEditorValue(const RBX::BrickColor& value);
template <>
void XItem<G3D::Vector3, PropertyGridItem, RBX::Reflection::TypedPropertyDescriptor<G3D::Vector3>>::setEditorValue(
	const G3D::Vector3& value
);
template <>
std::string XItem<std::string, PropertyGridItem, RBX::Reflection::PropertyDescriptor>::editorValue();
template <>
void XItem<std::string, PropertyGridItem, RBX::Reflection::PropertyDescriptor>::setEditorValue(
	const std::string& value
);
template <>
void XItem<std::string, PropertyGridItem, RBX::Reflection::PropertyDescriptor>::writeValue(
	RBX::Instance* instance, const std::string& value
);
template <>
std::string XItem<std::string, PropertyGridItem, RBX::Reflection::PropertyDescriptor>::readValue(
	RBX::Instance* instance
);
template <>
RBX::Soundscape::SoundId XItem<
	RBX::Soundscape::SoundId,
	PropertyGridItem,
	RBX::Reflection::TypedPropertyDescriptor<RBX::Soundscape::SoundId>>::editorValue();
template <>
void XItem<
	RBX::Soundscape::SoundId,
	PropertyGridItem,
	RBX::Reflection::TypedPropertyDescriptor<RBX::Soundscape::SoundId>>::
	setEditorValue(const RBX::Soundscape::SoundId& value);
template <>
RBX::TextureId XItem<RBX::TextureId, PropertyGridItem, RBX::Reflection::TypedPropertyDescriptor<RBX::TextureId>>::
	editorValue();
template <>
void XItem<RBX::TextureId, PropertyGridItem, RBX::Reflection::TypedPropertyDescriptor<RBX::TextureId>>::setEditorValue(
	const RBX::TextureId& value
);
template <>
int XItem<int, PropertyGridItemEnum, RBX::Reflection::EnumPropertyDescriptor>::editorValue();
template <>
void XItem<int, PropertyGridItemEnum, RBX::Reflection::EnumPropertyDescriptor>::setEditorValue(const int& value);
template <>
void XItem<int, PropertyGridItemEnum, RBX::Reflection::EnumPropertyDescriptor>::writeValue(
	RBX::Instance* instance, const int& value
);
template <>
int XItem<int, PropertyGridItemEnum, RBX::Reflection::EnumPropertyDescriptor>::readValue(RBX::Instance* instance);

class XBoolItem : public XItem<bool, PropertyGridItemBool, RBX::Reflection::TypedPropertyDescriptor<bool>>
{
public:
	XBoolItem(const RBX::Reflection::TypedPropertyDescriptor<bool>& descriptor, CPropGrid* grid);

	bool onDrawItemValue(QPainter& painter, QRect valueRect) override;
};

class XBrickColorItem : public XItem<
							RBX::BrickColor,
							CPropertyGridItemBrickColor,
							RBX::Reflection::TypedPropertyDescriptor<RBX::BrickColor>>
{
public:
	XBrickColorItem(const RBX::Reflection::TypedPropertyDescriptor<RBX::BrickColor>& descriptor, CPropGrid* grid);

	bool onDrawItemValue(QPainter& painter, QRect valueRect) override;
};

class Vector3ComponentItem
	: public XItem<G3D::Vector3, PropertyGridItem, RBX::Reflection::TypedPropertyDescriptor<G3D::Vector3>>
{
public:
	Vector3ComponentItem(
		const RBX::Reflection::TypedPropertyDescriptor<G3D::Vector3>& descriptor, CPropGrid* grid, int component
	);

	G3D::Vector3 editorValue() override;
	void setEditorValue(const G3D::Vector3& value) override;
	bool equalValues(const G3D::Vector3& first, const G3D::Vector3& second) override;
	void writeValue(RBX::Instance* instance, const G3D::Vector3& value) override;

private:
	int m_component;
	G3D::Vector3 m_vector;
};

class Vector3Item : public XItem<G3D::Vector3, PropertyGridItem, RBX::Reflection::TypedPropertyDescriptor<G3D::Vector3>>
{
public:
	Vector3Item(const RBX::Reflection::TypedPropertyDescriptor<G3D::Vector3>& descriptor, CPropGrid* grid);

	bool onAfterEdit(QString& value) override;
	void update(const CPropGrid::InstanceMap& instances) override;

protected:
	void onAddChildItem() override;

private:
	Vector3ComponentItem* m_components[3];
};

class RefItem : public XItem<std::string, PropertyGridItem, RBX::Reflection::PropertyDescriptor>
{
public:
	RefItem(const RBX::Reflection::RefPropertyDescriptor& descriptor, CPropGrid* grid);

	void onValueChanged(QString value) override;
	void update(const CPropGrid::InstanceMap& instances) override;

private:
	const RBX::Reflection::RefPropertyDescriptor* m_refDescriptor;
};

class ScriptItem : public XItem<std::string, PropertyGridItem, RBX::Reflection::PropertyDescriptor>
{
public:
	ScriptItem(const RBX::Reflection::PropertyDescriptor& descriptor, CPropGrid* grid);

	void onValueChanged(QString value) override;
	void update(const CPropGrid::InstanceMap& instances) override;
	void onInplaceButtonDown(PropertyGridInplaceButton* button) override;
};

class EnumItem : public XItem<int, PropertyGridItemEnum, RBX::Reflection::EnumPropertyDescriptor>
{
public:
	EnumItem(const RBX::Reflection::EnumPropertyDescriptor& descriptor, CPropGrid* grid);
};

template <class T>
class ContentItem : public XItem<T, PropertyGridItem, RBX::Reflection::TypedPropertyDescriptor<T>>
{
public:
	using ContentBase = XItem<T, PropertyGridItem, RBX::Reflection::TypedPropertyDescriptor<T>>;

	ContentItem(const RBX::Reflection::TypedPropertyDescriptor<T>& descriptor, CPropGrid* grid);

	void onValueChanged(QString value) override
	{
		switch (PropertyGridItem::constraints()->current()) {
		case 0:
			browse();
			break;
		default:
			ContentBase::commit(T(value.toLocal8Bit().constData()));
			break;
		case 1:
			ContentBase::commit(T());
			break;
		}
	}

	virtual void browse() = 0;
};

class SoundItem : public ContentItem<RBX::Soundscape::SoundId>
{
public:
	SoundItem(const RBX::Reflection::TypedPropertyDescriptor<RBX::Soundscape::SoundId>& descriptor, CPropGrid* grid);

	void browse() override;
};

class TextureItem : public ContentItem<RBX::TextureId>
{
public:
	TextureItem(const RBX::Reflection::TypedPropertyDescriptor<RBX::TextureId>& descriptor, CPropGrid* grid);

	void browse() override;
};

class CSelectionPropGrid : public CPropGrid, public RBX::Listener<RBX::Selection, RBX::SelectionChanged>
{
public:
	explicit CSelectionPropGrid(QWidget* parent = nullptr) : CPropGrid(parent) {}

	~CSelectionPropGrid() override { setSelection(nullptr); }

	RBX::Selection* selection() override { return m_selection.get(); }

	void setSelection(RBX::Selection* selection);

protected:
	using CPropGrid::onEvent;

	void onEvent(const RBX::Selection* source, RBX::SelectionChanged event) override;

private:
	std::shared_ptr<RBX::Selection> m_selection;
};

class CDataModelPropGrid : public CSelectionPropGrid, public ActiveDocView
{
public:
	explicit CDataModelPropGrid(QWidget* parent = nullptr) : CSelectionPropGrid(parent) {}

protected:
	void activeDataModelChanged(
		std::shared_ptr<RBX::DataModel> newDataModel, std::shared_ptr<RBX::DataModel> oldDataModel
	) override;
};

class PropertiesPane : public QWidget
{
public:
	explicit PropertiesPane(QWidget* parent = nullptr);

protected:
	void paintEvent(QPaintEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;

private:
	QRect captionFrame() const;
	QRect gridFrame() const;

	CSelectionCaption* m_caption;
	CDataModelPropGrid* m_grid;
};

#endif // PROPGRID_H
