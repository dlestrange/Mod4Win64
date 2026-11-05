# Build a RELEASE binary, deploy Qt and libopenmpt, and ZIP it.
# Example: .\scripts\package-windows.ps1 -BuildDir out\build\x64-Release -LibOpenMptDll C:\SDK\libopenmpt.dll -QtBin C:\Qt\6.12.0\msvc2022_64\bin
param(
    [string]$BuildDir = "out\build\x64-Release",
    [Parameter(Mandatory=$true)][string]$LibOpenMptDll,
    [string]$QtBin = "",
    [string]$OutputDir = "dist",
    [string[]]$ExtraDlls = @(),
    [switch]$SkipBuild
)
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
function FullPath([string]$path) {
    if ([System.IO.Path]::IsPathRooted($path)) { return $path }
    return (Join-Path $repo $path)
}
$build = FullPath $BuildDir
$output = FullPath $OutputDir
$openmpt = FullPath $LibOpenMptDll
if (-not (Test-Path -LiteralPath $openmpt -PathType Leaf)) {
    throw "libopenmpt.dll not found: $openmpt"
}
if ([System.IO.Path]::GetFileName($openmpt) -ine "libopenmpt.dll") {
    throw "Please pass the actual x64 libopenmpt.dll with -LibOpenMptDll."
}
if (-not $SkipBuild) {
    Write-Host "Building RELEASE from $build"
    & cmake --build $build --config Release
    if ($LASTEXITCODE -ne 0) { throw "Release build failed." }
}
# Ninja places exe directly in build folder; VS generators use Release subdir.
$exe = Join-Path $build "Mod4Win64.exe"
if (-not (Test-Path $exe)) { $exe = Join-Path $build "Release\Mod4Win64.exe" }
if (-not (Test-Path $exe)) {
    throw "Release Mod4Win64.exe not found in $build. Configure a Release profile."
}
if ($QtBin) {
    $deploy = Join-Path (FullPath $QtBin) "windeployqt.exe"
} else {
    $command = Get-Command "windeployqt.exe" -ErrorAction SilentlyContinue
    if (-not $command) { throw "Pass -QtBin C:\Qt\6.12.0\msvc2022_64\bin" }
    $deploy = $command.Source
}
if (-not (Test-Path $deploy)) { throw "windeployqt.exe not found: $deploy" }
$stage = Join-Path $output "Mod4Win64"
New-Item -ItemType Directory -Force -Path $output | Out-Null
if (Test-Path $stage) { Remove-Item $stage -Recurse -Force }
New-Item -ItemType Directory -Force -Path $stage | Out-Null
$stageExe = Join-Path $stage "Mod4Win64.exe"
Copy-Item -LiteralPath $exe -Destination $stageExe
Copy-Item -LiteralPath $openmpt -Destination (Join-Path $stage "libopenmpt.dll")
# The official libopenmpt DLL is NOT standalone: it loads these three
# companion DLLs on Windows. windeployqt cannot discover them because the
# application loads libopenmpt dynamically rather than linking against it.
# Fail the package if any are absent instead of shipping a broken ZIP.
$decoderFolder = Split-Path -Parent $openmpt
$openmptCompanions = @("openmpt-mpg123.dll", "openmpt-vorbis.dll", "openmpt-zlib.dll")
foreach ($name in $openmptCompanions) {
    $source = Join-Path $decoderFolder $name
    if (-not (Test-Path -LiteralPath $source -PathType Leaf)) {
        throw "Missing libopenmpt companion DLL: $source. Use the full matching x64 libopenmpt distribution."
    }
    Copy-Item -LiteralPath $source -Destination (Join-Path $stage $name)
}
foreach ($dll in $ExtraDlls) {
    $file = FullPath $dll
    if (-not (Test-Path $file)) { throw "Additional DLL not found: $file" }
    Copy-Item -LiteralPath $file -Destination $stage
}
Write-Host "Deploying Qt libraries, plugins, and compiler runtime..."
& $deploy --release --compiler-runtime --dir $stage $stageExe
if ($LASTEXITCODE -ne 0) { throw "windeployqt failed: check Qt kit/architecture." }
Copy-Item (Join-Path $repo "LICENSE") (Join-Path $stage "LICENSE.txt")
Copy-Item (Join-Path $repo "docs\RELEASING-WINDOWS.md") (Join-Path $stage "README-RELEASE.txt")
$zip = Join-Path $output "Mod4Win64-Windows-x64-portable.zip"
if (Test-Path $zip) { Remove-Item $zip -Force }
Compress-Archive -Path $stage -DestinationPath $zip -CompressionLevel Optimal
Write-Host ""
Write-Host "Portable folder: $stage"
Write-Host "Portable ZIP:    $zip"
Write-Warning "Check libopenmpt DLL architecture/dependencies and bundle applicable third-party licenses before PUBLIC distribution."
Write-Warning "Test this ZIP on a clean Windows machine without Qt/Visual Studio installed."
