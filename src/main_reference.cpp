#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <vector>
#include <random>
#include <ctime>
#include <cmath>
#include <algorithm>
#include <iostream>

// Game states
enum class GameState { Loading, Playing, GameOver };
enum class TileType { Empty, Wall, Door };
enum class EnemyType { Wolf, SmokeDemon, TophatOgre, RedDemon };

// Player in first-person
struct Player {
    double posX, posY;      // Position in world
    double dirX, dirY;      // Direction vector
    double planeX, planeY;  // Camera plane (for FOV)
    int health;
    sf::Clock damageClock;
    
    Player(double x, double y) 
        : posX(x), posY(y), dirX(-1), dirY(0), planeX(0), planeY(0.66), health(100) {}
};

// Sprite-based enemy (billboard)
struct Enemy {
    double x, y;
    EnemyType type;
    int health;
    float speed;
    const sf::Texture* texture;
    sf::IntRect textureRect;
    
    Enemy(double px, double py, EnemyType t, int hp, float spd, const sf::Texture* tex, sf::IntRect rect)
        : x(px), y(py), type(t), health(hp), speed(spd), texture(tex), textureRect(rect) {}
};

// Projectile
struct Projectile {
    double x, y;
    double dirX, dirY;
    float speed;
    const sf::Texture* texture;
    
    Projectile(double px, double py, double dx, double dy, const sf::Texture* tex)
        : x(px), y(py), dirX(dx), dirY(dy), speed(8.0f), texture(tex) {}
};

// Blood particle effect
struct BloodParticle {
    double x, y, z;  // z for height
    double velX, velY, velZ;
    sf::Clock lifeClock;
    float lifetime;
    int frameIndex;
    
    BloodParticle(double px, double py, double vx, double vy)
        : x(px), y(py), z(0.5), velX(vx), velY(vy), velZ(0.5), lifetime(0.5f), frameIndex(0) {}
};

// Helper functions
void generateDungeon(int width, int height, std::vector<std::vector<TileType>>& map);
bool findEmptySpot(const std::vector<std::vector<TileType>>& map, int& x, int& y);

int main() {
    // Window setup
    const unsigned int SCREEN_WIDTH = 1280;
    const unsigned int SCREEN_HEIGHT = 720;
    const int MAP_WIDTH = 64;
    const int MAP_HEIGHT = 64;
    
    sf::RenderWindow window(sf::VideoMode({SCREEN_WIDTH, SCREEN_HEIGHT}), "DOOM-style Raycaster");
    window.setFramerateLimit(60);
    window.setMouseCursorVisible(false);
    
    // Load font
    sf::Font font;
    if (!font.openFromFile("res/arial.ttf")) { 
        std::cerr << "Could not load font\n"; 
        return -1; 
    }
    
    // Load textures
    sf::Texture wallTexture, floorTexture, ceilingTexture;
    if (!wallTexture.loadFromFile("res/textures/world.png")) {
        std::cerr << "Could not load wall texture\n";
        return -1;
    }
    
    // Enemy textures
    sf::Texture wolfTexture, smokeDemonTexture, tophatOgreTexture, redDemonTexture;
    if (!wolfTexture.loadFromFile("res/textures/wolf.png")) { 
        std::cerr << "Could not load wolf.png\n"; 
        return -1; 
    }
    if (!smokeDemonTexture.loadFromFile("res/textures/smoke-demon.png")) { 
        std::cerr << "Could not load smoke-demon.png\n"; 
        return -1; 
    }
    if (!tophatOgreTexture.loadFromFile("res/textures/tophat-ogre.png")) { 
        std::cerr << "Could not load tophat-ogre.png\n"; 
        return -1; 
    }
    if (!redDemonTexture.loadFromFile("res/textures/Demon/Red/ALBUM008_72.png")) { 
        std::cerr << "Could not load red demon\n"; 
        return -1; 
    }
    
    // Blood textures
    sf::Texture bloodTextures[4];
    for (int i = 0; i < 4; i++) {
        std::string path = "res/textures/Blood/BLUD" + std::string(1, 'A' + i) + "0.png";
        if (!bloodTextures[i].loadFromFile(path)) {
            std::cerr << "Could not load blood texture " << i << "\n";
            return -1;
        }
    }
    
    // Projectile texture
    sf::Texture projectileTexture;
    if (!projectileTexture.loadFromFile("res/textures/Player Projectiles/WIDBALL.cells/000.PNG")) {
        std::cerr << "Could not load projectile\n";
        return -1;
    }
    
    // Music
    sf::Music music;
    if (!music.openFromFile("res/sfx/music.ogg")) {
        std::cerr << "Could not load music\n";
        return -1;
    }
    music.setVolume(50);
    music.setPlayingOffset(sf::Time::Zero);  // SFML 3.0 uses play() which loops by default
    
    // Generate dungeon
    std::vector<std::vector<TileType>> worldMap;
    generateDungeon(MAP_WIDTH, MAP_HEIGHT, worldMap);
    
    // Find starting position
    int startX, startY;
    if (!findEmptySpot(worldMap, startX, startY)) {
        startX = MAP_WIDTH / 2;
        startY = MAP_HEIGHT / 2;
    }
    
    std::cout << "Player starting at: (" << startX << ", " << startY << ")\n";
    std::cout << "Tile at start: " << (worldMap[startY][startX] == TileType::Empty ? "Empty" : "Wall") << "\n";
    
    // Initialize player
    Player player(startX + 0.5, startY + 0.5);
    
    // Game variables
    std::vector<Enemy> enemies;
    std::vector<Projectile> projectiles;
    std::vector<BloodParticle> bloodParticles;
    std::mt19937 rng(static_cast<unsigned int>(time(0)));
    
    int score = 0;
    GameState gameState = GameState::Playing;
    
    sf::Clock deltaClock;
    sf::Clock shootClock;
    sf::Clock enemySpawnClock;
    
    const float moveSpeed = 3.0f;
    const float rotSpeed = 2.5f;
    
    music.play();
    
    // Create image buffer for raycasting
    sf::Image screenBuffer({SCREEN_WIDTH, SCREEN_HEIGHT}, sf::Color::Black);
    sf::Texture screenTexture;
    screenTexture.loadFromImage(screenBuffer);
    sf::Sprite screenSprite(screenTexture);
    
    // UI elements
    sf::Text healthText(font, "Health: 100", 24);
    healthText.setFillColor(sf::Color::Red);
    healthText.setPosition({10, 10});
    
    sf::Text scoreText(font, "Score: 0", 24);
    scoreText.setFillColor(sf::Color::Yellow);
    scoreText.setPosition({10, 40});
    
    sf::Text ammoText(font, "Ammo: INF", 24);
    ammoText.setFillColor(sf::Color::White);
    ammoText.setPosition({10, 70});
    
    sf::Text debugText(font, "Debug", 20);
    debugText.setFillColor(sf::Color::Cyan);
    debugText.setPosition({10, SCREEN_HEIGHT - 30});
    
    sf::CircleShape crosshair(3);
    crosshair.setFillColor(sf::Color::Green);
    crosshair.setOrigin({3, 3});
    crosshair.setPosition({SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f});
    
    // Spawn initial enemies
    for (int i = 0; i < 10; i++) {
        int ex, ey;
        if (findEmptySpot(worldMap, ex, ey)) {
            // Random enemy type
            EnemyType type = static_cast<EnemyType>(std::uniform_int_distribution<int>(0, 3)(rng));
            const sf::Texture* tex;
            sf::IntRect rect;
            int health;
            float speed;
            
            switch(type) {
                case EnemyType::Wolf:
                    tex = &wolfTexture;
                    rect = sf::IntRect({0, 0}, {128, 128});
                    health = 30;
                    speed = 2.0f;
                    break;
                case EnemyType::SmokeDemon:
                    tex = &smokeDemonTexture;
                    rect = sf::IntRect({0, 0}, {160, 128});
                    health = 50;
                    speed = 1.5f;
                    break;
                case EnemyType::TophatOgre:
                    tex = &tophatOgreTexture;
                    rect = sf::IntRect({0, 0}, {160, 128});
                    health = 70;
                    speed = 1.0f;
                    break;
                case EnemyType::RedDemon:
                    tex = &redDemonTexture;
                    rect = sf::IntRect({0, 0}, {(int)redDemonTexture.getSize().x, (int)redDemonTexture.getSize().y});
                    health = 100;
                    speed = 0.8f;
                    break;
            }
            
            enemies.emplace_back(ex + 0.5, ey + 0.5, type, health, speed, tex, rect);
        }
    }
    
    // Main game loop
    while (window.isOpen()) {
        float deltaTime = deltaClock.restart().asSeconds();
        
        // Event handling
        while (const auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            if (event->is<sf::Event::KeyPressed>()) {
                if (event->getIf<sf::Event::KeyPressed>()->code == sf::Keyboard::Key::Escape) {
                    window.close();
                }
            }
            if (event->is<sf::Event::MouseButtonPressed>()) {
                if (event->getIf<sf::Event::MouseButtonPressed>()->button == sf::Mouse::Button::Left) {
                    // Shoot
                    projectiles.emplace_back(player.posX, player.posY, player.dirX, player.dirY, &projectileTexture);
                    shootClock.restart();
                }
            }
        }
        
        if (gameState == GameState::Playing) {
            // Movement
            float frameMove = moveSpeed * deltaTime;
            float frameRot = rotSpeed * deltaTime;
            
            // Forward/backward
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) {
                double newX = player.posX + player.dirX * frameMove;
                double newY = player.posY + player.dirY * frameMove;
                if (worldMap[(int)newY][(int)newX] == TileType::Empty) {
                    player.posX = newX;
                    player.posY = newY;
                }
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) {
                double newX = player.posX - player.dirX * frameMove;
                double newY = player.posY - player.dirY * frameMove;
                if (worldMap[(int)newY][(int)newX] == TileType::Empty) {
                    player.posX = newX;
                    player.posY = newY;
                }
            }
            
            // Strafe left/right
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) {
                double newX = player.posX - player.planeX * frameMove;
                double newY = player.posY - player.planeY * frameMove;
                if (worldMap[(int)newY][(int)newX] == TileType::Empty) {
                    player.posX = newX;
                    player.posY = newY;
                }
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) {
                double newX = player.posX + player.planeX * frameMove;
                double newY = player.posY + player.planeY * frameMove;
                if (worldMap[(int)newY][(int)newX] == TileType::Empty) {
                    player.posX = newX;
                    player.posY = newY;
                }
            }
            
            // Rotation
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) {
                double oldDirX = player.dirX;
                player.dirX = player.dirX * cos(frameRot) - player.dirY * sin(frameRot);
                player.dirY = oldDirX * sin(frameRot) + player.dirY * cos(frameRot);
                double oldPlaneX = player.planeX;
                player.planeX = player.planeX * cos(frameRot) - player.planeY * sin(frameRot);
                player.planeY = oldPlaneX * sin(frameRot) + player.planeY * cos(frameRot);
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) {
                double oldDirX = player.dirX;
                player.dirX = player.dirX * cos(-frameRot) - player.dirY * sin(-frameRot);
                player.dirY = oldDirX * sin(-frameRot) + player.dirY * cos(-frameRot);
                double oldPlaneX = player.planeX;
                player.planeX = player.planeX * cos(-frameRot) - player.planeY * sin(-frameRot);
                player.planeY = oldPlaneX * sin(-frameRot) + player.planeY * cos(-frameRot);
            }
            
            // Shoot with space
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space) && shootClock.getElapsedTime().asSeconds() > 0.2f) {
                projectiles.emplace_back(player.posX, player.posY, player.dirX, player.dirY, &projectileTexture);
                shootClock.restart();
            }
            
            // Update projectiles
            for (auto& proj : projectiles) {
                proj.x += proj.dirX * proj.speed * deltaTime;
                proj.y += proj.dirY * proj.speed * deltaTime;
            }
            
            // Remove projectiles that hit walls
            projectiles.erase(std::remove_if(projectiles.begin(), projectiles.end(), [&](const Projectile& p) {
                int mapX = (int)p.x;
                int mapY = (int)p.y;
                if (mapX < 0 || mapX >= MAP_WIDTH || mapY < 0 || mapY >= MAP_HEIGHT) return true;
                return worldMap[mapY][mapX] != TileType::Empty;
            }), projectiles.end());
            
            // Update enemies (move toward player)
            for (auto& enemy : enemies) {
                double dx = player.posX - enemy.x;
                double dy = player.posY - enemy.y;
                double dist = sqrt(dx * dx + dy * dy);
                if (dist > 0.5 && dist < 20.0) {
                    double moveX = (dx / dist) * enemy.speed * deltaTime;
                    double moveY = (dy / dist) * enemy.speed * deltaTime;
                    double newX = enemy.x + moveX;
                    double newY = enemy.y + moveY;
                    if (worldMap[(int)newY][(int)newX] == TileType::Empty) {
                        enemy.x = newX;
                        enemy.y = newY;
                    }
                }
            }
            
            // Projectile-enemy collision
            for (auto& proj : projectiles) {
                for (auto& enemy : enemies) {
                    if (enemy.health > 0) {
                        double dx = enemy.x - proj.x;
                        double dy = enemy.y - proj.y;
                        if (sqrt(dx * dx + dy * dy) < 0.5) {
                            enemy.health -= 25;
                            proj.x = -999; // Mark for removal
                            
                            // Spawn blood
                            for (int i = 0; i < 3; i++) {
                                double angle = std::uniform_real_distribution<double>(0.0, 6.28318)(rng);
                                double speed = std::uniform_real_distribution<double>(0.5, 1.5)(rng);
                                bloodParticles.emplace_back(enemy.x, enemy.y, cos(angle) * speed, sin(angle) * speed);
                            }
                            
                            if (enemy.health <= 0) {
                                score += 10 + (int)enemy.type * 10;
                            }
                            break;
                        }
                    }
                }
            }
            
            // Remove dead enemies
            enemies.erase(std::remove_if(enemies.begin(), enemies.end(), [](const Enemy& e) {
                return e.health <= 0;
            }), enemies.end());
            
            // Remove used projectiles
            projectiles.erase(std::remove_if(projectiles.begin(), projectiles.end(), [](const Projectile& p) {
                return p.x < -900;
            }), projectiles.end());
            
            // Update blood particles
            for (auto& blood : bloodParticles) {
                blood.x += blood.velX * deltaTime;
                blood.y += blood.velY * deltaTime;
                blood.z += blood.velZ * deltaTime;
                blood.velZ -= 2.0 * deltaTime; // Gravity
                blood.velX *= 0.95f;
                blood.velY *= 0.95f;
            }
            
            // Remove expired blood
            bloodParticles.erase(std::remove_if(bloodParticles.begin(), bloodParticles.end(), [](const BloodParticle& b) {
                return b.lifeClock.getElapsedTime().asSeconds() > b.lifetime || b.z < 0;
            }), bloodParticles.end());
            
            // Spawn new enemies periodically
            if (enemies.size() < 15 && enemySpawnClock.getElapsedTime().asSeconds() > 3.0f) {
                int ex, ey;
                if (findEmptySpot(worldMap, ex, ey)) {
                    EnemyType type = static_cast<EnemyType>(std::uniform_int_distribution<int>(0, 3)(rng));
                    const sf::Texture* tex;
                    sf::IntRect rect;
                    int health;
                    float speed;
                    
                    switch(type) {
                        case EnemyType::Wolf:
                            tex = &wolfTexture;
                            rect = sf::IntRect({0, 0}, {128, 128});
                            health = 30;
                            speed = 2.0f;
                            break;
                        case EnemyType::SmokeDemon:
                            tex = &smokeDemonTexture;
                            rect = sf::IntRect({0, 0}, {160, 128});
                            health = 50;
                            speed = 1.5f;
                            break;
                        case EnemyType::TophatOgre:
                            tex = &tophatOgreTexture;
                            rect = sf::IntRect({0, 0}, {160, 128});
                            health = 70;
                            speed = 1.0f;
                            break;
                        case EnemyType::RedDemon:
                            tex = &redDemonTexture;
                            rect = sf::IntRect({0, 0}, {(int)redDemonTexture.getSize().x, (int)redDemonTexture.getSize().y});
                            health = 100;
                            speed = 0.8f;
                            break;
                    }
                    
                    enemies.emplace_back(ex + 0.5, ey + 0.5, type, health, speed, tex, rect);
                    enemySpawnClock.restart();
                }
            }
        }
        
        // === RAYCASTING RENDERING ===
        screenBuffer = sf::Image({SCREEN_WIDTH, SCREEN_HEIGHT}, sf::Color(50, 50, 50)); // Floor color
        
        // Draw ceiling (top half)
        for (unsigned int y = 0; y < SCREEN_HEIGHT / 2; y++) {
            for (unsigned int x = 0; x < SCREEN_WIDTH; x++) {
                screenBuffer.setPixel({x, y}, sf::Color(30, 30, 30)); // Ceiling color
            }
        }
        
        // Raycasting for walls
        std::vector<double> zBuffer(SCREEN_WIDTH, 0.0);
        
        for (int x = 0; x < (int)SCREEN_WIDTH; x++) {
            double cameraX = 2.0 * x / (double)SCREEN_WIDTH - 1.0;
            double rayDirX = player.dirX + player.planeX * cameraX;
            double rayDirY = player.dirY + player.planeY * cameraX;
            
            int mapX = (int)player.posX;
            int mapY = (int)player.posY;
            
            double sideDistX, sideDistY;
            double deltaDistX = (rayDirX == 0) ? 1e30 : std::abs(1.0 / rayDirX);
            double deltaDistY = (rayDirY == 0) ? 1e30 : std::abs(1.0 / rayDirY);
            double perpWallDist;
            
            int stepX, stepY;
            bool hit = false;
            int side;
            
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
            
            // DDA algorithm
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
                
                if (mapX < 0 || mapX >= MAP_WIDTH || mapY < 0 || mapY >= MAP_HEIGHT ||
                    worldMap[mapY][mapX] == TileType::Wall) {
                    hit = true;
                }
            }
            
            if (side == 0) {
                perpWallDist = (sideDistX - deltaDistX);
            } else {
                perpWallDist = (sideDistY - deltaDistY);
            }
            
            zBuffer[x] = perpWallDist;
            
            int lineHeight = (int)(SCREEN_HEIGHT / perpWallDist);
            int drawStart = -lineHeight / 2 + SCREEN_HEIGHT / 2;
            if (drawStart < 0) drawStart = 0;
            int drawEnd = lineHeight / 2 + SCREEN_HEIGHT / 2;
            if (drawEnd >= (int)SCREEN_HEIGHT) drawEnd = SCREEN_HEIGHT - 1;
            
            // Wall color based on side and distance
            sf::Color color = sf::Color(100, 100, 100);
            if (side == 1) color = sf::Color(150, 150, 150);
            
            // Apply distance fog
            double fogFactor = std::min(1.0, perpWallDist / 20.0);
            color.r = (std::uint8_t)(color.r * (1.0 - fogFactor) + 50 * fogFactor);
            color.g = (std::uint8_t)(color.g * (1.0 - fogFactor) + 50 * fogFactor);
            color.b = (std::uint8_t)(color.b * (1.0 - fogFactor) + 50 * fogFactor);
            
            // Draw vertical line
            for (int y = drawStart; y < drawEnd; y++) {
                if (y >= 0 && y < (int)SCREEN_HEIGHT) {
                    screenBuffer.setPixel({(unsigned int)x, (unsigned int)y}, color);
                }
            }
        }
        
        // Sprite rendering (enemies, projectiles, blood)
        struct SpriteDrawInfo {
            double dist;
            double x, y;
            const sf::Texture* tex;
            sf::IntRect rect;
            sf::Color tint;
        };
        std::vector<SpriteDrawInfo> spritesToDraw;
        
        // Add enemies to sprite list
        for (const auto& enemy : enemies) {
            double dx = enemy.x - player.posX;
            double dy = enemy.y - player.posY;
            double dist = sqrt(dx * dx + dy * dy);
            spritesToDraw.push_back({dist, enemy.x, enemy.y, enemy.texture, enemy.textureRect, sf::Color::White});
        }
        
        // Add projectiles
        for (const auto& proj : projectiles) {
            double dx = proj.x - player.posX;
            double dy = proj.y - player.posY;
            double dist = sqrt(dx * dx + dy * dy);
            sf::IntRect rect({0, 0}, {(int)proj.texture->getSize().x, (int)proj.texture->getSize().y});
            spritesToDraw.push_back({dist, proj.x, proj.y, proj.texture, rect, sf::Color::White});
        }
        
        // Sort sprites by distance (far to near)
        std::sort(spritesToDraw.begin(), spritesToDraw.end(), [](const SpriteDrawInfo& a, const SpriteDrawInfo& b) {
            return a.dist > b.dist;
        });
        
        // Draw sprites
        for (const auto& sprite : spritesToDraw) {
            double spriteX = sprite.x - player.posX;
            double spriteY = sprite.y - player.posY;
            
            double invDet = 1.0 / (player.planeX * player.dirY - player.dirX * player.planeY);
            double transformX = invDet * (player.dirY * spriteX - player.dirX * spriteY);
            double transformY = invDet * (-player.planeY * spriteX + player.planeX * spriteY);
            
            if (transformY > 0.1) {
                int spriteScreenX = (int)((SCREEN_WIDTH / 2) * (1 + transformX / transformY));
                int spriteHeight = abs((int)(SCREEN_HEIGHT / transformY));
                int spriteWidth = abs((int)(SCREEN_HEIGHT / transformY));
                
                int drawStartY = -spriteHeight / 2 + SCREEN_HEIGHT / 2;
                if (drawStartY < 0) drawStartY = 0;
                int drawEndY = spriteHeight / 2 + SCREEN_HEIGHT / 2;
                if (drawEndY >= (int)SCREEN_HEIGHT) drawEndY = SCREEN_HEIGHT - 1;
                
                int drawStartX = -spriteWidth / 2 + spriteScreenX;
                if (drawStartX < 0) drawStartX = 0;
                int drawEndX = spriteWidth / 2 + spriteScreenX;
                if (drawEndX >= (int)SCREEN_WIDTH) drawEndX = SCREEN_WIDTH - 1;
                
                for (int stripe = drawStartX; stripe < drawEndX; stripe++) {
                    if (transformY < zBuffer[stripe]) {
                        int texX = (int)((stripe - (-spriteWidth / 2 + spriteScreenX)) * sprite.rect.size.x / spriteWidth);
                        if (texX < 0) texX = 0;
                        if (texX >= sprite.rect.size.x) texX = sprite.rect.size.x - 1;
                        
                        for (int y = drawStartY; y < drawEndY; y++) {
                            int d = y - SCREEN_HEIGHT / 2 + spriteHeight / 2;
                            int texY = d * sprite.rect.size.y / spriteHeight;
                            if (texY < 0) texY = 0;
                            if (texY >= sprite.rect.size.y) texY = sprite.rect.size.y - 1;
                            
                            sf::Color pixel = sprite.tex->copyToImage().getPixel({
                                (unsigned int)(sprite.rect.position.x + texX),
                                (unsigned int)(sprite.rect.position.y + texY)
                            });
                            
                            if (pixel.a > 128) {
                                screenBuffer.setPixel({(unsigned int)stripe, (unsigned int)y}, pixel);
                            }
                        }
                    }
                }
            }
        }
        
        // Update texture and draw to window
        if (!screenTexture.loadFromImage(screenBuffer)) {
            std::cerr << "Failed to load screen buffer\n";
        }
        screenSprite.setTexture(screenTexture);
        
        window.clear();
        window.draw(screenSprite);
        
        // Draw HUD
        healthText.setString("Health: " + std::to_string(player.health));
        scoreText.setString("Score: " + std::to_string(score));
        debugText.setString("Pos: (" + std::to_string((int)player.posX) + "," + std::to_string((int)player.posY) + 
                           ") Dir: (" + std::to_string(player.dirX).substr(0, 4) + "," + std::to_string(player.dirY).substr(0, 4) + 
                           ") FPS: " + std::to_string((int)(1.0f / deltaTime)));
        window.draw(healthText);
        window.draw(scoreText);
        window.draw(ammoText);
        window.draw(debugText);
        window.draw(crosshair);
        
        window.display();
    }
    
    return 0;
}

// === HELPER FUNCTIONS ===

void generateDungeon(int width, int height, std::vector<std::vector<TileType>>& map) {
    map.assign(height, std::vector<TileType>(width, TileType::Wall));
    
    std::mt19937 rng(static_cast<unsigned int>(time(0)));
    
    struct Room {
        int x, y, w, h;
        int centerX() const { return x + w / 2; }
        int centerY() const { return y + h / 2; }
    };
    
    std::vector<Room> rooms;
    
    // Create rooms
    int numRooms = std::uniform_int_distribution<int>(20, 30)(rng);
    
    for (int i = 0; i < numRooms; i++) {
        int roomWidth = std::uniform_int_distribution<int>(6, 15)(rng);
        int roomHeight = std::uniform_int_distribution<int>(6, 15)(rng);
        int roomX = std::uniform_int_distribution<int>(2, width - roomWidth - 2)(rng);
        int roomY = std::uniform_int_distribution<int>(2, height - roomHeight - 2)(rng);
        
        rooms.push_back({roomX, roomY, roomWidth, roomHeight});
        
        // Carve out the room
        for (int y = roomY; y < roomY + roomHeight; y++) {
            for (int x = roomX; x < roomX + roomWidth; x++) {
                map[y][x] = TileType::Empty;
            }
        }
    }
    
    // Create corridors connecting rooms
    for (size_t i = 1; i < rooms.size(); i++) {
        Room& prev = rooms[i - 1];
        Room& curr = rooms[i];
        
        // Horizontal corridor
        int startX = std::min(prev.centerX(), curr.centerX());
        int endX = std::max(prev.centerX(), curr.centerX());
        for (int x = startX; x <= endX; x++) {
            map[prev.centerY()][x] = TileType::Empty;
            // Make corridors wider
            if (prev.centerY() > 0) map[prev.centerY() - 1][x] = TileType::Empty;
            if (prev.centerY() < height - 1) map[prev.centerY() + 1][x] = TileType::Empty;
        }
        
        // Vertical corridor
        int startY = std::min(prev.centerY(), curr.centerY());
        int endY = std::max(prev.centerY(), curr.centerY());
        for (int y = startY; y <= endY; y++) {
            map[y][curr.centerX()] = TileType::Empty;
            // Make corridors wider
            if (curr.centerX() > 0) map[y][curr.centerX() - 1] = TileType::Empty;
            if (curr.centerX() < width - 1) map[y][curr.centerX() + 1] = TileType::Empty;
        }
    }
    
    std::cout << "Generated " << rooms.size() << " rooms\n";
    
    // Count empty spaces for debug
    int emptyCount = 0;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (map[y][x] == TileType::Empty) emptyCount++;
        }
    }
    std::cout << "Total empty tiles: " << emptyCount << " / " << (width * height) << "\n";
}

bool findEmptySpot(const std::vector<std::vector<TileType>>& map, int& x, int& y) {
    std::mt19937 rng(static_cast<unsigned int>(time(0) + x + y * 1000));
    
    for (int attempt = 0; attempt < 100; attempt++) {
        x = std::uniform_int_distribution<int>(1, (int)map[0].size() - 2)(rng);
        y = std::uniform_int_distribution<int>(1, (int)map.size() - 2)(rng);
        if (map[y][x] == TileType::Empty) {
            return true;
        }
    }
    return false;
}
