#include "Map.hpp"
#include <cmath>
#include <queue>
#include <algorithm>
#include <iostream>

namespace {

struct Node {
    int x, y;
    float g, f;
    bool operator>(const Node& o) const { return f > o.f; }
};

inline float Dist(int x1, int y1, int x2, int y2) {
    float dx = float(x1 - x2);
    float dy = float(y1 - y2);
    return std::sqrt(dx * dx + dy * dy);
}

} // namespace

Map::Map() = default;
Map::~Map() = default;

void Map::Initialize(int width, int height) {
    m_width = width;
    m_height = height;
    m_tiles.resize(m_width * m_height);

    GenerateTerrain();

    // Reveal starting area around Soviet base (tx: 6..28, ty: 42..68)
    for (int y = 42; y <= 68; ++y) {
        for (int x = 6; x <= 28; ++x) {
            if (IsInBounds(x, y)) {
                m_tiles[y * m_width + x].shroud = ShroudStatus::VISIBLE;
            }
        }
    }
}

void Map::GenerateTerrain() {
    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            MapTile& t = m_tiles[y * m_width + x];
            t.terrain = TerrainType::GRASS;
            t.oreAmount = 0.0f;
            t.buildingId = -1;
            t.shroud = ShroudStatus::SHROUDED;

            // River flowing diagonally
            // river center roughly y = 0.65 * x + 15
            float riverY = 0.60f * float(x) + 16.0f;
            float distToRiver = std::abs(float(y) - riverY);
            if (distToRiver < 2.5f) {
                t.terrain = TerrainType::WATER;
            }

            // Bridges / crossings across river at x=25..28 and x=48..51
            if ((x >= 24 && x <= 27) || (x >= 46 && x <= 49)) {
                if (distToRiver < 3.5f) {
                    t.terrain = TerrainType::DIRT;
                }
            }

            // Dirt road connecting bridges to base areas
            if ((x >= 24 && x <= 27 && y >= 32 && y <= 56) ||
                (y >= 54 && y <= 56 && x >= 15 && x <= 27) ||
                (x >= 46 && x <= 49 && y >= 18 && y <= 45)) {
                if (t.terrain != TerrainType::WATER) {
                    t.terrain = TerrainType::DIRT;
                }
            }

            // Rocky ridges / cliffs
            // Ridge 1 near center-left
            if ((x >= 12 && x <= 15 && y >= 25 && y <= 35) ||
                (x >= 55 && x <= 58 && y >= 38 && y <= 48)) {
                t.terrain = TerrainType::ROCKS;
            }
        }
    }

    // Ore field 1: Near Soviet base (x: 18..24, y: 46..51)
    for (int y = 46; y <= 51; ++y) {
        for (int x = 18; x <= 24; ++x) {
            if (IsInBounds(x, y) && m_tiles[y * m_width + x].terrain != TerrainType::WATER) {
                m_tiles[y * m_width + x].terrain = TerrainType::ORE;
                m_tiles[y * m_width + x].oreAmount = 800.0f;
            }
        }
    }

    // Ore field 2: Contested central field (x: 33..41, y: 32..38)
    for (int y = 32; y <= 38; ++y) {
        for (int x = 33; x <= 41; ++x) {
            if (IsInBounds(x, y) && m_tiles[y * m_width + x].terrain != TerrainType::WATER) {
                m_tiles[y * m_width + x].terrain = TerrainType::ORE;
                m_tiles[y * m_width + x].oreAmount = 1000.0f;
            }
        }
    }

    // Ore field 3: Near Allied base (x: 48..54, y: 22..27)
    for (int y = 22; y <= 27; ++y) {
        for (int x = 48; x <= 54; ++x) {
            if (IsInBounds(x, y) && m_tiles[y * m_width + x].terrain != TerrainType::WATER) {
                m_tiles[y * m_width + x].terrain = TerrainType::ORE;
                m_tiles[y * m_width + x].oreAmount = 800.0f;
            }
        }
    }
}

bool Map::IsInBounds(int tx, int ty) const {
    return tx >= 0 && tx < m_width && ty >= 0 && ty < m_height;
}

bool Map::IsPassable(int tx, int ty) const {
    if (!IsInBounds(tx, ty)) return false;
    const MapTile& t = m_tiles[ty * m_width + tx];
    if (t.terrain == TerrainType::WATER || t.terrain == TerrainType::ROCKS) return false;
    if (t.buildingId != -1) return false;
    return true;
}

bool Map::CanPlaceBuilding(int tx, int ty, int wTiles, int hTiles) const {
    if (tx < 0 || ty < 0 || tx + wTiles > m_width || ty + hTiles > m_height) return false;

    // Check if tiles are clear and on land
    for (int y = ty; y < ty + hTiles; ++y) {
        for (int x = tx; x < tx + wTiles; ++x) {
            const MapTile& t = m_tiles[y * m_width + x];
            if (t.terrain == TerrainType::WATER || t.terrain == TerrainType::ROCKS) return false;
            if (t.buildingId != -1) return false;
        }
    }
    return true;
}

void Map::SetBuildingOccupation(int tx, int ty, int wTiles, int hTiles, int buildingId) {
    for (int y = ty; y < ty + hTiles; ++y) {
        for (int x = tx; x < tx + wTiles; ++x) {
            if (IsInBounds(x, y)) {
                m_tiles[y * m_width + x].buildingId = buildingId;
            }
        }
    }
}

void Map::ClearBuildingOccupation(int tx, int ty, int wTiles, int hTiles) {
    for (int y = ty; y < ty + hTiles; ++y) {
        for (int x = tx; x < tx + wTiles; ++x) {
            if (IsInBounds(x, y)) {
                m_tiles[y * m_width + x].buildingId = -1;
            }
        }
    }
}

void Map::BeginFogPass() {
    for (auto& t : m_tiles) {
        if (t.shroud == ShroudStatus::VISIBLE) {
            t.shroud = ShroudStatus::FOG;
        }
    }
}

void Map::RevealVision(float worldX, float worldY, float sightRadiusTiles) {
    glm::ivec2 center = WorldToTile(worldX, worldY);
    int r = int(std::ceil(sightRadiusTiles));
    int r2 = r * r;

    for (int dy = -r; dy <= r; ++dy) {
        for (int dx = -r; dx <= r; ++dx) {
            if (dx * dx + dy * dy <= r2) {
                int tx = center.x + dx;
                int ty = center.y + dy;
                if (IsInBounds(tx, ty)) {
                    m_tiles[ty * m_width + tx].shroud = ShroudStatus::VISIBLE;
                }
            }
        }
    }
}

ShroudStatus Map::GetShroudAt(int tx, int ty) const {
    if (!IsInBounds(tx, ty)) return ShroudStatus::SHROUDED;
    return m_tiles[ty * m_width + tx].shroud;
}

bool Map::IsVisibleWorld(float wx, float wy) const {
    glm::ivec2 t = WorldToTile(wx, wy);
    return GetShroudAt(t.x, t.y) == ShroudStatus::VISIBLE;
}

glm::ivec2 Map::FindNearestOre(float wx, float wy, float maxSearchDist) {
    glm::ivec2 start = WorldToTile(wx, wy);
    int maxRadius = int(maxSearchDist / TILE_H);

    glm::ivec2 bestTile(-1, -1);
    float bestDist = 1e9f;

    for (int dy = -maxRadius; dy <= maxRadius; ++dy) {
        for (int dx = -maxRadius; dx <= maxRadius; ++dx) {
            int tx = start.x + dx;
            int ty = start.y + dy;
            if (IsInBounds(tx, ty)) {
                const MapTile& t = m_tiles[ty * m_width + tx];
                if (t.terrain == TerrainType::ORE && t.oreAmount > 20.0f) {
                    float d = float(dx * dx + dy * dy);
                    if (d < bestDist) {
                        bestDist = d;
                        bestTile = glm::ivec2(tx, ty);
                    }
                }
            }
        }
    }
    return bestTile;
}

float Map::HarvestOre(int tx, int ty, float amountRequested) {
    if (!IsInBounds(tx, ty)) return 0.0f;
    MapTile& t = m_tiles[ty * m_width + tx];
    if (t.terrain != TerrainType::ORE) return 0.0f;

    float mined = std::min(t.oreAmount, amountRequested);
    t.oreAmount -= mined;
    if (t.oreAmount <= 5.0f) {
        t.terrain = TerrainType::DIRT;
        t.oreAmount = 0.0f;
    }
    return mined;
}

void Map::RegenerateOre(float dt) {
    m_oreRegenTimer += dt;
    if (m_oreRegenTimer >= 10.0f) {
        m_oreRegenTimer = 0.0f;
        // Slowly grow ore back on existing ore patches
        for (auto& t : m_tiles) {
            if (t.terrain == TerrainType::ORE && t.oreAmount < 1000.0f) {
                t.oreAmount = std::min(1000.0f, t.oreAmount + 40.0f);
            }
        }
    }
}

std::vector<glm::vec2> Map::FindPath(glm::vec2 startPos, glm::vec2 endPos) {
    glm::ivec2 startT = WorldToTile(startPos.x, startPos.y);
    glm::ivec2 endT = WorldToTile(endPos.x, endPos.y);

    if (!IsInBounds(startT.x, startT.y) || !IsInBounds(endT.x, endT.y)) {
        return { endPos };
    }

    if (startT == endT) {
        return { endPos };
    }

    // If destination is impassable, find nearest passable neighbor (up to radius 3)
    if (!IsPassable(endT.x, endT.y)) {
        bool found = false;
        float bestDist = 1e9f;
        glm::ivec2 bestPassable = endT;

        for (int r = 1; r <= 3 && !found; ++r) {
            for (int dy = -r; dy <= r; ++dy) {
                for (int dx = -r; dx <= r; ++dx) {
                    int nx = endT.x + dx;
                    int ny = endT.y + dy;
                    if (IsInBounds(nx, ny) && IsPassable(nx, ny)) {
                        float d = float(dx * dx + dy * dy);
                        if (d < bestDist) {
                            bestDist = d;
                            bestPassable = glm::ivec2(nx, ny);
                            found = true;
                        }
                    }
                }
            }
        }
        if (found) {
            endT = bestPassable;
        }
    }

    // A* Pathfinding
    int totalTiles = m_width * m_height;
    std::vector<float> gScore(totalTiles, 1e9f);
    std::vector<int> cameFrom(totalTiles, -1);
    std::vector<bool> closed(totalTiles, false);

    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> openSet;

    int startIndex = startT.y * m_width + startT.x;
    int endIndex = endT.y * m_width + endT.x;

    gScore[startIndex] = 0.0f;
    openSet.push({ startT.x, startT.y, 0.0f, Dist(startT.x, startT.y, endT.x, endT.y) });

    static const int DX[8] = { 1, -1, 0, 0, 1, -1, 1, -1 };
    static const int DY[8] = { 0, 0, 1, -1, 1, 1, -1, -1 };
    static const float COST[8] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.414f, 1.414f, 1.414f, 1.414f };

    int iterations = 0;
    bool pathFound = false;

    while (!openSet.empty() && iterations++ < 2500) {
        Node current = openSet.top();
        openSet.pop();

        int currIdx = current.y * m_width + current.x;
        if (currIdx == endIndex) {
            pathFound = true;
            break;
        }

        if (closed[currIdx]) continue;
        closed[currIdx] = true;

        for (int dir = 0; dir < 8; ++dir) {
            int nx = current.x + DX[dir];
            int ny = current.y + DY[dir];

            if (!IsInBounds(nx, ny)) continue;
            int nextIdx = ny * m_width + nx;
            if (closed[nextIdx]) continue;

            // Diagonal corner-cutting check
            if (dir >= 4) {
                if (!IsPassable(current.x + DX[dir], current.y) || !IsPassable(current.x, current.y + DY[dir])) {
                    continue;
                }
            }

            if (!IsPassable(nx, ny) && nextIdx != endIndex) continue;

            float tentativeG = gScore[currIdx] + COST[dir];
            if (tentativeG < gScore[nextIdx]) {
                cameFrom[nextIdx] = currIdx;
                gScore[nextIdx] = tentativeG;
                float f = tentativeG + Dist(nx, ny, endT.x, endT.y);
                openSet.push({ nx, ny, tentativeG, f });
            }
        }
    }

    std::vector<glm::vec2> waypoints;
    if (pathFound) {
        int curr = endIndex;
        while (curr != startIndex && curr != -1) {
            int cx = curr % m_width;
            int cy = curr / m_width;
            waypoints.push_back(TileCenterToWorld(cx, cy));
            curr = cameFrom[curr];
        }
        std::reverse(waypoints.begin(), waypoints.end());
        // Append exact destination point
        if (waypoints.empty() || glm::length(waypoints.back() - endPos) > 4.0f) {
            waypoints.push_back(endPos);
        }
    } else {
        // Fallback: direct line to destination
        waypoints.push_back(endPos);
    }

    return waypoints;
}

void Map::Render(Renderer& renderer, float camX, float camY, float halfW, float halfH) {
    // Render isometric diamond tiles ordered from top to bottom
    // Iterate through all map tiles and draw them in isometric order
    for (int ty = 0; ty < m_height; ++ty) {
        for (int tx = 0; tx < m_width; ++tx) {
            glm::vec2 isoPos = TileCenterToWorld(tx, ty);

            // Frustum cull
            if (isoPos.x < camX - halfW - 48.0f || isoPos.x > camX + halfW + 48.0f ||
                isoPos.y < camY - halfH - 32.0f || isoPos.y > camY + halfH + 32.0f) {
                continue;
            }

            const MapTile& t = m_tiles[ty * m_width + tx];

            if (t.shroud == ShroudStatus::SHROUDED) {
                renderer.DrawSpriteCentered(SpriteId::SHROUD_BLACK, isoPos.x, isoPos.y, TILE_W, TILE_H);
                continue;
            }

            SpriteId terrainSprite = SpriteId::TERRAIN_GRASS;
            switch (t.terrain) {
                case TerrainType::GRASS: terrainSprite = SpriteId::TERRAIN_GRASS; break;
                case TerrainType::DIRT:  terrainSprite = SpriteId::TERRAIN_DIRT;  break;
                case TerrainType::WATER: terrainSprite = SpriteId::TERRAIN_WATER; break;
                case TerrainType::ROCKS: terrainSprite = SpriteId::TERRAIN_ROCKS; break;
                case TerrainType::ORE:   terrainSprite = SpriteId::TERRAIN_ORE;   break;
            }

            renderer.DrawSpriteCentered(terrainSprite, isoPos.x, isoPos.y, TILE_W, TILE_H);

            if (t.shroud == ShroudStatus::FOG) {
                renderer.DrawSpriteCentered(SpriteId::SHROUD_FOG, isoPos.x, isoPos.y, TILE_W, TILE_H);
            }
        }
    }
}
