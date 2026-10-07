#pragma once

#include <vector>
#include <unordered_map>
#include <string>
#include <cstdint>

#define GL_SILENCE_DEPRECATION
#define GLFW_INCLUDE_NONE
#include <OpenGL/gl3.h>

enum class SpriteId {
    WHITE_PIXEL,

    TERRAIN_GRASS,
    TERRAIN_DIRT,
    TERRAIN_WATER,
    TERRAIN_ROCKS,
    TERRAIN_ORE,
    SHROUD_BLACK,
    SHROUD_FOG,

    SOVIET_CONYARD,
    SOVIET_POWER,
    SOVIET_REFINERY,
    SOVIET_BARRACKS,
    SOVIET_WARFACTORY,
    SOVIET_RADAR,
    SOVIET_TESLA_COIL,

    ALLIED_CONYARD,
    ALLIED_POWER,
    ALLIED_BARRACKS,
    ALLIED_WARFACTORY,
    ALLIED_PILLBOX,

    SOVIET_CONSCRIPT,
    SOVIET_TESLA_TROOPER,
    SOVIET_HEAVY_TANK_BODY,
    SOVIET_HEAVY_TANK_TURRET,
    SOVIET_MAMMOTH_TANK_BODY,
    SOVIET_MAMMOTH_TANK_TURRET,
    SOVIET_V2_LAUNCHER,
    SOVIET_HARVESTER,

    ALLIED_RIFLEMAN,
    ALLIED_LIGHT_TANK_BODY,
    ALLIED_LIGHT_TANK_TURRET,
    ALLIED_MEDIUM_TANK_BODY,
    ALLIED_MEDIUM_TANK_TURRET,

    PROJECTILE_BULLET,
    PROJECTILE_ROCKET,
    PROJECTILE_SHELL,
    EXPLOSION_FRAME_0,
    EXPLOSION_FRAME_1,
    EXPLOSION_FRAME_2,
    EXPLOSION_FRAME_3,
    PARTICLE_SMOKE,
    PARTICLE_SPARK,
    PARTICLE_FLAME,

    UI_RADAR_BEZEL,
    UI_RADAR_SWEEP,
    UI_PANEL_BG,
    UI_POWER_METER_BG,
    UI_POWER_METER_FILL,
    UI_ICON_POWER,
    UI_ICON_REFINERY,
    UI_ICON_BARRACKS,
    UI_ICON_WARFACTORY,
    UI_ICON_RADAR,
    UI_ICON_TESLA,
    UI_ICON_CONSCRIPT,
    UI_ICON_TESLATROOPER,
    UI_ICON_HEAVYTANK,
    UI_ICON_MAMMOTH,
    UI_ICON_V2,
    UI_ICON_HARVESTER,
    UI_ICON_REPAIR,
    UI_ICON_SELL,
    SOVIET_CREST,

    COUNT
};

struct SpriteRect {
    float u0{0.0f}, v0{0.0f}, u1{1.0f}, v1{1.0f};
    int pixelW{0}, pixelH{0};
};

struct GlyphRect {
    float u0{0.0f}, v0{0.0f}, u1{1.0f}, v1{1.0f};
    int w{8}, h{12};
};

class TextureAtlas {
public:
    TextureAtlas();
    ~TextureAtlas();

    bool BuildAtlas();
    void Bind(int textureUnit = 0) const;

    const SpriteRect& GetSprite(SpriteId id) const;
    const GlyphRect& GetGlyph(char c) const;

    GLuint GetTextureId() const { return m_textureId; }

private:
    void GenerateAllSprites(std::vector<uint32_t>& buffer, int atlasW, int atlasH);
    void GenerateFont(std::vector<uint32_t>& buffer, int atlasW, int atlasH);

    GLuint m_textureId{0};
    std::unordered_map<SpriteId, SpriteRect> m_sprites;
    GlyphRect m_glyphs[128];
};
