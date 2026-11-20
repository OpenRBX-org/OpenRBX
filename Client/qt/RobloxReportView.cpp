#include "RobloxReportView.h"

#include "CommandIds.h"
#include "RobloxDoc.h"
#include "util/RunStateOwner.h"
#include "v8tree/Service.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QHeaderView>
#include <QKeyEvent>
#include <QPainter>
#include <QVBoxLayout>
#include <algorithm>

static const int refreshIntervalMilliseconds = 333;
static const int nameColumnWidth = 200;
static const int valueColumnWidth = 200;
static const int minColumnWidth = 10;
static const int noItemsTextMargin = 5;
static const char* const noItemsText = "There are no items to show.";

enum ReportColumn
{
	REPORT_COLUMN_NAME,
	REPORT_COLUMN_VALUE,
};

StatsItemRecord::StatsItemRecord(std::shared_ptr<RBX::Stats::Item> item, std::shared_ptr<RBX::DataModel> dataModel)
	: m_dataModel(dataModel), m_item(item)
{
	RBX::DataModel::Lock lock(dataModel);
	RBX::startListening<RBX::Instance, RBX::ChildAdded>(item, this);
	RBX::startListening<RBX::Instance, RBX::ChildRemoved>(item, this);
}

StatsItemRecord::~StatsItemRecord()
{
	RBX::DataModel::Lock lock(m_dataModel);
	RBX::stopListening<RBX::Instance, RBX::ChildAdded>(m_item, this);
	RBX::stopListening<RBX::Instance, RBX::ChildRemoved>(m_item, this);
}

QString StatsItemRecord::nameCaption() const
{
	RBX::DataModel::Lock lock(m_dataModel);
	return QString::fromLocal8Bit(m_item->getName().c_str());
}

QString StatsItemRecord::valueCaption() const
{
	RBX::DataModel::Lock lock(m_dataModel);
	return QString::fromLocal8Bit(m_item->getStringValue().c_str());
}

void StatsItemRecord::onMarshaledEvent(const RBX::Instance* source, RBX::ChildRemoved event)
{
	for (int i = static_cast<int>(m_children.size()) - 1; i >= 0; i--) {
		if (m_children[i]->item().get() == event.child.get()) {
			m_children.erase(m_children.begin() + i);
			break;
		}
	}
}

void StatsItemRecord::onMarshaledEvent(const RBX::Instance* source, RBX::ChildAdded event)
{
	std::shared_ptr<RBX::Stats::Item> item = std::static_pointer_cast<RBX::Stats::Item>(event.child);
	m_children.push_back(std::make_shared<StatsItemRecord>(item, m_dataModel));
}

StatsItemRow::StatsItemRow(const std::shared_ptr<StatsItemRecord>& record) : m_record(record)
{
}

QVariant StatsItemRow::data(int column, int role) const
{
	if (role != Qt::DisplayRole) {
		return QTreeWidgetItem::data(column, role);
	}

	std::shared_ptr<StatsItemRecord> record = m_record.lock();
	if (!record) {
		return QString();
	}

	switch (column) {
	case REPORT_COLUMN_NAME:
		return record->nameCaption();
	case REPORT_COLUMN_VALUE:
		return record->valueCaption();
	default:
		return QString();
	}
}

StatsReportControl::StatsReportControl(QWidget* parent)
	: ReportControl(parent), m_refreshTimer(this), m_autoWidth{nameColumnWidth, valueColumnWidth}
{
	setColumnCount(2);
	setHeaderLabels({QStringLiteral("Name"), QStringLiteral("Value")});

	header()->setSectionsMovable(true);
	header()->setFirstSectionMovable(true);
	header()->resizeSection(REPORT_COLUMN_NAME, nameColumnWidth);
	header()->resizeSection(REPORT_COLUMN_VALUE, valueColumnWidth);

	connect(header(), &QHeaderView::sectionResized, this, [this](int logicalIndex, int oldSize, int newSize) {
		if (m_sizingColumns || viewport()->width() <= 0) {
			return;
		}
		int first = header()->logicalIndex(0);
		int last = header()->logicalIndex(1);
		if (logicalIndex == first) {
			m_autoWidth[first] = newSize;
			m_autoWidth[last] = std::max(viewport()->width() - newSize, minColumnWidth);
		}
		else {
			m_autoWidth[first] = header()->sectionSize(first);
			m_autoWidth[last] = newSize;
		}
		applyColumnSizing();
	});
	connect(header(), &QHeaderView::sectionMoved, this, [this]() { applyColumnSizing(); });
	connect(this, &QTreeWidget::itemExpanded, this, [](QTreeWidgetItem* row) {
		if (std::shared_ptr<StatsItemRecord> record = static_cast<StatsItemRow*>(row)->record()) {
			record->setExpanded(true);
		}
	});
	connect(this, &QTreeWidget::itemCollapsed, this, [](QTreeWidgetItem* row) {
		if (std::shared_ptr<StatsItemRecord> record = static_cast<StatsItemRow*>(row)->record()) {
			record->setExpanded(false);
		}
	});
	connect(&m_refreshTimer, &QTimer::timeout, this, [this]() { refreshReport(); });
}

StatsReportControl::~StatsReportControl()
{
	setDataModel(std::shared_ptr<RBX::DataModel>());
}

void StatsReportControl::setDataModel(std::shared_ptr<RBX::DataModel> dataModel)
{
	if (dataModel == m_dataModel) {
		return;
	}

	if (m_dataModel) {
		RBX::DataModel::Lock lock(m_dataModel);

		RBX::stopListening<RBX::Instance, RBX::ChildAdded>(m_statsService, this);
		RBX::stopListening<RBX::Instance, RBX::ChildRemoved>(m_statsService, this);
		MarshaledInstanceListener<RBX::ChildAdded>::cancelPendingEvents();
		MarshaledInstanceListener<RBX::ChildRemoved>::cancelPendingEvents();

		m_records.clear();
	}

	populate();
	m_statsService.reset();
	m_dataModel = dataModel;

	if (m_dataModel) {
		RBX::DataModel::Lock lock(dataModel);
		m_statsService = RBX::shared_from(m_dataModel->create<RBX::Stats::StatsService>());
		m_refreshTimer.start(refreshIntervalMilliseconds);

		RBX::startListening<RBX::Instance, RBX::ChildRemoved>(m_statsService, this);
		RBX::startListening<RBX::Instance, RBX::ChildAdded>(m_statsService, this);
		m_dataModel->create<RBX::RunService>();
	}
	else {
		m_refreshTimer.stop();
	}
}

void StatsReportControl::refreshReport()
{
	RBX::DataModel::Lock lock(m_dataModel);
	populate();
	viewport()->update();
}

void StatsReportControl::onMarshaledEvent(const RBX::Instance* source, RBX::ChildRemoved event)
{
	for (int i = static_cast<int>(m_records.size()) - 1; i >= 0; i--) {
		if (m_records[i]->item().get() == event.child.get()) {
			m_records.erase(m_records.begin() + i);
			break;
		}
	}
}

void StatsReportControl::onMarshaledEvent(const RBX::Instance* source, RBX::ChildAdded event)
{
	std::shared_ptr<StatsItemRecord> record =
		std::make_shared<StatsItemRecord>(std::static_pointer_cast<RBX::Stats::Item>(event.child), m_dataModel);
	record->setExpanded(true);
	m_records.push_back(record);
}

void StatsReportControl::synchronizeRows(
	QTreeWidgetItem* parent, const std::vector<std::shared_ptr<StatsItemRecord>>& records
)
{
	int index = 0;
	for (const std::shared_ptr<StatsItemRecord>& record : records) {
		while (index < parent->childCount() && static_cast<StatsItemRow*>(parent->child(index))->record() != record) {
			delete parent->takeChild(index);
		}

		if (index == parent->childCount()) {
			parent->addChild(new StatsItemRow(record));
		}

		QTreeWidgetItem* row = parent->child(index);
		synchronizeRows(row, record->children());
		if (row->childCount() > 0 && row->isExpanded() != record->isExpanded()) {
			row->setExpanded(record->isExpanded());
		}
		index++;
	}

	while (parent->childCount() > index) {
		delete parent->takeChild(index);
	}
}

void StatsReportControl::populate()
{
	std::shared_ptr<StatsItemRecord> focusedRecord;
	if (StatsItemRow* focusedRow = static_cast<StatsItemRow*>(currentItem())) {
		focusedRecord = focusedRow->record();
	}

	synchronizeRows(invisibleRootItem(), m_records);

	StatsItemRow* focusedRow = static_cast<StatsItemRow*>(currentItem());
	if (focusedRow && (!focusedRecord || focusedRow->record() != focusedRecord || !isRowVisible(focusedRow))) {
		setCurrentItem(nullptr);
		focusedRow = nullptr;
	}
	if (!focusedRow && topLevelItemCount() > 0) {
		focusedRow = static_cast<StatsItemRow*>(topLevelItem(0));
		setCurrentItem(focusedRow);
	}

	clearSelection();
	if (focusedRow) {
		focusedRow->setSelected(true);
	}

	viewport()->update();
}

std::vector<QTreeWidgetItem*> StatsReportControl::visibleRows() const
{
	std::vector<QTreeWidgetItem*> rows;
	for (QTreeWidgetItem* row = topLevelItem(0); row; row = itemBelow(row)) {
		rows.push_back(row);
	}
	return rows;
}

bool StatsReportControl::isRowVisible(QTreeWidgetItem* row) const
{
	for (QTreeWidgetItem* parent = row->parent(); parent; parent = parent->parent()) {
		if (!parent->isExpanded()) {
			return false;
		}
	}
	return true;
}

void StatsReportControl::expandVisibleRows()
{
	std::vector<QTreeWidgetItem*> rows = visibleRows();
	for (auto it = rows.rbegin(); it != rows.rend(); ++it) {
		if ((*it)->childCount() > 0) {
			(*it)->setExpanded(true);
		}
	}

	if (currentItem() && isRowVisible(currentItem())) {
		scrollToItem(currentItem());
	}
}

void StatsReportControl::collapseVisibleRows()
{
	std::vector<QTreeWidgetItem*> rows = visibleRows();
	for (auto it = rows.rbegin(); it != rows.rend(); ++it) {
		if ((*it)->childCount() > 0) {
			(*it)->setExpanded(false);
		}
	}

	if (currentItem() && isRowVisible(currentItem())) {
		scrollToItem(currentItem());
	}
}

void StatsReportControl::applyColumnSizing()
{
	int total = viewport()->width();
	if (total <= 0) {
		return;
	}
	int first = header()->logicalIndex(0);
	int last = header()->logicalIndex(1);
	int width =
		std::max(m_autoWidth[first] * total / std::max(1, m_autoWidth[first] + m_autoWidth[last]), minColumnWidth);
	m_sizingColumns = true;
	header()->resizeSection(first, width);
	header()->resizeSection(last, std::max(total - width, minColumnWidth));
	m_sizingColumns = false;
}

bool StatsReportControl::canCopy() const
{
	return !selectedItems().isEmpty();
}

void StatsReportControl::copy()
{
	QString text;
	for (QTreeWidgetItem* row : visibleRows()) {
		if (!row->isSelected()) {
			continue;
		}

		QStringList captions;
		for (int visualIndex = 0; visualIndex < header()->count(); visualIndex++) {
			captions.append(row->text(header()->logicalIndex(visualIndex)));
		}

		text += captions.join('\t');
		text += "\r\n";
	}

	QGuiApplication::clipboard()->setText(text);
}

bool StatsReportControl::removeRecord(
	std::vector<std::shared_ptr<StatsItemRecord>>& records, const std::shared_ptr<StatsItemRecord>& record
)
{
	auto found = std::find(records.begin(), records.end(), record);
	if (found != records.end()) {
		records.erase(found);
		return true;
	}

	return std::any_of(records.begin(), records.end(), [&record](const std::shared_ptr<StatsItemRecord>& parent) {
		return removeRecord(parent->children(), record);
	});
}

void StatsReportControl::cut()
{
	copy();

	std::vector<QTreeWidgetItem*> rows = visibleRows();
	std::vector<std::shared_ptr<StatsItemRecord>> selectedRecords;
	int firstSelectedRow = -1;
	for (int i = 0; i < static_cast<int>(rows.size()); i++) {
		if (!rows[i]->isSelected()) {
			continue;
		}

		if (firstSelectedRow == -1) {
			firstSelectedRow = i;
		}
		if (std::shared_ptr<StatsItemRecord> record = static_cast<StatsItemRow*>(rows[i])->record()) {
			selectedRecords.push_back(record);
		}
	}

	for (auto it = selectedRecords.rbegin(); it != selectedRecords.rend(); ++it) {
		removeRecord(m_records, *it);
	}

	synchronizeRows(invisibleRootItem(), m_records);
	setCurrentItem(nullptr);
	clearSelection();

	if (firstSelectedRow != -1) {
		rows = visibleRows();
		if (!rows.empty()) {
			setCurrentItem(rows[std::min(firstSelectedRow, static_cast<int>(rows.size()) - 1)]);
		}
	}

	viewport()->update();
}

void StatsReportControl::paintEvent(QPaintEvent* event)
{
	ReportControl::paintEvent(event);

	if (topLevelItemCount() == 0) {
		QPainter painter(viewport());
		painter.setPen(palette().color(QPalette::Text));
		painter.drawText(
			viewport()->rect().adjusted(noItemsTextMargin, noItemsTextMargin, -noItemsTextMargin, -noItemsTextMargin),
			Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap,
			QString::fromLatin1(noItemsText)
		);
	}
}

void StatsReportControl::resizeEvent(QResizeEvent* event)
{
	ReportControl::resizeEvent(event);
	applyColumnSizing();
}

void StatsReportControl::keyPressEvent(QKeyEvent* event)
{
	if (topLevelItemCount() == 0) {
		event->ignore();
		return;
	}

	QTreeWidgetItem* focusedRow = currentItem();
	bool hasChildren = focusedRow && focusedRow->childCount() > 0;
	bool selectBlock = event->modifiers() & Qt::ShiftModifier;
	bool ignoreSelection = event->modifiers() & Qt::ControlModifier;
	bool keypad = event->modifiers() & Qt::KeypadModifier;

	switch (event->key()) {
	case Qt::Key_Plus:
		if (!keypad) {
			event->ignore();
		}
		else if (ignoreSelection) {
			expandVisibleRows();
		}
		else if (hasChildren && !focusedRow->isExpanded()) {
			focusedRow->setExpanded(true);
		}
		break;
	case Qt::Key_Minus:
		if (!keypad) {
			event->ignore();
		}
		else if (ignoreSelection) {
			collapseVisibleRows();
		}
		else if (hasChildren && focusedRow->isExpanded()) {
			focusedRow->setExpanded(false);
		}
		break;
	case Qt::Key_Right:
		if (hasChildren && !focusedRow->isExpanded()) {
			focusedRow->setExpanded(true);
		}
		else {
			QKeyEvent moveDown(QEvent::KeyPress, Qt::Key_Down, event->modifiers());
			ReportControl::keyPressEvent(&moveDown);
		}
		break;
	case Qt::Key_Left:
		if (hasChildren && focusedRow->isExpanded()) {
			focusedRow->setExpanded(false);
		}
		else if (focusedRow && !hasChildren && focusedRow->parent() && !selectBlock) {
			setCurrentItem(focusedRow->parent());
		}
		else {
			QKeyEvent moveUp(QEvent::KeyPress, Qt::Key_Up, event->modifiers());
			ReportControl::keyPressEvent(&moveUp);
		}
		break;
	case Qt::Key_Return:
	case Qt::Key_Enter:
		if (hasChildren) {
			focusedRow->setExpanded(!focusedRow->isExpanded());
		}
		break;
	case Qt::Key_Space:
		if (ignoreSelection && focusedRow) {
			focusedRow->setSelected(!focusedRow->isSelected());
		}
		break;
	case Qt::Key_Down:
	case Qt::Key_Up:
	case Qt::Key_Home:
	case Qt::Key_End:
	case Qt::Key_PageDown:
	case Qt::Key_PageUp:
		ReportControl::keyPressEvent(event);
		break;
	default:
		event->ignore();
		break;
	}
}

ReportPane::ReportPane(QWidget* parent) : StatsReportControl(parent)
{
	setDataModel(std::shared_ptr<RBX::DataModel>());
	ActiveDocView::refresh();
}

void ReportPane::activeDataModelChanged(
	std::shared_ptr<RBX::DataModel> newDataModel, std::shared_ptr<RBX::DataModel> oldDataModel
)
{
	setDataModel(newDataModel);
}

CRobloxReportDocView::CRobloxReportDocView() : m_report(new StatsReportControl(this))
{
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(m_report);
	setFocusProxy(m_report);

	addCommand(ID_EDIT_CUT, [this]() { m_report->cut(); });
	addCommand(ID_EDIT_COPY, [this]() { m_report->copy(); });
	addCommandUpdate(ID_EDIT_CUT, [this](CommandState& state) { state.enabled = m_report->canCopy(); });
	addCommandUpdate(ID_EDIT_COPY, [this](CommandState& state) { state.enabled = m_report->canCopy(); });
}

void CRobloxReportDocView::initialUpdate()
{
	View::initialUpdate();

	if (document()) {
		m_report->setDataModel(static_cast<CRobloxDoc*>(document())->dataModel());
	}
	else {
		m_report->setDataModel(std::shared_ptr<RBX::DataModel>());
	}
}

void CRobloxReportDocView::activate(bool active, View* other)
{
	View::activate(active, other);

	if (active) {
		ActiveDocView::activate(true, static_cast<CRobloxDoc*>(document()));
	}
	else if (other != this) {
		ActiveDocView::activate(false, static_cast<CRobloxDoc*>(document()));
	}
}
