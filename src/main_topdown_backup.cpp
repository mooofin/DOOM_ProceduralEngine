#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <vector>
#include <random>
#include <ctime>
#include <optional>
#include <cmath>
#include <algorithm>
#include <string>
#include <iostream>

//gaeme entity states (call them back again)
enum class GameState { Loading, Playing, GameOver };
enum class TileType { Grass, Trees, Water };
enum class EnemyType { Wolf, SmokeDemon, TophatOgre, RedDemon };

//player data for all the actions (no enemy logic here )
struct Player {
    sf::Sprite sprite;
    sf::Vector2f velocity;
    sf::Vector2f facingDirection = {0, -1};
    int health = 100;
    sf::Clock damageClock;

    Player(const sf::Texture& texture) : sprite(texture) {}
};

struct Bullet {
    sf::Sprite sprite;
    sf::Vector2f velocity;
    
    Bullet(const sf::Texture& texture) : sprite(texture) {
        sprite.setScale({2.0f, 2.0f});
        sprite.setOrigin({(float)texture.getSize().x / 2.f, (float)texture.getSize().y / 2.f});
    }
};

struct Enemy {
    sf::Sprite sprite;
    sf::Vector2f velocity;
    EnemyType type;
    int health;
    float speed;
    
    Enemy(const sf::Texture& texture, EnemyType t, int hp, float spd) 
        : sprite(texture), type(t), health(hp), speed(spd) {}
};

struct BloodParticle {
    sf::Sprite sprite;
    sf::Vector2f velocity;
    sf::Clock lifeClock;
    float lifetime;
    int frameIndex;
    
    BloodParticle(const sf::Texture& texture) : sprite(texture), lifetime(0.5f), frameIndex(0) {}
};

//loading screen data's 
struct ParallaxLayer {
    sf::Sprite sprite;
    float scrollSpeed;
    float offset;
    
    ParallaxLayer(const sf::Texture& texture, float speed) : sprite(texture), scrollSpeed(speed), offset(0.0f) {
        sprite.setScale({2.0f, 2.0f}); 
    }
};

// --- Helper Functions (Prototypes) ---
sf::Vector2f findValidSpawn(int, int, float, const std::vector<std::vector<TileType>>&);
void generateWorld(int, int, std::vector<std::vector<TileType>>&);
int countTreeNeighbors(int, int, int, int, const std::vector<std::vector<TileType>>&);

int main() {
    //make sfml draw the window (add more options ig?)
    const unsigned int WINDOW_WIDTH = 1280;
    const unsigned int WINDOW_HEIGHT = 720;
    const int WORLD_WIDTH = 200;
    const int WORLD_HEIGHT = 200;
    const float SPRITE_SCALE = 3.0f; 
    const float TILE_SIZE = 16.0f * SPRITE_SCALE;
    const float PLAYER_SCALE = 4.0f;  // Larger player sprite 

    sf::RenderWindow window(sf::VideoMode({WINDOW_WIDTH, WINDOW_HEIGHT}), "Procedural Adventure");
    window.setFramerateLimit(60);

    
    sf::Font font;
    if (!font.openFromFile("res/arial.ttf")) { std::cerr << "Could not load font\n"; return -1; }
    
    // Player and world textures
    sf::Texture playerTexture, overworldTexture; 
    if (!playerTexture.loadFromFile("res/textures/character.png")) { std::cerr << "Could not load character.png\n"; return -1; }
    if (!overworldTexture.loadFromFile("res/textures/world.png")) { std::cerr << "Could not load world.png\n"; return -1; }
    
    // Enemy textures
    sf::Texture wolfTexture, smokeDemonTexture, tophatOgreTexture, redDemonTexture;
    if (!wolfTexture.loadFromFile("res/textures/wolf.png")) { std::cerr << "Could not load wolf.png\n"; return -1; }
    if (!smokeDemonTexture.loadFromFile("res/textures/smoke-demon.png")) { std::cerr << "Could not load smoke-demon.png\n"; return -1; }
    if (!tophatOgreTexture.loadFromFile("res/textures/tophat-ogre.png")) { std::cerr << "Could not load tophat-ogre.png\n"; return -1; }
    if (!redDemonTexture.loadFromFile("res/textures/Demon/Red/ALBUM008_72.png")) { std::cerr << "Could not load red demon\n"; return -1; }
    
    // Blood effect textures
    sf::Texture bloodTextures[4];
    if (!bloodTextures[0].loadFromFile("res/textures/Blood/BLUDA0.png")) { std::cerr << "Could not load blood 0\n"; return -1; }
    if (!bloodTextures[1].loadFromFile("res/textures/Blood/BLUDB0.png")) { std::cerr << "Could not load blood 1\n"; return -1; }
    if (!bloodTextures[2].loadFromFile("res/textures/Blood/BLUDC0.png")) { std::cerr << "Could not load blood 2\n"; return -1; }
    if (!bloodTextures[3].loadFromFile("res/textures/Blood/BLUDD0.png")) { std::cerr << "Could not load blood 3\n"; return -1; }
    
    // Projectile texture
    sf::Texture projectileTexture;
    if (!projectileTexture.loadFromFile("res/textures/Player Projectiles/WIDBALL.cells/000.PNG")) { std::cerr << "Could not load projectile\n"; return -1; }
    
    // the mountain for bg 
    sf::Texture mountainBg, mountainFar, mountainMid, mountainTrees, mountainForeground;
    if (!mountainBg.loadFromFile("res/textures/parallax-mountain-bg.png")) { std::cerr << "Could not load mountain background\n"; return -1; }
    if (!mountainFar.loadFromFile("res/textures/parallax-mountain-montain-far.png")) { std::cerr << "Could not load mountain far\n"; return -1; }
    if (!mountainMid.loadFromFile("res/textures/parallax-mountain-mountains.png")) { std::cerr << "Could not load mountain mid\n"; return -1; }
    if (!mountainTrees.loadFromFile("res/textures/parallax-mountain-trees.png")) { std::cerr << "Could not load mountain trees\n"; return -1; }
    if (!mountainForeground.loadFromFile("res/textures/parallax-mountain-foreground-trees.png")) { std::cerr << "Could not load mountain foreground\n"; return -1; }
    
    playerTexture.setSmooth(false);
    overworldTexture.setSmooth(false);
    wolfTexture.setSmooth(false);
    smokeDemonTexture.setSmooth(false);
    tophatOgreTexture.setSmooth(false);
    redDemonTexture.setSmooth(false);
    for (int i = 0; i < 4; i++) bloodTextures[i].setSmooth(false);
    projectileTexture.setSmooth(false);
    mountainBg.setSmooth(true);
    mountainFar.setSmooth(true);
    mountainMid.setSmooth(true);
    mountainTrees.setSmooth(true);
    mountainForeground.setSmooth(true);

    sf::IntRect grassRect({0, 0}, {16, 16});
    sf::IntRect treesRect({16 * 5, 0}, {16, 16});
    sf::IntRect waterRect({16 * 10, 16 * 20}, {16, 16});
    
    std::vector<std::vector<TileType>> grid;
    generateWorld(WORLD_WIDTH, WORLD_HEIGHT, grid);
    
    sf::Music music;
    if (!music.openFromFile("res/sfx/music.ogg")) { std::cerr << "Could not load music.ogg\n"; return -1; }
    music.setVolume(50);
    
    //loader for the L screen 
    std::vector<ParallaxLayer> parallaxLayers;
    parallaxLayers.emplace_back(mountainBg, 0.1f);        // Background - slowest
    parallaxLayers.emplace_back(mountainFar, 0.2f);       // Far mountains
    parallaxLayers.emplace_back(mountainMid, 0.4f);       // Mid mountains
    parallaxLayers.emplace_back(mountainTrees, 0.6f);     // Trees
    parallaxLayers.emplace_back(mountainForeground, 0.8f); // Foreground - fastest
    
    // Position layers to cover the screen
    for (auto& layer : parallaxLayers) {
        layer.sprite.setPosition({0, WINDOW_HEIGHT - layer.sprite.getGlobalBounds().size.y});
    }
    
    // Loading screen variables
    float loadingProgress = 0.0f;
    sf::Clock loadingClock;
    const float loadingDuration = 3.0f; 
    GameState gameState = GameState::Loading;
    
    // Use one frame definition for player
    sf::IntRect playerFrameRect({64, 240}, {16, 24});

    // player data setup for drawing 
    Player player(playerTexture); 
    player.sprite.setTextureRect(playerFrameRect); 
    player.sprite.setScale({PLAYER_SCALE, PLAYER_SCALE});  // Larger player sprite
    player.sprite.setOrigin({(float)playerFrameRect.size.x / 2.f, (float)playerFrameRect.size.y / 2.f});
    float playerSpeed = 150.0f;
    const sf::Time invincibilityDuration = sf::seconds(1.0f);
    
    
    std::vector<Bullet> bullets;
    std::vector<Enemy> enemies;
    std::vector<BloodParticle> bloodParticles;
    float bulletSpeed = 300.0f;
    int score = 0;
    sf::Clock shootClock, enemySpawnClock;
    const sf::Time enemySpawnCooldown = sf::seconds(2.0f);
    const int maxEnemies = 20;
    
    // Random number generator for enemy variety
    std::mt19937 rng(static_cast<unsigned int>(time(0)));
    std::uniform_int_distribution<int> enemyTypeDist(0, 3);

    // --- UI Elements ---
    sf::Text scoreText(font, "Score: 0", 24);
    sf::Text gameOverText(font, "GAME OVER", 96);
    sf::Text finalScoreText(font, "", 48);
    sf::Text exitText(font, "Press any key to exit", 24);
    sf::RectangleShape healthBarBack({150.f, 15.f});
    sf::RectangleShape healthBarFront = healthBarBack;
    healthBarBack.setFillColor(sf::Color(50,50,50,200));
    healthBarFront.setFillColor(sf::Color::Red);
    sf::RectangleShape gameOverOverlay({(float)WINDOW_WIDTH, (float)WINDOW_HEIGHT});
    gameOverOverlay.setFillColor(sf::Color(0,0,0,150));
    
    // --- Loading Screen UI ---
    sf::Text loadingTitle(font, "PROCEDURAL ADVENTURE", 48);
    
    // Style loading screen elements
    loadingTitle.setFillColor(sf::Color::White);
    loadingTitle.setStyle(sf::Text::Bold);
    
    // Center loading screen elements
    sf::FloatRect titleBounds = loadingTitle.getLocalBounds();
    loadingTitle.setOrigin({titleBounds.size.x / 2.f, titleBounds.size.y / 2.f});
    loadingTitle.setPosition({WINDOW_WIDTH / 2.f, WINDOW_HEIGHT / 2.f});


    auto resetGame = [&]() {
        player.health = 100;
        player.sprite.setPosition(findValidSpawn(WORLD_WIDTH, WORLD_HEIGHT, TILE_SIZE, grid));
        player.damageClock.restart();
        enemies.clear();
        bullets.clear();
        bloodParticles.clear();
        score = 0;
        shootClock.restart();
        enemySpawnClock.restart();
        music.play();
        gameState = GameState::Playing;
    };
    
    sf::View view(sf::FloatRect({0.f, 0.f}, {(float)WINDOW_WIDTH, (float)WINDOW_HEIGHT}));

    //game loop for drawing
    sf::Clock deltaClock;
    while (window.isOpen()) {
        float dt = deltaClock.restart().asSeconds();
        while (const auto event = window.pollEvent()) { 
            if (event->is<sf::Event::Closed>()) window.close();
            if (gameState == GameState::GameOver && event->is<sf::Event::KeyPressed>()) {
                window.close();
            }
        }
        
        
        if (gameState == GameState::Loading) {
            loadingProgress = std::min(1.0f, loadingClock.getElapsedTime().asSeconds() / loadingDuration);
            
            
            for (auto& layer : parallaxLayers) {
                layer.offset += layer.scrollSpeed * dt * 30.0f; 
                float layerWidth = layer.sprite.getGlobalBounds().size.x;
                if (layer.offset >= layerWidth) {
                    layer.offset = 0.0f;
                }
                layer.sprite.setPosition({-layer.offset, layer.sprite.getPosition().y});
            }
            
            // Add subtle fade-in effect for text
            float fadeAlpha = std::min(255.0f, loadingProgress * 255.0f * 2.0f);
            unsigned char alpha = static_cast<unsigned char>(fadeAlpha);
            loadingTitle.setFillColor(sf::Color(255, 255, 255, alpha));
            
            
            if (loadingProgress >= 1.0f) {
                gameState = GameState::Playing;
                resetGame();
            }
        }
        
        if (gameState == GameState::Playing && music.getStatus() == sf::SoundSource::Status::Stopped) { 
            music.play(); 
        }

        //ALLLLL THE CONTROLSSSSS 
        if (gameState == GameState::Playing) {
            
            player.velocity = {0.f, 0.f};
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) player.velocity.y -= 1;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) player.velocity.y += 1;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) player.velocity.x -= 1;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) player.velocity.x += 1;
            
            sf::Vector2f moveDir = {0.f, 0.f};
            if(player.velocity.x != 0 || player.velocity.y != 0){
                float length = std::sqrt(player.velocity.x * player.velocity.x + player.velocity.y * player.velocity.y);
                moveDir = sf::Vector2f(player.velocity.x/length, player.velocity.y/length);
                player.facingDirection = moveDir; 
            }
            float currentSpeed = playerSpeed;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Enter)) { currentSpeed *= 2.0f; }
            player.sprite.move(moveDir * currentSpeed * dt);
            
            //kabbom shoot yall
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space) && shootClock.getElapsedTime() >= sf::seconds(0.5f)) {
                Bullet newBullet(projectileTexture);
                newBullet.sprite.setPosition(player.sprite.getPosition());
                newBullet.velocity = player.facingDirection * bulletSpeed;
                bullets.push_back(newBullet);
                shootClock.restart();
            }
            
            //enemy :3
            if (enemies.size() < maxEnemies && enemySpawnClock.getElapsedTime() >= enemySpawnCooldown) {
                enemySpawnClock.restart();
                
                // Randomly select enemy type
                int typeRoll = enemyTypeDist(rng);
                EnemyType type = static_cast<EnemyType>(typeRoll);
                
                const sf::Texture* enemyTex;
                sf::IntRect spriteRect;
                int health;
                float speed;
                float scale = 2.0f;
                
                switch(type) {
                    case EnemyType::Wolf:
                        enemyTex = &wolfTexture;
                        // Wolf spritesheet: 1024x384, appears to be 8x3 grid = 128x128 per sprite
                        spriteRect = sf::IntRect({0, 0}, {128, 128});
                        health = 30;
                        speed = 120.0f;
                        scale = 2.0f;
                        break;
                    case EnemyType::SmokeDemon:
                        enemyTex = &smokeDemonTexture;
                        // Smoke Demon: 960x512, appears to be 6x4 grid = 160x128 per sprite
                        spriteRect = sf::IntRect({0, 0}, {160, 128});
                        health = 50;
                        speed = 80.0f;
                        scale = 2.5f;
                        break;
                    case EnemyType::TophatOgre:
                        enemyTex = &tophatOgreTexture;
                        // Tophat Ogre: 960x512, appears to be 6x4 grid = 160x128 per sprite
                        spriteRect = sf::IntRect({0, 0}, {160, 128});
                        health = 70;
                        speed = 60.0f;
                        scale = 2.0f;
                        break;
                    case EnemyType::RedDemon:
                        enemyTex = &redDemonTexture;
                        // Red Demon: single sprite (not a sheet)
                        spriteRect = sf::IntRect({0, 0}, {(int)redDemonTexture.getSize().x, (int)redDemonTexture.getSize().y});
                        health = 100;
                        speed = 50.0f;
                        scale = 3.0f;
                        break;
                }
                
                Enemy enemy(*enemyTex, type, health, speed);
                enemy.sprite.setTextureRect(spriteRect);
                enemy.sprite.setScale({scale, scale});
                enemy.sprite.setOrigin({(float)spriteRect.size.x / 2.f, (float)spriteRect.size.y / 2.f});
                enemy.sprite.setPosition(findValidSpawn(WORLD_WIDTH, WORLD_HEIGHT, TILE_SIZE, grid));
                enemies.push_back(enemy);
            }

            // Updates & Collision.
            for (auto& enemy : enemies) {
                sf::Vector2f dirToPlayer = player.sprite.getPosition() - enemy.sprite.getPosition();
                float dist = std::sqrt(dirToPlayer.x * dirToPlayer.x + dirToPlayer.y * dirToPlayer.y);
                if (dist > 0) { 
                    enemy.sprite.move({dirToPlayer.x/dist * enemy.speed * dt, dirToPlayer.y/dist * enemy.speed * dt});
                }
            }
            
            for (auto& bullet : bullets) bullet.sprite.move(bullet.velocity * dt);
            
            // Bullet collision with terrain and enemies
            bullets.erase(std::remove_if(bullets.begin(), bullets.end(), [&](Bullet& b) {
                sf::Vector2i tileCoords((int)b.sprite.getPosition().x / TILE_SIZE, (int)b.sprite.getPosition().y / TILE_SIZE);
                
                int wrappedX = ((tileCoords.x % WORLD_WIDTH) + WORLD_WIDTH) % WORLD_WIDTH;
                int wrappedY = ((tileCoords.y % WORLD_HEIGHT) + WORLD_HEIGHT) % WORLD_HEIGHT;
                if(grid[wrappedY][wrappedX] != TileType::Grass) return true;

                for (auto& enemy : enemies) {
                    if (enemy.health > 0 && b.sprite.getGlobalBounds().findIntersection(enemy.sprite.getGlobalBounds()).has_value()) {
                        enemy.health -= 25;
                        
                        // Spawn blood particles
                        for (int i = 0; i < 3; i++) {
                            BloodParticle blood(bloodTextures[0]);
                            blood.sprite.setOrigin({(float)bloodTextures[0].getSize().x / 2.f, (float)bloodTextures[0].getSize().y / 2.f});
                            blood.sprite.setPosition(enemy.sprite.getPosition());
                            blood.sprite.setScale({2.0f, 2.0f});
                            
                            std::uniform_real_distribution<float> angleDist(0.0f, 6.28318f);
                            std::uniform_real_distribution<float> speedDist(50.0f, 150.0f);
                            float angle = angleDist(rng);
                            float particleSpeed = speedDist(rng);
                            blood.velocity = {std::cos(angle) * particleSpeed, std::sin(angle) * particleSpeed};
                            blood.lifetime = std::uniform_real_distribution<float>(0.3f, 0.6f)(rng);
                            bloodParticles.push_back(blood);
                        }
                        
                        if (enemy.health <= 0) {
                            score += 10 + static_cast<int>(enemy.type) * 10; // More points for tougher enemies
                        }
                        return true;
                    }
                }
                return false;
            }), bullets.end());
            
            // Update blood particles
            for (auto& blood : bloodParticles) {
                blood.sprite.move(blood.velocity * dt);
                blood.velocity *= 0.95f; // Friction
                
                // Animate through blood frames
                float elapsed = blood.lifeClock.getElapsedTime().asSeconds();
                blood.frameIndex = std::min(3, static_cast<int>(elapsed / (blood.lifetime / 4.0f)));
                blood.sprite.setTexture(bloodTextures[blood.frameIndex]);
                
                // Fade out
                float alpha = std::max(0.0f, 1.0f - (elapsed / blood.lifetime));
                blood.sprite.setColor(sf::Color(255, 255, 255, static_cast<std::uint8_t>(alpha * 255)));
            }
            
            // Remove expired blood particles
            bloodParticles.erase(std::remove_if(bloodParticles.begin(), bloodParticles.end(), 
                [](const BloodParticle& b) { return b.lifeClock.getElapsedTime().asSeconds() >= b.lifetime; }), 
                bloodParticles.end());
            
            enemies.erase(std::remove_if(enemies.begin(), enemies.end(), [](const Enemy& e){ return e.health <= 0; }), enemies.end());

            bool isInvincible = player.damageClock.getElapsedTime() < invincibilityDuration;
            if (!isInvincible) {
                for (auto& enemy : enemies) {
                    if (player.sprite.getGlobalBounds().findIntersection(enemy.sprite.getGlobalBounds()).has_value()) {
                        player.health -= 25;
                        player.damageClock.restart();
                        sf::Vector2f dir = player.sprite.getPosition() - enemy.sprite.getPosition();
                        float len = std::sqrt(dir.x*dir.x + dir.y*dir.y);
                        if (len > 0) enemy.sprite.move(-dir/len * TILE_SIZE);

                        if(player.health <= 0) {
                            player.health = 0;
                            gameState = GameState::GameOver;
                            music.stop();
                            finalScoreText.setString("Final Score: " + std::to_string(score));
                            sf::FloatRect textBounds = finalScoreText.getLocalBounds();
                            finalScoreText.setOrigin({textBounds.size.x / 2.f, textBounds.size.y / 2.f});
                            finalScoreText.setPosition({WINDOW_WIDTH / 2.f, WINDOW_HEIGHT / 2.f + 50});
                            textBounds = gameOverText.getLocalBounds();
                            gameOverText.setOrigin({textBounds.size.x / 2.f, textBounds.size.y / 2.f});
                            gameOverText.setPosition({WINDOW_WIDTH / 2.f, WINDOW_HEIGHT / 2.f - 50});
                            textBounds = exitText.getLocalBounds();
                            exitText.setOrigin({textBounds.size.x / 2.f, textBounds.size.y / 2.f});
                            exitText.setPosition({WINDOW_WIDTH / 2.f, WINDOW_HEIGHT / 2.f + 120});
                        }
                        break; 
                    }
                }
            }

            if (isInvincible) {
                bool flash = static_cast<int>(player.damageClock.getElapsedTime().asMilliseconds() / 100) % 2 == 0;
                player.sprite.setColor(flash ? sf::Color(255, 255, 255, 100) : sf::Color::White);
            } else {
                player.sprite.setColor(sf::Color::White);
            }

            scoreText.setString("Score: " + std::to_string(score));
            healthBarFront.setSize({(float)player.health/100.f * 150.f, 15.f});
        }

        
        window.clear(sf::Color(116, 182, 53));
        
        // --- Loading Screen Drawing ---
        if (gameState == GameState::Loading) {
            window.setView(window.getDefaultView());
            
            // Draw parallax layers
            for (const auto& layer : parallaxLayers) {
                // Draw multiple copies for seamless scrolling
                float layerWidth = layer.sprite.getGlobalBounds().size.x;
                int copiesNeeded = static_cast<int>(WINDOW_WIDTH / layerWidth) + 2;
                
                for (int i = 0; i < copiesNeeded; ++i) {
                    sf::Sprite copy = layer.sprite;
                    copy.setPosition({layer.sprite.getPosition().x + (i * layerWidth), layer.sprite.getPosition().y});
                    window.draw(copy);
                }
            }
            
            // Draw loading screen UI
            window.draw(loadingTitle);
        } else {
            // --- Game Drawing ---
            view.setCenter(player.sprite.getPosition());
            window.setView(view);

            sf::Sprite tileSprite(overworldTexture);
            tileSprite.setScale({SPRITE_SCALE, SPRITE_SCALE});
            
            // Calculate visible tile range with extra padding for seamless rendering
            int startX = (int)((view.getCenter().x - view.getSize().x / 2) / TILE_SIZE) - 2;
            int endX = (int)((view.getCenter().x + view.getSize().x / 2) / TILE_SIZE) + 4;
            int startY = (int)((view.getCenter().y - view.getSize().y / 2) / TILE_SIZE) - 2;
            int endY = (int)((view.getCenter().y + view.getSize().y / 2) / TILE_SIZE) + 4;

            for (int y = startY; y < endY; ++y) {
                for (int x = startX; x < endX; ++x) {
                    // Use modulo to wrap coordinates, creating infinite world effect
                    int wrappedX = ((x % WORLD_WIDTH) + WORLD_WIDTH) % WORLD_WIDTH;
                    int wrappedY = ((y % WORLD_HEIGHT) + WORLD_HEIGHT) % WORLD_HEIGHT;
                    
                    switch (grid[wrappedY][wrappedX]) {
                        case TileType::Grass: tileSprite.setTextureRect(grassRect); break;
                        case TileType::Trees: tileSprite.setTextureRect(treesRect); break;
                        case TileType::Water: tileSprite.setTextureRect(waterRect); break;
                    }
                    tileSprite.setPosition({(float)x * TILE_SIZE, (float)y * TILE_SIZE});
                    window.draw(tileSprite);
                }
            }
            
            for (const auto& bullet : bullets) window.draw(bullet.sprite);
            for (const auto& blood : bloodParticles) window.draw(blood.sprite);
            for (const auto& enemy : enemies) window.draw(enemy.sprite);
            window.draw(player.sprite);
            
            window.setView(window.getDefaultView());
            scoreText.setPosition({10, 10});
            healthBarBack.setPosition({10, 40});
            healthBarFront.setPosition({10, 40});
            window.draw(scoreText);
            window.draw(healthBarBack);
            window.draw(healthBarFront);

            if (gameState == GameState::GameOver) { 
                window.draw(gameOverOverlay);
                window.draw(gameOverText);
                window.draw(finalScoreText);
                window.draw(exitText);
            }
        }
        window.display();
    }
    return 0;
}

//  ALL THESE ARE THE MAIN DS FOCUSED FUNCTIONS 



sf::Vector2f findValidSpawn(int worldWidth, int worldHeight, float tileSize, const std::vector<std::vector<TileType>>& grid){
    std::mt19937 rng(static_cast<unsigned int>(time(0)));
    while(true){
        int x = std::uniform_int_distribution<int>(0, worldWidth - 1)(rng);
        int y = std::uniform_int_distribution<int>(0, worldHeight - 1)(rng);
        if(grid[y][x] == TileType::Grass){
            return {(float)x * tileSize + tileSize / 2, (float)y * tileSize + tileSize / 2};
        }
    }
}
void generateWorld(int worldWidth, int worldHeight, std::vector<std::vector<TileType>>& grid) {
    std::mt19937 rng(static_cast<unsigned int>(time(0)));
    std::uniform_int_distribution<int> noiseDist(0, 100);
    grid.assign(worldHeight, std::vector<TileType>(worldWidth, TileType::Trees));
    int initialTreeChance = 45;
    
    // Generate base pattern
    for (int y = 0; y < worldHeight; ++y) {
        for (int x = 0; x < worldWidth; ++x) {
            // Use a combination of noise and patterns for more natural distribution
            int noise = noiseDist(rng);
            int patternNoise = ((x * 7 + y * 11) % 100);
            int combinedNoise = (noise + patternNoise) / 2;
            
            if (combinedNoise > initialTreeChance) {
                grid[y][x] = TileType::Grass;
            }
        }
    }
    

/*Cellular automata is used to create natural clusters of trees and grass.

For 5 simulation steps:

Count the number of neighboring tree tiles (countTreeNeighbors).

If more than 4 neighbors are Trees → current tile becomes Trees.

If fewer than 4 neighbors are Trees → current tile becomes Grass.

This smooths the world and forms clusters instead of random noise 

*/


/* Cellular automata are grid-based simulations where each cell’s state (like grass, trees, or water) changes over time based on simple rules and the states of its neighbors*/
    int simulationSteps = 5;
    for (int i = 0; i < simulationSteps; ++i) {
        std::vector<std::vector<TileType>> nextGrid = grid;
        for (int y = 0; y < worldHeight; ++y) {
            for (int x = 0; x < worldWidth; ++x) {
                int neighbors = countTreeNeighbors(x, y, worldWidth, worldHeight, grid);
                if (neighbors > 4) nextGrid[y][x] = TileType::Trees;
                else if (neighbors < 4) nextGrid[y][x] = TileType::Grass;
            }
        }
        grid = nextGrid;
    }
    
   
    for (int y = 0; y < worldHeight; ++y) {
        for (int x = 0; x < worldWidth; ++x) {
            if (grid[y][x] == TileType::Grass) {
                int waterChance = ((x * 13 + y * 17) % 100);
                if (waterChance < 3) { // 3% chance for water
                    grid[y][x] = TileType::Water;
                }
            }
        }
    }
}


/*Used to smooth the map and form realistic tree/grass clusters.*/
int countTreeNeighbors(int x, int y, int width, int height, const std::vector<std::vector<TileType>>& grid) {
    int treeCount = 0;
    for (int ny = y - 1; ny <= y + 1; ++ny) {
        for (int nx = x - 1; nx <= x + 1; ++nx) {
            if (nx == x && ny == y) continue;
            if (nx < 0 || nx >= width || ny < 0 || ny >= height || grid[ny][nx] == TileType::Trees) {
                treeCount++;
            }
        }
    }
    return treeCount;
}
