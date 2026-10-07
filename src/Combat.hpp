#pragma once

#include "Entities.hpp"
#include "Map.hpp"
#include "AudioSystem.hpp"
#include "Renderer.hpp"

class CombatSystem {
public:
    CombatSystem();
    ~CombatSystem();

    void Update(float dt, EntityManager& entities, Map& map, AudioSystem& audio,
                int sovietPowerProduction, int sovietPowerDrain, int& sovietCredits, int& alliedCredits);

    void RenderActiveTeslaArcs(Renderer& renderer);

private:
    void UpdateUnits(float dt, EntityManager& entities, Map& map, AudioSystem& audio, int& sovietCredits, int& alliedCredits);
    void UpdateTeslaCoils(float dt, EntityManager& entities, AudioSystem& audio, bool sovietHasPower);
    void UpdateAlliedDefenses(float dt, EntityManager& entities, AudioSystem& audio);
    void UpdateProjectiles(float dt, EntityManager& entities, AudioSystem& audio);
    void UpdateHarvesters(float dt, EntityManager& entities, Map& map, AudioSystem& audio, int& sovietCredits, int& alliedCredits);

    struct ActiveArc {
        glm::vec2 start;
        glm::vec2 end;
        float timer;
    };
    std::vector<ActiveArc> m_activeArcs;
    float m_globalTime{0.0f};
};
