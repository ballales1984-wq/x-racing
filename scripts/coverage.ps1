param(
  [string]$Config = "Release",
  [string]$BuildDir = "$PSScriptRoot/../build",
  [string]$OccExe = $env:OCC_EXE
)

# Project 0 — code coverage harness (Windows / MSVC).
#
# Runs the unit test executable under OpenCppCoverage and produces an HTML
# report plus a Cobertura XML summary. OpenCppCoverage must be installed and
# reachable: pass its path via -OccExe, set $env:OCC_EXE, or place it in one
# of the well-known locations below.
#
# Usage:
#   pwsh scripts/coverage.ps1                # Release build, auto-detect OCC
#   pwsh scripts/coverage.ps1 -Config Debug -OccExe C:\tools\OpenCppCoverage\OpenCppCoverage.exe

$ErrorActionPreference = "Stop"

$root = Resolve-Path "$PSScriptRoot/.."

if (-not $OccExe) {
  $candidates = @(
    "C:\Program Files\OpenCppCoverage\OpenCppCoverage.exe",
    "C:\ProgramData\chocolatey\bin\OpenCppCoverage.exe",
    "C:\tools\OpenCppCoverage\OpenCppCoverage.exe",
    "$env:LOCALAPPDATA\Microsoft\WinGet\Packages\OpenCppCoverage*\OpenCppCoverage.exe"
  )
  foreach ($c in $candidates) {
    $found = Resolve-Path $c -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($found) { $OccExe = $found.Path; break }
  }
}

if (-not $OccExe -or -not (Test-Path $OccExe)) {
  Write-Error "OpenCppCoverage.exe not found. Install it from https://github.com/OpenCppCoverage/OpenCppCoverage or pass the OccExe parameter."
}

$testsExe = Get-ChildItem -Path $BuildDir -Recurse -Filter "project0_tests.exe" |
  Where-Object { $_.DirectoryName -like "*$Config*" } |
  Select-Object -First 1 -ExpandProperty FullName

if (-not $testsExe) {
  Write-Error "project0_tests.exe ($Config) not found under $BuildDir. Build the tests first."
}

$binDir = Join-Path $BuildDir "bin/$Config"
if (Test-Path $binDir) {
  $env:PATH = "$binDir;$env:PATH"
}
$engineBinDir = Join-Path $BuildDir "engine/$Config"
if (Test-Path $engineBinDir) {
  $env:PATH = "$engineBinDir;$env:PATH"
}
$assimpBinDir = Join-Path $BuildDir "_deps/assimp-build/bin/$Config"
if (Test-Path $assimpBinDir) {
  $env:PATH = "$assimpBinDir;$env:PATH"
}

$outDir = Join-Path $root "coverage"
$htmlDir = Join-Path $outDir "html"
$xmlFile = Join-Path $outDir "coverage.xml"
New-Item -ItemType Directory -Force -Path $htmlDir | Out-Null

$engineSrc = Join-Path $root "engine"
$gameSrc   = Join-Path $root "game"

Write-Host "OpenCppCoverage : $OccExe"
Write-Host "Test executable  : $testsExe"
Write-Host "Report (html)    : $htmlDir"
Write-Host "Report (cobertura): $xmlFile"

& $OccExe `
  --sources "$engineSrc" `
  --sources "$gameSrc" `
  --excluded_sources "$root/tests" `
  --excluded_sources "$root/vendor" `
  --excluded_sources "$root/build" `
  --excluded_sources "$root/_deps" `
  --export_type "html:$htmlDir" `
  --export_type "cobertura:$xmlFile" `
  --working_dir (Split-Path $testsExe) `
  -- "$testsExe"

if ($LASTEXITCODE -ne 0) {
  Write-Error "OpenCppCoverage / tests exited with code $LASTEXITCODE"
}

Write-Host "`nCoverage report written to $htmlDir/index.html"
