#include "AuthenticationNegotiator.h"

#include "util/Http.h"

#include <exception>

#ifdef _WIN32
#include <windows.h>
#endif

AuthenticationNegotiator::AuthenticationNegotiator(const char* host) : m_host(host)
{
}

AuthenticationNegotiator::~AuthenticationNegotiator() = default;

static bool isNegotiationSupported()
{
#ifdef _WIN32
	OSVERSIONINFO info = {0};
	info.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&info);
	return info.dwMajorVersion >= 6 ? true : false;
#else
	return true;
#endif
}

std::string AuthenticationNegotiator::negotiate(const char* url, const char* ticket)
{
	try {
		static bool enabled = isNegotiationSupported();
		if (!enabled) {
			return "";
		}

		std::string response;
		{
			std::string request(url);
			if (ticket) {
				request += "?suggest=";
				request += ticket;
			}

			RBX::Http http(request.c_str());
			http.additionalHeaders = "RBXAuthenticationNegotiation:";
			http.additionalHeaders += m_host;
			http.additionalHeaders += "\r\n";
			http.get(response);
		}
		return response;
	}
	catch (std::exception&) {
		return "";
	}
}
