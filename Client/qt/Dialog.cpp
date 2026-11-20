#include "Dialog.h"

#include "NativeWindow.h"
#include "Resources.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPainter>
#include <QPushButton>
#include <QRadioButton>
#include <QStyledItemDelegate>

static const int baseUnitX = 6;
static const int baseUnitY = 13;
static const int editTextTopMargin = 1;
static const int listBoxTextInset = 2;
static const int comboBoxChromeHeight = 8;
static const int borderedEditTextInset = 3;
static const char* const rasterDialogTypeface = "MS Sans Serif";
static const char* const outlineDialogTypeface = "Microsoft Sans Serif";
static const char* const shellDialogTypeface = "MS Shell Dlg";

enum
{
	WS_VISIBLE = 0x10000000,
	WS_DISABLED = 0x08000000,
	WS_EX_RIGHT = 0x00001000,
	SS_TYPEMASK = 0x0000001f,
	SS_NOPREFIX = 0x00000080,
	SS_RIGHT = 0x00000002,
	SS_BLACKFRAME = 0x00000007,
	BS_TYPEMASK = 0x0000000f,
	BS_DEFPUSHBUTTON = 0x00000001,
	BS_AUTOCHECKBOX = 0x00000003,
	BS_GROUPBOX = 0x00000007,
	BS_AUTORADIOBUTTON = 0x00000009,
	ES_PASSWORD = 0x00000020,
	WS_GROUP = 0x00020000,
	WS_VSCROLL = 0x00200000,
	WS_BORDER = 0x00800000,
	DS_3DLOOK = 0x00000004,
	DS_SHELLFONT = 0x00000048,
};

namespace {

class ListBoxDelegate : public QStyledItemDelegate
{
public:
	using QStyledItemDelegate::QStyledItemDelegate;

	QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override
	{
		return QSize(option.rect.width(), option.fontMetrics.height());
	}

	void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
	{
		bool selected = option.state & QStyle::State_Selected;
		const QPalette& palette = option.palette;
		painter->fillRect(option.rect, selected ? palette.color(QPalette::Highlight) : palette.color(QPalette::Base));
		painter->setPen(selected ? palette.color(QPalette::HighlightedText) : palette.color(QPalette::Text));
		painter->setFont(option.font);
		painter->drawText(
			option.rect.adjusted(listBoxTextInset, 0, 0, 0),
			Qt::AlignLeft | Qt::AlignTop | Qt::TextSingleLine,
			index.data().toString()
		);
	}
};

} // namespace

static int scaleRounded(int number, int numerator, int denominator)
{
	long long value = static_cast<long long>(number) * numerator;
	return static_cast<int>((value + (value >= 0 ? denominator / 2 : -denominator / 2)) / denominator);
}

QRect CDialog::MapDialogRect(int x, int y, int cx, int cy)
{
	return QRect(
		scaleRounded(x, baseUnitX, 4),
		scaleRounded(y, baseUnitY, 8),
		scaleRounded(cx, baseUnitX, 4),
		scaleRounded(cy, baseUnitY, 8)
	);
}

CDialog::CDialog(unsigned nIDTemplate, QWidget* parent)
	: QDialog(
		  parent,
		  Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint |
			  Qt::WindowCloseButtonHint
	  ),
	  m_nIDTemplate(nIDTemplate), m_result(IDCANCEL)
{
	CreateControls();
}

CDialog::CDialog(unsigned nIDTemplate, QWidget* parent, Child child)
	: QDialog(parent), m_nIDTemplate(nIDTemplate), m_result(IDCANCEL)
{
	setWindowFlags(Qt::Widget);
	CreateControls();
}

CDialog::~CDialog() = default;

QString CDialog::caption() const
{
	const Resources::DialogTemplate* resource = Resources::dialog(m_nIDTemplate);
	return resource ? QString::fromLatin1(resource->caption) : QString();
}

void CDialog::CreateControls()
{
	const Resources::DialogTemplate* resource = Resources::dialog(m_nIDTemplate);
	if (resource == nullptr) {
		return;
	}

	setWindowTitle(QString::fromLatin1(resource->caption));
	QFont font(QString::fromLatin1(resource->typeface));
	if (font.family() == rasterDialogTypeface) {
		font.setFamily(QString::fromLatin1(outlineDialogTypeface));
		font.setStyleStrategy(QFont::NoAntialias);
	}
	else if (font.family() == shellDialogTypeface && (resource->style & DS_SHELLFONT) != DS_SHELLFONT) {
		font.setFamily(QString::fromLatin1(outlineDialogTypeface));
	}
	font.setPixelSize(scaleRounded(resource->pointSize, logicalDpiY(), 72));
	font.setKerning(false);
	setFont(font);
	setFixedSize(MapDialogRect(0, 0, resource->cx, resource->cy).size());

	QButtonGroup* radioGroup = nullptr;
	QLabel* prefixLabel = nullptr;
	for (const Resources::DialogControl& control : resource->controls) {
		QString text = QString::fromLatin1(control.text);
		QString className = QString::fromLatin1(control.className);
		QWidget* widget = nullptr;

		if (className == "Static") {
			if ((control.style & SS_TYPEMASK) == SS_BLACKFRAME) {
				QFrame* frame = new QFrame(this);
				frame->setFrameShape(QFrame::Box);
				widget = frame;
			}
			else {
				QLabel* label = new QLabel(text, this);
				if ((control.style & SS_TYPEMASK) == SS_RIGHT || (control.exStyle & WS_EX_RIGHT)) {
					label->setAlignment(Qt::AlignRight | Qt::AlignTop);
				}
				else {
					label->setAlignment(Qt::AlignLeft | Qt::AlignTop);
				}
				label->setWordWrap((control.style & SS_TYPEMASK) <= SS_RIGHT);
				widget = label;
				if (!(control.style & SS_NOPREFIX)) {
					prefixLabel = label;
				}
			}
		}
		else if (className == "Edit") {
			QLineEdit* edit = new QLineEdit(this);
			edit->setAlignment(Qt::AlignLeft | Qt::AlignTop);
			bool bordered = (control.style & WS_BORDER) && !(resource->style & DS_3DLOOK);
			edit->setTextMargins(bordered ? borderedEditTextInset : 0, editTextTopMargin, 0, 0);
			if (control.style & ES_PASSWORD) {
				edit->setEchoMode(QLineEdit::Password);
			}
			widget = edit;
		}
		else if (className == "Button") {
			switch (control.style & BS_TYPEMASK) {
			case BS_AUTOCHECKBOX:
				widget = new QCheckBox(text, this);
				break;
			case BS_GROUPBOX:
				widget = new QGroupBox(text, this);
				break;
			case BS_AUTORADIOBUTTON: {
				QRadioButton* radio = new QRadioButton(text, this);
				if ((control.style & WS_GROUP) || radioGroup == nullptr) {
					radioGroup = new QButtonGroup(this);
				}
				radioGroup->addButton(radio);
				widget = radio;
				break;
			}
			default: {
				QPushButton* button = new QPushButton(text, this);
				button->setAutoDefault(false);
				if ((control.style & BS_TYPEMASK) == BS_DEFPUSHBUTTON) {
					button->setDefault(true);
				}
				int id = control.id;
				connect(button, &QPushButton::clicked, this, [this, id]() { OnCommand(id); });
				widget = button;
				break;
			}
			}
		}

		else if (className == "ListBox") {
			QListWidget* list = new QListWidget(this);
			list->setVerticalScrollBarPolicy(
				(control.style & WS_VSCROLL) ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff
			);
			list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
			list->setItemDelegate(new ListBoxDelegate(list));
			list->setUniformItemSizes(true);
			if (control.style & WS_BORDER) {
				list->setFrameShadow(QFrame::Plain);
			}
			widget = list;
		}
		else if (className == "ComboBox") {
			widget = new QComboBox(this);
		}
		if (widget == nullptr) {
			continue;
		}

		if (prefixLabel && widget != prefixLabel) {
			prefixLabel->setBuddy(widget);
			prefixLabel = nullptr;
		}

		widget->setFont(font);
		QRect geometry = MapDialogRect(control.x, control.y, control.cx, control.cy);
		if (qobject_cast<QComboBox*>(widget)) {
			geometry.setHeight(QFontMetrics(font).height() + comboBoxChromeHeight);
		}
		widget->setGeometry(geometry);
		widget->setVisible(control.style & WS_VISIBLE);
		widget->setEnabled(!(control.style & WS_DISABLED));
		if (control.id != -1) {
			m_controls[control.id] = widget;
		}
	}
}

void CDialog::showEvent(QShowEvent* event)
{
	QDialog::showEvent(event);
	if (!isWindow()) {
		return;
	}
	removeWindowIcon(this);
	if (const Resources::DialogTemplate* resource = Resources::dialog(m_nIDTemplate)) {
		applyDialogTemplateStyle(this, resource->style, resource->exStyle);
	}
	if (m_framePosition) {
		moveWindowFrame(this, *m_framePosition);
	}
	else if (!testAttribute(Qt::WA_Moved)) {
		centerWindowOverOwner(this);
	}
}

void CDialog::setFramePosition(const QPoint& position)
{
	m_framePosition = position;
}

int CDialog::DoModal()
{
	OnInitDialog();
	exec();
	return m_result;
}

bool CDialog::OnInitDialog()
{
	return true;
}

void CDialog::EndDialog(int result)
{
	m_result = result;
	done(result);
}

void CDialog::OnOK()
{
	EndDialog(IDOK);
}

void CDialog::OnCancel()
{
	EndDialog(IDCANCEL);
}

void CDialog::OnCommand(int id)
{
	switch (id) {
	case IDOK:
		OnOK();
		break;
	case IDCANCEL:
		OnCancel();
		break;
	}
}

void CDialog::reject()
{
	OnCancel();
}

QWidget* CDialog::GetDlgItem(int id) const
{
	auto it = m_controls.find(id);
	return it != m_controls.end() ? it->second : nullptr;
}

void CDialog::ReplaceDlgItem(int id, QWidget* widget)
{
	QWidget* old = GetDlgItem(id);
	widget->setFont(old->font());
	widget->setGeometry(old->geometry());
	widget->setEnabled(old->isEnabled());
	QWidget::setTabOrder(old, widget);
	for (QLabel* label : findChildren<QLabel*>(Qt::FindDirectChildrenOnly)) {
		if (label->buddy() == old) {
			label->setBuddy(widget);
		}
	}
	m_controls[id] = widget;
	delete old;
}

QString CDialog::GetDlgItemText(int id) const
{
	if (QLineEdit* edit = qobject_cast<QLineEdit*>(GetDlgItem(id))) {
		return edit->text();
	}
	if (QLabel* label = qobject_cast<QLabel*>(GetDlgItem(id))) {
		return label->text();
	}
	return QString();
}

void CDialog::SetDlgItemText(int id, const QString& text)
{
	if (QLineEdit* edit = qobject_cast<QLineEdit*>(GetDlgItem(id))) {
		edit->setText(text);
	}
	else if (QLabel* label = qobject_cast<QLabel*>(GetDlgItem(id))) {
		label->setText(text);
	}
}
