#include "Entities.hpp"
#include <cmath>
#include <algorithm>
#include <random>

EntityManager::EntityManager() = default;
EntityManager::~EntityManager() = default;

void EntityManager::Clear() {
    m_units.clear();
    m_structures.clear();
    m_projectiles.clear();
    m_particles.clear();
    m_floatingTexts.clear();
}

Unit& EntityManager::CreateUnit(UnitType type, Faction faction, glm::vec2 pos) {
    Unit u;
    u.id = m_nextEntityId++;
    u.type = type;
    u.faction = faction;
    u.pos = pos;
    u.bodyAngle = 0.0f;
    u.turretAngle = 0.0f;
    u.state = UnitState::IDLE;

    switch (type) {
        case UnitType::CONSCRIPT:
            u.health = u.maxHealth = 130.0f;
            u.speed = 85.0f;
            u.attackRange = 140.0f;
            u.damage = 14.0f;
            u.reloadTime = 0.7f;
            u.sightRadius = 6.0f;
            u.cost = 100;
            break;

        case UnitType::TESLA_TROOPER:
            u.health = u.maxHealth = 240.0f;
            u.speed = 70.0f;
            u.attackRange = 135.0f;
            u.damage = 48.0f;
            u.reloadTime = 1.1f;
            u.sightRadius = 6.0f;
            u.cost = 300;
            break;

        case UnitType::HEAVY_TANK:
            u.health = u.maxHealth = 650.0f;
            u.speed = 75.0f;
            u.attackRange = 210.0f;
            u.damage = 38.0f;
            u.reloadTime = 1.3f;
            u.sightRadius = 7.0f;
            u.cost = 800;
            break;

        case UnitType::MAMMOTH_TANK:
            u.health = u.maxHealth = 1300.0f;
            u.speed = 52.0f;
            u.attackRange = 240.0f;
            u.damage = 75.0f;
            u.reloadTime = 1.8f;
            u.sightRadius = 8.0f;
            u.cost = 1500;
            break;

        case UnitType::V2_ROCKET:
            u.health = u.maxHealth = 220.0f;
            u.speed = 65.0f;
            u.attackRange = 400.0f;
            u.damage = 130.0f;
            u.reloadTime = 3.8f;
            u.sightRadius = 8.5f;
            u.cost = 700;
            break;

        case UnitType::HARVESTER:
            u.health = u.maxHealth = 1100.0f;
            u.speed = 65.0f;
            u.attackRange = 0.0f;
            u.damage = 0.0f;
            u.reloadTime = 1.0f;
            u.sightRadius = 6.0f;
            u.cost = 1000;
            break;

        case UnitType::ALLIED_RIFLEMAN:
            u.health = u.maxHealth = 110.0f;
            u.speed = 85.0f;
            u.attackRange = 140.0f;
            u.damage = 12.0f;
            u.reloadTime = 0.65f;
            u.sightRadius = 6.0f;
            u.cost = 100;
            break;

        case UnitType::ALLIED_LIGHT_TANK:
            u.health = u.maxHealth = 380.0f;
            u.speed = 95.0f;
            u.attackRange = 190.0f;
            u.damage = 26.0f;
            u.reloadTime = 1.0f;
            u.sightRadius = 7.0f;
            u.cost = 600;
            break;

        case UnitType::ALLIED_MEDIUM_TANK:
            u.health = u.maxHealth = 580.0f;
            u.speed = 78.0f;
            u.attackRange = 210.0f;
            u.damage = 36.0f;
            u.reloadTime = 1.25f;
            u.sightRadius = 7.0f;
            u.cost = 800;
            break;
    }

    m_units.push_back(u);
    return m_units.back();
}

Structure& EntityManager::CreateStructure(StructureType type, Faction faction, int tx, int ty) {
    Structure s;
    s.id = m_nextEntityId++;
    s.type = type;
    s.faction = faction;
    s.tileX = tx;
    s.tileY = ty;
    s.wTiles = 2;
    s.hTiles = 2;

    switch (type) {
        case StructureType::SOVIET_CONYARD:
        case StructureType::ALLIED_CONYARD:
            s.wTiles = 3; s.hTiles = 3;
            s.health = s.maxHealth = 2200.0f;
            s.powerProduction = 60;
            s.powerDrain = 0;
            s.sightRadius = 9.0f;
            s.cost = 2500;
            break;

        case StructureType::SOVIET_POWER:
        case StructureType::ALLIED_POWER:
            s.wTiles = 2; s.hTiles = 2;
            s.health = s.maxHealth = 850.0f;
            s.powerProduction = 160;
            s.powerDrain = 0;
            s.sightRadius = 5.0f;
            s.cost = 300;
            break;

        case StructureType::SOVIET_REFINERY:
            s.wTiles = 3; s.hTiles = 3;
            s.health = s.maxHealth = 1300.0f;
            s.powerProduction = 0;
            s.powerDrain = 30;
            s.sightRadius = 7.0f;
            s.cost = 2000;
            break;

        case StructureType::SOVIET_BARRACKS:
        case StructureType::ALLIED_BARRACKS:
            s.wTiles = 2; s.hTiles = 2;
            s.health = s.maxHealth = 950.0f;
            s.powerProduction = 0;
            s.powerDrain = 20;
            s.sightRadius = 6.0f;
            s.cost = 400;
            break;

        case StructureType::SOVIET_WARFACTORY:
        case StructureType::ALLIED_WARFACTORY:
            s.wTiles = 3; s.hTiles = 3;
            s.health = s.maxHealth = 1600.0f;
            s.powerProduction = 0;
            s.powerDrain = 40;
            s.sightRadius = 7.0f;
            s.cost = 1500;
            break;

        case StructureType::SOVIET_RADAR:
            s.wTiles = 2; s.hTiles = 2;
            s.health = s.maxHealth = 900.0f;
            s.powerProduction = 0;
            s.powerDrain = 50;
            s.sightRadius = 12.0f;
            s.cost = 800;
            break;

        case StructureType::SOVIET_TESLA_COIL:
            s.wTiles = 2; s.hTiles = 2;
            s.health = s.maxHealth = 1000.0f;
            s.powerProduction = 0;
            s.powerDrain = 100;
            s.sightRadius = 8.0f;
            s.cost = 1200;
            break;

        case StructureType::ALLIED_PILLBOX:
            s.wTiles = 2; s.hTiles = 2;
            s.health = s.maxHealth = 700.0f;
            s.powerProduction = 0;
            s.powerDrain = 10;
            s.sightRadius = 7.0f;
            s.cost = 500;
            break;
    }

    s.rallyPoint = s.GetCenterWorld() + glm::vec2(0.0f, float(s.hTiles) * 20.0f);
    m_structures.push_back(s);
    return m_structures.back();
}

void EntityManager::SpawnProjectile(ProjectileType type, Faction faction, glm::vec2 start, glm::vec2 target, float damage, int targetUnit, int targetStruct) {
    Projectile p;
    p.id = m_nextEntityId++;
    p.type = type;
    p.faction = faction;
    p.pos = start;
    p.targetPos = target;
    p.targetUnitId = targetUnit;
    p.targetStructureId = targetStruct;
    p.damage = damage;

    float dx = target.x - start.x;
    float dy = target.y - start.y;
    p.angle = std::atan2(dy, dx);

    switch (type) {
        case ProjectileType::BULLET: p.speed = 550.0f; break;
        case ProjectileType::SHELL:  p.speed = 420.0f; break;
        case ProjectileType::ROCKET: p.speed = 280.0f; break;
    }

    m_projectiles.push_back(p);
}

void EntityManager::SpawnExplosion(glm::vec2 pos, float scale) {
    Particle p;
    p.type = ParticleType::EXPLOSION;
    p.pos = pos;
    p.vel = glm::vec2(0.0f);
    p.life = 0.0f;
    p.maxLife = 0.45f;
    p.size = 48.0f * scale;
    p.color = glm::vec4(1.0f);
    p.frame = 0;
    m_particles.push_back(p);

    // Spawn shockwave sparks
    static std::mt19937 rng(42);
    std::uniform_real_distribution<float> distA(0.0f, 6.28318f);
    std::uniform_real_distribution<float> distS(40.0f, 120.0f);

    for (int i = 0; i < 6; ++i) {
        float a = distA(rng);
        float s = distS(rng) * scale;
        SpawnSpark(pos, glm::vec2(std::cos(a) * s, std::sin(a) * s), glm::vec4(1.0f, 0.6f, 0.1f, 1.0f));
    }
}

void EntityManager::SpawnSpark(glm::vec2 pos, glm::vec2 vel, glm::vec4 color) {
    Particle p;
    p.type = ParticleType::SPARK;
    p.pos = pos;
    p.vel = vel;
    p.life = 0.0f;
    p.maxLife = 0.35f;
    p.size = 8.0f;
    p.color = color;
    m_particles.push_back(p);
}

void EntityManager::SpawnSmoke(glm::vec2 pos, glm::vec2 vel) {
    Particle p;
    p.type = ParticleType::SMOKE;
    p.pos = pos;
    p.vel = vel;
    p.life = 0.0f;
    p.maxLife = 0.8f;
    p.size = 14.0f;
    p.color = glm::vec4(0.4f, 0.4f, 0.4f, 0.65f);
    m_particles.push_back(p);
}

void EntityManager::SpawnFloatingText(glm::vec2 pos, const std::string& text, glm::vec4 color) {
    FloatingText ft;
    ft.pos = pos;
    ft.text = text;
    ft.color = color;
    ft.life = 0.0f;
    ft.maxLife = 1.4f;
    m_floatingTexts.push_back(ft);
}

void EntityManager::UpdateParticles(float dt) {
    for (auto& p : m_particles) {
        p.life += dt;
        p.pos += p.vel * dt;

        if (p.type == ParticleType::EXPLOSION) {
            float frac = p.life / p.maxLife;
            p.frame = std::min(3, int(frac * 4.0f));
        } else if (p.type == ParticleType::SMOKE) {
            p.size += dt * 12.0f;
            p.color.a = std::max(0.0f, (1.0f - (p.life / p.maxLife)) * 0.7f);
        } else if (p.type == ParticleType::SPARK) {
            p.vel *= 0.92f;
            p.color.a = std::max(0.0f, 1.0f - (p.life / p.maxLife));
        }
    }

    m_particles.erase(
        std::remove_if(m_particles.begin(), m_particles.end(), [](const Particle& p) {
            return p.life >= p.maxLife;
        }),
        m_particles.end()
    );

    for (auto& ft : m_floatingTexts) {
        ft.life += dt;
        ft.pos.y -= dt * 25.0f; // drift upwards
        ft.color.a = std::max(0.0f, 1.0f - (ft.life / ft.maxLife));
    }

    m_floatingTexts.erase(
        std::remove_if(m_floatingTexts.begin(), m_floatingTexts.end(), [](const FloatingText& ft) {
            return ft.life >= ft.maxLife;
        }),
        m_floatingTexts.end()
    );
}

void EntityManager::RenderParticles(Renderer& renderer) {
    for (const auto& p : m_particles) {
        if (p.type == ParticleType::EXPLOSION) {
            SpriteId fId = SpriteId::EXPLOSION_FRAME_0;
            switch (p.frame) {
                case 0: fId = SpriteId::EXPLOSION_FRAME_0; break;
                case 1: fId = SpriteId::EXPLOSION_FRAME_1; break;
                case 2: fId = SpriteId::EXPLOSION_FRAME_2; break;
                case 3: fId = SpriteId::EXPLOSION_FRAME_3; break;
            }
            renderer.DrawSpriteCentered(fId, p.pos.x, p.pos.y, p.size, p.size, 0.0f, p.color);
        } else if (p.type == ParticleType::SMOKE) {
            renderer.DrawSpriteCentered(SpriteId::PARTICLE_SMOKE, p.pos.x, p.pos.y, p.size, p.size, 0.0f, p.color);
        } else if (p.type == ParticleType::SPARK) {
            renderer.DrawSpriteCentered(SpriteId::PARTICLE_SPARK, p.pos.x, p.pos.y, p.size, p.size, 0.0f, p.color);
        }
    }
}

void EntityManager::RenderFloatingTexts(Renderer& renderer) {
    for (const auto& ft : m_floatingTexts) {
        renderer.DrawTextCentered(ft.text, ft.pos.x, ft.pos.y, 1.2f, ft.color, true);
    }
}

Unit* EntityManager::FindUnitById(int id) {
    for (auto& u : m_units) {
        if (u.id == id) return &u;
    }
    return nullptr;
}

Structure* EntityManager::FindStructureById(int id) {
    for (auto& s : m_structures) {
        if (s.id == id) return &s;
    }
    return nullptr;
}

void EntityManager::RemoveDeadEntities() {
    m_units.erase(
        std::remove_if(m_units.begin(), m_units.end(), [](const Unit& u) {
            return u.health <= 0.0f;
        }),
        m_units.end()
    );

    m_structures.erase(
        std::remove_if(m_structures.begin(), m_structures.end(), [](const Structure& s) {
            return s.health <= 0.0f;
        }),
        m_structures.end()
    );
}
