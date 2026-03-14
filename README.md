# Computer-Graphics

# Cyberpunk Market Simulation (OpenGL / GLSL, C++)

> A small cyberpunk-style street corner rendered in OpenGL, featuring a free-fly/FPS camera, Phong lighting, emissive neon signs with time-based animation, and a distance-responsive floating car.

<!-- Badges (optional) -->
<!-- ![C++](https://img.shields.io/badge/C%2B%2B-17-blue) -->
<!-- ![OpenGL](https://img.shields.io/badge/OpenGL-3.3%2B-green) -->
<!-- ![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20macOS%20%7C%20Linux-lightgrey) -->

---

## Gallery / Screenshots



<p align="center">
  <img src="whole_scene.PNG" width="85%" alt="Whole Scene Overview"/>
</p>

<p align="center">
  <img src="successful_texture.PNG" width="42%" alt="Successful Texture Close-up"/>
  <img src="lightVM.PNG" width="42%" alt="Lighting / Neon Effect"/>
</p>

<!-- More slots -->
<!--
<p align="center">
  <img src="docs/images/character_alpha.png" width="42%" alt="Character Alpha"/>
  <img src="docs/images/alley_walkthrough.png" width="42%" alt="Alley Walkthrough"/>
</p>
-->

---

## Demo Video (optional)

- YouTube link: https://youtu.be/zzhgN1XvLjY
- Local file in repo: `Project1.zip` (**TODO**)

---

## Features

### Camera & Controls
- Free-fly / FPS-style camera (Euler angles, view & projection matrices)
- Frame-rate independent movement using `deltaTime`
- **Alt speed modifier**: hold `Alt` + movement keys to move faster

### Lighting & Rendering
- Phong lighting (ambient + diffuse + specular), with **per-object parameter tuning**
- Depth testing for correct occlusion
- MSAA (`GL_MULTISAMPLE`) to reduce jagged edges
- Alpha blending (`GL_SRC_ALPHA`, `GL_ONE_MINUS_SRC_ALPHA`) for transparent assets
- Alpha-to-coverage (`GL_SAMPLE_ALPHA_TO_COVERAGE`) for smoother alpha textures (e.g., hair)

### Neon Shader (Emissive + Animation)
- Dedicated `neonShader` for neon signs / billboards
- Emissive tint/strength controls
- Time-based “breathing / flicker” using sine-driven parameters
- **Proximity-based stability**: neon becomes more stable when the camera is within a radius (`uStableNeon`)

### Animation Set Pieces
- **Floating retro car**: bobbing + rolling driven by sin waves
- Motion becomes **distance-responsive** (speed/amplitude adjusted based on camera distance)
- Characters tuned with slightly higher ambient/specular for readability in darker areas

### Ground & Effects
- Large manually-built textured ground quad (e.g., 400 × 400)
- Attempted steam effect via alpha blending (left in code for future improvement)

---

## Controls (Example)

> Adjust these keys to match your actual implementation.

- `W/A/S/D` — Move
- Mouse — Look around
- Scroll — FOV / Zoom
- `Alt` + `W/A/S/D` — Speed boost
- `ESC` — Exit

<!-- Add more if you have them -->
<!-- - `F` — Toggle flashlight (TODO) -->
<!-- - `1/2` — Switch shaders (TODO) -->

---

## Tech Stack

- **Language:** C++
- **Graphics:** OpenGL (3.3+ recommended), GLSL
- **Window/Input:** GLFW
- **OpenGL Loader:** GLAD
- **Math:** GLM
- **Model Loading:** Assimp
- **Textures:** stb_image
- **UI (if used):** Dear ImGui

> Many implementation patterns follow LearnOpenGL; scene layout, animation logic, and shaders are customized for this project.

---

## Build & Run

### Prerequisites
- A C++17-capable compiler (MSVC / clang / gcc)
- GPU driver supporting OpenGL 3.3+


