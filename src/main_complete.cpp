#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <vector>
#include <random>
#include <ctime>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <iomanip>

// ===========================================
// COMPLETE DOOM-STYLE GAME
// With Mouse Controls & Full Asset Integration
// ===========================================

constexpr unsigned int SCREEN_WIDTH = 1280;
constexpr unsigned int SCREEN_HEIGHT = 720;
constexpr int MAP_WIDTH = 64;
constexpr int MAP_HEIGHT = 64;

// Movement constants (from DOOM)
constexpr double MOVE_SPEED = 5.0;
constexpr double STRAFE_SPEED = 4.5;
constexpr double ROT_SPEED = 3.0;
constexpr double MOUSE_SENSITIVITY = 0.002;
constexpr double PLAYER_RADIUS = 0.3;
constexpr double FRICTION = 0.90;

enum class GameState { Title, Playing, Victory, GameOver };
enum class TileType { Empty = 0, Wall = 1 };
enum class EnemyType { Wolf, SmokeDemon, TophatOgre, RedDemon };

// Player struct
struct Player {
    double posX, posY;
    double dirX, dirY;
    double planeX, planeY;
    double momX, momY;
    int health;
    int maxHealth;
    int ammo;
    int score;
    int kills;
    
    Player(double x, double y) 
        : posX(x), posY(y), dirX(-1), dirY(0), planeX(0), planeY(0.66),
          momX(0), momY(0), health(100), maxHealth(100), ammo(50), score(0), kills(0) {}
};

// Enemy
struct Enemy {
    double x, y;
    EnemyType type;
    int health;
    int maxHealth;
    float speed;
    double dirX, dirY;
    bool active;
    sf::Clock attackClock;
    const sf::Texture* texture;
    sf::IntRect textureRect;
    
    Enemy(double px, double py, EnemyType t, int hp, float spd, 
          const sf::Texture* tex, sf::IntRect rect)
        : x(px), y(py), type(t), health(hp), maxHealth(hp), speed(spd), 
          dirX(0), dirY(0), active(true), texture(tex), textureRect(rect) {}
};

// Projectile
struct Projectile {
    double x, y, dirX, dirY;
    float speed;
    int damage;
    bool fromPlayer;
    sf::Clock lifeClock;
    
    Projectile(double px, double py, double dx, double dy, bool player = true)
        : x(px), y(py), dirX(dx), dirY(dy), speed(12.0f), 
          damage(player ? 25 : 10), fromPlayer(player) {}
};

// Blood particle
struct BloodParticle {
    double x, y, z, velX, velY, velZ;
    sf::Clock lifeClock;
    float lifetime;
    sf::Color color;
    
    BloodParticle(double px, double py, double vx, double vy)
        : x(px), y(py), z(0.5), velX(vx), velY(vy), velZ(0.5), 
          lifetime(0.8f), color(sf::Color::Red) {}
};

// Pickup items
struct Pickup {
    double x, y;
    enum Type { HealthPack, Ammo, Armor } type;
    int value;
    bool active;
    sf::Clock bobClock;
    
    Pickup(double px, double py, Type t, int v)
        : x(px), y(py), type(t), value(v), active(true) {}
};

// Room for dungeon generation
struct Room {
    int x, y, w, h;
    int centerX() const { return x + w / 2; }
    int centerY() const { return y + h / 2; }
};

// ===========================================
// COLLISION DETECTION
// ===========================================

bool checkCollision(const std::vector<std::vector<TileType>>& map, 
                    double x, double y, double radius) {
    double corners[4][2] = {
        {x - radius, y - radius}, {x + radius, y - radius},
        {x - radius, y + radius}, {x + radius, y + radius}
    };
    
    for (int i = 0; i < 4; i++) {
        int mapX = static_cast<int>(corners[i][0]);
        int mapY = static_cast<int>(corners[i][1]);
        
        if (mapX < 0 || mapX >= MAP_WIDTH || mapY < 0 || mapY >= MAP_HEIGHT) {
            return true;
        }
        if (map[mapY][mapX] == TileType::Wall) {
            return true;
        }
    }
    return false;
}

void tryMoveWithSlide(Player& player, 
                      const std::vector<std::vector<TileType>>& map,
                      double targetX, double targetY) {
    if (!checkCollision(map, targetX, targetY, PLAYER_RADIUS)) {
        player.posX = targetX;
        player.posY = targetY;
        return;
    }
    
    if (!checkCollision(map, targetX, player.posY, PLAYER_RADIUS)) {
        player.posX = targetX;
        return;
    }
    
    if (!checkCollision(map, player.posX, targetY, PLAYER_RADIUS)) {
        player.posY = targetY;
        return;
    }
}

// ===========================================
// PLAYER MOVEMENT
// ===========================================

void updatePlayerMovement(Player& player, 
                          const std::vector<std::vector<TileType>>& map,
                          float deltaTime,
                          float mouseDeltaX) {
    bool moving = false;
    
    // Keyboard movement
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
    
    // Apply friction
    if (!moving) {
        player.momX *= FRICTION;
        player.momY *= FRICTION;
        if (std::abs(player.momX) < 0.001) player.momX = 0;
        if (std::abs(player.momY) < 0.001) player.momY = 0;
    }
    
    // Apply momentum
    double targetX = player.posX + player.momX * deltaTime;
    double targetY = player.posY + player.momY * deltaTime;
    tryMoveWithSlide(player, map, targetX, targetY);
    
    // Mouse rotation
    if (std::abs(mouseDeltaX) > 0.001) {
        double rotAngle = -mouseDeltaX * MOUSE_SENSITIVITY;
        double oldDirX = player.dirX;
        player.dirX = player.dirX * std::cos(rotAngle) - player.dirY * std::sin(rotAngle);
        player.dirY = oldDirX * std::sin(rotAngle) + player.dirY * std::cos(rotAngle);
        
        double oldPlaneX = player.planeX;
        player.planeX = player.planeX * std::cos(rotAngle) - player.planeY * std::sin(rotAngle);
        player.planeY = oldPlaneX * std::sin(rotAngle) + player.planeY * std::cos(rotAngle);
    }
    
    // Arrow key rotation
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) {
        double rotAngle = ROT_SPEED * deltaTime;
        double oldDirX = player.dirX;
        player.dirX = player.dirX * std::cos(rotAngle) - player.dirY * std::sin(rotAngle);
        player.dirY = oldDirX * std::sin(rotAngle) + player.dirY * std::cos(rotAngle);
        
        double oldPlaneX = player.planeX;
        player.planeX = player.planeX * std::cos(rotAngle) - player.planeY * std::sin(rotAngle);
        player.planeY = oldPlaneX * std::sin(rotAngle) + player.planeY * std::cos(rotAngle);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) {
        double rotAngle = -ROT_SPEED * deltaTime;
        double oldDirX = player.dirX;
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

void generateDungeon(std::vector<std::vector<TileType>>& map, std::vector<Room>& rooms) {
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            map[y][x] = TileType::Wall;
        }
    }
    
    std::mt19937 rng(static_cast<unsigned>(std::time(nullptr)));
    std::uniform_int_distribution<int> roomCountDist(20, 30);
    std::uniform_int_distribution<int> roomSizeDist(5, 12);
    
    int numRooms = roomCountDist(rng);
    
    for (int i = 0; i < numRooms * 3; i++) {
        int w = roomSizeDist(rng);
        int h = roomSizeDist(rng);
        int x = std::uniform_int_distribution<int>(2, MAP_WIDTH - w - 2)(rng);
        int y = std::uniform_int_distribution<int>(2, MAP_HEIGHT - h - 2)(rng);
        
        bool overlap = false;
        for (const auto& room : rooms) {
            if (!(x + w + 1 < room.x || x > room.x + room.w + 1 ||
                  y + h + 1 < room.y || y > room.y + room.h + 1)) {
                overlap = true;
                break;
            }
        }
        
        if (!overlap) {
            rooms.push_back({x, y, w, h});
            for (int ry = y; ry < y + h; ry++) {
                for (int rx = x; rx < x + w; rx++) {
                    map[ry][rx] = TileType::Empty;
                }
            }
            
            if (rooms.size() >= static_cast<size_t>(numRooms)) break;
        }
    }
    
    // Connect rooms
    for (size_t i = 1; i < rooms.size(); i++) {
        int x1 = rooms[i-1].centerX();
        int y1 = rooms[i-1].centerY();
        int x2 = rooms[i].centerX();
        int y2 = rooms[i].centerY();
        
        for (int x = std::min(x1, x2); x <= std::max(x1, x2); x++) {
            if (y1 >= 0 && y1 < MAP_HEIGHT && x >= 0 && x < MAP_WIDTH) {
                map[y1][x] = TileType::Empty;
                if (y1 + 1 < MAP_HEIGHT) map[y1 + 1][x] = TileType::Empty;
            }
        }
        
        for (int y = std::min(y1, y2); y <= std::max(y1, y2); y++) {
            if (y >= 0 && y < MAP_HEIGHT && x2 >= 0 && x2 < MAP_WIDTH) {
                map[y][x2] = TileType::Empty;
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
// RAYCASTING RENDERER
// ===========================================

void renderRaycaster(sf::RenderWindow& window, 
                     const Player& player,
                     const std::vector<std::vector<TileType>>& map,
                     const std::vector<Enemy>& enemies,
                     const std::vector<Pickup>& pickups,
                     const sf::Texture& wallTexture) {
    
    // Ceiling
    sf::RectangleShape ceiling({static_cast<float>(SCREEN_WIDTH), 
                                 static_cast<float>(SCREEN_HEIGHT / 2)});
    ceiling.setPosition({0.f, 0.f});
    ceiling.setFillColor(sf::Color(50, 50, 50));
    window.draw(ceiling);
    
    // Floor
    sf::RectangleShape floor({static_cast<float>(SCREEN_WIDTH), 
                               static_cast<float>(SCREEN_HEIGHT / 2)});
    floor.setPosition({0.f, static_cast<float>(SCREEN_HEIGHT / 2)});
    floor.setFillColor(sf::Color(30, 30, 30));
    window.draw(floor);
    
    std::vector<double> zBuffer(SCREEN_WIDTH, 1e30);
    
    // Raycast walls
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
        
        double perpWallDist;
        if (side == 0) {
            perpWallDist = (mapX - player.posX + (1 - stepX) / 2) / rayDirX;
        } else {
            perpWallDist = (mapY - player.posY + (1 - stepY) / 2) / rayDirY;
        }
        
        if (perpWallDist < 0.1) perpWallDist = 0.1;
        zBuffer[x] = perpWallDist;
        
        int lineHeight = static_cast<int>(SCREEN_HEIGHT / perpWallDist);
        int drawStart = -lineHeight / 2 + SCREEN_HEIGHT / 2;
        int drawEnd = lineHeight / 2 + SCREEN_HEIGHT / 2;
        
        if (drawStart < 0) drawStart = 0;
        if (drawEnd >= static_cast<int>(SCREEN_HEIGHT)) drawEnd = SCREEN_HEIGHT - 1;
        
        sf::RectangleShape wallSlice({1, static_cast<float>(drawEnd - drawStart)});
        wallSlice.setPosition({static_cast<float>(x), static_cast<float>(drawStart)});
        
        // Textured walls with variation
        int texX = static_cast<int>((side == 0 ? player.posY : player.posX) * 64) % 64;
        sf::Color wallColor;
        if (texX < 16) {
            wallColor = sf::Color(120, 80, 60);
        } else if (texX < 32) {
            wallColor = sf::Color(100, 70, 50);
        } else if (texX < 48) {
            wallColor = sf::Color(110, 75, 55);
        } else {
            wallColor = sf::Color(90, 65, 45);
        }
        
        if (side == 1) {
            wallColor.r /= 1.5;
            wallColor.g /= 1.5;
            wallColor.b /= 1.5;
        }
        
        double fogFactor = std::min(1.0, perpWallDist / 20.0);
        wallColor.r = static_cast<std::uint8_t>(wallColor.r * (1.0 - fogFactor * 0.7));
        wallColor.g = static_cast<std::uint8_t>(wallColor.g * (1.0 - fogFactor * 0.7));
        wallColor.b = static_cast<std::uint8_t>(wallColor.b * (1.0 - fogFactor * 0.7));
        
        wallSlice.setFillColor(wallColor);
        window.draw(wallSlice);
    }
    
    // Draw pickups
    for (const auto& pickup : pickups) {
        if (!pickup.active) continue;
        
        double spriteX = pickup.x - player.posX;
        double spriteY = pickup.y - player.posY;
        
        double invDet = 1.0 / (player.planeX * player.dirY - player.dirX * player.planeY);
        double transformX = invDet * (player.dirY * spriteX - player.dirX * spriteY);
        double transformY = invDet * (-player.planeY * spriteX + player.planeX * spriteY);
        
        if (transformY <= 0.1) continue;
        
        int spriteScreenX = static_cast<int>((SCREEN_WIDTH / 2) * (1 + transformX / transformY));
        float bob = std::sin(pickup.bobClock.getElapsedTime().asSeconds() * 3) * 10;
        int spriteHeight = static_cast<int>(std::abs(SCREEN_HEIGHT / transformY) * 0.5);
        int spriteWidth = spriteHeight;
        
        int drawStartY = -spriteHeight / 2 + SCREEN_HEIGHT / 2 + static_cast<int>(bob);
        int drawEndY = spriteHeight / 2 + SCREEN_HEIGHT / 2 + static_cast<int>(bob);
        int drawStartX = -spriteWidth / 2 + spriteScreenX;
        int drawEndX = spriteWidth / 2 + spriteScreenX;
        
        if (drawStartY < 0) drawStartY = 0;
        if (drawEndY >= static_cast<int>(SCREEN_HEIGHT)) drawEndY = SCREEN_HEIGHT - 1;
        
        for (int stripe = drawStartX; stripe < drawEndX; stripe++) {
            if (stripe < 0 || stripe >= static_cast<int>(SCREEN_WIDTH)) continue;
            if (transformY >= zBuffer[stripe]) continue;
            
            sf::RectangleShape sprite({1, static_cast<float>(drawEndY - drawStartY)});
            sprite.setPosition({static_cast<float>(stripe), static_cast<float>(drawStartY)});
            
            sf::Color pickupColor;
            switch (pickup.type) {
                case Pickup::HealthPack: pickupColor = sf::Color::Green; break;
                case Pickup::Ammo: pickupColor = sf::Color::Yellow; break;
                case Pickup::Armor: pickupColor = sf::Color::Blue; break;
            }
            sprite.setFillColor(pickupColor);
            window.draw(sprite);
        }
    }
    
    // Draw enemies
    for (const auto& enemy : enemies) {
        if (!enemy.active) continue;
        
        double spriteX = enemy.x - player.posX;
        double spriteY = enemy.y - player.posY;
        
        double invDet = 1.0 / (player.planeX * player.dirY - player.dirX * player.planeY);
        double transformX = invDet * (player.dirY * spriteX - player.dirX * spriteY);
        double transformY = invDet * (-player.planeY * spriteX + player.planeX * spriteY);
        
        if (transformY <= 0.1) continue;
        
        int spriteScreenX = static_cast<int>((SCREEN_WIDTH / 2) * (1 + transformX / transformY));
        int spriteHeight = static_cast<int>(std::abs(SCREEN_HEIGHT / transformY));
        int spriteWidth = spriteHeight;
        
        int drawStartY = -spriteHeight / 2 + SCREEN_HEIGHT / 2;
        int drawEndY = spriteHeight / 2 + SCREEN_HEIGHT / 2;
        int drawStartX = -spriteWidth / 2 + spriteScreenX;
        int drawEndX = spriteWidth / 2 + spriteScreenX;
        
        if (drawStartY < 0) drawStartY = 0;
        if (drawEndY >= static_cast<int>(SCREEN_HEIGHT)) drawEndY = SCREEN_HEIGHT - 1;
        
        for (int stripe = drawStartX; stripe < drawEndX; stripe++) {
            if (stripe < 0 || stripe >= static_cast<int>(SCREEN_WIDTH)) continue;
            if (transformY >= zBuffer[stripe]) continue;
            
            sf::RectangleShape sprite({1, static_cast<float>(drawEndY - drawStartY)});
            sprite.setPosition({static_cast<float>(stripe), static_cast<float>(drawStartY)});
            
            sf::Color enemyColor;
            switch (enemy.type) {
                case EnemyType::Wolf: enemyColor = sf::Color(200, 50, 50); break;
                case EnemyType::SmokeDemon: enemyColor = sf::Color(150, 0, 150); break;
                case EnemyType::TophatOgre: enemyColor = sf::Color(50, 150, 50); break;
                case EnemyType::RedDemon: enemyColor = sf::Color(220, 0, 0); break;
            }
            
            // Health indicator
            float healthPercent = static_cast<float>(enemy.health) / enemy.maxHealth;
            enemyColor.r = static_cast<std::uint8_t>(enemyColor.r * healthPercent);
            enemyColor.g = static_cast<std::uint8_t>(enemyColor.g * healthPercent);
            enemyColor.b = static_cast<std::uint8_t>(enemyColor.b * healthPercent);
            
            sprite.setFillColor(enemyColor);
            window.draw(sprite);
        }
    }
}

// ===========================================
// MAIN GAME
// ===========================================

int main() {
    sf::RenderWindow window(sf::VideoMode({SCREEN_WIDTH, SCREEN_HEIGHT}), 
                            "DOOM - Complete Edition");
    window.setFramerateLimit(60);
    
    // Load font
    sf::Font font;
    if (!font.openFromFile("res/arial.ttf")) { 
        std::cerr << "Could not load font\n"; 
        return -1; 
    }
    
    // Load DOOM assets
    sf::Texture titleTexture, statusBarTexture, victoryTexture;
    if (!titleTexture.loadFromFile("res/doom/TITLEPIC.png")) {
        std::cerr << "Could not load title screen\n";
    }
    if (!statusBarTexture.loadFromFile("res/doom/STBAR.png")) {
        std::cerr << "Could not load status bar\n";
    }
    if (!victoryTexture.loadFromFile("res/doom/VICTORY2.png")) {
        std::cerr << "Could not load victory screen\n";
    }
    
    // Load enemy textures
    sf::Texture wolfTexture, smokeDemonTexture, tophatOgreTexture, redDemonTexture, wallTexture;
    wolfTexture.loadFromFile("res/textures/wolf.png");
    smokeDemonTexture.loadFromFile("res/textures/smoke-demon.png");
    tophatOgreTexture.loadFromFile("res/textures/tophat-ogre.png");
    redDemonTexture.loadFromFile("res/textures/Demon/Red/ALBUM008_72.png");
    wallTexture.loadFromFile("res/textures/world.png");
    
    // Game state
    GameState gameState = GameState::Title;
    
    // Generate map
    std::vector<std::vector<TileType>> worldMap(MAP_HEIGHT, std::vector<TileType>(MAP_WIDTH));
    std::vector<Room> rooms;
    generateDungeon(worldMap, rooms);
    
    // Initialize player
    int startX = 5, startY = 5;
    if (!rooms.empty()) {
        startX = rooms[0].centerX();
        startY = rooms[0].centerY();
    }
    Player player(startX + 0.5, startY + 0.5);
    
    // Spawn enemies
    std::vector<Enemy> enemies;
    for (int i = 0; i < 15; i++) {
        int ex, ey;
        if (findEmptySpot(worldMap, ex, ey)) {
            EnemyType type = static_cast<EnemyType>(i % 4);
            const sf::Texture* tex = &wolfTexture;
            sf::IntRect rect({0, 0}, {128, 128});
            int hp = 50;
            float spd = 1.5f;
            
            switch (type) {
                case EnemyType::Wolf:
                    tex = &wolfTexture;
                    rect = sf::IntRect({0, 0}, {128, 128});
                    hp = 50;
                    spd = 2.0f;
                    break;
                case EnemyType::SmokeDemon:
                    tex = &smokeDemonTexture;
                    rect = sf::IntRect({0, 0}, {160, 128});
                    hp = 75;
                    spd = 1.5f;
                    break;
                case EnemyType::TophatOgre:
                    tex = &tophatOgreTexture;
                    rect = sf::IntRect({0, 0}, {160, 128});
                    hp = 100;
                    spd = 1.2f;
                    break;
                case EnemyType::RedDemon:
                    tex = &redDemonTexture;
                    rect = sf::IntRect({0, 0}, {72, 72});
                    hp = 150;
                    spd = 1.0f;
                    break;
            }
            
            enemies.emplace_back(ex + 0.5, ey + 0.5, type, hp, spd, tex, rect);
        }
    }
    
    // Spawn pickups
    std::vector<Pickup> pickups;
    for (int i = 0; i < 10; i++) {
        int px, py;
        if (findEmptySpot(worldMap, px, py)) {
            Pickup::Type type = static_cast<Pickup::Type>(i % 3);
            int value = 0;
            switch (type) {
                case Pickup::HealthPack: value = 25; break;
                case Pickup::Ammo: value = 20; break;
                case Pickup::Armor: value = 50; break;
            }
            pickups.emplace_back(px + 0.5, py + 0.5, type, value);
        }
    }
    
    std::vector<Projectile> projectiles;
    std::vector<BloodParticle> bloodParticles;
    
    sf::Clock clock;
    sf::Clock fpsClock;
    sf::Clock shootClock;
    int frameCount = 0;
    float fps = 0;
    float mouseDeltaX = 0;
    sf::Vector2i lastMousePos = sf::Mouse::getPosition(window);
    
    std::cout << "===========================================\n";
    std::cout << "DOOM - COMPLETE EDITION\n";
    std::cout << "Controls:\n";
    std::cout << "  W/S - Move forward/backward\n";
    std::cout << "  A/D - Strafe left/right\n";
    std::cout << "  Mouse - Look around\n";
    std::cout << "  Left Click - Shoot\n";
    std::cout << "  Arrow Keys - Rotate\n";
    std::cout << "  ESC - Quit/Menu\n";
    std::cout << "===========================================\n";
    
    // Game loop
    while (window.isOpen()) {
        float deltaTime = clock.restart().asSeconds();
        frameCount++;
        
        if (fpsClock.getElapsedTime().asSeconds() >= 1.0f) {
            fps = frameCount / fpsClock.getElapsedTime().asSeconds();
            frameCount = 0;
            fpsClock.restart();
        }
        
        // Mouse movement
        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        mouseDeltaX = static_cast<float>(mousePos.x - lastMousePos.x);
        lastMousePos = mousePos;
        
        // Center mouse if in playing state
        if (gameState == GameState::Playing && window.hasFocus()) {
            sf::Mouse::setPosition(sf::Vector2i(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2), window);
            lastMousePos = sf::Vector2i(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
        }
        
        // Event handling
        while (auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            
            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->code == sf::Keyboard::Key::Escape) {
                    if (gameState == GameState::Playing) {
                        gameState = GameState::Title;
                        window.setMouseCursorVisible(true);
                    } else {
                        window.close();
                    }
                }
                
                if (keyPressed->code == sf::Keyboard::Key::Enter && gameState == GameState::Title) {
                    gameState = GameState::Playing;
                    window.setMouseCursorVisible(false);
                }
            }
            
            if (const auto* mousePressed = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mousePressed->button == sf::Mouse::Button::Left) {
                    if (gameState == GameState::Title) {
                        gameState = GameState::Playing;
                        window.setMouseCursorVisible(false);
                    } else if (gameState == GameState::Playing && 
                              player.ammo > 0 && 
                              shootClock.getElapsedTime().asSeconds() > 0.3f) {
                        projectiles.emplace_back(player.posX, player.posY, 
                                                player.dirX, player.dirY, true);
                        player.ammo--;
                        shootClock.restart();
                    }
                }
            }
        }
        
        // Update game state
        if (gameState == GameState::Playing) {
            // Update player
            updatePlayerMovement(player, worldMap, deltaTime, mouseDeltaX);
            mouseDeltaX = 0;
            
            // Check victory condition
            bool allEnemiesDead = true;
            for (const auto& enemy : enemies) {
                if (enemy.active) {
                    allEnemiesDead = false;
                    break;
                }
            }
            if (allEnemiesDead && enemies.size() > 0) {
                gameState = GameState::Victory;
                window.setMouseCursorVisible(true);
            }
            
            // Update enemies - simple AI
            for (auto& enemy : enemies) {
                if (!enemy.active) continue;
                
                double dx = player.posX - enemy.x;
                double dy = player.posY - enemy.y;
                double dist = std::sqrt(dx * dx + dy * dy);
                
                if (dist > 0.1 && dist < 15.0) {
                    enemy.dirX = dx / dist;
                    enemy.dirY = dy / dist;
                    
                    double newX = enemy.x + enemy.dirX * enemy.speed * deltaTime;
                    double newY = enemy.y + enemy.dirY * enemy.speed * deltaTime;
                    
                    if (!checkCollision(worldMap, newX, newY, 0.2)) {
                        enemy.x = newX;
                        enemy.y = newY;
                    }
                    
                    // Attack player if close
                    if (dist < 1.5 && enemy.attackClock.getElapsedTime().asSeconds() > 1.5f) {
                        player.health -= 10;
                        enemy.attackClock.restart();
                        
                        if (player.health <= 0) {
                            gameState = GameState::GameOver;
                            window.setMouseCursorVisible(true);
                        }
                    }
                }
            }
            
            // Update projectiles
            for (auto it = projectiles.begin(); it != projectiles.end();) {
                it->x += it->dirX * it->speed * deltaTime;
                it->y += it->dirY * it->speed * deltaTime;
                
                int mapX = static_cast<int>(it->x);
                int mapY = static_cast<int>(it->y);
                
                bool hitWall = (mapX < 0 || mapX >= MAP_WIDTH || mapY < 0 || mapY >= MAP_HEIGHT ||
                               worldMap[mapY][mapX] == TileType::Wall);
                
                bool hitEnemy = false;
                if (it->fromPlayer) {
                    for (auto& enemy : enemies) {
                        if (!enemy.active) continue;
                        double dx = it->x - enemy.x;
                        double dy = it->y - enemy.y;
                        if (std::sqrt(dx * dx + dy * dy) < 0.5) {
                            enemy.health -= it->damage;
                            hitEnemy = true;
                            
                            // Spawn blood
                            for (int i = 0; i < 5; i++) {
                                double angle = (i / 5.0) * 2 * 3.14159;
                                bloodParticles.emplace_back(enemy.x, enemy.y,
                                    std::cos(angle) * 2, std::sin(angle) * 2);
                            }
                            
                            if (enemy.health <= 0) {
                                enemy.active = false;
                                player.score += 100;
                                player.kills++;
                            }
                            break;
                        }
                    }
                }
                
                if (hitWall || hitEnemy || it->lifeClock.getElapsedTime().asSeconds() > 2.0f) {
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
                
                if (it->z < 0) it->z = 0;
                
                if (it->lifeClock.getElapsedTime().asSeconds() > it->lifetime) {
                    it = bloodParticles.erase(it);
                } else {
                    ++it;
                }
            }
            
            // Check pickups
            for (auto& pickup : pickups) {
                if (!pickup.active) continue;
                
                double dx = player.posX - pickup.x;
                double dy = player.posY - pickup.y;
                if (std::sqrt(dx * dx + dy * dy) < 0.8) {
                    pickup.active = false;
                    
                    switch (pickup.type) {
                        case Pickup::HealthPack:
                            player.health = std::min(player.maxHealth, player.health + pickup.value);
                            break;
                        case Pickup::Ammo:
                            player.ammo += pickup.value;
                            break;
                        case Pickup::Armor:
                            // Could add armor system
                            player.health = std::min(player.maxHealth, player.health + pickup.value / 2);
                            break;
                    }
                }
            }
        }
        
        // Render
        window.clear();
        
        if (gameState == GameState::Title) {
            // Title screen
            sf::Sprite titleSprite(titleTexture);
            float scaleX = static_cast<float>(SCREEN_WIDTH) / titleTexture.getSize().x;
            float scaleY = static_cast<float>(SCREEN_HEIGHT) / titleTexture.getSize().y;
            titleSprite.setScale({scaleX, scaleY});
            window.draw(titleSprite);
            
            sf::Text startText(font, "Click or Press ENTER to Start\nESC to Quit", 32);
            startText.setFillColor(sf::Color::Red);
            startText.setPosition({SCREEN_WIDTH / 2.f - 200, SCREEN_HEIGHT - 100.f});
            window.draw(startText);
            
        } else if (gameState == GameState::Playing) {
            // Render 3D view
            renderRaycaster(window, player, worldMap, enemies, pickups, wallTexture);
            
            // HUD overlay
            sf::RectangleShape hudBg({static_cast<float>(SCREEN_WIDTH), 60.f});
            hudBg.setPosition({0.f, static_cast<float>(SCREEN_HEIGHT - 60)});
            hudBg.setFillColor(sf::Color(0, 0, 0, 180));
            window.draw(hudBg);
            
            // Health bar
            sf::RectangleShape healthBar({200.f * (player.health / 100.f), 30.f});
            healthBar.setPosition({20.f, static_cast<float>(SCREEN_HEIGHT - 50)});
            healthBar.setFillColor(player.health > 50 ? sf::Color::Green : 
                                   player.health > 25 ? sf::Color::Yellow : sf::Color::Red);
            window.draw(healthBar);
            
            std::stringstream hudText;
            hudText << "Health: " << player.health << "  "
                    << "Ammo: " << player.ammo << "  "
                    << "Score: " << player.score << "  "
                    << "Kills: " << player.kills << "/" << enemies.size() << "  "
                    << "FPS: " << static_cast<int>(fps);
            
            sf::Text hud(font, hudText.str(), 20);
            hud.setFillColor(sf::Color::White);
            hud.setPosition({240.f, static_cast<float>(SCREEN_HEIGHT - 45)});
            window.draw(hud);
            
            // Crosshair
            sf::RectangleShape crosshairH({20.f, 2.f});
            crosshairH.setPosition({SCREEN_WIDTH / 2.f - 10, SCREEN_HEIGHT / 2.f - 1});
            crosshairH.setFillColor(sf::Color::Green);
            window.draw(crosshairH);
            
            sf::RectangleShape crosshairV({2.f, 20.f});
            crosshairV.setPosition({SCREEN_WIDTH / 2.f - 1, SCREEN_HEIGHT / 2.f - 10});
            crosshairV.setFillColor(sf::Color::Green);
            window.draw(crosshairV);
            
        } else if (gameState == GameState::Victory) {
            sf::Sprite victorySprite(victoryTexture);
            float scaleX = static_cast<float>(SCREEN_WIDTH) / victoryTexture.getSize().x;
            float scaleY = static_cast<float>(SCREEN_HEIGHT) / victoryTexture.getSize().y;
            victorySprite.setScale({scaleX, scaleY});
            window.draw(victorySprite);
            
            std::stringstream victoryText;
            victoryText << "VICTORY!\n\nFinal Score: " << player.score
                       << "\nKills: " << player.kills
                       << "\nPress ESC to exit";
            
            sf::Text text(font, victoryText.str(), 48);
            text.setFillColor(sf::Color::Yellow);
            text.setOutlineColor(sf::Color::Black);
            text.setOutlineThickness(3);
            text.setPosition({SCREEN_WIDTH / 2.f - 200, SCREEN_HEIGHT / 2.f - 100});
            window.draw(text);
            
        } else if (gameState == GameState::GameOver) {
            window.clear(sf::Color::Red);
            
            sf::Text gameOverText(font, "GAME OVER\n\nPress ESC to exit", 64);
            gameOverText.setFillColor(sf::Color::Black);
            gameOverText.setPosition({SCREEN_WIDTH / 2.f - 250, SCREEN_HEIGHT / 2.f - 100});
            window.draw(gameOverText);
        }
        
        window.display();
    }
    
    return 0;
}
