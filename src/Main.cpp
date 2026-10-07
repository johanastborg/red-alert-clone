#define GL_SILENCE_DEPRECATION
#define GLFW_INCLUDE_NONE
#include <OpenGL/gl3.h>
#include <GLFW/glfw3.h>
#include "Game.hpp"
#include <iostream>
#include <algorithm>

namespace {

void FramebufferSizeCallback(GLFWwindow* window, int fbWidth, int fbHeight) {
    auto* game = static_cast<Game*>(glfwGetWindowUserPointer(window));
    if (game && fbWidth > 0 && fbHeight > 0) {
        int winWidth, winHeight;
        glfwGetWindowSize(window, &winWidth, &winHeight);
        game->OnResize(winWidth, winHeight, fbWidth, fbHeight);
    }
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    auto* game = static_cast<Game*>(glfwGetWindowUserPointer(window));
    if (game) {
        game->OnKey(key, scancode, action, mods);
    }
}

void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    auto* game = static_cast<Game*>(glfwGetWindowUserPointer(window));
    if (game) {
        game->OnMouseButton(button, action, mods);
    }
}

void CursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    auto* game = static_cast<Game*>(glfwGetWindowUserPointer(window));
    if (game) {
        game->OnCursorPos(xpos, ypos);
    }
}

void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    auto* game = static_cast<Game*>(glfwGetWindowUserPointer(window));
    if (game) {
        game->OnScroll(xoffset, yoffset);
    }
}

} // namespace

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    const int initialWidth = 1280;
    const int initialHeight = 720;

    GLFWwindow* window = glfwCreateWindow(
        initialWidth, initialHeight,
        "Command & Conquer: Red Alert - Soviet Supremacy",
        nullptr, nullptr
    );

    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable V-Sync

    Game game;
    glfwSetWindowUserPointer(window, &game);

    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    glfwSetKeyCallback(window, KeyCallback);
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    glfwSetCursorPosCallback(window, CursorPosCallback);
    glfwSetScrollCallback(window, ScrollCallback);

    int winWidth, winHeight;
    glfwGetWindowSize(window, &winWidth, &winHeight);
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);

    if (!game.Initialize(window, winWidth, winHeight, fbWidth, fbHeight)) {
        std::cerr << "Failed to initialize Red Alert game engine." << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    double lastTime = glfwGetTime();

    std::cout << "========================================================\n"
              << " Command & Conquer: Red Alert - Soviet Supremacy Active\n"
              << " Faction: USSR / Soviet Red Army\n"
              << " Controls:\n"
              << "   - Left Click / Box Drag: Select Soviet units & buildings\n"
              << "   - Right Click: Move / Attack / Harvest ore\n"
              << "   - WASD / Arrow Keys / Middle Drag: Pan Camera\n"
              << "   - Mouse Wheel: Zoom In / Out\n"
              << "   - Space / H: Focus on Soviet Construction Yard\n"
              << "   - P: Pause Game\n"
              << "   - 1-9: Control Groups (Ctrl+1-9 to Assign)\n"
              << "========================================================"
              << std::endl;

    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        float dt = float(currentTime - lastTime);
        lastTime = currentTime;

        // Cap dt to avoid delta explosion during window drag/stalls
        dt = std::min(dt, 0.05f);

        glfwPollEvents();

        game.Update(dt);
        game.Render();

        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
