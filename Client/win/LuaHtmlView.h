#ifndef LUAHTMLVIEW_H
#define LUAHTMLVIEW_H

#include "SecureHtmlView.h"
#include "util/Signals.h"

#include <atlbase.h>
#include <atlcom.h>
#include <list>
#include <memory>
#include <mshtml.h>
#include <string>

namespace RBX {
class DataModel;
class Instance;

namespace Reflection {
class PropertyDescriptor;
}
} // namespace RBX

class CLuaFunction;
class DHTMLWindow;
class EventHandler;

class CLuaHtmlView : public CSecureHtmlView
{
	DECLARE_DYNCREATE(CLuaHtmlView)

public:
	class Binder : public std::enable_shared_from_this<Binder>
	{
	public:
		Binder(const Binder&) = delete;
		Binder& operator=(const Binder&) = delete;

		class PropBinding
		{
		public:
			PropBinding(
				std::shared_ptr<RBX::Instance> instance,
				const RBX::Reflection::PropertyDescriptor* descriptor,
				Binder* binder
			);

			void FUN_0042d7d0(const std::string& value);
			std::string FUN_0042d810();

		private:
			std::shared_ptr<RBX::Instance> m_unk0x00;
			const RBX::Reflection::PropertyDescriptor* m_unk0x08;
			RBX::Signals::ScopedConnection m_unk0x0c;
		};

		Binder(IHTMLInputElement* element);
		virtual ~Binder();

		void FUN_0042ebf0();
		bool FUN_0042f6e0();
		void FUN_00430750();
		void FUN_00430b30(
			std::shared_ptr<RBX::Instance> instance, const RBX::Reflection::PropertyDescriptor* descriptor
		);

	private:
		friend class CLuaHtmlView;

		std::list<std::shared_ptr<PropBinding> > m_unk0x0c;
		bool m_unk0x18;
		CComPtr<EventHandler> m_unk0x1c;
		CComPtr<IHTMLInputElement> m_unk0x20;
	};

protected:
	CLuaHtmlView();

public:
	virtual ~CLuaHtmlView();

	virtual void OnInitialUpdate() { CSecureHtmlView::OnInitialUpdate(); }

	virtual HRESULT OnGetHostInfo(DOCHOSTUIINFO* pInfo);
	virtual HRESULT OnGetExternal(LPDISPATCH* lppDispatch);
	virtual void OnDocumentComplete(LPCTSTR lpszURL);

	void FUN_0042d640();
	void FUN_0042e330(LPCTSTR html);
	void FUN_0042f1b0(std::shared_ptr<DHTMLWindow> window);
	void FUN_0042f370();
	void FUN_00430c90(
		std::shared_ptr<RBX::Instance> instance, const RBX::Reflection::PropertyDescriptor* descriptor, LPCTSTR name
	);

protected:
	virtual BOOL OnAmbientProperty(COleControlSite* pSite, DISPID dispid, VARIANT* pvar);

	afx_msg void OnDestroy();

	DECLARE_MESSAGE_MAP()

private:
	friend class DHTMLWindow;

	CComPtr<IDispatch> m_unk0xf4;
	CComObjectNoLock<CLuaFunction>* m_unk0xf8;
	std::shared_ptr<RBX::DataModel> m_unk0xfc;
	std::shared_ptr<DHTMLWindow> m_unk0x104;
	std::list<std::shared_ptr<Binder> > m_unk0x10c;
};

#endif // LUAHTMLVIEW_H
