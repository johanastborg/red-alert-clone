#pragma once

#include "TextureAtlas.hpp"
#include "Renderer.hpp"
#include <glm/glm.hpp>
#include <vector>
#include <string>

enum class Faction {
    SOVIET = 0,
    ALLIED = 1
};

enum class StructureType {
    SOVIET_CONYARD,
    SOVIET_POWER,
    SOVIET_REFINERY,
    SOVIET_BARRACKS,
    SOVIET_WARFACTORY,
    SOVIET_RADAR,
    SOVIET_TESLA_COIL,

    ALLIED_CONYARD,
    ALLIED_POWER,
    ALLIED_BARRACKS,
    ALLIED_WARFACTORY,
    ALLIED_PILLBOX
};

enum class UnitType {
    CONSCRIPT,
    TESLA_TROOPER,
    HEAVY_TANK,
    MAMMOTH_TANK,
    V2_ROCKET,
    HARVESTER,

    ALLIED_RIFLEMAN,
    ALLIED_LIGHT_TANK,
    ALLIED_MEDIUM_TANK
};

enum class UnitState {
    IDLE,
    MOVING,
    ATTACKING,
    HARVESTING,
    RETURNING_TO_REFINERY,
    UNLOADING
};

enum class TeslaState {
    IDLE,
    CHARGING,
    COOLDOWN
};

enum class ProjectileType {
    BULLET,
    SHELL,
    ROCKET
};

enum class ParticleType {
    SMOKE,
    SPARK,
    FLAME,
    EXPLOSION
};

struct Unit {
    int id{0};
    UnitType type{UnitType::CONSCRIPT};
    Faction faction{Faction::SOVIET};
    glm::vec2 pos{0.0f};
    glm::vec2 velocity{0.0f};
    float bodyAngle{0.0f};
    float turretAngle{0.0f};

    float health{100.0f};
    float maxHealth{100.0f};
    float speed{90.0f};
    float attackRange{150.0f};
    float damage{15.0f};
    float reloadTime{0.8f};
    float reloadTimer{0.0f};
    float sightRadius{6.0f}; // tiles
    int cost{100};

    bool selected{false};
    UnitState state{UnitState::IDLE};
    std::vector<glm::vec2> waypoints;

    int targetUnitId{-1};
    int targetStructureId{-1};

    // Harvester specific
    float oreCargo{0.0f};
    static constexpr float MAX_ORE_CARGO = 1000.0f;
    glm::ivec2 targetOreTile{-1, -1};
    int dockRefineryId{-1};
    float harvestTimer{0.0f};

    // Animation & firing alternator
    float animTimer{0.0f};
    int barrelAlternator{0};
    float selfRepairTimer{0.0f}; // Mammoth self repair
};

struct Structure {
    int id{0};
    StructureType type{StructureType::SOVIET_CONYARD};
    Faction faction{Faction::SOVIET};
    int tileX{0};
    int tileY{0};
    int wTiles{3};
    int hTiles{3};

    float health{1000.0f};
    float maxHealth{1000.0f};
    int powerProduction{0};
    int powerDrain{0};
    float sightRadius{8.0f};
    int cost{500};

    bool selected{false};
    glm::vec2 rallyPoint{0.0f};

    // Tesla Coil attack state
    TeslaState teslaState{TeslaState::IDLE};
    float teslaTimer{0.0f};
    int teslaTargetUnitId{-1};

    // Allied Pillbox machine gun
    float attackTimer{0.0f};
    int pillboxTargetId{-1};

    glm::vec2 GetCenterWorld(float /*dummy*/ = 0.0f) const {
        float cx = float(tileX) + float(wTiles) * 0.5f;
        float cy = float(tileY) + float(hTiles) * 0.5f;
        float isoX = (cx - cy) * 32.0f;
        float isoY = (cx + cy) * 16.0f;
        return glm::vec2(isoX, isoY);
    }
};

struct Projectile {
    int id{0};
    ProjectileType type{ProjectileType::BULLET};
    Faction faction{Faction::SOVIET};
    glm::vec2 pos{0.0f};
    glm::vec2 targetPos{0.0f};
    int targetUnitId{-1};
    int targetStructureId{-1};
    float speed{300.0f};
    float damage{25.0f};
    float angle{0.0f};
    float smokeTimer{0.0f};
};

struct Particle {
    ParticleType type{ParticleType::SMOKE};
    glm::vec2 pos{0.0f};
    glm::vec2 vel{0.0f};
    float life{0.0f};
    float maxLife{1.0f};
    float size{16.0f};
    glm::vec4 color{1.0f};
    int frame{0};
};

struct FloatingText {
    glm::vec2 pos{0.0f};
    std::string text;
    glm::vec4 color{1.0f};
    float life{0.0f};
    float maxLife{1.5f};
};

class EntityManager {
public:
    EntityManager();
    ~EntityManager();

    void Clear();

    // Unit factory
    Unit& CreateUnit(UnitType type, Faction faction, glm::vec2 pos);
    // Structure factory
    Structure& CreateStructure(StructureType type, Faction faction, int tx, int ty);

    void SpawnProjectile(ProjectileType type, Faction faction, glm::vec2 start, glm::vec2 target, float damage, int targetUnit = -1, int targetStruct = -1);
    void SpawnExplosion(glm::vec2 pos, float scale = 1.0f);
    void SpawnSpark(glm::vec2 pos, glm::vec2 vel, glm::vec4 color = glm::vec4(0.0f, 0.9f, 1.0f, 1.0f));
    void SpawnSmoke(glm::vec2 pos, glm::vec2 vel);
    void SpawnFloatingText(glm::vec2 pos, const std::string& text, glm::vec4 color = glm::vec4(1.0f, 0.85f, 0.2f, 1.0f));

    void UpdateParticles(float dt);
    void RenderParticles(Renderer& renderer);
    void RenderFloatingTexts(Renderer& renderer);

    std::vector<Unit>& GetUnits() { return m_units; }
    const std::vector<Unit>& GetUnits() const { return m_units; }

    std::vector<Structure>& GetStructures() { return m_structures; }
    const std::vector<Structure>& GetStructures() const { return m_structures; }

    std::vector<Projectile>& GetProjectiles() { return m_projectiles; }

    Unit* FindUnitById(int id);
    Structure* FindStructureById(int id);

    void RemoveDeadEntities();

private:
    int m_nextEntityId{1};
    std::vector<Unit> m_units;
    std::vector<Structure> m_structures;
    std::vector<Projectile> m_projectiles;
    std::vector<Particle> m_particles;
    std::vector<FloatingText> m_floatingTexts;
};
