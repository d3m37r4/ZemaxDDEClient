# ImGuiNotify (vendored)

- **Source:** https://github.com/TyomaVader/ImGuiNotify
- **Commit:** `d00e45f8d6b1e094bc9288d20eb3d2840f6a7d73` (branch `Dev`, 2026-10)
- **File:** `ImGuiNotify.hpp` from `win32Example/backends/` (header-only, v0.0.3 fork line)
- **License:** MIT (see `LICENSE`)
- Upstream is a maintained fork of the archived `patrickcjk/imgui-notify`.

## Local patches (marked `// patched:` in the header)

1. `IconsFontAwesome6.h` include path points to `assets/icons/fa/` (the app's copy).
2. Toast window name uses portable `std::snprintf` — MinGW has no `sprintf_s`.
3. Notification paddings are multiplied by the main viewport `DpiScale`, so the
   stack keeps its visual inset at any DPI (the library works in raw pixels).

Do not update the header blindly: re-apply the three patches and re-check
compatibility with the vendored Dear ImGui (docking + multi-viewport).
