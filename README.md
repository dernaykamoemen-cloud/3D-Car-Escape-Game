# 🚗 Road Escape — 3D Car Runner Game

A 3D endless-runner game built from scratch in **C++** using **OpenGL (GLUT)**. The player drives down a 3-lane road, dodging trains, collecting coins, and trying to survive while being chased by a police car.

![Language](https://img.shields.io/badge/language-C%2B%2B-blue)
![Graphics](https://img.shields.io/badge/graphics-OpenGL%20%2F%20GLUT-orange)

## 🎮 Demo

A gameplay video is included in this repository (`gameplay_demo.mp4`).

## 📖 About the Project

This project simulates a 3-lane road runner game with a fully custom 3D engine built without any external game framework — just raw OpenGL calls, real-time math, and game logic written from the ground up.

The player controls a car that automatically drives forward, switching between 3 lanes to:
- **Avoid trains** blocking one or two lanes
- **Collect coins** for points
- **Survive** as long as possible while a police car chases close behind

The game speeds up over time, increasing the difficulty as the player's score grows.

## ✨ Features

- **Real-time 3D rendering** with custom-built scenery: roads, buildings, trees, guard rails, and a procedurally generated environment that scrolls infinitely
- **Dynamic lighting and fog** for atmospheric depth
- **Custom sky rendering** with a gradient horizon, sun, glow halo, and clouds, drawn in 2D as a backdrop before the 3D scene
- **Lane-switching movement** with smooth interpolated transitions
- **Collision detection** between the player car and trains/coins
- **Progressive difficulty** — game speed increases the longer you survive
- **Procedural world generation** — buildings, trees, and obstacles are generated and cleaned up dynamically as the player moves forward
- **Multiple camera views**, including a first-person mode and side views
- **HUD** displaying score, coins collected, and game state (start / playing / game over)

## 🕹️ Controls

| Key | Action |
|---|---|
| `←` / `→` (Arrow keys) | Switch lanes |
| `D` | Toggle first-person view |
| `E` | Hold to look left |
| `R` | Hold to look right |
| `Space` | Start game / Restart after game over |
| `Esc` | Quit |

## 🛠️ Built With

- **C++**
- **OpenGL** (fixed-function pipeline)
- **GLUT** (OpenGL Utility Toolkit) — windowing, input handling, and the game loop

## ⚙️ How to Run

This project requires a C++ compiler and the **freeglut**/GLUT development library.

### Windows (MinGW example)
```bash
g++ main.cpp -o RoadEscape -lfreeglut -lopengl32 -lglu32
RoadEscape.exe
```

### Linux
```bash
sudo apt-get install freeglut3-dev
g++ main.cpp -o RoadEscape -lglut -lGL -lGLU
./RoadEscape
```

## 📚 What I Learned

Building this project helped me strengthen my skills in:
- 3D graphics programming with OpenGL (transformations, lighting, fog, blending)
- Real-time game loop design and frame-independent movement (delta time)
- Procedural content generation and cleanup for an "infinite" world
- Collision detection systems
- Managing game state (start, playing, game over) and a basic HUD

## 👤 Author

**Moemen Dernayka**
Computer Science Student, Lebanese University
[LinkedIn](https://www.linkedin.com/in/moemen-dernayka-3a04893a6)
