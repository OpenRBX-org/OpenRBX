#include "util/AudioSystem.h"

#define AL_ALEXT_PROTOTYPES
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
#include <AL/efx.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <libopenmpt/libopenmpt.h>
#include <stdexcept>
#include <vector>

#define DR_WAV_IMPLEMENTATION
#include <dr_wav.h>
#define DR_MP3_IMPLEMENTATION
#include <dr_mp3.h>
#define DR_FLAC_IMPLEMENTATION
#include <dr_flac.h>
#define STB_VORBIS_HEADER_ONLY
#include <stb_vorbis.c>

namespace RBX::Audio {

namespace Preset {

const ReverbProperties Off = {-10000, -10000, 0, 1.00f, 1.00f, -2602, 0.007f, 200, 0.011f, 5000.0f};
const ReverbProperties Generic = {-1000, -100, 0, 1.49f, 0.83f, -2602, 0.007f, 200, 0.011f, 5000.0f};
const ReverbProperties PaddedCell = {-1000, -6000, 0, 0.17f, 0.10f, -1204, 0.001f, 207, 0.002f, 5000.0f};
const ReverbProperties Bathroom = {-1000, -1200, 0, 1.49f, 0.54f, -370, 0.007f, 1030, 0.011f, 5000.0f};
const ReverbProperties LivingRoom = {-1000, -6000, 0, 0.50f, 0.10f, -1376, 0.003f, -1104, 0.004f, 5000.0f};
const ReverbProperties StoneRoom = {-1000, -300, 0, 2.31f, 0.64f, -711, 0.012f, 83, 0.017f, 5000.0f};
const ReverbProperties Auditorium = {-1000, -476, 0, 4.32f, 0.59f, -789, 0.020f, -289, 0.030f, 5000.0f};
const ReverbProperties ConcertHall = {-1000, -500, 0, 3.92f, 0.70f, -1230, 0.020f, -2, 0.029f, 5000.0f};
const ReverbProperties Cave = {-1000, 0, 0, 2.91f, 1.30f, -602, 0.015f, -302, 0.022f, 5000.0f};
const ReverbProperties Arena = {-1000, -698, 0, 7.24f, 0.33f, -1166, 0.020f, 16, 0.030f, 5000.0f};
const ReverbProperties Hangar = {-1000, -1000, 0, 10.05f, 0.23f, -602, 0.020f, 198, 0.030f, 5000.0f};
const ReverbProperties CarpettedHallway = {-1000, -4000, 0, 0.30f, 0.10f, -1831, 0.002f, -1630, 0.030f, 5000.0f};
const ReverbProperties Hallway = {-1000, -300, 0, 1.49f, 0.59f, -1219, 0.007f, 441, 0.011f, 5000.0f};
const ReverbProperties StoneCorridor = {-1000, -237, 0, 2.70f, 0.79f, -1214, 0.013f, 395, 0.020f, 5000.0f};
const ReverbProperties Alley = {-1000, -270, 0, 1.49f, 0.86f, -1204, 0.007f, -4, 0.011f, 5000.0f};
const ReverbProperties Forest = {-1000, -3300, 0, 1.49f, 0.54f, -2560, 0.162f, -229, 0.088f, 5000.0f};
const ReverbProperties City = {-1000, -800, 0, 1.49f, 0.67f, -2273, 0.007f, -1691, 0.011f, 5000.0f};
const ReverbProperties Mountains = {-1000, -2500, 0, 1.49f, 0.21f, -2780, 0.300f, -1434, 0.100f, 5000.0f};
const ReverbProperties Quarry = {-1000, -1000, 0, 1.49f, 0.83f, -10000, 0.061f, 500, 0.025f, 5000.0f};
const ReverbProperties Plain = {-1000, -2000, 0, 1.49f, 0.50f, -2466, 0.179f, -1926, 0.100f, 5000.0f};
const ReverbProperties ParkingLot = {-1000, 0, 0, 1.65f, 1.50f, -1363, 0.008f, -1153, 0.012f, 5000.0f};
const ReverbProperties SewerPipe = {-1000, -1000, 0, 2.81f, 0.14f, 429, 0.014f, 1023, 0.021f, 5000.0f};
const ReverbProperties UnderWater = {-1000, -4000, 0, 1.49f, 0.10f, -449, 0.007f, 1700, 0.011f, 5000.0f};

} // namespace Preset

static const int maxChannels = 100;
static const float speedOfSound = 340.0f;
static const float centrePan = 0.70710678f;
static const int moduleSampleRate = 48000;

struct SoundData
{
	bool is3D = false;
	float minDistance = 1.0f;
	float maxDistance = 10000.0f;
	int channels = 0;
	int sampleRate = 0;
	std::vector<float> samples;
	std::size_t frames = 0;
	std::size_t loopStart = 0;
	std::size_t loopEnd = 0;
	std::string module;
	std::int32_t moduleGain = 0;
	std::int32_t moduleSeparation = 100;
};

struct Voice
{
	ALuint source = 0;
	ALuint buffer = 0;
	unsigned int generation = 0;
	bool active = false;
	bool paused = false;
	std::shared_ptr<SoundData> sound;
	openmpt_module* module = nullptr;
	std::size_t position = 0;
	std::atomic<bool> looped = false;
	std::atomic<float> gainLeft = centrePan;
	std::atomic<float> gainRight = centrePan;
	float volume = 1.0f;
	float attenuation = 1.0f;
	Vector position3D = {};
	Vector velocity3D = {};
	void* userData = nullptr;
	EndCallback endCallback = nullptr;
};

struct System::Impl
{
	ALCdevice* device = nullptr;
	ALCcontext* context = nullptr;
	ALuint effect = 0;
	ALuint slot = 0;
	ALint linearResampler = 0;
	float dopplerScale = 1.0f;
	float distanceFactor = 1.0f;
	float rolloffScale = 1.0f;
	Vector listenerPosition = {};
	Vector listenerVelocity = {};
	Vector listenerForward = {0.0f, 0.0f, 1.0f};
	Vector listenerUp = {0.0f, 1.0f, 0.0f};
	std::array<Voice, maxChannels> voices;

	~Impl();

	void makeCurrent() const { alcSetThreadContext(context); }
	void apply3D(Voice& voice);
	void stop(Voice& voice);
	void end(Voice& voice);
};

static Vector subtract(const Vector& a, const Vector& b)
{
	return {a.x - b.x, a.y - b.y, a.z - b.z};
}

static float dot(const Vector& a, const Vector& b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

static float length(const Vector& v)
{
	return std::sqrt(dot(v, v));
}

static Vector cross(const Vector& a, const Vector& b)
{
	return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

static float millibelsToGain(int millibels)
{
	return std::pow(10.0f, millibels / 2000.0f);
}

static bool hasTag(const std::string& data, std::size_t offset, const char* tag)
{
	std::size_t size = std::strlen(tag);
	return data.size() >= offset + size && data.compare(offset, size, tag) == 0;
}

static bool decodeWav(const std::string& data, SoundData& sound)
{
	drwav wav;
	if (!drwav_init_memory_with_metadata(&wav, data.data(), data.size(), 0, nullptr)) {
		return false;
	}

	sound.channels = static_cast<int>(wav.channels);
	sound.sampleRate = static_cast<int>(wav.sampleRate);
	sound.samples.resize(static_cast<std::size_t>(wav.totalPCMFrameCount) * wav.channels);
	sound.frames =
		static_cast<std::size_t>(drwav_read_pcm_frames_f32(&wav, wav.totalPCMFrameCount, sound.samples.data()));
	sound.loopEnd = sound.frames;
	for (drwav_uint32 i = 0; i < wav.metadataCount; i++) {
		const drwav_metadata& metadata = wav.pMetadata[i];
		if (metadata.type == drwav_metadata_type_smpl && metadata.data.smpl.sampleLoopCount > 0) {
			const drwav_smpl_loop& loop = metadata.data.smpl.pLoops[0];
			sound.loopStart = std::min<std::size_t>(loop.firstSampleOffset, sound.frames);
			sound.loopEnd =
				std::clamp<std::size_t>(std::size_t(loop.lastSampleOffset) + 1, sound.loopStart, sound.frames);
			break;
		}
	}
	drwav_uninit(&wav);
	return true;
}

static bool decodeFlac(const std::string& data, SoundData& sound)
{
	drflac* flac = drflac_open_memory(data.data(), data.size(), nullptr);
	if (!flac) {
		return false;
	}

	sound.channels = flac->channels;
	sound.sampleRate = static_cast<int>(flac->sampleRate);
	sound.samples.resize(static_cast<std::size_t>(flac->totalPCMFrameCount) * flac->channels);
	sound.frames =
		static_cast<std::size_t>(drflac_read_pcm_frames_f32(flac, flac->totalPCMFrameCount, sound.samples.data()));
	sound.loopEnd = sound.frames;
	drflac_close(flac);
	return true;
}

static bool decodeVorbis(const std::string& data, SoundData& sound)
{
	int error = 0;
	stb_vorbis* vorbis = stb_vorbis_open_memory(
		reinterpret_cast<const unsigned char*>(data.data()), static_cast<int>(data.size()), &error, nullptr
	);
	if (!vorbis) {
		return false;
	}

	stb_vorbis_info info = stb_vorbis_get_info(vorbis);
	sound.channels = info.channels;
	sound.sampleRate = static_cast<int>(info.sample_rate);
	sound.samples.resize(std::size_t(stb_vorbis_stream_length_in_samples(vorbis)) * info.channels);
	sound.frames = static_cast<std::size_t>(stb_vorbis_get_samples_float_interleaved(
		vorbis, info.channels, sound.samples.data(), static_cast<int>(sound.samples.size())
	));
	sound.loopEnd = sound.frames;
	stb_vorbis_close(vorbis);
	return true;
}

static bool decodeMpeg(const std::string& data, SoundData& sound)
{
	drmp3 mp3;
	if (!drmp3_init_memory(&mp3, data.data(), data.size(), nullptr)) {
		return false;
	}

	drmp3_uint64 frames = drmp3_get_pcm_frame_count(&mp3);
	sound.channels = static_cast<int>(mp3.channels);
	sound.sampleRate = static_cast<int>(mp3.sampleRate);
	sound.samples.resize(static_cast<std::size_t>(frames) * mp3.channels);
	sound.frames = static_cast<std::size_t>(drmp3_read_pcm_frames_f32(&mp3, frames, sound.samples.data()));
	sound.loopEnd = sound.frames;
	drmp3_uninit(&mp3);
	return sound.frames > 0;
}

static bool decodeModule(const std::string& data, SoundData& sound)
{
	if (openmpt_probe_file_header_without_filesize(
			OPENMPT_PROBE_FILE_HEADER_FLAGS_MODULES,
			data.data(),
			data.size(),
			openmpt_log_func_silent,
			nullptr,
			openmpt_error_func_ignore,
			nullptr,
			nullptr,
			nullptr
		) != OPENMPT_PROBE_FILE_HEADER_RESULT_SUCCESS) {
		return false;
	}

	openmpt_module* module = openmpt_module_create_from_memory2(
		data.data(),
		data.size(),
		openmpt_log_func_silent,
		nullptr,
		openmpt_error_func_ignore,
		nullptr,
		nullptr,
		nullptr,
		nullptr
	);
	if (!module) {
		return false;
	}

	const char* type = openmpt_module_get_metadata(module, "type");
	const std::string format = type ? type : "";
	openmpt_free_string(type);
	const std::int32_t numChannels = openmpt_module_get_num_channels(module);
	openmpt_module_destroy(module);

	sound.module = data;
	if (format == "mod") {
		const double preAmp = std::clamp(256 / std::max(numChannels, 1), 32, 128);
		sound.moduleGain = 204 + static_cast<std::int32_t>(std::lround(2000.0 * std::log10(64.0 / preAmp)));
	}
	else if (format == "s3m") {
		const bool stereo = (data[0x33] & 0x80) != 0;
		sound.module[0x30] = 64;
		sound.module[0x33] = static_cast<char>(stereo ? 0xB0 : 0x30);
		sound.moduleGain = stereo ? 454 : 825;
		sound.moduleSeparation = 83;
	}
	else if (format == "xm") {
		sound.moduleGain = 551;
	}
	else if (format == "it") {
		sound.moduleGain = -302;
	}
	else {
		sound.module.clear();
		return false;
	}

	sound.channels = 2;
	sound.sampleRate = moduleSampleRate;
	return true;
}

static void decode(const std::string& data, SoundData& sound)
{
	bool decoded;
	if (hasTag(data, 0, "RIFF") || hasTag(data, 0, "FORM")) {
		decoded = decodeWav(data, sound);
	}
	else if (hasTag(data, 0, "fLaC")) {
		decoded = decodeFlac(data, sound);
	}
	else if (hasTag(data, 0, "OggS")) {
		decoded = decodeVorbis(data, sound);
	}
	else if (decodeModule(data, sound)) {
		decoded = true;
	}
	else {
		decoded = decodeMpeg(data, sound);
	}

	if (!decoded) {
		throw std::runtime_error("Unsupported sound format");
	}
	if (sound.channels != 1 && sound.channels != 2) {
		throw std::runtime_error("Unsupported sound channel count");
	}
}

static std::size_t renderSamples(Voice& voice, float* out, std::size_t frames)
{
	const SoundData& sound = *voice.sound;
	const bool looped = voice.looped.load(std::memory_order_relaxed);
	const float left = voice.gainLeft.load(std::memory_order_relaxed);
	const float right = voice.gainRight.load(std::memory_order_relaxed);
	const bool panned = sound.is3D || sound.channels == 1;

	std::size_t written = 0;
	while (written < frames) {
		std::size_t end = looped ? sound.loopEnd : sound.frames;
		if (voice.position >= end) {
			if (!looped || sound.loopEnd <= sound.loopStart) {
				break;
			}
			voice.position = sound.loopStart;
		}

		std::size_t count = std::min(frames - written, end - voice.position);
		const float* in = sound.samples.data() + voice.position * sound.channels;
		for (std::size_t i = 0; i < count; i++) {
			if (panned) {
				float mono = sound.channels == 1 ? in[i] : in[i * 2] + in[i * 2 + 1];
				out[(written + i) * 2] = mono * left;
				out[(written + i) * 2 + 1] = mono * right;
			}
			else {
				out[(written + i) * 2] = in[i * 2];
				out[(written + i) * 2 + 1] = in[i * 2 + 1];
			}
		}
		voice.position += count;
		written += count;
	}
	return written;
}

static std::size_t renderModule(Voice& voice, float* out, std::size_t frames)
{
	std::size_t written = openmpt_module_read_interleaved_float_stereo(voice.module, moduleSampleRate, frames, out);
	if (voice.sound->is3D) {
		const float left = voice.gainLeft.load(std::memory_order_relaxed);
		const float right = voice.gainRight.load(std::memory_order_relaxed);
		for (std::size_t i = 0; i < written; i++) {
			float mono = out[i * 2] + out[i * 2 + 1];
			out[i * 2] = mono * left;
			out[i * 2 + 1] = mono * right;
		}
	}
	return written;
}

static ALsizei AL_APIENTRY renderVoice(ALvoid* userptr, ALvoid* sampledata, ALsizei numbytes) noexcept
{
	Voice& voice = *static_cast<Voice*>(userptr);
	float* out = static_cast<float*>(sampledata);
	std::size_t frames = static_cast<std::size_t>(numbytes) / (2 * sizeof(float));
	std::size_t written = voice.module ? renderModule(voice, out, frames) : renderSamples(voice, out, frames);
	return static_cast<ALsizei>(written * 2 * sizeof(float));
}

void System::Impl::apply3D(Voice& voice)
{
	const SoundData& sound = *voice.sound;
	Vector relative = subtract(voice.position3D, listenerPosition);

	float distance = std::clamp(length(relative), sound.minDistance, sound.maxDistance);
	voice.attenuation = sound.minDistance / (sound.minDistance + rolloffScale * (distance - sound.minDistance));

	float x = dot(relative, cross(listenerUp, listenerForward));
	float z = dot(relative, listenerForward);
	float extent = std::fabs(x) + std::fabs(z);
	float pan = extent > 0.0f ? x / extent : 0.0f;
	voice.gainLeft.store(std::sqrt((1.0f - pan) * 0.5f), std::memory_order_relaxed);
	voice.gainRight.store(std::sqrt((1.0f + pan) * 0.5f), std::memory_order_relaxed);

	Vector motion = subtract(voice.velocity3D, listenerVelocity);
	float closing = (length(subtract(relative, motion)) - length(relative)) / distanceFactor;
	float pitch = std::max(1.0f + dopplerScale * closing / speedOfSound, 0.0f);

	alSourcef(voice.source, AL_GAIN, voice.volume * voice.attenuation);
	alSourcef(voice.source, AL_PITCH, pitch);
}

void System::Impl::stop(Voice& voice)
{
	alSourceStop(voice.source);
	alSourcei(voice.source, AL_BUFFER, 0);
	if (voice.module) {
		openmpt_module_destroy(voice.module);
		voice.module = nullptr;
	}
	voice.sound.reset();
	voice.active = false;
	voice.generation++;
}

System::Impl::~Impl()
{
	if (context) {
		makeCurrent();
		for (Voice& voice : voices) {
			if (voice.active) {
				stop(voice);
			}
			alDeleteSources(1, &voice.source);
			alDeleteBuffers(1, &voice.buffer);
		}
		alDeleteAuxiliaryEffectSlots(1, &slot);
		alDeleteEffects(1, &effect);
		alcSetThreadContext(nullptr);
		alcDestroyContext(context);
	}
	if (device) {
		alcCloseDevice(device);
	}
}

void System::Impl::end(Voice& voice)
{
	void* userData = voice.userData;
	EndCallback callback = voice.endCallback;
	stop(voice);
	if (callback) {
		callback(userData);
	}
}

System::System() : impl(std::make_unique<Impl>())
{
	impl->device = alcOpenDevice(nullptr);
	if (!impl->device) {
		throw std::runtime_error("No active sound driver detected");
	}

	const ALCint attributes[] = {
		ALC_HRTF_SOFT, ALC_FALSE, ALC_MONO_SOURCES, 0, ALC_STEREO_SOURCES, maxChannels, ALC_MAX_AUXILIARY_SENDS, 1, 0
	};
	impl->context = alcCreateContext(impl->device, attributes);
	if (!impl->context) {
		throw std::runtime_error("Unable to create the sound context");
	}

	impl->makeCurrent();
	alDistanceModel(AL_NONE);
	alDopplerFactor(0.0f);

	ALint resamplers = alGetInteger(AL_NUM_RESAMPLERS_SOFT);
	for (ALint i = 0; i < resamplers; i++) {
		if (!std::strcmp(alGetStringiSOFT(AL_RESAMPLER_NAME_SOFT, i), "Linear")) {
			impl->linearResampler = i;
		}
	}

	alGenEffects(1, &impl->effect);
	alEffecti(impl->effect, AL_EFFECT_TYPE, AL_EFFECT_EAXREVERB);
	alGenAuxiliaryEffectSlots(1, &impl->slot);

	for (Voice& voice : impl->voices) {
		alGenSources(1, &voice.source);
		alGenBuffers(1, &voice.buffer);
		alSourcei(voice.source, AL_SOURCE_RELATIVE, AL_TRUE);
		alSourcei(voice.source, AL_DIRECT_CHANNELS_SOFT, AL_TRUE);
		alSourcei(voice.source, AL_SOURCE_RESAMPLER_SOFT, impl->linearResampler);
		alSource3i(voice.source, AL_AUXILIARY_SEND_FILTER, static_cast<ALint>(impl->slot), 0, AL_FILTER_NULL);
	}

	if (alGetError() != AL_NO_ERROR) {
		throw std::runtime_error("Unable to initialise sound voices");
	}
}

System::~System() = default;

std::string System::getVersion() const
{
	impl->makeCurrent();
	return alGetString(AL_VERSION);
}

bool System::set3DSettings(float dopplerScale, float distanceFactor, float rolloffScale)
{
	if (dopplerScale < 0.0f || distanceFactor <= 0.0f || rolloffScale < 0.0f) {
		return false;
	}

	impl->dopplerScale = dopplerScale;
	impl->distanceFactor = distanceFactor;
	impl->rolloffScale = rolloffScale;
	return true;
}

void System::set3DListenerAttributes(
	const Vector& position, const Vector& velocity, const Vector& forward, const Vector& up
)
{
	impl->listenerPosition = position;
	impl->listenerVelocity = velocity;
	impl->listenerForward = forward;
	impl->listenerUp = up;
}

void System::setReverbProperties(const ReverbProperties& properties)
{
	impl->makeCurrent();
	ALuint effect = impl->effect;
	const float zero[3] = {};
	alEffectf(effect, AL_EAXREVERB_DENSITY, 1.0f);
	alEffectf(effect, AL_EAXREVERB_DIFFUSION, 1.0f);
	alEffectf(effect, AL_EAXREVERB_GAIN, millibelsToGain(properties.room));
	alEffectf(effect, AL_EAXREVERB_GAINHF, millibelsToGain(properties.roomHF));
	alEffectf(effect, AL_EAXREVERB_GAINLF, millibelsToGain(properties.roomLF));
	alEffectf(effect, AL_EAXREVERB_DECAY_TIME, properties.decayTime);
	alEffectf(effect, AL_EAXREVERB_DECAY_HFRATIO, properties.decayHFRatio);
	alEffectf(effect, AL_EAXREVERB_DECAY_LFRATIO, 1.0f);
	alEffectf(effect, AL_EAXREVERB_REFLECTIONS_GAIN, millibelsToGain(properties.reflections));
	alEffectf(effect, AL_EAXREVERB_REFLECTIONS_DELAY, properties.reflectionsDelay);
	alEffectfv(effect, AL_EAXREVERB_REFLECTIONS_PAN, zero);
	alEffectf(effect, AL_EAXREVERB_LATE_REVERB_GAIN, millibelsToGain(properties.reverb));
	alEffectf(effect, AL_EAXREVERB_LATE_REVERB_DELAY, properties.reverbDelay);
	alEffectfv(effect, AL_EAXREVERB_LATE_REVERB_PAN, zero);
	alEffectf(effect, AL_EAXREVERB_ECHO_DEPTH, 0.0f);
	alEffectf(effect, AL_EAXREVERB_MODULATION_DEPTH, 0.0f);
	alEffectf(effect, AL_EAXREVERB_AIR_ABSORPTION_GAINHF, 1.0f);
	alEffectf(effect, AL_EAXREVERB_HFREFERENCE, properties.hfReference);
	alEffectf(effect, AL_EAXREVERB_ROOM_ROLLOFF_FACTOR, 0.0f);
	alEffecti(effect, AL_EAXREVERB_DECAY_HFLIMIT, AL_FALSE);
	alAuxiliaryEffectSloti(impl->slot, AL_EFFECTSLOT_EFFECT, static_cast<ALint>(effect));
}

std::shared_ptr<SoundData> System::createSound(const std::string& data, bool is3D)
{
	std::shared_ptr<SoundData> sound = std::make_shared<SoundData>();
	decode(data, *sound);
	sound->is3D = is3D;
	sound->minDistance = impl->distanceFactor;
	sound->maxDistance = 10000.0f * impl->distanceFactor;
	return sound;
}

void System::releaseSound(const std::shared_ptr<SoundData>& sound)
{
	impl->makeCurrent();
	for (Voice& voice : impl->voices) {
		if (voice.active && voice.sound == sound) {
			impl->stop(voice);
		}
	}
}

Channel System::playSound(const std::shared_ptr<SoundData>& sound, bool paused)
{
	impl->makeCurrent();
	Voice* chosen = nullptr;
	for (int i = maxChannels - 1; i >= 0 && !chosen; i--) {
		if (!impl->voices[i].active) {
			chosen = &impl->voices[i];
		}
	}
	if (!chosen) {
		for (int i = maxChannels - 1; i >= 0; i--) {
			Voice& voice = impl->voices[i];
			if (!chosen || voice.volume * voice.attenuation < chosen->volume * chosen->attenuation) {
				chosen = &voice;
			}
		}
		impl->end(*chosen);
	}

	Voice& voice = *chosen;
	if (!sound->module.empty()) {
		voice.module = openmpt_module_create_from_memory2(
			sound->module.data(),
			sound->module.size(),
			openmpt_log_func_silent,
			nullptr,
			openmpt_error_func_ignore,
			nullptr,
			nullptr,
			nullptr,
			nullptr
		);
		if (!voice.module) {
			throw std::runtime_error("Unable to load the tracker module");
		}
		openmpt_module_set_render_param(voice.module, OPENMPT_MODULE_RENDER_INTERPOLATIONFILTER_LENGTH, 2);
		openmpt_module_set_render_param(voice.module, OPENMPT_MODULE_RENDER_MASTERGAIN_MILLIBEL, sound->moduleGain);
		openmpt_module_set_render_param(
			voice.module, OPENMPT_MODULE_RENDER_STEREOSEPARATION_PERCENT, sound->moduleSeparation
		);
		openmpt_module_set_repeat_count(voice.module, -1);
	}

	voice.sound = sound;
	voice.active = true;
	voice.paused = paused;
	voice.position = 0;
	voice.looped.store(false, std::memory_order_relaxed);
	voice.volume = 1.0f;
	voice.attenuation = 1.0f;
	voice.position3D = {};
	voice.velocity3D = {};
	voice.userData = nullptr;
	voice.endCallback = nullptr;
	voice.gainLeft.store(centrePan, std::memory_order_relaxed);
	voice.gainRight.store(centrePan, std::memory_order_relaxed);

	alBufferCallbackSOFT(voice.buffer, AL_FORMAT_STEREO_FLOAT32, sound->sampleRate, renderVoice, &voice);
	alSourcei(voice.source, AL_BUFFER, static_cast<ALint>(voice.buffer));
	alSourcef(voice.source, AL_GAIN, 1.0f);
	alSourcef(voice.source, AL_PITCH, 1.0f);
	if (sound->is3D) {
		impl->apply3D(voice);
	}
	if (!paused) {
		alSourcePlay(voice.source);
	}

	return Channel(this, static_cast<int>(chosen - impl->voices.data()), voice.generation);
}

void System::update()
{
	impl->makeCurrent();
	for (Voice& voice : impl->voices) {
		if (!voice.active) {
			continue;
		}

		ALint state = AL_INITIAL;
		alGetSourcei(voice.source, AL_SOURCE_STATE, &state);
		if (state == AL_STOPPED) {
			impl->end(voice);
		}
		else if (voice.sound->is3D) {
			impl->apply3D(voice);
		}
	}
}

std::size_t System::getMemoryUsed(const SoundData& sound)
{
	return sound.samples.size() * sizeof(float) + sound.module.size();
}

Voice* System::find(const Channel& channel)
{
	if (!channel) {
		return nullptr;
	}

	Voice& voice = channel.system->impl->voices[channel.index];
	return voice.active && voice.generation == channel.generation ? &voice : nullptr;
}

bool Channel::isPlaying() const
{
	Voice* voice = System::find(*this);
	if (!voice) {
		return false;
	}

	system->impl->makeCurrent();
	ALint state = AL_INITIAL;
	alGetSourcei(voice->source, AL_SOURCE_STATE, &state);
	return state != AL_STOPPED;
}

bool Channel::isPaused() const
{
	Voice* voice = System::find(*this);
	return !voice || voice->paused;
}

void Channel::setPaused(bool paused) const
{
	if (Voice* voice = System::find(*this)) {
		voice->paused = paused;
		system->impl->makeCurrent();
		if (paused) {
			alSourcePause(voice->source);
		}
		else {
			alSourcePlay(voice->source);
		}
	}
}

void Channel::setVolume(float volume) const
{
	if (Voice* voice = System::find(*this)) {
		voice->volume = volume;
		system->impl->makeCurrent();
		alSourcef(voice->source, AL_GAIN, voice->volume * voice->attenuation);
	}
}

void Channel::setLooped(bool looped) const
{
	if (Voice* voice = System::find(*this)) {
		voice->looped.store(looped, std::memory_order_relaxed);
	}
}

void Channel::set3DAttributes(const Vector& position, const Vector& velocity) const
{
	if (Voice* voice = System::find(*this)) {
		voice->position3D = position;
		voice->velocity3D = velocity;
		if (voice->sound->is3D) {
			system->impl->makeCurrent();
			system->impl->apply3D(*voice);
		}
	}
}

void Channel::setUserData(void* userData) const
{
	if (Voice* voice = System::find(*this)) {
		voice->userData = userData;
	}
}

void Channel::setEndCallback(EndCallback callback) const
{
	if (Voice* voice = System::find(*this)) {
		voice->endCallback = callback;
	}
}

void Channel::stop() const
{
	if (Voice* voice = System::find(*this)) {
		system->impl->makeCurrent();
		system->impl->stop(*voice);
	}
}

} // namespace RBX::Audio
