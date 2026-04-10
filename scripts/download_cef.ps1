# Download CEF (Chromium Embedded Framework) for ZAP Browser
# Run this script from the zap-browser root directory.

param(
    [string]$CefVersion = "latest",
    [string]$OutputDir  = ".\third_party\cef"
)

$ErrorActionPreference = "Stop"

Write-Host "⚡ ZAP Browser — CEF Setup Script" -ForegroundColor Cyan
Write-Host ""

# ── Fetch latest CEF build info ──────────────────────────────────────────────
$spotifyIndex = "https://cef-builds.spotifycdn.com/index.json"

Write-Host "📡 Fetching CEF build index..." -ForegroundColor Yellow
$index = Invoke-RestMethod -Uri $spotifyIndex

# Find latest stable Windows 64-bit standard distribution
$win64 = $index.'windows64'
$latestBuild = $win64.versions |
    Where-Object { $_.channel -eq "stable" } |
    Select-Object -First 1

if (-not $latestBuild) {
    Write-Error "Could not find a stable Windows 64-bit CEF build."
    exit 1
}

$cefFileName = ($latestBuild.files | Where-Object { $_.type -eq "standard" }).name
$cefUrl      = "https://cef-builds.spotifycdn.com/$cefFileName"

Write-Host "✅ Found: $cefFileName" -ForegroundColor Green
Write-Host "🔗 URL: $cefUrl" -ForegroundColor DarkGray
Write-Host ""

# ── Download ─────────────────────────────────────────────────────────────────
$zipPath = ".\third_party\$cefFileName"
New-Item -ItemType Directory -Force -Path ".\third_party" | Out-Null

Write-Host "⬇  Downloading CEF (~250MB)..." -ForegroundColor Yellow
Write-Host "   This may take several minutes on slower connections."
Write-Host ""

$ProgressPreference = 'SilentlyContinue'  # faster download
Invoke-WebRequest -Uri $cefUrl -OutFile $zipPath

Write-Host "✅ Download complete." -ForegroundColor Green

# ── Extract ──────────────────────────────────────────────────────────────────
Write-Host "📦 Extracting to $OutputDir ..." -ForegroundColor Yellow

if (Test-Path $OutputDir) {
    Remove-Item $OutputDir -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

# Use 7-zip if available (faster), else Expand-Archive
$sevenZip = Get-Command "7z" -ErrorAction SilentlyContinue
if ($sevenZip) {
    & 7z x $zipPath -o".\third_party\" -y | Out-Null
} else {
    Expand-Archive -Path $zipPath -DestinationPath ".\third_party\" -Force
}

# Rename extracted folder to just "cef"
$extracted = Get-ChildItem ".\third_party\" -Directory |
    Where-Object { $_.Name -like "cef_binary_*" } |
    Select-Object -First 1

if ($extracted) {
    Rename-Item $extracted.FullName $OutputDir -Force
}

Remove-Item $zipPath -Force

Write-Host ""
Write-Host "✅ CEF installed to: $OutputDir" -ForegroundColor Green
Write-Host ""
Write-Host "📋 Next steps:" -ForegroundColor Cyan
Write-Host "   1. cmake -B build -G 'Visual Studio 17 2022' -A x64 \"
Write-Host "         -DCMAKE_PREFIX_PATH='C:/Qt/6.6.0/msvc2022_64' \"
Write-Host "         -DCEF_ROOT='./third_party/cef'"
Write-Host "   2. cmake --build build --config Release"
Write-Host "   3. .\build\bin\Release\zap.exe"
