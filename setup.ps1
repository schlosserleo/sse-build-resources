# Bootstraps the vendored skse64/ tree: downloads the SKSE64 2.0.17 sources
# from the official site and applies skse64-patch.zip on top.
$ErrorActionPreference = "Stop"

$root = $PSScriptRoot
$dest = Join-Path $root "skse64"

if (Test-Path $dest) {
    throw "'$dest' already exists - remove it first if you want to re-run setup."
}

$url     = "https://skse.silverlock.org/beta/skse64_2_00_17.7z"
$archive = Join-Path $env:TEMP "skse64_2_00_17.7z"
$extract = Join-Path $env:TEMP "skse64_2_00_17_extract"

Write-Host "Downloading $url"
Invoke-WebRequest -Uri $url -OutFile $archive

Remove-Item -Recurse -Force $extract -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Path $extract | Out-Null

Write-Host "Extracting..."
$extracted = $false
try {
    # Windows' bundled bsdtar can read 7z archives
    tar -xf $archive -C $extract
    if ($LASTEXITCODE -eq 0) { $extracted = $true }
} catch {}

if (-not $extracted) {
    $sevenZip = @(
        "$env:ProgramFiles\7-Zip\7z.exe",
        "${env:ProgramFiles(x86)}\7-Zip\7z.exe"
    ) | Where-Object { Test-Path $_ } | Select-Object -First 1
    if (-not $sevenZip) {
        throw "Could not extract the archive - install 7-Zip or a tar with 7z support."
    }
    & $sevenZip x $archive "-o$extract" -y | Out-Null
    if ($LASTEXITCODE -ne 0) { throw "7-Zip extraction failed." }
}

Write-Host "Copying sources..."
Copy-Item -Recurse (Join-Path $extract "skse64_2_00_17\src") $dest

Write-Host "Applying patch..."
Expand-Archive -Path (Join-Path $root "skse64-patch.zip") -DestinationPath $dest -Force
Push-Location $dest
try {
    git init --quiet
    git -c core.autocrlf=false apply --whitespace=nowarn skse64.patch
    if ($LASTEXITCODE -ne 0) { throw "git apply failed." }
} finally {
    Pop-Location
}

Remove-Item -Recurse -Force $extract -ErrorAction SilentlyContinue
Remove-Item -Force $archive -ErrorAction SilentlyContinue

Write-Host "Done - skse64/ is ready."
