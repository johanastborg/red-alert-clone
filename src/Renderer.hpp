#pragma once

#include "TextureAtlas.hpp"
#include <glm/glm.hpp>
#include <string>
#include <vector>

struct Vertex {
    glm::vec2 pos;
    glm::vec2 uv;
    glm::vec4 color;
};

struct LineVertex {
    glm::vec2 pos;
    glm::vec4 color;
};

class Renderer {
public:
    Renderer();
    ~Renderer();

    bool Initialize(int winW, int winH, int fbW, int fbH);
    void Resize(int winW, int winH, int fbW, int fbH);

    void BeginFrame();
    void EndFrame();

    void SetCamera(float camX, float camY, float zoom);
    void SetScreenMode(); // Switches projection to pixel coordinates (0,0 is top-left)
    void SetWorldMode();  // Switches projection to world coordinates using camera

    glm::vec2 ScreenToWorld(float sx, float sy) const;
    glm::vec2 WorldToScreen(float wx, float wy) const;

    // Sprite drawing
    void DrawSprite(SpriteId id, float x, float y, float w, float h, float rotationRad = 0.0f, glm::vec4 tint = glm::vec4(1.0f));
    void DrawSpriteCentered(SpriteId id, float cx, float cy, float w, float h, float rotationRad = 0.0f, glm::vec4 tint = glm::vec4(1.0f));
    void DrawQuad(float x, float y, float w, float h, glm::vec4 color);
    void DrawQuadCentered(float cx, float cy, float w, float h, glm::vec4 color);

    // Line and shape drawing
    void DrawLine(float x0, float y0, float x1, float y1, glm::vec4 color);
    void DrawRectOutline(float x, float y, float w, float h, glm::vec4 color);
    void DrawCircleOutline(float cx, float cy, float radius, glm::vec4 color, int segments = 24);
    void DrawDiamondOutline(float cx, float cy, float w, float h, glm::vec4 color);
    void DrawDiamondFilled(float cx, float cy, float w, float h, glm::vec4 color);

    // Tesla electric lightning arc
    void DrawTeslaArc(float x0, float y0, float x1, float y1, float timer);

    // Text rendering
    void DrawText(const std::string& text, float x, float y, float scale = 1.0f, glm::vec4 color = glm::vec4(1.0f), bool shadow = true);
    void DrawTextCentered(const std::string& text, float cx, float y, float scale = 1.0f, glm::vec4 color = glm::vec4(1.0f), bool shadow = true);
    float GetTextWidth(const std::string& text, float scale = 1.0f) const;

    void Flush();
    void FlushLines();

    bool SaveScreenshot(const std::string& filepath);

    TextureAtlas& GetAtlas() { return m_atlas; }
    const TextureAtlas& GetAtlas() const { return m_atlas; }

    int GetScreenWidth() const { return m_screenW; }
    int GetScreenHeight() const { return m_screenH; }
    float GetCamX() const { return m_camX; }
    float GetCamY() const { return m_camY; }
    float GetZoom() const { return m_zoom; }

private:
    void InitShaders();
    void InitBuffers();

    int m_screenW{1280};
    int m_screenH{720};
    int m_fbW{1280};
    int m_fbH{720};
    float m_camX{0.0f};
    float m_camY{0.0f};
    float m_zoom{1.0f};

    glm::mat4 m_worldProjMatrix;
    glm::mat4 m_screenProjMatrix;
    glm::mat4 m_currentMatrix;

    TextureAtlas m_atlas;

    // Sprite batch
    GLuint m_spriteShader{0};
    GLuint m_spriteVAO{0};
    GLuint m_spriteVBO{0};
    GLuint m_spriteEBO{0};
    GLint m_uSpriteMVP{-1};
    GLint m_uSpriteTexture{-1};

    static constexpr size_t MAX_QUADS = 4096;
    static constexpr size_t MAX_VERTICES = MAX_QUADS * 4;
    static constexpr size_t MAX_INDICES = MAX_QUADS * 6;
    std::vector<Vertex> m_quadVertices;

    // Line batch
    GLuint m_lineShader{0};
    GLuint m_lineVAO{0};
    GLuint m_lineVBO{0};
    GLint m_uLineMVP{-1};

    static constexpr size_t MAX_LINE_VERTICES = 8192;
    std::vector<LineVertex> m_lineVertices;
};
