# 🎮 Tetris (OpenGL / freeglut)

A classic Tetris game written in C using OpenGL and freeglut for rendering, built with Visual Studio on Windows.

## ✨ Features

- All 7 classic tetrominoes (I, J, L, O, S, T, Z) with full rotation
- **Ghost piece** — translucent shadow shows exactly where the piece will land
- **Hard drop** (Space) — instantly slams the piece down
- Progressive difficulty — speed increases every 30 seconds
- Leveling system — every 10 lines clears advances the level
- Classic scoring: 1 line = 100 pts, 2 = 300, 3 = 500, 4 = 800 (multiplied by level)
- Persistent high-score saved to `Record.txt`
- Main menu, game over screen with **NEW RECORD!** banner

## 🕹 Controls

| Key | Action |
|-----|--------|
| ← → | Move piece left / right |
| ↑ | Rotate piece |
| ↓ | Soft drop (move down faster) |
| Space | Hard drop (instant) |
| Home | Slam to bottom |

## 🛠 Build (Visual Studio)

1. Open `Tetris.sln` in Visual Studio 2022 (or later).
2. Select **Release | x64** (or x86).
3. Press **Ctrl+Shift+B** to build.
4. Run from `Release/Tetris.exe` — make sure `freeglut.dll` is in the same folder.

### Dependencies

- [freeglut 3.0.0](https://www.transmissionzero.co.uk/software/freeglut-devel/) — included via NuGet package `freeglut.3.0.0.v140`
- OpenGL (ships with Windows drivers)
- Visual C++ runtime (v140+)

The NuGet package is restored automatically when you open the solution.

## 📁 Project Structure

```
stu-Tetris/
├── Tetris/
│   ├── Tetris.cpp          # All game logic and rendering
│   ├── packages.config     # NuGet dependency (freeglut)
│   └── Record.txt          # High-score file (auto-created)
├── Release/
│   ├── Tetris.exe          # Pre-built binary
│   └── freeglut.dll        # Required runtime DLL
└── Tetris.sln              # Visual Studio solution
```

## 🧩 Technical Notes

- Rendering is done with immediate-mode OpenGL 1.x via freeglut — straightforward and dependency-light.
- Game state machine: `MENU → PLAYING → GAME_OVER → MENU`.
- Piece data is stored as a `[7][4][4][4]` integer array (piece × rotation × row × col).
- Collision detection checks both wall/floor bounds and occupied grid cells.
- Speed starts at 1000 ms per tick and decreases by 100 ms every 30 s, flooring at 100 ms.

## 📜 License

This project was written as a student exercise. Feel free to use and modify it freely.
