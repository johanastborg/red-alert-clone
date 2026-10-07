#pragma once

#include "Entities.hpp"
#include "Map.hpp"
#include "AudioSystem.hpp"
#include "Renderer.hpp"
#include <string>
#include <vector>

enum class BuildTab {
    STRUCTURES,
    UNITS
};

struct BuildItem {
    std::string name;
    int cost;
    float buildTime;
    SpriteId icon;
    StructureType structType;
    UnitType unitType;
    bool isStructure;

    bool inProgress{false};
    float progress{0.0f}; // 0.0 to 1.0
    bool ready{false};
};

class UI {
public:
    UI();
    ~UI();

    void Initialize();
    void Update(float dt, int& sovietCredits, EntityManager& entities, Map& map, AudioSystem& audio);
    void Render(Renderer& renderer, const EntityManager& entities, const Map& map,
                int sovietCredits, int powerProd, int powerDrain, float camX, float camY, float zoom);

    // Mouse interactions
    bool HandleMouseClick(float mouseX, float mouseY, int button, float screenW, int& sovietCredits,
                          EntityManager& entities, Map& map, AudioSystem& audio);
    bool IsMouseOverSidebar(float mouseX, float screenW) const { return mouseX >= (screenW - SIDEBAR_WIDTH); }

    // Building placement state
    bool IsInPlacementMode() const { return m_placingStructure; }
    StructureType GetPlacingType() const { return m_placingType; }
    void CancelPlacement(int& sovietCredits, AudioSystem& audio);
    void ConfirmPlacement(int tx, int ty, EntityManager& entities, Map& map, AudioSystem& audio);

    // Announcements
    void AddNotification(const std::string& msg);

    // Minimap interaction
    bool HandleMinimapClick(float mouseX, float mouseY, float screenW, float& outCamX, float& outCamY, const Map& map);

    static constexpr float SIDEBAR_WIDTH = 250.0f;

private:
    BuildTab m_currentTab{BuildTab::STRUCTURES};
    std::vector<BuildItem> m_structureItems;
    std::vector<BuildItem> m_unitItems;

    bool m_placingStructure{false};
    StructureType m_placingType{StructureType::SOVIET_POWER};
    int m_placingItemIndex{-1};

    bool m_repairMode{false};
    bool m_sellMode{false};

    float m_radarSweepAngle{0.0f};

    struct Notification {
        std::string text;
        float timer;
    };
    std::vector<Notification> m_notifications;

    void RenderMinimap(Renderer& renderer, const EntityManager& entities, const Map& map,
                       float camX, float camY, float zoom, float x, float y, float w, float h);
    void RenderPowerMeter(Renderer& renderer, int powerProd, int powerDrain, float x, float y, float w, float h);
    void RenderBuildButtons(Renderer& renderer, float startX, float startY);
    void RenderSelectedInfo(Renderer& renderer, const EntityManager& entities, float x, float y, float w, float h);
};
