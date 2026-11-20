#ifndef ROBLOXREPORTVIEW_H
#define ROBLOXREPORTVIEW_H

#include "ActiveDocView.h"
#include "DocView.h"
#include "ReportControl.h"
#include "util/Events.h"
#include "util/object.h"
#include "v8datamodel/DataModel.h"
#include "v8datamodel/Stats.h"
#include "v8tree/Instance.h"

#include <QCoreApplication>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <memory>
#include <mutex>
#include <vector>

template <class Event>
class MarshaledInstanceListener : public RBX::Listener<RBX::Instance, Event>
{
public:
	MarshaledInstanceListener() : m_pendingEvents(std::make_shared<bool>(true)) {}

	void cancelPendingEvents()
	{
		std::scoped_lock lock(m_pendingEventsMutex);
		m_pendingEvents = std::make_shared<bool>(true);
	}

protected:
	void onEvent(const RBX::Instance* source, Event event) override
	{
		std::weak_ptr<bool> pendingEvents;
		{
			std::scoped_lock lock(m_pendingEventsMutex);
			pendingEvents = m_pendingEvents;
		}

		std::shared_ptr<const RBX::Instance> eventSource = RBX::shared_from(source);
		QMetaObject::invokeMethod(
			QCoreApplication::instance(),
			[this, pendingEvents, eventSource, event]() {
				if (pendingEvents.expired()) {
					return;
				}

				try {
					onMarshaledEvent(eventSource.get(), event);
				}
				catch (...) {
				}
			},
			Qt::QueuedConnection
		);
	}

	virtual void onMarshaledEvent(const RBX::Instance* source, Event event) = 0;

private:
	std::shared_ptr<bool> m_pendingEvents;
	std::mutex m_pendingEventsMutex;
};

class StatsItemRecord : public MarshaledInstanceListener<RBX::ChildAdded>,
						public MarshaledInstanceListener<RBX::ChildRemoved>
{
public:
	StatsItemRecord(std::shared_ptr<RBX::Stats::Item> item, std::shared_ptr<RBX::DataModel> dataModel);
	~StatsItemRecord() override;

	QString nameCaption() const;
	QString valueCaption() const;

	const std::shared_ptr<RBX::Stats::Item>& item() const { return m_item; }
	std::vector<std::shared_ptr<StatsItemRecord>>& children() { return m_children; }
	bool isExpanded() const { return m_expanded; }
	void setExpanded(bool expanded) { m_expanded = expanded; }

protected:
	void onMarshaledEvent(const RBX::Instance* source, RBX::ChildRemoved event) override;
	void onMarshaledEvent(const RBX::Instance* source, RBX::ChildAdded event) override;

private:
	std::shared_ptr<RBX::DataModel> m_dataModel;
	std::shared_ptr<RBX::Stats::Item> m_item;
	std::vector<std::shared_ptr<StatsItemRecord>> m_children;
	bool m_expanded = false;
};

class StatsItemRow : public QTreeWidgetItem
{
public:
	explicit StatsItemRow(const std::shared_ptr<StatsItemRecord>& record);

	std::shared_ptr<StatsItemRecord> record() const { return m_record.lock(); }

	QVariant data(int column, int role) const override;

private:
	std::weak_ptr<StatsItemRecord> m_record;
};

class StatsReportControl : public ReportControl,
						   public MarshaledInstanceListener<RBX::ChildAdded>,
						   public MarshaledInstanceListener<RBX::ChildRemoved>
{
public:
	explicit StatsReportControl(QWidget* parent = nullptr);
	~StatsReportControl() override;

	void setDataModel(std::shared_ptr<RBX::DataModel> dataModel);
	void refreshReport();
	void populate();

	bool canCopy() const;
	void copy();
	void cut();

protected:
	void onMarshaledEvent(const RBX::Instance* source, RBX::ChildRemoved event) override;
	void onMarshaledEvent(const RBX::Instance* source, RBX::ChildAdded event) override;

	void paintEvent(QPaintEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;
	void keyPressEvent(QKeyEvent* event) override;

private:
	void synchronizeRows(QTreeWidgetItem* parent, const std::vector<std::shared_ptr<StatsItemRecord>>& records);
	std::vector<QTreeWidgetItem*> visibleRows() const;
	bool isRowVisible(QTreeWidgetItem* row) const;
	void expandVisibleRows();
	void collapseVisibleRows();
	void applyColumnSizing();
	static bool removeRecord(
		std::vector<std::shared_ptr<StatsItemRecord>>& records, const std::shared_ptr<StatsItemRecord>& record
	);

	QTimer m_refreshTimer;
	std::shared_ptr<RBX::DataModel> m_dataModel;
	std::shared_ptr<RBX::Stats::StatsService> m_statsService;
	std::vector<std::shared_ptr<StatsItemRecord>> m_records;
	int m_autoWidth[2];
	bool m_sizingColumns = false;
};

class ReportPane : public StatsReportControl, public ActiveDocView
{
public:
	explicit ReportPane(QWidget* parent = nullptr);

protected:
	void activeDataModelChanged(
		std::shared_ptr<RBX::DataModel> newDataModel, std::shared_ptr<RBX::DataModel> oldDataModel
	) override;
};

class CRobloxReportDocView : public View
{
public:
	CRobloxReportDocView();

	void initialUpdate() override;
	void activate(bool active, View* other) override;

	StatsReportControl* reportControl() const { return m_report; }

private:
	StatsReportControl* m_report;
};

#endif // ROBLOXREPORTVIEW_H
