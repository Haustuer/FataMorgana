# TODO: Repository Restructuring for Publishing

**Date:** 2026-04-22
**Status:** Planning - Testing Required First
**Goal:** Prepare FataMorgana for public library publishing

---

## Current State

### What We Have Now
```
FataMorgana/                          # Current structure
├── lib/
│   ├── FataMorgana/                 # ✅ Library - ready to publish
│   └── FataMorgana-WebUI/           # ✅ Library - ready to publish
├── src/
│   └── main.cpp                     # ⚠️ Your private implementation
├── server/
│   ├── gradientFrameServer.js       # ✅ Reference server
│   └── package.json
├── platformio.ini                    # ⚠️ Your project config
├── *.md files (root)                 # ⚠️ Mix of docs
└── examples/ (currently empty)
```

**Status:**
- ✅ Libraries are ready (FataMorgana, FataMorgana-WebUI)
- ✅ Server is working
- ✅ Documentation exists
- ⚠️ Your implementation mixed with library
- ⚠️ Structure not ideal for publishing

---

## Target State

### After Restructuring

**Public Repo (FataMorgana):**
```
FataMorgana/                          # Public on GitHub
├── lib/
│   ├── FataMorgana/                 # Core library
│   │   ├── src/
│   │   ├── examples/               # Simple examples
│   │   ├── library.json
│   │   ├── library.properties
│   │   └── README.md
│   └── FataMorgana-WebUI/          # WebUI library
│       └── ...
├── server/                          # Reference server implementation
│   ├── README.md
│   ├── package.json
│   ├── gradientFrameServer.js
│   └── examples/
├── docs/                            # All documentation
│   ├── API.md
│   ├── GETTING_STARTED.md
│   ├── FATAMORGANA_PROTOCOL.md
│   └── ...
├── test/                            # Protocol tests
├── tools/                           # Utilities
├── README.md                        # Public readme
└── LICENSE
```

**Your Private Repo (FataMorgana-MyProject):**
```
FataMorgana-MyProject/               # Your private implementation
├── src/
│   └── main.cpp                    # Your application code
├── platformio.ini                   # Your hardware config
├── lib/                             # Can be empty (uses published libs)
└── README.md                        # Your project notes
```

**Benefits:**
- ✅ Public repo is clean library for community
- ✅ Your implementation stays private
- ✅ Easy to publish to PlatformIO/Arduino registries
- ✅ Clear separation between library and your use case

---

## Phase 1: TESTING (DO THIS FIRST!)

**⚠️ IMPORTANT: Test everything BEFORE restructuring!**

### Task 1.1: Verify Current Setup Works
- [ ] Compile current code successfully
  ```bash
  pio run
  ```
- [ ] Note firmware size (Flash/RAM usage)
- [ ] Upload to ESP hardware
- [ ] Test basic functionality:
  - [ ] WiFi connection works
  - [ ] FataMorgana client receives frames
  - [ ] LEDs display correctly
  - [ ] WebUI accessible
  - [ ] Configuration saves/loads
  - [ ] Statistics reporting works

### Task 1.2: Test Server
- [ ] Start Node.js server
  ```bash
  cd server
  npm install
  npm start
  ```
- [ ] Access web interface: `http://localhost:3001`
- [ ] Send test frames:
  - [ ] Gradient pattern
  - [ ] Solid colors
  - [ ] Custom pattern
- [ ] Verify frames reach ESP device
- [ ] Test discovery protocol
- [ ] Check device shows up in server UI

### Task 1.3: Test All Examples (in lib/)
- [ ] BasicClient compiles
- [ ] CustomMapping compiles
- [ ] RectangleWithTransforms compiles
- [ ] WithWebInterface compiles
- [ ] Upload one example to verify it works

### Task 1.4: Document Current Working State
- [ ] Create snapshot branch: `git checkout -b snapshot-before-restructure`
- [ ] Commit everything: `git add -A && git commit -m "Snapshot before restructuring"`
- [ ] Tag for safety: `git tag v1.0.0-pre-restructure`
- [ ] Note any issues or bugs found during testing
- [ ] Document your hardware setup (LED count, GPIO pins, etc.)

### Task 1.5: Backup Everything
- [ ] Full git backup: `git clone --mirror`
- [ ] Export platformio.ini settings
- [ ] Screenshot of working system
- [ ] Note any custom modifications

**⚠️ DO NOT PROCEED TO PHASE 2 UNTIL ALL TESTS PASS!**

---

## Phase 2: Preparation

### Task 2.1: Create Private Repo
- [ ] Create new private GitHub repo: `FataMorgana-MyProject`
- [ ] Initialize with README
- [ ] Clone to local machine
- [ ] Don't move anything yet - just prep the destination

### Task 2.2: Review What Moves Where

**Files to MOVE to Private Repo:**
```
src/main.cpp              → FataMorgana-MyProject/src/main.cpp
platformio.ini            → FataMorgana-MyProject/platformio.ini
.pio/                     → (don't copy - regenerate)
.vscode/                  → FataMorgana-MyProject/.vscode/
```

**Files to STAY in Public Repo:**
```
lib/FataMorgana/          → stays
lib/FataMorgana-WebUI/    → stays
server/                   → stays
*.md files                → move to docs/
examples/                 → stays (library examples)
```

**Files to REORGANIZE in Public Repo:**
```
Current location          → New location
─────────────────────────────────────────────────
API.md                    → docs/API.md
GETTING_STARTED.md        → docs/GETTING_STARTED.md
FATAMORGANA_PROTOCOL.md   → docs/FATAMORGANA_PROTOCOL.md
LIBRARY_ARCHITECTURE.md   → docs/LIBRARY_ARCHITECTURE.md
TRANSFORM_FEATURE.md      → docs/TRANSFORM_FEATURE.md
CHANGELOG.md              → CHANGELOG.md (stays at root)
README.md                 → README.md (rewrite for public)
```

### Task 2.3: Plan platformio.ini for Private Repo

Your private repo needs `platformio.ini` that uses PUBLISHED libraries:

```ini
[env:d1_mini_lite]
platform = espressif8266
board = d1_mini_lite
framework = arduino

lib_deps =
    # Use published libraries from registry
    FataMorgana           # Will pull from PlatformIO registry
    FataMorgana-WebUI     # Will pull from PlatformIO registry
    tzapu/WiFiManager@^2.0.17

# Your custom settings
upload_port = COM3       # Your port
monitor_speed = 115200
```

**Before publishing:** You'll use local libraries
**After publishing:** Switch to registry versions

---

## Phase 3: Restructure Public Repo

### Task 3.1: Create docs/ Directory
- [ ] Create `docs/` directory
- [ ] Move documentation files:
  ```bash
  git mv API.md docs/
  git mv GETTING_STARTED.md docs/
  git mv FATAMORGANA_PROTOCOL.md docs/
  git mv LIBRARY_ARCHITECTURE.md docs/
  git mv TRANSFORM_FEATURE.md docs/
  git mv NETWORK_CONFIG.md docs/
  git mv DISCOVERY_RESPONSE_FORMAT.md docs/
  git mv ESP_WEBSOCKET_UPDATE.md docs/
  git mv IMPLEMENTATION_SUMMARY.md docs/
  git mv PROJECT_IDEA.md docs/
  # Keep CHANGELOG.md at root
  # Keep README.md at root (will rewrite)
  ```
- [ ] Update cross-references in moved docs
- [ ] Commit: `git commit -m "docs: Organize documentation into docs/ directory"`

### Task 3.2: Remove Your Implementation
- [ ] **IMPORTANT:** Verify private repo is ready first!
- [ ] Remove from public repo:
  ```bash
  git rm -r src/
  git rm platformio.ini
  git rm -r .pio/
  git rm -r .vscode/  # If present
  ```
- [ ] Commit: `git commit -m "refactor: Remove private implementation (moved to separate repo)"`

### Task 3.3: Update Root README.md
- [ ] Rewrite README.md for public library
- [ ] Focus on library usage, not your project
- [ ] Include:
  - [ ] What is FataMorgana
  - [ ] Quick start (library installation)
  - [ ] Link to documentation
  - [ ] Link to server
  - [ ] Examples
  - [ ] Contributing guidelines
  - [ ] License
- [ ] Commit: `git commit -m "docs: Rewrite README for library users"`

### Task 3.4: Add .gitignore Updates
- [ ] Update `.gitignore`:
  ```gitignore
  # PlatformIO
  .pio/
  .vscode/

  # Project-specific (not library)
  src/
  platformio.ini

  # OS
  .DS_Store
  Thumbs.db

  # IDE
  *.swp
  *.swo
  *~

  # Logs
  *.log
  ```
- [ ] Commit: `git commit -m "chore: Update .gitignore for library repo"`

### Task 3.5: Clean Up Root Directory
- [ ] Remove temporary files:
  - [ ] `WIFIMANAGER_UPDATES.md` (implementation notes)
  - [ ] `REFACTORING_SUMMARY.md` (can move to docs/ or delete)
  - [ ] `TODO_*.md` files (move to docs/ or .github/)
  - [ ] Any other development notes
- [ ] Keep essential files:
  - [ ] `README.md`
  - [ ] `LICENSE`
  - [ ] `CHANGELOG.md`
  - [ ] `.gitignore`
- [ ] Commit: `git commit -m "chore: Clean up root directory"`

### Task 3.6: Verify Library Structure
- [ ] Check `lib/FataMorgana/` structure:
  ```
  lib/FataMorgana/
  ├── src/
  │   ├── FataMorgana.h
  │   ├── FataMorganaClient.h
  │   ├── FataMorganaClient.cpp
  │   ├── FataMorganaRenderer.h
  │   ├── FataMorganaRenderer.cpp
  │   ├── FataMorganaProtocol.h
  │   └── FataMorganaConfig.h
  ├── examples/
  │   ├── BasicClient/
  │   ├── CustomMapping/
  │   └── RectangleWithTransforms/
  ├── library.json
  ├── library.properties
  ├── keywords.txt
  └── README.md
  ```
- [ ] Same for `lib/FataMorgana-WebUI/`
- [ ] Commit if any fixes needed

### Task 3.7: Update Server README
- [ ] Create/update `server/README.md`
- [ ] Explain:
  - [ ] What the server does
  - [ ] How to install
  - [ ] How to run
  - [ ] API endpoints
  - [ ] Configuration options
  - [ ] "This is a reference implementation"
- [ ] Commit: `git commit -m "docs: Add server documentation"`

### Task 3.8: Create Public README Structure
Create comprehensive README with sections:

```markdown
# FataMorgana

[Badges: License, Version, Build Status]

## Overview
Brief description and key features

## Quick Start
### Installation
- PlatformIO: `pio lib install FataMorgana`
- Arduino IDE: Library Manager → "FataMorgana"

### Basic Example
[Minimal code example]

## Components
- **FataMorgana** - Core protocol library
- **FataMorgana-WebUI** - Optional web interface
- **Server** - Node.js reference server

## Documentation
- [Getting Started](docs/GETTING_STARTED.md)
- [API Reference](docs/API.md)
- [Protocol Specification](docs/FATAMORGANA_PROTOCOL.md)

## Examples
[List examples with brief description]

## Hardware
[Supported platforms, wiring diagram link]

## Server
[Brief description, link to server/README.md]

## Contributing
[Link to CONTRIBUTING.md]

## License
MIT - See LICENSE file
```

---

## Phase 4: Move to Private Repo

### Task 4.1: Copy Implementation to Private Repo
- [ ] In your private repo directory:
  ```bash
  cd FataMorgana-MyProject

  # Copy from public repo (before you deleted them!)
  cp ../FataMorgana/src/main.cpp src/
  cp ../FataMorgana/platformio.ini .
  cp ../FataMorgana/.vscode/settings.json .vscode/  # if exists
  ```

### Task 4.2: Update platformio.ini in Private Repo
- [ ] Initially use local libraries:
  ```ini
  [env:d1_mini_lite]
  platform = espressif8266
  board = d1_mini_lite
  framework = arduino

  lib_deps =
      # Temporary: Use local libraries during development
      symlink://../FataMorgana/lib/FataMorgana
      symlink://../FataMorgana/lib/FataMorgana-WebUI
      tzapu/WiFiManager@^2.0.17

  # TODO: After publishing, switch to:
  # lib_deps =
  #     FataMorgana
  #     FataMorgana-WebUI
  #     tzapu/WiFiManager@^2.0.17
  ```

### Task 4.3: Test Private Repo
- [ ] Compile in private repo
- [ ] Upload to hardware
- [ ] Verify everything still works
- [ ] Commit to private repo

### Task 4.4: Create Private README
- [ ] Write README for your project
- [ ] Include:
  - [ ] What your project does
  - [ ] Hardware setup
  - [ ] Configuration notes
  - [ ] Personal notes/reminders
  - [ ] Link to public FataMorgana repo
- [ ] Commit

---

## Phase 5: Publish Preparation

### Task 5.1: Update Library Metadata
- [ ] Review `lib/FataMorgana/library.json`:
  - [ ] Version number correct
  - [ ] Repository URL correct
  - [ ] Author info (replace placeholder)
  - [ ] Keywords appropriate
  - [ ] Dependencies correct
- [ ] Same for `lib/FataMorgana-WebUI/library.json`
- [ ] Update `library.properties` files similarly
- [ ] Commit: `git commit -m "chore: Update library metadata for publishing"`

### Task 5.2: Create CONTRIBUTING.md
- [ ] Create `CONTRIBUTING.md`:
  - [ ] How to report bugs
  - [ ] How to submit PRs
  - [ ] Code style guidelines
  - [ ] Testing requirements
  - [ ] Community guidelines
- [ ] Commit: `git commit -m "docs: Add contributing guidelines"`

### Task 5.3: Add GitHub Templates
- [ ] Create `.github/ISSUE_TEMPLATE.md`
- [ ] Create `.github/PULL_REQUEST_TEMPLATE.md`
- [ ] Create `.github/workflows/ci.yml` (optional CI)
- [ ] Commit: `git commit -m "chore: Add GitHub templates"`

### Task 5.4: Final Review
- [ ] Test all examples compile
- [ ] Check all documentation links work
- [ ] Verify README is clear
- [ ] Check LICENSE file
- [ ] Spell check documentation
- [ ] Make sure no private info leaked

### Task 5.5: Tag Release
- [ ] Create tag: `git tag v1.0.0`
- [ ] Push: `git push origin main --tags`

---

## Phase 6: Publishing

### Task 6.1: PlatformIO Library Registry
- [ ] Visit: https://platformio.org/lib/show/
- [ ] Register library (or update if exists)
- [ ] Provide GitHub URL
- [ ] Verify library appears in registry
- [ ] Test installation: `pio lib install FataMorgana`

### Task 6.2: Arduino Library Manager
- [ ] Submit to: https://github.com/arduino/library-registry
- [ ] Follow submission guidelines
- [ ] Wait for approval
- [ ] Test in Arduino IDE once approved

### Task 6.3: Update Private Repo
- [ ] Once libraries published, update `platformio.ini`:
  ```ini
  lib_deps =
      FataMorgana           # From registry now!
      FataMorgana-WebUI
      tzapu/WiFiManager@^2.0.17
  ```
- [ ] Remove symlinks
- [ ] Test compilation still works
- [ ] Commit

---

## Phase 7: Documentation

### Task 7.1: Create Release Announcement
- [ ] Write announcement:
  - [ ] What is FataMorgana
  - [ ] Key features
  - [ ] Getting started
  - [ ] Where to get help
  - [ ] Link to repo

### Task 7.2: Update GitHub Repo Settings
- [ ] Set description
- [ ] Add topics/tags: led, esp8266, esp32, udp, multicast, arduino
- [ ] Enable discussions (for community questions)
- [ ] Enable issues
- [ ] Add website link (if you have one)

### Task 7.3: Create Documentation Site (Optional)
- [ ] Consider GitHub Pages
- [ ] Or ReadTheDocs
- [ ] Or just link to docs/ folder

---

## Rollback Plan (If Something Goes Wrong)

### If Restructure Fails:
```bash
# Return to snapshot
git checkout snapshot-before-restructure

# Or restore from tag
git checkout v1.0.0-pre-restructure
```

### If Private Repo Issues:
- Keep old implementation in snapshot branch
- Can always recover from git history

---

## Checklist Summary

### Before Starting:
- [ ] **Phase 1 complete:** All tests pass ✅
- [ ] Snapshot/backup created ✅
- [ ] Private repo created ✅

### Restructuring:
- [ ] **Phase 3 complete:** Public repo cleaned ✅
- [ ] **Phase 4 complete:** Private repo working ✅
- [ ] Both repos independently functional ✅

### Publishing:
- [ ] **Phase 5 complete:** Metadata ready ✅
- [ ] **Phase 6 complete:** Published to registries ✅
- [ ] **Phase 7 complete:** Documentation live ✅

---

## Notes

### Questions to Answer Before Restructuring:

1. **Author Information:**
   - [ ] What name/email for library.json?
   - [ ] Same for both libraries?

2. **Repository URL:**
   - [ ] GitHub username?
   - [ ] Repo name: `FataMorgana` or something else?

3. **License:**
   - [ ] MIT license OK?
   - [ ] Update author in LICENSE file?

4. **Version Numbering:**
   - [ ] Start at v1.0.0?
   - [ ] Or v0.9.0 (beta)?

5. **Private Repo:**
   - [ ] Name: `FataMorgana-MyProject`?
   - [ ] Or something else?

### Timeline Estimate:

- **Phase 1 (Testing):** 2-4 hours
- **Phase 2-3 (Restructure):** 2-3 hours
- **Phase 4 (Private repo):** 1 hour
- **Phase 5 (Prep):** 2-3 hours
- **Phase 6 (Publishing):** 1-2 days (waiting for approval)
- **Total:** ~1-2 days of work, spread over a week

### Success Criteria:

✅ Public repo ready for community use
✅ Libraries published and installable
✅ Your implementation private and working
✅ All tests passing
✅ Documentation complete
✅ No private information leaked

---

**REMEMBER: Test thoroughly (Phase 1) before restructuring!**
