#ifndef V8DATAMODEL_GLOBALSETTINGS_H
#define V8DATAMODEL_GLOBALSETTINGS_H

#include "v8tree/Instance.h"
#include "v8tree/Service.h"

#include <memory>
#include <mutex>

namespace RBX {

extern const char sGlobalSettings[];

class GlobalSettings : public DescribedNonCreatable<GlobalSettings, ServiceProvider, sGlobalSettings>
{
public:
	GlobalSettings();

	static std::recursive_mutex mutex;

	static std::shared_ptr<GlobalSettings> singleton();
	void loadState();
	void saveState();
	void eraseSettingsStore();

	class Item : public NonFactoryProduct<Instance, static_cast<const char*>(0)>
	{
	protected:
		virtual bool askAddChild(const Instance* instance) const { return dynamic_cast<const Item*>(instance) != NULL; }
	};
};

template <class T, const char* sName>
class GlobalSettingsItem : public DescribedCreatable<T, GlobalSettings::Item, sName>, public Service
{
private:
	static GlobalSettingsItem* sing;

protected:
	GlobalSettingsItem()
	{
		this->setName(sName);
		sing = this;
	}

	virtual ~GlobalSettingsItem() { sing = NULL; }

public:
	static T& singleton()
	{
		std::lock_guard<std::recursive_mutex> lock(GlobalSettings::mutex);
		if (sing == NULL) {
			std::shared_ptr<T> s = Creatable<Instance>::create<T>();
			s->setParent(GlobalSettings::singleton().get());
		}
		return static_cast<T&>(*sing);
	}
};

template <class T, const char* sName>
GlobalSettingsItem<T, sName>* GlobalSettingsItem<T, sName>::sing = NULL;

} // namespace RBX

#endif // V8DATAMODEL_GLOBALSETTINGS_H
