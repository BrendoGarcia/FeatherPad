param([string]$Tag = '')
$ErrorActionPreference = 'Stop'
$version = (Select-String -Path CMakeLists.txt -Pattern 'project\(FeatherPad VERSION ([0-9]+\.[0-9]+\.[0-9]+)').Matches.Groups[1].Value
if (-not $version) { throw 'CMake version missing' }
if ($Tag -and $Tag -cne "v$version") { throw 'Tag must match the CMake stable version' }
$binaryVersion = (Get-Item build/Release/FeatherPad.exe).VersionInfo.FileVersion
if ($binaryVersion -ne $version) { throw "EXE version $binaryVersion differs from CMake $version" }
Copy-Item LEIA-ME.md,LICENSE build/Release/
New-Item release-assets -ItemType Directory -Force | Out-Null
Copy-Item build/Release/FeatherPad.exe,build/Release/Updater.exe,build/Release/SDL3.dll,LEIA-ME.md,LICENSE release-assets/
Copy-Item build/Release/licenses/SDL3.txt,build/Release/licenses/ViGEmClient.txt,build/Release/licenses/zlib.txt release-assets/
$zip = "release-assets/FeatherPad-Windows-x64-$version.zip"
Compress-Archive -Path build/Release/FeatherPad.exe,build/Release/Updater.exe,build/Release/SDL3.dll,build/Release/licenses,build/Release/LEIA-ME.md,build/Release/LICENSE -DestinationPath $zip -Force
$archive = [System.IO.Compression.ZipFile]::OpenRead((Resolve-Path $zip))
try {
  $names = @($archive.Entries | ForEach-Object { $_.FullName.Replace('\', '/') })
  foreach ($required in @('FeatherPad.exe','Updater.exe','SDL3.dll','LICENSE','LEIA-ME.md','licenses/SDL3.txt','licenses/ViGEmClient.txt')) {
    if ($required -cnotin $names) { throw "Missing package file: $required" }
  }
} finally { $archive.Dispose() }
& ./build/Release/update_tests.exe --verify-package $zip
if ($LASTEXITCODE -ne 0) { throw 'Native updater cannot read the release ZIP' }
Get-ChildItem release-assets -File | Where-Object Name -ne 'SHA256SUMS.txt' | Sort-Object Name | ForEach-Object {
  "$((Get-FileHash $_.FullName -Algorithm SHA256).Hash.ToLower())  $($_.Name)"
} | Set-Content release-assets/SHA256SUMS.txt -Encoding utf8
Write-Host "Verified native package and individual update assets for $version"
