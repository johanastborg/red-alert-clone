#include "Renderer.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <cmath>

namespace {

const char* SPRITE_VS = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aUV;
layout (location = 2) in vec4 aColor;

uniform mat4 uMVP;

out vec2 vUV;
out vec4 vColor;

void main() {
    gl_Position = uMVP * vec4(aPos, 0.0, 1.0);
    vUV = aUV;
    vColor = aColor;
}
)";

const char* SPRITE_FS = R"(
#version 330 core
in vec2 vUV;
in vec4 vColor;

uniform sampler2D uTexture;

out vec4 FragColor;

void main() {
    vec4 texColor = texture(uTexture, vUV);
    FragColor = texColor * vColor;
}
)";

const char* LINE_VS = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec4 aColor;

uniform mat4 uMVP;

out vec4 vColor;

void main() {
    gl_Position = uMVP * vec4(aPos, 0.0, 1.0);
    vColor = aColor;
}
)";

const char* LINE_FS = R"(
#version 330 core
in vec4 vColor;

out vec4 FragColor;

void main() {
    FragColor = vColor;
}
)";

GLuint CompileShader(GLenum type, const char* source) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &source, nullptr);
    glCompileShader(s);

    GLint success;
    glGetShaderiv(s, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info[512];
        glGetShaderInfoLog(s, 512, nullptr, info);
        std::cerr << "Shader compilation error: " << info << std::endl;
    }
    return s;
}

GLuint CreateProgram(const char* vs, const char* fs) {
    GLuint v = CompileShader(GL_VERTEX_SHADER, vs);
    GLuint f = CompileShader(GL_FRAGMENT_SHADER, fs);
    GLuint p = glCreateProgram();
    glAttachShader(p, v);
    glAttachShader(p, f);
    glLinkProgram(p);

    GLint success;
    glGetProgramiv(p, GL_LINK_STATUS, &success);
    if (!success) {
        char info[512];
        glGetProgramInfoLog(p, 512, nullptr, info);
        std::cerr << "Program linking error: " << info << std::endl;
    }
    glDeleteShader(v);
    glDeleteShader(f);
    return p;
}

} // namespace

Renderer::Renderer() = default;

Renderer::~Renderer() {
    if (m_spriteVAO) glDeleteVertexArrays(1, &m_spriteVAO);
    if (m_spriteVBO) glDeleteBuffers(1, &m_spriteVBO);
    if (m_spriteEBO) glDeleteBuffers(1, &m_spriteEBO);
    if (m_spriteShader) glDeleteProgram(m_spriteShader);

    if (m_lineVAO) glDeleteVertexArrays(1, &m_lineVAO);
    if (m_lineVBO) glDeleteBuffers(1, &m_lineVBO);
    if (m_lineShader) glDeleteProgram(m_lineShader);
}

bool Renderer::Initialize(int winW, int winH, int fbW, int fbH) {
    m_screenW = winW;
    m_screenH = winH;
    m_fbW = fbW;
    m_fbH = fbH;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    if (!m_atlas.BuildAtlas()) {
        return false;
    }

    InitShaders();
    InitBuffers();
    Resize(winW, winH, fbW, fbH);

    return true;
}

void Renderer::InitShaders() {
    m_spriteShader = CreateProgram(SPRITE_VS, SPRITE_FS);
    m_uSpriteMVP = glGetUniformLocation(m_spriteShader, "uMVP");
    m_uSpriteTexture = glGetUniformLocation(m_spriteShader, "uTexture");

    m_lineShader = CreateProgram(LINE_VS, LINE_FS);
    m_uLineMVP = glGetUniformLocation(m_lineShader, "uMVP");
}

void Renderer::InitBuffers() {
    // Sprite batch setup
    m_quadVertices.reserve(MAX_VERTICES);

    std::vector<GLuint> indices(MAX_INDICES);
    for (size_t i = 0, offset = 0; i < MAX_INDICES; i += 6, offset += 4) {
        indices[i + 0] = GLuint(offset + 0);
        indices[i + 1] = GLuint(offset + 1);
        indices[i + 2] = GLuint(offset + 2);
        indices[i + 3] = GLuint(offset + 2);
        indices[i + 4] = GLuint(offset + 3);
        indices[i + 5] = GLuint(offset + 0);
    }

    glGenVertexArrays(1, &m_spriteVAO);
    glGenBuffers(1, &m_spriteVBO);
    glGenBuffers(1, &m_spriteEBO);

    glBindVertexArray(m_spriteVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_spriteVBO);
    glBufferData(GL_ARRAY_BUFFER, MAX_VERTICES * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_spriteEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, MAX_INDICES * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

    // layout (location = 0) in vec2 aPos;
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));

    // layout (location = 1) in vec2 aUV;
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));

    // layout (location = 2) in vec4 aColor;
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));

    // Line batch setup
    m_lineVertices.reserve(MAX_LINE_VERTICES);

    glGenVertexArrays(1, &m_lineVAO);
    glGenBuffers(1, &m_lineVBO);

    glBindVertexArray(m_lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_lineVBO);
    glBufferData(GL_ARRAY_BUFFER, MAX_LINE_VERTICES * sizeof(LineVertex), nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void*)offsetof(LineVertex, pos));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void*)offsetof(LineVertex, color));

    glBindVertexArray(0);
}

void Renderer::Resize(int winW, int winH, int fbW, int fbH) {
    m_screenW = winW;
    m_screenH = winH;
    m_fbW = fbW;
    m_fbH = fbH;
    glViewport(0, 0, fbW, fbH);

    // Screen projection: (0,0) top-left to (winW, winH) bottom-right
    m_screenProjMatrix = glm::ortho(0.0f, float(winW), float(winH), 0.0f, -1.0f, 1.0f);
    SetCamera(m_camX, m_camY, m_zoom);
}

void Renderer::SetCamera(float camX, float camY, float zoom) {
    m_camX = camX;
    m_camY = camY;
    m_zoom = zoom;

    float halfW = (float(m_screenW) * 0.5f) / zoom;
    float halfH = (float(m_screenH) * 0.5f) / zoom;

    m_worldProjMatrix = glm::ortho(
        camX - halfW, camX + halfW,
        camY + halfH, camY - halfH,
        -1.0f, 1.0f
    );
}

void Renderer::SetScreenMode() {
    Flush();
    FlushLines();
    m_currentMatrix = m_screenProjMatrix;
}

void Renderer::SetWorldMode() {
    Flush();
    FlushLines();
    m_currentMatrix = m_worldProjMatrix;
}

glm::vec2 Renderer::ScreenToWorld(float sx, float sy) const {
    float halfW = (float(m_screenW) * 0.5f) / m_zoom;
    float halfH = (float(m_screenH) * 0.5f) / m_zoom;
    float wx = m_camX - halfW + (sx / float(m_screenW)) * (2.0f * halfW);
    float wy = m_camY - halfH + (sy / float(m_screenH)) * (2.0f * halfH);
    return glm::vec2(wx, wy);
}

glm::vec2 Renderer::WorldToScreen(float wx, float wy) const {
    float halfW = (float(m_screenW) * 0.5f) / m_zoom;
    float halfH = (float(m_screenH) * 0.5f) / m_zoom;
    float sx = ((wx - (m_camX - halfW)) / (2.0f * halfW)) * float(m_screenW);
    float sy = ((wy - (m_camY - halfH)) / (2.0f * halfH)) * float(m_screenH);
    return glm::vec2(sx, sy);
}

void Renderer::BeginFrame() {
    glClearColor(0.08f, 0.08f, 0.09f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    m_quadVertices.clear();
    m_lineVertices.clear();
    SetWorldMode();
}

void Renderer::EndFrame() {
    Flush();
    FlushLines();
}

void Renderer::Flush() {
    if (m_quadVertices.empty()) return;

    glUseProgram(m_spriteShader);
    glUniformMatrix4fv(m_uSpriteMVP, 1, GL_FALSE, glm::value_ptr(m_currentMatrix));
    glUniform1i(m_uSpriteTexture, 0);

    m_atlas.Bind(0);

    glBindVertexArray(m_spriteVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_spriteVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, m_quadVertices.size() * sizeof(Vertex), m_quadVertices.data());

    GLsizei numQuads = GLsizei(m_quadVertices.size() / 4);
    glDrawElements(GL_TRIANGLES, numQuads * 6, GL_UNSIGNED_INT, nullptr);

    m_quadVertices.clear();
}

void Renderer::FlushLines() {
    if (m_lineVertices.empty()) return;

    glUseProgram(m_lineShader);
    glUniformMatrix4fv(m_uLineMVP, 1, GL_FALSE, glm::value_ptr(m_currentMatrix));

    glBindVertexArray(m_lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_lineVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, m_lineVertices.size() * sizeof(LineVertex), m_lineVertices.data());

    glDrawArrays(GL_LINES, 0, GLsizei(m_lineVertices.size()));

    m_lineVertices.clear();
}

void Renderer::DrawSprite(SpriteId id, float x, float y, float w, float h, float rotationRad, glm::vec4 tint) {
    if (m_quadVertices.size() + 4 >= MAX_VERTICES) {
        Flush();
    }

    const SpriteRect& sr = m_atlas.GetSprite(id);

    glm::vec2 p0(-w * 0.5f, -h * 0.5f);
    glm::vec2 p1( w * 0.5f, -h * 0.5f);
    glm::vec2 p2( w * 0.5f,  h * 0.5f);
    glm::vec2 p3(-w * 0.5f,  h * 0.5f);

    float cx = x + w * 0.5f;
    float cy = y + h * 0.5f;

    if (rotationRad != 0.0f) {
        float cosR = std::cos(rotationRad);
        float sinR = std::sin(rotationRad);
        auto Rot = [cosR, sinR](glm::vec2 p) {
            return glm::vec2(p.x * cosR - p.y * sinR, p.x * sinR + p.y * cosR);
        };
        p0 = Rot(p0);
        p1 = Rot(p1);
        p2 = Rot(p2);
        p3 = Rot(p3);
    }

    p0 += glm::vec2(cx, cy);
    p1 += glm::vec2(cx, cy);
    p2 += glm::vec2(cx, cy);
    p3 += glm::vec2(cx, cy);

    m_quadVertices.push_back({ p0, { sr.u0, sr.v0 }, tint });
    m_quadVertices.push_back({ p1, { sr.u1, sr.v0 }, tint });
    m_quadVertices.push_back({ p2, { sr.u1, sr.v1 }, tint });
    m_quadVertices.push_back({ p3, { sr.u0, sr.v1 }, tint });
}

void Renderer::DrawSpriteCentered(SpriteId id, float cx, float cy, float w, float h, float rotationRad, glm::vec4 tint) {
    DrawSprite(id, cx - w * 0.5f, cy - h * 0.5f, w, h, rotationRad, tint);
}

void Renderer::DrawQuad(float x, float y, float w, float h, glm::vec4 color) {
    DrawSprite(SpriteId::WHITE_PIXEL, x, y, w, h, 0.0f, color);
}

void Renderer::DrawQuadCentered(float cx, float cy, float w, float h, glm::vec4 color) {
    DrawSpriteCentered(SpriteId::WHITE_PIXEL, cx, cy, w, h, 0.0f, color);
}

void Renderer::DrawLine(float x0, float y0, float x1, float y1, glm::vec4 color) {
    if (m_lineVertices.size() + 2 >= MAX_LINE_VERTICES) {
        FlushLines();
    }
    m_lineVertices.push_back({ { x0, y0 }, color });
    m_lineVertices.push_back({ { x1, y1 }, color });
}

void Renderer::DrawRectOutline(float x, float y, float w, float h, glm::vec4 color) {
    DrawLine(x, y, x + w, y, color);
    DrawLine(x + w, y, x + w, y + h, color);
    DrawLine(x + w, y + h, x, y + h, color);
    DrawLine(x, y + h, x, y, color);
}

void Renderer::DrawCircleOutline(float cx, float cy, float radius, glm::vec4 color, int segments) {
    float step = (2.0f * 3.14159265f) / float(segments);
    for (int i = 0; i < segments; ++i) {
        float a0 = i * step;
        float a1 = (i + 1) * step;
        DrawLine(
            cx + std::cos(a0) * radius, cy + std::sin(a0) * radius,
            cx + std::cos(a1) * radius, cy + std::sin(a1) * radius,
            color
        );
    }
}

void Renderer::DrawDiamondOutline(float cx, float cy, float w, float h, glm::vec4 color) {
    float hw = w * 0.5f;
    float hh = h * 0.5f;
    DrawLine(cx, cy - hh, cx + hw, cy, color);
    DrawLine(cx + hw, cy, cx, cy + hh, color);
    DrawLine(cx, cy + hh, cx - hw, cy, color);
    DrawLine(cx - hw, cy, cx, cy - hh, color);
}

void Renderer::DrawDiamondFilled(float cx, float cy, float w, float h, glm::vec4 color) {
    // 4 vertices forming a diamond
    float hw = w * 0.5f;
    float hh = h * 0.5f;
    if (m_quadVertices.size() + 4 >= MAX_VERTICES) {
        Flush();
    }
    const SpriteRect& sr = m_atlas.GetSprite(SpriteId::WHITE_PIXEL);
    // p0: top, p1: right, p2: bottom, p3: left
    glm::vec2 p0(cx, cy - hh);
    glm::vec2 p1(cx + hw, cy);
    glm::vec2 p2(cx, cy + hh);
    glm::vec2 p3(cx - hw, cy);

    m_quadVertices.push_back({ p0, { sr.u0, sr.v0 }, color });
    m_quadVertices.push_back({ p1, { sr.u1, sr.v0 }, color });
    m_quadVertices.push_back({ p2, { sr.u1, sr.v1 }, color });
    m_quadVertices.push_back({ p3, { sr.u0, sr.v1 }, color });
}

void Renderer::DrawTeslaArc(float x0, float y0, float x1, float y1, float timer) {
    // Multi-segment dynamic fractal lightning arc
    constexpr int NUM_SEGMENTS = 10;
    float dx = x1 - x0;
    float dy = y1 - y0;
    float length = std::sqrt(dx * dx + dy * dy);
    if (length < 1.0f) return;

    float nx = -dy / length;
    float ny =  dx / length;

    std::vector<glm::vec2> points(NUM_SEGMENTS + 1);
    points[0] = glm::vec2(x0, y0);
    points[NUM_SEGMENTS] = glm::vec2(x1, y1);

    for (int i = 1; i < NUM_SEGMENTS; ++i) {
        float frac = float(i) / float(NUM_SEGMENTS);
        float basePx = x0 + dx * frac;
        float basePy = y0 + dy * frac;

        // Dynamic jitter pseudo-noise
        float seed = timer * 45.0f + float(i * 17);
        float jitter = (std::sin(seed * 3.7f) + std::cos(seed * 7.1f) * 0.5f) * 14.0f;
        points[i] = glm::vec2(basePx + nx * jitter, basePy + ny * jitter);
    }

    // Outer cyan electric envelope
    glm::vec4 cyanGlow(0.0f, 0.85f, 1.0f, 0.6f);
    glm::vec4 whiteCore(0.9f, 1.0f, 1.0f, 0.95f);

    for (int i = 0; i < NUM_SEGMENTS; ++i) {
        glm::vec2 pA = points[i];
        glm::vec2 pB = points[i + 1];

        // Draw multiple offset lines for thickness and glow
        DrawLine(pA.x - nx * 1.5f, pA.y - ny * 1.5f, pB.x - nx * 1.5f, pB.y - ny * 1.5f, cyanGlow);
        DrawLine(pA.x + nx * 1.5f, pA.y + ny * 1.5f, pB.x + nx * 1.5f, pB.y + ny * 1.5f, cyanGlow);
        // White core
        DrawLine(pA.x, pA.y, pB.x, pB.y, whiteCore);

        // Branching fork sparks (on 3rd and 7th segments)
        if (i == 3 || i == 6) {
            float forkLen = 16.0f;
            float forkAngle = (i == 3 ? 0.7f : -0.7f);
            float fx = (dx * std::cos(forkAngle) - dy * std::sin(forkAngle)) / length * forkLen;
            float fy = (dx * std::sin(forkAngle) + dy * std::cos(forkAngle)) / length * forkLen;
            DrawLine(pB.x, pB.y, pB.x + fx, pB.y + fy, cyanGlow);
            DrawLine(pB.x, pB.y, pB.x + fx * 0.8f, pB.y + fy * 0.8f, whiteCore);
        }
    }
}

void Renderer::DrawText(const std::string& text, float x, float y, float scale, glm::vec4 color, bool shadow) {
    float curX = x;
    float charW = 8.0f * scale;
    float charH = 12.0f * scale;

    if (shadow) {
        glm::vec4 shadowColor(0.0f, 0.0f, 0.0f, color.a * 0.8f);
        float sx = x + 1.5f * scale;
        float sy = y + 1.5f * scale;
        for (char c : text) {
            if (c == '\n') continue;
            const GlyphRect& g = m_atlas.GetGlyph(c);
            if (m_quadVertices.size() + 4 >= MAX_VERTICES) Flush();
            m_quadVertices.push_back({ { sx, sy }, { g.u0, g.v0 }, shadowColor });
            m_quadVertices.push_back({ { sx + charW, sy }, { g.u1, g.v0 }, shadowColor });
            m_quadVertices.push_back({ { sx + charW, sy + charH }, { g.u1, g.v1 }, shadowColor });
            m_quadVertices.push_back({ { sx, sy + charH }, { g.u0, g.v1 }, shadowColor });
            sx += charW;
        }
    }

    for (char c : text) {
        if (c == '\n') continue;
        const GlyphRect& g = m_atlas.GetGlyph(c);
        if (m_quadVertices.size() + 4 >= MAX_VERTICES) Flush();
        m_quadVertices.push_back({ { curX, y }, { g.u0, g.v0 }, color });
        m_quadVertices.push_back({ { curX + charW, y }, { g.u1, g.v0 }, color });
        m_quadVertices.push_back({ { curX + charW, y + charH }, { g.u1, g.v1 }, color });
        m_quadVertices.push_back({ { curX, y + charH }, { g.u0, g.v1 }, color });
        curX += charW;
    }
}

void Renderer::DrawTextCentered(const std::string& text, float cx, float y, float scale, glm::vec4 color, bool shadow) {
    float width = GetTextWidth(text, scale);
    DrawText(text, cx - width * 0.5f, y, scale, color, shadow);
}

float Renderer::GetTextWidth(const std::string& text, float scale) const {
    return float(text.length()) * 8.0f * scale;
}
