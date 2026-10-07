#include "Game.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>

Game::Game() = default;
Game::~Game() = default;

bool Game::Initialize(GLFWwindow* window, int winW, int winH, int fbW, int fbH) {
    m_window = window;
    m_windowW = winW;
    m_windowH = winH;

    if (!m_audio.Initialize()) {
        std::cerr << "Game: Warning: Audio failed to initialize." << std::endl;
    }

    if (!m_renderer.Initialize(winW, winH, fbW, fbH)) {
        std::cerr << "Game: Error: Renderer failed to initialize." << std::endl;
        return false;
    }

    m_map.Initialize(72, 72);
    m_ui.Initialize();

    SetupInitialBattlefield();

    // Immediately sync camera with renderer so initial frame renders the base centered in the main window
    m_renderer.SetCamera(m_camX, m_camY, m_zoom);

    std::cout << "Game: Soviet RTS initialized successfully." << std::endl;
    return true;
}

void Game::SetupInitialBattlefield() {
    m_entities.Clear();

    // Setup Soviet Base in Southwestern region with all iconic structures
    // 1. Soviet Construction Yard (3x3)
    Structure& cy = m_entities.CreateStructure(StructureType::SOVIET_CONYARD, Faction::SOVIET, 14, 56);
    m_map.SetBuildingOccupation(cy.tileX, cy.tileY, cy.wTiles, cy.hTiles, cy.id);

    // 2. Primary Tesla Reactor (2x2)
    Structure& pwr1 = m_entities.CreateStructure(StructureType::SOVIET_POWER, Faction::SOVIET, 10, 52);
    m_map.SetBuildingOccupation(pwr1.tileX, pwr1.tileY, pwr1.wTiles, pwr1.hTiles, pwr1.id);

    // 3. Auxiliary Tesla Reactor (2x2)
    Structure& pwr2 = m_entities.CreateStructure(StructureType::SOVIET_POWER, Faction::SOVIET, 10, 48);
    m_map.SetBuildingOccupation(pwr2.tileX, pwr2.tileY, pwr2.wTiles, pwr2.hTiles, pwr2.id);

    // 4. Soviet Radar Dome (2x2)
    Structure& rad = m_entities.CreateStructure(StructureType::SOVIET_RADAR, Faction::SOVIET, 14, 52);
    m_map.SetBuildingOccupation(rad.tileX, rad.tileY, rad.wTiles, rad.hTiles, rad.id);

    // 5. Soviet Ore Refinery (3x3) - Adjacent to the gold ore field
    Structure& ref = m_entities.CreateStructure(StructureType::SOVIET_REFINERY, Faction::SOVIET, 19, 51);
    m_map.SetBuildingOccupation(ref.tileX, ref.tileY, ref.wTiles, ref.hTiles, ref.id);

    // 6. Soviet Barracks (2x2)
    Structure& bar = m_entities.CreateStructure(StructureType::SOVIET_BARRACKS, Faction::SOVIET, 10, 57);
    m_map.SetBuildingOccupation(bar.tileX, bar.tileY, bar.wTiles, bar.hTiles, bar.id);

    // 7. Soviet War Factory (3x3)
    Structure& wf = m_entities.CreateStructure(StructureType::SOVIET_WARFACTORY, Faction::SOVIET, 14, 61);
    m_map.SetBuildingOccupation(wf.tileX, wf.tileY, wf.wTiles, wf.hTiles, wf.id);

    // 8. Soviet Tesla Coil (2x2) - Defending eastern approach
    Structure& tc = m_entities.CreateStructure(StructureType::SOVIET_TESLA_COIL, Faction::SOVIET, 19, 56);
    m_map.SetBuildingOccupation(tc.tileX, tc.tileY, tc.wTiles, tc.hTiles, tc.id);

    // Initial Soviet Army deployed around base
    m_entities.CreateUnit(UnitType::HARVESTER, Faction::SOVIET, Map::TileCenterToWorld(21, 55));
    m_entities.CreateUnit(UnitType::HEAVY_TANK, Faction::SOVIET, Map::TileCenterToWorld(17, 60));
    m_entities.CreateUnit(UnitType::HEAVY_TANK, Faction::SOVIET, Map::TileCenterToWorld(19, 61));
    m_entities.CreateUnit(UnitType::CONSCRIPT, Faction::SOVIET, Map::TileCenterToWorld(12, 60));
    m_entities.CreateUnit(UnitType::CONSCRIPT, Faction::SOVIET, Map::TileCenterToWorld(13, 60));
    m_entities.CreateUnit(UnitType::CONSCRIPT, Faction::SOVIET, Map::TileCenterToWorld(14, 60));
    m_entities.CreateUnit(UnitType::TESLA_TROOPER, Faction::SOVIET, Map::TileCenterToWorld(18, 59));

    // Initialize Enemy Allied AI Base
    m_alliedAI.Initialize(m_entities, m_map);

    // Center camera on Soviet Base in the main window
    // Center point of the base cluster in isometric coordinates:
    // cx = 15.5, cy = 56.0 -> isoX = -1296.0f, isoY = 1144.0f
    // Offset by +half sidebar width so the base is centered in the visible tactical window
    float baseCenterX = -1296.0f;
    float baseCenterY = 1144.0f;
    m_zoom = 1.0f;
    m_camX = baseCenterX + (UI::SIDEBAR_WIDTH * 0.5f) / m_zoom;
    m_camY = baseCenterY;
}

void Game::Update(float dt) {
    if (m_paused) return;

    HandleCameraPan(dt);
    m_renderer.SetCamera(m_camX, m_camY, m_zoom);

    // 1. Calculate Soviet Power production vs drain
    m_sovietPowerProd = 0;
    m_sovietPowerDrain = 0;
    for (const auto& s : m_entities.GetStructures()) {
        if (s.faction == Faction::SOVIET && s.health > 0.0f) {
            m_sovietPowerProd += s.powerProduction;
            m_sovietPowerDrain += s.powerDrain;
        }
    }

    // 2. Update Fog of War (reveal around Soviet units & structures)
    m_map.BeginFogPass();
    for (const auto& u : m_entities.GetUnits()) {
        if (u.faction == Faction::SOVIET && u.health > 0.0f) {
            m_map.RevealVision(u.pos.x, u.pos.y, u.sightRadius);
        }
    }
    for (const auto& s : m_entities.GetStructures()) {
        if (s.faction == Faction::SOVIET && s.health > 0.0f) {
            glm::vec2 c = s.GetCenterWorld();
            m_map.RevealVision(c.x, c.y, s.sightRadius);
        }
    }

    // 3. Ore regeneration
    m_map.RegenerateOre(dt);

    // 4. Combat & movement simulation
    m_combat.Update(dt, m_entities, m_map, m_audio, m_sovietPowerProd, m_sovietPowerDrain, m_sovietCredits, m_alliedCredits);

    // 5. Enemy Allied AI
    m_alliedAI.Update(dt, m_entities, m_map, m_audio);

    // 6. Particle & cleanup
    m_entities.UpdateParticles(dt);
    m_entities.RemoveDeadEntities();

    // 7. UI update
    m_ui.Update(dt, m_sovietCredits, m_entities, m_map, m_audio);

    // 8. Order feedback marker timer
    if (m_orderMarkerTimer > 0.0f) {
        m_orderMarkerTimer -= dt;
    }
}

void Game::HandleCameraPan(float dt) {
    float pan = m_camSpeed * dt * (1.0f / m_zoom);

    if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(m_window, GLFW_KEY_UP) == GLFW_PRESS) {
        m_camY -= pan;
    }
    if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS || glfwGetKey(m_window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        m_camY += pan;
    }
    if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(m_window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        m_camX -= pan;
    }
    if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS || glfwGetKey(m_window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        m_camX += pan;
    }

    // Clamp camera within isometric battlefield diamond bounds
    float halfMapX = float(m_map.GetWidth()) * (Map::TILE_W * 0.5f);
    float maxMapY = float(m_map.GetWidth() + m_map.GetHeight()) * (Map::TILE_H * 0.5f);
    m_camX = std::max(-halfMapX + 150.0f, std::min(halfMapX - 150.0f, m_camX));
    m_camY = std::max(50.0f, std::min(maxMapY - 50.0f, m_camY));
}

void Game::Render() {
    m_renderer.BeginFrame();

    float halfW = (float(m_windowW) * 0.5f) / m_zoom;
    float halfH = (float(m_windowH) * 0.5f) / m_zoom;

    // 1. Render Map isometric terrain & shroud
    m_map.Render(m_renderer, m_camX, m_camY, halfW, halfH);

    // 2. 2.5D Isometric Depth-Sorted Rendering
    // Collect structures and units to render in ascending Y (back to front)
    struct DepthItem {
        float sortY;
        enum { STRUCTURE, UNIT } type;
        size_t index;
    };
    std::vector<DepthItem> items;
    items.reserve(m_entities.GetStructures().size() + m_entities.GetUnits().size());

    const auto& structures = m_entities.GetStructures();
    for (size_t i = 0; i < structures.size(); ++i) {
        if (structures[i].health > 0.0f) {
            // Hide enemy structures in unexplored shroud
            if (structures[i].faction == Faction::ALLIED &&
                m_map.GetShroudAt(structures[i].tileX, structures[i].tileY) == ShroudStatus::SHROUDED) {
                continue;
            }
            items.push_back({ structures[i].GetCenterWorld().y, DepthItem::STRUCTURE, i });
        }
    }

    const auto& units = m_entities.GetUnits();
    for (size_t i = 0; i < units.size(); ++i) {
        if (units[i].health > 0.0f) {
            // Hide enemy units in fog
            if (units[i].faction == Faction::ALLIED && !m_map.IsVisibleWorld(units[i].pos.x, units[i].pos.y)) {
                continue;
            }
            items.push_back({ units[i].pos.y, DepthItem::UNIT, i });
        }
    }

    std::sort(items.begin(), items.end(), [](const DepthItem& a, const DepthItem& b) {
        return a.sortY < b.sortY;
    });

    for (const auto& item : items) {
        if (item.type == DepthItem::STRUCTURE) {
            const auto& s = structures[item.index];
            glm::vec2 c = s.GetCenterWorld();

            SpriteId spId = SpriteId::SOVIET_CONYARD;
            float sw = 192.0f;
            float sh = 150.0f;
            float baseCenterY = 100.0f;

            switch (s.type) {
                case StructureType::SOVIET_CONYARD:
                    spId = SpriteId::SOVIET_CONYARD; sw = 192.0f; sh = 150.0f; baseCenterY = 100.0f; break;
                case StructureType::SOVIET_POWER:
                    spId = SpriteId::SOVIET_POWER; sw = 128.0f; sh = 112.0f; baseCenterY = 78.0f; break;
                case StructureType::SOVIET_REFINERY:
                    spId = SpriteId::SOVIET_REFINERY; sw = 192.0f; sh = 150.0f; baseCenterY = 100.0f; break;
                case StructureType::SOVIET_BARRACKS:
                    spId = SpriteId::SOVIET_BARRACKS; sw = 128.0f; sh = 110.0f; baseCenterY = 76.0f; break;
                case StructureType::SOVIET_WARFACTORY:
                    spId = SpriteId::SOVIET_WARFACTORY; sw = 192.0f; sh = 155.0f; baseCenterY = 105.0f; break;
                case StructureType::SOVIET_RADAR:
                    spId = SpriteId::SOVIET_RADAR; sw = 128.0f; sh = 120.0f; baseCenterY = 86.0f; break;
                case StructureType::SOVIET_TESLA_COIL:
                    spId = SpriteId::SOVIET_TESLA_COIL; sw = 128.0f; sh = 150.0f; baseCenterY = 116.0f; break;
                case StructureType::ALLIED_CONYARD:
                    spId = SpriteId::ALLIED_CONYARD; sw = 192.0f; sh = 150.0f; baseCenterY = 100.0f; break;
                case StructureType::ALLIED_POWER:
                    spId = SpriteId::ALLIED_POWER; sw = 128.0f; sh = 112.0f; baseCenterY = 78.0f; break;
                case StructureType::ALLIED_BARRACKS:
                    spId = SpriteId::ALLIED_BARRACKS; sw = 128.0f; sh = 110.0f; baseCenterY = 76.0f; break;
                case StructureType::ALLIED_WARFACTORY:
                    spId = SpriteId::ALLIED_WARFACTORY; sw = 192.0f; sh = 155.0f; baseCenterY = 105.0f; break;
                case StructureType::ALLIED_PILLBOX:
                    spId = SpriteId::ALLIED_PILLBOX; sw = 128.0f; sh = 85.0f; baseCenterY = 51.0f; break;
            }

            float drawX = c.x - sw * 0.5f;
            float drawY = c.y - baseCenterY;

            glm::vec4 tint(1.0f);
            if (s.faction == Faction::ALLIED && m_map.GetShroudAt(s.tileX, s.tileY) == ShroudStatus::FOG) {
                tint = glm::vec4(0.6f, 0.6f, 0.6f, 1.0f);
            }

            m_renderer.DrawSprite(spId, drawX, drawY, sw, sh, 0.0f, tint);

            // Tesla Coil glowing charging aura at discharge sphere
            if (s.type == StructureType::SOVIET_TESLA_COIL && s.teslaState == TeslaState::CHARGING) {
                glm::vec2 tip = c + glm::vec2(0.0f, -98.0f);
                m_renderer.DrawCircleOutline(tip.x, tip.y, 16.0f, glm::vec4(0.0f, 0.9f, 1.0f, 0.85f));
                m_renderer.DrawCircleOutline(tip.x, tip.y, 9.0f, glm::vec4(0.8f, 1.0f, 1.0f, 0.95f));
            }

            // Health bar & Isometric selection outline
            if (s.selected || s.health < s.maxHealth) {
                float frac = s.health / s.maxHealth;
                glm::vec4 hpCol = frac > 0.5f ? glm::vec4(0.15f, 0.85f, 0.2f, 1.0f)
                                : frac > 0.25f ? glm::vec4(0.9f, 0.85f, 0.1f, 1.0f)
                                : glm::vec4(0.9f, 0.15f, 0.15f, 1.0f);

                float barW = (s.wTiles >= 3) ? 120.0f : 80.0f;
                float barX = c.x - barW * 0.5f;
                float barY = drawY - 6.0f;

                m_renderer.DrawQuad(barX, barY, barW, 4.0f, glm::vec4(0.1f, 0.1f, 0.1f, 0.9f));
                m_renderer.DrawQuad(barX, barY, barW * frac, 4.0f, hpCol);
                m_renderer.DrawRectOutline(barX, barY, barW, 4.0f, glm::vec4(0.5f, 0.5f, 0.5f, 1.0f));

                if (s.selected) {
                    float diamondW = (s.wTiles >= 3) ? 192.0f : 128.0f;
                    float diamondH = (s.hTiles >= 3) ? 96.0f : 64.0f;
                    m_renderer.DrawDiamondOutline(c.x, c.y, diamondW, diamondH, glm::vec4(0.2f, 1.0f, 0.3f, 0.85f));
                    m_renderer.DrawDiamondOutline(c.x, c.y, diamondW + 2.0f, diamondH + 1.0f, glm::vec4(0.1f, 0.8f, 0.2f, 0.45f));
                }
            }

        } else if (item.type == DepthItem::UNIT) {
            const auto& u = units[item.index];

            SpriteId bodyId = SpriteId::SOVIET_CONSCRIPT;
            SpriteId turretId = SpriteId::SOVIET_HEAVY_TANK_TURRET;
            bool hasTurret = false;
            float size = 42.0f;

            switch (u.type) {
                case UnitType::CONSCRIPT:
                    bodyId = SpriteId::SOVIET_CONSCRIPT; size = 28.0f; break;
                case UnitType::TESLA_TROOPER:
                    bodyId = SpriteId::SOVIET_TESLA_TROOPER; size = 28.0f; break;
                case UnitType::HEAVY_TANK:
                    bodyId = SpriteId::SOVIET_HEAVY_TANK_BODY;
                    turretId = SpriteId::SOVIET_HEAVY_TANK_TURRET;
                    hasTurret = true; size = 44.0f; break;
                case UnitType::MAMMOTH_TANK:
                    bodyId = SpriteId::SOVIET_MAMMOTH_TANK_BODY;
                    turretId = SpriteId::SOVIET_MAMMOTH_TANK_TURRET;
                    hasTurret = true; size = 56.0f; break;
                case UnitType::V2_ROCKET:
                    bodyId = SpriteId::SOVIET_V2_LAUNCHER; size = 44.0f; break;
                case UnitType::HARVESTER:
                    bodyId = SpriteId::SOVIET_HARVESTER; size = 50.0f; break;
                case UnitType::ALLIED_RIFLEMAN:
                    bodyId = SpriteId::ALLIED_RIFLEMAN; size = 28.0f; break;
                case UnitType::ALLIED_LIGHT_TANK:
                    bodyId = SpriteId::ALLIED_LIGHT_TANK_BODY;
                    turretId = SpriteId::ALLIED_LIGHT_TANK_TURRET;
                    hasTurret = true; size = 36.0f; break;
                case UnitType::ALLIED_MEDIUM_TANK:
                    bodyId = SpriteId::ALLIED_MEDIUM_TANK_BODY;
                    turretId = SpriteId::ALLIED_MEDIUM_TANK_TURRET;
                    hasTurret = true; size = 44.0f; break;
            }

            // Draw unit body
            m_renderer.DrawSpriteCentered(bodyId, u.pos.x, u.pos.y, size, size, u.bodyAngle + 1.570796f);

            // Draw independently rotating turret
            if (hasTurret) {
                m_renderer.DrawSpriteCentered(turretId, u.pos.x, u.pos.y, size, size, u.turretAngle + 1.570796f);
            }

            // Selection brackets & Health bar
            if (u.selected || u.health < u.maxHealth) {
                float frac = u.health / u.maxHealth;
                glm::vec4 hpCol = frac > 0.5f ? glm::vec4(0.15f, 0.85f, 0.2f, 1.0f)
                                : frac > 0.25f ? glm::vec4(0.9f, 0.85f, 0.1f, 1.0f)
                                : glm::vec4(0.9f, 0.15f, 0.15f, 1.0f);
                float barW = size * 0.8f;
                m_renderer.DrawQuadCentered(u.pos.x, u.pos.y - size * 0.58f, barW, 4.0f, glm::vec4(0.1f, 0.1f, 0.1f, 0.9f));
                m_renderer.DrawQuad(u.pos.x - barW * 0.5f, u.pos.y - size * 0.58f - 2.0f, barW * frac, 4.0f, hpCol);

                if (u.selected) {
                    // Isometric diamond selection ring on the ground
                    m_renderer.DrawDiamondOutline(u.pos.x, u.pos.y + 4.0f, size * 1.1f, size * 0.55f, glm::vec4(0.1f, 1.0f, 0.2f, 0.9f));
                }
            }
        }
    }

    // 3. Render Projectiles
    for (const auto& p : m_entities.GetProjectiles()) {
        SpriteId prjId = SpriteId::PROJECTILE_BULLET;
        float pSize = 10.0f;
        if (p.type == ProjectileType::SHELL) {
            prjId = SpriteId::PROJECTILE_SHELL;
            pSize = 14.0f;
        } else if (p.type == ProjectileType::ROCKET) {
            prjId = SpriteId::PROJECTILE_ROCKET;
            pSize = 20.0f;
        }
        m_renderer.DrawSpriteCentered(prjId, p.pos.x, p.pos.y, pSize, pSize, p.angle + 1.570796f);
    }

    // 4. Render Tesla Lightning Arcs
    m_combat.RenderActiveTeslaArcs(m_renderer);

    // 5. Render Particles & Floating texts
    m_entities.RenderParticles(m_renderer);
    m_entities.RenderFloatingTexts(m_renderer);

    // 6. Isometric Building Placement Mode preview
    if (m_ui.IsInPlacementMode()) {
        glm::vec2 mouseWorld = m_renderer.ScreenToWorld(float(m_mouseX), float(m_mouseY));
        glm::ivec2 tile = Map::WorldToTile(mouseWorld.x, mouseWorld.y);

        int wTiles = 2, hTiles = 2;
        StructureType pType = m_ui.GetPlacingType();
        SpriteId previewSprite = SpriteId::SOVIET_POWER;
        float pSw = 128.0f, pSh = 112.0f, pBaseCenterY = 78.0f;

        switch (pType) {
            case StructureType::SOVIET_CONYARD:
                wTiles = 3; hTiles = 3; previewSprite = SpriteId::SOVIET_CONYARD; pSw = 192.0f; pSh = 150.0f; pBaseCenterY = 100.0f; break;
            case StructureType::SOVIET_POWER:
                wTiles = 2; hTiles = 2; previewSprite = SpriteId::SOVIET_POWER; pSw = 128.0f; pSh = 112.0f; pBaseCenterY = 78.0f; break;
            case StructureType::SOVIET_REFINERY:
                wTiles = 3; hTiles = 3; previewSprite = SpriteId::SOVIET_REFINERY; pSw = 192.0f; pSh = 150.0f; pBaseCenterY = 100.0f; break;
            case StructureType::SOVIET_BARRACKS:
                wTiles = 2; hTiles = 2; previewSprite = SpriteId::SOVIET_BARRACKS; pSw = 128.0f; pSh = 110.0f; pBaseCenterY = 76.0f; break;
            case StructureType::SOVIET_WARFACTORY:
                wTiles = 3; hTiles = 3; previewSprite = SpriteId::SOVIET_WARFACTORY; pSw = 192.0f; pSh = 155.0f; pBaseCenterY = 105.0f; break;
            case StructureType::SOVIET_RADAR:
                wTiles = 2; hTiles = 2; previewSprite = SpriteId::SOVIET_RADAR; pSw = 128.0f; pSh = 120.0f; pBaseCenterY = 86.0f; break;
            case StructureType::SOVIET_TESLA_COIL:
                wTiles = 2; hTiles = 2; previewSprite = SpriteId::SOVIET_TESLA_COIL; pSw = 128.0f; pSh = 150.0f; pBaseCenterY = 116.0f; break;
            default: break;
        }

        bool canPlace = m_map.CanPlaceBuilding(tile.x, tile.y, wTiles, hTiles);
        glm::vec4 gridFill = canPlace ? glm::vec4(0.1f, 0.9f, 0.2f, 0.35f) : glm::vec4(0.9f, 0.1f, 0.1f, 0.35f);
        glm::vec4 gridOutline = canPlace ? glm::vec4(0.3f, 1.0f, 0.4f, 0.85f) : glm::vec4(1.0f, 0.2f, 0.2f, 0.85f);

        for (int y = 0; y < hTiles; ++y) {
            for (int x = 0; x < wTiles; ++x) {
                glm::vec2 tc = Map::TileCenterToWorld(tile.x + x, tile.y + y);
                m_renderer.DrawDiamondFilled(tc.x, tc.y, Map::TILE_W, Map::TILE_H, gridFill);
                m_renderer.DrawDiamondOutline(tc.x, tc.y, Map::TILE_W, Map::TILE_H, gridOutline);
            }
        }

        // Render semi-transparent 3D isometric building ghost preview
        float cx = float(tile.x) + float(wTiles) * 0.5f;
        float cy = float(tile.y) + float(hTiles) * 0.5f;
        glm::vec2 previewCenter((cx - cy) * 32.0f, (cx + cy) * 16.0f);
        glm::vec4 ghostTint = canPlace ? glm::vec4(0.6f, 1.0f, 0.6f, 0.75f) : glm::vec4(1.0f, 0.4f, 0.4f, 0.7f);
        m_renderer.DrawSprite(previewSprite, previewCenter.x - pSw * 0.5f, previewCenter.y - pBaseCenterY, pSw, pSh, 0.0f, ghostTint);
    }

    // 7. Tactical Order Marker (animated feedback on ground in world coordinates)
    if (m_orderMarkerTimer > 0.0f) {
        float alpha = m_orderMarkerTimer / 0.5f;
        float progress = 1.0f - alpha;
        float radW = 28.0f + progress * 24.0f;
        float radH = 14.0f + progress * 12.0f;
        glm::vec4 col = m_orderMarkerIsAttack
            ? glm::vec4(1.0f, 0.15f, 0.15f, alpha * 0.9f)
            : glm::vec4(0.15f, 1.0f, 0.3f, alpha * 0.9f);
        m_renderer.DrawDiamondOutline(m_orderMarkerPos.x, m_orderMarkerPos.y, radW, radH, col);
        m_renderer.DrawDiamondOutline(m_orderMarkerPos.x, m_orderMarkerPos.y, radW * 0.65f, radH * 0.65f, col);
    }

    // 8. Box Selection Outline (rendered in screen coordinates)
    if (m_isBoxSelecting) {
        m_renderer.SetScreenMode();
        float bx = std::min(m_boxStartScreen.x, m_boxEndScreen.x);
        float by = std::min(m_boxStartScreen.y, m_boxEndScreen.y);
        float bw = std::abs(m_boxStartScreen.x - m_boxEndScreen.x);
        float bh = std::abs(m_boxStartScreen.y - m_boxEndScreen.y);
        m_renderer.DrawQuad(bx, by, bw, bh, glm::vec4(0.1f, 0.8f, 0.2f, 0.15f));
        m_renderer.DrawRectOutline(bx, by, bw, bh, glm::vec4(0.2f, 1.0f, 0.3f, 0.9f));
        m_renderer.SetWorldMode();
    }

    // 8. Render HUD & Sidebar
    m_ui.Render(m_renderer, m_entities, m_map, m_sovietCredits, m_sovietPowerProd, m_sovietPowerDrain, m_camX, m_camY, m_zoom);

    m_renderer.EndFrame();
}

void Game::OnMouseButton(int button, int action, int /*mods*/) {
    if (action == GLFW_PRESS) {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            // Check if clicking on UI sidebar
            if (m_ui.IsMouseOverSidebar(float(m_mouseX), float(m_windowW))) {
                // Check if minimap clicked
                float newCamX, newCamY;
                if (m_ui.HandleMinimapClick(float(m_mouseX), float(m_mouseY), float(m_windowW), newCamX, newCamY, m_map)) {
                    m_camX = newCamX;
                    m_camY = newCamY;
                } else {
                    m_ui.HandleMouseClick(float(m_mouseX), float(m_mouseY), button, float(m_windowW), m_sovietCredits, m_entities, m_map, m_audio);
                }
                return;
            }

            // Battlefield left click
            if (m_ui.IsInPlacementMode()) {
                glm::vec2 mouseW = m_renderer.ScreenToWorld(float(m_mouseX), float(m_mouseY));
                glm::ivec2 tile = Map::WorldToTile(mouseW.x, mouseW.y);
                int wTiles = 2, hTiles = 2;
                StructureType pType = m_ui.GetPlacingType();
                if (pType == StructureType::SOVIET_CONYARD || pType == StructureType::SOVIET_REFINERY || pType == StructureType::SOVIET_WARFACTORY) {
                    wTiles = 3; hTiles = 3;
                }
                if (m_map.CanPlaceBuilding(tile.x, tile.y, wTiles, hTiles)) {
                    m_ui.ConfirmPlacement(tile.x, tile.y, m_entities, m_map, m_audio);
                }
                return;
            }

            // Start box selection
            m_isBoxSelecting = true;
            m_boxStartScreen = glm::vec2(float(m_mouseX), float(m_mouseY));
            m_boxEndScreen = m_boxStartScreen;

        } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
            if (m_ui.IsInPlacementMode()) {
                m_ui.CancelPlacement(m_sovietCredits, m_audio);
                return;
            }

            // Right click issues Move / Attack order
            glm::vec2 mouseW = m_renderer.ScreenToWorld(float(m_mouseX), float(m_mouseY));
            IssueOrder(mouseW, false);

        } else if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
            m_mouseMiddleDown = true;
            m_lastDragMouseX = m_mouseX;
            m_lastDragMouseY = m_mouseY;
        }
    } else if (action == GLFW_RELEASE) {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (m_isBoxSelecting) {
                m_isBoxSelecting = false;
                HandleBoxSelection();
            }
        } else if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
            m_mouseMiddleDown = false;
        }
    }
}

void Game::HandleBoxSelection() {
    float dragDist = glm::length(m_boxEndScreen - m_boxStartScreen);

    if (dragDist < 6.0f) {
        // Single left-click
        glm::vec2 clickWorld = m_renderer.ScreenToWorld(m_boxStartScreen.x, m_boxStartScreen.y);
        glm::ivec2 clickTile = Map::WorldToTile(clickWorld.x, clickWorld.y);

        // Check if any Soviet combat unit or harvester is already selected
        int selectedSovietUnits = 0;
        for (const auto& u : m_entities.GetUnits()) {
            if (u.selected && u.faction == Faction::SOVIET && u.health > 0.0f) {
                selectedSovietUnits++;
            }
        }

        // 1. Check if clicking on an enemy Allied unit
        int enemyUnitId = -1;
        for (const auto& u : m_entities.GetUnits()) {
            if (u.faction == Faction::ALLIED && u.health > 0.0f) {
                if (glm::length(u.pos - clickWorld) < 32.0f) {
                    enemyUnitId = u.id;
                    break;
                }
            }
        }

        // 2. Check if clicking on an enemy Allied structure
        int enemyStructId = -1;
        for (const auto& s : m_entities.GetStructures()) {
            if (s.faction == Faction::ALLIED && s.health > 0.0f) {
                if (clickTile.x >= s.tileX && clickTile.x < s.tileX + s.wTiles &&
                    clickTile.y >= s.tileY && clickTile.y < s.tileY + s.hTiles) {
                    enemyStructId = s.id;
                    break;
                }
                glm::vec2 c = s.GetCenterWorld();
                float sw = (s.wTiles >= 3) ? 192.0f : 128.0f;
                float sh = (s.wTiles >= 3) ? 150.0f : 110.0f;
                float baseCenterY = (s.wTiles >= 3) ? 100.0f : 78.0f;
                float drawX = c.x - sw * 0.5f;
                float drawY = c.y - baseCenterY;
                if (clickWorld.x >= drawX && clickWorld.x <= drawX + sw &&
                    clickWorld.y >= drawY && clickWorld.y <= drawY + sh) {
                    enemyStructId = s.id;
                    break;
                }
            }
        }

        // If clicking on an enemy entity
        if (enemyUnitId != -1 || enemyStructId != -1) {
            if (selectedSovietUnits > 0) {
                // ATTACK ORDER!
                IssueOrder(clickWorld, true);
                return;
            } else {
                // Inspect enemy unit or structure
                for (auto& u : m_entities.GetUnits()) u.selected = false;
                for (auto& s : m_entities.GetStructures()) s.selected = false;
                if (enemyUnitId != -1) {
                    Unit* tgt = m_entities.FindUnitById(enemyUnitId);
                    if (tgt) tgt->selected = true;
                } else {
                    Structure* tgtS = m_entities.FindStructureById(enemyStructId);
                    if (tgtS) tgtS->selected = true;
                }
                return;
            }
        }

        // 3. Check if clicking on a friendly Soviet unit
        Unit* friendlyUnit = nullptr;
        float bestDist = 32.0f;
        for (auto& u : m_entities.GetUnits()) {
            if (u.faction == Faction::SOVIET && u.health > 0.0f) {
                float d = glm::length(u.pos - clickWorld);
                if (d < bestDist) {
                    bestDist = d;
                    friendlyUnit = &u;
                }
            }
        }

        if (friendlyUnit) {
            for (auto& u : m_entities.GetUnits()) u.selected = false;
            for (auto& s : m_entities.GetStructures()) s.selected = false;
            friendlyUnit->selected = true;
            m_audio.Play(SoundId::SELECT, 0.9f);
            return;
        }

        // 4. Check if clicking on a friendly Soviet structure
        Structure* friendlyStruct = nullptr;
        for (auto& s : m_entities.GetStructures()) {
            if (s.faction == Faction::SOVIET && s.health > 0.0f) {
                bool hit = false;
                if (clickTile.x >= s.tileX && clickTile.x < s.tileX + s.wTiles &&
                    clickTile.y >= s.tileY && clickTile.y < s.tileY + s.hTiles) {
                    hit = true;
                }
                glm::vec2 c = s.GetCenterWorld();
                float sw = (s.wTiles >= 3) ? 192.0f : 128.0f;
                float sh = (s.wTiles >= 3) ? 150.0f : 110.0f;
                float baseCenterY = (s.wTiles >= 3) ? 100.0f : 78.0f;
                float drawX = c.x - sw * 0.5f;
                float drawY = c.y - baseCenterY;
                if (clickWorld.x >= drawX && clickWorld.x <= drawX + sw &&
                    clickWorld.y >= drawY && clickWorld.y <= drawY + sh) {
                    hit = true;
                }
                if (hit) {
                    friendlyStruct = &s;
                    break;
                }
            }
        }

        if (friendlyStruct) {
            for (auto& u : m_entities.GetUnits()) u.selected = false;
            for (auto& s : m_entities.GetStructures()) s.selected = false;
            friendlyStruct->selected = true;
            m_audio.Play(SoundId::SELECT, 0.8f);
            return;
        }

        // 5. Pressed on a ground location / ore
        if (selectedSovietUnits > 0) {
            // Selected units MOVE there!
            IssueOrder(clickWorld, false);
            return;
        } else {
            // Empty ground clicked with nothing selected: clear any selected structure
            for (auto& s : m_entities.GetStructures()) s.selected = false;
        }

    } else {
        // Box drag selection: screen-space test
        float minX = std::min(m_boxStartScreen.x, m_boxEndScreen.x);
        float maxX = std::max(m_boxStartScreen.x, m_boxEndScreen.x);
        float minY = std::min(m_boxStartScreen.y, m_boxEndScreen.y);
        float maxY = std::max(m_boxStartScreen.y, m_boxEndScreen.y);

        // Deselect all
        for (auto& u : m_entities.GetUnits()) u.selected = false;
        for (auto& s : m_entities.GetStructures()) s.selected = false;

        int selectedCount = 0;
        for (auto& u : m_entities.GetUnits()) {
            if (u.faction == Faction::SOVIET && u.health > 0.0f) {
                glm::vec2 sPos = m_renderer.WorldToScreen(u.pos.x, u.pos.y);
                if (sPos.x >= minX && sPos.x <= maxX && sPos.y >= minY && sPos.y <= maxY) {
                    u.selected = true;
                    selectedCount++;
                }
            }
        }

        if (selectedCount > 0) {
            m_audio.Play(SoundId::SELECT, 0.9f);
        }
    }
}

void Game::IssueOrder(glm::vec2 targetWorld, bool isAttackExplicit) {
    // Check if target is an enemy unit or structure
    int targetEnemyUnit = -1;
    int targetEnemyStruct = -1;

    for (const auto& u : m_entities.GetUnits()) {
        if (u.faction == Faction::ALLIED && u.health > 0.0f) {
            if (glm::length(u.pos - targetWorld) < 32.0f) {
                targetEnemyUnit = u.id;
                break;
            }
        }
    }

    if (targetEnemyUnit == -1) {
        glm::ivec2 targetTile = Map::WorldToTile(targetWorld.x, targetWorld.y);
        for (const auto& s : m_entities.GetStructures()) {
            if (s.faction == Faction::ALLIED && s.health > 0.0f) {
                if (targetTile.x >= s.tileX && targetTile.x < s.tileX + s.wTiles &&
                    targetTile.y >= s.tileY && targetTile.y < s.tileY + s.hTiles) {
                    targetEnemyStruct = s.id;
                    break;
                }
                glm::vec2 c = s.GetCenterWorld();
                float sw = (s.wTiles >= 3) ? 140.0f : 90.0f;
                float sh = (s.wTiles >= 3) ? 110.0f : 80.0f;
                float drawX = c.x - sw * 0.5f;
                float drawY = c.y - sh + 20.0f;
                if (targetWorld.x >= drawX && targetWorld.x <= drawX + sw &&
                    targetWorld.y >= drawY && targetWorld.y <= drawY + sh) {
                    targetEnemyStruct = s.id;
                    break;
                }
            }
        }
    }

    // Collect selected Soviet units
    std::vector<Unit*> selectedUnits;
    for (auto& u : m_entities.GetUnits()) {
        if (u.selected && u.faction == Faction::SOVIET && u.health > 0.0f) {
            selectedUnits.push_back(&u);
        }
    }

    if (selectedUnits.empty()) return;

    bool isAttack = (targetEnemyUnit != -1 || targetEnemyStruct != -1 || isAttackExplicit);

    // Set up visual order feedback marker
    m_orderMarkerPos = targetWorld;
    m_orderMarkerTimer = 0.5f;
    m_orderMarkerIsAttack = isAttack;

    int count = int(selectedUnits.size());
    int cols = int(std::ceil(std::sqrt(float(count))));

    for (int i = 0; i < count; ++i) {
        Unit* u = selectedUnits[i];

        if (isAttack) {
            // Attack order!
            u->targetUnitId = targetEnemyUnit;
            u->targetStructureId = targetEnemyStruct;
            u->state = UnitState::ATTACKING;
            u->waypoints = m_map.FindPath(u->pos, targetWorld);
        } else {
            // Move or harvest order
            u->targetUnitId = -1;
            u->targetStructureId = -1;

            if (u->type == UnitType::HARVESTER) {
                glm::ivec2 tile = Map::WorldToTile(targetWorld.x, targetWorld.y);
                if (m_map.IsInBounds(tile.x, tile.y) && m_map.GetShroudAt(tile.x, tile.y) != ShroudStatus::SHROUDED) {
                    u->targetOreTile = tile;
                    u->state = UnitState::MOVING;
                    u->waypoints = m_map.FindPath(u->pos, targetWorld);
                    continue;
                }
            }

            // Formation offset
            glm::vec2 unitTarget = targetWorld;
            if (count > 1) {
                int r = i / cols;
                int c = i % cols;
                float offX = (float(c) - float(cols - 1) * 0.5f) * 32.0f;
                float offY = (float(r) - float(cols - 1) * 0.5f) * 20.0f;
                unitTarget += glm::vec2(offX, offY);
            }

            u->state = UnitState::MOVING;
            u->waypoints = m_map.FindPath(u->pos, unitTarget);
        }
    }

    if (isAttack) {
        m_audio.Play(SoundId::ATTACK_ORDER, 0.95f);
        m_entities.SpawnSpark(targetWorld, glm::vec2(0.0f, -10.0f), glm::vec4(1.0f, 0.2f, 0.2f, 1.0f));
    } else {
        m_audio.Play(SoundId::MOVE_ORDER, 0.9f);
        m_entities.SpawnSpark(targetWorld, glm::vec2(0.0f, -10.0f), glm::vec4(0.2f, 1.0f, 0.3f, 1.0f));
    }
}

void Game::OnCursorPos(double xpos, double ypos) {
    m_mouseX = xpos;
    m_mouseY = ypos;

    if (m_mouseMiddleDown) {
        float dx = float(xpos - m_lastDragMouseX);
        float dy = float(ypos - m_lastDragMouseY);
        m_camX -= dx * (1.0f / m_zoom);
        m_camY -= dy * (1.0f / m_zoom);
        m_lastDragMouseX = xpos;
        m_lastDragMouseY = ypos;
    }

    if (m_isBoxSelecting) {
        m_boxEndScreen = glm::vec2(float(xpos), float(ypos));
    }
}

void Game::OnScroll(double /*xoffset*/, double yoffset) {
    if (yoffset > 0.0) {
        m_zoom = std::min(1.8f, m_zoom * 1.15f);
    } else if (yoffset < 0.0) {
        m_zoom = std::max(0.6f, m_zoom / 1.15f);
    }
}

void Game::OnKey(int key, int /*scancode*/, int action, int mods) {
    if (action != GLFW_PRESS) return;

    if (key == GLFW_KEY_SPACE || key == GLFW_KEY_H) {
        // Focus on Soviet ConYard centered in main window
        for (const auto& s : m_entities.GetStructures()) {
            if (s.faction == Faction::SOVIET && s.type == StructureType::SOVIET_CONYARD) {
                glm::vec2 c = s.GetCenterWorld();
                m_camX = c.x + (UI::SIDEBAR_WIDTH * 0.5f) / m_zoom;
                m_camY = c.y;
                m_audio.Play(SoundId::RADAR_PING, 0.8f);
                break;
            }
        }
    } else if (key == GLFW_KEY_P) {
        m_paused = !m_paused;
        m_ui.AddNotification(m_paused ? "GAME PAUSED" : "GAME RESUMED");
    } else if (key >= GLFW_KEY_1 && key <= GLFW_KEY_9) {
        int group = key - GLFW_KEY_1;
        if (mods & GLFW_MOD_CONTROL) {
            // Assign control group
            m_controlGroups[group].clear();
            for (const auto& u : m_entities.GetUnits()) {
                if (u.selected && u.faction == Faction::SOVIET) {
                    m_controlGroups[group].push_back(u.id);
                }
            }
            m_audio.Play(SoundId::CLICK, 0.9f);
            m_ui.AddNotification("ASSIGNED CONTROL GROUP " + std::to_string(group + 1));
        } else {
            // Select control group
            for (auto& u : m_entities.GetUnits()) u.selected = false;
            int count = 0;
            for (int id : m_controlGroups[group]) {
                Unit* u = m_entities.FindUnitById(id);
                if (u && u->health > 0.0f) {
                    u->selected = true;
                    count++;
                }
            }
            if (count > 0) {
                m_audio.Play(SoundId::SELECT, 0.9f);
            }
        }
    } else if (key == GLFW_KEY_ESCAPE) {
        if (m_ui.IsInPlacementMode()) {
            m_ui.CancelPlacement(m_sovietCredits, m_audio);
        } else {
            for (auto& u : m_entities.GetUnits()) u.selected = false;
            for (auto& s : m_entities.GetStructures()) s.selected = false;
        }
    }
}

void Game::OnResize(int winW, int winH, int fbW, int fbH) {
    m_windowW = winW;
    m_windowH = winH;
    m_renderer.Resize(winW, winH, fbW, fbH);
    m_renderer.SetCamera(m_camX, m_camY, m_zoom);
}
