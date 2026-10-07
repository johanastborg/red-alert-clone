#pragma once

#include <vector>
#include <unordered_map>
#include <cstdint>

#define GL_SILENCE_DEPRECATION
#include <OpenAL/al.h>
#include <OpenAL/alc.h>

enum class SoundId {
    CLICK,
    SELECT,
    MOVE_ORDER,
    ATTACK_ORDER,
    RIFLE_FIRE,
    TANK_CANNON,
    MAMMOTH_FIRE,
    TESLA_CHARGE,
    TESLA_ZAP,
    ROCKET_LAUNCH,
    EXPLOSION_SMALL,
    EXPLOSION_LARGE,
    CREDITS_TICK,
    BUILDING_PLACE,
    BUILDING_READY,
    UNIT_READY,
    BASE_ALARM,
    POWER_LOW,
    RADAR_PING,
    VICTORY_FANFARE
};

class AudioSystem {
public:
    AudioSystem();
    ~AudioSystem();

    bool Initialize();
    void Shutdown();

    void Play(SoundId id, float volume = 1.0f, float pitch = 1.0f);
    void PlayAt(SoundId id, float worldX, float worldY, float camX, float camY, float screenHalfW, float volume = 1.0f);

    void Update(float dt);

private:
    void GenerateAllSounds();
    void RegisterSound(SoundId id, const std::vector<int16_t>& pcm, int sampleRate = 22050);

    ALCdevice* m_device{nullptr};
    ALCcontext* m_context{nullptr};

    std::unordered_map<SoundId, ALuint> m_buffers;
    static constexpr int NUM_SOURCES = 24;
    ALuint m_sources[NUM_SOURCES]{0};
    int m_nextSourceIndex{0};

    bool m_initialized{false};
};
