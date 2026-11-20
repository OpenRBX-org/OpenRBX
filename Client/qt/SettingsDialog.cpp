#include "SettingsDialog.h"

#include "Resources.h"
#include "SplitterPane.h"
#include "SystemMessageBox.h"
#include "WhidbeyStyle.h"
#include "v8datamodel/GlobalSettings.h"
#include "v8datamodel/Selection.h"
#include "v8tree/Service.h"

#include <QHeaderView>
#include <QSignalBlocker>
#include <QSplitter>

static const unsigned settingsTemplateId = 122;
static const unsigned editSettingsWarningStringId = 136;
static const unsigned settingsSavedStringId = 142;
static const unsigned settingsResetStringId = 143;
static const int placeholderControlId = 1001;
static const int resetControlId = 1002;
static const int explorerWidth = 200;
static const int splitterBarWidth = 4;

void CSettingsPropGrid::createItem(
	const RBX::Reflection::ClassDescriptor* classDescriptor, const RBX::Reflection::PropertyDescriptor* descriptor
)
{
	if (descriptor->owner != RBX::Instance::classDescriptor()) {
		CPropGrid::createItem(classDescriptor, descriptor);
	}
}

CSettingsExplorer::CSettingsExplorer(QWidget* parent) : ReportControl(parent)
{
	setColumnCount(1);
	setHeaderLabel("Name");
	setColumnSortAllowed(true);
	header()->setStretchLastSection(true);
	connect(this, &QTreeWidget::itemSelectionChanged, this, [this]() { selectionChanged(); });
}

CSettingsExplorer::~CSettingsExplorer()
{
	setRoot(nullptr);
}

void CSettingsExplorer::setRoot(RBX::Instance* root)
{
	RBX::stopListening<RBX::Instance, RBX::DescendentAdded>(m_root, this);
	RBX::stopListening<RBX::Instance, RBX::DescendentRemoving>(m_root, this);

	clear();
	m_root = RBX::shared_from(root);

	RBX::startListening<RBX::Instance, RBX::DescendentAdded>(m_root, this);
	RBX::startListening<RBX::Instance, RBX::DescendentRemoving>(m_root, this);
}

void CSettingsExplorer::addRecord(std::shared_ptr<RBX::Instance> instance)
{
	QTreeWidgetItem* item = new QTreeWidgetItem(this);
	item->setText(0, QString::fromStdString(instance->getName()));
	item->setData(0, Qt::UserRole, QVariant::fromValue(static_cast<void*>(instance.get())));
	QSignalBlocker blocker(this);
	focusFirstRow();
}

void CSettingsExplorer::removeRecord(RBX::Instance* instance)
{
	for (int i = topLevelItemCount() - 1; i >= 0; i--) {
		if (topLevelItem(i)->data(0, Qt::UserRole).value<void*>() == instance) {
			delete takeTopLevelItem(i);
			break;
		}
	}
}

void CSettingsExplorer::onEvent(const RBX::Instance* source, RBX::DescendentAdded event)
{
	if (dynamic_cast<RBX::GlobalSettings::Item*>(event.instance.get())) {
		std::shared_ptr<RBX::Instance> instance = RBX::shared_from(event.instance.get());
		QMetaObject::invokeMethod(this, [this, instance]() { addRecord(instance); }, Qt::QueuedConnection);
	}
}

void CSettingsExplorer::onEvent(const RBX::Instance* source, RBX::DescendentRemoving event)
{
	if (dynamic_cast<RBX::GlobalSettings::Item*>(event.instance.get())) {
		RBX::Instance* instance = event.instance.get();
		QMetaObject::invokeMethod(this, [this, instance]() { removeRecord(instance); }, Qt::QueuedConnection);
	}
}

void CSettingsExplorer::selectionChanged()
{
	RBX::Selection* selection = RBX::ServiceProvider::create<RBX::Selection>(m_root.get());
	std::vector<RBX::Instance*> instances;
	for (QTreeWidgetItem* item : selectedItems()) {
		instances.push_back(static_cast<RBX::Instance*>(item->data(0, Qt::UserRole).value<void*>()));
	}

	selection->setSelection(instances.begin(), instances.end());
}

CSettingsDialog::CSettingsDialog(QWidget* parent) : CDialog(settingsTemplateId, parent)
{
}

void CSettingsDialog::show(QWidget* parent)
{
	if (SystemMessageBox::warning(
			parent,
			Resources::string(appTitleStringId),
			Resources::string(editSettingsWarningStringId),
			QMessageBox::Ok | QMessageBox::Cancel
		) == QMessageBox::Ok) {
		CSettingsDialog dialog(parent);

		if (dialog.DoModal() == IDOK) {
			SystemMessageBox::information(
				parent, Resources::string(appTitleStringId), Resources::string(settingsSavedStringId)
			);
			RBX::GlobalSettings::singleton()->saveState();
		}
	}
}

void CSettingsDialog::resetAllSettings()
{
	SystemMessageBox::information(this, Resources::string(appTitleStringId), Resources::string(settingsResetStringId));
	RBX::GlobalSettings::singleton()->eraseSettingsStore();
	EndDialog(IDCANCEL);
}

void CSettingsDialog::OnCommand(int id)
{
	if (id == resetControlId) {
		resetAllSettings();
		return;
	}

	CDialog::OnCommand(id);
}

bool CSettingsDialog::OnInitDialog()
{
	if (!CDialog::OnInitDialog()) {
		return false;
	}

	m_splitter = new QSplitter(Qt::Horizontal, this);
	m_splitter->setHandleWidth(splitterBarWidth);
	m_explorer = new CSettingsExplorer(nullptr);
	m_explorer->setFrameShape(QFrame::NoFrame);
	m_grid = new CSettingsPropGrid(nullptr);
	m_splitter->addWidget(new SplitterPane(m_explorer, QMargins(2, 2, 1, 2), true, m_splitter));
	m_splitter->addWidget(new SplitterPane(m_grid, QMargins(1, 2, 2, 2), true, m_splitter));

	m_explorer->setRoot(RBX::GlobalSettings::singleton().get());

	RBX::Selection* selection = RBX::ServiceProvider::create<RBX::Selection>(RBX::GlobalSettings::singleton().get());
	m_grid->setSelection(selection);

	m_splitter->setGeometry(GetDlgItem(placeholderControlId)->geometry());
	int explorerPane = explorerWidth + 3;
	m_splitter->setSizes({explorerPane, m_splitter->width() - explorerPane - splitterBarWidth});
	m_splitter->show();
	return true;
}
