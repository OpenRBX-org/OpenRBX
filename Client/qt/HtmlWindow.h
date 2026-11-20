#ifndef HTMLWINDOW_H
#define HTMLWINDOW_H

#include "HtmlView.h"
#include "reflection/signal.h"
#include "script/ThreadRef.h"
#include "util/Signals.h"
#include "v8tree/Instance.h"

#include <QPointer>
#include <QString>
#include <list>
#include <memory>
#include <mutex>
#include <string>

class CLuaFunction;
class DHTMLWindow;

namespace RBX {
class DataModel;
}

extern char sDHTMLWindowService[];
extern char sDHTMLWindow[];

class CLuaHtmlView : public CHtmlView
{
public:
	class Binder : public std::enable_shared_from_this<Binder>
	{
	public:
		class PropBinding
		{
		public:
			PropBinding(
				std::shared_ptr<RBX::Instance> instance,
				const RBX::Reflection::PropertyDescriptor* descriptor,
				Binder* binder
			);

			void setValue(const std::string& value);
			std::string value();

		private:
			std::shared_ptr<RBX::Instance> m_instance;
			const RBX::Reflection::PropertyDescriptor* m_descriptor;
			RBX::Signals::ScopedConnection m_connection;
		};

		Binder(CLuaHtmlView* view, const QString& element);
		Binder(const Binder&) = delete;
		Binder& operator=(const Binder&) = delete;
		~Binder();

		const QString& element() const { return m_element; }

		void updateElement();
		void elementChanged(const std::string& value);
		void postUpdate();
		void bind(std::shared_ptr<RBX::Instance> instance, const RBX::Reflection::PropertyDescriptor* descriptor);

	private:
		std::list<std::shared_ptr<PropBinding>> m_bindings;
		bool m_updatePosted = false;
		QPointer<CLuaHtmlView> m_view;
		QString m_element;
	};

	explicit CLuaHtmlView(QWidget* parent = nullptr);
	~CLuaHtmlView() override;

	void closeFrame();
	void setBody(const QString& html);
	void setWindow(std::shared_ptr<DHTMLWindow> window);
	void setCallback(RBX::Lua::FunctionRef function);
	void unbind();
	void bind(
		std::shared_ptr<RBX::Instance> instance,
		const RBX::Reflection::PropertyDescriptor* descriptor,
		const QString& name
	);

protected:
	std::shared_ptr<CExternal> OnGetExternal() override;
	void OnDocumentComplete(const QString& url) override;
	void OnHostEvent(const QString& name, const QJsonValue& value) override;

private:
	friend class Binder;

	std::shared_ptr<CLuaFunction> m_function;
	std::shared_ptr<RBX::DataModel> m_dataModel;
	std::shared_ptr<DHTMLWindow> m_window;
	std::list<std::shared_ptr<Binder>> m_binders;
	bool m_documentLoaded = false;
};

class DHTMLWindowService : public RBX::DescribedCreatable<DHTMLWindowService, RBX::Instance, sDHTMLWindowService>
{
public:
	DHTMLWindowService();

	bool askAddChild(const RBX::Instance* instance) const override;

	std::shared_ptr<RBX::Instance> newWindow(int width, int height);
};

class DHTMLWindow : public RBX::DescribedNonCreatable<DHTMLWindow, RBX::Instance, sDHTMLWindow>
{
public:
	DHTMLWindow();
	~DHTMLWindow() override = default;

	bool askSetParent(const RBX::Instance* instance) const override
	{
		return dynamic_cast<const DHTMLWindowService*>(instance) != nullptr;
	}

	void close() { setParent(nullptr); }
	void show();
	void resize(int x, int y);
	void setCallback(RBX::Lua::FunctionRef function);
	void navigate(std::string url);
	void setBody(std::string html);
	void bind(std::shared_ptr<RBX::Instance> instance, std::string property, std::string name);
	void unbind();

	static RBX::Reflection::SignalDesc<DHTMLWindow, void(std::string)> event_documentComplete;

protected:
	void onServiceProvider(const RBX::ServiceProvider* oldProvider, const RBX::ServiceProvider* newProvider) override;

private:
	friend class CLuaHtmlView;
	friend class DHTMLWindowService;

	template <class Function>
	void withView(Function function)
	{
		std::scoped_lock lock(m_mutex);
		if (m_view) {
			function(m_view);
		}
	}

	void createView(int width, int height);

	std::recursive_mutex m_mutex;
	CLuaHtmlView* m_view = nullptr;
};

inline bool DHTMLWindowService::askAddChild(const RBX::Instance* instance) const
{
	return dynamic_cast<const DHTMLWindow*>(instance) != nullptr;
}

#endif // HTMLWINDOW_H
