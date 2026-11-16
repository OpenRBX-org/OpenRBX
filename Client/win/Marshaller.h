#ifndef MARSHALLER_H
#define MARSHALLER_H

#include <atlbase.h>
#include <atlwin.h>
#include <deque>
#include <functional>
#include <map>
#include <mutex>

class Marshaller : public CWindowImpl<Marshaller>
{
public:
	Marshaller(DWORD threadId);

	~Marshaller() override = default;

	// clang-format off
	BEGIN_MSG_MAP(Marshaller)
		MESSAGE_HANDLER(WM_USER + 101, FUN_00418660)
	END_MSG_MAP()
	// clang-format on

	void OnFinalMessage(HWND hWnd) override { delete this; }

	void FUN_00418fd0(std::function<void()> function)
	{
		{
			std::scoped_lock lock(m_unk0x40);
			m_unk0x2c.push_back(function);
		}

		::PostMessage(m_hWnd, WM_USER + 101, 0, 0);
	}

	static Marshaller* FUN_0041a4a0();

private:
	LRESULT FUN_00418660(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

	int m_unk0x24;
	DWORD m_unk0x28;
	std::deque<std::function<void()>> m_unk0x2c;
	std::mutex m_unk0x40;

	static std::map<DWORD, Marshaller*> g_unk0x00953144;
	static std::recursive_mutex g_unk0x00953150;
};

extern Marshaller* g_unk0x00952f1c;

#endif // MARSHALLER_H
