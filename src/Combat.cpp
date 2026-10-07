#include "Combat.hpp"
#include <cmath>
#include <algorithm>

namespace {
inline float DistSq(glm::vec2 a, glm::vec2 b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return dx * dx + dy * dy;
}

inline float RotateTowards(float current, float target, float maxDelta) {
    float diff = target - current;
    while (diff < -3.14159265f) diff += 6.2831853f;
    while (diff >  3.14159265f) diff -= 6.2831853f;
    if (std::abs(diff) <= maxDelta) return target;
    return current + (diff > 0.0f ? maxDelta : -maxDelta);
}
} // namespace

CombatSystem::CombatSystem() = default;
CombatSystem::~CombatSystem() = default;

void CombatSystem::Update(float dt, EntityManager& entities, Map& map, AudioSystem& audio,
                          int sovietPowerProd, int sovietPowerDrain, int& sovietCredits, int& alliedCredits) {
    m_globalTime += dt;

    // Update active lightning arcs
    for (auto& arc : m_activeArcs) {
        arc.timer -= dt;
    }
    m_activeArcs.erase(
        std::remove_if(m_activeArcs.begin(), m_activeArcs.end(), [](const ActiveArc& a) {
            return a.timer <= 0.0f;
        }),
        m_activeArcs.end()
    );

    bool sovietHasPower = (sovietPowerProd >= sovietPowerDrain);

    UpdateHarvesters(dt, entities, map, audio, sovietCredits, alliedCredits);
    UpdateUnits(dt, entities, map, audio, sovietCredits, alliedCredits);
    UpdateTeslaCoils(dt, entities, audio, sovietHasPower);
    UpdateAlliedDefenses(dt, entities, audio);
    UpdateProjectiles(dt, entities, audio);
}

void CombatSystem::UpdateUnits(float dt, EntityManager& entities, Map& /*map*/, AudioSystem& audio,
                               int& /*sovietCredits*/, int& /*alliedCredits*/) {
    for (auto& u : entities.GetUnits()) {
        if (u.health <= 0.0f) continue;
        if (u.type == UnitType::HARVESTER) continue; // Handled separately

        if (u.reloadTimer > 0.0f) {
            u.reloadTimer -= dt;
        }

        // Mammoth Tank self-repair when idle and damaged (< 50% HP)
        if (u.type == UnitType::MAMMOTH_TANK && u.state == UnitState::IDLE) {
            if (u.health < u.maxHealth * 0.5f) {
                u.selfRepairTimer += dt;
                if (u.selfRepairTimer >= 1.0f) {
                    u.selfRepairTimer = 0.0f;
                    u.health = std::min(u.maxHealth * 0.5f, u.health + 20.0f);
                    entities.SpawnSpark(u.pos, glm::vec2(0.0f, -15.0f), glm::vec4(0.2f, 1.0f, 0.4f, 1.0f));
                }
            }
        }

        // Find targets only if IDLE (moving units obey move commands until destination)
        if (u.state == UnitState::IDLE && u.targetUnitId == -1 && u.targetStructureId == -1) {
            // Scan for enemies in vision range
            float bestDistSq = (u.attackRange * 1.5f) * (u.attackRange * 1.5f);
            Faction enemyFaction = (u.faction == Faction::SOVIET) ? Faction::ALLIED : Faction::SOVIET;

            for (const auto& other : entities.GetUnits()) {
                if (other.faction == enemyFaction && other.health > 0.0f) {
                    float d2 = DistSq(u.pos, other.pos);
                    if (d2 < bestDistSq) {
                        bestDistSq = d2;
                        u.targetUnitId = other.id;
                        u.state = UnitState::ATTACKING;
                    }
                }
            }

            if (u.targetUnitId == -1) {
                for (const auto& s : entities.GetStructures()) {
                    if (s.faction == enemyFaction && s.health > 0.0f) {
                        float d2 = DistSq(u.pos, s.GetCenterWorld());
                        if (d2 < bestDistSq) {
                            bestDistSq = d2;
                            u.targetStructureId = s.id;
                            u.state = UnitState::ATTACKING;
                        }
                    }
                }
            }
        }

        // Target validation
        glm::vec2 targetPos = u.pos;
        bool hasTarget = false;
        if (u.targetUnitId != -1) {
            Unit* tgt = entities.FindUnitById(u.targetUnitId);
            if (!tgt || tgt->health <= 0.0f) {
                u.targetUnitId = -1;
                u.state = UnitState::IDLE;
            } else {
                targetPos = tgt->pos;
                hasTarget = true;
            }
        } else if (u.targetStructureId != -1) {
            Structure* tgtS = entities.FindStructureById(u.targetStructureId);
            if (!tgtS || tgtS->health <= 0.0f) {
                u.targetStructureId = -1;
                u.state = UnitState::IDLE;
            } else {
                targetPos = tgtS->GetCenterWorld();
                hasTarget = true;
            }
        }

        // State behavior
        if (u.state == UnitState::ATTACKING && hasTarget) {
            float dist = std::sqrt(DistSq(u.pos, targetPos));
            float aimAngle = std::atan2(targetPos.y - u.pos.y, targetPos.x - u.pos.x);
            u.turretAngle = RotateTowards(u.turretAngle, aimAngle, dt * 5.0f);

            if (dist > u.attackRange) {
                // Move towards target
                glm::vec2 dir = (targetPos - u.pos) / dist;
                u.bodyAngle = RotateTowards(u.bodyAngle, std::atan2(dir.y, dir.x), dt * 4.0f);
                u.pos += dir * (u.speed * dt);
                u.animTimer += dt * 8.0f;
            } else {
                // In range: Fire weapon!
                if (u.reloadTimer <= 0.0f && std::abs(u.turretAngle - aimAngle) < 0.25f) {
                    u.reloadTimer = u.reloadTime;

                    if (u.type == UnitType::CONSCRIPT || u.type == UnitType::ALLIED_RIFLEMAN) {
                        entities.SpawnProjectile(ProjectileType::BULLET, u.faction, u.pos, targetPos, u.damage, u.targetUnitId, u.targetStructureId);
                        audio.Play(SoundId::RIFLE_FIRE, 0.7f);
                    } else if (u.type == UnitType::TESLA_TROOPER) {
                        m_activeArcs.push_back({ u.pos, targetPos, 0.15f });
                        audio.Play(SoundId::TESLA_ZAP, 0.7f);
                        if (u.targetUnitId != -1) {
                            Unit* tU = entities.FindUnitById(u.targetUnitId);
                            if (tU) tU->health -= u.damage;
                        } else if (u.targetStructureId != -1) {
                            Structure* tS = entities.FindStructureById(u.targetStructureId);
                            if (tS) tS->health -= u.damage;
                        }
                    } else if (u.type == UnitType::HEAVY_TANK) {
                        u.barrelAlternator = 1 - u.barrelAlternator;
                        glm::vec2 perp(-std::sin(u.turretAngle), std::cos(u.turretAngle));
                        glm::vec2 muzzle = u.pos + perp * (u.barrelAlternator ? 5.0f : -5.0f);
                        entities.SpawnProjectile(ProjectileType::SHELL, u.faction, muzzle, targetPos, u.damage, u.targetUnitId, u.targetStructureId);
                        audio.Play(SoundId::TANK_CANNON, 0.85f);
                    } else if (u.type == UnitType::MAMMOTH_TANK) {
                        // Twin heavy shells + dual missiles!
                        entities.SpawnProjectile(ProjectileType::SHELL, u.faction, u.pos, targetPos, u.damage, u.targetUnitId, u.targetStructureId);
                        entities.SpawnProjectile(ProjectileType::ROCKET, u.faction, u.pos, targetPos, 35.0f, u.targetUnitId, u.targetStructureId);
                        audio.Play(SoundId::MAMMOTH_FIRE, 1.0f);
                    } else if (u.type == UnitType::V2_ROCKET) {
                        entities.SpawnProjectile(ProjectileType::ROCKET, u.faction, u.pos, targetPos, u.damage, u.targetUnitId, u.targetStructureId);
                        audio.Play(SoundId::ROCKET_LAUNCH, 0.9f);
                    } else if (u.type == UnitType::ALLIED_LIGHT_TANK || u.type == UnitType::ALLIED_MEDIUM_TANK) {
                        entities.SpawnProjectile(ProjectileType::SHELL, u.faction, u.pos, targetPos, u.damage, u.targetUnitId, u.targetStructureId);
                        audio.Play(SoundId::TANK_CANNON, 0.8f);
                    }
                }
            }
        } else if (!u.waypoints.empty()) {
            // Move along waypoints
            glm::vec2 nextWp = u.waypoints.front();
            float dist = std::sqrt(DistSq(u.pos, nextWp));
            float step = u.speed * dt;

            if (dist <= std::max(10.0f, step)) {
                u.pos = nextWp;
                u.waypoints.erase(u.waypoints.begin());
                if (u.waypoints.empty()) {
                    u.state = UnitState::IDLE;
                }
            } else {
                glm::vec2 dir = (nextWp - u.pos) / dist;
                float moveAngle = std::atan2(dir.y, dir.x);
                u.bodyAngle = RotateTowards(u.bodyAngle, moveAngle, dt * 8.0f);
                u.turretAngle = RotateTowards(u.turretAngle, moveAngle, dt * 8.0f);
                u.pos += dir * step;
                u.animTimer += dt * 8.0f;
            }
        } else {
            u.state = UnitState::IDLE;
        }
    }

    // Soft separation between units to prevent clumping
    for (size_t i = 0; i < entities.GetUnits().size(); ++i) {
        auto& u1 = entities.GetUnits()[i];
        if (u1.health <= 0.0f) continue;
        float r1 = (u1.type == UnitType::CONSCRIPT || u1.type == UnitType::ALLIED_RIFLEMAN) ? 14.0f : 24.0f;

        for (size_t j = i + 1; j < entities.GetUnits().size(); ++j) {
            auto& u2 = entities.GetUnits()[j];
            if (u2.health <= 0.0f) continue;
            float r2 = (u2.type == UnitType::CONSCRIPT || u2.type == UnitType::ALLIED_RIFLEMAN) ? 14.0f : 24.0f;

            float minDist = r1 + r2;
            float d2 = DistSq(u1.pos, u2.pos);
            if (d2 < minDist * minDist && d2 > 0.001f) {
                float d = std::sqrt(d2);
                glm::vec2 push = ((u1.pos - u2.pos) / d) * (minDist - d) * 0.5f;
                u1.pos += push * (dt * 12.0f);
                u2.pos -= push * (dt * 12.0f);
            }
        }
    }
}

void CombatSystem::UpdateTeslaCoils(float dt, EntityManager& entities, AudioSystem& audio, bool sovietHasPower) {
    for (auto& s : entities.GetStructures()) {
        if (s.type != StructureType::SOVIET_TESLA_COIL || s.health <= 0.0f) continue;

        if (!sovietHasPower) {
            s.teslaState = TeslaState::IDLE;
            s.teslaTimer = 0.0f;
            continue;
        }

        glm::vec2 coilCenter = s.GetCenterWorld();
        glm::vec2 coilTip = coilCenter + glm::vec2(0.0f, -98.0f);
        float coilRangeSq = 280.0f * 280.0f;

        if (s.teslaState == TeslaState::IDLE) {
            // Scan for Allied targets
            int bestTgt = -1;
            float bestDist = coilRangeSq;
            for (const auto& u : entities.GetUnits()) {
                if (u.faction == Faction::ALLIED && u.health > 0.0f) {
                    float d2 = DistSq(coilCenter, u.pos);
                    if (d2 < bestDist) {
                        bestDist = d2;
                        bestTgt = u.id;
                    }
                }
            }

            if (bestTgt != -1) {
                s.teslaTargetUnitId = bestTgt;
                s.teslaState = TeslaState::CHARGING;
                s.teslaTimer = 0.7f;
                audio.Play(SoundId::TESLA_CHARGE, 1.0f);
            }
        } else if (s.teslaState == TeslaState::CHARGING) {
            s.teslaTimer -= dt;
            // Spawn charging plasma sparks at coil tip
            entities.SpawnSpark(coilTip, glm::vec2((m_globalTime * 10.0f), -10.0f), glm::vec4(0.0f, 0.9f, 1.0f, 1.0f));

            if (s.teslaTimer <= 0.0f) {
                // Fire lightning discharge!
                Unit* tgt = entities.FindUnitById(s.teslaTargetUnitId);
                if (tgt && tgt->health > 0.0f) {
                    m_activeArcs.push_back({ coilTip, tgt->pos, 0.22f });
                    audio.Play(SoundId::TESLA_ZAP, 1.0f);
                    tgt->health -= 140.0f;
                    entities.SpawnExplosion(tgt->pos, 0.7f);
                }
                s.teslaState = TeslaState::COOLDOWN;
                s.teslaTimer = 1.6f;
            }
        } else if (s.teslaState == TeslaState::COOLDOWN) {
            s.teslaTimer -= dt;
            if (s.teslaTimer <= 0.0f) {
                s.teslaState = TeslaState::IDLE;
            }
        }
    }
}

void CombatSystem::UpdateAlliedDefenses(float dt, EntityManager& entities, AudioSystem& audio) {
    for (auto& s : entities.GetStructures()) {
        if (s.type != StructureType::ALLIED_PILLBOX || s.health <= 0.0f) continue;

        s.attackTimer -= dt;
        glm::vec2 boxCenter = s.GetCenterWorld();
        float rangeSq = 200.0f * 200.0f;

        if (s.attackTimer <= 0.0f) {
            // Find Soviet targets
            for (const auto& u : entities.GetUnits()) {
                if (u.faction == Faction::SOVIET && u.health > 0.0f) {
                    if (DistSq(boxCenter, u.pos) <= rangeSq) {
                        entities.SpawnProjectile(ProjectileType::BULLET, Faction::ALLIED, boxCenter, u.pos, 16.0f, u.id);
                        audio.Play(SoundId::RIFLE_FIRE, 0.65f);
                        s.attackTimer = 0.45f;
                        break;
                    }
                }
            }
        }
    }
}

void CombatSystem::UpdateProjectiles(float dt, EntityManager& entities, AudioSystem& audio) {
    auto& projs = entities.GetProjectiles();
    for (auto& p : projs) {
        float dx = p.targetPos.x - p.pos.x;
        float dy = p.targetPos.y - p.pos.y;
        float dist = std::sqrt(dx * dx + dy * dy);

        // Rocket smoke
        if (p.type == ProjectileType::ROCKET) {
            p.smokeTimer += dt;
            if (p.smokeTimer >= 0.04f) {
                p.smokeTimer = 0.0f;
                entities.SpawnSmoke(p.pos, glm::vec2(-std::cos(p.angle) * 20.0f, -std::sin(p.angle) * 20.0f));
            }
        }

        float step = p.speed * dt;
        if (step >= dist) {
            // Hit!
            p.pos = p.targetPos;

            if (p.targetUnitId != -1) {
                Unit* u = entities.FindUnitById(p.targetUnitId);
                if (u) u->health -= p.damage;
            } else if (p.targetStructureId != -1) {
                Structure* s = entities.FindStructureById(p.targetStructureId);
                if (s) s->health -= p.damage;
            }

            if (p.type == ProjectileType::BULLET) {
                entities.SpawnSpark(p.pos, glm::vec2(0.0f, -10.0f));
            } else if (p.type == ProjectileType::SHELL) {
                entities.SpawnExplosion(p.pos, 0.65f);
                audio.Play(SoundId::EXPLOSION_SMALL, 0.7f);
            } else if (p.type == ProjectileType::ROCKET) {
                entities.SpawnExplosion(p.pos, 1.1f);
                audio.Play(SoundId::EXPLOSION_LARGE, 0.9f);
            }

            p.damage = 0.0f; // mark for removal
        } else {
            p.pos += glm::vec2(dx / dist, dy / dist) * step;
        }
    }

    projs.erase(
        std::remove_if(projs.begin(), projs.end(), [](const Projectile& p) {
            return p.damage <= 0.0f;
        }),
        projs.end()
    );
}

void CombatSystem::UpdateHarvesters(float dt, EntityManager& entities, Map& map, AudioSystem& audio,
                                    int& sovietCredits, int& /*alliedCredits*/) {
    for (auto& u : entities.GetUnits()) {
        if (u.type != UnitType::HARVESTER || u.health <= 0.0f) continue;

        if (u.state == UnitState::IDLE) {
            if (u.oreCargo >= Unit::MAX_ORE_CARGO * 0.9f) {
                u.state = UnitState::RETURNING_TO_REFINERY;
            } else {
                // Find closest ore tile
                glm::ivec2 oreT = map.FindNearestOre(u.pos.x, u.pos.y);
                if (oreT.x != -1) {
                    u.targetOreTile = oreT;
                    u.waypoints = map.FindPath(u.pos, Map::TileCenterToWorld(oreT.x, oreT.y));
                    u.state = UnitState::MOVING;
                }
            }
        } else if (u.state == UnitState::MOVING) {
            if (u.waypoints.empty()) {
                // Arrived at destination
                if (u.oreCargo >= Unit::MAX_ORE_CARGO * 0.9f) {
                    u.state = UnitState::UNLOADING;
                    u.harvestTimer = 1.8f;
                } else if (u.targetOreTile.x != -1) {
                    u.state = UnitState::HARVESTING;
                    u.targetOreTile = glm::ivec2(-1, -1);
                } else {
                    u.state = UnitState::IDLE;
                }
            } else {
                glm::vec2 nextWp = u.waypoints.front();
                float dist = std::sqrt(DistSq(u.pos, nextWp));
                float step = u.speed * dt;
                if (dist <= std::max(10.0f, step)) {
                    u.pos = nextWp;
                    u.waypoints.erase(u.waypoints.begin());
                } else {
                    glm::vec2 dir = (nextWp - u.pos) / dist;
                    u.bodyAngle = RotateTowards(u.bodyAngle, std::atan2(dir.y, dir.x), dt * 6.0f);
                    u.pos += dir * step;
                }
            }
        } else if (u.state == UnitState::HARVESTING) {
            glm::ivec2 curTile = map.WorldToTile(u.pos.x, u.pos.y);
            float mined = map.HarvestOre(curTile.x, curTile.y, 140.0f * dt);
            u.oreCargo += mined;

            if (mined > 0.0f) {
                entities.SpawnSpark(u.pos + glm::vec2(0.0f, -10.0f), glm::vec2(0.0f, -15.0f), glm::vec4(1.0f, 0.85f, 0.0f, 1.0f));
            }

            if (u.oreCargo >= Unit::MAX_ORE_CARGO || mined <= 0.0f) {
                // Full or tile depleted, find Refinery dock
                Structure* refinery = nullptr;
                float bestDistSq = 1e9f;
                for (auto& s : entities.GetStructures()) {
                    if (s.faction == u.faction && s.type == StructureType::SOVIET_REFINERY && s.health > 0.0f) {
                        float d2 = DistSq(u.pos, s.GetCenterWorld());
                        if (d2 < bestDistSq) {
                            bestDistSq = d2;
                            refinery = &s;
                        }
                    }
                }

                if (refinery) {
                    glm::vec2 dockPos = refinery->GetCenterWorld() + glm::vec2(24.0f, 16.0f);
                    u.waypoints = map.FindPath(u.pos, dockPos);
                    u.dockRefineryId = refinery->id;
                    u.state = UnitState::MOVING;
                } else {
                    u.state = UnitState::IDLE;
                }
            }
        } else if (u.state == UnitState::RETURNING_TO_REFINERY) {
            Structure* refinery = nullptr;
            for (auto& s : entities.GetStructures()) {
                if (s.faction == u.faction && s.type == StructureType::SOVIET_REFINERY && s.health > 0.0f) {
                    refinery = &s;
                    break;
                }
            }
            if (refinery) {
                glm::vec2 dockPos = refinery->GetCenterWorld() + glm::vec2(24.0f, 16.0f);
                u.waypoints = map.FindPath(u.pos, dockPos);
                u.state = UnitState::MOVING;
            } else {
                u.state = UnitState::IDLE;
            }
        } else if (u.state == UnitState::UNLOADING) {
            u.harvestTimer -= dt;
            if (u.harvestTimer <= 0.0f) {
                int earned = int(u.oreCargo);
                u.oreCargo = 0.0f;
                if (u.faction == Faction::SOVIET) {
                    sovietCredits += earned;
                    audio.Play(SoundId::CREDITS_TICK, 1.0f);
                    entities.SpawnFloatingText(u.pos, "+$" + std::to_string(earned), glm::vec4(1.0f, 0.85f, 0.1f, 1.0f));
                }
                u.state = UnitState::IDLE;
            }
        }
    }
}

void CombatSystem::RenderActiveTeslaArcs(Renderer& renderer) {
    for (const auto& arc : m_activeArcs) {
        renderer.DrawTeslaArc(arc.start.x, arc.start.y, arc.end.x, arc.end.y, m_globalTime);
    }
}
