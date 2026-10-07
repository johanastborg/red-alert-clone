#include "TextureAtlas.hpp"
#include <cmath>
#include <cstring>
#include <iostream>
#include <algorithm>

namespace {

inline uint32_t RGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
    return (uint32_t(a) << 24) | (uint32_t(b) << 16) | (uint32_t(g) << 8) | uint32_t(r);
}

inline void PutPixel(std::vector<uint32_t>& buf, int w, int h, int x, int y, uint32_t color) {
    if (x >= 0 && x < w && y >= 0 && y < h) {
        // Alpha blend
        uint32_t a = (color >> 24) & 0xFF;
        if (a == 255) {
            buf[y * w + x] = color;
        } else if (a > 0) {
            uint32_t dst = buf[y * w + x];
            uint32_t dr = dst & 0xFF;
            uint32_t dg = (dst >> 8) & 0xFF;
            uint32_t db = (dst >> 16) & 0xFF;
            uint32_t sr = color & 0xFF;
            uint32_t sg = (color >> 8) & 0xFF;
            uint32_t sb = (color >> 16) & 0xFF;
            uint32_t invA = 255 - a;
            uint32_t r = (sr * a + dr * invA) / 255;
            uint32_t g = (sg * a + dg * invA) / 255;
            uint32_t b = (sb * a + db * invA) / 255;
            buf[y * w + x] = RGBA(r, g, b, 255);
        }
    }
}

void DrawRect(std::vector<uint32_t>& buf, int w, int h, int x, int y, int rw, int rh, uint32_t color) {
    for (int j = 0; j < rh; ++j) {
        for (int i = 0; i < rw; ++i) {
            PutPixel(buf, w, h, x + i, y + j, color);
        }
    }
}

void DrawRectOutline(std::vector<uint32_t>& buf, int w, int h, int x, int y, int rw, int rh, uint32_t color) {
    for (int i = 0; i < rw; ++i) {
        PutPixel(buf, w, h, x + i, y, color);
        PutPixel(buf, w, h, x + i, y + rh - 1, color);
    }
    for (int j = 0; j < rh; ++j) {
        PutPixel(buf, w, h, x, y + j, color);
        PutPixel(buf, w, h, x + rw - 1, y + j, color);
    }
}

void DrawCircle(std::vector<uint32_t>& buf, int w, int h, int cx, int cy, int radius, uint32_t color, bool fill = true) {
    int r2 = radius * radius;
    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            int d = dx * dx + dy * dy;
            if (fill) {
                if (d <= r2) PutPixel(buf, w, h, cx + dx, cy + dy, color);
            } else {
                if (std::abs(d - r2) <= radius * 2) PutPixel(buf, w, h, cx + dx, cy + dy, color);
            }
        }
    }
}

void DrawLine(std::vector<uint32_t>& buf, int w, int h, int x0, int y0, int x1, int y1, uint32_t color) {
    int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;
    while (true) {
        PutPixel(buf, w, h, x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void DrawStar(std::vector<uint32_t>& buf, int w, int h, int cx, int cy, int r, uint32_t color) {
    // 5-point star
    DrawCircle(buf, w, h, cx, cy, r / 2, color, true);
    for (int a = 0; a < 5; ++a) {
        float rad = a * (2.0f * 3.14159f / 5.0f) - 3.14159f / 2.0f;
        int tipX = cx + int(std::cos(rad) * r);
        int tipY = cy + int(std::sin(rad) * r);
        DrawLine(buf, w, h, cx, cy, tipX, tipY, color);
    }
}

void DrawIsoDiamond(std::vector<uint32_t>& buf, int w, int h, int x, int y, int dw, int dh, uint32_t fillColor, uint32_t edgeColor) {
    float cx = float(x) + float(dw) * 0.5f;
    float cy = float(y) + float(dh) * 0.5f;
    float hw = float(dw) * 0.5f;
    float hh = float(dh) * 0.5f;

    for (int j = 0; j < dh; ++j) {
        for (int i = 0; i < dw; ++i) {
            float dx = std::abs((float(x + i) + 0.5f) - cx) / hw;
            float dy = std::abs((float(y + j) + 0.5f) - cy) / hh;
            float d = dx + dy;
            if (d <= 1.0f) {
                if (d > 0.94f) {
                    PutPixel(buf, w, h, x + i, y + j, edgeColor);
                } else {
                    PutPixel(buf, w, h, x + i, y + j, fillColor);
                }
            }
        }
    }
}

// Draw a filled convex 4-point polygon with bounds clipping
void DrawIsoQuad(std::vector<uint32_t>& buf, int W, int H,
                 float x0, float y0, float x1, float y1,
                 float x2, float y2, float x3, float y3,
                 uint32_t color) {
    int minX = std::max(0, int(std::floor(std::min({x0, x1, x2, x3}))));
    int maxX = std::min(W - 1, int(std::ceil(std::max({x0, x1, x2, x3}))));
    int minY = std::max(0, int(std::floor(std::min({y0, y1, y2, y3}))));
    int maxY = std::min(H - 1, int(std::ceil(std::max({y0, y1, y2, y3}))));

    float px[4] = { x0, x1, x2, x3 };
    float py[4] = { y0, y1, y2, y3 };

    for (int y = minY; y <= maxY; ++y) {
        float fy = float(y) + 0.5f;
        for (int x = minX; x <= maxX; ++x) {
            float fx = float(x) + 0.5f;
            bool hasPos = false;
            bool hasNeg = false;
            for (int i = 0; i < 4; ++i) {
                int j = (i + 1) % 4;
                float edge = (fx - px[i]) * (py[j] - py[i]) - (fy - py[i]) * (px[j] - px[i]);
                if (edge > 0.001f) hasPos = true;
                else if (edge < -0.001f) hasNeg = true;
                if (hasPos && hasNeg) break;
            }
            if (!(hasPos && hasNeg)) {
                PutPixel(buf, W, H, x, y, color);
            }
        }
    }
}

// Draw a soft isometric ground shadow
void DrawIsoShadow(std::vector<uint32_t>& buf, int W, int H,
                   float cx, float cy, float baseW, float baseH, float offsetX = 8.0f, float offsetY = 4.0f) {
    float hw = baseW * 0.5f;
    float hh = baseH * 0.5f;
    float sx = cx + offsetX;
    float sy = cy + offsetY;
    int minX = std::max(0, int(std::floor(sx - hw)));
    int maxX = std::min(W - 1, int(std::ceil(sx + hw)));
    int minY = std::max(0, int(std::floor(sy - hh)));
    int maxY = std::min(H - 1, int(std::ceil(sy + hh)));

    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            float dx = std::abs((float(x) + 0.5f) - sx) / hw;
            float dy = std::abs((float(y) + 0.5f) - sy) / hh;
            if (dx + dy <= 1.0f) {
                uint32_t orig = buf[y * W + x];
                if ((orig >> 24) > 0) {
                    uint32_t r = ((orig & 0xFF) * 55) / 100;
                    uint32_t g = (((orig >> 8) & 0xFF) * 55) / 100;
                    uint32_t b = (((orig >> 16) & 0xFF) * 55) / 100;
                    buf[y * W + x] = RGBA(r, g, b, (orig >> 24) & 0xFF);
                } else {
                    buf[y * W + x] = RGBA(0, 0, 0, 75);
                }
            }
        }
    }
}

// Draw a 3D isometric extruded cube with directional shading
void DrawIsoCube(std::vector<uint32_t>& buf, int W, int H,
                 float cx, float cy, float baseW, float baseH, float wallH,
                 uint32_t colTop, uint32_t colLeft, uint32_t colRight,
                 uint32_t colOutline = 0) {
    float hw = baseW * 0.5f;
    float hh = baseH * 0.5f;

    // Ground diamond vertices
    float gTopX = cx,        gTopY = cy - hh;
    float gRightX = cx + hw, gRightY = cy;
    float gBotX = cx,        gBotY = cy + hh;
    float gLeftX = cx - hw,  gLeftY = cy;

    // Roof diamond vertices (shifted up by wallH)
    float rTopX = gTopX,     rTopY = gTopY - wallH;
    float rRightX = gRightX, rRightY = gRightY - wallH;
    float rBotX = gBotX,     rBotY = gBotY - wallH;
    float rLeftX = gLeftX,   rLeftY = gLeftY - wallH;

    // 1. Left Wall (South-West)
    DrawIsoQuad(buf, W, H, gLeftX, gLeftY, gBotX, gBotY, rBotX, rBotY, rLeftX, rLeftY, colLeft);
    // 2. Right Wall (South-East)
    DrawIsoQuad(buf, W, H, gBotX, gBotY, gRightX, gRightY, rRightX, rRightY, rBotX, rBotY, colRight);
    // 3. Top Roof Diamond
    DrawIsoQuad(buf, W, H, rLeftX, rLeftY, rBotX, rBotY, rRightX, rRightY, rTopX, rTopY, colTop);

    // 4. Outlines
    if ((colOutline >> 24) > 0) {
        DrawLine(buf, W, H, int(gLeftX), int(gLeftY), int(gBotX), int(gBotY), colOutline);
        DrawLine(buf, W, H, int(gBotX), int(gBotY), int(gRightX), int(gRightY), colOutline);
        DrawLine(buf, W, H, int(gLeftX), int(gLeftY), int(rLeftX), int(rLeftY), colOutline);
        DrawLine(buf, W, H, int(gBotX), int(gBotY), int(rBotX), int(rBotY), colOutline);
        DrawLine(buf, W, H, int(gRightX), int(gRightY), int(rRightX), int(rRightY), colOutline);
        DrawLine(buf, W, H, int(rLeftX), int(rLeftY), int(rBotX), int(rBotY), colOutline);
        DrawLine(buf, W, H, int(rBotX), int(rBotY), int(rRightX), int(rRightY), colOutline);
        DrawLine(buf, W, H, int(rRightX), int(rRightY), int(rTopX), int(rTopY), colOutline);
        DrawLine(buf, W, H, int(rTopX), int(rTopY), int(rLeftX), int(rLeftY), colOutline);
    }
}

// Draw a 3D isometric vertical cylinder
void DrawIsoCylinder(std::vector<uint32_t>& buf, int W, int H,
                     float cx, float cy, float rx, float ry, float height,
                     uint32_t colTop, uint32_t colLight, uint32_t colDark,
                     uint32_t colOutline = 0) {
    int minX = int(cx - rx);
    int maxX = int(cx + rx);

    for (int x = minX; x <= maxX; ++x) {
        if (x < 0 || x >= W) continue;
        float normX = (float(x) - cx) / rx;
        if (std::abs(normX) > 1.0f) continue;
        float halfH = ry * std::sqrt(1.0f - normX * normX);

        int yBot = int(cy + halfH);
        int yTop = int(cy + halfH - height);

        float lightFrac = 0.5f - normX * 0.45f;
        lightFrac = std::max(0.0f, std::min(1.0f, lightFrac));

        uint32_t r = uint32_t((colLight & 0xFF) * lightFrac + (colDark & 0xFF) * (1.0f - lightFrac));
        uint32_t g = uint32_t(((colLight >> 8) & 0xFF) * lightFrac + ((colDark >> 8) & 0xFF) * (1.0f - lightFrac));
        uint32_t b = uint32_t(((colLight >> 16) & 0xFF) * lightFrac + ((colDark >> 16) & 0xFF) * (1.0f - lightFrac));
        uint32_t col = RGBA(r, g, b, 255);

        for (int y = yTop; y <= yBot; ++y) {
            PutPixel(buf, W, H, x, y, col);
        }
    }

    // Top Ellipse
    for (int dy = -int(ry); dy <= int(ry); ++dy) {
        for (int dx = -int(rx); dx <= int(rx); ++dx) {
            float d = (float(dx) * dx) / (rx * rx) + (float(dy) * dy) / (ry * ry);
            if (d <= 1.0f) {
                PutPixel(buf, W, H, int(cx + dx), int(cy - height + dy), colTop);
            }
        }
    }

    if ((colOutline >> 24) > 0) {
        DrawLine(buf, W, H, minX, int(cy), minX, int(cy - height), colOutline);
        DrawLine(buf, W, H, maxX, int(cy), maxX, int(cy - height), colOutline);
        for (int x = minX; x <= maxX; ++x) {
            float normX = (float(x) - cx) / rx;
            if (std::abs(normX) <= 1.0f) {
                float halfH = ry * std::sqrt(1.0f - normX * normX);
                PutPixel(buf, W, H, x, int(cy + halfH), colOutline);
                PutPixel(buf, W, H, x, int(cy + halfH - height), colOutline);
                PutPixel(buf, W, H, x, int(cy - halfH - height), colOutline);
            }
        }
    }
}

// Draw a 3D isometric hemisphere dome
void DrawIsoDome(std::vector<uint32_t>& buf, int W, int H,
                 float cx, float cy, float rx, float ry,
                 uint32_t colHighlight, uint32_t colShadow, uint32_t colLine = 0) {
    for (int dy = -int(ry); dy <= 0; ++dy) {
        for (int dx = -int(rx); dx <= int(rx); ++dx) {
            float d2 = (float(dx) * dx) / (rx * rx) + (float(dy) * dy) / (ry * ry);
            if (d2 <= 1.0f) {
                float z = std::sqrt(1.0f - d2);
                float nx = (float(dx) / rx);
                float ny = (float(dy) / ry);
                float dot = -nx * 0.55f - ny * 0.65f + z * 0.52f;
                dot = std::max(0.0f, std::min(1.0f, dot * 0.5f + 0.5f));

                uint32_t r = uint32_t((colHighlight & 0xFF) * dot + (colShadow & 0xFF) * (1.0f - dot));
                uint32_t g = uint32_t(((colHighlight >> 8) & 0xFF) * dot + ((colShadow >> 8) & 0xFF) * (1.0f - dot));
                uint32_t b = uint32_t(((colHighlight >> 16) & 0xFF) * dot + ((colShadow >> 16) & 0xFF) * (1.0f - dot));

                int rad = int(std::sqrt(float(dx * dx + dy * dy)));
                if ((colLine >> 24) > 0 && (rad % 7 == 0 || std::abs(dx) % 8 == 0)) {
                    r = uint32_t(r * 0.7f);
                    g = uint32_t(g * 0.7f);
                    b = uint32_t(b * 0.7f);
                }

                PutPixel(buf, W, H, int(cx + dx), int(cy + dy), RGBA(r, g, b, 255));
            }
        }
    }
}

} // namespace

TextureAtlas::TextureAtlas() = default;

TextureAtlas::~TextureAtlas() {
    if (m_textureId) {
        glDeleteTextures(1, &m_textureId);
        m_textureId = 0;
    }
}

bool TextureAtlas::BuildAtlas() {
    constexpr int ATLAS_W = 2048;
    constexpr int ATLAS_H = 2048;
    std::vector<uint32_t> buffer(ATLAS_W * ATLAS_H, RGBA(0, 0, 0, 0));

    GenerateAllSprites(buffer, ATLAS_W, ATLAS_H);
    GenerateFont(buffer, ATLAS_W, ATLAS_H);

    glGenTextures(1, &m_textureId);
    glBindTexture(GL_TEXTURE_2D, m_textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, ATLAS_W, ATLAS_H, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer.data());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    std::cout << "TextureAtlas: Built " << ATLAS_W << "x" << ATLAS_H << " atlas with "
              << m_sprites.size() << " sprites." << std::endl;
    return true;
}

void TextureAtlas::Bind(int textureUnit) const {
    glActiveTexture(GL_TEXTURE0 + textureUnit);
    glBindTexture(GL_TEXTURE_2D, m_textureId);
}

const SpriteRect& TextureAtlas::GetSprite(SpriteId id) const {
    static SpriteRect fallback;
    auto it = m_sprites.find(id);
    if (it != m_sprites.end()) return it->second;
    return fallback;
}

const GlyphRect& TextureAtlas::GetGlyph(char c) const {
    uint8_t uc = static_cast<uint8_t>(c);
    if (uc < 128) return m_glyphs[uc];
    return m_glyphs['?'];
}

void TextureAtlas::GenerateAllSprites(std::vector<uint32_t>& buf, int W, int H) {
    auto AddSprite = [this, W, H](SpriteId id, int x, int y, int sw, int sh) {
        SpriteRect r;
        r.pixelW = sw;
        r.pixelH = sh;
        r.u0 = float(x) / float(W);
        r.v0 = float(y) / float(H);
        r.u1 = float(x + sw) / float(W);
        r.v1 = float(y + sh) / float(H);
        m_sprites[id] = r;
    };

    int curX = 0;
    int curY = 0;
    int sX = 0;
    int sY = 0;
    int rowH = 0;

    // Helper macro to allocate a tile with shelf row-height allocation
    #define ALLOC_SPRITE(ID, PW, PH) \
        if (curX + (PW) + 4 > W) { \
            curX = 0; \
            curY += rowH + 4; \
            rowH = 0; \
        } \
        sX = curX; sY = curY; \
        AddSprite(ID, sX, sY, PW, PH); \
        curX += (PW) + 4; \
        rowH = std::max(rowH, (PH));

    // 0. WHITE_PIXEL
    {
        ALLOC_SPRITE(SpriteId::WHITE_PIXEL, 8, 8);
        DrawRect(buf, W, H, sX, sY, 8, 8, RGBA(255, 255, 255, 255));
    }

    // 1. TERRAIN_GRASS (64x32 Isometric Diamond)
    {
        ALLOC_SPRITE(SpriteId::TERRAIN_GRASS, 64, 32);
        DrawIsoDiamond(buf, W, H, sX, sY, 64, 32, RGBA(52, 98, 44), RGBA(45, 85, 38));
        // Noise & blades within diamond
        for (int y = 0; y < 32; ++y) {
            for (int x = 0; x < 64; ++x) {
                float dx = std::abs((float(x) + 0.5f) - 32.0f) / 32.0f;
                float dy = std::abs((float(y) + 0.5f) - 16.0f) / 16.0f;
                if (dx + dy < 0.92f) {
                    int noise = ((x * 37 + y * 91) % 17);
                    if (noise < 4) PutPixel(buf, W, H, sX + x, sY + y, RGBA(62, 118, 52));
                    else if (noise > 13) PutPixel(buf, W, H, sX + x, sY + y, RGBA(42, 80, 35));
                }
            }
        }
    }

    // 2. TERRAIN_DIRT (64x32 Isometric Diamond)
    {
        ALLOC_SPRITE(SpriteId::TERRAIN_DIRT, 64, 32);
        DrawIsoDiamond(buf, W, H, sX, sY, 64, 32, RGBA(115, 86, 52), RGBA(95, 70, 42));
        for (int y = 0; y < 32; ++y) {
            for (int x = 0; x < 64; ++x) {
                float dx = std::abs((float(x) + 0.5f) - 32.0f) / 32.0f;
                float dy = std::abs((float(y) + 0.5f) - 16.0f) / 16.0f;
                if (dx + dy < 0.92f) {
                    int noise = ((x * 53 + y * 79) % 19);
                    if (noise < 4) PutPixel(buf, W, H, sX + x, sY + y, RGBA(138, 105, 68));
                    else if (noise > 14) PutPixel(buf, W, H, sX + x, sY + y, RGBA(90, 68, 40));
                }
            }
        }
    }

    // 3. TERRAIN_WATER (64x32 Isometric Diamond)
    {
        ALLOC_SPRITE(SpriteId::TERRAIN_WATER, 64, 32);
        DrawIsoDiamond(buf, W, H, sX, sY, 64, 32, RGBA(26, 68, 105), RGBA(20, 52, 85));
        for (int y = 0; y < 32; ++y) {
            for (int x = 0; x < 64; ++x) {
                float dx = std::abs((float(x) + 0.5f) - 32.0f) / 32.0f;
                float dy = std::abs((float(y) + 0.5f) - 16.0f) / 16.0f;
                if (dx + dy < 0.90f) {
                    int wave = int(std::sin(x * 0.35f + y * 0.30f) * 10);
                    if (wave > 6) PutPixel(buf, W, H, sX + x, sY + y, RGBA(50, 110, 160));
                    else if (wave < -6) PutPixel(buf, W, H, sX + x, sY + y, RGBA(18, 48, 80));
                }
            }
        }
        DrawLine(buf, W, H, sX + 20, sY + 14, sX + 36, sY + 16, RGBA(120, 175, 220, 180));
    }

    // 4. TERRAIN_ROCKS (64x32 Isometric Diamond)
    {
        ALLOC_SPRITE(SpriteId::TERRAIN_ROCKS, 64, 32);
        DrawIsoDiamond(buf, W, H, sX, sY, 64, 32, RGBA(65, 60, 55), RGBA(50, 46, 42));
        DrawCircle(buf, W, H, sX + 32, sY + 16, 10, RGBA(90, 85, 80), true);
        DrawCircle(buf, W, H, sX + 30, sY + 14, 6, RGBA(115, 110, 105), true);
    }

    // 5. TERRAIN_ORE (64x32 Isometric Diamond)
    {
        ALLOC_SPRITE(SpriteId::TERRAIN_ORE, 64, 32);
        DrawIsoDiamond(buf, W, H, sX, sY, 64, 32, RGBA(75, 55, 35), RGBA(58, 42, 26));
        // Golden crystalline nuggets on diamond
        for (int i = 0; i < 9; ++i) {
            int ox = (i * 11 + 7) % 44 + 10;
            int oy = (i * 7 + 5) % 18 + 7;
            DrawCircle(buf, W, H, sX + ox, sY + oy, 2, RGBA(255, 195, 0), true);
            PutPixel(buf, W, H, sX + ox, sY + oy, RGBA(255, 245, 150));
        }
    }

    // 6. SHROUD_BLACK & SHROUD_FOG (64x32 Isometric Diamond)
    {
        ALLOC_SPRITE(SpriteId::SHROUD_BLACK, 64, 32);
        DrawIsoDiamond(buf, W, H, sX, sY, 64, 32, RGBA(0, 0, 0, 255), RGBA(0, 0, 0, 255));

        ALLOC_SPRITE(SpriteId::SHROUD_FOG, 64, 32);
        DrawIsoDiamond(buf, W, H, sX, sY, 64, 32, RGBA(0, 0, 0, 150), RGBA(0, 0, 0, 150));
    }

    // --- SOVIET BUILDINGS ---

    // 7. SOVIET_CONYARD (3x3 tiles -> 192x150, baseCenter=(96,100))
    {
        ALLOC_SPRITE(SpriteId::SOVIET_CONYARD, 192, 150);
        // Ground shadow
        DrawIsoShadow(buf, W, H, sX + 96, sY + 100, 184, 92, 10.0f, 6.0f);
        // Concrete foundation slab
        DrawIsoCube(buf, W, H, sX + 96, sY + 100, 184, 92, 5.0f,
                    RGBA(95, 98, 102), RGBA(75, 78, 82), RGBA(52, 54, 57), RGBA(30, 30, 32));
        // Secondary inner foundation platform
        DrawIsoCube(buf, W, H, sX + 96, sY + 95, 164, 82, 4.0f,
                    RGBA(115, 118, 122), RGBA(92, 95, 99), RGBA(64, 66, 70));
        // Main Soviet Command Bunker (center-left)
        DrawIsoCube(buf, W, H, sX + 80, sY + 86, 96, 48, 32.0f,
                    RGBA(195, 42, 32), RGBA(165, 30, 24), RGBA(115, 18, 14), RGBA(45, 8, 6));
        // Upper observation / radar command block
        DrawIsoCube(buf, W, H, sX + 70, sY + 54, 48, 24, 14.0f,
                    RGBA(155, 32, 26), RGBA(135, 25, 20), RGBA(95, 14, 10), RGBA(40, 8, 6));
        // Armored glass viewing slits
        DrawLine(buf, W, H, sX + 54, sY + 44, sX + 66, sY + 50, RGBA(100, 210, 255));
        DrawLine(buf, W, H, sX + 74, sY + 50, sX + 86, sY + 44, RGBA(70, 180, 230));
        // Communications mast and radar dish
        DrawLine(buf, W, H, sX + 58, sY + 36, sX + 58, sY + 22, RGBA(200, 205, 215));
        DrawCircle(buf, W, H, sX + 58, sY + 26, 6, RGBA(230, 235, 240), true);
        DrawCircle(buf, W, H, sX + 58, sY + 26, 6, RGBA(100, 105, 115), false);
        // Golden Soviet Star on the command bunker roof
        DrawStar(buf, W, H, sX + 94, sY + 52, 9, RGBA(255, 215, 0));
        // Yellow construction gantry crane
        DrawIsoCylinder(buf, W, H, sX + 136, sY + 76, 7, 4, 30.0f,
                        RGBA(240, 200, 20), RGBA(220, 180, 15), RGBA(150, 120, 10), RGBA(40, 30, 0));
        DrawLine(buf, W, H, sX + 136, sY + 46, sX + 104, sY + 30, RGBA(245, 210, 25));
        DrawLine(buf, W, H, sX + 136, sY + 47, sX + 104, sY + 31, RGBA(245, 210, 25));
        DrawIsoCube(buf, W, H, sX + 148, sY + 52, 14, 7, 10.0f,
                    RGBA(85, 88, 92), RGBA(65, 68, 72), RGBA(45, 48, 52));
        DrawLine(buf, W, H, sX + 114, sY + 35, sX + 114, sY + 58, RGBA(180, 180, 180));
        DrawRect(buf, W, H, sX + 112, sY + 58, 5, 5, RGBA(240, 200, 20));
        // Armored blast doors on front-right wall with caution threshold
        DrawIsoQuad(buf, W, H, sX + 88, sY + 98, sX + 112, sY + 86, sX + 112, sY + 72, sX + 88, sY + 84, RGBA(35, 38, 42));
        DrawLine(buf, W, H, sX + 88, sY + 98, sX + 112, sY + 86, RGBA(240, 200, 20));
    }

    // 8. SOVIET_POWER (Tesla Reactor, 2x2 tiles -> 128x112, baseCenter=(64,78))
    {
        ALLOC_SPRITE(SpriteId::SOVIET_POWER, 128, 112);
        // Ground shadow
        DrawIsoShadow(buf, W, H, sX + 64, sY + 78, 120, 60, 8.0f, 4.0f);
        // Concrete foundation slab
        DrawIsoCube(buf, W, H, sX + 64, sY + 78, 120, 60, 4.0f,
                    RGBA(95, 98, 102), RGBA(75, 78, 82), RGBA(52, 54, 57), RGBA(30, 30, 32));
        // Left reactor block
        DrawIsoCube(buf, W, H, sX + 38, sY + 68, 42, 22, 34.0f,
                    RGBA(185, 38, 30), RGBA(160, 30, 24), RGBA(110, 18, 14), RGBA(40, 8, 6));
        // Right reactor block
        DrawIsoCube(buf, W, H, sX + 90, sY + 68, 42, 22, 34.0f,
                    RGBA(185, 38, 30), RGBA(160, 30, 24), RGBA(110, 18, 14), RGBA(40, 8, 6));
        // Cooling louvers on reactor blocks
        for (int y = sY + 44; y < sY + 62; y += 4) {
            DrawLine(buf, W, H, sX + 44, y, sX + 54, y - 5, RGBA(40, 42, 45));
            DrawLine(buf, W, H, sX + 96, y, sX + 106, y - 5, RGBA(40, 42, 45));
        }
        // Central sunken energy trench with glowing Tesla induction core
        DrawIsoCube(buf, W, H, sX + 64, sY + 72, 34, 18, 14.0f,
                    RGBA(25, 45, 60), RGBA(20, 35, 48), RGBA(15, 25, 35), RGBA(10, 15, 20));
        DrawIsoCube(buf, W, H, sX + 64, sY + 67, 24, 12, 10.0f,
                    RGBA(140, 240, 255), RGBA(0, 180, 235), RGBA(0, 130, 185));
        DrawIsoCube(buf, W, H, sX + 64, sY + 65, 14, 7, 7.0f,
                    RGBA(255, 255, 255), RGBA(160, 245, 255), RGBA(80, 210, 255));
        // Magnetic induction coils bridging across the trench
        for (int dy = -4; dy <= 4; dy += 3) {
            DrawLine(buf, W, H, sX + 53, sY + 60 + dy, sX + 75, sY + 60 + dy, RGBA(255, 255, 255));
        }
        // Warning beacons on corner gables
        PutPixel(buf, W, H, sX + 38, sY + 45, RGBA(255, 40, 40));
        PutPixel(buf, W, H, sX + 90, sY + 45, RGBA(255, 40, 40));
    }

    // 9. SOVIET_REFINERY (3x3 tiles -> 192x150, baseCenter=(96,100))
    {
        ALLOC_SPRITE(SpriteId::SOVIET_REFINERY, 192, 150);
        // Ground shadow
        DrawIsoShadow(buf, W, H, sX + 96, sY + 100, 184, 92, 10.0f, 6.0f);
        // Concrete foundation slab
        DrawIsoCube(buf, W, H, sX + 96, sY + 100, 184, 92, 5.0f,
                    RGBA(95, 98, 102), RGBA(75, 78, 82), RGBA(52, 54, 57), RGBA(30, 30, 32));
        // Central Processing Warehouse (back-right)
        DrawIsoCube(buf, W, H, sX + 116, sY + 76, 92, 46, 36.0f,
                    RGBA(175, 36, 28), RGBA(150, 28, 22), RGBA(102, 16, 12), RGBA(40, 8, 6));
        // Industrial roof panel lines
        for (int rx = -35; rx <= 35; rx += 14) {
            DrawLine(buf, W, H, sX + 116 + rx - 20, sY + 40 + rx/2, sX + 116 + rx + 20, sY + 40 - rx/2, RGBA(135, 24, 18));
        }
        // Dual exhaust chimneys on warehouse roof
        DrawIsoCylinder(buf, W, H, sX + 135, sY + 36, 4, 2, 14.0f,
                        RGBA(60, 62, 65), RGBA(50, 52, 55), RGBA(35, 37, 40));
        DrawIsoCylinder(buf, W, H, sX + 148, sY + 42, 4, 2, 14.0f,
                        RGBA(60, 62, 65), RGBA(50, 52, 55), RGBA(35, 37, 40));
        // Twin 3D Cylindrical Ore Storage Silos (on left)
        // Silo 1 (rear)
        DrawIsoCylinder(buf, W, H, sX + 68, sY + 66, 18, 9, 46.0f,
                        RGBA(195, 42, 32), RGBA(175, 35, 26), RGBA(105, 16, 12), RGBA(40, 8, 6));
        DrawLine(buf, W, H, sX + 50, sY + 36, sX + 86, sY + 36, RGBA(75, 78, 82));
        DrawLine(buf, W, H, sX + 50, sY + 48, sX + 86, sY + 48, RGBA(75, 78, 82));
        DrawLine(buf, W, H, sX + 64, sY + 28, sX + 64, sY + 60, RGBA(255, 215, 0));
        // Silo 2 (front)
        DrawIsoCylinder(buf, W, H, sX + 46, sY + 84, 20, 10, 44.0f,
                        RGBA(195, 42, 32), RGBA(175, 35, 26), RGBA(105, 16, 12), RGBA(40, 8, 6));
        DrawLine(buf, W, H, sX + 26, sY + 54, sX + 66, sY + 54, RGBA(75, 78, 82));
        DrawLine(buf, W, H, sX + 26, sY + 68, sX + 66, sY + 68, RGBA(75, 78, 82));
        DrawLine(buf, W, H, sX + 42, sY + 48, sX + 42, sY + 80, RGBA(255, 215, 0));
        // Ore Unloading Hopper & Harvester Docking Ramp (front-right)
        DrawIsoCube(buf, W, H, sX + 130, sY + 104, 64, 32, 6.0f,
                    RGBA(85, 88, 92), RGBA(70, 72, 75), RGBA(50, 52, 55));
        for (int i = 0; i < 6; ++i) {
            DrawLine(buf, W, H, sX + 108 + i*6, sY + 108 - i*3, sX + 112 + i*6, sY + 108 - i*3, RGBA(240, 200, 20));
        }
        DrawIsoCube(buf, W, H, sX + 120, sY + 92, 36, 18, 8.0f,
                    RGBA(55, 42, 25), RGBA(45, 34, 20), RGBA(30, 22, 12));
        for (int i = 0; i < 14; ++i) {
            PutPixel(buf, W, H, sX + 110 + (i * 7) % 22, sY + 82 + (i * 5) % 10, RGBA(255, 225, 50));
            PutPixel(buf, W, H, sX + 111 + (i * 7) % 22, sY + 82 + (i * 5) % 10, RGBA(240, 185, 20));
        }
    }

    // 10. SOVIET_BARRACKS (2x2 tiles -> 128x110, baseCenter=(64,76))
    {
        ALLOC_SPRITE(SpriteId::SOVIET_BARRACKS, 128, 110);
        // Ground shadow
        DrawIsoShadow(buf, W, H, sX + 64, sY + 76, 120, 60, 8.0f, 4.0f);
        // Concrete foundation slab
        DrawIsoCube(buf, W, H, sX + 64, sY + 76, 120, 60, 4.0f,
                    RGBA(95, 98, 102), RGBA(75, 78, 82), RGBA(52, 54, 57), RGBA(30, 30, 32));
        // Reinforced concrete bunker base
        DrawIsoCube(buf, W, H, sX + 64, sY + 72, 98, 50, 22.0f,
                    RGBA(125, 128, 132), RGBA(105, 108, 112), RGBA(75, 78, 82), RGBA(35, 38, 42));
        // Sloped Soviet Crimson roof tier
        DrawIsoCube(buf, W, H, sX + 64, sY + 54, 78, 40, 16.0f,
                    RGBA(190, 38, 30), RGBA(160, 28, 22), RGBA(110, 16, 12), RGBA(40, 8, 6));
        // Golden Soviet Star on the front gabled roof
        DrawStar(buf, W, H, sX + 64, sY + 44, 9, RGBA(255, 215, 0));
        // Heavy armored blast door
        DrawIsoQuad(buf, W, H, sX + 46, sY + 76, sX + 60, sY + 83, sX + 60, sY + 68, sX + 46, sY + 61, RGBA(30, 32, 36));
        PutPixel(buf, W, H, sX + 53, sY + 64, RGBA(60, 240, 60));
        // Stacked sandbag defense parapet
        for (int sb = 0; sb < 4; ++sb) {
            DrawIsoCube(buf, W, H, sX + 38 + sb*6, sY + 84 + sb*3, 10, 5, 5.0f,
                        RGBA(175, 150, 105), RGBA(155, 130, 90), RGBA(120, 100, 65));
        }
        // Communications whip antenna
        DrawLine(buf, W, H, sX + 88, sY + 36, sX + 88, sY + 18, RGBA(195, 200, 208));
        PutPixel(buf, W, H, sX + 88, sY + 18, RGBA(255, 40, 40));
    }

    // 11. SOVIET_WARFACTORY (3x3 tiles -> 192x155, baseCenter=(96,105))
    {
        ALLOC_SPRITE(SpriteId::SOVIET_WARFACTORY, 192, 155);
        // Ground shadow
        DrawIsoShadow(buf, W, H, sX + 96, sY + 105, 184, 92, 10.0f, 6.0f);
        // Concrete foundation slab
        DrawIsoCube(buf, W, H, sX + 96, sY + 105, 184, 92, 5.0f,
                    RGBA(95, 98, 102), RGBA(75, 78, 82), RGBA(52, 54, 57), RGBA(30, 30, 32));
        // Massive industrial assembly hangar
        DrawIsoCube(buf, W, H, sX + 96, sY + 84, 152, 76, 42.0f,
                    RGBA(175, 36, 28), RGBA(148, 28, 22), RGBA(102, 16, 12), RGBA(40, 8, 6));
        // Curved roof monitor with skylights
        DrawIsoCube(buf, W, H, sX + 96, sY + 54, 96, 48, 14.0f,
                    RGBA(145, 26, 20), RGBA(125, 22, 16), RGBA(85, 12, 8));
        for (int dy = -10; dy <= 10; dy += 5) {
            DrawLine(buf, W, H, sX + 76 + dy*2, sY + 44 + dy, sX + 116 + dy*2, sY + 44 + dy, RGBA(120, 210, 245));
        }
        // Massive vehicle roll-up blast doors
        DrawIsoQuad(buf, W, H, sX + 102, sY + 98, sX + 146, sY + 76, sX + 146, sY + 50, sX + 102, sY + 72, RGBA(45, 48, 52));
        for (int s = 0; s < 6; ++s) {
            DrawLine(buf, W, H, sX + 104, sY + 74 + s*4, sX + 144, sY + 54 + s*4, RGBA(70, 75, 80));
        }
        DrawLine(buf, W, H, sX + 102, sY + 98, sX + 102, sY + 72, RGBA(245, 205, 20));
        DrawLine(buf, W, H, sX + 102, sY + 72, sX + 146, sY + 50, RGBA(245, 205, 20));
        DrawLine(buf, W, H, sX + 146, sY + 50, sX + 146, sY + 76, RGBA(245, 205, 20));
        // Overhead yellow crane runway track on roof
        DrawLine(buf, W, H, sX + 50, sY + 62, sX + 120, sY + 27, RGBA(245, 205, 20));
        DrawLine(buf, W, H, sX + 50, sY + 63, sX + 120, sY + 28, RGBA(245, 205, 20));
        DrawRect(buf, W, H, sX + 80, sY + 44, 8, 6, RGBA(60, 62, 65));
    }

    // 12. SOVIET_RADAR (2x2 tiles -> 128x120, baseCenter=(64,86))
    {
        ALLOC_SPRITE(SpriteId::SOVIET_RADAR, 128, 120);
        // Ground shadow
        DrawIsoShadow(buf, W, H, sX + 64, sY + 86, 118, 58, 8.0f, 4.0f);
        // Concrete foundation slab
        DrawIsoCube(buf, W, H, sX + 64, sY + 86, 118, 58, 5.0f,
                    RGBA(95, 98, 102), RGBA(75, 78, 82), RGBA(52, 54, 57), RGBA(30, 30, 32));
        // Octagonal concrete bunker body
        DrawIsoCube(buf, W, H, sX + 64, sY + 81, 94, 46, 20.0f,
                    RGBA(118, 122, 126), RGBA(98, 102, 106), RGBA(68, 72, 76), RGBA(35, 38, 42));
        DrawLine(buf, W, H, sX + 17, sY + 81, sX + 64, sY + 104, RGBA(180, 32, 25));
        DrawLine(buf, W, H, sX + 64, sY + 104, sX + 111, sY + 81, RGBA(140, 22, 18));
        // 3D Geodesic Radar Dome
        DrawIsoDome(buf, W, H, sX + 64, sY + 61, 34.0f, 22.0f,
                    RGBA(215, 45, 35), RGBA(115, 18, 14), RGBA(40, 8, 6));
        // Satellite dish
        DrawLine(buf, W, H, sX + 96, sY + 64, sX + 96, sY + 48, RGBA(180, 185, 195));
        DrawCircle(buf, W, H, sX + 96, sY + 48, 8, RGBA(235, 240, 245), true);
        DrawCircle(buf, W, H, sX + 96, sY + 48, 8, RGBA(120, 125, 135), false);
        PutPixel(buf, W, H, sX + 64, sY + 38, RGBA(255, 40, 40));
        PutPixel(buf, W, H, sX + 64, sY + 37, RGBA(255, 150, 150));
    }

    // 13. SOVIET_TESLA_COIL (2x2 tiles -> 128x150, baseCenter=(64,116))
    {
        ALLOC_SPRITE(SpriteId::SOVIET_TESLA_COIL, 128, 150);
        // Ground shadow
        DrawIsoShadow(buf, W, H, sX + 64, sY + 116, 96, 48, 8.0f, 4.0f);
        // Stepped concrete foundation
        DrawIsoCube(buf, W, H, sX + 64, sY + 116, 96, 48, 6.0f,
                    RGBA(95, 98, 102), RGBA(75, 78, 82), RGBA(52, 54, 57), RGBA(30, 30, 32));
        DrawIsoCube(buf, W, H, sX + 64, sY + 110, 72, 36, 6.0f,
                    RGBA(118, 122, 126), RGBA(95, 98, 102), RGBA(65, 68, 72), RGBA(30, 30, 32));
        DrawLine(buf, W, H, sX + 28, sY + 110, sX + 64, sY + 128, RGBA(185, 35, 28));
        DrawLine(buf, W, H, sX + 64, sY + 128, sX + 100, sY + 110, RGBA(125, 20, 16));
        // Steel lattice tower legs
        DrawLine(buf, W, H, sX + 44, sY + 104, sX + 55, sY + 34, RGBA(180, 185, 195));
        DrawLine(buf, W, H, sX + 45, sY + 104, sX + 56, sY + 34, RGBA(180, 185, 195));
        DrawLine(buf, W, H, sX + 84, sY + 104, sX + 73, sY + 34, RGBA(95, 100, 110));
        DrawLine(buf, W, H, sX + 83, sY + 104, sX + 72, sY + 34, RGBA(95, 100, 110));
        // Tower cross-bracing
        for (int step = 0; step < 4; ++step) {
            float f0 = float(step) / 4.0f;
            float f1 = float(step + 1) / 4.0f;
            int y0 = int(sY + 104 - f0 * 70.0f);
            int y1 = int(sY + 104 - f1 * 70.0f);
            int xL0 = int(sX + 44 + f0 * 11.0f);
            int xR0 = int(sX + 84 - f0 * 11.0f);
            int xL1 = int(sX + 44 + f1 * 11.0f);
            int xR1 = int(sX + 84 - f1 * 11.0f);
            DrawLine(buf, W, H, xL0, y0, xR0, y0, RGBA(140, 145, 155));
            DrawLine(buf, W, H, xL0, y0, xR1, y1, RGBA(130, 135, 145));
            DrawLine(buf, W, H, xR0, y0, xL1, y1, RGBA(110, 115, 125));
        }
        // Ceramic insulator rings
        DrawIsoCylinder(buf, W, H, sX + 64, sY + 84, 16, 8, 6.0f,
                        RGBA(230, 235, 242), RGBA(205, 210, 220), RGBA(140, 145, 155), RGBA(60, 65, 75));
        DrawIsoCylinder(buf, W, H, sX + 64, sY + 68, 14, 7, 6.0f,
                        RGBA(230, 235, 242), RGBA(205, 210, 220), RGBA(140, 145, 155), RGBA(60, 65, 75));
        DrawIsoCylinder(buf, W, H, sX + 64, sY + 52, 12, 6, 6.0f,
                        RGBA(230, 235, 242), RGBA(205, 210, 220), RGBA(140, 145, 155), RGBA(60, 65, 75));
        DrawIsoCylinder(buf, W, H, sX + 64, sY + 38, 10, 5, 5.0f,
                        RGBA(230, 235, 242), RGBA(205, 210, 220), RGBA(140, 145, 155), RGBA(60, 65, 75));
        // Crowning metallic discharge sphere
        DrawCircle(buf, W, H, sX + 64, sY + 18, 11, RGBA(185, 245, 255), true);
        DrawCircle(buf, W, H, sX + 64, sY + 18, 8, RGBA(120, 220, 255), true);
        DrawCircle(buf, W, H, sX + 62, sY + 16, 5, RGBA(255, 255, 255), true);
        DrawCircle(buf, W, H, sX + 64, sY + 18, 11, RGBA(0, 120, 180), false);
        // Crackling electric sparks
        DrawLine(buf, W, H, sX + 64, sY + 7, sX + 57, sY + 13, RGBA(120, 240, 255));
        DrawLine(buf, W, H, sX + 75, sY + 12, sX + 70, sY + 16, RGBA(180, 250, 255));
        DrawLine(buf, W, H, sX + 53, sY + 22, sX + 58, sY + 20, RGBA(120, 240, 255));
    }

    // --- ALLIED BUILDINGS (Blue faction) ---

    // 14. ALLIED_CONYARD (3x3 tiles -> 192x150, baseCenter=(96,100))
    {
        ALLOC_SPRITE(SpriteId::ALLIED_CONYARD, 192, 150);
        DrawIsoShadow(buf, W, H, sX + 96, sY + 100, 184, 92, 10.0f, 6.0f);
        DrawIsoCube(buf, W, H, sX + 96, sY + 100, 184, 92, 5.0f,
                    RGBA(115, 118, 125), RGBA(92, 95, 102), RGBA(65, 68, 75), RGBA(30, 32, 38));
        DrawIsoCube(buf, W, H, sX + 96, sY + 95, 164, 82, 4.0f,
                    RGBA(130, 134, 142), RGBA(105, 109, 117), RGBA(75, 79, 87));
        DrawIsoCube(buf, W, H, sX + 80, sY + 86, 96, 48, 32.0f,
                    RGBA(52, 108, 190), RGBA(42, 88, 160), RGBA(25, 55, 110), RGBA(12, 28, 60));
        DrawIsoCube(buf, W, H, sX + 70, sY + 54, 48, 24, 14.0f,
                    RGBA(65, 125, 210), RGBA(50, 100, 175), RGBA(30, 65, 125));
        DrawLine(buf, W, H, sX + 54, sY + 44, sX + 66, sY + 50, RGBA(180, 240, 255));
        DrawLine(buf, W, H, sX + 74, sY + 50, sX + 86, sY + 44, RGBA(140, 210, 245));
        DrawStar(buf, W, H, sX + 94, sY + 52, 9, RGBA(245, 250, 255));
        DrawIsoCylinder(buf, W, H, sX + 136, sY + 76, 7, 4, 30.0f,
                        RGBA(60, 120, 205), RGBA(45, 95, 170), RGBA(25, 60, 115), RGBA(10, 25, 55));
        DrawLine(buf, W, H, sX + 136, sY + 46, sX + 104, sY + 30, RGBA(235, 240, 248));
        DrawLine(buf, W, H, sX + 114, sY + 35, sX + 114, sY + 58, RGBA(180, 185, 195));
        DrawRect(buf, W, H, sX + 112, sY + 58, 5, 5, RGBA(235, 240, 248));
        DrawCircle(buf, W, H, sX + 58, sY + 26, 6, RGBA(235, 240, 245), true);
        DrawCircle(buf, W, H, sX + 58, sY + 26, 6, RGBA(100, 105, 115), false);
    }

    // 15. ALLIED_POWER (2x2 tiles -> 128x112, baseCenter=(64,78))
    {
        ALLOC_SPRITE(SpriteId::ALLIED_POWER, 128, 112);
        DrawIsoShadow(buf, W, H, sX + 64, sY + 78, 120, 60, 8.0f, 4.0f);
        DrawIsoCube(buf, W, H, sX + 64, sY + 78, 120, 60, 4.0f,
                    RGBA(115, 118, 125), RGBA(92, 95, 102), RGBA(65, 68, 75), RGBA(30, 32, 38));
        DrawIsoCube(buf, W, H, sX + 64, sY + 74, 96, 48, 30.0f,
                    RGBA(52, 108, 190), RGBA(42, 88, 160), RGBA(25, 55, 110), RGBA(12, 28, 60));
        DrawIsoCylinder(buf, W, H, sX + 48, sY + 48, 10, 5, 24.0f,
                        RGBA(85, 90, 98), RGBA(72, 76, 84), RGBA(48, 52, 58), RGBA(25, 28, 32));
        DrawIsoCylinder(buf, W, H, sX + 80, sY + 48, 10, 5, 24.0f,
                        RGBA(85, 90, 98), RGBA(72, 76, 84), RGBA(48, 52, 58), RGBA(25, 28, 32));
        DrawCircle(buf, W, H, sX + 48, sY + 24, 4, RGBA(0, 190, 245), true);
        DrawCircle(buf, W, H, sX + 80, sY + 24, 4, RGBA(0, 190, 245), true);
    }

    // 16. ALLIED_BARRACKS (2x2 tiles -> 128x110, baseCenter=(64,76))
    {
        ALLOC_SPRITE(SpriteId::ALLIED_BARRACKS, 128, 110);
        DrawIsoShadow(buf, W, H, sX + 64, sY + 76, 120, 60, 8.0f, 4.0f);
        DrawIsoCube(buf, W, H, sX + 64, sY + 76, 120, 60, 4.0f,
                    RGBA(115, 118, 125), RGBA(92, 95, 102), RGBA(65, 68, 75), RGBA(30, 32, 38));
        DrawIsoCube(buf, W, H, sX + 64, sY + 72, 98, 50, 22.0f,
                    RGBA(132, 136, 144), RGBA(110, 114, 122), RGBA(80, 84, 92), RGBA(35, 38, 45));
        DrawIsoCube(buf, W, H, sX + 64, sY + 54, 78, 40, 16.0f,
                    RGBA(52, 108, 190), RGBA(42, 88, 160), RGBA(25, 55, 110), RGBA(12, 28, 60));
        DrawStar(buf, W, H, sX + 64, sY + 44, 9, RGBA(245, 250, 255));
        DrawIsoQuad(buf, W, H, sX + 46, sY + 76, sX + 60, sY + 83, sX + 60, sY + 68, sX + 46, sY + 61, RGBA(35, 38, 45));
        PutPixel(buf, W, H, sX + 53, sY + 64, RGBA(60, 240, 60));
        DrawLine(buf, W, H, sX + 88, sY + 36, sX + 88, sY + 18, RGBA(215, 220, 230));
    }

    // 17. ALLIED_WARFACTORY (3x3 tiles -> 192x155, baseCenter=(96,105))
    {
        ALLOC_SPRITE(SpriteId::ALLIED_WARFACTORY, 192, 155);
        DrawIsoShadow(buf, W, H, sX + 96, sY + 105, 184, 92, 10.0f, 6.0f);
        DrawIsoCube(buf, W, H, sX + 96, sY + 105, 184, 92, 5.0f,
                    RGBA(115, 118, 125), RGBA(92, 95, 102), RGBA(65, 68, 75), RGBA(30, 32, 38));
        DrawIsoCube(buf, W, H, sX + 96, sY + 84, 152, 76, 42.0f,
                    RGBA(52, 108, 190), RGBA(42, 88, 160), RGBA(25, 55, 110), RGBA(12, 28, 60));
        DrawIsoCube(buf, W, H, sX + 96, sY + 54, 96, 48, 14.0f,
                    RGBA(40, 85, 155), RGBA(32, 70, 130), RGBA(20, 45, 90));
        for (int dy = -10; dy <= 10; dy += 5) {
            DrawLine(buf, W, H, sX + 76 + dy*2, sY + 44 + dy, sX + 116 + dy*2, sY + 44 + dy, RGBA(180, 240, 255));
        }
        DrawIsoQuad(buf, W, H, sX + 102, sY + 98, sX + 146, sY + 76, sX + 146, sY + 50, sX + 102, sY + 72, RGBA(40, 44, 50));
        for (int s = 0; s < 6; ++s) {
            DrawLine(buf, W, H, sX + 104, sY + 74 + s*4, sX + 144, sY + 54 + s*4, RGBA(75, 80, 90));
        }
    }

    // 18. ALLIED_PILLBOX (2x2 tiles -> 128x85, baseCenter=(64,51))
    {
        ALLOC_SPRITE(SpriteId::ALLIED_PILLBOX, 128, 85);
        DrawIsoShadow(buf, W, H, sX + 64, sY + 51, 96, 48, 6.0f, 3.0f);
        DrawIsoCube(buf, W, H, sX + 64, sY + 51, 96, 48, 4.0f,
                    RGBA(115, 118, 125), RGBA(92, 95, 102), RGBA(65, 68, 75), RGBA(30, 32, 38));
        DrawIsoCube(buf, W, H, sX + 64, sY + 47, 76, 38, 18.0f,
                    RGBA(135, 140, 148), RGBA(112, 116, 125), RGBA(80, 85, 95), RGBA(35, 38, 45));
        DrawIsoCube(buf, W, H, sX + 64, sY + 31, 32, 16, 8.0f,
                    RGBA(52, 108, 190), RGBA(42, 88, 160), RGBA(25, 55, 110), RGBA(12, 28, 60));
        DrawIsoQuad(buf, W, H, sX + 46, sY + 48, sX + 62, sY + 56, sX + 62, sY + 52, sX + 46, sY + 44, RGBA(15, 15, 18));
        DrawLine(buf, W, H, sX + 49, sY + 49, sX + 44, sY + 53, RGBA(25, 25, 30));
        DrawLine(buf, W, H, sX + 50, sY + 49, sX + 45, sY + 53, RGBA(60, 60, 70));
        DrawLine(buf, W, H, sX + 57, sY + 53, sX + 52, sY + 57, RGBA(25, 25, 30));
        DrawLine(buf, W, H, sX + 58, sY + 53, sX + 53, sY + 57, RGBA(60, 60, 70));
        for (int sb = 0; sb < 4; ++sb) {
            DrawIsoCube(buf, W, H, sX + 38 + sb*6, sY + 54 + sb*3, 10, 5, 5.0f,
                        RGBA(175, 150, 105), RGBA(155, 130, 90), RGBA(120, 100, 65));
        }
    }

    // --- SOVIET UNITS ---

    // 19. SOVIET_CONSCRIPT (32x32)
    {
        ALLOC_SPRITE(SpriteId::SOVIET_CONSCRIPT, 32, 32);
        // Red uniform body
        DrawRect(buf, W, H, sX + 11, sY + 12, 10, 12, RGBA(175, 28, 22));
        // Head / ushanka hat
        DrawCircle(buf, W, H, sX + 16, sY + 8, 5, RGBA(90, 70, 50), true);
        PutPixel(buf, W, H, sX + 16, sY + 7, RGBA(255, 215, 0)); // Red/gold star badge
        // Face
        DrawRect(buf, W, H, sX + 14, sY + 9, 4, 3, RGBA(225, 185, 150));
        // AK-47 Rifle
        DrawLine(buf, W, H, sX + 16, sY + 15, sX + 26, sY + 15, RGBA(40, 40, 42));
        DrawRect(buf, W, H, sX + 20, sY + 16, 2, 4, RGBA(140, 80, 30)); // wood stock
        // Boots
        DrawRect(buf, W, H, sX + 12, sY + 24, 3, 5, RGBA(25, 25, 25));
        DrawRect(buf, W, H, sX + 17, sY + 24, 3, 5, RGBA(25, 25, 25));
    }

    // 20. SOVIET_TESLA_TROOPER (32x32)
    {
        ALLOC_SPRITE(SpriteId::SOVIET_TESLA_TROOPER, 32, 32);
        // Armored heavy insulated suit
        DrawRect(buf, W, H, sX + 10, sY + 11, 12, 13, RGBA(100, 105, 110));
        DrawRectOutline(buf, W, H, sX + 10, sY + 11, 12, 13, RGBA(180, 30, 25));
        // Enclosed helmet with visor
        DrawCircle(buf, W, H, sX + 16, sY + 7, 5, RGBA(80, 85, 90), true);
        DrawRect(buf, W, H, sX + 14, sY + 7, 4, 2, RGBA(0, 220, 255)); // cyan glow visor
        // Tesla gauntlet rods
        DrawLine(buf, W, H, sX + 18, sY + 14, sX + 27, sY + 12, RGBA(200, 205, 210));
        DrawCircle(buf, W, H, sX + 27, sY + 12, 2, RGBA(0, 220, 255), true);
        // Boots
        DrawRect(buf, W, H, sX + 11, sY + 24, 4, 5, RGBA(40, 42, 45));
        DrawRect(buf, W, H, sX + 17, sY + 24, 4, 5, RGBA(40, 42, 45));
    }

    // 21. SOVIET_HEAVY_TANK_BODY (48x48)
    {
        ALLOC_SPRITE(SpriteId::SOVIET_HEAVY_TANK_BODY, 48, 48);
        // Treads
        DrawRect(buf, W, H, sX + 6, sY + 6, 8, 36, RGBA(45, 48, 50));
        DrawRect(buf, W, H, sX + 34, sY + 6, 8, 36, RGBA(45, 48, 50));
        for (int y = 8; y < 42; y += 4) {
            DrawLine(buf, W, H, sX + 6, sY + y, sX + 14, sY + y, RGBA(80, 82, 85));
            DrawLine(buf, W, H, sX + 34, sY + y, sX + 42, sY + y, RGBA(80, 82, 85));
        }
        // Red armored hull
        DrawRect(buf, W, H, sX + 14, sY + 8, 20, 32, RGBA(175, 28, 22));
        DrawRectOutline(buf, W, H, sX + 14, sY + 8, 20, 32, RGBA(100, 15, 12));
        // Engine grilles at rear
        DrawRect(buf, W, H, sX + 17, sY + 32, 14, 6, RGBA(60, 62, 65));
    }

    // 22. SOVIET_HEAVY_TANK_TURRET (48x48)
    {
        ALLOC_SPRITE(SpriteId::SOVIET_HEAVY_TANK_TURRET, 48, 48);
        // Centered turret dome
        DrawCircle(buf, W, H, sX + 24, sY + 24, 10, RGBA(195, 32, 25), true);
        DrawCircle(buf, W, H, sX + 24, sY + 24, 10, RGBA(110, 18, 14), false);
        // Soviet Star on turret hatch
        DrawStar(buf, W, H, sX + 24, sY + 25, 4, RGBA(255, 215, 0));
        // Dual long cannon barrels pointing up (angle 0)
        DrawRect(buf, W, H, sX + 20, sY + 4, 3, 16, RGBA(75, 78, 80));
        DrawRect(buf, W, H, sX + 25, sY + 4, 3, 16, RGBA(75, 78, 80));
        // Muzzle brakes
        DrawRect(buf, W, H, sX + 19, sY + 3, 5, 3, RGBA(40, 42, 45));
        DrawRect(buf, W, H, sX + 24, sY + 3, 5, 3, RGBA(40, 42, 45));
    }

    // 23. SOVIET_MAMMOTH_TANK_BODY (64x64)
    {
        ALLOC_SPRITE(SpriteId::SOVIET_MAMMOTH_TANK_BODY, 64, 64);
        // Massive quad treads
        DrawRect(buf, W, H, sX + 4, sY + 6, 12, 52, RGBA(40, 42, 45));
        DrawRect(buf, W, H, sX + 48, sY + 6, 12, 52, RGBA(40, 42, 45));
        for (int y = 8; y < 58; y += 4) {
            DrawLine(buf, W, H, sX + 4, sY + y, sX + 16, sY + y, RGBA(85, 90, 95));
            DrawLine(buf, W, H, sX + 48, sY + y, sX + 60, sY + y, RGBA(85, 90, 95));
        }
        // Heavy super armor chassis
        DrawRect(buf, W, H, sX + 16, sY + 8, 32, 48, RGBA(160, 24, 18));
        DrawRectOutline(buf, W, H, sX + 16, sY + 8, 32, 48, RGBA(80, 10, 8));
        // Front ram bumper
        DrawRect(buf, W, H, sX + 18, sY + 6, 28, 4, RGBA(90, 92, 95));
    }

    // 24. SOVIET_MAMMOTH_TANK_TURRET (64x64)
    {
        ALLOC_SPRITE(SpriteId::SOVIET_MAMMOTH_TANK_TURRET, 64, 64);
        // Heavy angular fortress turret
        DrawRect(buf, W, H, sX + 18, sY + 20, 28, 28, RGBA(185, 30, 24));
        DrawRectOutline(buf, W, H, sX + 18, sY + 20, 28, 28, RGBA(95, 12, 10));
        // Side missile pod pods
        DrawRect(buf, W, H, sX + 12, sY + 26, 6, 16, RGBA(60, 62, 65));
        DrawRect(buf, W, H, sX + 46, sY + 26, 6, 16, RGBA(60, 62, 65));
        DrawCircle(buf, W, H, sX + 15, sY + 29, 2, RGBA(220, 50, 40), true);
        DrawCircle(buf, W, H, sX + 15, sY + 37, 2, RGBA(220, 50, 40), true);
        DrawCircle(buf, W, H, sX + 49, sY + 29, 2, RGBA(220, 50, 40), true);
        DrawCircle(buf, W, H, sX + 49, sY + 37, 2, RGBA(220, 50, 40), true);
        // Massive twin 120mm main cannons
        DrawRect(buf, W, H, sX + 24, sY + 2, 6, 22, RGBA(70, 72, 75));
        DrawRect(buf, W, H, sX + 34, sY + 2, 6, 22, RGBA(70, 72, 75));
        DrawRect(buf, W, H, sX + 23, sY + 1, 8, 4, RGBA(40, 42, 45));
        DrawRect(buf, W, H, sX + 33, sY + 1, 8, 4, RGBA(40, 42, 45));
        // Golden crest
        DrawStar(buf, W, H, sX + 32, sY + 32, 5, RGBA(255, 215, 0));
    }

    // 25. SOVIET_V2_LAUNCHER (48x48)
    {
        ALLOC_SPRITE(SpriteId::SOVIET_V2_LAUNCHER, 48, 48);
        // Half-track body
        DrawRect(buf, W, H, sX + 10, sY + 10, 28, 30, RGBA(155, 25, 20));
        DrawRectOutline(buf, W, H, sX + 10, sY + 10, 28, 30, RGBA(80, 10, 8));
        // Giant V2 rocket sitting on launch rail
        DrawRect(buf, W, H, sX + 21, sY + 4, 6, 28, RGBA(240, 240, 245));
        // Red nosecone
        DrawLine(buf, W, H, sX + 21, sY + 4, sX + 24, sY + 0, RGBA(220, 30, 25));
        DrawLine(buf, W, H, sX + 26, sY + 4, sX + 24, sY + 0, RGBA(220, 30, 25));
        // Rocket fins
        DrawLine(buf, W, H, sX + 19, sY + 30, sX + 21, sY + 24, RGBA(220, 30, 25));
        DrawLine(buf, W, H, sX + 29, sY + 30, sX + 27, sY + 24, RGBA(220, 30, 25));
    }

    // 26. SOVIET_HARVESTER (56x56)
    {
        ALLOC_SPRITE(SpriteId::SOVIET_HARVESTER, 56, 56);
        // Heavy treads
        DrawRect(buf, W, H, sX + 4, sY + 6, 10, 44, RGBA(45, 48, 52));
        DrawRect(buf, W, H, sX + 42, sY + 6, 10, 44, RGBA(45, 48, 52));
        // Cab & chassis
        DrawRect(buf, W, H, sX + 14, sY + 10, 28, 36, RGBA(160, 28, 22));
        DrawRectOutline(buf, W, H, sX + 14, sY + 10, 28, 36, RGBA(85, 12, 10));
        // Front ore scoop
        DrawRect(buf, W, H, sX + 12, sY + 4, 32, 6, RGBA(100, 102, 105));
        // Ore bed with golden crystals
        DrawRect(buf, W, H, sX + 18, sY + 24, 20, 18, RGBA(60, 45, 25));
        for (int i = 0; i < 8; ++i) {
            PutPixel(buf, W, H, sX + 20 + (i * 5) % 16, sY + 26 + (i * 7) % 14, RGBA(255, 215, 0));
        }
    }

    // --- ALLIED UNITS ---

    // 27. ALLIED_RIFLEMAN (32x32)
    {
        ALLOC_SPRITE(SpriteId::ALLIED_RIFLEMAN, 32, 32);
        DrawRect(buf, W, H, sX + 11, sY + 12, 10, 12, RGBA(35, 85, 155));
        DrawCircle(buf, W, H, sX + 16, sY + 8, 5, RGBA(60, 90, 60), true); // helmet
        DrawRect(buf, W, H, sX + 14, sY + 9, 4, 3, RGBA(225, 185, 150));
        DrawLine(buf, W, H, sX + 16, sY + 15, sX + 26, sY + 15, RGBA(40, 40, 42));
    }

    // 28. ALLIED_LIGHT_TANK_BODY & TURRET (40x40 & 40x40)
    {
        ALLOC_SPRITE(SpriteId::ALLIED_LIGHT_TANK_BODY, 40, 40);
        DrawRect(buf, W, H, sX + 6, sY + 6, 6, 28, RGBA(45, 48, 50));
        DrawRect(buf, W, H, sX + 28, sY + 6, 6, 28, RGBA(45, 48, 50));
        DrawRect(buf, W, H, sX + 12, sY + 8, 16, 24, RGBA(40, 95, 170));
        DrawRectOutline(buf, W, H, sX + 12, sY + 8, 16, 24, RGBA(20, 50, 95));

        ALLOC_SPRITE(SpriteId::ALLIED_LIGHT_TANK_TURRET, 40, 40);
        DrawCircle(buf, W, H, sX + 20, sY + 20, 8, RGBA(50, 110, 195), true);
        DrawRect(buf, W, H, sX + 19, sY + 4, 3, 14, RGBA(65, 70, 75));
    }

    // 29. ALLIED_MEDIUM_TANK_BODY & TURRET (48x48 & 48x48)
    {
        ALLOC_SPRITE(SpriteId::ALLIED_MEDIUM_TANK_BODY, 48, 48);
        DrawRect(buf, W, H, sX + 6, sY + 6, 8, 36, RGBA(45, 48, 50));
        DrawRect(buf, W, H, sX + 34, sY + 6, 8, 36, RGBA(45, 48, 50));
        DrawRect(buf, W, H, sX + 14, sY + 8, 20, 32, RGBA(35, 85, 160));
        DrawRectOutline(buf, W, H, sX + 14, sY + 8, 20, 32, RGBA(15, 45, 90));

        ALLOC_SPRITE(SpriteId::ALLIED_MEDIUM_TANK_TURRET, 48, 48);
        DrawCircle(buf, W, H, sX + 24, sY + 24, 10, RGBA(45, 100, 180), true);
        DrawRect(buf, W, H, sX + 22, sY + 3, 4, 18, RGBA(65, 70, 75));
    }

    // --- PROJECTILES & EXPLOSIONS ---

    // 30. PROJECTILE_BULLET, ROCKET, SHELL
    {
        ALLOC_SPRITE(SpriteId::PROJECTILE_BULLET, 8, 8);
        DrawCircle(buf, W, H, sX + 4, sY + 4, 3, RGBA(255, 230, 100), true);

        ALLOC_SPRITE(SpriteId::PROJECTILE_ROCKET, 16, 16);
        DrawRect(buf, W, H, sX + 6, sY + 2, 4, 10, RGBA(220, 220, 230));
        DrawRect(buf, W, H, sX + 6, sY + 2, 4, 3, RGBA(220, 40, 30)); // tip
        DrawCircle(buf, W, H, sX + 8, sY + 13, 3, RGBA(255, 140, 20), true); // plume

        ALLOC_SPRITE(SpriteId::PROJECTILE_SHELL, 12, 12);
        DrawCircle(buf, W, H, sX + 6, sY + 6, 4, RGBA(255, 200, 50), true);
    }

    // 31. EXPLOSION ANIMATION FRAMES (48x48 each)
    {
        ALLOC_SPRITE(SpriteId::EXPLOSION_FRAME_0, 48, 48);
        DrawCircle(buf, W, H, sX + 24, sY + 24, 8, RGBA(255, 255, 200), true);
        DrawCircle(buf, W, H, sX + 24, sY + 24, 12, RGBA(255, 180, 0), true);

        ALLOC_SPRITE(SpriteId::EXPLOSION_FRAME_1, 48, 48);
        DrawCircle(buf, W, H, sX + 24, sY + 24, 16, RGBA(255, 100, 0), true);
        DrawCircle(buf, W, H, sX + 24, sY + 24, 10, RGBA(255, 230, 50), true);

        ALLOC_SPRITE(SpriteId::EXPLOSION_FRAME_2, 48, 48);
        DrawCircle(buf, W, H, sX + 24, sY + 24, 20, RGBA(200, 40, 10), true);
        DrawCircle(buf, W, H, sX + 24, sY + 24, 13, RGBA(255, 120, 0), true);

        ALLOC_SPRITE(SpriteId::EXPLOSION_FRAME_3, 48, 48);
        DrawCircle(buf, W, H, sX + 24, sY + 24, 22, RGBA(80, 75, 70, 200), true);
        DrawCircle(buf, W, H, sX + 24, sY + 24, 12, RGBA(160, 50, 20, 160), true);
    }

    // 32. PARTICLES: SMOKE, SPARK, FLAME
    {
        ALLOC_SPRITE(SpriteId::PARTICLE_SMOKE, 16, 16);
        DrawCircle(buf, W, H, sX + 8, sY + 8, 6, RGBA(80, 80, 85, 200), true);

        ALLOC_SPRITE(SpriteId::PARTICLE_SPARK, 8, 8);
        DrawCircle(buf, W, H, sX + 4, sY + 4, 3, RGBA(0, 220, 255), true);
        PutPixel(buf, W, H, sX + 4, sY + 4, RGBA(255, 255, 255));

        ALLOC_SPRITE(SpriteId::PARTICLE_FLAME, 12, 12);
        DrawCircle(buf, W, H, sX + 6, sY + 6, 5, RGBA(255, 120, 0), true);
        PutPixel(buf, W, H, sX + 6, sY + 6, RGBA(255, 240, 80));
    }

    // --- UI ELEMENTS ---

    // 33. UI_RADAR_BEZEL (160x160)
    {
        ALLOC_SPRITE(SpriteId::UI_RADAR_BEZEL, 160, 160);
        DrawRect(buf, W, H, sX, sY, 160, 160, RGBA(25, 28, 30));
        DrawRectOutline(buf, W, H, sX, sY, 160, 160, RGBA(65, 70, 75));
        DrawRectOutline(buf, W, H, sX + 2, sY + 2, 156, 156, RGBA(15, 16, 18));
    }

    // 34. UI_RADAR_SWEEP (48x48)
    {
        ALLOC_SPRITE(SpriteId::UI_RADAR_SWEEP, 48, 48);
        DrawLine(buf, W, H, sX + 24, sY + 24, sX + 44, sY + 24, RGBA(0, 255, 120, 220));
    }

    // 35. UI_PANEL_BG (64x64)
    {
        ALLOC_SPRITE(SpriteId::UI_PANEL_BG, 64, 64);
        DrawRect(buf, W, H, sX, sY, 64, 64, RGBA(38, 42, 46));
        DrawRectOutline(buf, W, H, sX, sY, 64, 64, RGBA(60, 65, 72));
    }

    // 36. UI_POWER_METER_BG & FILL
    {
        ALLOC_SPRITE(SpriteId::UI_POWER_METER_BG, 24, 128);
        DrawRect(buf, W, H, sX, sY, 24, 128, RGBA(18, 20, 22));
        DrawRectOutline(buf, W, H, sX, sY, 24, 128, RGBA(55, 58, 62));

        ALLOC_SPRITE(SpriteId::UI_POWER_METER_FILL, 24, 128);
        DrawRect(buf, W, H, sX, sY, 24, 128, RGBA(40, 220, 60));
    }

    // 37. SOVIET_CREST (32x32)
    {
        ALLOC_SPRITE(SpriteId::SOVIET_CREST, 32, 32);
        DrawCircle(buf, W, H, sX + 16, sY + 16, 14, RGBA(180, 25, 20), true);
        DrawCircle(buf, W, H, sX + 16, sY + 16, 14, RGBA(255, 215, 0), false);
        DrawStar(buf, W, H, sX + 16, sY + 16, 9, RGBA(255, 215, 0));
    }

    // 38. UI ICONS FOR BUILDINGS & UNITS (48x48 each)
    {
        ALLOC_SPRITE(SpriteId::UI_ICON_POWER, 48, 48);
        DrawRect(buf, W, H, sX, sY, 48, 48, RGBA(45, 50, 55));
        DrawRectOutline(buf, W, H, sX, sY, 48, 48, RGBA(90, 95, 100));
        DrawRect(buf, W, H, sX + 10, sY + 10, 28, 28, RGBA(170, 30, 25));
        DrawCircle(buf, W, H, sX + 24, sY + 24, 8, RGBA(0, 220, 255), true);

        ALLOC_SPRITE(SpriteId::UI_ICON_REFINERY, 48, 48);
        DrawRect(buf, W, H, sX, sY, 48, 48, RGBA(45, 50, 55));
        DrawRectOutline(buf, W, H, sX, sY, 48, 48, RGBA(90, 95, 100));
        DrawCircle(buf, W, H, sX + 18, sY + 24, 8, RGBA(170, 30, 25), true);
        DrawRect(buf, W, H, sX + 26, sY + 16, 14, 18, RGBA(255, 215, 0));

        ALLOC_SPRITE(SpriteId::UI_ICON_BARRACKS, 48, 48);
        DrawRect(buf, W, H, sX, sY, 48, 48, RGBA(45, 50, 55));
        DrawRectOutline(buf, W, H, sX, sY, 48, 48, RGBA(90, 95, 100));
        DrawRect(buf, W, H, sX + 8, sY + 12, 32, 24, RGBA(160, 25, 20));
        DrawStar(buf, W, H, sX + 24, sY + 22, 6, RGBA(255, 215, 0));

        ALLOC_SPRITE(SpriteId::UI_ICON_WARFACTORY, 48, 48);
        DrawRect(buf, W, H, sX, sY, 48, 48, RGBA(45, 50, 55));
        DrawRectOutline(buf, W, H, sX, sY, 48, 48, RGBA(90, 95, 100));
        DrawRect(buf, W, H, sX + 8, sY + 8, 32, 32, RGBA(155, 28, 22));
        DrawRect(buf, W, H, sX + 14, sY + 22, 20, 16, RGBA(35, 38, 42));

        ALLOC_SPRITE(SpriteId::UI_ICON_RADAR, 48, 48);
        DrawRect(buf, W, H, sX, sY, 48, 48, RGBA(45, 50, 55));
        DrawRectOutline(buf, W, H, sX, sY, 48, 48, RGBA(90, 95, 100));
        DrawCircle(buf, W, H, sX + 24, sY + 24, 14, RGBA(180, 30, 25), true);
        DrawCircle(buf, W, H, sX + 24, sY + 24, 8, RGBA(230, 235, 240), true);

        ALLOC_SPRITE(SpriteId::UI_ICON_TESLA, 48, 48);
        DrawRect(buf, W, H, sX, sY, 48, 48, RGBA(45, 50, 55));
        DrawRectOutline(buf, W, H, sX, sY, 48, 48, RGBA(90, 95, 100));
        DrawLine(buf, W, H, sX + 16, sY + 40, sX + 24, sY + 16, RGBA(170, 175, 180));
        DrawLine(buf, W, H, sX + 32, sY + 40, sX + 24, sY + 16, RGBA(170, 175, 180));
        DrawCircle(buf, W, H, sX + 24, sY + 12, 6, RGBA(0, 220, 255), true);

        ALLOC_SPRITE(SpriteId::UI_ICON_CONSCRIPT, 48, 48);
        DrawRect(buf, W, H, sX, sY, 48, 48, RGBA(45, 50, 55));
        DrawRectOutline(buf, W, H, sX, sY, 48, 48, RGBA(90, 95, 100));
        DrawCircle(buf, W, H, sX + 24, sY + 16, 7, RGBA(90, 70, 50), true);
        DrawRect(buf, W, H, sX + 16, sY + 24, 16, 16, RGBA(175, 28, 22));

        ALLOC_SPRITE(SpriteId::UI_ICON_TESLATROOPER, 48, 48);
        DrawRect(buf, W, H, sX, sY, 48, 48, RGBA(45, 50, 55));
        DrawRectOutline(buf, W, H, sX, sY, 48, 48, RGBA(90, 95, 100));
        DrawCircle(buf, W, H, sX + 24, sY + 16, 7, RGBA(100, 105, 110), true);
        DrawRect(buf, W, H, sX + 15, sY + 24, 18, 16, RGBA(100, 105, 110));
        DrawCircle(buf, W, H, sX + 32, sY + 24, 3, RGBA(0, 220, 255), true);

        ALLOC_SPRITE(SpriteId::UI_ICON_HEAVYTANK, 48, 48);
        DrawRect(buf, W, H, sX, sY, 48, 48, RGBA(45, 50, 55));
        DrawRectOutline(buf, W, H, sX, sY, 48, 48, RGBA(90, 95, 100));
        DrawRect(buf, W, H, sX + 10, sY + 16, 28, 20, RGBA(175, 28, 22));
        DrawRect(buf, W, H, sX + 18, sY + 8, 3, 10, RGBA(75, 78, 80));
        DrawRect(buf, W, H, sX + 26, sY + 8, 3, 10, RGBA(75, 78, 80));

        ALLOC_SPRITE(SpriteId::UI_ICON_MAMMOTH, 48, 48);
        DrawRect(buf, W, H, sX, sY, 48, 48, RGBA(45, 50, 55));
        DrawRectOutline(buf, W, H, sX, sY, 48, 48, RGBA(90, 95, 100));
        DrawRect(buf, W, H, sX + 8, sY + 14, 32, 24, RGBA(160, 24, 18));
        DrawRect(buf, W, H, sX + 16, sY + 6, 5, 12, RGBA(70, 72, 75));
        DrawRect(buf, W, H, sX + 26, sY + 6, 5, 12, RGBA(70, 72, 75));

        ALLOC_SPRITE(SpriteId::UI_ICON_V2, 48, 48);
        DrawRect(buf, W, H, sX, sY, 48, 48, RGBA(45, 50, 55));
        DrawRectOutline(buf, W, H, sX, sY, 48, 48, RGBA(90, 95, 100));
        DrawRect(buf, W, H, sX + 12, sY + 16, 24, 20, RGBA(155, 25, 20));
        DrawRect(buf, W, H, sX + 22, sY + 6, 4, 18, RGBA(240, 240, 245));

        ALLOC_SPRITE(SpriteId::UI_ICON_HARVESTER, 48, 48);
        DrawRect(buf, W, H, sX, sY, 48, 48, RGBA(45, 50, 55));
        DrawRectOutline(buf, W, H, sX, sY, 48, 48, RGBA(90, 95, 100));
        DrawRect(buf, W, H, sX + 10, sY + 14, 28, 24, RGBA(160, 28, 22));
        DrawRect(buf, W, H, sX + 16, sY + 18, 16, 12, RGBA(255, 215, 0));

        ALLOC_SPRITE(SpriteId::UI_ICON_REPAIR, 48, 48);
        DrawRect(buf, W, H, sX, sY, 48, 48, RGBA(45, 50, 55));
        DrawRectOutline(buf, W, H, sX, sY, 48, 48, RGBA(90, 95, 100));
        // Wrench icon
        DrawLine(buf, W, H, sX + 14, sY + 34, sX + 32, sY + 16, RGBA(220, 225, 230));
        DrawLine(buf, W, H, sX + 15, sY + 35, sX + 33, sY + 17, RGBA(220, 225, 230));
        DrawCircle(buf, W, H, sX + 32, sY + 16, 5, RGBA(220, 225, 230), false);

        ALLOC_SPRITE(SpriteId::UI_ICON_SELL, 48, 48);
        DrawRect(buf, W, H, sX, sY, 48, 48, RGBA(45, 50, 55));
        DrawRectOutline(buf, W, H, sX, sY, 48, 48, RGBA(90, 95, 100));
        // Dollar icon
        DrawCircle(buf, W, H, sX + 24, sY + 24, 10, RGBA(255, 215, 0), false);
        DrawLine(buf, W, H, sX + 24, sY + 10, sX + 24, sY + 38, RGBA(255, 215, 0));
    }

    #undef ALLOC_SPRITE
}

// 8x12 pixel monospace bitmap font covering ASCII 32..126
void TextureAtlas::GenerateFont(std::vector<uint32_t>& buf, int W, int H) {
    int startY = 1750;
    int curX = 0;
    int curY = startY;

    // 8x12 character patterns (simplified crisp 8x12 font)
    // We can generate clean procedural pixel representations for letters, numbers, punctuation
    for (int c = 32; c < 127; ++c) {
        if (curX + 10 > W) {
            curX = 0;
            curY += 16;
        }

        int gx = curX;
        int gy = curY;
        int gw = 8;
        int gh = 12;

        // Draw character representation into buffer
        // Background transparent
        // Standard glyphs:
        auto Plot = [&](int x, int y) {
            if (x >= 0 && x < 8 && y >= 0 && y < 12) {
                PutPixel(buf, W, H, gx + x, gy + y, RGBA(255, 255, 255));
            }
        };

        char ch = static_cast<char>(c);
        if (ch >= 'A' && ch <= 'Z') {
            // Draw A-Z
            switch (ch) {
                case 'A':
                    for (int y = 2; y <= 10; ++y) { Plot(1, y); Plot(6, y); }
                    for (int x = 2; x <= 5; ++x) { Plot(x, 1); Plot(x, 6); }
                    break;
                case 'B':
                    for (int y = 1; y <= 10; ++y) Plot(1, y);
                    for (int x = 2; x <= 5; ++x) { Plot(x, 1); Plot(x, 6); Plot(x, 10); }
                    Plot(6, 2); Plot(6, 3); Plot(6, 4); Plot(6, 5); Plot(6, 7); Plot(6, 8); Plot(6, 9);
                    break;
                case 'C':
                    for (int y = 2; y <= 9; ++y) Plot(1, y);
                    for (int x = 2; x <= 6; ++x) { Plot(x, 1); Plot(x, 10); }
                    Plot(6, 2); Plot(6, 9);
                    break;
                case 'D':
                    for (int y = 1; y <= 10; ++y) Plot(1, y);
                    for (int x = 2; x <= 5; ++x) { Plot(x, 1); Plot(x, 10); }
                    for (int y = 2; y <= 9; ++y) Plot(6, y);
                    break;
                case 'E':
                    for (int y = 1; y <= 10; ++y) Plot(1, y);
                    for (int x = 2; x <= 6; ++x) { Plot(x, 1); Plot(x, 10); }
                    for (int x = 2; x <= 5; ++x) Plot(x, 5);
                    break;
                case 'F':
                    for (int y = 1; y <= 10; ++y) Plot(1, y);
                    for (int x = 2; x <= 6; ++x) Plot(x, 1);
                    for (int x = 2; x <= 5; ++x) Plot(x, 5);
                    break;
                case 'G':
                    for (int y = 2; y <= 9; ++y) Plot(1, y);
                    for (int x = 2; x <= 6; ++x) { Plot(x, 1); Plot(x, 10); }
                    for (int y = 5; y <= 9; ++y) Plot(6, y);
                    Plot(4, 5); Plot(5, 5);
                    break;
                case 'H':
                    for (int y = 1; y <= 10; ++y) { Plot(1, y); Plot(6, y); }
                    for (int x = 2; x <= 5; ++x) Plot(x, 5);
                    break;
                case 'I':
                    for (int y = 1; y <= 10; ++y) Plot(4, y);
                    for (int x = 2; x <= 6; ++x) { Plot(x, 1); Plot(x, 10); }
                    break;
                case 'J':
                    for (int y = 1; y <= 9; ++y) Plot(5, y);
                    for (int x = 2; x <= 5; ++x) Plot(x, 10);
                    Plot(1, 8); Plot(1, 9);
                    break;
                case 'K':
                    for (int y = 1; y <= 10; ++y) Plot(1, y);
                    for (int i = 0; i <= 4; ++i) { Plot(6 - i, 1 + i); Plot(2 + i, 6 + i); }
                    break;
                case 'L':
                    for (int y = 1; y <= 10; ++y) Plot(1, y);
                    for (int x = 2; x <= 6; ++x) Plot(x, 10);
                    break;
                case 'M':
                    for (int y = 1; y <= 10; ++y) { Plot(1, y); Plot(6, y); }
                    Plot(2, 2); Plot(3, 4); Plot(4, 4); Plot(5, 2);
                    break;
                case 'N':
                    for (int y = 1; y <= 10; ++y) { Plot(1, y); Plot(6, y); }
                    for (int i = 2; i <= 5; ++i) Plot(i, i * 2);
                    break;
                case 'O':
                    for (int y = 2; y <= 9; ++y) { Plot(1, y); Plot(6, y); }
                    for (int x = 2; x <= 5; ++x) { Plot(x, 1); Plot(x, 10); }
                    break;
                case 'P':
                    for (int y = 1; y <= 10; ++y) Plot(1, y);
                    for (int x = 2; x <= 5; ++x) { Plot(x, 1); Plot(x, 6); }
                    for (int y = 2; y <= 5; ++y) Plot(6, y);
                    break;
                case 'Q':
                    for (int y = 2; y <= 9; ++y) { Plot(1, y); Plot(6, y); }
                    for (int x = 2; x <= 5; ++x) { Plot(x, 1); Plot(x, 10); }
                    Plot(4, 8); Plot(5, 9); Plot(6, 10);
                    break;
                case 'R':
                    for (int y = 1; y <= 10; ++y) Plot(1, y);
                    for (int x = 2; x <= 5; ++x) { Plot(x, 1); Plot(x, 6); }
                    for (int y = 2; y <= 5; ++y) Plot(6, y);
                    for (int i = 0; i <= 4; ++i) Plot(2 + i, 6 + i);
                    break;
                case 'S':
                    for (int x = 2; x <= 6; ++x) Plot(x, 1);
                    Plot(1, 2); Plot(1, 3); Plot(1, 4);
                    for (int x = 2; x <= 5; ++x) Plot(x, 5);
                    Plot(6, 6); Plot(6, 7); Plot(6, 8); Plot(6, 9);
                    for (int x = 1; x <= 5; ++x) Plot(x, 10);
                    break;
                case 'T':
                    for (int x = 1; x <= 7; ++x) Plot(x, 1);
                    for (int y = 2; y <= 10; ++y) Plot(4, y);
                    break;
                case 'U':
                    for (int y = 1; y <= 9; ++y) { Plot(1, y); Plot(6, y); }
                    for (int x = 2; x <= 5; ++x) Plot(x, 10);
                    break;
                case 'V':
                    for (int y = 1; y <= 7; ++y) { Plot(1, y); Plot(6, y); }
                    Plot(2, 8); Plot(5, 8); Plot(3, 9); Plot(4, 9); Plot(3, 10); Plot(4, 10);
                    break;
                case 'W':
                    for (int y = 1; y <= 10; ++y) { Plot(1, y); Plot(6, y); }
                    Plot(2, 9); Plot(5, 9); Plot(3, 7); Plot(4, 7);
                    break;
                case 'X':
                    for (int i = 0; i <= 9; ++i) {
                        Plot(1 + (i * 5) / 9, 1 + i);
                        Plot(6 - (i * 5) / 9, 1 + i);
                    }
                    break;
                case 'Y':
                    Plot(1, 1); Plot(2, 2); Plot(3, 3); Plot(4, 4);
                    Plot(7, 1); Plot(6, 2); Plot(5, 3);
                    for (int y = 5; y <= 10; ++y) Plot(4, y);
                    break;
                case 'Z':
                    for (int x = 1; x <= 6; ++x) { Plot(x, 1); Plot(x, 10); }
                    for (int i = 0; i <= 8; ++i) Plot(6 - (i * 5) / 8, 2 + i);
                    break;
            }
        } else if (ch >= '0' && ch <= '9') {
            // Digits 0-9
            switch (ch) {
                case '0':
                    for (int y = 2; y <= 9; ++y) { Plot(1, y); Plot(6, y); }
                    for (int x = 2; x <= 5; ++x) { Plot(x, 1); Plot(x, 10); }
                    Plot(5, 3); Plot(4, 5); Plot(3, 7); Plot(2, 9);
                    break;
                case '1':
                    for (int y = 1; y <= 10; ++y) Plot(4, y);
                    Plot(2, 3); Plot(3, 2);
                    for (int x = 2; x <= 6; ++x) Plot(x, 10);
                    break;
                case '2':
                    for (int x = 2; x <= 5; ++x) Plot(x, 1);
                    Plot(1, 2); Plot(6, 2); Plot(6, 3); Plot(6, 4);
                    Plot(5, 5); Plot(4, 6); Plot(3, 7); Plot(2, 8); Plot(1, 9);
                    for (int x = 1; x <= 6; ++x) Plot(x, 10);
                    break;
                case '3':
                    for (int x = 1; x <= 5; ++x) { Plot(x, 1); Plot(x, 10); }
                    Plot(6, 2); Plot(6, 3); Plot(6, 7); Plot(6, 8); Plot(6, 9);
                    for (int x = 2; x <= 5; ++x) Plot(x, 5);
                    break;
                case '4':
                    for (int y = 1; y <= 6; ++y) Plot(1, y);
                    for (int x = 1; x <= 6; ++x) Plot(x, 6);
                    for (int y = 1; y <= 10; ++y) Plot(5, y);
                    break;
                case '5':
                    for (int x = 1; x <= 6; ++x) Plot(x, 1);
                    for (int y = 1; y <= 5; ++y) Plot(1, y);
                    for (int x = 1; x <= 5; ++x) { Plot(x, 5); Plot(x, 10); }
                    Plot(6, 6); Plot(6, 7); Plot(6, 8); Plot(6, 9);
                    break;
                case '6':
                    for (int y = 2; y <= 9; ++y) Plot(1, y);
                    for (int x = 2; x <= 5; ++x) { Plot(x, 1); Plot(x, 5); Plot(x, 10); }
                    Plot(6, 6); Plot(6, 7); Plot(6, 8); Plot(6, 9);
                    break;
                case '7':
                    for (int x = 1; x <= 6; ++x) Plot(x, 1);
                    for (int i = 0; i <= 8; ++i) Plot(6 - (i * 3) / 8, 2 + i);
                    break;
                case '8':
                    for (int y = 2; y <= 4; ++y) { Plot(1, y); Plot(6, y); }
                    for (int y = 6; y <= 9; ++y) { Plot(1, y); Plot(6, y); }
                    for (int x = 2; x <= 5; ++x) { Plot(x, 1); Plot(x, 5); Plot(x, 10); }
                    break;
                case '9':
                    for (int y = 2; y <= 5; ++y) { Plot(1, y); Plot(6, y); }
                    for (int x = 2; x <= 5; ++x) { Plot(x, 1); Plot(x, 5); Plot(x, 10); }
                    for (int y = 6; y <= 9; ++y) Plot(6, y);
                    break;
            }
        } else {
            // Symbols
            switch (ch) {
                case '$':
                    for (int y = 0; y <= 11; ++y) Plot(4, y);
                    for (int x = 2; x <= 6; ++x) { Plot(x, 2); Plot(x, 6); Plot(x, 9); }
                    Plot(2, 3); Plot(2, 4); Plot(2, 5);
                    Plot(6, 7); Plot(6, 8);
                    break;
                case ':':
                    Plot(4, 4); Plot(4, 5); Plot(4, 8); Plot(4, 9);
                    break;
                case '/':
                    for (int i = 0; i <= 9; ++i) Plot(6 - (i * 5) / 9, 1 + i);
                    break;
                case '-':
                    for (int x = 2; x <= 6; ++x) Plot(x, 6);
                    break;
                case '+':
                    for (int x = 2; x <= 6; ++x) Plot(x, 6);
                    for (int y = 4; y <= 8; ++y) Plot(4, y);
                    break;
                case '!':
                    for (int y = 1; y <= 7; ++y) Plot(4, y);
                    Plot(4, 9); Plot(4, 10);
                    break;
                case '?':
                    for (int x = 2; x <= 5; ++x) Plot(x, 1);
                    Plot(1, 2); Plot(6, 2); Plot(6, 3); Plot(5, 4); Plot(4, 5); Plot(4, 6);
                    Plot(4, 9); Plot(4, 10);
                    break;
                case '.':
                    Plot(3, 9); Plot(4, 9); Plot(3, 10); Plot(4, 10);
                    break;
                case ',':
                    Plot(4, 8); Plot(4, 9); Plot(3, 10);
                    break;
                case '(':
                    Plot(4, 1); Plot(3, 2);
                    for (int y = 3; y <= 8; ++y) Plot(2, y);
                    Plot(3, 9); Plot(4, 10);
                    break;
                case ')':
                    Plot(3, 1); Plot(4, 2);
                    for (int y = 3; y <= 8; ++y) Plot(5, y);
                    Plot(4, 9); Plot(3, 10);
                    break;
                case '[':
                    for (int y = 1; y <= 10; ++y) Plot(2, y);
                    for (int x = 2; x <= 5; ++x) { Plot(x, 1); Plot(x, 10); }
                    break;
                case ']':
                    for (int y = 1; y <= 10; ++y) Plot(5, y);
                    for (int x = 2; x <= 5; ++x) { Plot(x, 1); Plot(x, 10); }
                    break;
                case '%':
                    Plot(2, 2); Plot(3, 3);
                    for (int i = 0; i <= 8; ++i) Plot(6 - (i * 4) / 8, 2 + i);
                    Plot(5, 8); Plot(6, 9);
                    break;
                case '>':
                    Plot(2, 3); Plot(3, 4); Plot(4, 5); Plot(5, 6); Plot(4, 7); Plot(3, 8); Plot(2, 9);
                    break;
                case '<':
                    Plot(5, 3); Plot(4, 4); Plot(3, 5); Plot(2, 6); Plot(3, 7); Plot(4, 8); Plot(5, 9);
                    break;
                default:
                    // Lowercase letters match uppercase style
                    if (ch >= 'a' && ch <= 'z') {
                        char up = ch - 32;
                        // Map glyph
                        m_glyphs[c] = m_glyphs[static_cast<uint8_t>(up)];
                    }
                    break;
            }
        }

        GlyphRect gr;
        gr.w = gw;
        gr.h = gh;
        gr.u0 = float(gx) / float(W);
        gr.v0 = float(gy) / float(H);
        gr.u1 = float(gx + gw) / float(W);
        gr.v1 = float(gy + gh) / float(H);
        m_glyphs[c] = gr;

        curX += gw + 2;
    }

    // Mirror lowercase to uppercase glyphs
    for (char lc = 'a'; lc <= 'z'; ++lc) {
        m_glyphs[static_cast<uint8_t>(lc)] = m_glyphs[static_cast<uint8_t>(lc - 32)];
    }
}
