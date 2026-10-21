# spritex 👾

> **A high-performance generative AI in C++ for synthesizing pixel art sprites via matrix generation.**

`spritex` is a fast, native C++ engine and generative model designed to synthesize pixel art sprites. By treating sprites as discrete 2D token matrices mapped to customizable color palettes, `spritex` generates crisp, retro-style pixel art exported directly to PNG.

---

## 🎯 Core Concepts

- **Discrete Matrix Representation:** Pixel art is represented as numerical grids (e.g., indexed palette tokens: `0` for transparent, `1` for outline, `2` for base color, etc.).
- **High-Performance C++ Pipeline:** Fast inference, tensor operations, and direct PNG rasterization without overhead.
- **Generative AI Core:** Synthesizes novel sprites based on learned structures, seeds, or conditioning prompts.
- **Custom Palettes:** Decoupled geometry and coloring—generate the sprite shape once, apply multiple retro palettes on the fly.

---

## 📁 Project Structure

```text
spritex/
├── CMakeLists.txt      # Build configuration
├── include/spritex/    # Public C++ headers
├── src/                # Implementation files
├── third_party/        # Lightweight vendor dependencies (e.g. stb_image_write)
└── examples/           # Sample matrices and generation scripts
```

---

## 🚀 Building & Running

*(Instructions will be updated as build pipeline is established)*

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

---

## 📄 License
MIT
