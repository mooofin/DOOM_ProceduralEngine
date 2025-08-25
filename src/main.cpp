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

// --- Game State & Entity Enums ---
enum class GameState { Playing, GameOver };
enum class TileType { Grass, Trees, Water };

// --- Game Object Structs (with constructors) ---
struct Player {
    sf::Sprite sprite;
    // FIX: Changed '.' to '::' for namespace access
    sf::Vector2f velocity;
    sf::Vector2f facingDirection = {0, -1};
    int health = 100;

    Player(const sf::Texture& texture) : sprite(texture) {}
};

struct Bullet {
    sf::CircleShape shape;
    sf::Vector2f velocity;
};

struct Enemy {
    sf::Sprite sprite;
    sf::Vector2f velocity;

    Enemy(const sf::Texture& texture) : sprite(texture) {}
};


// --- Helper Functions (Prototypes) ---
sf::Vector2f findValidSpawn(int, int, float, const std::vector<std::vector<TileType>>&);
void generateWorld(int, int, std::vector<std::vector<TileType>>&);
int countTreeNeighbors(int, int, int, int, const std::vector<std::vector<TileType>>&);

int main() {
    // --- Window and World Setup ---
    const unsigned int WINDOW_WIDTH = 1280;
    const unsigned int WINDOW_HEIGHT = 720;
    const int WORLD_WIDTH = 100;
    const int WORLD_HEIGHT = 100;
    const float SPRITE_SCALE = 2.0f; 
    const float TILE_SIZE = 16.0f * SPRITE_SCALE; 

    sf::RenderWindow window(sf::VideoMode({WINDOW_WIDTH, WINDOW_HEIGHT}), "Procedural Adventure");
    window.setFramerateLimit(60);

    // --- Asset Loading ---
    sf::Font font;
    if (!font.openFromFile("res/arial.ttf")) { std::cerr << "Could not load font\n"; return -1; }

    sf::Texture playerTexture, enemyTexture, overworldTexture;
    if (!playerTexture.loadFromFile("res/textures/character.png")) { std::cerr << "Could not load character.png\n"; return -1; }
    if (!enemyTexture.loadFromFile("res/textures/npc.png")) { std::cerr << "Could not load npc.png\n"; return -1; }
    if (!overworldTexture.loadFromFile("res/textures/overworld.png")) { std::cerr << "Could not load overworld.png\n"; return -1; }
    
    playerTexture.setSmooth(false);
    enemyTexture.setSmooth(false);
    overworldTexture.setSmooth(false);

    // Using Vector2i for sf::IntRect constructors for SFML 3
    sf::IntRect grassRect({0, 0}, {16, 16});
    sf::IntRect treesRect({16 * 5, 0}, {16, 16});
    sf::IntRect waterRect({16 * 10, 16 * 20}, {16, 16});

    std::vector<std::vector<TileType>> grid;
    generateWorld(WORLD_WIDTH, WORLD_HEIGHT, grid);

    sf::Music music;
    if (!music.openFromFile("res/sfx/music.ogg")) { std::cerr << "Could not load music.ogg\n"; return -1; }
    music.setVolume(50);

    // --- Player Setup ---
    Player player(playerTexture); 
    player.sprite.setScale({SPRITE_SCALE, SPRITE_SCALE});
    player.sprite.setOrigin({playerTexture.getSize().x / 2.f, playerTexture.getSize().y / 2.f});
    float playerSpeed = 150.0f;
    
    // --- Game Objects ---
    std::vector<Bullet> bullets;
    std::vector<Enemy> enemies;
    float bulletSpeed = 300.0f;
    GameState gameState = GameState::Playing;
    int score = 0;
    sf::Clock shootClock, enemySpawnClock;
    const sf::Time enemySpawnCooldown = sf::seconds(4.0f);
    const int maxEnemies = 10;
    
    // --- UI Elements ---
    sf::Text scoreText(font, "Score: 0", 24);
    sf::Text gameOverText(font, "GAME OVER", 96);
    sf::Text finalScoreText(font);
    sf::Text exitText(font, "Press any key to exit", 24);
    sf::RectangleShape healthBarBack({150.f, 15.f});
    sf::RectangleShape healthBarFront = healthBarBack;
    healthBarBack.setFillColor(sf::Color(50,50,50,200));
    healthBarFront.setFillColor(sf::Color::Red);

    auto resetGame = [&]() {
        player.health = 100;
        player.sprite.setPosition(findValidSpawn(WORLD_WIDTH, WORLD_HEIGHT, TILE_SIZE, grid));
        enemies.clear();
        bullets.clear();
        score = 0;
        shootClock.restart();
        enemySpawnClock.restart();
        music.play();
    };
    resetGame(); 

    sf::View view(sf::FloatRect({0.f, 0.f}, {(float)WINDOW_WIDTH, (float)WINDOW_HEIGHT}));

    // --- Main Game Loop ---
    sf::Clock deltaClock;
    while (window.isOpen()) {
        float dt = deltaClock.restart().asSeconds();

        while (const auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();
            if (gameState == GameState::GameOver && event->is<sf::Event::KeyPressed>()) {
                window.close();
            }
        }

        if (music.getStatus() == sf::SoundSource::Status::Stopped) { music.play(); }

        if (gameState == GameState::Playing) {
            // --- Player Input ---
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
            player.sprite.move(moveDir * playerSpeed * dt);
            
            // --- Player Shooting ---
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space) && shootClock.getElapsedTime() >= sf::seconds(0.5f)) {
                Bullet newBullet;
                newBullet.shape.setRadius(6.f);
                newBullet.shape.setFillColor(sf::Color(255, 255, 100, 200));
                newBullet.shape.setOrigin({6.f, 6.f});
                newBullet.shape.setPosition(player.sprite.getPosition());
                newBullet.velocity = player.facingDirection * bulletSpeed;
                bullets.push_back(newBullet);
                shootClock.restart();
            }
            
            // --- Enemy Spawning ---
            if (enemies.size() < maxEnemies && enemySpawnClock.getElapsedTime() >= enemySpawnCooldown) {
                enemySpawnClock.restart();
                Enemy enemy(enemyTexture);
                enemy.sprite.setScale({SPRITE_SCALE, SPRITE_SCALE});
                enemy.sprite.setOrigin({enemyTexture.getSize().x/2.f, enemyTexture.getSize().y/2.f});
                enemy.sprite.setPosition(findValidSpawn(WORLD_WIDTH, WORLD_HEIGHT, TILE_SIZE, grid));
                enemies.push_back(enemy);
            }

            // --- Updates ---
            for (auto& enemy : enemies) {
                sf::Vector2f dirToPlayer = player.sprite.getPosition() - enemy.sprite.getPosition();
                float dist = std::sqrt(dirToPlayer.x * dirToPlayer.x + dirToPlayer.y * dirToPlayer.y);
                if (dist > 0) { 
                    enemy.sprite.move({dirToPlayer.x/dist * 70.f * dt, dirToPlayer.y/dist * 70.f * dt});
                }
            }
            for (auto& bullet : bullets) bullet.shape.move(bullet.velocity * dt);

            // --- Collision Detection ---
            bullets.erase(std::remove_if(bullets.begin(), bullets.end(), [&](Bullet& b) {
                for (auto& enemy : enemies) {
                    if (enemy.sprite.getScale().x > 0 && b.shape.getGlobalBounds().findIntersection(enemy.sprite.getGlobalBounds()).has_value()) {
                        enemy.sprite.setScale({0,0});
                        score += 10;
                        return true;
                    }
                }
                return false;
            }), bullets.end());
            enemies.erase(std::remove_if(enemies.begin(), enemies.end(), [](const Enemy& e){ return e.sprite.getScale().x == 0; }), enemies.end());

            for (const auto& enemy : enemies) {
                if (player.sprite.getGlobalBounds().findIntersection(enemy.sprite.getGlobalBounds()).has_value()) {
                    player.health -= 25;
                    if(player.health <= 0) {
                        gameState = GameState::GameOver;
                        music.stop();
                        finalScoreText.setString("Final Score: " + std::to_string(score));
                    }
                }
            }

            scoreText.setString("Score: " + std::to_string(score));
            healthBarFront.setSize({(float)player.health/100.f * 150.f, 15.f});
        } // End of GameState::Playing

        // --- Drawing ---
        window.clear(sf::Color(116, 182, 53));
        view.setCenter(player.sprite.getPosition());
        window.setView(view);

        // Tilemap Rendering
        sf::Sprite tileSprite(overworldTexture);
        tileSprite.setScale({SPRITE_SCALE, SPRITE_SCALE});

        int startX = std::max(0, (int)((view.getCenter().x - view.getSize().x / 2) / TILE_SIZE));
        int endX = std::min(WORLD_WIDTH, (int)((view.getCenter().x + view.getSize().x / 2) / TILE_SIZE) + 2);
        int startY = std::max(0, (int)((view.getCenter().y - view.getSize().y / 2) / TILE_SIZE));
        int endY = std::min(WORLD_HEIGHT, (int)((view.getCenter().y + view.getSize().y / 2) / TILE_SIZE) + 2);

        for (int y = startY; y < endY; ++y) {
            for (int x = startX; x < endX; ++x) {
                switch (grid[y][x]) {
                    case TileType::Grass: tileSprite.setTextureRect(grassRect); break;
                    case TileType::Trees: tileSprite.setTextureRect(treesRect); break;
                    case TileType::Water: tileSprite.setTextureRect(waterRect); break;
                }
                tileSprite.setPosition({(float)x * TILE_SIZE, (float)y * TILE_SIZE});
                window.draw(tileSprite);
            }
        }
        
        // Draw Game Objects
        for (const auto& bullet : bullets) window.draw(bullet.shape);
        for (const auto& enemy : enemies) window.draw(enemy.sprite);
        window.draw(player.sprite);
        
        // Draw UI
        window.setView(window.getDefaultView());
        scoreText.setPosition({10, 10});
        healthBarBack.setPosition({10, 40});
        healthBarFront.setPosition({10, 40});
        window.draw(scoreText);
        window.draw(healthBarBack);
        window.draw(healthBarFront);

        if (gameState == GameState::GameOver) { /* ... Draw Game Over UI ... */ }
        window.display();
    }
    return 0;
}


// --- Full Function Implementations ---
sf::Vector2f findValidSpawn(int worldWidth, int worldHeight, float tileSize, const std::vector<std::vector<TileType>>& grid){
    std::mt19937 rng(static_cast<unsigned int>(time(0)));
    while(true){
        int x = std::uniform_int_distribution<int>(0, worldWidth - 1)(rng);
        int y = std::uniform_int_distribution<int>(0, worldHeight - 1)(rng);
        if(grid[y][x] == TileType::Grass){
            // FIX: Use the 'tileSize' parameter, not the missing 'TILE_SIZE' constant
            return {(float)x * tileSize + tileSize / 2, (float)y * tileSize + tileSize / 2};
        }
    }
}
void generateWorld(int worldWidth, int worldHeight, std::vector<std::vector<TileType>>& grid) {
    std::mt19937 rng(static_cast<unsigned int>(time(0)));
    std::uniform_int_distribution<int> noiseDist(0, 100);
    grid.assign(worldHeight, std::vector<TileType>(worldWidth, TileType::Trees));

    int initialTreeChance = 45;
    for (int y = 0; y < worldHeight; ++y) for (int x = 0; x < worldWidth; ++x) if (noiseDist(rng) > initialTreeChance) grid[y][x] = TileType::Grass;

    int simulationSteps = 5;
    for (int i = 0; i < simulationSteps; ++i) {
        std::vector<std::vector<TileType>> nextGrid = grid;
        for (int y = 0; y < worldHeight; ++y) for (int x = 0; x < worldWidth; ++x) {
            int neighbors = countTreeNeighbors(x, y, worldWidth, worldHeight, grid);
            if (neighbors > 4) nextGrid[y][x] = TileType::Trees;
            else if (neighbors < 4) nextGrid[y][x] = TileType::Grass;
        }
        grid = nextGrid;
    }
}
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