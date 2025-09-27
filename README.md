# 2D Procedural Adventure

Welcome to Procedural Adventure! This is a simple yet powerful 2D top-down shooter built from the ground up in C++ and SFML. The game features an infinitely replayable world, dynamic enemies, a cinematic loading screen, and a complete game loop, all built on fundamental data structures and algorithms.

<img width="1200" height="676" alt="image" src="https://github.com/user-attachments/assets/5e2a82a2-f9a4-4f8b-b053-8c586ca75c84" />

This project was developed to serve as a practical demonstration of procedural generation, real-time entity management, and advanced graphics techniques within a game context.

<img width="1920" height="1080" alt="screenshot-1757774764" src="https://github.com/user-attachments/assets/66e6d72f-1829-4b7d-9a53-42dd617e522f" />


## Table of Contents


1.  [How it Works: A Technical Deep Dive](#how-it-works-a-technical-deep-dive)
    - [The Core: Data Structures & Algorithms](#the-core-data-structures--algorithms)
    - [World Generation: Cellular Automata](#world-generation-cellular-automata)
    - [Infinite World System](#infinite-world-system)
    - [Parallax Loading Screen](#parallax-loading-screen)
    - [Rendering Pipeline: The Art of Spritesheets](#rendering-pipeline-the-art-of-spritesheets)
2.   - [Prerequisites](#prerequisites)
    - [Asset Setup (The Most Important Step!)](#asset-setup-the-most-important-step)
    - [Build Instructions (Using Nix)](#build-instructions-using-nix)
3.  [How to Play](#how-to-play)
    - [Objective](#objective)
    - [Controls](#controls)
4.  [Code Structure Overview](#code-structure-overview)



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

### Infinite World System

The game features a truly infinite world that never shows the background color, creating a seamless exploration experience.

-   **Coordinate Wrapping:** Using modulo operations (`((x % WORLD_WIDTH) + WORLD_WIDTH) % WORLD_WIDTH`), the game wraps coordinates so that when you reach the edge of the 200x200 world, you seamlessly appear on the opposite side.
-   **Seamless Rendering:** The rendering system draws extra tiles around the visible area and wraps their coordinates, ensuring smooth transitions as you move across world boundaries.
-   **Collision System Integration:** All collision detection (bullets, enemies, player movement) works with wrapped coordinates, so gameplay remains consistent across world boundaries.
-   **Performance Optimization:** Only visible tiles are rendered, maintaining 60 FPS even with the larger world size.

### Parallax Loading Screen

The game features a cinematic loading screen that demonstrates advanced graphics techniques.

-   **Multi-Layer Parallax:** Five mountain layers scroll at different speeds (0.1x to 0.8x) creating a convincing sense of depth and movement.
-   **Seamless Scrolling:** Each layer uses multiple sprite copies to create infinite horizontal scrolling without visible seams.
-   **Smooth Transitions:** The loading screen includes fade-in effects for text elements and smooth progress indication.
-   **Asset Management:** All mountain textures are loaded and optimized with proper scaling and positioning.

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
        ├── arial.ttf           
        ├── textures/
        │   ├── character.png  
        │   ├── npc.png         
        │   ├── world.png       
        │   ├── parallax-mountain-bg.png                   
        │   ├── parallax-mountain-montain-far.png         
        │   ├── parallax-mountain-mountains.png            
        │   ├── parallax-mountain-trees.png               
        │   └── parallax-mountain-foreground-trees.png     
        └── sfx/
            └── music.ogg      
```

**Note:** The mountain textures are used for the parallax loading screen. You can find free parallax mountain assets online, or create your own layered mountain backgrounds. The game expects these specific filenames for the loading screen to work properly.

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

-   **Global Structs & Enums:** `GameState` (Loading, Playing, GameOver), `TileType` (Grass, Trees, Water), `Player`, `Enemy`, `Bullet`, and `ParallaxLayer` are defined at the top to structure all our game data.
-   **Helper Function Prototypes:** Signatures for our world generation and utility functions.
-   **`main()` function:**
    -   **Setup:** Initializes the window, loads all assets (textures, sounds, fonts), and sets up spritesheet coordinates.
    -   **Loading Screen Setup:** Creates parallax layers, loading screen UI elements, and initializes the loading state.
    -   **Game State & UI:** Defines all the variables for the game state, UI text, and the `resetGame` lambda function.
    -   **Game Loop:** The main `while (window.isOpen())` loop runs the game.
        -   **Event Handling:** Checks for window closing and other events.
        -   **Loading Screen Logic (`if (gameState == GameState::Loading)`):** Handles parallax scrolling, progress updates, and transition to gameplay.
        -   **Game Logic (`if (gameState == GameState::Playing)`):** All gameplay—player input, enemy spawning, AI, updates, and collision—happens inside this block.
        -   **Drawing:** The last part of the loop, responsible for clearing the screen and drawing either the loading screen or the game world and UI.
-   **Helper Function Implementations:** The full C++ code for `findValidSpawn`, `generateWorld`, and `countTreeNeighbors` is at the very bottom of the file.

### Key Technical Features

-   **Infinite World Rendering:** Uses modulo operations to wrap coordinates and create seamless world boundaries.
-   **Parallax Scrolling:** Implements multi-layer scrolling with different speeds for depth perception.
-   **State Management:** Clean separation between Loading, Playing, and Game Over states.
-   **Memory Management:** Efficient use of `std::vector` for dynamic entity management with the erase-remove idiom.
-   **Performance Optimization:** Only renders visible tiles and uses efficient collision detection algorithms.
