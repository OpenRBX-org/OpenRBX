#include "HtmlView.h"

#include "External.h"
#include "InstanceExplorer.h"
#include "Resources.h"
#include "WhidbeyStyle.h"

#include <QDesktopServices>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QResizeEvent>
#include <QUrl>
#include <webview/webview.h>

#ifdef _WIN32
#include <shlwapi.h>
#include <wrl.h>

using Microsoft::WRL::Callback;
using Microsoft::WRL::ComPtr;
#else
#include <QAbstractNativeEventFilter>
#include <QApplication>
#include <QPlatformSurfaceEvent>
#include <QWindow>
#include <gtk/gtkx.h>
#include <map>
#include <xcb/xcb.h>
#endif

static const char* const bridgeScript = R"js((function () {
	var bridge = "%1";
	var systemColors = %2;
	function invoke(request) {
		var xhr = new XMLHttpRequest();
		xhr.open("POST", bridge, false);
		xhr.setRequestHeader("Content-Type", "text/plain");
		xhr.send(JSON.stringify(request));
		var response = JSON.parse(xhr.responseText);
		if (response.error !== undefined) {
			throw new Error(response.error);
		}
		return decode(response.value);
	}
	function decode(value) {
		if (value !== null && typeof value === "object") {
			if (value.__rbxobject !== undefined) {
				return wrap(value.__rbxobject, value.members);
			}
			if (value.__rbxsafearray !== undefined) {
				return { __rbxsafearray: value.__rbxsafearray.map(decode) };
			}
		}
		return value;
	}
	function wrap(id, members) {
		var object = {};
		Object.keys(members).forEach(function (name) {
			if (members[name] === "method") {
				object[name] = function () {
					return invoke({ kind: "call", object: id, name: name, args: Array.prototype.slice.call(arguments) });
				};
			} else {
				Object.defineProperty(object, name, {
					get: function () {
						return invoke({ kind: "get", object: id, name: name });
					}
				});
			}
		});
		return object;
	}
	window.VBArray = function (array) {
		this.items = array.__rbxsafearray;
	};
	window.VBArray.prototype.toArray = function () {
		return this.items.slice();
	};
	window.VBArray.prototype.getItem = function (index) {
		return this.items[index];
	};
	window.VBArray.prototype.lbound = function () {
		return 0;
	};
	window.VBArray.prototype.ubound = function () {
		return this.items.length - 1;
	};
	window.VBArray.prototype.dimensions = function () {
		return 1;
	};
	var root = null;
	Object.defineProperty(window, "external", {
		configurable: true,
		get: function () {
			if (root === null) {
				root = wrap(0, invoke({ kind: "members", object: 0 }));
			}
			return root;
		}
	});
	window.rbxHostEvent = function (name, value) {
		invoke({ kind: "event", object: 0, name: name, value: value });
	};
	window.close = function () {
		var element = document.getElementById("DialogResult");
		invoke({ kind: "close", object: 0, result: element ? String(element.value) : "" });
	};
	document.addEventListener("DOMContentLoaded", function () {
		var style = document.createElement("style");
		var chevron = function (points, color) {
			return "url(\"data:image/svg+xml," + encodeURIComponent(
				"<svg xmlns='http://www.w3.org/2000/svg' width='17' height='35'><polyline points='" + points +
				"' fill='none' stroke='" + color + "' stroke-width='2'/></svg>") + "\")";
		};
		var up = "3.5,25.5 8.5,20.5 13.5,25.5";
		var down = "3.5,9.5 8.5,14.5 13.5,9.5";
		style.textContent = "body { margin: 15px 10px; } p { margin: 19px 0; } p:first-child { margin-top: 0; } " +
			"a { color: #0066CC; } textarea { overflow-y: scroll; resize: none; box-sizing: border-box; " +
			"vertical-align: bottom; border: 1px solid #ABADB3; } " +
			"button, input[type=button], input[type=submit] { background: #E1E1E1; border: 1px solid #ADADAD; " +
			"border-radius: 0; color: #000000; } " +
			"button:hover, input[type=button]:hover, input[type=submit]:hover { background: #E5F1FB; " +
			"border-color: #0078D7; } " +
			"::-webkit-scrollbar { width: 17px; height: 17px; background: #F0F0F0; } " +
			"::-webkit-scrollbar-thumb { background: #CDCDCD; border: 1px solid #F0F0F0; } " +
			"::-webkit-scrollbar-thumb:hover { background: #A6A6A6; } " +
			"::-webkit-scrollbar-thumb:active { background: #606060; } " +
			"::-webkit-scrollbar-button:vertical { height: 35px; background-color: #F0F0F0; " +
			"background-repeat: no-repeat; } " +
			"::-webkit-scrollbar-button:vertical:start:increment, " +
			"::-webkit-scrollbar-button:vertical:end:decrement { display: none; } " +
			"::-webkit-scrollbar-button:vertical:decrement { background-image: " + chevron(up, "#606060") + "; } " +
			"::-webkit-scrollbar-button:vertical:increment { background-image: " + chevron(down, "#606060") + "; } " +
			"::-webkit-scrollbar-button:vertical:hover { background-color: #DADADA; }";
		document.head.insertBefore(style, document.head.firstChild);

		var elements = document.querySelectorAll("[bgcolor]");
		for (var i = 0; i < elements.length; i++) {
			var color = elements[i].getAttribute("bgcolor");
			var systemColor = systemColors[color.toLowerCase()];
			if (systemColor) {
				elements[i].style.backgroundColor = systemColor;
			}
			else if (CSS.supports("background-color", color)) {
				elements[i].style.backgroundColor = color;
			}
		}
	});
})();)js";

#ifdef _WIN32
static const wchar_t* const bridgeUrl = L"https://rbx.external/";

static QString fromWide(LPWSTR text)
{
	QString result = QString::fromWCharArray(text);
	CoTaskMemFree(text);
	return result;
}

struct CHtmlView::Engine
{
	webview_t webview = nullptr;
	ComPtr<ICoreWebView2Controller> controller;
	ComPtr<ICoreWebView2> core;
	ComPtr<ICoreWebView2Environment> environment;
	HWND widget = nullptr;
};
#else
static const char* const bridgeUrl = "rbx-external://bridge/";

enum
{
	XEMBED_FOCUS_CURRENT = 0,
	XEMBED_WINDOW_ACTIVATE = 1,
	XEMBED_WINDOW_DEACTIVATE = 2,
	XEMBED_REQUEST_FOCUS = 3,
	XEMBED_FOCUS_IN = 4,
	XEMBED_FOCUS_OUT = 5,
};

static Display* gdkDisplay()
{
	return GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
}

static Atom xembedAtom()
{
	static Atom atom = XInternAtom(gdkDisplay(), "_XEMBED", False);
	return atom;
}

struct CHtmlView::Engine
{
	webview_t webview = nullptr;
	GtkWidget* plug = nullptr;
	WebKitWebView* view = nullptr;
	Window plugWindow = 0;

	static std::map<WebKitWebView*, CHtmlView*>& views()
	{
		static std::map<WebKitWebView*, CHtmlView*> registry;
		return registry;
	}

	static CHtmlView* find(WebKitWebView* view)
	{
		auto it = views().find(view);
		return it != views().end() ? it->second : nullptr;
	}

	void send(long message, long detail = 0)
	{
		XEvent event = {};
		event.xclient.type = ClientMessage;
		event.xclient.window = plugWindow;
		event.xclient.message_type = xembedAtom();
		event.xclient.format = 32;
		event.xclient.data.l[0] = CurrentTime;
		event.xclient.data.l[1] = message;
		event.xclient.data.l[2] = detail;
		XSendEvent(gdkDisplay(), plugWindow, False, NoEventMask, &event);
		XFlush(gdkDisplay());
	}

	void resize(int width, int height)
	{
		XResizeWindow(gdkDisplay(), plugWindow, std::max(1, width), std::max(1, height));
		XFlush(gdkDisplay());
	}

	static void bridgeRequest(WebKitURISchemeRequest* request, gpointer)
	{
		QByteArray body;
		if (GInputStream* input = webkit_uri_scheme_request_get_http_body(request)) {
			char buffer[4096];
			gssize read;
			while ((read = g_input_stream_read(input, buffer, sizeof(buffer), nullptr, nullptr)) > 0) {
				body.append(buffer, read);
			}
			g_object_unref(input);
		}

		CHtmlView* owner = find(webkit_uri_scheme_request_get_web_view(request));
		QByteArray reply = owner ? owner->OnBridgeRequest(body) : QByteArray("{\"error\":\"no view\"}");
		GBytes* bytes = g_bytes_new(reply.constData(), reply.size());
		GInputStream* stream = g_memory_input_stream_new_from_bytes(bytes);
		WebKitURISchemeResponse* response = webkit_uri_scheme_response_new(stream, reply.size());
		webkit_uri_scheme_response_set_status(response, 200, "OK");
		webkit_uri_scheme_response_set_content_type(response, "application/json");
		SoupMessageHeaders* headers = soup_message_headers_new(SOUP_MESSAGE_HEADERS_RESPONSE);
		soup_message_headers_append(headers, "Access-Control-Allow-Origin", "*");
		webkit_uri_scheme_response_set_http_headers(response, headers);
		webkit_uri_scheme_request_finish_with_response(request, response);
		g_object_unref(response);
		g_object_unref(stream);
		g_bytes_unref(bytes);
	}

	static gboolean decidePolicy(
		WebKitWebView* view, WebKitPolicyDecision* decision, WebKitPolicyDecisionType type, gpointer
	)
	{
		CHtmlView* owner = find(view);
		if (!owner) {
			return FALSE;
		}

		if (type == WEBKIT_POLICY_DECISION_TYPE_NEW_WINDOW_ACTION) {
			WebKitURIRequest* request =
				webkit_navigation_policy_decision_get_request(WEBKIT_NAVIGATION_POLICY_DECISION(decision));
			owner->Navigate(QString::fromUtf8(webkit_uri_request_get_uri(request)), QStringLiteral("_blank"));
			webkit_policy_decision_ignore(decision);
			return TRUE;
		}

		if (type == WEBKIT_POLICY_DECISION_TYPE_RESPONSE) {
			WebKitResponsePolicyDecision* response = WEBKIT_RESPONSE_POLICY_DECISION(decision);
			if (webkit_response_policy_decision_is_main_frame_main_resource(response)) {
				return FALSE;
			}

			WebKitURIRequest* request = webkit_response_policy_decision_get_request(response);
			bool cancel = false;
			owner->OnBeforeNavigate2(
				QString::fromUtf8(webkit_uri_request_get_uri(request)), QStringLiteral("_frame"), &cancel
			);
			if (cancel) {
				webkit_policy_decision_ignore(decision);
				return TRUE;
			}
		}
		return FALSE;
	}

	static void loadChanged(WebKitWebView* view, WebKitLoadEvent event, gpointer)
	{
		CHtmlView* owner = find(view);
		if (!owner) {
			return;
		}

		QString url = QString::fromUtf8(webkit_web_view_get_uri(view));
		if (event == WEBKIT_LOAD_STARTED) {
			if (url.startsWith("data:")) {
				return;
			}

			bool cancel = false;
			owner->OnBeforeNavigate2(url, QString(), &cancel);
			if (cancel) {
				webkit_web_view_stop_loading(view);
			}
			else {
				owner->m_busy = true;
			}
		}
		else if (event == WEBKIT_LOAD_FINISHED) {
			owner->m_locationUrl = url;
			owner->m_busy = false;
			owner->OnNavigateComplete2(url);
			owner->OnDocumentComplete(url);
		}
	}

	static void titleChanged(WebKitWebView* view, GParamSpec*, gpointer)
	{
		if (CHtmlView* owner = find(view)) {
			owner->OnTitleChange(QString::fromUtf8(webkit_web_view_get_title(view)));
		}
	}

	static void historyChanged(WebKitBackForwardList*, WebKitBackForwardListItem*, gpointer, gpointer data)
	{
		WebKitWebView* view = static_cast<WebKitWebView*>(data);
		if (CHtmlView* owner = find(view)) {
			owner->OnCommandStateChange(CSC_NAVIGATEBACK, webkit_web_view_can_go_back(view));
			owner->OnCommandStateChange(CSC_NAVIGATEFORWARD, webkit_web_view_can_go_forward(view));
			owner->OnCommandStateChange(CSC_UPDATECOMMANDS, true);
		}
	}

	static void closeRequested(WebKitWebView* view, gpointer)
	{
		if (CHtmlView* owner = find(view)) {
			owner->OnWindowClosing(QString());
		}
	}

	static CHtmlView* findHost(xcb_window_t window)
	{
		for (auto& [view, owner] : views()) {
			if (owner->m_host->winId() == window) {
				return owner;
			}
		}
		return nullptr;
	}

	static CHtmlView* findFocused()
	{
		QWidget* focus = QApplication::focusWidget();
		for (auto& [view, owner] : views()) {
			if (owner->m_host == focus) {
				return owner;
			}
		}
		return nullptr;
	}

	static void forwardKey(CHtmlView* owner, const xcb_key_press_event_t* key, bool press)
	{
		Window target = owner->m_engine->plugWindow;
		XEvent forwarded = {};
		forwarded.xkey.type = press ? KeyPress : KeyRelease;
		forwarded.xkey.display = gdkDisplay();
		forwarded.xkey.window = target;
		forwarded.xkey.root = key->root;
		forwarded.xkey.time = key->time;
		forwarded.xkey.x = key->event_x;
		forwarded.xkey.y = key->event_y;
		forwarded.xkey.x_root = key->root_x;
		forwarded.xkey.y_root = key->root_y;
		forwarded.xkey.state = key->state;
		forwarded.xkey.keycode = key->detail;
		forwarded.xkey.same_screen = True;
		XSendEvent(gdkDisplay(), target, False, NoEventMask, &forwarded);
		XFlush(gdkDisplay());
	}
};

class XEmbedFilter : public QAbstractNativeEventFilter
{
public:
	bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override
	{
		xcb_generic_event_t* event = static_cast<xcb_generic_event_t*>(message);
		unsigned type = event->response_type & ~0x80;
		if (type == XCB_CLIENT_MESSAGE) {
			xcb_client_message_event_t* client = reinterpret_cast<xcb_client_message_event_t*>(event);
			if (client->type != xembedAtom() || client->data.data32[1] != XEMBED_REQUEST_FOCUS) {
				return false;
			}

			CHtmlView* owner = CHtmlView::Engine::findHost(client->window);
			if (owner) {
				owner->m_host->setFocus(Qt::MouseFocusReason);
				owner->OnBrowserFocus();
			}
			return owner != nullptr;
		}

		if (type == XCB_KEY_PRESS || type == XCB_KEY_RELEASE) {
			CHtmlView* owner = CHtmlView::Engine::findFocused();
			if (owner) {
				CHtmlView::Engine::forwardKey(
					owner, reinterpret_cast<xcb_key_press_event_t*>(event), type == XCB_KEY_PRESS
				);
			}
			return owner != nullptr;
		}
		return false;
	}
};
#endif

QString CHtmlView::quoteScriptString(const QString& text)
{
	QString json = QString::fromUtf8(QJsonDocument(QJsonArray{text}).toJson(QJsonDocument::Compact));
	return json.mid(1, json.size() - 2);
}

static QString systemColorTable()
{
	const WhidbeyColors& c = WhidbeyStyle::colors();
	const std::pair<const char*, QColor> colors[] = {
		{"activecaption", c.activeCaption},
		{"buttonface", c.face},
		{"buttonhighlight", c.buttonHighlight},
		{"buttonshadow", c.shadow},
		{"buttontext", c.buttonText},
		{"captiontext", c.captionText},
		{"graytext", c.grayText},
		{"highlight", c.highlight},
		{"highlighttext", c.highlightText},
		{"inactivecaption", c.inactiveCaption},
		{"inactivecaptiontext", c.inactiveCaptionText},
		{"infobackground", c.infoBackground},
		{"infotext", c.infoText},
		{"menutext", c.menuText},
		{"threeddarkshadow", c.darkShadow},
		{"threedface", c.face},
		{"threedhighlight", c.buttonHighlight},
		{"threedlightshadow", c.buttonLight},
		{"threedshadow", c.shadow},
		{"window", c.window},
		{"windowtext", c.windowText},
	};
	QJsonObject table;
	for (const auto& [name, color] : colors) {
		table.insert(QLatin1String(name), color.name());
	}
	return QString::fromUtf8(QJsonDocument(table).toJson(QJsonDocument::Compact));
}

static QString bridgeInitScript()
{
#ifdef _WIN32
	return QString::fromLatin1(bridgeScript).arg(QString::fromWCharArray(bridgeUrl), systemColorTable());
#else
	return QString::fromLatin1(bridgeScript).arg(QString::fromLatin1(bridgeUrl), systemColorTable());
#endif
}

CHtmlView::CHtmlView(QWidget* parent) : View(parent), m_host(new QWidget(this)), m_busy(false), m_customGeometry(false)
{
	m_host->setAttribute(Qt::WA_NativeWindow);
	m_host->setBackgroundRole(QPalette::Base);
	m_host->setAutoFillBackground(true);
}

CHtmlView::~CHtmlView()
{
#ifdef _WIN32
	if (m_engine && m_engine->webview) {
		webview_destroy(m_engine->webview);
	}
#else
	destroyBrowser();
#endif
}

#ifndef _WIN32
void CHtmlView::destroyBrowser()
{
	if (!m_engine || !m_engine->webview) {
		return;
	}
	Engine::views().erase(m_engine->view);
	webview_destroy(m_engine->webview);
	gtk_widget_destroy(m_engine->plug);
	XSync(gdkDisplay(), False);
	m_engine->webview = nullptr;
	m_engine->view = nullptr;
	m_engine->plug = nullptr;
	m_engine->plugWindow = 0;
	m_ready = false;
}

bool CHtmlView::eventFilter(QObject* watched, QEvent* event)
{
	if (event->type() == QEvent::PlatformSurface && watched == m_host->windowHandle() &&
		static_cast<QPlatformSurfaceEvent*>(event)->surfaceEventType() ==
			QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed) {
		destroyBrowser();
	}
	return View::eventFilter(watched, event);
}
#endif

void CHtmlView::SetBrowserGeometry(const QRect& rect)
{
	m_customGeometry = true;
	m_host->setGeometry(rect);
#ifdef _WIN32
	if (m_engine && m_engine->widget) {
		MoveWindow(m_engine->widget, 0, 0, rect.width(), rect.height(), TRUE);
	}
#else
	if (m_engine && m_engine->plugWindow) {
		m_engine->resize(rect.width(), rect.height());
	}
#endif
}

void CHtmlView::resizeEvent(QResizeEvent* event)
{
	View::resizeEvent(event);
	if (!m_customGeometry) {
		m_host->setGeometry(rect());
#ifdef _WIN32
		if (m_engine && m_engine->widget) {
			MoveWindow(m_engine->widget, 0, 0, width(), height(), TRUE);
		}
#else
		if (m_engine && m_engine->plugWindow) {
			m_engine->resize(width(), height());
		}
#endif
	}
}

std::shared_ptr<CExternal> CHtmlView::OnGetExternal()
{
	return nullptr;
}

void CHtmlView::OnBeforeNavigate2(const QString& url, const QString& targetFrameName, bool* cancel)
{
	if (!isTrustedUrl(url.toStdString().c_str())) {
		*cancel = true;
	}
}

void CHtmlView::OnNavigateComplete2(const QString& url)
{
}

void CHtmlView::OnDocumentComplete(const QString& url)
{
}

void CHtmlView::OnTitleChange(const QString& text)
{
}

void CHtmlView::OnCommandStateChange(int command, bool enable)
{
}

void CHtmlView::OnWindowClosing(const QString& dialogResult)
{
}

void CHtmlView::OnBrowserFocus()
{
}

void CHtmlView::OnHostEvent(const QString& name, const QJsonValue& value)
{
}

void CHtmlView::SetScriptsEnabled(bool enabled)
{
	m_scriptsEnabled = enabled;
#ifdef _WIN32
	if (m_engine && m_engine->core) {
		ComPtr<ICoreWebView2Settings> settings;
		if (SUCCEEDED(m_engine->core->get_Settings(&settings))) {
			settings->put_IsScriptEnabled(enabled);
		}
	}
#else
	if (m_engine && m_engine->view) {
		webkit_settings_set_enable_javascript(webkit_web_view_get_settings(m_engine->view), enabled);
	}
#endif
}

void CHtmlView::InstallBridge()
{
	ExecuteScript(bridgeInitScript());
}

void CHtmlView::ResetExternal()
{
	m_externalObjects.reset();
}

QByteArray CHtmlView::OnBridgeRequest(const QByteArray& request)
{
	QJsonObject call = QJsonDocument::fromJson(request).object();
	if (call["kind"].toString() == "event") {
		QString name = call["name"].toString();
		QJsonValue value = call["value"];
		QMetaObject::invokeMethod(this, [this, name, value]() { OnHostEvent(name, value); }, Qt::QueuedConnection);
		return "{\"value\":null}";
	}

	if (call["kind"].toString() == "close") {
		QString result = call["result"].toString();
		QMetaObject::invokeMethod(this, [this, result]() { OnWindowClosing(result); }, Qt::QueuedConnection);
		return "{\"value\":null}";
	}

	if (!m_externalObjects) {
		std::shared_ptr<CExternal> external = OnGetExternal();
		if (!external) {
			return "{\"error\":\"window.external is not available\"}";
		}
		m_externalObjects.reset(new CExternalObjects(external));
	}
	return m_externalObjects->Dispatch(request, m_locationUrl);
}

bool CHtmlView::create()
{
	m_engine.reset(new Engine);

#ifdef _WIN32
	HWND host = reinterpret_cast<HWND>(m_host->winId());
	m_engine->webview = webview_create(0, host);
	if (m_engine->webview == nullptr) {
		return false;
	}

	m_engine->widget =
		static_cast<HWND>(webview_get_native_handle(m_engine->webview, WEBVIEW_NATIVE_HANDLE_KIND_UI_WIDGET));
	m_engine->controller = static_cast<ICoreWebView2Controller*>(
		webview_get_native_handle(m_engine->webview, WEBVIEW_NATIVE_HANDLE_KIND_BROWSER_CONTROLLER)
	);
	m_engine->controller->get_CoreWebView2(&m_engine->core);

	ComPtr<ICoreWebView2_2> core2;
	if (SUCCEEDED(m_engine->core.As(&core2))) {
		core2->get_Environment(&m_engine->environment);
	}

	ComPtr<ICoreWebView2_13> core13;
	ComPtr<ICoreWebView2Profile> profile;
	if (SUCCEEDED(m_engine->core.As(&core13)) && SUCCEEDED(core13->get_Profile(&profile))) {
		profile->put_PreferredColorScheme(COREWEBVIEW2_PREFERRED_COLOR_SCHEME_LIGHT);
	}

	ComPtr<ICoreWebView2Controller2> controller2;
	if (SUCCEEDED(m_engine->controller.As(&controller2))) {
		controller2->put_DefaultBackgroundColor(COREWEBVIEW2_COLOR{255, 255, 255, 255});
	}

	std::string script = bridgeInitScript().toStdString();
	webview_init(m_engine->webview, script.c_str());
	SetScriptsEnabled(m_scriptsEnabled);

	EventRegistrationToken token;
	m_engine->core->add_NavigationStarting(
		Callback<ICoreWebView2NavigationStartingEventHandler>(
			[this](ICoreWebView2*, ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT {
				LPWSTR uri;
				args->get_Uri(&uri);
				QString url = fromWide(uri);
				if (url.startsWith("data:")) {
					return S_OK;
				}

				bool cancel = false;
				OnBeforeNavigate2(url, QString(), &cancel);
				if (cancel) {
					args->put_Cancel(TRUE);
				}
				else {
					m_busy = true;
				}
				return S_OK;
			}
		).Get(),
		&token
	);
	m_engine->core->add_FrameNavigationStarting(
		Callback<ICoreWebView2NavigationStartingEventHandler>(
			[this](ICoreWebView2*, ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT {
				LPWSTR uri;
				args->get_Uri(&uri);
				bool cancel = false;
				OnBeforeNavigate2(fromWide(uri), QStringLiteral("_frame"), &cancel);
				if (cancel) {
					args->put_Cancel(TRUE);
				}
				return S_OK;
			}
		).Get(),
		&token
	);
	m_engine->core->add_NavigationCompleted(
		Callback<ICoreWebView2NavigationCompletedEventHandler>(
			[this](ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs*) -> HRESULT {
				LPWSTR uri;
				sender->get_Source(&uri);
				m_locationUrl = fromWide(uri);
				m_busy = false;
				OnNavigateComplete2(m_locationUrl);
				OnDocumentComplete(m_locationUrl);
				return S_OK;
			}
		).Get(),
		&token
	);
	m_engine->core->add_DocumentTitleChanged(
		Callback<ICoreWebView2DocumentTitleChangedEventHandler>([this](ICoreWebView2* sender, IUnknown*) -> HRESULT {
			LPWSTR title;
			sender->get_DocumentTitle(&title);
			OnTitleChange(fromWide(title));
			return S_OK;
		}).Get(),
		&token
	);
	m_engine->core->add_HistoryChanged(
		Callback<ICoreWebView2HistoryChangedEventHandler>([this](ICoreWebView2* sender, IUnknown*) -> HRESULT {
			BOOL canGoBack = FALSE;
			BOOL canGoForward = FALSE;
			sender->get_CanGoBack(&canGoBack);
			sender->get_CanGoForward(&canGoForward);
			OnCommandStateChange(CSC_NAVIGATEBACK, canGoBack != FALSE);
			OnCommandStateChange(CSC_NAVIGATEFORWARD, canGoForward != FALSE);
			OnCommandStateChange(CSC_UPDATECOMMANDS, true);
			return S_OK;
		}).Get(),
		&token
	);
	m_engine->core->add_NewWindowRequested(
		Callback<ICoreWebView2NewWindowRequestedEventHandler>(
			[this](ICoreWebView2*, ICoreWebView2NewWindowRequestedEventArgs* args) -> HRESULT {
				LPWSTR uri;
				args->get_Uri(&uri);
				args->put_Handled(TRUE);
				Navigate(fromWide(uri), QStringLiteral("_blank"));
				return S_OK;
			}
		).Get(),
		&token
	);
	m_engine->core->add_WindowCloseRequested(
		Callback<ICoreWebView2WindowCloseRequestedEventHandler>([this](ICoreWebView2*, IUnknown*) -> HRESULT {
			OnWindowClosing(QString());
			return S_OK;
		}).Get(),
		&token
	);

	m_engine->controller->add_GotFocus(
		Callback<ICoreWebView2FocusChangedEventHandler>([this](ICoreWebView2Controller*, IUnknown*) -> HRESULT {
			OnBrowserFocus();
			return S_OK;
		}).Get(),
		&token
	);

	std::wstring filter = std::wstring(bridgeUrl) + L"*";
	m_engine->core->AddWebResourceRequestedFilter(filter.c_str(), COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL);
	m_engine->core->add_WebResourceRequested(
		Callback<ICoreWebView2WebResourceRequestedEventHandler>(
			[this](ICoreWebView2*, ICoreWebView2WebResourceRequestedEventArgs* args) -> HRESULT {
				ComPtr<ICoreWebView2WebResourceRequest> request;
				args->get_Request(&request);

				QByteArray body;
				ComPtr<IStream> content;
				if (SUCCEEDED(request->get_Content(&content)) && content) {
					char buffer[4096];
					ULONG read = 0;
					while (SUCCEEDED(content->Read(buffer, sizeof(buffer), &read)) && read > 0) {
						body.append(buffer, read);
					}
				}

				QByteArray reply = OnBridgeRequest(body);
				ComPtr<IStream> stream;
				stream.Attach(SHCreateMemStream(reinterpret_cast<const BYTE*>(reply.constData()), reply.size()));

				ComPtr<ICoreWebView2WebResourceResponse> response;
				m_engine->environment->CreateWebResourceResponse(
					stream.Get(),
					200,
					L"OK",
					L"Content-Type: application/json\r\nAccess-Control-Allow-Origin: *",
					&response
				);
				args->put_Response(response.Get());
				return S_OK;
			}
		).Get(),
		&token
	);

	MoveWindow(m_engine->widget, 0, 0, m_host->width(), m_host->height(), TRUE);
	ShowWindow(m_engine->widget, SW_SHOW);
#else
	static bool initialized = false;
	if (!initialized) {
		gdk_set_allowed_backends("x11");
		if (!gtk_init_check(nullptr, nullptr)) {
			return false;
		}

		WebKitWebContext* context = webkit_web_context_get_default();
		webkit_web_context_register_uri_scheme(context, "rbx-external", Engine::bridgeRequest, nullptr, nullptr);
		WebKitSecurityManager* security = webkit_web_context_get_security_manager(context);
		webkit_security_manager_register_uri_scheme_as_cors_enabled(security, "rbx-external");
		webkit_security_manager_register_uri_scheme_as_secure(security, "rbx-external");

		static XEmbedFilter filter;
		QCoreApplication::instance()->installNativeEventFilter(&filter);
		initialized = true;
	}

	m_engine->plug = gtk_plug_new(m_host->winId());
	m_engine->webview = webview_create(0, m_engine->plug);
	if (m_engine->webview == nullptr) {
		gtk_widget_destroy(m_engine->plug);
		return false;
	}

	m_engine->view =
		WEBKIT_WEB_VIEW(webview_get_native_handle(m_engine->webview, WEBVIEW_NATIVE_HANDLE_KIND_BROWSER_CONTROLLER));
	Engine::views()[m_engine->view] = this;

	GdkRGBA white = {1.0, 1.0, 1.0, 1.0};
	webkit_web_view_set_background_color(m_engine->view, &white);
	webkit_settings_set_hardware_acceleration_policy(
		webkit_web_view_get_settings(m_engine->view), WEBKIT_HARDWARE_ACCELERATION_POLICY_NEVER
	);

	std::string script = bridgeInitScript().toStdString();
	webview_init(m_engine->webview, script.c_str());
	SetScriptsEnabled(m_scriptsEnabled);

	g_signal_connect(m_engine->view, "decide-policy", G_CALLBACK(Engine::decidePolicy), nullptr);
	g_signal_connect(m_engine->view, "load-changed", G_CALLBACK(Engine::loadChanged), nullptr);
	g_signal_connect(m_engine->view, "notify::title", G_CALLBACK(Engine::titleChanged), nullptr);
	g_signal_connect(m_engine->view, "close", G_CALLBACK(Engine::closeRequested), nullptr);
	g_signal_connect(
		webkit_web_view_get_back_forward_list(m_engine->view),
		"changed",
		G_CALLBACK(Engine::historyChanged),
		m_engine->view
	);

	gtk_widget_show_all(m_engine->plug);
	m_engine->plugWindow = GDK_WINDOW_XID(gtk_widget_get_window(m_engine->plug));
	m_host->windowHandle()->installEventFilter(this);
	m_engine->resize(m_host->width(), m_host->height());
	XMapWindow(gdkDisplay(), m_engine->plugWindow);
	XFlush(gdkDisplay());

	QObject::connect(qApp, &QApplication::focusChanged, this, [this](QWidget* old, QWidget* now) {
		if (now == m_host) {
			m_engine->send(XEMBED_FOCUS_IN, XEMBED_FOCUS_CURRENT);
		}
		else if (old == m_host) {
			m_engine->send(XEMBED_FOCUS_OUT);
		}
	});
	QObject::connect(qApp, &QGuiApplication::focusWindowChanged, this, [this](QWindow* window) {
		bool active = window && window == m_host->window()->windowHandle();
		m_engine->send(active ? XEMBED_WINDOW_ACTIVATE : XEMBED_WINDOW_DEACTIVATE);
	});
	if (m_host->window()->isActiveWindow()) {
		m_engine->send(XEMBED_WINDOW_ACTIVATE);
	}
#endif

	m_ready = true;

	if (!m_pendingHtml.isEmpty()) {
		webview_set_html(m_engine->webview, m_pendingHtml.toStdString().c_str());
		m_pendingHtml.clear();
	}
	else if (!m_pendingUrl.isEmpty()) {
		webview_navigate(m_engine->webview, m_pendingUrl.toStdString().c_str());
		m_pendingUrl.clear();
	}
	return true;
}

void CHtmlView::Navigate(const QString& url, const QString& targetFrameName)
{
	if (targetFrameName == "_blank") {
		QDesktopServices::openUrl(QUrl(url));
		return;
	}

	if (!m_ready) {
		m_pendingUrl = url;
		return;
	}
	webview_navigate(m_engine->webview, url.toStdString().c_str());
}

void CHtmlView::LoadFromResource(unsigned id)
{
	QFile file(Resources::htmlPath(id));
	if (!file.open(QIODevice::ReadOnly)) {
		return;
	}

	QString html = QString::fromLatin1(file.readAll());
	if (!m_ready) {
		m_pendingHtml = html;
		return;
	}
	webview_set_html(m_engine->webview, html.toStdString().c_str());
}

void CHtmlView::ExecuteScript(const QString& script)
{
	if (m_ready) {
		webview_eval(m_engine->webview, script.toStdString().c_str());
	}
}

void CHtmlView::GoBack()
{
#ifdef _WIN32
	if (m_engine && m_engine->core) {
		m_engine->core->GoBack();
	}
#else
	if (m_engine && m_engine->view) {
		webkit_web_view_go_back(m_engine->view);
	}
#endif
}

void CHtmlView::GoForward()
{
#ifdef _WIN32
	if (m_engine && m_engine->core) {
		m_engine->core->GoForward();
	}
#else
	if (m_engine && m_engine->view) {
		webkit_web_view_go_forward(m_engine->view);
	}
#endif
}

void CHtmlView::Refresh()
{
#ifdef _WIN32
	if (m_engine && m_engine->core) {
		m_engine->core->Reload();
	}
#else
	if (m_engine && m_engine->view) {
		webkit_web_view_reload(m_engine->view);
	}
#endif
}

void CHtmlView::Stop()
{
#ifdef _WIN32
	if (m_engine && m_engine->core) {
		m_engine->core->Stop();
	}
#else
	if (m_engine && m_engine->view) {
		webkit_web_view_stop_loading(m_engine->view);
	}
#endif
}
