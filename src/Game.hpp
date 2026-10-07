#pragma once

#include "AudioSystem.hpp"
#include "Renderer.hpp"
#include "Map.hpp"
#include "Entities.hpp"
#include "Combat.hpp"
#include "AI.hpp"
#include "UI.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

class Game {
public:
    Game();
    ~Game();

    bool Initialize(GLFWwindow* window, int winW, int winH, int fbW, int fbH);
    void Update(float dt);
    void Render();

    // Input handlers
    void OnKey(int key, int scancode, int action, int mods);
    void OnMouseButton(int button, int action, int mods);
    void OnCursorPos(double xpos, double ypos);
    void OnScroll(double xoffset, double yoffset);
    void OnResize(int winW, int winH, int fbW, int fbH);

private:
    void SetupInitialBattlefield();
    void HandleCameraPan(float dt);
    void HandleBoxSelection();
    void IssueOrder(glm::vec2 targetWorld, bool isAttackExplicit = false);

    // Tactical Order feedback marker
    glm::vec2 m_orderMarkerPos{0.0f};
    float m_orderMarkerTimer{0.0f};
    bool m_orderMarkerIsAttack{false};

    GLFWwindow* m_window{nullptr};
    int m_windowW{1280};
    int m_windowH{720};

    AudioSystem m_audio;
    Renderer m_renderer;
    Map m_map;
    EntityManager m_entities;
    CombatSystem m_combat;
    AlliedAI m_alliedAI;
    UI m_ui;

    // Camera
    float m_camX{800.0f};
    float m_camY{2700.0f};
    float m_zoom{1.0f};
    float m_camSpeed{600.0f};

    // Economy & Power
    int m_sovietCredits{5000};
    int m_alliedCredits{5000};
    int m_sovietPowerProd{0};
    int m_sovietPowerDrain{0};

    // Selection
    bool m_isBoxSelecting{false};
    glm::vec2 m_boxStartScreen{0.0f};
    glm::vec2 m_boxEndScreen{0.0f};

    // Mouse state
    double m_mouseX{0.0};
    double m_mouseY{0.0};
    bool m_mouseMiddleDown{false};
    double m_lastDragMouseX{0.0};
    double m_lastDragMouseY{0.0};

    // Control groups (1-9)
    std::vector<int> m_controlGroups[10];

    bool m_paused{false};
};
