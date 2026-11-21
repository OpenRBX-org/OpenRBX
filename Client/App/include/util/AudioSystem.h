#ifndef UTIL_AUDIOSYSTEM_H
#define UTIL_AUDIOSYSTEM_H

#include <cstddef>
#include <memory>
#include <string>

namespace RBX::Audio {

struct Vector
{
	float x;
	float y;
	float z;
};

struct ReverbProperties
{
	int room;
	int roomHF;
	int roomLF;
	float decayTime;
	float decayHFRatio;
	int reflections;
	float reflectionsDelay;
	int reverb;
	float reverbDelay;
	float hfReference;
};

namespace Preset {

extern const ReverbProperties Off;
extern const ReverbProperties Generic;
extern const ReverbProperties PaddedCell;
extern const ReverbProperties Bathroom;
extern const ReverbProperties LivingRoom;
extern const ReverbProperties StoneRoom;
extern const ReverbProperties Auditorium;
extern const ReverbProperties ConcertHall;
extern const ReverbProperties Cave;
extern const ReverbProperties Arena;
extern const ReverbProperties Hangar;
extern const ReverbProperties CarpettedHallway;
extern const ReverbProperties Hallway;
extern const ReverbProperties StoneCorridor;
extern const ReverbProperties Alley;
extern const ReverbProperties Forest;
extern const ReverbProperties City;
extern const ReverbProperties Mountains;
extern const ReverbProperties Quarry;
extern const ReverbProperties Plain;
extern const ReverbProperties ParkingLot;
extern const ReverbProperties SewerPipe;
extern const ReverbProperties UnderWater;

} // namespace Preset

class System;
struct SoundData;
struct Voice;

using EndCallback = void (*)(void* userData);

class Channel
{
public:
	Channel() = default;

	explicit operator bool() const { return system != nullptr; }

	bool isPlaying() const;
	bool isPaused() const;

	void setPaused(bool paused) const;
	void setVolume(float volume) const;
	void setLooped(bool looped) const;
	void set3DAttributes(const Vector& position, const Vector& velocity) const;
	void setUserData(void* userData) const;
	void setEndCallback(EndCallback callback) const;
	void stop() const;

private:
	friend class System;

	Channel(System* system, int index, unsigned int generation) : system(system), index(index), generation(generation)
	{
	}

	System* system = nullptr;
	int index = 0;
	unsigned int generation = 0;
};

class System
{
public:
	System();
	~System();

	System(const System&) = delete;
	System& operator=(const System&) = delete;

	std::string getVersion() const;

	bool set3DSettings(float dopplerScale, float distanceFactor, float rolloffScale);
	void set3DListenerAttributes(
		const Vector& position, const Vector& velocity, const Vector& forward, const Vector& up
	);
	void setReverbProperties(const ReverbProperties& properties);

	std::shared_ptr<SoundData> createSound(const std::string& data, bool is3D);
	void releaseSound(const std::shared_ptr<SoundData>& sound);
	Channel playSound(const std::shared_ptr<SoundData>& sound, bool paused);

	void update();

	static std::size_t getMemoryUsed(const SoundData& sound);

private:
	friend class Channel;

	static Voice* find(const Channel& channel);

	struct Impl;
	std::unique_ptr<Impl> impl;
};

} // namespace RBX::Audio

#endif // UTIL_AUDIOSYSTEM_H
