#include "External.h"

#include "AuthenticationNegotiator.h"
#include "ErrorUploader.h"
#include "InstanceExplorer.h"
#include "LogManager.h"
#include "MainFrm.h"
#include "Roblox.h"
#include "RobloxDoc.h"
#include "Workspace.h"
#include "script/ScriptContext.h"
#include "util/ContentProvider.h"
#include "util/Http.h"
#include "util/standardout.h"
#include "v8datamodel/DebugSettings.h"

#include <G3D/format.h>
#include <QJsonDocument>
#include <QUrl>

#ifdef _WIN32
#include <iphlpapi.h>
#include <windows.h>
#else
#include <ifaddrs.h>
#include <net/if.h>
#include <sys/socket.h>
#endif

static std::runtime_error comError(const std::string& message)
{
	RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, "%s", message.c_str());
	return std::runtime_error(message);
}

static void checkSiteTrust(const QString& siteUrl)
{
	if (siteUrl.isEmpty()) {
		return;
	}

	if (!isTrustedUrl(siteUrl.toStdString().c_str())) {
		throw comError(G3D::format("Untrusted Site: %s", siteUrl.toStdString().c_str()));
	}
}

static RBX::Reflection::Value toReflectionValue(const QJsonValue& value, size_t index)
{
	switch (value.type()) {
	case QJsonValue::Double: {
		double number = value.toDouble();
		if (number == static_cast<double>(static_cast<int>(number))) {
			return static_cast<int>(number);
		}
		return static_cast<float>(number);
	}
	case QJsonValue::String:
		return value.toString().toStdString();
	case QJsonValue::Bool:
		return value.toBool();
	case QJsonValue::Null:
		return RBX::Reflection::Value();
	default:
		throw std::runtime_error(G3D::format("Invalid argument %d VT_TYPE=%d", (int) index + 1, 9));
	}
}

static std::vector<RBX::Reflection::Value> arguments(const QJsonArray& args, qsizetype first)
{
	std::vector<RBX::Reflection::Value> result;
	for (qsizetype i = first; i < args.size() && i < first + 4; i++) {
		if (args[i].isUndefined()) {
			continue;
		}
		result.push_back(toReflectionValue(args[i], result.size()));
	}
	return result;
}

static QJsonValue toJson(const RBX::Reflection::Value& value);

static QJsonValue toSafeArray(const std::vector<RBX::Reflection::Value>& values)
{
	QJsonArray array;
	for (const RBX::Reflection::Value& value : values) {
		array.append(toJson(value));
	}
	return CExternalObjects::SafeArray(array);
}

static QJsonValue toJson(const RBX::Reflection::Value& value)
{
	if (value.isType<void>()) {
		return QJsonValue(QJsonValue::Null);
	}
	if (value.isType<int>()) {
		return value.cast<int>();
	}
	if (value.isType<bool>()) {
		return value.cast<bool>();
	}
	if (value.isType<float>()) {
		return value.cast<float>();
	}
	if (value.isType<double>()) {
		return value.cast<double>();
	}
	if (value.isType<std::string>()) {
		return QString::fromStdString(value.cast<std::string>());
	}
	if (value.isType<RBX::ContentId>()) {
		return QString::fromStdString(value.cast<RBX::ContentId>().c_str());
	}
	if (value.isType<std::vector<RBX::Reflection::Value>>()) {
		return toSafeArray(value.cast<const std::vector<RBX::Reflection::Value>&>());
	}
	return QJsonValue(QJsonValue::Undefined);
}

QJsonValue CExternal::Get(const QString& name, const QString& siteUrl, CExternalObjects& objects)
{
	throw std::runtime_error(G3D::format("Unknown name %s", name.toStdString().c_str()));
}

QJsonValue CExternal::Call(
	const QString& name, const QJsonArray& args, const QString& siteUrl, CExternalObjects& objects
)
{
	throw std::runtime_error(G3D::format("Unknown name %s", name.toStdString().c_str()));
}

QJsonValue CExternalObjects::SafeArray(const QJsonArray& values)
{
	QJsonObject wrapped;
	wrapped["__rbxsafearray"] = values;
	return wrapped;
}

CExternalObjects::CExternalObjects(std::shared_ptr<CExternal> root) : m_nextId(1)
{
	m_objects[0] = std::move(root);
}

CExternal* CExternalObjects::Find(int id) const
{
	auto it = m_objects.find(id);
	return it != m_objects.end() ? it->second.get() : nullptr;
}

QJsonValue CExternalObjects::Wrap(std::shared_ptr<CExternal> object)
{
	if (!object) {
		return QJsonValue(QJsonValue::Null);
	}

	QJsonObject members;
	for (auto& [name, kind] : object->GetMembers()) {
		members[name] = kind == CExternal::Method ? "method" : "property";
	}

	int id = m_nextId++;
	m_objects[id] = std::move(object);

	QJsonObject encoded;
	encoded["__rbxobject"] = id;
	encoded["members"] = members;
	return encoded;
}

QByteArray CExternalObjects::Dispatch(const QByteArray& request, const QString& siteUrl)
{
	QJsonObject response;
	try {
		QJsonObject call = QJsonDocument::fromJson(request).object();
		QString kind = call["kind"].toString();
		int id = call["object"].toInt();
		CExternal* object = Find(id);
		if (object == nullptr) {
			throw std::runtime_error("Object has been released");
		}

		if (kind == "members") {
			QJsonObject members;
			for (auto& [name, memberKind] : object->GetMembers()) {
				members[name] = memberKind == CExternal::Method ? "method" : "property";
			}
			response["value"] = members;
		}
		else if (kind == "get") {
			response["value"] = object->Get(call["name"].toString(), siteUrl, *this);
		}
		else if (kind == "call") {
			QJsonArray args = call["args"].toArray();
			response["value"] = object->Call(call["name"].toString(), args, siteUrl, *this);
		}
		else if (kind == "release") {
			if (id != 0) {
				m_objects.erase(id);
			}
			response["value"] = QJsonValue(QJsonValue::Null);
		}
	}
	catch (std::exception& e) {
		response["error"] = QString::fromLocal8Bit(e.what());
	}
	return QJsonDocument(response).toJson(QJsonDocument::Compact);
}

std::map<QString, CExternal::MemberKind> CBrowserViewExternal::GetMembers() const
{
	return {{"IsRobloxIDE", Property}, {"GetApp", Method}};
}

QJsonValue CBrowserViewExternal::Get(const QString& name, const QString& siteUrl, CExternalObjects& objects)
{
	if (name == "IsRobloxIDE") {
		return m_isRobloxIDE;
	}
	return CExternal::Get(name, siteUrl, objects);
}

QJsonValue CBrowserViewExternal::Call(
	const QString& name, const QJsonArray& args, const QString& siteUrl, CExternalObjects& objects
)
{
	if (name == "GetApp") {
		return objects.Wrap(std::make_shared<CApp>());
	}
	return CExternal::Call(name, args, siteUrl, objects);
}

std::map<QString, CExternal::MemberKind> CApp::GetMembers() const
{
	return {
		{"CreateGame", Method},
		{"LaunchIDE", Method},
		{"Version", Property},
		{"ErrorReporting", Property},
		{"HasErrorFiles", Property},
		{"SendErrorFiles", Method},
		{"IsUploadingErrorFiles", Property},
		{"DeleteErrorFiles", Method},
		{"ClearFileCache", Method},
		{"ID", Property},
		{"RobloxAuthenticate", Method},
	};
}

QJsonValue CApp::Get(const QString& name, const QString& siteUrl, CExternalObjects& objects)
{
	if (name == "Version") {
		return QString::fromStdString(theApp.version());
	}
	if (name == "ErrorReporting") {
		return static_cast<int>(RBX::DebugSettings::singleton().errorReporting);
	}
	if (name == "HasErrorFiles") {
		return theApp.logManager().hasCrashLogs();
	}
	if (name == "IsUploadingErrorFiles") {
		return theApp.errorUploader().IsUploading();
	}
	if (name == "ID") {
#ifdef _WIN32
		IP_ADAPTER_INFO adapterInfo[100];
		DWORD bufferLength = sizeof(adapterInfo);
		DWORD result = GetAdaptersInfo(adapterInfo, &bufferLength);
		if (result != ERROR_SUCCESS) {
			throw comError("GetAdaptersInfo failed");
		}

		QString id;
		if (bufferLength > 0) {
			std::unique_ptr<RBX::MD5Hasher> hasher(RBX::MD5Hasher::create());
			for (unsigned int i = 0; i < bufferLength / sizeof(IP_ADAPTER_INFO); i++) {
				hasher->addData(adapterInfo[i].AdapterName);
			}
			id = QString::fromLatin1(hasher->c_str());
		}
		return id;
#else
		ifaddrs* addresses = nullptr;
		if (getifaddrs(&addresses) != 0) {
			throw comError("getifaddrs failed");
		}

		QString id;
		std::unique_ptr<RBX::MD5Hasher> hasher(RBX::MD5Hasher::create());
		bool found = false;
		for (ifaddrs* address = addresses; address; address = address->ifa_next) {
			if (address->ifa_addr && address->ifa_addr->sa_family == AF_PACKET &&
				!(address->ifa_flags & IFF_LOOPBACK)) {
				hasher->addData(address->ifa_name);
				found = true;
			}
		}
		freeifaddrs(addresses);
		if (found) {
			id = QString::fromLatin1(hasher->c_str());
		}
		return id;
#endif
	}
	return CExternal::Get(name, siteUrl, objects);
}

QJsonValue CApp::Call(const QString& name, const QJsonArray& args, const QString& siteUrl, CExternalObjects& objects)
{
	if (name == "LaunchIDE") {
		if (!theApp.showMainFrame(false)) {
			throw std::runtime_error("LaunchIDE failed");
		}
		return QJsonValue(QJsonValue::Null);
	}
	if (name == "SendErrorFiles") {
		theApp.errorUploader().Upload(args.at(0).toString().toStdString().c_str(), false);
		return QJsonValue(QJsonValue::Null);
	}
	if (name == "ClearFileCache") {
		RBX::ContentProvider::singleton().clearFileCache();
		return QJsonValue(QJsonValue::Null);
	}
	if (name == "DeleteErrorFiles") {
		theApp.logManager().archiveCrashLogs();
		return QJsonValue(QJsonValue::Null);
	}
	if (name == "CreateGame") {
		CRobloxDoc* doc = nullptr;
		try {
			checkSiteTrust(siteUrl);

			switch (args.at(0).toInt()) {
			case 0:
				return objects.Wrap(std::make_shared<CWorkspaceExternal>(std::make_shared<CWorkspace>()));
			case 2: {
				doc = theApp.playDocument();
				if (!doc->views().empty()) {
					theApp.mainFrame()->activateWindow();
				}
				return objects.Wrap(std::make_shared<CWorkspaceExternal>(doc->workspace()));
			}
			default:
				throw comError("requested WindowType is invalid");
			}
		}
		catch (std::exception& e) {
			if (doc) {
				doc->close();
			}
			throw comError(e.what());
		}
	}
	if (name == "RobloxAuthenticate") {
		std::string url = args.at(0).toString().toStdString();
		std::string ticket = args.at(1).toString().toStdString();
		RBX::StandardOut::singleton()->print(
			RBX::MESSAGE_INFO, "CApp::RobloxAuthenticate %s %s", url.c_str(), ticket.c_str()
		);
		try {
			std::string host = QUrl(CRobloxApp::baseUrl()).host().toStdString();
			AuthenticationNegotiator authenticator(host.c_str());
			authenticator.negotiate(url.c_str(), ticket.c_str());
		}
		catch (std::exception& e) {
			throw comError(e.what());
		}
		return QJsonValue(QJsonValue::Null);
	}
	return CExternal::Call(name, args, siteUrl, objects);
}

CWorkspaceExternal::CWorkspaceExternal(std::shared_ptr<CWorkspace> workspace) : m_workspace(std::move(workspace))
{
}

std::map<QString, CExternal::MemberKind> CWorkspaceExternal::GetMembers() const
{
	return {
		{"Insert", Method},
		{"Write", Method},
		{"WriteSelection", Method},
		{"ExecScript", Method},
		{"ExecUrlScript", Method},
		{"Close", Method},
	};
}

QJsonValue CWorkspaceExternal::Call(
	const QString& name, const QJsonArray& args, const QString& siteUrl, CExternalObjects& objects
)
{
	if (name == "Close") {
		m_workspace->Close();
		return QJsonValue(QJsonValue::Null);
	}

	checkSiteTrust(siteUrl);

	if (!m_workspace->dataModel()) {
		throw comError("Workspace is closed");
	}

	if (name == "Insert") {
		std::string url = args.at(0).toString().toStdString();
		if (!isTrustedUrl(url.c_str())) {
			throw comError(G3D::format("Untrusted Url: %s", url.c_str()));
		}

		try {
			m_workspace->insertContent(RBX::ContentId(url), false);
		}
		catch (const std::exception& e) {
			throw comError(e.what());
		}
		return QJsonValue(QJsonValue::Null);
	}
	if (name == "Write" || name == "WriteSelection") {
		try {
			std::shared_ptr<CContent> content = std::make_shared<CContent>();
			if (name == "Write") {
				m_workspace->Write(content->m_stream);
			}
			else {
				m_workspace->WriteSelection(content->m_stream);
			}
			return objects.Wrap(content);
		}
		catch (const std::exception& e) {
			throw comError(e.what());
		}
	}
	if (name == "ExecScript" || name == "ExecUrlScript") {
		std::string script = args.at(0).toString().toStdString();
		std::vector<RBX::Reflection::Value> values = arguments(args, 1);
		std::unique_ptr<std::vector<RBX::Reflection::Value>> results =
			name == "ExecScript" ? m_workspace->ExecScript(script, values) : m_workspace->ExecUrlScript(script, values);
		return toSafeArray(*results);
	}
	return CExternal::Call(name, args, siteUrl, objects);
}

std::map<QString, CExternal::MemberKind> CLuaFunction::GetMembers() const
{
	return {{"Call", Method}, {"CallEx", Method}};
}

QJsonValue CLuaFunction::Call(
	const QString& name, const QJsonArray& args, const QString& siteUrl, CExternalObjects& objects
)
{
	if (name == "Call") {
		try {
			std::vector<RBX::Reflection::Value> values = arguments(args, 0);

			std::unique_ptr<std::vector<RBX::Reflection::Value>> results;
			{
				if (!dataModel) {
					throw std::runtime_error("DataModel is closed");
				}
				RBX::DataModel::Lock lock(dataModel);
				results = dataModel->create<RBX::ScriptContext>()->call(function, values);
			}

			if (results->size() > 0) {
				return toJson(results->front());
			}
			return QJsonValue();
		}
		catch (std::exception& e) {
			RBX::StandardOut::singleton()->print(RBX::MESSAGE_WARNING, "CLuaFunction::Call failed, %s", e.what());
			throw comError(e.what());
		}
	}

	return CExternal::Call(name, args, siteUrl, objects);
}

std::map<QString, CExternal::MemberKind> CContent::GetMembers() const
{
	return {{"Upload", Method}};
}

QJsonValue CContent::Call(
	const QString& name, const QJsonArray& args, const QString& siteUrl, CExternalObjects& objects
)
{
	if (name == "Upload") {
		std::string url = args.at(0).toString().toStdString();
		if (!isTrustedUrl(url.c_str())) {
			throw comError(G3D::format("Untrusted Url: %s", url.c_str()));
		}

		try {
			std::string response;
			RBX::Http(url.c_str()).post(m_stream, true, response);
		}
		catch (const std::exception& e) {
			throw comError(e.what());
		}
		return QJsonValue(QJsonValue::Null);
	}
	return CExternal::Call(name, args, siteUrl, objects);
}
