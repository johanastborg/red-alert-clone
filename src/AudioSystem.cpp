#include "AudioSystem.hpp"
#include <cmath>
#include <random>
#include <algorithm>
#include <iostream>

namespace {
constexpr float PI = 3.14159265358979323846f;
constexpr int SAMPLE_RATE = 22050;

inline float clamp(float v, float mn, float mx) {
    return std::max(mn, std::min(mx, v));
}

// Simple white noise generator with seed
struct NoiseGen {
    uint32_t state = 123456789;
    float next() {
        state = state * 1664525u + 1013904223u;
        return (float(state) / 4294967296.0f) * 2.0f - 1.0f;
    }
};

std::vector<int16_t> GenerateClick() {
    int samples = int(SAMPLE_RATE * 0.015f);
    std::vector<int16_t> pcm(samples);
    for (int i = 0; i < samples; ++i) {
        float t = float(i) / SAMPLE_RATE;
        float env = 1.0f - float(i) / samples;
        float val = std::sin(2.0f * PI * 1800.0f * t) * env * 0.8f;
        pcm[i] = int16_t(clamp(val, -1.0f, 1.0f) * 32000.0f);
    }
    return pcm;
}

std::vector<int16_t> GenerateSelect() {
    int samples = int(SAMPLE_RATE * 0.09f);
    std::vector<int16_t> pcm(samples);
    NoiseGen noise;
    for (int i = 0; i < samples; ++i) {
        float t = float(i) / SAMPLE_RATE;
        float env = 1.0f - float(i) / samples;
        float freq = (t < 0.045f) ? 950.0f : 1400.0f;
        float tone = std::sin(2.0f * PI * freq * t);
        float squelch = noise.next() * (t < 0.02f ? 0.25f : 0.02f);
        float val = (tone * 0.7f + squelch) * env;
        pcm[i] = int16_t(clamp(val, -1.0f, 1.0f) * 28000.0f);
    }
    return pcm;
}

std::vector<int16_t> GenerateMoveOrder() {
    int samples = int(SAMPLE_RATE * 0.11f);
    std::vector<int16_t> pcm(samples);
    for (int i = 0; i < samples; ++i) {
        float t = float(i) / SAMPLE_RATE;
        float env = 1.0f - float(i) / samples;
        float freq = 600.0f + 500.0f * (t / 0.11f);
        float tone = std::sin(2.0f * PI * freq * t) * 0.8f;
        pcm[i] = int16_t(clamp(tone * env, -1.0f, 1.0f) * 29000.0f);
    }
    return pcm;
}

std::vector<int16_t> GenerateAttackOrder() {
    int samples = int(SAMPLE_RATE * 0.13f);
    std::vector<int16_t> pcm(samples);
    for (int i = 0; i < samples; ++i) {
        float t = float(i) / SAMPLE_RATE;
        float env = 1.0f - float(i) / samples;
        float f1 = 520.0f + 200.0f * std::sin(2.0f * PI * 40.0f * t);
        float val = std::sin(2.0f * PI * f1 * t) * 0.85f * env;
        pcm[i] = int16_t(clamp(val, -1.0f, 1.0f) * 30000.0f);
    }
    return pcm;
}

std::vector<int16_t> GenerateRifleFire() {
    int samples = int(SAMPLE_RATE * 0.08f);
    std::vector<int16_t> pcm(samples);
    NoiseGen noise;
    float lp = 0.0f;
    for (int i = 0; i < samples; ++i) {
        float t = float(i) / SAMPLE_RATE;
        float env = std::exp(-t * 55.0f);
        float n = noise.next();
        lp += (n - lp) * 0.65f;
        float pop = std::sin(2.0f * PI * 350.0f * t) * std::exp(-t * 90.0f);
        float val = (lp * 0.8f + pop * 0.5f) * env;
        pcm[i] = int16_t(clamp(val, -1.0f, 1.0f) * 31000.0f);
    }
    return pcm;
}

std::vector<int16_t> GenerateTankCannon() {
    int samples = int(SAMPLE_RATE * 0.38f);
    std::vector<int16_t> pcm(samples);
    NoiseGen noise;
    float lp = 0.0f;
    for (int i = 0; i < samples; ++i) {
        float t = float(i) / SAMPLE_RATE;
        float env = std::exp(-t * 11.0f);
        // Pitch drop from 140Hz down to 35Hz
        float freq = 35.0f + 110.0f * std::exp(-t * 22.0f);
        float sub = std::sin(2.0f * PI * freq * t);
        float n = noise.next();
        lp += (n - lp) * 0.28f;
        float val = (sub * 0.65f + lp * 0.7f) * env;
        pcm[i] = int16_t(clamp(val, -1.0f, 1.0f) * 32000.0f);
    }
    return pcm;
}

std::vector<int16_t> GenerateMammothFire() {
    int samples = int(SAMPLE_RATE * 0.55f);
    std::vector<int16_t> pcm(samples);
    NoiseGen noise;
    float lp1 = 0.0f, lp2 = 0.0f;
    for (int i = 0; i < samples; ++i) {
        float t = float(i) / SAMPLE_RATE;
        // First shot at t=0, second shot at t=0.09
        float env1 = (t >= 0.0f) ? std::exp(-(t) * 9.0f) : 0.0f;
        float env2 = (t >= 0.09f) ? std::exp(-(t - 0.09f) * 9.0f) : 0.0f;
        
        float f1 = 30.0f + 120.0f * std::exp(-t * 18.0f);
        float sub1 = std::sin(2.0f * PI * f1 * t) * env1;
        float n1 = noise.next();
        lp1 += (n1 - lp1) * 0.35f;

        float sub2 = 0.0f;
        if (t >= 0.09f) {
            float f2 = 28.0f + 110.0f * std::exp(-(t - 0.09f) * 18.0f);
            sub2 = std::sin(2.0f * PI * f2 * (t - 0.09f)) * env2;
            float n2 = noise.next();
            lp2 += (n2 - lp2) * 0.35f;
        }

        float val = (sub1 * 0.6f + lp1 * 0.5f * env1) + (sub2 * 0.65f + lp2 * 0.55f * env2);
        pcm[i] = int16_t(clamp(val, -1.0f, 1.0f) * 32500.0f);
    }
    return pcm;
}

std::vector<int16_t> GenerateTeslaCharge() {
    int samples = int(SAMPLE_RATE * 0.70f);
    std::vector<int16_t> pcm(samples);
    float phase = 0.0f;
    for (int i = 0; i < samples; ++i) {
        float t = float(i) / SAMPLE_RATE;
        float progress = t / 0.70f;
        float env = std::min(1.0f, progress * 2.0f);
        // Exponential sweep from 150 Hz to 1800 Hz
        float freq = 150.0f * std::pow(12.0f, progress);
        // FM modulation (electric buzz)
        float mod = std::sin(2.0f * PI * 120.0f * t) * 40.0f;
        phase += 2.0f * PI * (freq + mod) / SAMPLE_RATE;
        float val = std::sin(phase) * 0.65f;
        // Add 60Hz high-voltage hum
        val += std::sin(2.0f * PI * 60.0f * t) * 0.3f;
        pcm[i] = int16_t(clamp(val * env, -1.0f, 1.0f) * 28000.0f);
    }
    return pcm;
}

std::vector<int16_t> GenerateTeslaZap() {
    int samples = int(SAMPLE_RATE * 0.42f);
    std::vector<int16_t> pcm(samples);
    NoiseGen noise;
    for (int i = 0; i < samples; ++i) {
        float t = float(i) / SAMPLE_RATE;
        float env = std::exp(-t * 9.0f);
        // Electric buzz: rapid pulse train + broadband crackle
        float pulse = std::fmod(t * 360.0f, 1.0f) < 0.2f ? 1.0f : -1.0f;
        float crackle = (noise.next() > 0.4f ? 1.0f : -0.2f) * noise.next();
        float tone = std::sin(2.0f * PI * (800.0f + 400.0f * noise.next()) * t);
        float val = (pulse * 0.4f + crackle * 0.6f + tone * 0.3f) * env;
        // Distort for electric sizzle
        val = clamp(val * 1.5f, -1.0f, 1.0f);
        pcm[i] = int16_t(val * 31500.0f);
    }
    return pcm;
}

std::vector<int16_t> GenerateRocketLaunch() {
    int samples = int(SAMPLE_RATE * 0.52f);
    std::vector<int16_t> pcm(samples);
    NoiseGen noise;
    float bp = 0.0f, bp2 = 0.0f;
    for (int i = 0; i < samples; ++i) {
        float t = float(i) / SAMPLE_RATE;
        float env = std::min(1.0f, t * 12.0f) * std::exp(-t * 2.5f);
        float n = noise.next();
        bp += (n - bp) * 0.4f;
        bp2 += (bp - bp2) * 0.3f;
        float whine = std::sin(2.0f * PI * (300.0f + 600.0f * t) * t) * 0.25f;
        float val = (bp2 * 0.75f + whine) * env;
        pcm[i] = int16_t(clamp(val, -1.0f, 1.0f) * 29000.0f);
    }
    return pcm;
}

std::vector<int16_t> GenerateExplosionSmall() {
    int samples = int(SAMPLE_RATE * 0.32f);
    std::vector<int16_t> pcm(samples);
    NoiseGen noise;
    float lp = 0.0f;
    for (int i = 0; i < samples; ++i) {
        float t = float(i) / SAMPLE_RATE;
        float env = std::exp(-t * 14.0f);
        float n = noise.next();
        lp += (n - lp) * 0.45f;
        float thump = std::sin(2.0f * PI * 65.0f * t) * std::exp(-t * 20.0f);
        float val = (lp * 0.75f + thump * 0.6f) * env;
        pcm[i] = int16_t(clamp(val, -1.0f, 1.0f) * 31000.0f);
    }
    return pcm;
}

std::vector<int16_t> GenerateExplosionLarge() {
    int samples = int(SAMPLE_RATE * 0.75f);
    std::vector<int16_t> pcm(samples);
    NoiseGen noise;
    float lp = 0.0f, lp_sub = 0.0f;
    for (int i = 0; i < samples; ++i) {
        float t = float(i) / SAMPLE_RATE;
        float env = std::exp(-t * 5.5f);
        float n = noise.next();
        lp += (n - lp) * 0.25f;
        // Sub rumble
        float f = 45.0f * std::exp(-t * 2.0f);
        float sub = std::sin(2.0f * PI * f * t);
        lp_sub += (sub - lp_sub) * 0.5f;
        float crackle = (t < 0.1f) ? n * 0.5f : 0.0f;
        float val = (lp * 0.6f + lp_sub * 0.7f + crackle) * env;
        pcm[i] = int16_t(clamp(val, -1.0f, 1.0f) * 32700.0f);
    }
    return pcm;
}

std::vector<int16_t> GenerateCreditsTick() {
    int samples = int(SAMPLE_RATE * 0.13f);
    std::vector<int16_t> pcm(samples);
    for (int i = 0; i < samples; ++i) {
        float t = float(i) / SAMPLE_RATE;
        float env = std::exp(-t * 35.0f);
        float bell = std::sin(2.0f * PI * 1320.0f * t) * 0.6f +
                     std::sin(2.0f * PI * 2640.0f * t) * 0.4f;
        pcm[i] = int16_t(clamp(bell * env, -1.0f, 1.0f) * 27000.0f);
    }
    return pcm;
}

std::vector<int16_t> GenerateBuildingPlace() {
    int samples = int(SAMPLE_RATE * 0.30f);
    std::vector<int16_t> pcm(samples);
    NoiseGen noise;
    float lp = 0.0f;
    for (int i = 0; i < samples; ++i) {
        float t = float(i) / SAMPLE_RATE;
        float env = std::exp(-t * 16.0f);
        float thud = std::sin(2.0f * PI * 70.0f * t) * 0.7f;
        float n = noise.next();
        lp += (n - lp) * 0.3f;
        float metallic = std::sin(2.0f * PI * 440.0f * t) * std::exp(-t * 30.0f) * 0.4f;
        float val = (thud + lp * 0.5f + metallic) * env;
        pcm[i] = int16_t(clamp(val, -1.0f, 1.0f) * 31000.0f);
    }
    return pcm;
}

std::vector<int16_t> GenerateBuildingReady() {
    // Two bright bell chimes: C6 (1046Hz) and G6 (1568Hz)
    int samples = int(SAMPLE_RATE * 0.45f);
    std::vector<int16_t> pcm(samples);
    for (int i = 0; i < samples; ++i) {
        float t = float(i) / SAMPLE_RATE;
        float env1 = std::exp(-t * 10.0f);
        float chime1 = std::sin(2.0f * PI * 1046.0f * t) * env1;
        float env2 = (t >= 0.15f) ? std::exp(-(t - 0.15f) * 9.0f) : 0.0f;
        float chime2 = (t >= 0.15f) ? std::sin(2.0f * PI * 1568.0f * (t - 0.15f)) * env2 : 0.0f;
        float val = chime1 * 0.5f + chime2 * 0.6f;
        pcm[i] = int16_t(clamp(val, -1.0f, 1.0f) * 28000.0f);
    }
    return pcm;
}

std::vector<int16_t> GenerateUnitReady() {
    // Soviet fanfare triad: A4 (440Hz), C#5 (554Hz), E5 (659Hz)
    int samples = int(SAMPLE_RATE * 0.50f);
    std::vector<int16_t> pcm(samples);
    for (int i = 0; i < samples; ++i) {
        float t = float(i) / SAMPLE_RATE;
        float f = 440.0f;
        if (t > 0.12f && t <= 0.24f) f = 554.37f;
        else if (t > 0.24f) f = 659.25f;
        float env = (t > 0.24f) ? std::exp(-(t - 0.24f) * 6.0f) : 0.85f;
        // Brass-like rich harmonics
        float val = (std::sin(2.0f * PI * f * t) * 0.6f +
                     std::sin(2.0f * PI * f * 2.0f * t) * 0.25f +
                     std::sin(2.0f * PI * f * 3.0f * t) * 0.15f) * env;
        pcm[i] = int16_t(clamp(val, -1.0f, 1.0f) * 27000.0f);
    }
    return pcm;
}

std::vector<int16_t> GenerateBaseAlarm() {
    // Klaxon siren (two-tone alternating 650Hz / 850Hz)
    int samples = int(SAMPLE_RATE * 0.55f);
    std::vector<int16_t> pcm(samples);
    for (int i = 0; i < samples; ++i) {
        float t = float(i) / SAMPLE_RATE;
        float f = (std::fmod(t, 0.26f) < 0.13f) ? 660.0f : 880.0f;
        float env = 0.85f * (1.0f - t / 0.55f * 0.3f);
        float val = std::sin(2.0f * PI * f * t) * env;
        pcm[i] = int16_t(clamp(val, -1.0f, 1.0f) * 29000.0f);
    }
    return pcm;
}

std::vector<int16_t> GeneratePowerLow() {
    int samples = int(SAMPLE_RATE * 0.45f);
    std::vector<int16_t> pcm(samples);
    for (int i = 0; i < samples; ++i) {
        float t = float(i) / SAMPLE_RATE;
        float env = std::exp(-t * 5.0f);
        float buzz = (std::sin(2.0f * PI * 100.0f * t) +
                      std::sin(2.0f * PI * 200.0f * t) * 0.5f +
                      std::sin(2.0f * PI * 300.0f * t) * 0.25f) * 0.6f;
        pcm[i] = int16_t(clamp(buzz * env, -1.0f, 1.0f) * 26000.0f);
    }
    return pcm;
}

std::vector<int16_t> GenerateRadarPing() {
    int samples = int(SAMPLE_RATE * 0.28f);
    std::vector<int16_t> pcm(samples);
    for (int i = 0; i < samples; ++i) {
        float t = float(i) / SAMPLE_RATE;
        float env = std::exp(-t * 14.0f);
        float val = std::sin(2.0f * PI * 1760.0f * t) * env * 0.7f;
        pcm[i] = int16_t(clamp(val, -1.0f, 1.0f) * 25000.0f);
    }
    return pcm;
}

std::vector<int16_t> GenerateVictoryFanfare() {
    int samples = int(SAMPLE_RATE * 1.2f);
    std::vector<int16_t> pcm(samples);
    for (int i = 0; i < samples; ++i) {
        float t = float(i) / SAMPLE_RATE;
        float f = 440.0f; // A4
        if (t >= 0.2f && t < 0.4f) f = 554.37f; // C#5
        else if (t >= 0.4f && t < 0.6f) f = 659.25f; // E5
        else if (t >= 0.6f) f = 880.0f; // A5
        float env = (t >= 0.6f) ? std::exp(-(t - 0.6f) * 3.0f) : 0.85f;
        float val = (std::sin(2.0f * PI * f * t) * 0.7f +
                     std::sin(2.0f * PI * f * 2.0f * t) * 0.3f) * env;
        pcm[i] = int16_t(clamp(val, -1.0f, 1.0f) * 30000.0f);
    }
    return pcm;
}

} // namespace

AudioSystem::AudioSystem() = default;

AudioSystem::~AudioSystem() {
    Shutdown();
}

bool AudioSystem::Initialize() {
    m_device = alcOpenDevice(nullptr);
    if (!m_device) {
        std::cerr << "AudioSystem: Failed to open default audio device." << std::endl;
        return false;
    }
    m_context = alcCreateContext(m_device, nullptr);
    if (!m_context) {
        std::cerr << "AudioSystem: Failed to create OpenAL context." << std::endl;
        alcCloseDevice(m_device);
        m_device = nullptr;
        return false;
    }
    alcMakeContextCurrent(m_context);

    // Generate OpenAL sources pool
    alGenSources(NUM_SOURCES, m_sources);
    for (int i = 0; i < NUM_SOURCES; ++i) {
        alSourcef(m_sources[i], AL_PITCH, 1.0f);
        alSourcef(m_sources[i], AL_GAIN, 1.0f);
        alSource3f(m_sources[i], AL_POSITION, 0.0f, 0.0f, 0.0f);
        alSource3f(m_sources[i], AL_VELOCITY, 0.0f, 0.0f, 0.0f);
        alSourcei(m_sources[i], AL_LOOPING, AL_FALSE);
    }

    GenerateAllSounds();
    m_initialized = true;
    std::cout << "AudioSystem: Initialized with " << m_buffers.size() << " sound buffers and "
              << NUM_SOURCES << " audio channels." << std::endl;
    return true;
}

void AudioSystem::RegisterSound(SoundId id, const std::vector<int16_t>& pcm, int sampleRate) {
    ALuint buffer = 0;
    alGenBuffers(1, &buffer);
    alBufferData(buffer, AL_FORMAT_MONO16, pcm.data(), ALsizei(pcm.size() * sizeof(int16_t)), sampleRate);
    m_buffers[id] = buffer;
}

void AudioSystem::GenerateAllSounds() {
    RegisterSound(SoundId::CLICK, GenerateClick());
    RegisterSound(SoundId::SELECT, GenerateSelect());
    RegisterSound(SoundId::MOVE_ORDER, GenerateMoveOrder());
    RegisterSound(SoundId::ATTACK_ORDER, GenerateAttackOrder());
    RegisterSound(SoundId::RIFLE_FIRE, GenerateRifleFire());
    RegisterSound(SoundId::TANK_CANNON, GenerateTankCannon());
    RegisterSound(SoundId::MAMMOTH_FIRE, GenerateMammothFire());
    RegisterSound(SoundId::TESLA_CHARGE, GenerateTeslaCharge());
    RegisterSound(SoundId::TESLA_ZAP, GenerateTeslaZap());
    RegisterSound(SoundId::ROCKET_LAUNCH, GenerateRocketLaunch());
    RegisterSound(SoundId::EXPLOSION_SMALL, GenerateExplosionSmall());
    RegisterSound(SoundId::EXPLOSION_LARGE, GenerateExplosionLarge());
    RegisterSound(SoundId::CREDITS_TICK, GenerateCreditsTick());
    RegisterSound(SoundId::BUILDING_PLACE, GenerateBuildingPlace());
    RegisterSound(SoundId::BUILDING_READY, GenerateBuildingReady());
    RegisterSound(SoundId::UNIT_READY, GenerateUnitReady());
    RegisterSound(SoundId::BASE_ALARM, GenerateBaseAlarm());
    RegisterSound(SoundId::POWER_LOW, GeneratePowerLow());
    RegisterSound(SoundId::RADAR_PING, GenerateRadarPing());
    RegisterSound(SoundId::VICTORY_FANFARE, GenerateVictoryFanfare());
}

void AudioSystem::Play(SoundId id, float volume, float pitch) {
    if (!m_initialized) return;
    auto it = m_buffers.find(id);
    if (it == m_buffers.end()) return;

    // Pick next source from pool
    ALuint src = m_sources[m_nextSourceIndex];
    m_nextSourceIndex = (m_nextSourceIndex + 1) % NUM_SOURCES;

    alSourceStop(src);
    alSourcei(src, AL_BUFFER, it->second);
    alSourcef(src, AL_GAIN, clamp(volume, 0.0f, 1.0f));
    alSourcef(src, AL_PITCH, clamp(pitch, 0.5f, 2.0f));
    alSource3f(src, AL_POSITION, 0.0f, 0.0f, 0.0f);
    alSourcePlay(src);
}

void AudioSystem::PlayAt(SoundId id, float worldX, float worldY, float camX, float camY, float screenHalfW, float volume) {
    if (!m_initialized) return;
    float dx = worldX - camX;
    float dy = worldY - camY;
    float dist = std::sqrt(dx * dx + dy * dy);

    // Distance attenuation
    float maxDist = screenHalfW * 2.5f;
    float atten = 1.0f - clamp(dist / maxDist, 0.0f, 0.85f);
    float pan = clamp(dx / (screenHalfW * 1.2f), -1.0f, 1.0f);

    auto it = m_buffers.find(id);
    if (it == m_buffers.end()) return;

    ALuint src = m_sources[m_nextSourceIndex];
    m_nextSourceIndex = (m_nextSourceIndex + 1) % NUM_SOURCES;

    alSourceStop(src);
    alSourcei(src, AL_BUFFER, it->second);
    alSourcef(src, AL_GAIN, clamp(volume * atten, 0.0f, 1.0f));
    alSourcef(src, AL_PITCH, 1.0f);
    alSource3f(src, AL_POSITION, pan * 2.0f, 0.0f, -1.0f);
    alSourcePlay(src);
}

void AudioSystem::Update(float /*dt*/) {
    // OpenAL handles playback asynchronously in background threads
}

void AudioSystem::Shutdown() {
    if (!m_initialized) return;
    for (int i = 0; i < NUM_SOURCES; ++i) {
        if (m_sources[i]) {
            alSourceStop(m_sources[i]);
        }
    }
    alDeleteSources(NUM_SOURCES, m_sources);
    for (auto& [id, buf] : m_buffers) {
        alDeleteBuffers(1, &buf);
    }
    m_buffers.clear();

    if (m_context) {
        alcMakeContextCurrent(nullptr);
        alcDestroyContext(m_context);
        m_context = nullptr;
    }
    if (m_device) {
        alcCloseDevice(m_device);
        m_device = nullptr;
    }
    m_initialized = false;
}
