#ifndef WEBSERVICE_WEBSERVICE_H
#define WEBSERVICE_WEBSERVICE_H

#include <map>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace RBX {
class DataModel;
}

namespace Roblox {

class StandardOutLog;

class WebService
{
	struct Job
	{
		std::shared_ptr<RBX::DataModel> dataModel;
		double expiration;
	};

	using Jobs = std::map<std::string, Job>;

	Jobs jobs;
	std::recursive_mutex sync;
	const std::string version;
	StandardOutLog& standardOutLog;

	Jobs::iterator getJob(const std::string& jobID);
	static void touch(Job& job, double timeout);
	static double secondsToTimeout(const Job& job);

public:
	WebService(std::string version, StandardOutLog& standardOutLog);

	const std::string& getVersion() const { return version; }
	nlohmann::json getStatus();
	nlohmann::json openJob(const std::string& jobID, const nlohmann::json& script, double timeout);
	void touchJob(const std::string& jobID, double timeout);
	nlohmann::json execute(const std::string& jobID, const nlohmann::json& script);
	void closeJob(const std::string& jobID);
	nlohmann::json batchJob(const std::string& jobID, const nlohmann::json& script, double timeout);
	double getTimeout(const std::string& jobID);
	std::vector<std::string> getAllJobs();
	int closeOrphanedJobs(const std::vector<std::string>& activeJobs);
	int closeTimedoutJobs();
	void closeAllJobs();
	nlohmann::json getStandardOutMessages(int maxCount);
};

} // namespace Roblox

#endif // WEBSERVICE_WEBSERVICE_H
