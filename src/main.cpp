/*
Procedural Engine - Survival!
- Generates a cave-like world using cellular automata
- Player movement and shooting with bullets
- Enemies that spawn and move randomly
- Collision detection with walls, bullets, and enemies
- Scoreboard system to track player score
*/

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

// Define our tile types for better clarity
enum class TileType {
    Floor,
    Wall,
    Water
};

// --- Game Object Structs ---

struct Player {
    sf::RectangleShape shape;
    sf::Vector2f velocity;
    sf::Vector2f facingDirection = {0, -1};
};

struct Bullet {
    sf::RectangleShape shape;
    sf::Vector2f velocity;
};

struct Enemy {
    sf::RectangleShape shape;
    sf::Vector2f velocity;
};

// --- Helper Functions ---

int countWallNeighbors(int x, int y, int width, int height, const std::vector<std::vector<TileType>>& grid) {
    int wallCount = 0;
    for (int neighborY = y - 1; neighborY <= y + 1; ++neighborY) {
        for (int neighborX = x - 1; neighborX <= x + 1; ++neighborX) {
            if (neighborX == x && neighborY == y) continue;
            if (neighborX < 0 || neighborX >= width || neighborY < 0 || neighborY >= height || grid[neighborY][neighborX] == TileType::Wall) {
                wallCount++;
            }
        }
    }
    return wallCount;
}

void generateWorld(int worldWidth, int worldHeight, float tileSize, std::vector<sf::RectangleShape>& tiles, std::vector<std::vector<TileType>>& grid) {
    std::mt19937 rng(static_cast<unsigned int>(time(0)));
    std::uniform_int_distribution<int> noiseDist(0, 100);
    grid.assign(worldHeight, std::vector<TileType>(worldWidth, TileType::Wall));

    int initialWallChance = 45;
    for (int y = 0; y < worldHeight; ++y) for (int x = 0; x < worldWidth; ++x) if (noiseDist(rng) > initialWallChance) grid[y][x] = TileType::Floor;

    int simulationSteps = 5;
    for (int i = 0; i < simulationSteps; ++i) {
        std::vector<std::vector<TileType>> nextGrid = grid;
        for (int y = 0; y < worldHeight; ++y) for (int x = 0; x < worldWidth; ++x) {
            int neighbors = countWallNeighbors(x, y, worldWidth, worldHeight, grid);
            if (neighbors > 4) nextGrid[y][x] = TileType::Wall;
            else if (neighbors < 4) nextGrid[y][x] = TileType::Floor;
        }
        grid = nextGrid;
    }
    
    int waterChance = 5;
    for (int y = 0; y < worldHeight; ++y) for (int x = 0; x < worldWidth; ++x) if (grid[y][x] == TileType::Floor && noiseDist(rng) < waterChance) grid[y][x] = TileType::Water;

    tiles.clear();
    for (int y = 0; y < worldHeight; ++y) {
        for (int x = 0; x < worldWidth; ++x) {
            sf::RectangleShape tileShape({tileSize, tileSize});
            tileShape.setPosition({x * tileSize, y * tileSize});
            switch (grid[y][x]) {
                case TileType::Wall: tileShape.setFillColor(sf::Color(80, 80, 80)); break;
                case TileType::Floor: tileShape.setFillColor(sf::Color(150, 120, 90)); break;
                case TileType::Water: tileShape.setFillColor(sf::Color(50, 50, 200)); break;
            }
            tiles.push_back(tileShape);
        }
    }
}

int main() {
    // --- Window and World Setup ---
    const unsigned int WINDOW_WIDTH = 1280;
    const unsigned int WINDOW_HEIGHT = 720;
    const int WORLD_WIDTH = 100;
    const int WORLD_HEIGHT = 100;
    const float TILE_SIZE = 32.0f;

    sf::RenderWindow window(sf::VideoMode({WINDOW_WIDTH, WINDOW_HEIGHT}), "Procedural Engine - Now with Music!");
    window.setFramerateLimit(60);

    // --- World Generation ---
    std::vector<sf::RectangleShape> tiles;
    std::vector<std::vector<TileType>> grid;
    generateWorld(WORLD_WIDTH, WORLD_HEIGHT, TILE_SIZE, tiles, grid);

    // --- Player Setup ---
    Player player;
    player.shape.setSize({TILE_SIZE * 0.8f, TILE_SIZE * 0.8f});
    player.shape.setFillColor(sf::Color::Cyan);
    float playerSpeed = 200.0f;

    for (int y = 0; y < WORLD_HEIGHT; ++y) for (int x = 0; x < WORLD_WIDTH; ++x) if (grid[y][x] == TileType::Floor) { player.shape.setPosition({x * TILE_SIZE, y * TILE_SIZE}); goto player_placed; }
    player_placed:;

    // --- Game Object Vectors & Shooting Variables ---
    std::vector<Bullet> bullets;
    std::vector<Enemy> enemies;
    float bulletSpeed = 400.0f;
    sf::Clock shootClock;
    const sf::Time shootCooldown = sf::seconds(0.3f);

    // --- Enemy Spawner ---
    sf::Clock enemySpawnClock;
    const sf::Time enemySpawnCooldown = sf::seconds(3.0f);
    const int maxEnemies = 15;
    std::mt19937 rng(static_cast<unsigned int>(time(0)));
    std::uniform_int_distribution<int> dirDist(0, 3);
    float enemySpeed = 100.0f;

    // --- Music Setup ---
    sf::Music music;
    if (!music.openFromFile("res/sfx/music.ogg")) {
        std::cerr << "Error: Could not load music from res/sfx/music.ogg" << std::endl;
        return -1;
    }
    music.setVolume(50);
    music.play(); // Start the music once.

    // --- Scoreboard Setup ---
    int score = 0;
    sf::Font font;
    if (!font.openFromFile("res/arial.ttf")) { std::cerr << "Error: Could not load font" << std::endl; return -1; }
    sf::Text scoreText(font, "Score: 0", 24);
    scoreText.setPosition({10.f, 10.f});

    // --- View (Camera) Setup ---
    sf::View view(sf::FloatRect({0.f, 0.f}, {static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT)}));

    // --- Game Loop ---
    sf::Clock deltaClock;
    while (window.isOpen()) {
        float dt = deltaClock.restart().asSeconds();

        // --- Event Handling ---
        while (const auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();
        }

        // --- NEW: Manual music loop logic ---
        // This is the workaround. We check if the music has stopped, and if so,
        // we start it again from the beginning.
        if (music.getStatus() == sf::SoundSource::Status::Stopped) {
            music.play();
        }

        // --- Player Input and Movement ---
        player.velocity = {0.f, 0.f};
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) player.velocity.y -= playerSpeed;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) player.velocity.y += playerSpeed;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) player.velocity.x -= playerSpeed;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) player.velocity.x += playerSpeed;
        
        if (player.velocity.x != 0 || player.velocity.y != 0) {
            float length = std::sqrt(player.velocity.x * player.velocity.x + player.velocity.y * player.velocity.y);
            if (length > 0) player.facingDirection = {player.velocity.x / length, player.velocity.y / length};
        }

        // --- Shooting Input ---
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space) && shootClock.getElapsedTime() >= shootCooldown) {
            Bullet newBullet;
            newBullet.shape.setSize({TILE_SIZE * 0.3f, TILE_SIZE * 0.3f});
            newBullet.shape.setFillColor(sf::Color::Yellow);
            sf::Vector2f playerCenter = player.shape.getPosition() + player.shape.getSize() / 2.f;
            newBullet.shape.setPosition(playerCenter - newBullet.shape.getSize() / 2.f); 
            newBullet.velocity = player.facingDirection * bulletSpeed;
            bullets.push_back(newBullet);
            shootClock.restart();
        }

        // --- Enemy Spawning Logic ---
        if (enemies.size() < maxEnemies && enemySpawnClock.getElapsedTime() >= enemySpawnCooldown) {
            enemySpawnClock.restart();
            while (true) {
                int randX = std::uniform_int_distribution<int>(0, WORLD_WIDTH - 1)(rng);
                int randY = std::uniform_int_distribution<int>(0, WORLD_HEIGHT - 1)(rng);
                if (grid[randY][randX] == TileType::Floor) {
                    Enemy enemy;
                    enemy.shape.setSize({TILE_SIZE, TILE_SIZE});
                    enemy.shape.setFillColor(sf::Color::Red);
                    enemy.shape.setPosition({randX * TILE_SIZE, randY * TILE_SIZE});
                    enemy.velocity = {0, enemySpeed};
                    enemies.push_back(enemy);
                    break;
                }
            }
        }
        
        // --- Updates and Collision ---
        sf::Vector2f lastGoodPosition = player.shape.getPosition();
        player.shape.move(player.velocity * dt);
        sf::FloatRect playerBounds = player.shape.getGlobalBounds();
        int pStartX = std::max(0, static_cast<int>(playerBounds.position.x / TILE_SIZE));
        int pEndX = std::min(WORLD_WIDTH - 1, static_cast<int>((playerBounds.position.x + playerBounds.size.x) / TILE_SIZE));
        int pStartY = std::max(0, static_cast<int>(playerBounds.position.y / TILE_SIZE));
        int pEndY = std::min(WORLD_HEIGHT - 1, static_cast<int>((playerBounds.position.y + playerBounds.size.y) / TILE_SIZE));

        for (int y = pStartY; y <= pEndY; ++y) {
            for (int x = pStartX; x <= pEndX; ++x) {
                if (grid[y][x] != TileType::Floor) {
                     sf::FloatRect wallBounds({x * TILE_SIZE, y * TILE_SIZE}, {TILE_SIZE, TILE_SIZE});
                     if (playerBounds.findIntersection(wallBounds).has_value()) {
                         player.shape.setPosition(lastGoodPosition);
                     }
                }
            }
        }
        for (auto& bullet : bullets) bullet.shape.move(bullet.velocity * dt);
        for (auto& enemy : enemies) {
            sf::Vector2f enemyLastGoodPosition = enemy.shape.getPosition();
            enemy.shape.move(enemy.velocity * dt);
            sf::FloatRect enemyBounds = enemy.shape.getGlobalBounds();
            int ex = static_cast<int>((enemyBounds.position.x + enemyBounds.size.x / 2) / TILE_SIZE);
            int ey = static_cast<int>((enemyBounds.position.y + enemyBounds.size.y / 2) / TILE_SIZE);
            if (ex < 0 || ex >= WORLD_WIDTH || ey < 0 || ey >= WORLD_HEIGHT || grid[ey][ex] != TileType::Floor) {
                enemy.shape.setPosition(enemyLastGoodPosition);
                int newDir = dirDist(rng);
                if (newDir == 0) enemy.velocity = {0, -enemySpeed};
                else if (newDir == 1) enemy.velocity = {0, enemySpeed};
                else if (newDir == 2) enemy.velocity = {-enemySpeed, 0};
                else enemy.velocity = {enemySpeed, 0};
            }
        }

        bullets.erase(std::remove_if(bullets.begin(), bullets.end(), [&](Bullet& b) {
            sf::FloatRect bulletBounds = b.shape.getGlobalBounds();
            int bx = static_cast<int>(bulletBounds.position.x / TILE_SIZE);
            int by = static_cast<int>(bulletBounds.position.y / TILE_SIZE);
            if (bx < 0 || bx >= WORLD_WIDTH || by < 0 || by >= WORLD_HEIGHT || grid[by][bx] != TileType::Floor) return true;
            for (auto& enemy : enemies) {
                if (enemy.shape.getSize().x == 0) continue;
                if (bulletBounds.findIntersection(enemy.shape.getGlobalBounds()).has_value()) {
                    enemy.shape.setSize({0,0});
                    score += 10;
                    return true;
                }
            }
            return false;
        }), bullets.end());

        enemies.erase(std::remove_if(enemies.begin(), enemies.end(), [](const Enemy& e){ return e.shape.getSize().x == 0; }), enemies.end());
        
        for (const auto& enemy : enemies) {
            if (player.shape.getGlobalBounds().findIntersection(enemy.shape.getGlobalBounds()).has_value()) {
                window.close();
            }
        }

        // --- Update Score Text ---
        scoreText.setString("Score: " + std::to_string(score));

        // --- View and Drawing ---
        view.setCenter(player.shape.getPosition());
        window.clear(sf::Color::Black);
        
        window.setView(view);
        for (const auto& tile : tiles) window.draw(tile);
        for (const auto& bullet : bullets) window.draw(bullet.shape);
        for (const auto& enemy : enemies) window.draw(enemy.shape);
        window.draw(player.shape);
        
        window.setView(window.getDefaultView());
        window.draw(scoreText);

        window.display();
    }

    return 0;
}
