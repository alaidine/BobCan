# BobCan

<img src="Resources/LogoLarge.png" alt="logo" width="256" />

Game made for the the SkillTree GameJam. <a href="https://aalaaiddine.itch.io/bobcan">Here is the itch.io page.</a>

## How to play the game

First compile or get it from the <a href="https://aalaaiddine.itch.io/bobcan">itch.io page.</a>

### On Windows (Visual Studio)

Open BobCan.sln and build (x64), vcpkg.exe should be available in path.

### With CMake (Linux / cross-platform)

SFML and spdlog are fetched and built automatically via `FetchContent`, so they
don't need to be pre-installed. SFML still needs a few **system** development
packages on Linux (X11, OpenGL, etc.) that cannot be fetched:

```bash
# Debian / Ubuntu
sudo apt install build-essential cmake git \
    libx11-dev libxrandr-dev libxcursor-dev libxi-dev \
    libudev-dev libgl1-mesa-dev libfreetype-dev
```

Then build and run:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
cd build && ./BobCan        # run from where the Resources folder was copied
```

The build copies `Resources/` next to the binary, so run the game from that
directory (assets are loaded with relative paths).

### The rules and lore

You are Bob and after you get out of work there is an alien invasion. The alien
starts attacking you but you can only walk around at first. Every time you die you
resurrect and earn a **skill point** to spend in a branching **skill tree**. The
goal of the game is to grow enough skills to beat the alien.

The alien shoots projectiles at you. When your HP reaches 0 you fall, gain a skill
point, and the skill tree opens so you can unlock a new ability before trying again
(the alien's HP resets each attempt).

### The skill tree

```
            [WALK]
            /    \
        [JUMP]   [PUNCH]
          |         |
      [SHIELD]   [SHOOT]
```

- **WALK** — known from the start.
- **JUMP** — jump over projectiles.
- **SHIELD** — hold to block incoming projectiles.
- **PUNCH** — close-range melee on the alien.
- **SHOOT** — fire projectiles at the alien from a distance.

### Controls

| Action | Key |
| --- | --- |
| Move | `A` / `D` |
| Jump (needs JUMP) | `Space` |
| Punch (needs PUNCH) | `F` |
| Shield (needs SHIELD) | hold `Left Shift` |
| Shoot (needs SHOOT) | `Left Click` toward target |
| Start / confirm | `Enter` |
| Skill tree: select | `Left` / `Right` |
| Skill tree: unlock / resume | `Enter` / `Space` |
| Quit | `Escape` |
