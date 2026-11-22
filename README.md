# Odyssee Quest Inspector

Prototype **Union C++ plugin** for Gothic II: Odyssee (G2A, Windows x86), **no Ninja**.

**Controls:** apostrophe `'` toggles overlay; F6 cycles flag filters; left/right turns pages; R reloads parser values.

Shows **734 known MIS_ variable names** collected from a 2024 Odyssee save. Raw values do not equal canonical quest counts and do not prove anything was missed. Read-only, no quest edits.

## Windows build

Use Visual Studio 2022 + C++ workload, CMake and Git; execute `Build-Windows.ps1`, which retrieves the [public Union plugin template](https://github.com/Patrix9999/union-plugin-template), compiles x86 Gothic II Addon and packages `release/OdysseeQuestInspector.vdf`.

Or download a successful GitHub Actions build artifact from **Actions → Build Union G2A VDF**.

**Status:** prototype pending Windows CI and in-game validation. A build success alone does not guarantee game compatibility. Back up saves and test with a disposable install first. Install resulting VDF into `Gothic II/Data/`.
