#ifndef PATHFINDER_H
#define PATHFINDER_H

#include <Arduino.h>
#include "config.h"

// A* pathfinding on a SLIDING 2D grid map
// Grid: 0=free, 1=obstacle, 2=unknown
//
// The grid is always centered on the robot. When the robot moves near
// the edge, the entire grid shifts to keep the robot near the center.
// This allows unlimited travel range while using fixed 20x20 memory.
//
// World coordinates (cm) are tracked separately from grid coordinates.
// Target is stored in world cm, then converted to grid-relative position.

struct GridPoint {
    int x;
    int y;
};

class Pathfinder {
public:
    uint8_t grid[GRID_SIZE][GRID_SIZE];   // occupancy grid (slides with robot)
    GridPoint robotPos;                    // robot position in grid coords
    GridPoint targetGrid;                  // target in grid coords (recalculated)
    GridPoint path[GRID_SIZE * GRID_SIZE]; // computed path
    int pathLength;

    // World-space tracking (in cm, independent of grid)
    float worldOriginX = 0;  // world X of grid cell (0,0) in cm
    float worldOriginY = 0;  // world Y of grid cell (0,0) in cm
    float targetWorldX = 0;  // target in world cm
    float targetWorldY = 0;
    bool  hasTarget = false;
    bool  targetReached = false;

    // Breadcrumb trail for backtracking (world cm)
    static const int MAX_CRUMBS = 50;
    float crumbX[MAX_CRUMBS];
    float crumbY[MAX_CRUMBS];
    int   crumbCount = 0;
    int   backtrackIdx = -1;  // -1 = not backtracking

    void begin() {
        clearMap();
        robotPos = {GRID_SIZE / 2, GRID_SIZE / 2};
        targetGrid = {GRID_SIZE / 2, 0};
        pathLength = 0;
        hasTarget = false;
        targetReached = false;
        crumbCount = 0;
        backtrackIdx = -1;
        worldOriginX = -(GRID_SIZE / 2) * CELL_SIZE_CM;
        worldOriginY = -(GRID_SIZE / 2) * CELL_SIZE_CM;
    }

    void clearMap() {
        for (int y = 0; y < GRID_SIZE; y++)
            for (int x = 0; x < GRID_SIZE; x++)
                grid[y][x] = 2; // unknown
    }

    // Set target in real-world cm (relative to robot start = 0,0)
    // FIX BUG 4: Does NOT reset backtrackIdx (backtrack uses this too)
    void setTargetWorld(float wx, float wy) {
        targetWorldX = wx;
        targetWorldY = wy;
        hasTarget = true;
        targetReached = false;
        // NOTE: backtrackIdx is NOT touched here - startBacktrack() manages it
        updateTargetGrid();
    }

    // Legacy grid-based target (for backward compat)
    void setTarget(int gx, int gy) {
        float wx = worldOriginX + gx * CELL_SIZE_CM;
        float wy = worldOriginY + gy * CELL_SIZE_CM;
        setTargetWorld(wx, wy);
    }

    // Update robot position from encoder world coordinates (cm)
    void updateRobotWorld(float worldX, float worldY) {
        // FIX BUG 7: Use floor() for correct negative coordinate mapping
        int gx = (int)floor((worldX - worldOriginX) / CELL_SIZE_CM);
        int gy = (int)floor((worldY - worldOriginY) / CELL_SIZE_CM);

        // Check if robot is near grid edge -> slide the grid
        const int EDGE_MARGIN = 4;

        if (gx < EDGE_MARGIN) {
            shiftGrid(-(GRID_SIZE / 2 - EDGE_MARGIN), 0);
        } else if (gx >= GRID_SIZE - EDGE_MARGIN) {
            shiftGrid(GRID_SIZE / 2 - EDGE_MARGIN, 0);
        }
        if (gy < EDGE_MARGIN) {
            shiftGrid(0, -(GRID_SIZE / 2 - EDGE_MARGIN));
        } else if (gy >= GRID_SIZE - EDGE_MARGIN) {
            shiftGrid(0, GRID_SIZE / 2 - EDGE_MARGIN);
        }

        // Recalculate grid position after any shift
        gx = (int)floor((worldX - worldOriginX) / CELL_SIZE_CM);
        gy = (int)floor((worldY - worldOriginY) / CELL_SIZE_CM);

        if (inBounds(gx, gy)) {
            robotPos = {gx, gy};
            grid[gy][gx] = 0; // mark as free
        }

        // Update target grid position
        if (hasTarget) {
            updateTargetGrid();
        }

        // Drop breadcrumb every ~20cm for backtracking
        if (crumbCount == 0 ||
            dist2D(worldX, worldY, crumbX[crumbCount - 1], crumbY[crumbCount - 1]) > 20.0) {
            dropCrumb(worldX, worldY);
        }

        // Check if target reached (within 15cm)
        if (hasTarget && dist2D(worldX, worldY, targetWorldX, targetWorldY) < 15.0) {
            targetReached = true;
        }
    }

    // Mark obstacle at world position from sensor reading
    void markObstacle(float distCm, float angleRad) {
        int cellDist = (int)(distCm / CELL_SIZE_CM);
        int ox = robotPos.x + (int)(cellDist * sin(angleRad));
        int oy = robotPos.y - (int)(cellDist * cos(angleRad));

        if (inBounds(ox, oy)) {
            grid[oy][ox] = 1;
        }
        markLineFree(robotPos.x, robotPos.y, ox, oy);
    }

    // Mark cells along a line as free (ray-casting)
    void markLineFree(int x0, int y0, int x1, int y1) {
        int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
        int dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
        int err = dx - dy;

        while (x0 != x1 || y0 != y1) {
            if (inBounds(x0, y0) && grid[y0][x0] != 1) {
                grid[y0][x0] = 0;
            }
            int e2 = 2 * err;
            if (e2 > -dy) { err -= dy; x0 += sx; }
            if (e2 < dx)  { err += dx; y0 += sy; }
        }
    }

    // Run A* pathfinding
    bool findPath() {
        pathLength = 0;

        if (!hasTarget || targetReached) return false;

        // If target is off-grid, pathfind toward the grid edge closest to target
        GridPoint goal = targetGrid;
        if (!inBounds(goal.x, goal.y)) {
            goal.x = constrain(goal.x, 0, GRID_SIZE - 1);
            goal.y = constrain(goal.y, 0, GRID_SIZE - 1);
        }

        struct Node {
            int x, y;
            int g, f;
            int parentIdx;
            bool valid;
        };

        static Node openList[100];
        static Node closedList[200];
        int openCount = 0;
        int closedCount = 0;

        static bool visited[GRID_SIZE][GRID_SIZE];
        for (int y = 0; y < GRID_SIZE; y++)
            for (int x = 0; x < GRID_SIZE; x++)
                visited[y][x] = false;

        openList[openCount++] = {
            robotPos.x, robotPos.y,
            0, heuristic(robotPos.x, robotPos.y, goal),
            -1, true
        };
        visited[robotPos.y][robotPos.x] = true;

        const int dx[] = {0, 0, -1, 1};
        const int dy[] = {-1, 1, 0, 0};

        while (openCount > 0) {
            int bestIdx = 0;
            for (int i = 1; i < openCount; i++) {
                if (openList[i].f < openList[bestIdx].f)
                    bestIdx = i;
            }

            Node current = openList[bestIdx];
            openList[bestIdx] = openList[--openCount];

            int currentClosed = closedCount;
            if (closedCount < 200) {
                closedList[closedCount++] = current;
            } else {
                return false;
            }

            if (current.x == goal.x && current.y == goal.y) {
                int idx = currentClosed;
                int tempPath[200];
                int tempLen = 0;

                while (idx >= 0 && tempLen < 200) {
                    tempPath[tempLen++] = idx;
                    idx = closedList[idx].parentIdx;
                }

                pathLength = 0;
                for (int i = tempLen - 1; i >= 0 && pathLength < GRID_SIZE * GRID_SIZE; i--) {
                    path[pathLength++] = {
                        closedList[tempPath[i]].x,
                        closedList[tempPath[i]].y
                    };
                }
                return true;
            }

            for (int d = 0; d < 4; d++) {
                int nx = current.x + dx[d];
                int ny = current.y + dy[d];

                if (!inBounds(nx, ny)) continue;
                if (visited[ny][nx]) continue;
                if (grid[ny][nx] == 1) continue;

                visited[ny][nx] = true;
                int g = current.g + 1;
                int f = g + heuristic(nx, ny, goal);

                if (openCount < 100) {
                    openList[openCount++] = {nx, ny, g, f, currentClosed, true};
                }
            }
        }

        return false;
    }

    // Get next direction: 0=forward, -1=left, 1=right, -2=arrived
    int getNextDirection(float headingRad) {
        if (targetReached) return -2;
        if (pathLength < 2) return -2;

        GridPoint next = path[1];
        int dx = next.x - robotPos.x;
        int dy = next.y - robotPos.y;

        float targetAngle = atan2(dx, -dy);
        float diff = targetAngle - headingRad;

        while (diff > PI)  diff -= 2 * PI;
        while (diff < -PI) diff += 2 * PI;

        if (abs(diff) < 0.3) return 0;
        if (diff > 0) return 1;
        return -1;
    }

    // --- BACKTRACKING ---

    // Start backtracking to origin (follows breadcrumbs in reverse)
    void startBacktrack() {
        if (crumbCount == 0) return;
        backtrackIdx = crumbCount - 1;
        hasTarget = true;
        targetReached = false;
        // Set first backtrack waypoint
        setTargetWorld(crumbX[backtrackIdx], crumbY[backtrackIdx]);
    }

    // Call during navigation to advance through breadcrumbs
    // Returns true if still backtracking, false if back at origin
    bool updateBacktrack(float robotWorldX, float robotWorldY) {
        if (backtrackIdx < 0) return false;

        // Check if we reached current breadcrumb
        if (dist2D(robotWorldX, robotWorldY, crumbX[backtrackIdx], crumbY[backtrackIdx]) < 15.0) {
            backtrackIdx--;
            if (backtrackIdx < 0) {
                // Reached origin
                hasTarget = false;
                targetReached = true;
                return false;
            }
            // Set next breadcrumb as target
            setTargetWorld(crumbX[backtrackIdx], crumbY[backtrackIdx]);
        }
        return true;
    }

    bool isBacktracking() { return backtrackIdx >= 0; }

    // Legacy compat
    void updateRobotPos(int gx, int gy) {
        if (inBounds(gx, gy)) {
            robotPos = {gx, gy};
            grid[gy][gx] = 0;
        }
    }

private:
    bool inBounds(int x, int y) {
        return x >= 0 && x < GRID_SIZE && y >= 0 && y < GRID_SIZE;
    }

    int heuristic(int x, int y, GridPoint goal) {
        return abs(x - goal.x) + abs(y - goal.y);
    }

    float dist2D(float x1, float y1, float x2, float y2) {
        float dx = x1 - x2;
        float dy = y1 - y2;
        return sqrt(dx * dx + dy * dy);
    }

    // Convert target world coords to grid coords
    void updateTargetGrid() {
        targetGrid.x = (int)floor((targetWorldX - worldOriginX) / CELL_SIZE_CM);
        targetGrid.y = (int)floor((targetWorldY - worldOriginY) / CELL_SIZE_CM);
    }

    // Shift the grid by (shiftX, shiftY) cells
    // Positive shiftX = grid moves right (robot was near right edge)
    // FIX BUG 8: static temp array to avoid 400 bytes on stack
    void shiftGrid(int shiftX, int shiftY) {
        static uint8_t temp[GRID_SIZE][GRID_SIZE];

        // Copy shifted data
        for (int y = 0; y < GRID_SIZE; y++) {
            for (int x = 0; x < GRID_SIZE; x++) {
                int srcX = x + shiftX;
                int srcY = y + shiftY;
                if (srcX >= 0 && srcX < GRID_SIZE && srcY >= 0 && srcY < GRID_SIZE) {
                    temp[y][x] = grid[srcY][srcX];
                } else {
                    temp[y][x] = 2; // unknown for new cells
                }
            }
        }

        // Copy back
        memcpy(grid, temp, sizeof(grid));

        // Update world origin
        worldOriginX += shiftX * CELL_SIZE_CM;
        worldOriginY += shiftY * CELL_SIZE_CM;
    }

    // Drop a breadcrumb for backtracking
    void dropCrumb(float wx, float wy) {
        if (crumbCount < MAX_CRUMBS) {
            crumbX[crumbCount] = wx;
            crumbY[crumbCount] = wy;
            crumbCount++;
        } else {
            // Shift array: drop oldest, keep newest
            for (int i = 0; i < MAX_CRUMBS - 1; i++) {
                crumbX[i] = crumbX[i + 1];
                crumbY[i] = crumbY[i + 1];
            }
            crumbX[MAX_CRUMBS - 1] = wx;
            crumbY[MAX_CRUMBS - 1] = wy;
        }
    }
};

#endif // PATHFINDER_H
