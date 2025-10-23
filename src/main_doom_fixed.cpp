#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <vector>
#include <random>
#include <ctime>
#include <cmath>
#include <algorithm>
#include <iostream>

// ===========================================
// DOOM-STYLE RAYCASTER WITH PROPER COLLISION
// Based on id Software's original DOOM engine
// ===========================================

constexpr unsigned int SCREEN_WIDTH = 1280;
constexpr unsigned int SCREEN_HEIGHT = 720;
constexpr int MAP_WIDTH = 64;
constexpr int MAP_HEIGHT = 64;

// Movement constants (from DOOM's p_local.h and p_user.c)
constexpr double MOVE_SPEED = 5.0;      // Units per second (like DOOM's forwardmove)
constexpr double STRAFE_SPEED = 4.5;    // Slightly slower strafe
constexpr double ROT_SPEED = 3.0;       // Radians per second
constexpr double PLAYER_RADIUS = 0.3;   // Collision radius
constexpr double FRICTION = 0.90;       // Momentum decay

enum class TileType { Empty = 0, Wall = 1 };
enum class EnemyType { Wolf, SmokeDemon, TophatOgre, RedDemon };

// Player struct (based on DOOM's player_t and mobj_t)
struct Player {
    double posX, posY;          // Position (like mobj_t->x, y)
    double dirX, dirY;          // Direction vector (derived from angle)
    double planeX, planeY;      // Camera plane (FOV)
    double momX, momY;          // Momentum (like mobj_t->momx, momy)
    int health;
    
    Player(double x, double y) 
        : posX(x), posY(y), dirX(-1), dirY(0), planeX(0), planeY(0.66),
          momX(0), momY(0), health(100) {}
};

// Enemy (billboard sprite)
struct Enemy {
    double x, y;
    EnemyType type;
    int health;
    float speed;
    const sf::Texture* texture;
    sf::IntRect textureRect;
    
    Enemy(double px, double py, EnemyType t, int hp, float spd, 
          const sf::Texture* tex, sf::IntRect rect)
        : x(px), y(py), type(t), health(hp), speed(spd), 
          texture(tex), textureRect(rect) {}
};

// Projectile
struct Projectile {
    double x, y, dirX, dirY;
    float speed;
    const sf::Texture* texture;
    
    Projectile(double px, double py, double dx, double dy, const sf::Texture* tex)
        : x(px), y(py), dirX(dx), dirY(dy), speed(10.0f), texture(tex) {}
};

// Blood particle
struct BloodParticle {
    double x, y, z, velX, velY, velZ;
    sf::Clock lifeClock;
    float lifetime;
    int frameIndex;
    
    BloodParticle(double px, double py, double vx, double vy)
        : x(px), y(py), z(0.5), velX(vx), velY(vy), velZ(0.5), 
          lifetime(0.5f), frameIndex(0) {}
};

// ===========================================
// COLLISION DETECTION (DOOM-style)
// Based on P_CheckPosition and P_TryMove
// ===========================================

bool checkCollision(const std::vector<std::vector<TileType>>& map, 
                    double x, double y, double radius) {
    // Check 4 corners of bounding box
    double corners[4][2] = {
        {x - radius, y - radius},
        {x + radius, y - radius},
        {x - radius, y + radius},
        {x + radius, y + radius}
    };
    
    for (int i = 0; i < 4; i++) {
        int mapX = static_cast<int>(corners[i][0]);
        int mapY = static_cast<int>(corners[i][1]);
        
        if (mapX < 0 || mapX >= MAP_WIDTH || mapY < 0 || mapY >= MAP_HEIGHT) {
            return true; // Out of bounds = collision
        }
        
        if (map[mapY][mapX] == TileType::Wall) {
            return true;
        }
    }
    
    return false;
}

// DOOM-style slide move: try to move, if blocked, slide along wall
void tryMoveWithSlide(Player& player, 
                      const std::vector<std::vector<TileType>>& map,
                      double targetX, double targetY) {
    // Try full movement first
    if (!checkCollision(map, targetX, targetY, PLAYER_RADIUS)) {
        player.posX = targetX;
        player.posY = targetY;
        return;
    }
    
    // Blocked! Try sliding along X axis only (DOOM's P_SlideMove logic)
    if (!checkCollision(map, targetX, player.posY, PLAYER_RADIUS)) {
        player.posX = targetX;
        return;
    }
    
    // Try sliding along Y axis only
    if (!checkCollision(map, player.posX, targetY, PLAYER_RADIUS)) {
        player.posY = targetY;
        return;
    }
    
    // Completely blocked, don't move
}

// ===========================================
// PLAYER MOVEMENT (DOOM's P_MovePlayer + P_Thrust)
// ===========================================

void updatePlayerMovement(Player& player, 
                          const std::vector<std::vector<TileType>>& map,
                          float deltaTime) {
    // Apply input forces (like P_Thrust in p_user.c)
    bool moving = false;
    
    // Forward/backward (forwardmove)
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) {
        player.momX += player.dirX * MOVE_SPEED * deltaTime;
        player.momY += player.dirY * MOVE_SPEED * deltaTime;
        moving = true;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) {
        player.momX -= player.dirX * MOVE_SPEED * deltaTime;
        player.momY -= player.dirY * MOVE_SPEED * deltaTime;
        moving = true;
    }
    
    // Strafe (sidemove)
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) {
        player.momX += player.planeX * STRAFE_SPEED * deltaTime;
        player.momY += player.planeY * STRAFE_SPEED * deltaTime;
        moving = true;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) {
        player.momX -= player.planeX * STRAFE_SPEED * deltaTime;
        player.momY -= player.planeY * STRAFE_SPEED * deltaTime;
        moving = true;
    }
    
    // Apply friction (like P_XYMovement)
    if (!moving) {
        player.momX *= FRICTION;
        player.momY *= FRICTION;
        
        // Stop if very slow
        if (std::abs(player.momX) < 0.001) player.momX = 0;
        if (std::abs(player.momY) < 0.001) player.momY = 0;
    }
    
    // Try to move with momentum (P_TryMove + P_SlideMove)
    double targetX = player.posX + player.momX * deltaTime;
    double targetY = player.posY + player.momY * deltaTime;
    
    tryMoveWithSlide(player, map, targetX, targetY);
    
    // Rotation (angle adjustment)
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) {
        double oldDirX = player.dirX;
        double rotAngle = ROT_SPEED * deltaTime;
        player.dirX = player.dirX * std::cos(rotAngle) - player.dirY * std::sin(rotAngle);
        player.dirY = oldDirX * std::sin(rotAngle) + player.dirY * std::cos(rotAngle);
        
        double oldPlaneX = player.planeX;
        player.planeX = player.planeX * std::cos(rotAngle) - player.planeY * std::sin(rotAngle);
        player.planeY = oldPlaneX * std::sin(rotAngle) + player.planeY * std::cos(rotAngle);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) {
        double oldDirX = player.dirX;
        double rotAngle = -ROT_SPEED * deltaTime;
        player.dirX = player.dirX * std::cos(rotAngle) - player.dirY * std::sin(rotAngle);
        player.dirY = oldDirX * std::sin(rotAngle) + player.dirY * std::cos(rotAngle);
        
        double oldPlaneX = player.planeX;
        player.planeX = player.planeX * std::cos(rotAngle) - player.planeY * std::sin(rotAngle);
        player.planeY = oldPlaneX * std::sin(rotAngle) + player.planeY * std::cos(rotAngle);
    }
}

// ===========================================
// DUNGEON GENERATION
// ===========================================

struct Room {
    int x, y, w, h;
};

void generateDungeon(std::vector<std::vector<TileType>>& map) {
    // Fill with walls
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            map[y][x] = TileType::Wall;
        }
    }
    
    std::mt19937 rng(static_cast<unsigned>(std::time(nullptr)));
    std::uniform_int_distribution<int> roomCountDist(15, 25);
    std::uniform_int_distribution<int> roomSizeDist(4, 10);
    
    std::vector<Room> rooms;
    int numRooms = roomCountDist(rng);
    
    // Create rooms
    for (int i = 0; i < numRooms * 2; i++) { // Try more times
        int w = roomSizeDist(rng);
        int h = roomSizeDist(rng);
        int x = std::uniform_int_distribution<int>(2, MAP_WIDTH - w - 2)(rng);
        int y = std::uniform_int_distribution<int>(2, MAP_HEIGHT - h - 2)(rng);
        
        // Check overlap
        bool overlap = false;
        for (const auto& room : rooms) {
            if (!(x + w < room.x || x > room.x + room.w ||
                  y + h < room.y || y > room.y + room.h)) {
                overlap = true;
                break;
            }
        }
        
        if (!overlap) {
            rooms.push_back({x, y, w, h});
            
            // Carve room
            for (int ry = y; ry < y + h; ry++) {
                for (int rx = x; rx < x + w; rx++) {
                    map[ry][rx] = TileType::Empty;
                }
            }
            
            if (rooms.size() >= static_cast<size_t>(numRooms)) break;
        }
    }
    
    // Connect rooms with corridors
    for (size_t i = 1; i < rooms.size(); i++) {
        int x1 = rooms[i-1].x + rooms[i-1].w / 2;
        int y1 = rooms[i-1].y + rooms[i-1].h / 2;
        int x2 = rooms[i].x + rooms[i].w / 2;
        int y2 = rooms[i].y + rooms[i].h / 2;
        
        // Horizontal corridor
        for (int x = std::min(x1, x2); x <= std::max(x1, x2); x++) {
            if (y1 >= 0 && y1 < MAP_HEIGHT && x >= 0 && x < MAP_WIDTH) {
                map[y1][x] = TileType::Empty;
                // Widen corridor
                if (y1 + 1 < MAP_HEIGHT) map[y1 + 1][x] = TileType::Empty;
            }
        }
        
        // Vertical corridor
        for (int y = std::min(y1, y2); y <= std::max(y1, y2); y++) {
            if (y >= 0 && y < MAP_HEIGHT && x2 >= 0 && x2 < MAP_WIDTH) {
                map[y][x2] = TileType::Empty;
                // Widen corridor
                if (x2 + 1 < MAP_WIDTH) map[y][x2 + 1] = TileType::Empty;
            }
        }
    }
    
    std::cout << "Generated " << rooms.size() << " rooms\n";
}

bool findEmptySpot(const std::vector<std::vector<TileType>>& map, int& x, int& y) {
    std::mt19937 rng(static_cast<unsigned>(std::time(nullptr)) + x + y);
    for (int attempts = 0; attempts < 100; attempts++) {
        x = std::uniform_int_distribution<int>(2, MAP_WIDTH - 3)(rng);
        y = std::uniform_int_distribution<int>(2, MAP_HEIGHT - 3)(rng);
        
        if (map[y][x] == TileType::Empty) {
            return true;
        }
    }
    return false;
}

// ===========================================
// RAYCASTING (DOOM's R_RenderPlayerView)
// ===========================================

void renderRaycaster(sf::RenderWindow& window, 
                     const Player& player,
                     const std::vector<std::vector<TileType>>& map,
                     const std::vector<Enemy>& enemies,
                     const sf::Texture& wallTexture) {
    
    // Draw ceiling (gray)
    sf::RectangleShape ceiling({static_cast<float>(SCREEN_WIDTH), 
                                 static_cast<float>(SCREEN_HEIGHT / 2)});
    ceiling.setPosition({0.f, 0.f});
    ceiling.setFillColor(sf::Color(60, 60, 60));
    window.draw(ceiling);
    
    // Draw floor (darker gray)
    sf::RectangleShape floor({static_cast<float>(SCREEN_WIDTH), 
                               static_cast<float>(SCREEN_HEIGHT / 2)});
    floor.setPosition({0.f, static_cast<float>(SCREEN_HEIGHT / 2)});
    floor.setFillColor(sf::Color(40, 40, 40));
    window.draw(floor);
    
    // Z-buffer for sprite depth sorting
    std::vector<double> zBuffer(SCREEN_WIDTH, 1e30);
    
    // Cast rays for walls (DDA algorithm from r_main.c)
    for (int x = 0; x < static_cast<int>(SCREEN_WIDTH); x++) {
        double cameraX = 2 * x / static_cast<double>(SCREEN_WIDTH) - 1;
        double rayDirX = player.dirX + player.planeX * cameraX;
        double rayDirY = player.dirY + player.planeY * cameraX;
        
        int mapX = static_cast<int>(player.posX);
        int mapY = static_cast<int>(player.posY);
        
        double deltaDistX = (rayDirX == 0) ? 1e30 : std::abs(1 / rayDirX);
        double deltaDistY = (rayDirY == 0) ? 1e30 : std::abs(1 / rayDirY);
        
        double sideDistX, sideDistY;
        int stepX, stepY;
        
        if (rayDirX < 0) {
            stepX = -1;
            sideDistX = (player.posX - mapX) * deltaDistX;
        } else {
            stepX = 1;
            sideDistX = (mapX + 1.0 - player.posX) * deltaDistX;
        }
        
        if (rayDirY < 0) {
            stepY = -1;
            sideDistY = (player.posY - mapY) * deltaDistY;
        } else {
            stepY = 1;
            sideDistY = (mapY + 1.0 - player.posY) * deltaDistY;
        }
        
        // DDA
        int side = 0;
        bool hit = false;
        while (!hit) {
            if (sideDistX < sideDistY) {
                sideDistX += deltaDistX;
                mapX += stepX;
                side = 0;
            } else {
                sideDistY += deltaDistY;
                mapY += stepY;
                side = 1;
            }
            
            if (mapX < 0 || mapX >= MAP_WIDTH || mapY < 0 || mapY >= MAP_HEIGHT) {
                hit = true;
            } else if (map[mapY][mapX] == TileType::Wall) {
                hit = true;
            }
        }
        
        // Calculate distance
        double perpWallDist;
        if (side == 0) {
            perpWallDist = (mapX - player.posX + (1 - stepX) / 2) / rayDirX;
        } else {
            perpWallDist = (mapY - player.posY + (1 - stepY) / 2) / rayDirY;
        }
        
        if (perpWallDist < 0.1) perpWallDist = 0.1;
        zBuffer[x] = perpWallDist;
        
        // Calculate wall height
        int lineHeight = static_cast<int>(SCREEN_HEIGHT / perpWallDist);
        int drawStart = -lineHeight / 2 + SCREEN_HEIGHT / 2;
        int drawEnd = lineHeight / 2 + SCREEN_HEIGHT / 2;
        
        if (drawStart < 0) drawStart = 0;
        if (drawEnd >= static_cast<int>(SCREEN_HEIGHT)) drawEnd = SCREEN_HEIGHT - 1;
        
        // Draw wall slice with shading
        sf::RectangleShape wallSlice({1, static_cast<float>(drawEnd - drawStart)});
        wallSlice.setPosition({static_cast<float>(x), static_cast<float>(drawStart)});
        
        sf::Color wallColor = (side == 1) ? sf::Color(100, 100, 100) : sf::Color(150, 150, 150);
        
        // Distance fog
        double fogFactor = std::min(1.0, perpWallDist / 20.0);
        wallColor.r = static_cast<std::uint8_t>(wallColor.r * (1.0 - fogFactor * 0.7));
        wallColor.g = static_cast<std::uint8_t>(wallColor.g * (1.0 - fogFactor * 0.7));
        wallColor.b = static_cast<std::uint8_t>(wallColor.b * (1.0 - fogFactor * 0.7));
        
        wallSlice.setFillColor(wallColor);
        window.draw(wallSlice);
    }
    
    // Draw enemies (billboard sprites - from r_things.c R_ProjectSprite)
    for (const auto& enemy : enemies) {
        double spriteX = enemy.x - player.posX;
        double spriteY = enemy.y - player.posY;
        
        double invDet = 1.0 / (player.planeX * player.dirY - player.dirX * player.planeY);
        double transformX = invDet * (player.dirY * spriteX - player.dirX * spriteY);
        double transformY = invDet * (-player.planeY * spriteX + player.planeX * spriteY);
        
        if (transformY <= 0) continue;
        
        int spriteScreenX = static_cast<int>((SCREEN_WIDTH / 2) * (1 + transformX / transformY));
        int spriteHeight = static_cast<int>(std::abs(SCREEN_HEIGHT / transformY));
        int spriteWidth = spriteHeight;
        
        int drawStartY = -spriteHeight / 2 + SCREEN_HEIGHT / 2;
        int drawEndY = spriteHeight / 2 + SCREEN_HEIGHT / 2;
        int drawStartX = -spriteWidth / 2 + spriteScreenX;
        int drawEndX = spriteWidth / 2 + spriteScreenX;
        
        if (drawStartY < 0) drawStartY = 0;
        if (drawEndY >= static_cast<int>(SCREEN_HEIGHT)) drawEndY = SCREEN_HEIGHT - 1;
        
        // Draw sprite columns
        for (int stripe = drawStartX; stripe < drawEndX; stripe++) {
            if (stripe < 0 || stripe >= static_cast<int>(SCREEN_WIDTH)) continue;
            if (transformY >= zBuffer[stripe]) continue;
            
            sf::RectangleShape spriteSlice({1, static_cast<float>(drawEndY - drawStartY)});
            spriteSlice.setPosition({static_cast<float>(stripe), static_cast<float>(drawStartY)});
            
            sf::Color enemyColor;
            switch (enemy.type) {
                case EnemyType::Wolf: enemyColor = sf::Color::Red; break;
                case EnemyType::SmokeDemon: enemyColor = sf::Color(128, 0, 128); break;
                case EnemyType::TophatOgre: enemyColor = sf::Color(0, 128, 0); break;
                case EnemyType::RedDemon: enemyColor = sf::Color(180, 0, 0); break;
            }
            
            spriteSlice.setFillColor(enemyColor);
            window.draw(spriteSlice);
        }
    }
}

// ===========================================
// MAIN GAME LOOP
// ===========================================

int main() {
    sf::RenderWindow window(sf::VideoMode({SCREEN_WIDTH, SCREEN_HEIGHT}), 
                            "DOOM Raycaster - Fixed Movement");
    window.setFramerateLimit(60);
    window.setMouseCursorVisible(false);
    
    // Load font
    sf::Font font;
    if (!font.openFromFile("res/arial.ttf")) { 
        std::cerr << "Could not load font\n"; 
        return -1; 
    }
    
    // Load textures
    sf::Texture wallTexture, wolfTexture, smokeDemonTexture, tophatOgreTexture, redDemonTexture;
    
    if (!wallTexture.loadFromFile("res/textures/world.png")) {
        std::cerr << "Could not load wall texture\n";
    }
    if (!wolfTexture.loadFromFile("res/textures/wolf.png")) {
        std::cerr << "Could not load wolf.png\n";
    }
    if (!smokeDemonTexture.loadFromFile("res/textures/smoke-demon.png")) {
        std::cerr << "Could not load smoke-demon.png\n";
    }
    if (!tophatOgreTexture.loadFromFile("res/textures/tophat-ogre.png")) {
        std::cerr << "Could not load tophat-ogre.png\n";
    }
    if (!redDemonTexture.loadFromFile("res/textures/Demon/Red/ALBUM008_72.png")) {
        std::cerr << "Could not load red demon texture\n";
    }
    
    // Generate map
    std::vector<std::vector<TileType>> worldMap(MAP_HEIGHT, std::vector<TileType>(MAP_WIDTH));
    generateDungeon(worldMap);
    
    // Find starting position
    int startX = 5, startY = 5;
    findEmptySpot(worldMap, startX, startY);
    
    Player player(startX + 0.5, startY + 0.5);
    
    // Spawn enemies
    std::vector<Enemy> enemies;
    for (int i = 0; i < 8; i++) {
        int ex, ey;
        if (findEmptySpot(worldMap, ex, ey)) {
            EnemyType type = static_cast<EnemyType>(i % 4);
            const sf::Texture* tex = &wolfTexture;
            sf::IntRect rect({0, 0}, {128, 128});
            
            switch (type) {
                case EnemyType::Wolf:
                    tex = &wolfTexture;
                    rect = sf::IntRect({0, 0}, {128, 128});
                    break;
                case EnemyType::SmokeDemon:
                    tex = &smokeDemonTexture;
                    rect = sf::IntRect({0, 0}, {160, 128});
                    break;
                case EnemyType::TophatOgre:
                    tex = &tophatOgreTexture;
                    rect = sf::IntRect({0, 0}, {160, 128});
                    break;
                case EnemyType::RedDemon:
                    tex = &redDemonTexture;
                    rect = sf::IntRect({0, 0}, {72, 72});
                    break;
            }
            
            enemies.emplace_back(ex + 0.5, ey + 0.5, type, 100, 1.5f, tex, rect);
        }
    }
    
    std::vector<Projectile> projectiles;
    std::vector<BloodParticle> bloodParticles;
    
    sf::Clock clock;
    sf::Clock fpsClock;
    int frameCount = 0;
    float fps = 0;
    
    std::cout << "===========================================\n";
    std::cout << "DOOM-STYLE RAYCASTER - CONTROLS:\n";
    std::cout << "W/S - Move forward/backward\n";
    std::cout << "A/D - Strafe left/right\n";
    std::cout << "Left/Right arrows - Rotate\n";
    std::cout << "Space - Shoot\n";
    std::cout << "ESC - Quit\n";
    std::cout << "===========================================\n";
    
    // Game loop (like D_DoomLoop)
    while (window.isOpen()) {
        float deltaTime = clock.restart().asSeconds();
        frameCount++;
        
        if (fpsClock.getElapsedTime().asSeconds() >= 1.0f) {
            fps = frameCount / fpsClock.getElapsedTime().asSeconds();
            frameCount = 0;
            fpsClock.restart();
        }
        
        // Event handling
        while (auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->code == sf::Keyboard::Key::Escape) {
                    window.close();
                }
                if (keyPressed->code == sf::Keyboard::Key::Space) {
                    // Shoot projectile
                    projectiles.emplace_back(player.posX, player.posY, 
                                            player.dirX, player.dirY, &wolfTexture);
                }
            }
        }
        
        // Update player movement (P_MovePlayer + P_Thrust)
        updatePlayerMovement(player, worldMap, deltaTime);
        
        // Update projectiles
        for (auto it = projectiles.begin(); it != projectiles.end();) {
            it->x += it->dirX * it->speed * deltaTime;
            it->y += it->dirY * it->speed * deltaTime;
            
            int mapX = static_cast<int>(it->x);
            int mapY = static_cast<int>(it->y);
            
            if (mapX < 0 || mapX >= MAP_WIDTH || mapY < 0 || mapY >= MAP_HEIGHT ||
                worldMap[mapY][mapX] == TileType::Wall) {
                it = projectiles.erase(it);
            } else {
                ++it;
            }
        }
        
        // Update blood particles
        for (auto it = bloodParticles.begin(); it != bloodParticles.end();) {
            it->velZ -= 9.8 * deltaTime;
            it->x += it->velX * deltaTime;
            it->y += it->velY * deltaTime;
            it->z += it->velZ * deltaTime;
            
            if (it->lifeClock.getElapsedTime().asSeconds() > it->lifetime) {
                it = bloodParticles.erase(it);
            } else {
                ++it;
            }
        }
        
        // Render
        window.clear();
        renderRaycaster(window, player, worldMap, enemies, wallTexture);
        
        // HUD
        sf::Text debugText(font,
            "Pos: (" + std::to_string(static_cast<int>(player.posX)) + ", " + 
            std::to_string(static_cast<int>(player.posY)) + ")\n" +
            "Dir: (" + std::to_string(player.dirX).substr(0, 5) + ", " + 
            std::to_string(player.dirY).substr(0, 5) + ")\n" +
            "Mom: (" + std::to_string(player.momX).substr(0, 5) + ", " + 
            std::to_string(player.momY).substr(0, 5) + ")\n" +
            "FPS: " + std::to_string(static_cast<int>(fps)) + "\n" +
            "Health: " + std::to_string(player.health),
            16
        );
        debugText.setFillColor(sf::Color::White);
        debugText.setPosition({10.f, 10.f});
        window.draw(debugText);
        
        window.display();
    }
    
    return 0;
}
