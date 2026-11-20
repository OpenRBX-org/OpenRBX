#include "HtmlWindow.h"

#include "External.h"
#include "MainFrm.h"
#include "Roblox.h"
#include "reflection/member.h"
#include "reflection/property.h"
#include "reflection/reflection.h"
#include "util/Name.h"
#include "util/standardout.h"
#include "v8datamodel/DataModel.h"

#include <G3D/format.h>
#include <QCoreApplication>
#include <QJsonObject>
#include <QScreen>
#include <QVBoxLayout>
#include <algorithm>
#include <exception>
#include <functional>
#include <stdexcept>

char sDHTMLWindowService[] = "HtmlService";
char sDHTMLWindow[] = "HtmlWindow";

template const RBX::FactoryProduct<DHTMLWindowService, RBX::Instance, sDHTMLWindowService>::Creator
	RBX::FactoryProduct<DHTMLWindowService, RBX::Instance, sDHTMLWindowService>::creator;

static RBX::Reflection::BoundFuncDesc<DHTMLWindowService, std::shared_ptr<RBX::Instance>(int, int), 2> desc_newWindow(
	&DHTMLWindowService::newWindow,
	"NewWindow",
	"width",
	500,
	"height",
	300,
	RBX::Reflection::FunctionDescriptor::AnyCaller
);

static RBX::Reflection::BoundFuncDesc<DHTMLWindow, void(std::string), 1> desc_navigate(
	&DHTMLWindow::navigate, "Navigate", "url", "about:blank", RBX::Reflection::FunctionDescriptor::AnyCaller
);

static RBX::Reflection::BoundFuncDesc<DHTMLWindow, void(std::string), 1> desc_setBody(
	&DHTMLWindow::setBody, "SetBody", "html", RBX::Reflection::FunctionDescriptor::AnyCaller
);

static RBX::Reflection::BoundFuncDesc<DHTMLWindow, void(RBX::Lua::FunctionRef), 1> desc_setCallback(
	&DHTMLWindow::setCallback, "SetCallback", "function", RBX::Reflection::FunctionDescriptor::AnyCaller
);

static RBX::Reflection::BoundFuncDesc<DHTMLWindow, void(int, int), 2> desc_resize(
	&DHTMLWindow::resize, "Resize", "x", "y", RBX::Reflection::FunctionDescriptor::AnyCaller
);

static RBX::Reflection::BoundFuncDesc<DHTMLWindow, void(), 0> desc_show(
	&DHTMLWindow::show, "Show", RBX::Reflection::FunctionDescriptor::AnyCaller
);

static RBX::Reflection::BoundFuncDesc<DHTMLWindow, void(), 0> desc_close(
	&DHTMLWindow::close, "Close", RBX::Reflection::FunctionDescriptor::AnyCaller
);

static RBX::Reflection::BoundFuncDesc<DHTMLWindow, void(std::shared_ptr<RBX::Instance>, std::string, std::string), 3>
	desc_bind(
		&DHTMLWindow::bind,
		"Bind",
		"object",
		"property",
		"htmlElementId",
		RBX::Reflection::FunctionDescriptor::AnyCaller
	);

static RBX::Reflection::BoundFuncDesc<DHTMLWindow, void(), 0> desc_unbind(
	&DHTMLWindow::unbind, "Unbind", RBX::Reflection::FunctionDescriptor::AnyCaller
);

RBX::Reflection::SignalDesc<DHTMLWindow, void(std::string)> DHTMLWindow::event_documentComplete(
	"DocumentComplete", "url"
);

static void postToGuiThread(std::function<void()> function)
{
	QMetaObject::invokeMethod(
		QCoreApplication::instance(),
		[function = std::move(function)]() {
			try {
				function();
			}
			catch (std::exception& e) {
				RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, e);
			}
		},
		Qt::QueuedConnection
	);
}

CLuaHtmlView::Binder::PropBinding::PropBinding(
	std::shared_ptr<RBX::Instance> instance, const RBX::Reflection::PropertyDescriptor* descriptor, Binder* binder
)
	: m_instance(instance), m_descriptor(descriptor)
{
	m_connection = RBX::Instance::event_propertyChanged.connect(
		instance.get(),
		[descriptor, binder](const RBX::Reflection::PropertyDescriptor* changed) {
			if (descriptor == changed) {
				binder->postUpdate();
			}
		},
		10
	);
}

void CLuaHtmlView::Binder::PropBinding::setValue(const std::string& value)
{
	m_descriptor->setStringValue(m_instance.get(), value);
}

std::string CLuaHtmlView::Binder::PropBinding::value()
{
	return m_descriptor->getStringValue(m_instance.get());
}

CLuaHtmlView::Binder::Binder(CLuaHtmlView* view, const QString& element) : m_view(view), m_element(element)
{
	m_view->ExecuteScript(QStringLiteral(
							  "(function () { var name = %1;"
							  " var element = document.getElementById(name) || document.getElementsByName(name)[0];"
							  " element.onchange = function () { window.rbxHostEvent(\"change\", { element: name, "
							  "value: element.value }); };"
							  " })();"
	)
							  .arg(CHtmlView::quoteScriptString(element)));
}

CLuaHtmlView::Binder::~Binder()
{
	if (!m_view) {
		return;
	}
	m_view->ExecuteScript(QStringLiteral(
							  "(function () { var name = %1;"
							  " var element = document.getElementById(name) || document.getElementsByName(name)[0];"
							  " if (element) { element.onchange = null; } })();"
	)
							  .arg(CHtmlView::quoteScriptString(m_element)));
}

void CLuaHtmlView::Binder::updateElement()
{
	m_updatePosted = false;
	if (!m_view) {
		return;
	}

	std::string value;
	std::list<std::shared_ptr<PropBinding>>::const_iterator iter = m_bindings.begin();
	std::list<std::shared_ptr<PropBinding>>::const_iterator end = m_bindings.end();
	if (iter != end) {
		value = (*iter)->value();
		++iter;
	}

	while (iter != end) {
		if (value != (*iter)->value()) {
			value.clear();
			break;
		}

		++iter;
	}

	m_view->ExecuteScript(
		QStringLiteral(
			"(function () { var name = %1;"
			" var element = document.getElementById(name) || document.getElementsByName(name)[0];"
			" if (element) { element.value = %2; } })();"
		)
			.arg(CHtmlView::quoteScriptString(m_element), CHtmlView::quoteScriptString(QString::fromStdString(value)))
	);
}

void CLuaHtmlView::Binder::elementChanged(const std::string& value)
{
	std::for_each(m_bindings.begin(), m_bindings.end(), [&value](const std::shared_ptr<PropBinding>& binding) {
		binding->setValue(value);
	});
}

void CLuaHtmlView::Binder::postUpdate()
{
	if (!m_updatePosted) {
		postToGuiThread([self = shared_from_this()] { self->updateElement(); });
		m_updatePosted = true;
	}
}

void CLuaHtmlView::Binder::bind(
	std::shared_ptr<RBX::Instance> instance, const RBX::Reflection::PropertyDescriptor* descriptor
)
{
	m_bindings.push_back(std::make_shared<PropBinding>(instance, descriptor, this));
	updateElement();
}

CLuaHtmlView::CLuaHtmlView(QWidget* parent) : CHtmlView(parent)
{
	SetScriptsEnabled(false);
}

CLuaHtmlView::~CLuaHtmlView()
{
	if (m_window) {
		RBX::DataModel::Lock lock(m_dataModel);
		m_binders.clear();
		m_window->m_view = nullptr;
		m_window->setParent(nullptr);
	}
}

void CLuaHtmlView::closeFrame()
{
	window()->deleteLater();
}

std::shared_ptr<CExternal> CLuaHtmlView::OnGetExternal()
{
	return m_function;
}

void CLuaHtmlView::setBody(const QString& html)
{
	if (!m_documentLoaded) {
		throw std::runtime_error("The HtmlDocument is not loaded");
	}

	ExecuteScript(QStringLiteral("document.body.innerHTML = %1;").arg(CHtmlView::quoteScriptString(html)));
}

void CLuaHtmlView::setWindow(std::shared_ptr<DHTMLWindow> window)
{
	m_window = window;
	m_dataModel = RBX::shared_from_dynamic_cast<RBX::DataModel, RBX::Instance>(window->getRootAncestor());

	m_function = std::make_shared<CLuaFunction>();
	m_function->dataModel = m_dataModel;
}

void CLuaHtmlView::setCallback(RBX::Lua::FunctionRef function)
{
	m_function->function = function;
}

void CLuaHtmlView::unbind()
{
	RBX::DataModel::Lock lock(m_dataModel);
	m_binders.clear();
}

void CLuaHtmlView::bind(
	std::shared_ptr<RBX::Instance> instance, const RBX::Reflection::PropertyDescriptor* descriptor, const QString& name
)
{
	if (!m_documentLoaded) {
		throw std::runtime_error("The HtmlDocument is not loaded");
	}

	RBX::DataModel::Lock lock(m_dataModel);
	Binder* binder = nullptr;
	for (const std::shared_ptr<Binder>& existing : m_binders) {
		if (existing->element() == name) {
			binder = existing.get();
			break;
		}
	}

	if (binder == nullptr) {
		m_binders.push_back(std::make_shared<Binder>(this, name));
		binder = m_binders.back().get();
	}

	binder->bind(instance, descriptor);
}

void CLuaHtmlView::OnDocumentComplete(const QString& url)
{
	CHtmlView::OnDocumentComplete(url);

	m_documentLoaded = true;
	InstallBridge();

	if (m_window) {
		RBX::DataModel::Lock lock(m_dataModel);
		DHTMLWindow::event_documentComplete.fire(m_window.get(), url.toStdString());
	}
}

void CLuaHtmlView::OnHostEvent(const QString& name, const QJsonValue& value)
{
	if (name != "change") {
		return;
	}

	QString element = value["element"].toString();
	std::string text = value["value"].toString().toStdString();
	for (const std::shared_ptr<Binder>& binder : m_binders) {
		if (binder->element() == element) {
			try {
				binder->elementChanged(text);
			}
			catch (std::exception& e) {
				RBX::StandardOut::singleton()->print(RBX::MESSAGE_WARNING, e);
			}
			break;
		}
	}
}

DHTMLWindowService::DHTMLWindowService()
{
	setName("WindowService");
}

DHTMLWindow::DHTMLWindow()
{
	setName("Window");
}

void DHTMLWindow::createView(int width, int height)
{
	QWidget* frame = new QWidget(theApp.mainFrame(), Qt::Tool);
	frame->setAttribute(Qt::WA_DeleteOnClose);
	frame->resize(width, height);
	QVBoxLayout* layout = new QVBoxLayout(frame);
	layout->setContentsMargins(0, 0, 0, 0);

	CLuaHtmlView* view = new CLuaHtmlView(frame);
	layout->addWidget(view);
	{
		std::scoped_lock lock(m_mutex);
		m_view = view;
	}
	view->create();
	view->initialUpdate();
}

void DHTMLWindow::onServiceProvider(const RBX::ServiceProvider* oldProvider, const RBX::ServiceProvider* newProvider)
{
	RBX::Instance::onServiceProvider(oldProvider, newProvider);

	if (oldProvider) {
		postToGuiThread([self = RBX::shared_from(this)] {
			self->withView([](CLuaHtmlView* view) { view->closeFrame(); });
		});
	}

	std::scoped_lock lock(m_mutex);
	if (newProvider) {
		postToGuiThread([self = RBX::shared_from(this)] {
			self->withView([self](CLuaHtmlView* view) { view->setWindow(self); });
		});
	}
}

void DHTMLWindow::show()
{
	postToGuiThread([self = RBX::shared_from(this)] {
		self->withView([](CLuaHtmlView* view) {
			QWidget* frame = view->window();
			QRect screen = frame->screen()->availableGeometry();
			frame->move(
				screen.center() - QPoint(frame->frameGeometry().width() / 2, frame->frameGeometry().height() / 2)
			);
			frame->show();
		});
	});
}

std::shared_ptr<RBX::Instance> DHTMLWindowService::newWindow(int width, int height)
{
	std::shared_ptr<DHTMLWindow> window = RBX::Creatable<RBX::Instance>::create<DHTMLWindow>();
	postToGuiThread([window, width, height] { window->createView(width, height); });
	window->setParent(this);
	return window;
}

void DHTMLWindow::resize(int x, int y)
{
	postToGuiThread([self = RBX::shared_from(this), x, y] {
		self->withView([x, y](CLuaHtmlView* view) { view->window()->resize(x, y); });
	});
}

void DHTMLWindow::setCallback(RBX::Lua::FunctionRef function)
{
	postToGuiThread([self = RBX::shared_from(this), function] {
		self->withView([&function](CLuaHtmlView* view) { view->setCallback(function); });
	});
}

void DHTMLWindow::navigate(std::string url)
{
	postToGuiThread([self = RBX::shared_from(this), url] {
		self->withView([&url](CLuaHtmlView* view) { view->Navigate(QString::fromStdString(url)); });
	});
}

void DHTMLWindow::setBody(std::string html)
{
	postToGuiThread([self = RBX::shared_from(this), html] {
		self->withView([&html](CLuaHtmlView* view) { view->setBody(QString::fromStdString(html)); });
	});
}

void DHTMLWindow::bind(std::shared_ptr<RBX::Instance> instance, std::string property, std::string name)
{
	if (!instance) {
		throw std::runtime_error("The object to bind to must not be nil");
	}

	RBX::Reflection::MemberDescriptorContainer<RBX::Reflection::PropertyDescriptor>::Iterator iter =
		instance->findProperty(RBX::Name::lookup(property));
	if (iter == instance->properties_end()) {
		throw std::runtime_error(G3D::format("Instance does not have property %s", property.c_str()));
	}

	const RBX::Reflection::PropertyDescriptor* descriptor = &(*iter).getDescriptor();
	postToGuiThread([self = RBX::shared_from(this), instance, descriptor, name] {
		self->withView([&](CLuaHtmlView* view) { view->bind(instance, descriptor, QString::fromStdString(name)); });
	});
}

void DHTMLWindow::unbind()
{
	postToGuiThread([self = RBX::shared_from(this)] { self->withView([](CLuaHtmlView* view) { view->unbind(); }); });
}
