# Treasure Runner

A multi-language dungeon exploration game built in C and Python as part of my CIS2750 course at the University of Guelph. The game engine is written in C and exposed to Python via `ctypes`. The user interface is built with the `curses` library.

---

## Project Structure

```
.
├── assets/                  # INI config files and player profile JSON files
├── c/
│   ├── include/             # C header files
│   └── src/                 # C source files
├── data_gen/                # Datagen library (provided, not modified)
├── dist/                    # Compiled shared libraries (libbackend.so, libpuzzlegen.so)
└── python/
    ├── run_game.py          # Game entry point
    └── treasure_runner/
        ├── bindings/
        │   └── bindings.py  # Low-level ctypes bindings to C library
        ├── models/
        │   ├── exceptions.py
        │   ├── game_engine.py
        │   └── player.py
        └── ui/
            └── game_ui.py   # Curses-based user interface
```

---

## How to Run

From the **repo root**:

```bash
python3 python/run_game.py --config assets/new.ini --profile assets/nadia.json
```

Or from inside the **python folder**:

```bash
cd python/
python3 run_game.py --config ../assets/new.ini --profile ../assets/nadia.json
```

If the profile file does not exist it will be created. You will be prompted to enter a player name on first launch.

---

## Building the C Library

```bash
cd c/
make dist
```

This compiles `libbackend.so` into `dist/`. The Python layer loads this at runtime via `ctypes`.

---

## Controls

| Key | Action |
|-----|--------|
| `W` / `↑` | Move north |
| `S` / `↓` | Move south |
| `A` / `←` | Move west |
| `D` / `→` | Move east |
| `>` | Enter portal (must be standing on it) |
| `r` | Reset game to initial state |
| `q` | Quit |

---

## Game Elements

| Symbol | Meaning |
|--------|---------|
| `@` | Player |
| `#` | Wall |
| `$` | Treasure (gold) |
| `X` (green) | Portal — open, press `>` to enter |
| `X` (red) | Portal — locked, push block onto switch first |
| `O` | Pushable block |
| `=` | Pressure switch (unpressed) |
| `+` | Pressure switch (pressed — block is on it) |

---

## Features

### Required
- **MVC Architecture** — Clean separation of model (Player), controller (GameEngine), and view (GameUI). No curses code outside `game_ui.py`, no C data replicated on the Python side.
- **Player Profile** — JSON persistence tracking games played, max treasures collected, most rooms visited, and last played timestamp. Profile is displayed on the splash screen and quit screen.
- **Curses UI** — Full terminal UI with message bar, room render, legend, minimap, status bar, splash screen, and quit screen. Adapts to terminal size and raises an error if the terminal is too small.
- **Game Runner** — `run_game.py` with `--config` and `--profile` flags.

### Extended
- **Collect All the Treasure** — The game ends in victory when every treasure in the world is collected. Progress is shown live in the message bar (e.g. `12/20`). A victory screen displays elapsed time and final stats.
- **Locked Doors (Switches)** — Gated portals cannot be traversed until their linked pressure switch is activated by pushing a block onto it. Locked portals are shown as a red `X`. The switch shows `=` when unpressed and `+` when pressed (block consumed). Unlocking is reflected in real time.

---


## Architecture

```
GameUI (curses, view)
    │
    ▼
GameEngine (ctypes controller)
    │
    ▼
libbackend.so (C game engine)
    │
    ├── Graph (room connectivity)
    ├── Room (layout, entities)
    ├── Player (position, collected treasures)
    └── WorldLoader (datagen → Room structs)
```

The C side owns all game state. Python only owns the player profile dict, the visited-rooms set, the victory flag, and the current message string.

---

## Testing

C tests use the [Check](https://libcheck.github.io/check/) framework:

```bash
cd c/
make test
```

Python tests use `unittest`:

```bash
cd python/
python3 -m pytest
```

---

## Attribution

Curses UI structure informed by examples provided in CIS*2750 course lectures.

---

## Author

Created by Caiden Kowalchuk

