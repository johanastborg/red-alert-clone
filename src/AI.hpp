#pragma once

#include "Entities.hpp"
#include "Map.hpp"
#include "AudioSystem.hpp"

class AlliedAI {
public:
    AlliedAI();
    ~AlliedAI();

    void Initialize(EntityManager& entities, Map& map);
    void Update(float dt, EntityManager& entities, Map& map, AudioSystem& audio);

    int GetWaveCount() const { return m_waveCount; }
    float GetNextWaveTime() const { return m_waveTimer; }

private:
    float m_productionTimer{5.0f};
    float m_waveTimer{35.0f};
    int m_waveCount{0};

    void ProduceUnits(EntityManager& entities, Map& map);
    void LaunchAttackWave(EntityManager& entities, Map& map, AudioSystem& audio);
};
