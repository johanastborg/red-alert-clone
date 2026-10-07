#pragma once

#include "Renderer.hpp"
#include <vector>
#include <glm/glm.hpp>

enum class TerrainType {
    GRASS,
    DIRT,
    WATER,
    ROCKS,
    ORE
};

enum class ShroudStatus {
    SHROUDED = 0, // Unexplored black
    FOG = 1,      // Explored, not in vision
    VISIBLE = 2   // Currently in sight
};

struct MapTile {
    TerrainType terrain{TerrainType::GRASS};
    float oreAmount{0.0f}; // 0 to 1000
    int buildingId{-1};    // Building occupying tile, or -1
    ShroudStatus shroud{ShroudStatus::SHROUDED};
};

class Map {
public:
    Map();
    ~Map();

    void Initialize(int width = 72, int height = 72);
    void Render(Renderer& renderer, float camX, float camY, float halfW, float halfH);

    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }
    float GetTileSize() const { return TILE_SIZE; }

    bool IsInBounds(int tx, int ty) const;
    bool IsPassable(int tx, int ty) const;
    bool CanPlaceBuilding(int tx, int ty, int wTiles, int hTiles) const;

    void SetBuildingOccupation(int tx, int ty, int wTiles, int hTiles, int buildingId);
    void ClearBuildingOccupation(int tx, int ty, int wTiles, int hTiles);

    // Fog of war
    void BeginFogPass();
    void RevealVision(float worldX, float worldY, float sightRadiusTiles);
    ShroudStatus GetShroudAt(int tx, int ty) const;
    bool IsVisibleWorld(float wx, float wy) const;

    // Coordinate transforms (Isometric 2:1 projection)
    static constexpr float TILE_W = 64.0f;
    static constexpr float TILE_H = 32.0f;
    static constexpr float TILE_SIZE = 32.0f;

    static glm::vec2 TileToWorld(float gx, float gy) {
        float isoX = (gx - gy) * (TILE_W * 0.5f);
        float isoY = (gx + gy) * (TILE_H * 0.5f);
        return glm::vec2(isoX, isoY);
    }

    static glm::vec2 TileCenterToWorld(int tx, int ty) {
        return TileToWorld(float(tx) + 0.5f, float(ty) + 0.5f);
    }

    static glm::ivec2 WorldToTile(float isoX, float isoY) {
        float gx = (isoX / TILE_W) + (isoY / TILE_H);
        float gy = (isoY / TILE_H) - (isoX / TILE_W);
        return glm::ivec2(int(std::floor(gx)), int(std::floor(gy)));
    }

    // Economy & resources
    glm::ivec2 FindNearestOre(float wx, float wy, float maxSearchDist = 2000.0f);
    float HarvestOre(int tx, int ty, float amountRequested);
    void RegenerateOre(float dt);

    // Pathfinding
    std::vector<glm::vec2> FindPath(glm::vec2 startPos, glm::vec2 endPos);

private:
    void GenerateTerrain();

    int m_width{72};
    int m_height{72};
    std::vector<MapTile> m_tiles;

    float m_oreRegenTimer{0.0f};
};
