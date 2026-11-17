#include "WebService.h"

#include "ThumbnailGenerator.h"
#include "Version.h"
#include "ViewBase.h"
#include "network/API.h"
#include "reflection/type.h"
#include "script/ScriptContext.h"
#include "util/ContentProvider.h"
#include "util/Name.h"
#include "util/Profiling.h"
#include "util/Sound.h"
#include "util/Utilities.h"
#include "util/standardout.h"
#include "v8datamodel/DataModel.h"
#include "v8datamodel/Workspace.h"
#include "v8datamodel/factoryregistration.h"

#include <G3D/System.h>
#include <G3D/format.h>
#include <algorithm>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <httplib.h>
#include <queue>
#include <sstream>
#include <stdexcept>

template const RBX::Name& RBX::Name::doDeclare<RBX::sWorkspace>();

namespace Roblox {

class StandardOutLog : public RBX::Listener<RBX::StandardOut, RBX::StandardOutMessage>
{
private:
	std::shared_ptr<RBX::StandardOut> standardOut;

public:
	std::mutex messagesMutex;
	std::queue<RBX::StandardOutMessage> messages;

	StandardOutLog() : standardOut(RBX::StandardOut::singleton())
	{
		std::scoped_lock lock(RBX::StandardOut::singleton()->sync);
		RBX::startListening(standardOut, this);
	}

	~StandardOutLog() override
	{
		std::scoped_lock lock(RBX::StandardOut::singleton()->sync);
		RBX::stopListening(standardOut, this);
	}

protected:
	void onEvent(const RBX::StandardOut* source, RBX::StandardOutMessage event) override
	{
		std::scoped_lock lock(messagesMutex);

		std::FILE* stream = event.type >= RBX::MESSAGE_WARNING ? stderr : stdout;
		std::fprintf(stream, "%s\n", event.message.c_str());
		std::fflush(stream);

		messages.push(event);

		if (messages.size() > 1000) {
			messages.pop();
		}
	}
};

static const int maxTableDepth = 10;

static void convert(const nlohmann::json& source, RBX::Reflection::Value& dest, int depth)
{
	switch (source.type()) {
	case nlohmann::json::value_t::number_integer:
	case nlohmann::json::value_t::number_unsigned:
	case nlohmann::json::value_t::number_float:
		dest = source.get<double>();
		break;

	case nlohmann::json::value_t::boolean:
		dest = source.get<bool>();
		break;

	case nlohmann::json::value_t::string:
		dest = source.get<std::string>();
		break;

	case nlohmann::json::value_t::array:
		if (depth < maxTableDepth) {
			std::vector<RBX::Reflection::Value> table(source.size());
			for (size_t i = 0; i < source.size(); ++i) {
				convert(source[i], table[i], depth + 1);
			}
			dest = table;
		}
		break;

	default:
		break;
	}
}

static nlohmann::json convertToLua(const RBX::Reflection::Value& source, int depth)
{
	if (source.isType<int>()) {
		return source.cast<int>();
	}

	if (source.isType<float>()) {
		return source.cast<float>();
	}

	if (source.isType<double>()) {
		return source.cast<double>();
	}

	if (source.isType<bool>()) {
		return source.cast<bool>();
	}

	if (source.isType<std::string>()) {
		return source.cast<std::string>();
	}

	if (source.isType<RBX::ContentId>()) {
		return source.get<std::string>();
	}

	if (depth < maxTableDepth && source.isType<std::vector<RBX::Reflection::Value>>()) {
		const std::vector<RBX::Reflection::Value>& table = source.cast<const std::vector<RBX::Reflection::Value>&>();

		nlohmann::json result = nlohmann::json::array();
		for (const RBX::Reflection::Value& item : table) {
			result.push_back(convertToLua(item, depth + 1));
		}
		return result;
	}

	return nullptr;
}

WebService::WebService(std::string version, StandardOutLog& standardOutLog)
	: version(std::move(version)), standardOutLog(standardOutLog)
{
}

void WebService::touch(Job& job, double timeout)
{
	job.expiration = G3D::System::getLocalTime() + timeout;
}

double WebService::secondsToTimeout(const Job& job)
{
	return job.expiration - G3D::System::getLocalTime();
}

WebService::Jobs::iterator WebService::getJob(const std::string& jobID)
{
	Jobs::iterator job = jobs.find(jobID);

	if (job == jobs.end()) {
		throw std::runtime_error(G3D::format("Job %s not found", jobID.c_str()));
	}

	return job;
}

nlohmann::json WebService::getStatus()
{
	std::scoped_lock lock(sync);
	return {{"version", version}, {"environmentCount", jobs.size()}};
}

std::vector<std::string> WebService::getAllJobs()
{
	std::scoped_lock lock(sync);

	std::vector<std::string> result;
	result.reserve(jobs.size());
	for (const auto& [id, job] : jobs) {
		result.push_back(id);
	}

	return result;
}

void WebService::touchJob(const std::string& jobID, double timeout)
{
	std::scoped_lock lock(sync);
	touch(getJob(jobID)->second, timeout);
}

nlohmann::json WebService::openJob(const std::string& jobID, const nlohmann::json& script, double timeout)
{
	std::shared_ptr<RBX::DataModel> dataModel = RBX::DataModel::createDataModel();
	{
		std::scoped_lock lock(sync);
		Job job;
		job.dataModel = dataModel;
		touch(job, timeout);
		jobs[jobID] = job;
	}
	RBX::StandardOut::singleton()->print(RBX::MESSAGE_INFO, "Opened Job %s", jobID.c_str());

	return execute(jobID, script);
}

void WebService::closeJob(const std::string& jobID)
{
	std::shared_ptr<RBX::DataModel> dataModel;
	{
		std::scoped_lock lock(sync);
		Jobs::iterator iter = jobs.find(jobID);
		if (iter != jobs.end()) {
			dataModel = iter->second.dataModel;
			jobs.erase(iter);
		}
	}

	if (dataModel) {
		RBX::DataModel::closeDataModel(dataModel, true);
		RBX::StandardOut::singleton()->print(RBX::MESSAGE_INFO, "Closed Job %s", jobID.c_str());
	}
}

int WebService::closeTimedoutJobs()
{
	std::vector<std::shared_ptr<RBX::DataModel>> dataModels;
	{
		std::scoped_lock lock(sync);

		Jobs keep;
		for (const Jobs::value_type& job : jobs) {
			if (secondsToTimeout(job.second) >= 0) {
				keep.insert(job);
			}
			else {
				dataModels.push_back(job.second.dataModel);
			}
		}

		jobs = keep;
	}

	for (const std::shared_ptr<RBX::DataModel>& dataModel : dataModels) {
		RBX::DataModel::closeDataModel(dataModel, true);
	}

	return static_cast<int>(dataModels.size());
}

int WebService::closeOrphanedJobs(const std::vector<std::string>& activeJobs)
{
	std::vector<std::shared_ptr<RBX::DataModel>> dataModels;
	{
		Jobs keep;
		std::scoped_lock lock(sync);
		for (const Jobs::value_type& job : jobs) {
			if (std::find(activeJobs.begin(), activeJobs.end(), job.first) != activeJobs.end()) {
				keep.insert(job);
			}
			else {
				dataModels.push_back(job.second.dataModel);
			}
		}

		jobs = keep;
	}

	for (const std::shared_ptr<RBX::DataModel>& dataModel : dataModels) {
		RBX::DataModel::closeDataModel(dataModel, true);
	}

	return static_cast<int>(dataModels.size());
}

void WebService::closeAllJobs()
{
	std::scoped_lock lock(sync);

	for (const Jobs::value_type& job : jobs) {
		RBX::DataModel::closeDataModel(job.second.dataModel, true);
		RBX::StandardOut::singleton()->print(RBX::MESSAGE_INFO, "Closed Job %s", job.first.c_str());
	}

	jobs.clear();
}

double WebService::getTimeout(const std::string& jobID)
{
	std::scoped_lock lock(sync);
	return secondsToTimeout(getJob(jobID)->second);
}

nlohmann::json WebService::execute(const std::string& jobID, const nlohmann::json& script)
{
	std::string code = script.value("script", std::string());
	if (RBX::ContentProvider::isHttpUrl(code)) {
		std::unique_ptr<std::istream> stream = RBX::ContentProvider::singleton().getContent(RBX::ContentId(code));
		std::stringstream out;
		out << stream->rdbuf();
		code = out.str();
	}

	std::shared_ptr<RBX::DataModel> dataModel;
	{
		std::scoped_lock lock(sync);
		dataModel = getJob(jobID)->second.dataModel;
	}

	std::unique_ptr<std::vector<RBX::Reflection::Value>> tuple;
	{
		std::vector<RBX::Reflection::Value> args;
		if (script.contains("arguments")) {
			const nlohmann::json& arguments = script.at("arguments");
			args.resize(arguments.size());
			for (size_t i = 0; i < arguments.size(); ++i) {
				convert(arguments.at(i), args[i], 1);
			}
		}

		const std::string name = script.value("name", std::string());

		RBX::DataModel::Lock lock(dataModel);
		RBX::ScriptContext* context = dataModel ? dataModel->create<RBX::ScriptContext>() : nullptr;
		tuple = context->executeInNewThread(RBX::Security::TrustedWebService, code.c_str(), name.c_str(), args);
	}

	nlohmann::json result = nlohmann::json::array();
	for (const RBX::Reflection::Value& value : *tuple) {
		result.push_back(convertToLua(value, 1));
	}

	return result;
}

nlohmann::json WebService::batchJob(const std::string& jobID, const nlohmann::json& script, double timeout)
{
	nlohmann::json result = openJob(jobID, script, timeout);
	closeJob(jobID);
	return result;
}

nlohmann::json WebService::getStandardOutMessages(int maxCount)
{
	std::scoped_lock lock(standardOutLog.messagesMutex);

	while (standardOutLog.messages.size() > static_cast<size_t>(maxCount)) {
		standardOutLog.messages.pop();
	}

	nlohmann::json result = nlohmann::json::array();
	while (!standardOutLog.messages.empty()) {
		const RBX::StandardOutMessage& message = standardOutLog.messages.front();
		result.push_back({{"type", message.type}, {"time", message.time}, {"text", message.message}});
		standardOutLog.messages.pop();
	}

	return result;
}

} // namespace Roblox

static std::string initRoblox(const std::string& contentFolder)
{
	RBX::Profiling::init(true);

	RBX::StandardOut::singleton()->print(RBX::MESSAGE_INFO, "Intializing Roblox Web Service");

	RBX::ContentProvider::singleton();

	if (!contentFolder.empty()) {
		RBX::ContentProvider::singleton().setAssetFolder(contentFolder.c_str());
	}

	static RBX::FactoryRegistrator registerFactoryObjects;
	ThumbnailGenerator::classDescriptor();

	RBX::Soundscape::SoundService::soundDisabled = true;
	RBX::ViewBase::getShadowsEnabled() = false;

	RBX::Network::API::init(WEBSERVICE_VERSION_STRING);

	std::string dotVersion;
	const std::vector<std::string> tokens = RBX::tokenize(WEBSERVICE_VERSION_STRING, " ,.");
	for (std::vector<std::string>::const_iterator token = tokens.begin(); token != tokens.end(); ++token) {
		if (token != tokens.begin()) {
			dotVersion += ".";
		}
		dotVersion += *token;
	}

	return dotVersion;
}

static void reply(httplib::Response& response, const nlohmann::json& body, int status = 200)
{
	response.status = status;
	response.set_content(body.dump(), "application/json");
}

template <class Handler>
static httplib::Server::Handler route(Handler handler)
{
	return [handler](const httplib::Request& request, httplib::Response& response) {
		try {
			const nlohmann::json body =
				request.body.empty() ? nlohmann::json::object() : nlohmann::json::parse(request.body);
			reply(response, handler(request, body));
		}
		catch (const nlohmann::json::exception& e) {
			reply(response, {{"error", e.what()}}, 400);
		}
		catch (const std::exception& e) {
			reply(response, {{"error", e.what()}}, 500);
		}
		catch (...) {
			reply(response, {{"error", "unknown exception"}}, 500);
		}
	};
}

static httplib::Server* runningServer;

static void stopServer(int)
{
	runningServer->stop();
}

int main(int argc, char* argv[])
{
	std::string host = "127.0.0.1";
	int port = 64989;
	std::string contentFolder;

	for (int i = 1; i < argc; ++i) {
		const std::string arg = argv[i];

		if (arg == "--host" && i + 1 < argc) {
			host = argv[++i];
		}
		else if (arg == "--port" && i + 1 < argc) {
			port = std::atoi(argv[++i]);
		}
		else if (arg == "--content" && i + 1 < argc) {
			contentFolder = argv[++i];
		}
		else {
			std::fprintf(stderr, "usage: %s [--host address] [--port number] [--content folder]\n", argv[0]);
			return 1;
		}
	}

	Roblox::StandardOutLog standardOutLog;

	std::string dotVersion;
	try {
		dotVersion = initRoblox(contentFolder);
	}
	catch (const std::exception& e) {
		RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, e);
		return 1;
	}

	Roblox::WebService service(dotVersion, standardOutLog);

	using Request = const httplib::Request&;
	using Body = const nlohmann::json&;

	httplib::Server server;

	server.Get("/version", route([&](Request, Body) -> nlohmann::json { return {{"version", service.getVersion()}}; }));

	server.Get("/status", route([&](Request, Body) { return service.getStatus(); }));

	server.Get("/messages", route([&](Request request, Body) -> nlohmann::json {
				   const int maxCount =
					   request.has_param("maxCount") ? std::stoi(request.get_param_value("maxCount")) : 1000;
				   return {{"messages", service.getStandardOutMessages(maxCount)}};
			   }));

	server.Get("/jobs", route([&](Request, Body) -> nlohmann::json { return {{"jobs", service.getAllJobs()}}; }));

	server.Delete("/jobs", route([&](Request, Body) {
					  service.closeAllJobs();
					  return nlohmann::json::object();
				  }));

	server.Post("/jobs/close-timedout", route([&](Request, Body) -> nlohmann::json {
					return {{"closed", service.closeTimedoutJobs()}};
				}));

	server.Post("/jobs/close-orphaned", route([&](Request, Body body) -> nlohmann::json {
					return {
						{"closed", service.closeOrphanedJobs(body.at("activeJobs").get<std::vector<std::string>>())}
					};
				}));

	server.Put(
		"/jobs/:id", route([&](Request request, Body body) -> nlohmann::json {
			return {
				{"result",
				 service.openJob(request.path_params.at("id"), body.at("script"), body.at("timeout").get<double>())}
			};
		})
	);

	server.Delete("/jobs/:id", route([&](Request request, Body) {
					  service.closeJob(request.path_params.at("id"));
					  return nlohmann::json::object();
				  }));

	server.Post("/jobs/:id/execute", route([&](Request request, Body body) -> nlohmann::json {
					return {{"result", service.execute(request.path_params.at("id"), body.at("script"))}};
				}));

	server.Post(
		"/jobs/:id/batch", route([&](Request request, Body body) -> nlohmann::json {
			return {
				{"result",
				 service.batchJob(request.path_params.at("id"), body.at("script"), body.at("timeout").get<double>())}
			};
		})
	);

	server.Post("/jobs/:id/touch", route([&](Request request, Body body) {
					service.touchJob(request.path_params.at("id"), body.at("timeout").get<double>());
					return nlohmann::json::object();
				}));

	server.Get("/jobs/:id/timeout", route([&](Request request, Body) -> nlohmann::json {
				   return {{"timeout", service.getTimeout(request.path_params.at("id"))}};
			   }));

	runningServer = &server;
	std::signal(SIGINT, stopServer);
	std::signal(SIGTERM, stopServer);

	if (!server.bind_to_port(host, port)) {
		RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, "Unable to listen on %s:%d", host.c_str(), port);
		return 1;
	}

	RBX::StandardOut::singleton()->print(RBX::MESSAGE_INFO, "Listening on %s:%d", host.c_str(), port);
	server.listen_after_bind();

	service.closeAllJobs();

	return 0;
}

template void RBX::Name::callDoDeclare<RBX::Soundscape::sSoundService>();
template const RBX::Name& RBX::Name::doDeclare<RBX::Soundscape::sSoundService>();
template void RBX::Name::callDoDeclare<RBX::Soundscape::sSoundChannel>();
template const RBX::Name& RBX::Name::doDeclare<RBX::Soundscape::sSoundChannel>();

template const RBX::FactoryProduct<RBX::Soundscape::SoundService, RBX::Instance, RBX::Soundscape::sSoundService>::
	Creator RBX::FactoryProduct<RBX::Soundscape::SoundService, RBX::Instance, RBX::Soundscape::sSoundService>::creator;
template const RBX::FactoryProduct<RBX::Soundscape::SoundChannel, RBX::Instance, RBX::Soundscape::sSoundChannel>::
	Creator RBX::FactoryProduct<RBX::Soundscape::SoundChannel, RBX::Instance, RBX::Soundscape::sSoundChannel>::creator;
