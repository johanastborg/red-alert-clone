#include "UI.hpp"
#include <iomanip>
#include <sstream>
#include <cmath>
#include <iostream>

UI::UI() = default;
UI::~UI() = default;

void UI::Initialize() {
    // Structure items
    m_structureItems = {
        { "TESLA POWER", 300, 6.0f, SpriteId::UI_ICON_POWER, StructureType::SOVIET_POWER, UnitType::CONSCRIPT, true },
        { "ORE REFINERY", 2000, 14.0f, SpriteId::UI_ICON_REFINERY, StructureType::SOVIET_REFINERY, UnitType::CONSCRIPT, true },
        { "BARRACKS", 400, 8.0f, SpriteId::UI_ICON_BARRACKS, StructureType::SOVIET_BARRACKS, UnitType::CONSCRIPT, true },
        { "WAR FACTORY", 1500, 12.0f, SpriteId::UI_ICON_WARFACTORY, StructureType::SOVIET_WARFACTORY, UnitType::CONSCRIPT, true },
        { "RADAR DOME", 800, 10.0f, SpriteId::UI_ICON_RADAR, StructureType::SOVIET_RADAR, UnitType::CONSCRIPT, true },
        { "TESLA COIL", 1200, 12.0f, SpriteId::UI_ICON_TESLA, StructureType::SOVIET_TESLA_COIL, UnitType::CONSCRIPT, true }
    };

    // Unit items
    m_unitItems = {
        { "CONSCRIPT", 100, 4.0f, SpriteId::UI_ICON_CONSCRIPT, StructureType::SOVIET_POWER, UnitType::CONSCRIPT, false },
        { "TESLA TROOPER", 300, 7.0f, SpriteId::UI_ICON_TESLATROOPER, StructureType::SOVIET_POWER, UnitType::TESLA_TROOPER, false },
        { "HEAVY TANK", 800, 10.0f, SpriteId::UI_ICON_HEAVYTANK, StructureType::SOVIET_POWER, UnitType::HEAVY_TANK, false },
        { "MAMMOTH TANK", 1500, 18.0f, SpriteId::UI_ICON_MAMMOTH, StructureType::SOVIET_POWER, UnitType::MAMMOTH_TANK, false },
        { "V2 ROCKET", 700, 11.0f, SpriteId::UI_ICON_V2, StructureType::SOVIET_POWER, UnitType::V2_ROCKET, false },
        { "HARVESTER", 1000, 12.0f, SpriteId::UI_ICON_HARVESTER, StructureType::SOVIET_POWER, UnitType::HARVESTER, false }
    };

    AddNotification("SOVIET COMMAND ACTIVE: EXPAND BASE AND DESTROY ALLIES!");
}

void UI::AddNotification(const std::string& msg) {
    m_notifications.push_back({ msg, 4.5f });
}

void UI::Update(float dt, int& /*sovietCredits*/, EntityManager& entities, Map& /*map*/, AudioSystem& audio) {
    m_radarSweepAngle += dt * 2.2f;
    if (m_radarSweepAngle > 6.28318f) m_radarSweepAngle -= 6.28318f;

    // Update notifications
    for (auto& n : m_notifications) n.timer -= dt;
    m_notifications.erase(
        std::remove_if(m_notifications.begin(), m_notifications.end(), [](const Notification& n) {
            return n.timer <= 0.0f;
        }),
        m_notifications.end()
    );

    // Update build queues
    for (size_t i = 0; i < m_structureItems.size(); ++i) {
        auto& item = m_structureItems[i];
        if (item.inProgress && !item.ready) {
            item.progress += dt / item.buildTime;
            if (item.progress >= 1.0f) {
                item.progress = 1.0f;
                item.ready = true;
                item.inProgress = false;
                audio.Play(SoundId::BUILDING_READY, 1.0f);
                AddNotification(item.name + " READY FOR DEPLOYMENT!");
            }
        }
    }

    for (size_t i = 0; i < m_unitItems.size(); ++i) {
        auto& item = m_unitItems[i];
        if (item.inProgress && !item.ready) {
            item.progress += dt / item.buildTime;
            if (item.progress >= 1.0f) {
                item.progress = 0.0f;
                item.inProgress = false;
                item.ready = false;

                // Spawn unit out of Barracks (if infantry) or War Factory (if vehicle)
                StructureType reqType = (item.unitType == UnitType::CONSCRIPT || item.unitType == UnitType::TESLA_TROOPER)
                                      ? StructureType::SOVIET_BARRACKS : StructureType::SOVIET_WARFACTORY;

                glm::vec2 spawnPos = Map::TileCenterToWorld(16, 56);
                for (const auto& s : entities.GetStructures()) {
                    if (s.faction == Faction::SOVIET && s.type == reqType && s.health > 0.0f) {
                        spawnPos = s.rallyPoint;
                        break;
                    }
                }

                entities.CreateUnit(item.unitType, Faction::SOVIET, spawnPos);
                audio.Play(SoundId::UNIT_READY, 1.0f);
                AddNotification(item.name + " DEPLOYED TO BATTLEFIELD!");
            }
        }
    }
}

bool UI::HandleMouseClick(float mouseX, float mouseY, int button, float screenW, int& sovietCredits,
                          EntityManager& entities, Map& /*map*/, AudioSystem& audio) {
    float sidebarX = screenW - SIDEBAR_WIDTH;

    if (mouseX < sidebarX) {
        return false; // Battlefield click
    }

    if (button != 0) return true; // Only handle left click

    audio.Play(SoundId::CLICK, 0.8f);

    // 2. Tabs:
    if (mouseY >= 250.0f && mouseY <= 280.0f) {
        if (mouseX >= sidebarX + 10.0f && mouseX <= sidebarX + 120.0f) {
            m_currentTab = BuildTab::STRUCTURES;
            return true;
        } else if (mouseX >= sidebarX + 130.0f && mouseX <= sidebarX + 240.0f) {
            m_currentTab = BuildTab::UNITS;
            return true;
        }
    }

    // 3. Build Buttons
    auto& items = (m_currentTab == BuildTab::STRUCTURES) ? m_structureItems : m_unitItems;
    for (size_t i = 0; i < items.size(); ++i) {
        int col = int(i % 2);
        int row = int(i / 2);
        float bx = sidebarX + 15.0f + float(col) * 115.0f;
        float by = 290.0f + float(row) * 68.0f;
        float bw = 105.0f;
        float bh = 62.0f;

        if (mouseX >= bx && mouseX <= bx + bw && mouseY >= by && mouseY <= by + bh) {
            auto& it = items[i];
            if (it.ready && it.isStructure) {
                // Enter placement mode!
                m_placingStructure = true;
                m_placingType = it.structType;
                m_placingItemIndex = int(i);
                audio.Play(SoundId::SELECT, 0.9f);
                AddNotification("SELECT TARGET LOCATION ON MAP TO DEPLOY " + it.name);
            } else if (!it.inProgress && !it.ready) {
                // Check tech prerequisites:
                // War factory requires Refinery; Tesla Coil requires Radar + Power
                bool techOk = true;
                if (it.structType == StructureType::SOVIET_WARFACTORY) {
                    bool hasRef = false;
                    for (const auto& s : entities.GetStructures()) {
                        if (s.faction == Faction::SOVIET && s.type == StructureType::SOVIET_REFINERY && s.health > 0.0f) {
                            hasRef = true; break;
                        }
                    }
                    if (!hasRef) {
                        AddNotification("REQUIRES ORE REFINERY FIRST!");
                        techOk = false;
                    }
                }

                if (techOk) {
                    if (sovietCredits >= it.cost) {
                        sovietCredits -= it.cost;
                        it.inProgress = true;
                        it.progress = 0.0f;
                        it.ready = false;
                        audio.Play(SoundId::CLICK, 1.0f);
                        AddNotification("BUILDING: " + it.name);
                    } else {
                        AddNotification("INSUFFICIENT FUNDS FOR " + it.name);
                    }
                }
            }
            return true;
        }
    }

    // 4. Action buttons: Repair & Sell
    float actY = 502.0f;
    if (mouseY >= actY && mouseY <= actY + 36.0f) {
        if (mouseX >= sidebarX + 15.0f && mouseX <= sidebarX + 120.0f) {
            m_repairMode = !m_repairMode;
            m_sellMode = false;
            AddNotification(m_repairMode ? "REPAIR MODE: CLICK DAMAGED BUILDING" : "REPAIR MODE CANCELLED");
            return true;
        } else if (mouseX >= sidebarX + 130.0f && mouseX <= sidebarX + 235.0f) {
            m_sellMode = !m_sellMode;
            m_repairMode = false;
            AddNotification(m_sellMode ? "SELL MODE: CLICK BUILDING TO DEMOLISH & REFUND" : "SELL MODE CANCELLED");
            return true;
        }
    }

    return true;
}

bool UI::HandleMinimapClick(float mouseX, float mouseY, float screenW, float& outCamX, float& outCamY, const Map& map) {
    float sidebarX = screenW - SIDEBAR_WIDTH;
    float rx = sidebarX + 15.0f;
    float ry = 15.0f;
    float rw = 220.0f;
    float rh = 170.0f;

    if (mouseX >= rx && mouseX <= rx + rw && mouseY >= ry && mouseY <= ry + rh) {
        float normX = (mouseX - rx) / rw;
        float normY = (mouseY - ry) / rh;
        int targetTx = int(normX * float(map.GetWidth()));
        int targetTy = int(normY * float(map.GetHeight()));
        glm::vec2 targetIso = Map::TileCenterToWorld(targetTx, targetTy);
        outCamX = targetIso.x;
        outCamY = targetIso.y;
        return true;
    }
    return false;
}

void UI::CancelPlacement(int& sovietCredits, AudioSystem& audio) {
    if (m_placingStructure && m_placingItemIndex >= 0 && m_placingItemIndex < int(m_structureItems.size())) {
        auto& it = m_structureItems[m_placingItemIndex];
        sovietCredits += it.cost;
        it.ready = false;
        it.inProgress = false;
        it.progress = 0.0f;
        m_placingStructure = false;
        m_placingItemIndex = -1;
        audio.Play(SoundId::CLICK, 1.0f);
        AddNotification("CONSTRUCTION CANCELLED: REFUNDED $" + std::to_string(it.cost));
    }
}

void UI::ConfirmPlacement(int tx, int ty, EntityManager& entities, Map& map, AudioSystem& audio) {
    if (!m_placingStructure || m_placingItemIndex < 0 || m_placingItemIndex >= int(m_structureItems.size())) return;

    auto& it = m_structureItems[m_placingItemIndex];
    Structure& s = entities.CreateStructure(it.structType, Faction::SOVIET, tx, ty);
    map.SetBuildingOccupation(tx, ty, s.wTiles, s.hTiles, s.id);

    // If refinery built, spawn a complimentary Soviet Ore Harvester!
    if (s.type == StructureType::SOVIET_REFINERY) {
        glm::vec2 harvPos = Map::TileCenterToWorld(tx + 1, ty + 3);
        entities.CreateUnit(UnitType::HARVESTER, Faction::SOVIET, harvPos);
        AddNotification("ORE REFINERY OPERATIONAL: HARVESTER DEPLOYED!");
    } else {
        AddNotification(it.name + " OPERATIONAL!");
    }

    audio.Play(SoundId::BUILDING_PLACE, 1.0f);

    it.ready = false;
    it.inProgress = false;
    it.progress = 0.0f;
    m_placingStructure = false;
    m_placingItemIndex = -1;
}

void UI::Render(Renderer& renderer, const EntityManager& entities, const Map& map,
                int sovietCredits, int powerProd, int powerDrain, float camX, float camY, float zoom) {
    renderer.SetScreenMode();

    float screenW = float(renderer.GetScreenWidth());
    float screenH = float(renderer.GetScreenHeight());
    float sidebarX = screenW - SIDEBAR_WIDTH;

    // Sidebar Background Panel
    renderer.DrawQuad(sidebarX, 0.0f, SIDEBAR_WIDTH, screenH, glm::vec4(0.14f, 0.15f, 0.17f, 0.96f));
    renderer.DrawLine(sidebarX, 0.0f, sidebarX, screenH, glm::vec4(0.28f, 0.30f, 0.33f, 1.0f));

    // 1. Radar Minimap
    RenderMinimap(renderer, entities, map, camX, camY, zoom, sidebarX + 15.0f, 15.0f, 220.0f, 170.0f);

    // 2. Power Meter & Credits
    RenderPowerMeter(renderer, powerProd, powerDrain, sidebarX + 15.0f, 195.0f, 20.0f, 45.0f);

    // Credits counter
    renderer.DrawSprite(SpriteId::SOVIET_CREST, sidebarX + 45.0f, 200.0f, 32.0f, 32.0f);
    std::string credStr = "$" + std::to_string(sovietCredits);
    renderer.DrawText(credStr, sidebarX + 85.0f, 208.0f, 1.6f, glm::vec4(1.0f, 0.85f, 0.15f, 1.0f), true);

    // 3. Build Tabs
    float tabY = 250.0f;
    bool isStruct = (m_currentTab == BuildTab::STRUCTURES);
    glm::vec4 activeTabColor(0.65f, 0.12f, 0.10f, 1.0f);
    glm::vec4 inactiveTabColor(0.22f, 0.24f, 0.26f, 1.0f);

    renderer.DrawQuad(sidebarX + 10.0f, tabY, 110.0f, 28.0f, isStruct ? activeTabColor : inactiveTabColor);
    renderer.DrawRectOutline(sidebarX + 10.0f, tabY, 110.0f, 28.0f, glm::vec4(0.45f, 0.48f, 0.52f, 1.0f));
    renderer.DrawTextCentered("STRUCTURES", sidebarX + 65.0f, tabY + 8.0f, 1.0f, glm::vec4(1.0f));

    renderer.DrawQuad(sidebarX + 130.0f, tabY, 110.0f, 28.0f, !isStruct ? activeTabColor : inactiveTabColor);
    renderer.DrawRectOutline(sidebarX + 130.0f, tabY, 110.0f, 28.0f, glm::vec4(0.45f, 0.48f, 0.52f, 1.0f));
    renderer.DrawTextCentered("UNITS", sidebarX + 185.0f, tabY + 8.0f, 1.0f, glm::vec4(1.0f));

    // 4. Build Buttons Grid
    RenderBuildButtons(renderer, sidebarX + 15.0f, 288.0f);

    // 5. Action Buttons (Repair / Sell)
    float actY = 500.0f;
    renderer.DrawQuad(sidebarX + 15.0f, actY, 105.0f, 34.0f, m_repairMode ? glm::vec4(0.2f, 0.6f, 0.2f, 1.0f) : glm::vec4(0.25f, 0.27f, 0.30f, 1.0f));
    renderer.DrawRectOutline(sidebarX + 15.0f, actY, 105.0f, 34.0f, glm::vec4(0.5f, 0.52f, 0.55f, 1.0f));
    renderer.DrawSprite(SpriteId::UI_ICON_REPAIR, sidebarX + 20.0f, actY + 5.0f, 24.0f, 24.0f);
    renderer.DrawText("REPAIR", sidebarX + 50.0f, actY + 12.0f, 1.0f, glm::vec4(1.0f));

    renderer.DrawQuad(sidebarX + 130.0f, actY, 105.0f, 34.0f, m_sellMode ? glm::vec4(0.7f, 0.2f, 0.2f, 1.0f) : glm::vec4(0.25f, 0.27f, 0.30f, 1.0f));
    renderer.DrawRectOutline(sidebarX + 130.0f, actY, 105.0f, 34.0f, glm::vec4(0.5f, 0.52f, 0.55f, 1.0f));
    renderer.DrawSprite(SpriteId::UI_ICON_SELL, sidebarX + 135.0f, actY + 5.0f, 24.0f, 24.0f);
    renderer.DrawText("SELL", sidebarX + 172.0f, actY + 12.0f, 1.0f, glm::vec4(1.0f));

    // 6. Selected Info Panel at bottom
    RenderSelectedInfo(renderer, entities, sidebarX + 10.0f, 545.0f, 230.0f, 160.0f);

    // 7. Tactical Notifications Banner at top
    if (!m_notifications.empty()) {
        const auto& n = m_notifications.front();
        float bw = renderer.GetTextWidth(n.text, 1.3f) + 40.0f;
        float bx = (screenW - SIDEBAR_WIDTH - bw) * 0.5f;
        renderer.DrawQuad(bx, 12.0f, bw, 28.0f, glm::vec4(0.08f, 0.08f, 0.10f, 0.85f));
        renderer.DrawRectOutline(bx, 12.0f, bw, 28.0f, glm::vec4(0.85f, 0.2f, 0.15f, 0.9f));
        renderer.DrawTextCentered(n.text, bx + bw * 0.5f, 18.0f, 1.3f, glm::vec4(1.0f, 0.9f, 0.2f, 1.0f));
    }
}

void UI::RenderMinimap(Renderer& renderer, const EntityManager& entities, const Map& map,
                       float camX, float camY, float /*zoom*/, float rx, float ry, float rw, float rh) {
    // Bezel
    renderer.DrawQuad(rx, ry, rw, rh, glm::vec4(0.08f, 0.10f, 0.12f, 1.0f));
    renderer.DrawRectOutline(rx, ry, rw, rh, glm::vec4(0.45f, 0.48f, 0.52f, 1.0f));

    float mapW = float(map.GetWidth());
    float mapH = float(map.GetHeight());

    // Draw terrain pixels
    for (int y = 0; y < map.GetHeight(); y += 2) {
        for (int x = 0; x < map.GetWidth(); x += 2) {
            ShroudStatus s = map.GetShroudAt(x, y);
            if (s == ShroudStatus::SHROUDED) continue;

            float px = rx + (float(x) / mapW) * rw;
            float py = ry + (float(y) / mapH) * rh;

            glm::vec4 color(0.18f, 0.35f, 0.15f, 1.0f); // grass
            if (s == ShroudStatus::FOG) color *= 0.55f;

            renderer.DrawQuad(px, py, 3.0f, 3.0f, color);
        }
    }

    // Units on minimap
    for (const auto& u : entities.GetUnits()) {
        if (u.health <= 0.0f) continue;
        if (u.faction == Faction::ALLIED && !map.IsVisibleWorld(u.pos.x, u.pos.y)) continue;

        glm::ivec2 ut = Map::WorldToTile(u.pos.x, u.pos.y);
        float px = rx + (float(ut.x) / mapW) * rw;
        float py = ry + (float(ut.y) / mapH) * rh;

        glm::vec4 blipColor = (u.faction == Faction::SOVIET)
                            ? glm::vec4(1.0f, 0.15f, 0.15f, 1.0f)
                            : glm::vec4(0.2f, 0.55f, 1.0f, 1.0f);
        if (u.type == UnitType::HARVESTER) blipColor = glm::vec4(1.0f, 0.85f, 0.1f, 1.0f);

        renderer.DrawQuad(px - 1.5f, py - 1.5f, 3.0f, 3.0f, blipColor);
    }

    // Structures on minimap
    for (const auto& s : entities.GetStructures()) {
        if (s.health <= 0.0f) continue;
        glm::vec2 c = s.GetCenterWorld();
        if (s.faction == Faction::ALLIED && !map.IsVisibleWorld(c.x, c.y)) continue;

        float px = rx + (float(s.tileX + s.wTiles / 2) / mapW) * rw;
        float py = ry + (float(s.tileY + s.hTiles / 2) / mapH) * rh;
        glm::vec4 blipColor = (s.faction == Faction::SOVIET)
                            ? glm::vec4(0.9f, 0.1f, 0.1f, 1.0f)
                            : glm::vec4(0.2f, 0.45f, 0.9f, 1.0f);

        renderer.DrawQuad(px - 3.0f, py - 3.0f, 6.0f, 6.0f, blipColor);
    }

    // Camera viewport rectangle
    glm::ivec2 camTile = Map::WorldToTile(camX, camY);
    float vx0 = rx + (float(camTile.x - 7) / mapW) * rw;
    float vy0 = ry + (float(camTile.y - 7) / mapH) * rh;
    float vw = (14.0f / mapW) * rw;
    float vh = (14.0f / mapH) * rh;
    renderer.DrawRectOutline(vx0, vy0, vw, vh, glm::vec4(1.0f, 1.0f, 1.0f, 0.8f));

    // Rotating Radar Sweep Line
    float cx = rx + rw * 0.5f;
    float cy = ry + rh * 0.5f;
    float radius = std::min(rw, rh) * 0.48f;
    float sweepX = cx + std::cos(m_radarSweepAngle) * radius;
    float sweepY = cy + std::sin(m_radarSweepAngle) * radius;
    renderer.DrawLine(cx, cy, sweepX, sweepY, glm::vec4(0.0f, 0.95f, 0.5f, 0.7f));
}

void UI::RenderPowerMeter(Renderer& renderer, int powerProd, int powerDrain, float x, float y, float w, float h) {
    renderer.DrawQuad(x, y, w, h, glm::vec4(0.08f, 0.08f, 0.09f, 1.0f));
    renderer.DrawRectOutline(x, y, w, h, glm::vec4(0.4f, 0.42f, 0.45f, 1.0f));

    float maxP = std::max(100.0f, float(std::max(powerProd, powerDrain)) * 1.2f);
    float drainFrac = float(powerDrain) / maxP;
    float prodFrac = float(powerProd) / maxP;

    bool hasPower = (powerProd >= powerDrain);
    glm::vec4 barColor = hasPower ? glm::vec4(0.15f, 0.85f, 0.25f, 1.0f) : glm::vec4(0.9f, 0.15f, 0.15f, 1.0f);

    float fillH = h * std::min(1.0f, prodFrac);
    renderer.DrawQuad(x + 2.0f, y + h - fillH, w - 4.0f, fillH, barColor);

    // Indicator line for power demand
    float drainLineY = y + h - (h * std::min(1.0f, drainFrac));
    renderer.DrawLine(x - 2.0f, drainLineY, x + w + 2.0f, drainLineY, glm::vec4(1.0f, 1.0f, 0.2f, 1.0f));
}

void UI::RenderBuildButtons(Renderer& renderer, float startX, float startY) {
    const auto& items = (m_currentTab == BuildTab::STRUCTURES) ? m_structureItems : m_unitItems;

    for (size_t i = 0; i < items.size(); ++i) {
        const auto& it = items[i];
        int col = int(i % 2);
        int row = int(i / 2);
        float bx = startX + float(col) * 115.0f;
        float by = startY + float(row) * 68.0f;
        float bw = 105.0f;
        float bh = 62.0f;

        // Button background
        renderer.DrawQuad(bx, by, bw, bh, glm::vec4(0.18f, 0.20f, 0.23f, 1.0f));
        renderer.DrawRectOutline(bx, by, bw, bh, glm::vec4(0.40f, 0.42f, 0.45f, 1.0f));

        // Icon
        renderer.DrawSprite(it.icon, bx + 6.0f, by + 6.0f, 36.0f, 36.0f);

        // Name & Cost
        renderer.DrawText(it.name, bx + 46.0f, by + 8.0f, 0.8f, glm::vec4(0.95f), false);
        renderer.DrawText("$" + std::to_string(it.cost), bx + 46.0f, by + 24.0f, 0.9f, glm::vec4(1.0f, 0.85f, 0.2f, 1.0f), false);

        // Progress overlay
        if (it.inProgress) {
            float ph = bh * it.progress;
            renderer.DrawQuad(bx, by + bh - ph, bw, ph, glm::vec4(0.0f, 0.6f, 0.9f, 0.35f));
            int pct = int(it.progress * 100.0f);
            renderer.DrawTextCentered(std::to_string(pct) + "%", bx + bw * 0.5f, by + bh - 16.0f, 1.0f, glm::vec4(1.0f));
        }

        // Ready flashing indicator
        if (it.ready) {
            renderer.DrawQuad(bx, by, bw, bh, glm::vec4(0.1f, 0.8f, 0.2f, 0.4f));
            renderer.DrawRectOutline(bx, by, bw, bh, glm::vec4(0.2f, 1.0f, 0.3f, 1.0f));
            renderer.DrawTextCentered("READY", bx + bw * 0.5f, by + bh - 18.0f, 1.2f, glm::vec4(1.0f, 1.0f, 0.2f, 1.0f));
        }
    }
}

void UI::RenderSelectedInfo(Renderer& renderer, const EntityManager& entities, float x, float y, float w, float h) {
    renderer.DrawQuad(x, y, w, h, glm::vec4(0.12f, 0.13f, 0.15f, 0.95f));
    renderer.DrawRectOutline(x, y, w, h, glm::vec4(0.35f, 0.38f, 0.42f, 1.0f));

    // Find selected unit
    const Unit* selU = nullptr;
    for (const auto& u : entities.GetUnits()) {
        if (u.selected && u.health > 0.0f) {
            selU = &u;
            break;
        }
    }

    if (selU) {
        std::string name = "SOVIET UNIT";
        SpriteId icon = SpriteId::UI_ICON_CONSCRIPT;
        switch (selU->type) {
            case UnitType::CONSCRIPT: name = "CONSCRIPT INFANTRY"; icon = SpriteId::UI_ICON_CONSCRIPT; break;
            case UnitType::TESLA_TROOPER: name = "TESLA TROOPER"; icon = SpriteId::UI_ICON_TESLATROOPER; break;
            case UnitType::HEAVY_TANK: name = "SOVIET HEAVY TANK"; icon = SpriteId::UI_ICON_HEAVYTANK; break;
            case UnitType::MAMMOTH_TANK: name = "MAMMOTH SUPER TANK"; icon = SpriteId::UI_ICON_MAMMOTH; break;
            case UnitType::V2_ROCKET: name = "V2 ROCKET LAUNCHER"; icon = SpriteId::UI_ICON_V2; break;
            case UnitType::HARVESTER: name = "ORE HARVESTER"; icon = SpriteId::UI_ICON_HARVESTER; break;
            default: break;
        }

        renderer.DrawSprite(icon, x + 10.0f, y + 10.0f, 44.0f, 44.0f);
        renderer.DrawText(name, x + 60.0f, y + 12.0f, 1.0f, glm::vec4(1.0f));

        // Health bar
        float frac = selU->health / selU->maxHealth;
        glm::vec4 hpColor = frac > 0.5f ? glm::vec4(0.15f, 0.85f, 0.2f, 1.0f)
                          : frac > 0.25f ? glm::vec4(0.9f, 0.85f, 0.1f, 1.0f)
                          : glm::vec4(0.9f, 0.15f, 0.15f, 1.0f);
        renderer.DrawQuad(x + 60.0f, y + 30.0f, 150.0f, 12.0f, glm::vec4(0.05f, 0.05f, 0.05f, 1.0f));
        renderer.DrawQuad(x + 60.0f, y + 30.0f, 150.0f * frac, 12.0f, hpColor);
        renderer.DrawRectOutline(x + 60.0f, y + 30.0f, 150.0f, 12.0f, glm::vec4(0.6f, 0.6f, 0.6f, 1.0f));

        std::string hpStr = "HP: " + std::to_string(int(selU->health)) + " / " + std::to_string(int(selU->maxHealth));
        renderer.DrawText(hpStr, x + 14.0f, y + 62.0f, 0.95f, glm::vec4(0.85f));

        if (selU->type == UnitType::HARVESTER) {
            std::string cargoStr = "ORE CARGO: " + std::to_string(int(selU->oreCargo)) + " / 1000";
            renderer.DrawText(cargoStr, x + 14.0f, y + 80.0f, 0.95f, glm::vec4(1.0f, 0.85f, 0.2f, 1.0f));
        } else {
            std::string statsStr = "ARMOR: HEAVY | RANGE: " + std::to_string(int(selU->attackRange));
            renderer.DrawText(statsStr, x + 14.0f, y + 80.0f, 0.9f, glm::vec4(0.75f));
            std::string dmgStr = "WEAPON DAMAGE: " + std::to_string(int(selU->damage));
            renderer.DrawText(dmgStr, x + 14.0f, y + 96.0f, 0.9f, glm::vec4(0.75f));
        }
        return;
    }

    // Find selected structure
    const Structure* selS = nullptr;
    for (const auto& s : entities.GetStructures()) {
        if (s.selected && s.health > 0.0f) {
            selS = &s;
            break;
        }
    }

    if (selS) {
        std::string sName = "SOVIET BUILDING";
        SpriteId sIcon = SpriteId::UI_ICON_POWER;
        switch (selS->type) {
            case StructureType::SOVIET_CONYARD: sName = "CONSTRUCTION YARD"; sIcon = SpriteId::SOVIET_CREST; break;
            case StructureType::SOVIET_POWER: sName = "TESLA REACTOR"; sIcon = SpriteId::UI_ICON_POWER; break;
            case StructureType::SOVIET_REFINERY: sName = "ORE REFINERY"; sIcon = SpriteId::UI_ICON_REFINERY; break;
            case StructureType::SOVIET_BARRACKS: sName = "SOVIET BARRACKS"; sIcon = SpriteId::UI_ICON_BARRACKS; break;
            case StructureType::SOVIET_WARFACTORY: sName = "WAR FACTORY"; sIcon = SpriteId::UI_ICON_WARFACTORY; break;
            case StructureType::SOVIET_RADAR: sName = "RADAR DOME"; sIcon = SpriteId::UI_ICON_RADAR; break;
            case StructureType::SOVIET_TESLA_COIL: sName = "TESLA COIL"; sIcon = SpriteId::UI_ICON_TESLA; break;
            default: break;
        }

        renderer.DrawSprite(sIcon, x + 10.0f, y + 10.0f, 44.0f, 44.0f);
        renderer.DrawText(sName, x + 60.0f, y + 12.0f, 1.0f, glm::vec4(1.0f));

        float frac = selS->health / selS->maxHealth;
        glm::vec4 hpColor = frac > 0.5f ? glm::vec4(0.15f, 0.85f, 0.2f, 1.0f)
                          : frac > 0.25f ? glm::vec4(0.9f, 0.85f, 0.1f, 1.0f)
                          : glm::vec4(0.9f, 0.15f, 0.15f, 1.0f);
        renderer.DrawQuad(x + 60.0f, y + 30.0f, 150.0f, 12.0f, glm::vec4(0.05f, 0.05f, 0.05f, 1.0f));
        renderer.DrawQuad(x + 60.0f, y + 30.0f, 150.0f * frac, 12.0f, hpColor);
        renderer.DrawRectOutline(x + 60.0f, y + 30.0f, 150.0f, 12.0f, glm::vec4(0.6f, 0.6f, 0.6f, 1.0f));

        std::string hpStr = "HP: " + std::to_string(int(selS->health)) + " / " + std::to_string(int(selS->maxHealth));
        renderer.DrawText(hpStr, x + 14.0f, y + 62.0f, 0.95f, glm::vec4(0.85f));

        if (selS->powerProduction > 0) {
            renderer.DrawText("POWER OUTPUT: +" + std::to_string(selS->powerProduction), x + 14.0f, y + 80.0f, 0.9f, glm::vec4(0.2f, 0.9f, 0.3f, 1.0f));
        } else if (selS->powerDrain > 0) {
            renderer.DrawText("POWER DRAIN: -" + std::to_string(selS->powerDrain), x + 14.0f, y + 80.0f, 0.9f, glm::vec4(0.9f, 0.3f, 0.3f, 1.0f));
        }

        if (selS->type == StructureType::SOVIET_TESLA_COIL) {
            renderer.DrawText("DEFENSE: 140 HIGH-VOLTAGE ARC", x + 14.0f, y + 96.0f, 0.85f, glm::vec4(0.0f, 0.9f, 1.0f, 1.0f));
        }
        return;
    }

    // Default standby message
    renderer.DrawTextCentered("SELECT UNIT OR BUILDING", x + w * 0.5f, y + 40.0f, 1.0f, glm::vec4(0.55f));
    renderer.DrawTextCentered("FOR TACTICAL STATUS", x + w * 0.5f, y + 60.0f, 1.0f, glm::vec4(0.55f));
}
