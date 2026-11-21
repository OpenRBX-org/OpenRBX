#include "util/Sound.h"

#include "reflection/enumconverter.h"
#include "util/Events.h"
#include "util/Math.h"
#include "util/Name.h"
#include "util/Velocity.h"
#include "util/standardout.h"
#include "v8datamodel/Camera.h"
#include "v8datamodel/DebugSettings.h"
#include "v8datamodel/GameSettings.h"
#include "v8datamodel/PVInstance.h"
#include "v8datamodel/PartInstance.h"
#include "v8datamodel/Stats.h"
#include "v8datamodel/Workspace.h"
#include "v8xml/XmlElement.h"

#include <G3D/System.h>
#include <G3D/g3dmath.h>
#include <algorithm>

char sStockSound[] = "StockSound";

static const double gcTime = 5.0;

class StockSound : public RBX::DescribedCreatable<StockSound, RBX::Soundscape::SoundChannel, sStockSound>
{
public:
	StockSound();
};

class SoundServiceStatsItem : public RBX::Stats::Item
{
private:
	const RBX::Soundscape::SoundService* service;
	unsigned int currentalloced;
	unsigned int maxalloced;
	unsigned int numSounds;
	unsigned int numUnusedSounds;

public:
	SoundServiceStatsItem(const RBX::Soundscape::SoundService* service);

	void update() override;
};

namespace RBX::Soundscape {

char sSoundService[] = "SoundService";

char sSoundChannel[] = "Sound";

bool SoundService::soundDisabled = false;

int SoundChannel::getPlayCount() const
{
	return playCount;
}

bool SoundChannel::isPaused() const
{
	return channel.isPaused();
}

bool SoundChannel::isPlaying() const
{
	return channel.isPlaying();
}

SoundId SoundChannel::getSoundId() const
{
	return soundId;
}

float SoundChannel::getVolume() const
{
	return volume;
}

bool SoundChannel::getLooped() const
{
	return looped;
}

Sound::Sound(Audio::System* system, SoundId id, bool is3D) : data(), system(system), refCount(0), id(id), is3D(is3D)
{
}

} // namespace RBX::Soundscape

RBX::Reflection::BoundProp<float, 1> RBX::Soundscape::SoundService::prop_dopplerscale(
	"DopplerScale",
	"Data",
	&RBX::Soundscape::SoundService::dopplerscale,
	&RBX::Soundscape::SoundService::on3DSettingChanged
);

RBX::Reflection::BoundProp<float, 1> RBX::Soundscape::SoundService::prop_distancefactor(
	"DistanceFactor",
	"Data",
	&RBX::Soundscape::SoundService::distancefactor,
	&RBX::Soundscape::SoundService::on3DSettingChanged
);

RBX::Reflection::BoundProp<float, 1> RBX::Soundscape::SoundService::prop_rolloffscale(
	"RolloffScale",
	"Data",
	&RBX::Soundscape::SoundService::rolloffscale,
	&RBX::Soundscape::SoundService::on3DSettingChanged
);

RBX::Reflection::EnumPropDescriptor<RBX::Soundscape::SoundService, RBX::Soundscape::ReverbType> RBX::Soundscape::
	SoundService::prop_AmbientReverb(
		"AmbientReverb",
		"Data",
		&RBX::Soundscape::SoundService::getAmbientReverb,
		&RBX::Soundscape::SoundService::setAmbientReverb
	);
static RBX::Reflection::PropDescriptor<RBX::Soundscape::SoundChannel, RBX::Soundscape::SoundId> sound_desc_SoundId(
	"SoundId", "Data", &RBX::Soundscape::SoundChannel::getSoundId, &RBX::Soundscape::SoundChannel::setSoundId
);

static RBX::Reflection::PropDescriptor<RBX::Soundscape::SoundChannel, float> sound_desc_Volume(
	"Volume", "Data", &RBX::Soundscape::SoundChannel::getVolume, &RBX::Soundscape::SoundChannel::setVolume
);

static RBX::Reflection::PropDescriptor<RBX::Soundscape::SoundChannel, int> sound_desc_PlayCount(
	"PlayCount",
	"Data",
	&RBX::Soundscape::SoundChannel::getPlayCount,
	&RBX::Soundscape::SoundChannel::setPlayCount,
	RBX::Reflection::PropertyDescriptor::STREAMING
);

static RBX::Reflection::PropDescriptor<RBX::Soundscape::SoundChannel, bool> sound_desc_Looped(
	"Looped", "Behavior", &RBX::Soundscape::SoundChannel::getLooped, &RBX::Soundscape::SoundChannel::setLooped
);

static RBX::Reflection::PropDescriptor<RBX::Soundscape::SoundChannel, bool> sound_desc_IsPlaying(
	"IsPlaying", "Data", &RBX::Soundscape::SoundChannel::isPlaying, 0, RBX::Reflection::PropertyDescriptor::UI
);

static RBX::Reflection::PropDescriptor<RBX::Soundscape::SoundChannel, bool> sound_desc_isPaused(
	"IsPaused", "Data", &RBX::Soundscape::SoundChannel::isPaused, 0, RBX::Reflection::PropertyDescriptor::UI
);

RBX::Reflection::BoundProp<bool, 1> RBX::Soundscape::SoundChannel::sound_desc_playOnRemove(
	"PlayOnRemove", "Behavior", &RBX::Soundscape::SoundChannel::playOnRemove
);

static RBX::Reflection::BoundFuncDesc<RBX::Soundscape::SoundChannel, void(), 0> sound_playFunction(
	&RBX::Soundscape::SoundChannel::play, "Play", RBX::Reflection::FunctionDescriptor::AnyCaller
);

static RBX::Reflection::BoundFuncDesc<RBX::Soundscape::SoundChannel, void(), 0> sound_pauseFunction(
	&RBX::Soundscape::SoundChannel::pause, "Pause", RBX::Reflection::FunctionDescriptor::AnyCaller
);

static RBX::Reflection::BoundFuncDesc<RBX::Soundscape::SoundChannel, void(), 0> sound_stopFunction(
	&RBX::Soundscape::SoundChannel::stop, "Stop", RBX::Reflection::FunctionDescriptor::AnyCaller
);

bool convert(const G3D::Vector3& src, RBX::Audio::Vector& dst)
{
	G3D::Vector3 filter = src;

	if (RBX::Math::isDenormal(filter.x)) {
		filter.x = 0.0f;
	}
	if (RBX::Math::isDenormal(filter.y)) {
		filter.y = 0.0f;
	}
	if (RBX::Math::isDenormal(filter.z)) {
		filter.z = 0.0f;
	}

	if (RBX::Math::isNanInfDenormVector3(filter)) {
		return false;
	}

	filter = filter.clamp(-5000.0f, 5000.0f);
	dst.x = filter.x;
	dst.y = filter.y;
	dst.z = filter.z;

	return true;
}

namespace RBX {

template <>
std::string StringConverter<Soundscape::SoundId>::convertToString(const Soundscape::SoundId& value)
{
	return value.toString();
}

template <>
bool StringConverter<Soundscape::SoundId>::convertToValue(const std::string& text, Soundscape::SoundId& value)
{
	value = Soundscape::SoundId(text);
	return true;
}

namespace Reflection {

template <>
const Type& Type::singleton<Soundscape::SoundId>()
{
	return singleton<ContentId>();
}

template <>
void TypedPropertyDescriptor<Soundscape::SoundId>::readValue(
	DescribedBase* instance, const XmlElement* element, IReferenceBinder& binder
) const
{
	if (!element->isXsiNil()) {
		ContentId value;
		if (element->getValue(value)) {
			setValue(instance, Soundscape::SoundId(value));
		}
	}
}

template <>
void TypedPropertyDescriptor<Soundscape::SoundId>::writeValue(const DescribedBase* instance, XmlElement* element) const
{
	element->setValue(getValue(instance));
}

template <>
bool TypedPropertyDescriptor<Soundscape::SoundId>::hasStringValue() const
{
	return true;
}

template <>
std::string TypedPropertyDescriptor<Soundscape::SoundId>::getStringValue(const DescribedBase* instance) const
{
	return StringConverter<Soundscape::SoundId>::convertToString(getValue(instance));
}

template <>
bool TypedPropertyDescriptor<Soundscape::SoundId>::setStringValue(
	DescribedBase* instance, const std::string& text
) const
{
	Soundscape::SoundId value;
	if (StringConverter<Soundscape::SoundId>::convertToValue(text, value)) {
		setValue(instance, value);
		return true;
	}

	return false;
}

} // namespace Reflection
} // namespace RBX

namespace RBX::Soundscape {

SoundChannel::~SoundChannel() = default;

} // namespace RBX::Soundscape

namespace RBX::Soundscape {

void SoundService::getSoundStats(
	const LoadedSounds& sounds, unsigned int& numSounds, unsigned int& numUnusedSounds, std::size_t& memoryUsed
)
{
	for (LoadedSounds::const_iterator iter = sounds.begin(); iter != sounds.end(); ++iter) {
		numSounds++;
		if (iter->second->refCount == 0) {
			numUnusedSounds++;
		}
		memoryUsed += iter->second->getMemoryUsed();
	}
}

void SoundService::getSoundStats(unsigned int& numSounds, unsigned int& numUnusedSounds, std::size_t& memoryUsed) const
{
	getSoundStats(loadedSounds, numSounds, numUnusedSounds, memoryUsed);
	getSoundStats(loaded3DSounds, numSounds, numUnusedSounds, memoryUsed);
}

} // namespace RBX::Soundscape

namespace RBX::Soundscape {

void SoundService::updateAmbientReverb()
{
	if (!system) {
		return;
	}

	switch (ambientReverb) {
	case NoReverb:
		system->setReverbProperties(Audio::Preset::Off);
		break;
	case GenericReverb:
		system->setReverbProperties(Audio::Preset::Generic);
		break;
	case PaddedCell:
		system->setReverbProperties(Audio::Preset::PaddedCell);
		break;
	case Room:
		system->setReverbProperties(Audio::Preset::PaddedCell);
		break;
	case Bathroom:
		system->setReverbProperties(Audio::Preset::Bathroom);
		break;
	case LivingRoom:
		system->setReverbProperties(Audio::Preset::LivingRoom);
		break;
	case StoneRoom:
		system->setReverbProperties(Audio::Preset::StoneRoom);
		break;
	case Auditorium:
		system->setReverbProperties(Audio::Preset::Auditorium);
		break;
	case ConcertHall:
		system->setReverbProperties(Audio::Preset::ConcertHall);
		break;
	case Cave:
		system->setReverbProperties(Audio::Preset::Cave);
		break;
	case Arena:
		system->setReverbProperties(Audio::Preset::Arena);
		break;
	case Hangar:
		system->setReverbProperties(Audio::Preset::Hangar);
		break;
	case CarpettedHallway:
		system->setReverbProperties(Audio::Preset::CarpettedHallway);
		break;
	case Hallway:
		system->setReverbProperties(Audio::Preset::Hallway);
		break;
	case StoneCorridor:
		system->setReverbProperties(Audio::Preset::StoneCorridor);
		break;
	case Alley:
		system->setReverbProperties(Audio::Preset::Alley);
		break;
	case Forest:
		system->setReverbProperties(Audio::Preset::Forest);
		break;
	case City:
		system->setReverbProperties(Audio::Preset::City);
		break;
	case Mountains:
		system->setReverbProperties(Audio::Preset::Mountains);
		break;
	case Quarry:
		system->setReverbProperties(Audio::Preset::Quarry);
		break;
	case Plain:
		system->setReverbProperties(Audio::Preset::Plain);
		break;
	case ParkingLot:
		system->setReverbProperties(Audio::Preset::ParkingLot);
		break;
	case SewerPipe:
		system->setReverbProperties(Audio::Preset::SewerPipe);
		break;
	case UnderWater:
		system->setReverbProperties(Audio::Preset::UnderWater);
		break;
	}
}

void SoundService::update3DSettings()
{
	if (system && !system->set3DSettings(dopplerscale, distancefactor, rolloffscale) &&
		DebugSettings::singleton().soundWarnings) {
		StandardOut::singleton()->print(MESSAGE_WARNING, "Invalid 3D sound settings");
	}
}

void SoundChannel::releaseChannel()
{
	if (channel) {
		channel.setUserData(nullptr);

		channel = Audio::Channel();
	}

	if (sound) {
		sound->refCount--;
		sound.reset();
	}
}

void SoundChannel::onServiceProvider(const ServiceProvider* oldProvider, const ServiceProvider* newProvider)
{
	releaseChannel();
	Instance::onServiceProvider(oldProvider, newProvider);
}

void SoundChannel::updateLooped()
{
	channel.setLooped(looped);
}

void SoundChannel::update3D(const Audio::Channel& channel)
{
	if (is3D && part) {
		Audio::Vector pos;
		if (convert(part->getCoordinateFrame().translation, pos)) {
			Audio::Vector vel;
			if (convert(part->getLinearVelocity(), vel)) {
				channel.set3DAttributes(pos, vel);
			}
		}
	}
}

const std::shared_ptr<Audio::SoundData>& Sound::get()
{
	if (!data) {
		std::shared_ptr<const std::string> content = ContentProvider::singleton().requestContentString(id);
		if (content) {
			data = system->createSound(*content, is3D);
		}
	}

	return data;
}

std::size_t Sound::getMemoryUsed() const
{
	return data ? Audio::System::getMemoryUsed(*data) : 0;
}

void SoundService::openAudio()
{
	if (soundDisabled || mySoundDisabled || !GameSettings::singleton().soundEnabled) {
		return;
	}

	try {
		system = std::make_unique<Audio::System>();

		update3DSettings();
		if (ambientReverb != NoReverb) {
			updateAmbientReverb();
		}
	}
	catch (const std::exception& e) {
		StandardOut::singleton()->print(MESSAGE_WARNING, "%s", e.what());
	}
}

} // namespace RBX::Soundscape

static void releaseSound(const std::pair<RBX::Soundscape::SoundId, std::shared_ptr<RBX::Soundscape::Sound>>& p)
{
	p.second->release();
}

namespace RBX {
namespace Soundscape {

void SoundService::gcSounds(LoadedSounds& sounds)
{
	LoadedSounds::iterator iter = sounds.begin();
	while (iter != sounds.end()) {
		if (iter->second->refCount == 0 && G3D::uniformRandom() >= 0.7f) {
			iter->second->release();
			iter = sounds.erase(iter);
		}
		else {
			++iter;
		}
	}
}

void SoundChannel::onEvent(const RunService* source, Heartbeat event)
{
	if (!isPaused()) {
		update3D(channel);
	}
}

void Sound::release()
{
	if (data) {
		system->releaseSound(data);
		data.reset();
	}
}

Sound::~Sound()
{
	release();
}

void SoundService::closeAudio()
{
	stockSounds.clear();
	removeAllChildren();

	std::for_each(loadedSounds.begin(), loadedSounds.end(), releaseSound);
	std::for_each(loaded3DSounds.begin(), loaded3DSounds.end(), releaseSound);

	loadedSounds.clear();
	loaded3DSounds.clear();

	system.reset();
}

SoundService::~SoundService() = default;

} // namespace Soundscape

namespace Reflection {

template <>
EnumDesc<Soundscape::ReverbType>::EnumDesc() : EnumDescriptor("ReverbType", typeid(Soundscape::ReverbType))
{
	addPair(Soundscape::NoReverb, "NoReverb");
	addPair(Soundscape::GenericReverb, "GenericReverb");
	addPair(Soundscape::PaddedCell, "PaddedCell");
	addPair(Soundscape::Room, "Room");
	addPair(Soundscape::Bathroom, "Bathroom");
	addPair(Soundscape::LivingRoom, "LivingRoom");
	addPair(Soundscape::StoneRoom, "StoneRoom");
	addPair(Soundscape::Auditorium, "Auditorium");
	addPair(Soundscape::ConcertHall, "ConcertHall");
	addPair(Soundscape::Cave, "Cave");
	addPair(Soundscape::Arena, "Arena");
	addPair(Soundscape::Hangar, "Hangar");
	addPair(Soundscape::CarpettedHallway, "CarpettedHallway");
	addPair(Soundscape::Hallway, "Hallway");
	addPair(Soundscape::StoneCorridor, "StoneCorridor");
	addPair(Soundscape::Alley, "Alley");
	addPair(Soundscape::Forest, "Forest");
	addPair(Soundscape::City, "City");
	addPair(Soundscape::Mountains, "Mountains");
	addPair(Soundscape::Quarry, "Quarry");
	addPair(Soundscape::Plain, "Plain");
	addPair(Soundscape::ParkingLot, "ParkingLot");
	addPair(Soundscape::SewerPipe, "SewerPipe");
	addPair(Soundscape::UnderWater, "UnderWater");
}

} // namespace Reflection

namespace Soundscape {

std::shared_ptr<Sound> SoundService::loadSound(SoundId id, bool is3D)
{
	LoadedSounds& sounds = is3D ? loaded3DSounds : loadedSounds;

	LoadedSounds::iterator iter = sounds.find(id);
	if (iter != sounds.end()) {
		return iter->second;
	}

	std::shared_ptr<Sound> result(new Sound(system.get(), id, is3D));
	sounds[id] = result;
	return result;
}

} // namespace Soundscape

void registerSound()
{
	Soundscape::SoundChannel::classDescriptor();
	StockSound::classDescriptor();
	Soundscape::SoundService::classDescriptor();
}

namespace Soundscape {

SoundService::SoundService()
	: system(), dopplerscale(1.0f), distancefactor(10.0f), rolloffscale(1.0f), ambientReverb(NoReverb),
	  nextGarbageCollectTime(G3D::System::getTick()), mySoundDisabled(false)
{
	setName("Soundscape");
}

void SoundService::garbageCollectSounds()
{
	if (!(G3D::System::getTick() < nextGarbageCollectTime)) {
		gcSounds(loadedSounds);
		gcSounds(loaded3DSounds);
		nextGarbageCollectTime = G3D::System::getTick() + gcTime;
	}
}

void SoundService::onEvent(const RunService* source, Heartbeat event)
{
	if (system) {
		Workspace* workspace = ServiceProvider::find<Workspace>(this);
		if (workspace) {
			Camera* camera = workspace->getCamera();
			if (camera) {
				G3D::CoordinateFrame cf = camera->getCameraCoordinateFrame();

				Audio::Vector listener_pos;
				if (convert(cf.translation, listener_pos)) {
					Audio::Vector listener_forward;
					if (convert(-cf.lookVector(), listener_forward)) {
						Audio::Vector listener_up;
						if (convert(cf.upVector(), listener_up)) {
							Audio::Vector listener_vel;
							listener_vel.x = listener_vel.y = listener_vel.z = 0.0f;
							PVInstance* pvInstance = dynamic_cast<PVInstance*>(camera->getCameraSubjectInstance());
							if (pvInstance) {
								PartInstance* part = static_cast<IPrimaryPart*>(pvInstance)->getPrimaryPart();
								if (part) {
									convert(part->getVelocity().linear, listener_vel);
								}
							}

							system->set3DListenerAttributes(listener_pos, listener_vel, listener_forward, listener_up);
						}
					}
				}
			}
		}

		garbageCollectSounds();
		system->update();
	}
}

bool SoundChannel::askSetParent(const Instance* parent) const
{
	return true;
}

SoundChannel::SoundChannel()
	: sound(), channel(), volume(0.5f), playOnRemove(false), is3D(false), looped(false), soundDisabled(false),
	  playCount(-1), part(nullptr)
{
	setName("Sound");
}

void SoundChannel::updateListenState()
{
	if (part && is3D && channel) {
		startListening(ServiceProvider::find<RunService>(this), this);
	}
	else {
		stopListening(ServiceProvider::find<RunService>(this), this);
	}
}

} // namespace Soundscape
} // namespace RBX

static void callbackChannelEnd(void* userData)
{
	RBX::Soundscape::SoundChannel* sound = static_cast<RBX::Soundscape::SoundChannel*>(userData);
	if (sound) {
		sound->onChannelEnd();
	}
}

namespace RBX::Soundscape {

void SoundChannel::onChannelEnd()
{
	releaseChannel();
	updateListenState();
}

} // namespace RBX::Soundscape

SoundServiceStatsItem::SoundServiceStatsItem(const RBX::Soundscape::SoundService* service)
	: service(service), currentalloced(0), maxalloced(0)
{
	setName("Sound");
	createBoundMemChildItem("Current", currentalloced);
	createBoundMemChildItem("Max", maxalloced);
	createBoundChildItem("# Sounds", numSounds);
	createBoundChildItem("# Unused", numUnusedSounds);
}

void SoundServiceStatsItem::update()
{
	if (service->enabled()) {
		setValue(0, service->getVersion());

		numUnusedSounds = 0;
		numSounds = 0;
		std::size_t memoryUsed = 0;
		service->getSoundStats(numSounds, numUnusedSounds, memoryUsed);
		currentalloced = static_cast<unsigned int>(memoryUsed);
		maxalloced = std::max(maxalloced, currentalloced);
	}
	else {
		setValue(0, "-disabled-");
	}
}

namespace RBX::Soundscape {

void SoundChannel::playSound(Instance* context)
{
	if (soundDisabled) {
		return;
	}

	try {
		if (looped && channel.isPlaying()) {
			channel.setPaused(false);
			return;
		}

		SoundService* soundService = ServiceProvider::create<SoundService>(context);

		if (soundService) {
			if (!soundService->enabled()) {
				soundDisabled = true;
				return;
			}

			releaseChannel();

			is3D = part != nullptr;
			sound = soundService->loadSound(soundId, is3D);
			sound->refCount++;
			const std::shared_ptr<Audio::SoundData>& data = sound->get();
			if (data) {
				channel = soundService->system->playSound(data, true);

				updateLooped();

				update3D(channel);

				channel.setVolume(volume);

				channel.setUserData(this);
				channel.setEndCallback(callbackChannelEnd);

				channel.setPaused(false);

				updateListenState();
			}
		}
	}
	catch (std::exception& e) {
		StandardOut::singleton()->print(MESSAGE_ERROR, e.what());
	}
}

} // namespace RBX::Soundscape

StockSound::StockSound()
{
	propArchivable.setValue(this, false);
}

namespace RBX::Soundscape {

void SoundService::setAmbientReverb(const ReverbType& value)
{
	if (value != ambientReverb) {
		ambientReverb = value;

		raisePropertyChanged(prop_AmbientReverb);
		updateAmbientReverb();
	}
}

void SoundChannel::setSoundId(SoundId value)
{
	if (soundId != value) {
		soundId = value;

		if (!soundDisabled && GameSettings::singleton().soundEnabled) {
			ContentProvider::singleton().requestContentString(soundId);
		}

		releaseChannel();

		raisePropertyChanged(sound_desc_SoundId);
	}
}

void SoundChannel::setVolume(float value)
{
	value = G3D::clamp(value, 0.0f, 1.0f);
	if (volume != value) {
		volume = value;
		channel.setVolume(volume);

		raisePropertyChanged(sound_desc_Volume);
	}
}

void SoundChannel::setLooped(bool value)
{
	if (looped != value) {
		looped = value;
		updateLooped();

		raisePropertyChanged(sound_desc_Looped);
	}
}

void SoundChannel::play()
{
	playSound(this);
	playCount = std::max(playCount + 1, 1);
	raisePropertyChanged(sound_desc_PlayCount);
}

void SoundChannel::pause()
{
	channel.setPaused(true);

	updateListenState();

	if (playCount != 0) {
		playCount = 0;
		raisePropertyChanged(sound_desc_PlayCount);
	}
}

void SoundChannel::stop()
{
	if (channel) {
		channel.setUserData(nullptr);
		channel.stop();
		channel = Audio::Channel();
	}

	updateListenState();

	if (playCount != -1) {
		playCount = -1;
		raisePropertyChanged(sound_desc_PlayCount);
	}
}

void SoundService::loadStockSound(SoundType soundType, std::string file)
{
	std::shared_ptr<StockSound> sound = Creatable<Instance>::create<StockSound>();

	sound->setName(Reflection::EnumDesc<SoundType>::singleton().convertToString(soundType));
	sound->setSoundId(ContentId::fromAssets(file));

	sound->setParent(this);
	stockSounds[soundType] = sound;
}

void SoundService::playSound(SoundType soundType)
{
	StockSounds::iterator it = stockSounds.find(soundType);

	if (it != stockSounds.end()) {
		it->second->play();
	}
}

void SoundChannel::onAncestorChanged(const AncestorChanged& event)
{
	const ServiceProvider* oldProvider = ServiceProvider::findServiceProvider(event.oldParent);
	const ServiceProvider* newProvider = ServiceProvider::findServiceProvider(event.newParent);

	stopListening(oldProvider ? oldProvider->find<RunService>() : nullptr, this);

	if (oldProvider) {
		if (looped) {
			stop();
		}
		releaseChannel();
	}

	Instance::onAncestorChanged(event);

	if (event.child == this) {
		part = dynamic_cast<PartInstance*>(event.newParent);
		updateListenState();
	}

	if (newProvider) {
		SoundService* soundService = newProvider->create<SoundService>();
		if (soundService && soundService->enabled()) {
			soundDisabled = false;

			ContentProvider::singleton().requestContentString(soundId);

			updateListenState();
			if (playCount > 0) {
				playSound(this);
			}
		}
	}

	if (oldProvider) {
		Workspace* oldWorkspace = oldProvider->find<Workspace>();
		if (oldWorkspace && (event.oldParent == oldWorkspace || event.oldParent->isDescendentOf(oldWorkspace))) {
			if (newProvider) {
				Workspace* newWorkspace = newProvider->find<Workspace>();
				if (newWorkspace &&
					(event.newParent == newWorkspace || event.newParent->isDescendentOf(newWorkspace))) {
					return;
				}
			}

			if (playOnRemove) {
				playSound(event.oldParent);
			}
			releaseChannel();
		}
	}
}

void SoundChannel::setPlayCount(int value)
{
	if (value == -1) {
		stop();
	}
	else if (value == 0) {
		pause();
	}
	else if (value > playCount) {
		playSound(this);
		playCount = value;
		raisePropertyChanged(sound_desc_PlayCount);
	}
}

void SoundService::loadStockSounds()
{
	stockSounds.clear();
	removeAllChildren();

	loadStockSound(BOING_SOUND, "Sounds\\bass.wav");
	loadStockSound(BOMB_SOUND, "Sounds\\collide.wav");
	loadStockSound(BREAK_SOUND, "Sounds\\glassbreak.wav");
	loadStockSound(CLICK_SOUND, "Sounds\\switch.wav");
	loadStockSound(CLOCK_SOUND, "Sounds\\clickfast.wav");
	loadStockSound(RUBBERBAND_SOUND, "Sounds\\Rubber band sling shot.wav");
	loadStockSound(PAGE_SOUND, "Sounds\\pageturn.wav");
	loadStockSound(PING_SOUND, "Sounds\\electronicpingshort.wav");
	loadStockSound(SNAP_SOUND, "Sounds\\snap.wav");
	loadStockSound(SPLAT_SOUND, "Sounds\\splat.wav");
	loadStockSound(STEP_SOUND, "Sounds\\SWITCH3.wav");
	loadStockSound(STEP_ON_SOUND, "Sounds\\flashbulb.wav");
	loadStockSound(SWOOSH_SOUND, "Sounds\\swoosh.wav");
	loadStockSound(VICTORY_SOUND, "Sounds\\victory.wav");
}

void SoundService::onServiceProvider(const ServiceProvider* oldProvider, const ServiceProvider* newProvider)
{
	stopListening(oldProvider ? oldProvider->find<RunService>() : nullptr, this);

	if (statsItem) {
		statsItem->setParent(nullptr);
		statsItem.reset();
	}

	if (oldProvider) {
		closeAudio();
	}

	Instance::onServiceProvider(oldProvider, newProvider);

	startListening(newProvider ? newProvider->find<RunService>() : nullptr, this);

	if (newProvider) {
		openAudio();
		loadStockSounds();

		Stats::StatsService* stats = newProvider->create<Stats::StatsService>();
		if (stats) {
			statsItem = Creatable<Instance>::create<SoundServiceStatsItem>(this);
			statsItem->setParent(stats);
		}
	}
}

} // namespace RBX::Soundscape

template const RBX::Name& RBX::Name::doDeclare<RBX::sGameSettings>();
template const RBX::Name& RBX::Name::doDeclare<sStockSound>();
template void RBX::Name::callDoDeclare<RBX::sGameSettings>();
template void RBX::Name::callDoDeclare<sStockSound>();
template RBX::GameSettings& RBX::GlobalSettingsItem<RBX::GameSettings, RBX::sGameSettings>::singleton();
template const RBX::FactoryProduct<StockSound, RBX::Soundscape::SoundChannel, sStockSound>::Creator
	RBX::FactoryProduct<StockSound, RBX::Soundscape::SoundChannel, sStockSound>::creator;
