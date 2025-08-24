# 2D Procedural Top-Down Shooter

A simple yet feature-rich 2D top-down shooter built in C++17 with the SFML 3.0 library. This project serves as a practical demonstration of procedural world generation, sprite-based entity management, dynamic enemy spawning, and a complete game loop with scoring and audio.
---

## Table of Contents

1.  [Key Features](#key-features)
2.  [System Architecture](#system-architecture)
    - [Core Technologies](#core-technologies)
    - [World Generation: Cellular Automata](#world-generation-cellular-automata)
    - [Game Loop & State Management](#game-loop--state-management)
    - [Entity Management](#entity-management)
    - [Rendering Pipeline](#rendering-pipeline)
3.  [Getting Started](#getting-started)
    - [Prerequisites](#prerequisites)
    - [Asset Setup (Crucial Step)](#asset-setup-crucial-step)
    - [Build Instructions](#build-instructions)
4.  [How to Play](#how-to-play)
    - [Objective](#objective)
    - [Controls](#controls)
5.  [Project File Structure](#project-file-structure)
6.  [Future Improvements](#future-improvements)

## Key Features

-   **Infinite, Unique Levels:** The world is procedurally generated at runtime using a Cellular Automata algorithm, creating organic, cave-like structures for a new experience every time.
-   **Dynamic Combat:** Face off against a horde of enemies that spawn dynamically over time.
-   **Sprite-Based Graphics:** All game objects—player, enemies, and bullets—are represented by sprites, moving from a geometric prototype to a visually appealing game.
-   **Complete Game Loop:** The game features a full play cycle, from starting the game to a "Game Over" screen that displays your final score.
-   **Scoring System:** Earn points for every enemy you destroy and fight for a high score.
-   **Atmospheric Audio:** A looping background music track sets the mood for the action.
-   **Dynamic Camera:** A smooth `sf::View` follows the player's movement, keeping the action centered on-screen.

## System Architecture

This project is built as a single-file application for simplicity, but it follows several important architectural patterns.

### Core Technologies

-   **Language:** C++17
-   **Graphics & Audio:** [SFML 3.0](https://www.sfml-dev.org/)
-   **Build System:** [CMake](https://cmake.org/)

### World Generation: Cellular Automata

Instead of a simple random walk, the map is generated using a more robust Cellular Automata algorithm, which creates more natural and complex environments. The process works in two phases:

1.  **Random Noise Pass:** The world grid is filled with a random pattern of walls and floors based on a set probability. This creates a chaotic, noisy starting point.
2.  **Smoothing/Simulation Pass:** The algorithm iterates through the map multiple times. In each pass, every tile checks its 8 neighbors. Based on a simple rule ("if a tile has more than 4 wall neighbors, it becomes a wall; otherwise, it becomes a floor"), the noise is smoothed out, allowing larger, organic cave structures and open areas to form.

### Game Loop & State Management

The core of the application is a main `while (window.isOpen())` loop. To manage different phases of the game, a `GameState` enum is used:

-   `GameState::Playing`: In this state, all game logic is active. The player can move and shoot, enemies spawn and move, and collision detection is performed.
-   `GameState::GameOver`: Triggered when the player collides with an enemy. In this state, all game logic is paused. Only the Game Over UI is displayed, and the game waits for user input to close the window.

### Entity Management

The game uses a simple, data-oriented approach for managing game objects. Each entity type is represented by a `struct` that contains its necessary data.

-   `Player`, `Enemy`, `Bullet`: These structs contain an `sf::Sprite` for visuals and an `sf::Vector2f` for velocity. This is a significant step up from the initial `sf::RectangleShape` prototypes.
-   **Dynamic Vectors:** All active entities (enemies, bullets) are stored in `std::vector` containers. The game efficiently removes "dead" entities from these vectors using the `std::remove_if` algorithm.

### Rendering Pipeline

The rendering process is separated into two distinct layers to properly handle the game world and the user interface:

1.  **World Rendering:** The game world (tiles, player, enemies, bullets) is drawn using a `sf::View` that acts as a camera. This view is centered on the player each frame, ensuring the player is always in the middle of the screen.
2.  **UI Rendering:** After the world is drawn, the view is reset to the window's default view. The UI elements (Score, Game Over text) are then drawn on top. This guarantees that the UI remains fixed in place (e.g., in the top-left corner) regardless of where the player is in the world.

## Getting Started

Follow these instructions to compile and run the project on your local machine.

### Prerequisites

-   A C++17 compliant compiler (e.g., GCC, Clang, MSVC).
-   [CMake](https://cmake.org/download/) (version 3.10 or higher).
-   [Git](https://git-scm.com/downloads).
-   (For Linux/NixOS) `openalSoft` or your distribution's OpenAL equivalent, as it is a runtime dependency for SFML Audio.

### Asset Setup (Crucial Step)

The game will not run without the necessary assets. You must create the following folder structure in the root of your project and populate it with your chosen files.

```
/YourProjectRoot/
    └── res/
        ├── arial.ttf           <-- A font file (or any .ttf)
        ├── textures/
        │   ├── player.png
        │   ├── enemy.png
        │   └── bullet.png
        └── sfx/
            └── music.ogg       <-- A looping music track
```

### Build Instructions

1.  **Clone the repository:**
    ```bash
    git clone <your-repository-url>
    cd <repository-name>
    ```

2.  **Create a build directory:**
    ```bash
    mkdir build
    cd build
    ```

3.  **Configure the project with CMake:**
    ```bash
    cmake ..
    ```

4.  **Compile the project:**
    ```bash
    # On Linux or macOS
    make

    # On Windows with Visual Studio
    cmake --build .
    ```
    This will create the executable in the `build/bin/` directory and automatically copy the `res` folder into the `build` directory.

5.  **Run the game:**
    ```bash
    # From within the 'build' directory
    ./bin/main
    ```

## How to Play

### Objective

Survive for as long as you can in the procedurally generated caves. Destroy enemies to increase your score and achieve a new high score! The game ends when you collide with an enemy.

### Controls

| Key     | Action          |
| :------ | :-------------- |
| **W**   | Move Up         |
| **A**   | Move Left       |
| **S**   | Move Down       |
| **D**   | Move Right      |
| **Space** | Shoot           |

## Project File Structure

```
.
├── CMakeLists.txt      # The main CMake build script
├── shell.nix           # (Optional) Nix environment definition
├── src/
│   └── main.cpp        # All game logic is contained here
└── res/
    ├── arial.ttf       # Font for UI text
    ├── textures/
    │   ├── player.png
    │   ├── enemy.png
    │   └── bullet.png
    └── sfx/
        └── music.ogg
```

## Future Improvements

This project provides a solid foundation. Here are some ideas for future expansion:

-   **More Enemy Types:** Introduce enemies with different movement patterns (e.g., chasing, fleeing) or ranged attacks.
-   **Player Health & Power-ups:** Implement a health system for the player and add collectible items like health packs, temporary invincibility, or weapon upgrades.
-   **Sound Effects:** Add sound effects for shooting, enemy hits, and enemy destruction to make the gameplay more satisfying.
-   **Game States:** Expand the state manager to include a Main Menu, a Pause screen, and a "You Win" condition.
-   **Advanced Map Generation:** Enhance the Cellular Automata to generate more distinct features like large open rooms, connecting corridors, or different biomes.
