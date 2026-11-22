param([string]$Workspace = "")
$ErrorActionPreference = "Stop"
$root = $PSScriptRoot
if (!$Workspace) { $Workspace = Join-Path $root "_build" }
$template = Join-Path $Workspace "union-plugin-template"
$templateCommit = "42a8a0cb61a5390696cdbcfe1a918c9e1b25b6f8"
$build = Join-Path $Workspace "compiled"
$release = Join-Path $root "release"
New-Item -ItemType Directory -Force -Path $Workspace, $release | Out-Null
function Check($msg) { if ($LASTEXITCODE -ne 0) { throw "$msg (code $LASTEXITCODE)" } }
if (!(Test-Path (Join-Path $template ".git"))) {
  git clone --no-checkout --depth 1 https://github.com/Patrix9999/union-plugin-template.git $template
  Check "Failed to fetch Union template"
}
git -C $template cat-file -e "$templateCommit^{commit}" 2>$null
if ($LASTEXITCODE -ne 0) {
  git -C $template fetch --depth 1 origin $templateCommit
  Check "Failed to fetch pinned Union template"
}
git -C $template checkout --detach $templateCommit
Check "Failed to select pinned Union template"
git -C $template submodule update --init --recursive
Check "Failed to fetch Union dependencies"
Copy-Item (Join-Path $root "src/Plugin.ipp") (Join-Path $template "src/Plugin.ipp") -Force
Copy-Item (Join-Path $root "src/kondehra.hpp") (Join-Path $template "src/kondehra.hpp") -Force
Copy-Item (Join-Path $root "src/quest_manifest.hpp") (Join-Path $template "src/quest_manifest.hpp") -Force
$cmake = Join-Path $template "CMakeLists.txt"
$body = [IO.File]::ReadAllText($cmake)
$body = $body.Replace("project(UnionPlugin", "project(OdysseeQuestInspector")
if ($body -notmatch "project\(OdysseeQuestInspector") { throw "Upstream CMake file changed" }
[IO.File]::WriteAllText($cmake, $body)
cmake -S $template -B $build -G "Visual Studio 17 2022" -A Win32 -DGOTHIC_API_G2A=ON
Check "CMake configure failed"
cmake --build $build --config Release --parallel 2
Check "C++ or VDF build failed"
$vdf = Join-Path $build "vdf/OdysseeQuestInspector.vdf"
if (!(Test-Path $vdf)) { throw "VDF not found at $vdf" }
Copy-Item $vdf (Join-Path $release "OdysseeQuestInspector.vdf") -Force
Write-Host "Build succeeded, VDF in release/"
