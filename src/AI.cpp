#include "AI.hpp"
#include <iostream>

AlliedAI::AlliedAI() = default;
AlliedAI::~AlliedAI() = default;

void AlliedAI::Initialize(EntityManager& entities, Map& map) {
    // Setup initial Allied base in top-right corner
    entities.CreateStructure(StructureType::ALLIED_CONYARD, Faction::ALLIED, 54, 12);
    entities.CreateStructure(StructureType::ALLIED_POWER, Faction::ALLIED, 50, 10);
    entities.CreateStructure(StructureType::ALLIED_BARRACKS, Faction::ALLIED, 58, 16);
    entities.CreateStructure(StructureType::ALLIED_WARFACTORY, Faction::ALLIED, 52, 18);
    entities.CreateStructure(StructureType::ALLIED_PILLBOX, Faction::ALLIED, 45, 26);
    entities.CreateStructure(StructureType::ALLIED_PILLBOX, Faction::ALLIED, 38, 20);

    // Initial guards
    entities.CreateUnit(UnitType::ALLIED_RIFLEMAN, Faction::ALLIED, Map::TileCenterToWorld(56, 20));
    entities.CreateUnit(UnitType::ALLIED_RIFLEMAN, Faction::ALLIED, Map::TileCenterToWorld(57, 21));
    entities.CreateUnit(UnitType::ALLIED_LIGHT_TANK, Faction::ALLIED, Map::TileCenterToWorld(54, 23));

    // Mark tiles occupied on map
    for (const auto& s : entities.GetStructures()) {
        if (s.faction == Faction::ALLIED) {
            map.SetBuildingOccupation(s.tileX, s.tileY, s.wTiles, s.hTiles, s.id);
        }
    }

    m_waveTimer = 35.0f;
    m_waveCount = 0;
}

void AlliedAI::Update(float dt, EntityManager& entities, Map& map, AudioSystem& audio) {
    // Check if Allied base has production structures
    bool hasBarracks = false;
    bool hasFactory = false;

    for (const auto& s : entities.GetStructures()) {
        if (s.faction == Faction::ALLIED && s.health > 0.0f) {
            if (s.type == StructureType::ALLIED_BARRACKS) hasBarracks = true;
            if (s.type == StructureType::ALLIED_WARFACTORY) hasFactory = true;
        }
    }

    // Unit Production Cycle
    m_productionTimer -= dt;
    if (m_productionTimer <= 0.0f) {
        m_productionTimer = 16.0f - std::min(6.0f, float(m_waveCount) * 1.0f);
        if (hasBarracks || hasFactory) {
            ProduceUnits(entities, map);
        }
    }

    // Attack Wave Timer
    m_waveTimer -= dt;
    if (m_waveTimer <= 0.0f) {
        m_waveTimer = 38.0f;
        m_waveCount++;
        LaunchAttackWave(entities, map, audio);
    }
}

void AlliedAI::ProduceUnits(EntityManager& entities, Map& /*map*/) {
    // Produce units according to wave count
    glm::vec2 spawnPoint = Map::TileCenterToWorld(54, 22);

    if (m_waveCount == 0) {
        entities.CreateUnit(UnitType::ALLIED_RIFLEMAN, Faction::ALLIED, spawnPoint + glm::vec2(-15.0f, 0.0f));
        entities.CreateUnit(UnitType::ALLIED_RIFLEMAN, Faction::ALLIED, spawnPoint + glm::vec2(15.0f, 0.0f));
    } else if (m_waveCount == 1) {
        entities.CreateUnit(UnitType::ALLIED_LIGHT_TANK, Faction::ALLIED, spawnPoint);
        entities.CreateUnit(UnitType::ALLIED_RIFLEMAN, Faction::ALLIED, spawnPoint + glm::vec2(20.0f, 0.0f));
    } else {
        entities.CreateUnit(UnitType::ALLIED_MEDIUM_TANK, Faction::ALLIED, spawnPoint);
        entities.CreateUnit(UnitType::ALLIED_LIGHT_TANK, Faction::ALLIED, spawnPoint + glm::vec2(-20.0f, 10.0f));
        entities.CreateUnit(UnitType::ALLIED_RIFLEMAN, Faction::ALLIED, spawnPoint + glm::vec2(20.0f, 10.0f));
    }
}

void AlliedAI::LaunchAttackWave(EntityManager& entities, Map& map, AudioSystem& audio) {
    // Find Soviet target (ConYard, Refinery, Power, or unit)
    glm::vec2 targetPos(Map::TileCenterToWorld(16, 56)); // Soviet base default
    int targetStructId = -1;

    for (const auto& s : entities.GetStructures()) {
        if (s.faction == Faction::SOVIET && s.health > 0.0f) {
            targetPos = s.GetCenterWorld();
            targetStructId = s.id;
            break;
        }
    }

    // Command all idle Allied combat units to attack target
    int unitsInWave = 0;
    for (auto& u : entities.GetUnits()) {
        if (u.faction == Faction::ALLIED && u.health > 0.0f) {
            u.waypoints = map.FindPath(u.pos, targetPos);
            u.targetStructureId = targetStructId;
            u.state = UnitState::MOVING;
            unitsInWave++;
        }
    }

    if (unitsInWave > 0) {
        audio.Play(SoundId::BASE_ALARM, 0.9f);
        entities.SpawnFloatingText(targetPos, "WARNING: ALLIED ATTACK INCOMING!", glm::vec4(1.0f, 0.2f, 0.2f, 1.0f));
        std::cout << "Allied AI: Launched attack wave #" << m_waveCount << " with " << unitsInWave << " units." << std::endl;
    }
}
