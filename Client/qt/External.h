#ifndef EXTERNAL_H
#define EXTERNAL_H

#include "script/ThreadRef.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>
#include <map>
#include <memory>
#include <sstream>

class CExternalObjects;
class CWorkspace;

namespace RBX {
class DataModel;
}

class CExternal
{
public:
	enum MemberKind
	{
		Method,
		Property,
	};

	virtual ~CExternal() = default;

	virtual std::map<QString, MemberKind> GetMembers() const = 0;
	virtual QJsonValue Get(const QString& name, const QString& siteUrl, CExternalObjects& objects);
	virtual QJsonValue Call(
		const QString& name, const QJsonArray& args, const QString& siteUrl, CExternalObjects& objects
	);
};

class CExternalObjects
{
public:
	explicit CExternalObjects(std::shared_ptr<CExternal> root);

	QByteArray Dispatch(const QByteArray& request, const QString& siteUrl);
	QJsonValue Wrap(std::shared_ptr<CExternal> object);

	static QJsonValue SafeArray(const QJsonArray& values);

private:
	CExternal* Find(int id) const;

	std::map<int, std::shared_ptr<CExternal>> m_objects;
	int m_nextId;
};

class CBrowserViewExternal : public CExternal
{
public:
	explicit CBrowserViewExternal(bool isRobloxIDE) : m_isRobloxIDE(isRobloxIDE) {}

	std::map<QString, MemberKind> GetMembers() const override;
	QJsonValue Get(const QString& name, const QString& siteUrl, CExternalObjects& objects) override;
	QJsonValue Call(
		const QString& name, const QJsonArray& args, const QString& siteUrl, CExternalObjects& objects
	) override;

private:
	bool m_isRobloxIDE;
};

class CApp : public CExternal
{
public:
	std::map<QString, MemberKind> GetMembers() const override;
	QJsonValue Get(const QString& name, const QString& siteUrl, CExternalObjects& objects) override;
	QJsonValue Call(
		const QString& name, const QJsonArray& args, const QString& siteUrl, CExternalObjects& objects
	) override;
};

class CWorkspaceExternal : public CExternal
{
public:
	explicit CWorkspaceExternal(std::shared_ptr<CWorkspace> workspace);

	std::map<QString, MemberKind> GetMembers() const override;
	QJsonValue Call(
		const QString& name, const QJsonArray& args, const QString& siteUrl, CExternalObjects& objects
	) override;

private:
	std::shared_ptr<CWorkspace> m_workspace;
};

class CLuaFunction : public CExternal
{
public:
	std::map<QString, MemberKind> GetMembers() const override;
	QJsonValue Call(
		const QString& name, const QJsonArray& args, const QString& siteUrl, CExternalObjects& objects
	) override;

	std::shared_ptr<RBX::DataModel> dataModel;
	RBX::Lua::FunctionRef function;
};

class CContent : public CExternal
{
public:
	std::map<QString, MemberKind> GetMembers() const override;
	QJsonValue Call(
		const QString& name, const QJsonArray& args, const QString& siteUrl, CExternalObjects& objects
	) override;

	std::stringstream m_stream;
};

#endif // EXTERNAL_H
