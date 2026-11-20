#ifndef AUTHENTICATIONNEGOTIATOR_H
#define AUTHENTICATIONNEGOTIATOR_H

#include <string>

class AuthenticationNegotiator
{
public:
	AuthenticationNegotiator(const char* host);
	~AuthenticationNegotiator();

	std::string negotiate(const char* url, const char* ticket);

private:
	std::string m_host;
};

#endif // AUTHENTICATIONNEGOTIATOR_H
