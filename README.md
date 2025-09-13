# 2D Procedural Adventure

Welcome to Procedural Adventure! This is a simple yet powerful 2D top-down shooter built from the ground up in C++ and SFML. The game features an infinitely replayable world, dynamic enemies, and a complete game loop, all built on fundamental data structures and algorithms.

<img width="1200" height="676" alt="image" src="https://github.com/user-attachments/assets/5e2a82a2-f9a4-4f8b-b053-8c586ca75c84" />


This project was developed to serve as a practical demonstration of procedural generation and real-time 
entity management within a game context.

<img width="1920" height="1080" alt="screenshot-1757774764" src="https://github.com/user-attachments/assets/66e6d72f-1829-4b7d-9a53-42dd617e522f" />
---

## Table of Contents

1.  [Key Features](#key-features)
2.  [How it Works: A Technical Deep Dive](#how-it-works-a-technical-deep-dive)
    - [The Core: Data Structures & Algorithms](#the-core-data-structures--algorithms)
    - [World Generation: Cellular Automata](#world-generation-cellular-automata)
    - [Rendering Pipeline: The Art of Spritesheets](#rendering-pipeline-the-art-of-spritesheets)
3.  [Setting Up Your System](#setting-up-your-system)
    - [Prerequisites](#prerequisites)
    - [Asset Setup (The Most Important Step!)](#asset-setup-the-most-important-step)
    - [Build Instructions (Using Nix)](#build-instructions-using-nix)
4.  [How to Play](#how-to-play)
    - [Objective](#objective)
    - [Controls](#controls)
5.  [Code Structure Overview](#code-structure-overview)

## Key Features

-   **Explore Infinite Worlds:** Every time you play, a brand new map is generated using a Cellular Automata algorithm, creating organic, cave-like environments.
-   **Dynamic Combat:** Fight against an ever-growing horde of enemies that spawn over time and hunt you down.
-   **Sprite-Based Visuals:** The game uses classic, Zelda-themed sprites for the player, enemies, and the world itself, all rendered from spritesheets.
-   **Player Progression:** A simple health and scoring system tracks your performance.
-   **Full Game Loop:** Features a "Playing" state and a "Game Over" screen that displays your final score.
-   **Atmospheric Audio:** A looping background music track sets the mood for adventure.

## How it Works: A Technical Deep Dive

This project might be in a single file, but it's built on several key computer science concepts.

### The Core: Data Structures & Algorithms

This game is a direct application of fundamental data structures.

-   **The World Grid (`std::vector<std::vector<TileType>>`):** The entire game world is represented by a 2D matrix (a vector of vectors). This is a highly efficient data structure for grid-based worlds because it gives us constant-time `O(1)` access to any tile's data just by using its `[y][x]` coordinates. This is essential for fast rendering and collision checks.

-   **Dynamic Entity Management (`std::vector<Enemy>` and `std::vector<Bullet>`):** We don't know how many enemies or bullets will be on screen at any given moment. An `std::vector` is the perfect choice here. It's a dynamic array that can grow as new enemies spawn and shrink as they are defeated.

-   **The Erase-Remove Idiom (`std::remove_if`):** To efficiently remove "dead" enemies or bullets that have hit a wall, the code uses the standard C++ algorithm `std::remove_if`. This algorithm iterates through the vector, moves all the "dead" elements to the end in a single pass (`O(n)` complexity), and then the `vector::erase` method cleans them up. This is far more performant than finding and erasing elements one by one.

### World Generation: Cellular Automata

To create natural-looking caves instead of rigid corridors, the world is generated using a Cellular Automata algorithm. This is a fascinating process that mimics natural patterns.

1.  **Random Noise:** First, the world grid is filled with a random pattern of "Trees" (walls) and "Grass" (floors). This looks like static on a TV screen.
2.  **Simulation/Smoothing:** The algorithm then runs through several simulation steps. In each step, every single tile looks at its 8 immediate neighbors and follows a simple rule:
    -   "If I am a tree and have 4 or more tree neighbors, I stay a tree."
    -   "If I am a floor and have 5 or more tree neighbors, I become a tree."
    -   Otherwise, I become a floor.
    This process naturally carves out large, open caverns and smooths out the initial noise into a playable map.

### Rendering Pipeline: The Art of Spritesheets

The game's visuals are not just colored squares; they are individual images "cut out" from larger spritesheet files.

-   **`sf::IntRect`:** A simple struct holding four integers (`left`, `top`, `width`, `height`) that defines a rectangular area on a texture. We define one of these for each tile type we want to draw (e.g., `grassRect`, `treesRect`).
-   **`sprite.setTextureRect(rect)`:** This is the magic function. Before drawing a sprite, we tell it which `IntRect` to use. This instructs the GPU to only render the small portion of the spritesheet defined by our rectangle, effectively letting us "cut out" the specific tile we need.
-   **The Camera (`sf::View`):** The game world is much larger than the window. To handle this, we use a `sf::View`, which acts as a 2D camera. Every frame, we center this view on the player's position. When we tell SFML to draw using this view, it automatically handles drawing the correct part of the world, giving the illusion of a smooth-scrolling camera.

## Setting Up Your System

Follow these steps to get the game running on your machine.

### Prerequisites

-   A C++17 compliant compiler (GCC, Clang, etc.).
-   **CMake** (version 3.10+).
-   **Git**.
-   **Nix:** The project is configured to be built within a Nix shell, which manages all dependencies.

### Asset Setup (The Most Important Step!)

The game will crash if it cannot find the required assets. You must create the following folder structure in the root of your project and place your files inside. **The filenames and paths must be exact.**

```
/YourProjectRoot/
    ├── src/
    │   └── main.cpp
    ├── CMakeLists.txt
    └── res/
        ├── arial.ttf           <-- A font file for the score
        ├── textures/
        │   ├── character.png   <-- Your player spritesheet
        │   ├── npc.png         <-- Your enemy spritesheet
        │   └── world.png       <-- Your tilesheet for the map
        └── sfx/
            └── music.ogg       <-- Your background music
```

### Build Instructions (Using Nix)

1.  **Clone the repository** (or ensure you are in the project's root directory).

2.  **Review the `shell.nix` file.** It provides the Nix environment with all the necessary libraries to compile and run the game. It should contain at least:
    ```nix
    # shell.nix
    { pkgs ? import <nixpkgs> {} }:

    pkgs.mkShell {
      buildInputs = [
        pkgs.cmake
        pkgs.sfml
        pkgs.openalSoft # Runtime dependency for SFML Audio
        # Any other X11 libs you might have needed
      ];
    }
    ```

3.  **Enter the Nix Shell.** From your project's root directory, run:
    ```bash
    nix-shell
    ```
    Your terminal prompt will change, indicating you are now in the specialized environment.

4.  **Create and navigate to the build directory.** This keeps our compiled files separate from our source code.
    ```bash
    mkdir build
    cd build
    ```

5.  **Run CMake.** This command reads your `CMakeLists.txt` file and generates the `Makefile` needed for compilation.
    ```bash
    cmake ..
    ```

6.  **Compile the code.** Now, simply run `make`. This will compile your `main.cpp` and link it against the SFML libraries. The custom command in our CMake file will also automatically copy your `res` folder into this `build` directory.
    ```bash
    make
    ```

7.  **Run the game!** The final executable is placed in the `bin` folder.
    ```bash
    ./bin/main
    ```

## How to Play

### Objective

You are dropped into a procedurally generated world filled with hostile clones of yourself. Survive for as long as you can by defeating enemies to increase your score. The game ends when your health runs out.

### Controls

| Key           | Action          |
| :------------ | :-------------- |
| **W, A, S, D** or **Arrow Keys** | Move Character  |
| **Spacebar**    | Shoot           |
| **Enter** (Hold)| Sprint / Run Faster |

## Code Structure Overview

For simplicity, the entire game logic resides in `main.cpp`. It is organized in the following way:

-   **Global Structs & Enums:** `GameState`, `TileType`, `Player`, `Enemy`, and `Bullet` are defined at the top to structure all our game data.
-   **Helper Function Prototypes:** Signatures for our world generation functions.
-   **`main()` function:**
    -   **Setup:** Initializes the window, loads all assets (textures, sounds, fonts), and sets up spritesheet coordinates.
    -   **Game State & UI:** Defines all the variables for the game state, UI text, and the `resetGame` lambda function.
    -   **Game Loop:** The main `while (window.isOpen())` loop runs the game.
        -   **Event Handling:** Checks for window closing and other events.
        -   **Game Logic (`if (gameState == GameState::Playing)`):** All gameplay—player input, enemy spawning, AI, updates, and collision—happens inside this block.
        -   **Drawing:** The last part of the loop, responsible for clearing the screen and drawing the world and UI.
-   **Helper Function Implementations:** The full C++ code for `findValidSpawn`, `generateWorld`, and `countTreeNeighbors` is at the very bottom of the file.
