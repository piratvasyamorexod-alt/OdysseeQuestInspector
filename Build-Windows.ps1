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
$sources = @("Plugin.ipp", "journal_mouse_win32.hpp", "journal_cursor.rc", "journal_cursor.cur", "quest_manifest.hpp", "kondehra.hpp", "tamora.hpp", "tamora_rules.hpp", "albatros.hpp", "albatros_rules.hpp", "adarmos.hpp", "adarmos_rules.hpp", "departure.hpp", "departure_rules.hpp")
foreach ($source in $sources) {
  Copy-Item (Join-Path $root "src/$source") (Join-Path $template "src/$source") -Force
}
$textureDir = Join-Path $template "vdf/_Work/Data/Textures/_Compiled"
New-Item -ItemType Directory -Force -Path $textureDir | Out-Null
foreach ($texture in @("OQI_JOURNAL-C.TEX", "OQI_SCROLL-C.TEX", "OQI_CURSOR-C.TEX")) {
  Copy-Item (Join-Path $root "textures/$texture") $textureDir -Force
}
$cmake = Join-Path $template "CMakeLists.txt"
$body = [IO.File]::ReadAllText($cmake)
$body = $body.Replace("project(UnionPlugin", "project(OdysseeQuestInspector")
$body = $body.Replace('VERSION ${PROJECT_VERSION}', 'VERSION 1.2.1.0')
if ($body -notmatch "project\(OdysseeQuestInspector") { throw "Upstream CMake file changed" }
$body += "`r`ntarget_sources(OdysseeQuestInspector PRIVATE src/journal_cursor.rc)`r`n"
[IO.File]::WriteAllText($cmake, $body)
cmake -S $template -B $build -G "Visual Studio 17 2022" -A Win32 -DGOTHIC_API_G2A=ON
Check "CMake configure failed"
cmake --build $build --config Release --parallel 2
Check "C++ or VDF build failed"
$vdf = Join-Path $build "vdf/OdysseeQuestInspector.vdf"
if (!(Test-Path $vdf)) { throw "VDF not found at $vdf" }
Copy-Item $vdf (Join-Path $release "OdysseeQuestInspector.vdf") -Force
Write-Host "Build succeeded, VDF in release/"
