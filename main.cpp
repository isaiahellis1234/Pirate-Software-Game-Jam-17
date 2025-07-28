#include <vector>
#include <cmath>
#include <iostream>
#include <unordered_map>
#include <algorithm>
#include <raylib.h>

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int CELL_SIZE = 32;
const int health = 1000;

struct CellCoord {
    int x, y;
    bool operator==(const CellCoord& other) const { return x == other.x && y == other.y; }
};

namespace std {
    template<> struct hash<CellCoord> {
        size_t operator()(const CellCoord& c) const {
            return hash<int>()(c.x) ^ (hash<int>()(c.y) << 1);
        }
    };
}

class red; // Forward declaration

class blue {
public:
    float x, y;
    float radius = 2.0f;
    Vector2 pos;
    int targetIndex = -1;
    std::vector<red>* redList = nullptr;
    int health = health + rand() % 11;

    void Update();
    void Draw();
    void Separate(const std::vector<blue*>& nearby, float minDist = 12.0f);
    CellCoord GetCell() const;
    bool IsAlive() const { return health > 0; }
};

class red {
public:
    float x, y;
    float radius = 2.0f;
    Vector2 pos;
    int targetIndex = -1;
    std::vector<blue>* blueList = nullptr;
    int health = health + rand() % 11;

    void Update();
    void Draw();
    void Separate(const std::vector<red*>& nearby, float minDist = 12.0f);
    CellCoord GetCell() const;
    bool IsAlive() const { return health > 0; }
};

CellCoord red::GetCell() const {
    return { (int)(x / CELL_SIZE), (int)(y / CELL_SIZE) };
}

CellCoord blue::GetCell() const {
    return { (int)(x / CELL_SIZE), (int)(y / CELL_SIZE) };
}

void red::Update() {
    if (blueList && targetIndex >= 0 && targetIndex < (int)blueList->size()) {
        blue& target = (*blueList)[targetIndex];
        float dx = target.x - x;
        float dy = target.y - y;
        float dist = dx * dx + dy * dy;
        if (dist > 1.0f) {
            float invDist = 1.0f / sqrtf(dist);
            x += dx * invDist;
            y += dy * invDist;
            pos = {x, y};
        }
    }
}

void red::Draw() {
    DrawCircle((int)x, (int)y, radius, RED);
}

void red::Separate(const std::vector<red*>& nearby, float minDist) {
    float moveX = 0, moveY = 0;
    int count = 0;
    float minDistSq = minDist * minDist;
    for (const auto* other : nearby) {
        if (other == this) continue;
        float dx = x - other->x;
        float dy = y - other->y;
        float distSq = dx * dx + dy * dy;
        if (distSq < minDistSq && distSq > 0.01f) {
            float dist = sqrtf(distSq);
            moveX += dx / dist;
            moveY += dy / dist;
            count++;
        }
    }
    if (count > 0) {
        x += moveX;
        y += moveY;
        pos = {x, y};
    }
}

void blue::Update() {
    if (redList && targetIndex >= 0 && targetIndex < (int)redList->size()) {
        red& target = (*redList)[targetIndex];
        float dx = target.x - x;
        float dy = target.y - y;
        float dist = dx * dx + dy * dy;
        if (dist > 1.0f) {
            float invDist = 1.0f / sqrtf(dist);
            x += dx * invDist;
            y += dy * invDist;
            pos = {x, y};
        }
    }
}

void blue::Draw() {
    DrawCircle((int)x, (int)y, radius, BLUE);
}

void blue::Separate(const std::vector<blue*>& nearby, float minDist) {
    float moveX = 0, moveY = 0;
    int count = 0;
    float minDistSq = minDist * minDist;
    for (const auto* other : nearby) {
        if (other == this) continue;
        float dx = x - other->x;
        float dy = y - other->y;
        float distSq = dx * dx + dy * dy;
        if (distSq < minDistSq && distSq > 0.01f) {
            float dist = sqrtf(distSq);
            moveX += dx / dist;
            moveY += dy / dist;
            count++;
        }
    }
    if (count > 0) {
        x += moveX;
        y += moveY;
        pos = {x, y};
    }
}


int main() {
    bool paused = false;
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Red vs Blue");
    SetTargetFPS(60);

    Camera2D camera = {0};
    camera.target = {SCREEN_WIDTH/2.0f, SCREEN_HEIGHT/2.0f};
    camera.offset = {SCREEN_WIDTH/2.0f, SCREEN_HEIGHT/2.0f};
    camera.zoom = 1.0f;

    int num = 0;
    char numInput[8] = "";
    int numInputLen = 0;
    bool numInputActive = false;
    int userTeam = 0;
    bool inMenu = true;
    bool simulationOver = false;
    bool userWon = false;
    double endTime = 0;
    int secondsLeft = 0;

    std::vector<red> redCircles;
    std::vector<blue> blueCircles;
    int frame = 0;

    while (!WindowShouldClose()) {
        // Toggle pause
        if (!inMenu && !simulationOver && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) paused = !paused;
        if (inMenu) {
            BeginDrawing();
            ClearBackground(DARKGRAY);
            DrawText("Choose Your Team", SCREEN_WIDTH/2 - 130, 80, 32, YELLOW);
            Rectangle redBtn = {SCREEN_WIDTH/2 - 150, 150, 120, 60};
            Rectangle blueBtn = {SCREEN_WIDTH/2 + 30, 150, 120, 60};
            DrawRectangleRec(redBtn, RED);
            DrawRectangleRec(blueBtn, BLUE);
            DrawText("RED", redBtn.x + 30, redBtn.y + 15, 28, WHITE);
            DrawText("BLUE", blueBtn.x + 20, blueBtn.y + 15, 28, WHITE);
            DrawText("Enter team size:", SCREEN_WIDTH/2 - 110, 250, 20, LIGHTGRAY);
            Rectangle inputBox = {SCREEN_WIDTH/2 - 60, 280, 120, 40};
            DrawRectangleRec(inputBox, numInputActive ? LIGHTGRAY : BLACK);
            DrawText(numInput, SCREEN_WIDTH/2 - 50, 290, 24, WHITE);
            DrawText("Anything > 1000 may slow down your computer", SCREEN_WIDTH/2 - 350, 330, 30, LIGHTGRAY);
            // Draw blinking cursor if active
            if (numInputActive && ((GetTime()*2)) - (int)(GetTime()*2) < 0.5) {
                int textWidth = MeasureText(numInput, 24);
                DrawRectangle(SCREEN_WIDTH/2 - 50 + textWidth + 2, 292, 12, 24, WHITE);
            }
            // Mouse click to activate input
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                Vector2 mouse = GetMousePosition();
                if (CheckCollisionPointRec(mouse, inputBox)) numInputActive = true;
                else numInputActive = false;
                if (CheckCollisionPointRec(mouse, redBtn)) userTeam = 1;
                if (CheckCollisionPointRec(mouse, blueBtn)) userTeam = 2;
            }
            // Handle keyboard input for number box
            if (numInputActive) {
                int key = GetCharPressed();
                while (key > 0) {
                    if (key >= '0' && key <= '9' && numInputLen < 6) {
                        numInput[numInputLen++] = (char)key;
                        numInput[numInputLen] = '\0';
                    } else if (key == KEY_BACKSPACE && numInputLen > 0) {
                        numInput[--numInputLen] = '\0';
                    }
                    key = GetCharPressed();
                }
            }
            // Allow backspace (for some platforms GetCharPressed doesn't catch it)
            if (numInputActive && IsKeyPressed(KEY_BACKSPACE) && numInputLen > 0) {
                numInput[--numInputLen] = '\0';
            }
            // Convert input to int
            num = atoi(numInput);
            if (userTeam && num > 0) {
                redCircles.reserve(num);
                blueCircles.reserve(num);
                for (int i = 0; i < num; i++) {
                    red r;
                    r.x = 100 + (rand() % 51 - 25);
                    r.y = 75 + rand() % 451;
                    r.pos = {r.x, r.y};
                    r.targetIndex = i;
                    redCircles.push_back(r);
                    blue b;
                    b.x = 700 + (rand() % 51 - 25);
                    b.y = 75 + rand() % 451;
                    b.pos = {b.x, b.y};
                    b.targetIndex = i;
                    blueCircles.push_back(b);
                }
                for (auto& r : redCircles) r.blueList = &blueCircles;
                for (auto& b : blueCircles) b.redList = &redCircles;
                inMenu = false;
            }
            EndDrawing();
            continue;
        }

        if (simulationOver) {
            if (endTime == 0) {
                endTime = GetTime() + 5.0;
            }
            secondsLeft = (int)(endTime - GetTime()) + 1;
            if (secondsLeft < 0) secondsLeft = 0;
            BeginDrawing();
            ClearBackground(BLACK);
            DrawText(userWon ? "You Win!" : "You Lose!", SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 - 40, 40, userWon ? GREEN : RED);
            DrawText(TextFormat("Restarting in %d...", secondsLeft), SCREEN_WIDTH/2 - 120, SCREEN_HEIGHT/2 + 10, 32, YELLOW);
            EndDrawing();
            if (GetTime() >= endTime) {
                // Reset all state for a new game
                num = 0;
                numInput[0] = '\0';
                numInputLen = 0;
                numInputActive = false;
                userTeam = 0;
                redCircles.clear();
                blueCircles.clear();
                inMenu = true;
                simulationOver = false;
                userWon = false;
                endTime = 0;
                frame = 0;
            }
            continue;
        }

        // Camera panning (WASD/arrows) always allowed outside menu
        if (!inMenu) {
            float camSpeed = 10.0f / camera.zoom;
            if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) camera.target.x += camSpeed;
            if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  camera.target.x -= camSpeed;
            if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))  camera.target.y += camSpeed;
            if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))    camera.target.y -= camSpeed;
        }

        float wheel = GetMouseWheelMove();
        if (wheel != 0) camera.zoom += wheel * 0.1f;
        if (camera.zoom < 0.1f) camera.zoom = 0.1f;

        if (!paused) {
            frame++;
            std::unordered_map<CellCoord, std::vector<red*>> redGrid;
            std::unordered_map<CellCoord, std::vector<blue*>> blueGrid;

            for (auto& r : redCircles) redGrid[r.GetCell()].push_back(&r);
            for (auto& b : blueCircles) blueGrid[b.GetCell()].push_back(&b);

            for (auto& r : redCircles) r.Update();
            for (auto& b : blueCircles) b.Update();

            for (auto& r : redCircles) {
                CellCoord c = r.GetCell();
                for (int dx = -1; dx <= 1; dx++) {
                    for (int dy = -1; dy <= 1; dy++) {
                        CellCoord n = {c.x + dx, c.y + dy};
                        auto it = blueGrid.find(n);
                        if (it != blueGrid.end()) {
                            for (auto* b : it->second) {
                                float dx = r.x - b->x;
                                float dy = r.y - b->y;
                                float distSq = dx * dx + dy * dy;
                                float minDist = r.radius + b->radius;
                                if (distSq < minDist * minDist) {
                                    r.health -= 1 + rand() % 5;
                                    b->health -= 1 + rand() % 5;
                                }
                            }
                        }
                    }
                }
            }

            redCircles.erase(std::remove_if(redCircles.begin(), redCircles.end(), [](const red& r){ return !r.IsAlive(); }), redCircles.end());
            blueCircles.erase(std::remove_if(blueCircles.begin(), blueCircles.end(), [](const blue& b){ return !b.IsAlive(); }), blueCircles.end());

            for (auto& r : redCircles) r.targetIndex = blueCircles.empty() ? -1 : rand() % blueCircles.size();
            for (auto& b : blueCircles) b.targetIndex = redCircles.empty() ? -1 : rand() % redCircles.size();

            if (frame % 2 == 0) {
                for (auto& r : redCircles) {
                    std::vector<red*> neighbors;
                    CellCoord c = r.GetCell();
                    for (int dx = -1; dx <= 1; dx++)
                        for (int dy = -1; dy <= 1; dy++) {
                            auto it = redGrid.find({c.x + dx, c.y + dy});
                            if (it != redGrid.end())
                                neighbors.insert(neighbors.end(), it->second.begin(), it->second.end());
                        }
                    r.Separate(neighbors);
                }
            } else {
                for (auto& b : blueCircles) {
                    std::vector<blue*> neighbors;
                    CellCoord c = b.GetCell();
                    for (int dx = -1; dx <= 1; dx++)
                        for (int dy = -1; dy <= 1; dy++) {
                            auto it = blueGrid.find({c.x + dx, c.y + dy});
                            if (it != blueGrid.end())
                                neighbors.insert(neighbors.end(), it->second.begin(), it->second.end());
                        }
                    b.Separate(neighbors);
                }
            }
        }

        if (!paused && (redCircles.empty() || blueCircles.empty())) {
            simulationOver = true;
            userWon = (userTeam == 1 && !redCircles.empty()) || (userTeam == 2 && !blueCircles.empty());
            endTime = 0; // Will be set on next simulationOver frame
            continue;
        }

        if (frame % 2 == 0) {
            BeginDrawing();
            ClearBackground(Color{89, 21, 43, 255});
            BeginMode2D(camera);
            Rectangle camView = {
                camera.target.x - SCREEN_WIDTH / (2 * camera.zoom),
                camera.target.y - SCREEN_HEIGHT / (2 * camera.zoom),
                SCREEN_WIDTH / camera.zoom,
                SCREEN_HEIGHT / camera.zoom
            };
            for (auto& r : redCircles) if (CheckCollisionPointRec({r.x, r.y}, camView)) r.Draw();
            for (auto& b : blueCircles) if (CheckCollisionPointRec({b.x, b.y}, camView)) b.Draw();
            EndMode2D();
            DrawFPS(10, 10);
            if (paused) DrawText("PAUSED (P)", 20, 60, 32, YELLOW);
            EndDrawing();
        }
    }

    CloseWindow();
    return 0;
}
