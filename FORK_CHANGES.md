# Fork changes (BerubePascal/xournalpp)

This fork adds personal modifications on top of the official
[Xournal++](https://github.com/xournalpp/xournalpp) (based on 1.3.8+dev, October 2026).
This file lists them, how to use them, and how to keep the fork up to date.

**File compatibility:** none of these changes alters the file format. Files saved with this
version open in official Xournal++; fork-only extras (e.g. the margin line) are simply not shown there.

---

## Modifications

Most recent first. Each line in the "Files" sections is relative to `src/core/` unless noted.

### Background drop-down applies at once
- In the toolbar's background drop-down (next to *Insert page*), choosing a **background, paper size
  or orientation applies it to the current page immediately** (and to new pages).
  Clicking an already selected background applies it again (e.g. on another page).
- *Apply to all pages* stays; the redundant *Apply to current page* button is removed.
- Files: `gui/menus/popoverMenus/PageTypeSelectionPopover.*`, `gui/menus/PageTypeSelectionMenuBase.*`

### Margin-line pages
- New backgrounds **Graph with vertical line** and **Plain with vertical line**, listed right after
  *Graph* and *Plain*.
- Same line as *Ruled with vertical line* (pink, 0.5 pt; dark blue on dark pages), about 25 mm from
  the left. On graph paper it sits on the **closest grid line** and follows the grid spacing.
- Stored as a background setting `ml` (distance in points), e.g. `config=ml=70.87`.
- Files: `view/background/MarginLine.*` (new), `view/background/GraphBackgroundView.*`,
  `view/background/BackgroundView.cpp`, `model/BackgroundConfig.h`,
  `resources-templates/pagetemplates.ini.in` (repo root)

### Background spacing
- **Journal → Background Spacing…**: sets the spacing (mm) of the lines/grid on **this page** or
  **all pages**, in one undo step. Applies to ruled, lined, graph, dotted and isometric pages.
- Files: `model/BackgroundSpacing.*` (new), `gui/dialog/BackgroundSpacingDialog.*` (new),
  `control/PageBackgroundChangeController.*`

### Line width slider
- Toolbar item **Line Width Slider** (add it via *Customize toolbar*), shown in mm, logarithmic.
- Its range follows the current tool (pen 0.15–5 mm, highlighter 0.35–10.6 mm, eraser 0.35–6.4 mm),
  with marks at the tool's standard sizes; it snaps to them when close.
- In-between widths are kept per tool and saved in the settings. Size buttons go back to the sizes.
- Not yet: changing the width of already selected strokes with the slider.
- Files: `gui/toolbarMenubar/ToolThicknessSlider.*` (new), `control/Tool.*`, `control/ToolHandler.*`,
  `control/Control.*`

### Shapes work from any tool
- Shape buttons/menus are never greyed out. Choosing a shape while a tool that cannot draw shapes
  is active (eraser, selection, text…) switches to the **pen**. The highlighter is kept.
- Files: `control/actions/ActionProperties.h`, `control/Control.cpp`

### Shape handles (edit drawn shapes)
- Select a single simple shape: round handles appear.
  - **Straight-segment shapes** (line, rectangle, coordinate system, straight-edged spline, arrows):
    drag any corner (grid snapping).
  - **Arcs**: drag the start or end along the circle (rotation snapping).
- Arrowheads are kept and re-aimed. **Alt** toggles snapping. Each drag can be undone.
- Handles show until the selection is moved, resized or rotated (reselect to get them back).
- Shapes are recognized from their points (corners, arcs, arrowhead pattern), no extra data is stored.
- Files: `control/tools/ShapePoints.*` (new), `control/tools/ShapePointsEditor.*` (new),
  `undo/ShapePointsUndoAction.*` (new), `control/tools/EditSelection.*`,
  `control/tools/EditSelectionContents.*`, `control/tools/CursorSelectionType.h`, `gui/XournalppCursor.cpp`

### Arc tool
- **Draw Arc** (Ctrl+0, in *Tools → Drawing Type*, the Draw drop-down, or as a toolbar button):
  click the **center**, click the **start** (sets the radius), then **press-drag-release** to place
  the end. The arc follows the pointer and goes the way it moved (can exceed 180°).
  Escape cancels. Grid/rotation snapping, fill and arrowheads supported.
- New icon `xopp-draw-arc.svg` for the four icon themes (`ui/icons*/hicolor/scalable/actions/`).
- Files: `control/tools/ArcHandler.*` (new), `control/ToolEnums.h`, `gui/PageView.cpp`,
  `gui/dialog/ButtonConfigGui.cpp`, `gui/toolbarMenubar/DrawingTypeComboToolButton.*`

### Arrowheads per shape
- Line, spline, coordinate system and arc can have arrowheads: **none / start / end / both**,
  remembered separately for each shape.
- Choose it from the **▸** on each shape row of the Draw drop-down, the **▾** of the separate shape
  buttons (kept beside the icon even in vertical toolbars), or *Tools → Drawing Type → Arrowheads*.
  **Ctrl+9** cycles the arrowheads of the current shape. The original Arrow tools are unchanged.
- Files: `control/tools/Arrowheads.*` (new), `gui/toolbarMenubar/ArrowheadPopoverFactory.*` (new),
  `control/tools/BaseShapeHandler.*`, `control/tools/SplineHandler.cpp`, `gui/toolbarMenubar/ToolButton.*`,
  `gui/toolbarMenubar/ToolMenuHandler.*`

### Default tool turns fill off
- Fix: selecting the *Default Tool* no longer keeps the pen's fill turned on.
- File: `control/settings/ButtonConfig.cpp`

### Windows test build on GitHub
- **Actions → Build Windows (for testing) → Run workflow** builds an installer and a portable zip of
  any branch (~30 min), downloadable from the run's *Artifacts*.
- File: `.github/workflows/build-windows-for-testing.yml`

### Shared changes
Several features add entries to these official files; they are the likeliest to need attention when
merging official updates:
- `enums/Action.enum.h` and `enums/generated/Action.NameMap.generated.h`: new actions are appended
  at the end. The generator script (PHP) was not used: **edit both files by hand, in the same order.**
- `ui/mainmenubar.xml` (repo root), `control/actions/ActionProperties.h`, `gui/toolbarMenubar/ToolMenuHandler.cpp`

---

## Updating from official Xournal++

The official repository is the `xournalpp` remote; this fork is `origin`.

1. `git fetch xournalpp`, then merge `xournalpp/master` into `master`.
2. Resolve conflicts if any (usually: keep both sides in the files listed under *Shared changes*).
3. Rebuild and test, then push to `origin`.

⚠️ On GitHub, **Sync fork → "Update branch"** is safe. **Never click "Discard commits"**: it erases
all the modifications above from the fork.

## Building on Windows

Requires MSYS2 (UCRT64) with the packages listed in `readme/WindowsBuild.md`.
From an MSYS2 UCRT64 shell (set `PROCESSOR_ARCHITECTURE=AMD64` first if CMake fails with an empty
`CMAKE_SYSTEM_PROCESSOR`):

```sh
mkdir build && cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX=install
cmake --build . && cmake --build . --target install
```

Run `build/install/bin/xournalpp.exe` with `C:\msys64\ucrt64\bin` on the `PATH`
(the local, git-ignored launcher `build/Run Xournal++ (local build).bat` does this).

User settings, toolbars and personal plugins are in `%LOCALAPPDATA%\xournalpp` and are not affected
by builds or updates.
