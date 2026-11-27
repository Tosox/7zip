# Builds the setup exe the way 7-Zip's own is made: the installer program (C/Util/7zipInstall)
# followed by a 7z archive of the files it installs. Run it after the x64 and x86 builds.
#
#   -Upstream  the official 7-Zip x64 installer of the same version; the language files and
#              the help come from it, as they aren't part of the source tree
param(
  [Parameter(Mandatory)] [string]$Version,
  [Parameter(Mandatory)] [string]$Upstream,
  [string]$OutDir = "package"
)
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$OutDir = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutDir)
$Upstream = (Resolve-Path -LiteralPath $Upstream).Path

$files = [ordered]@{
  "CPP/7zip/Bundles/Format7zF/x64/7z.dll"     = "7z.dll"
  "CPP/7zip/UI/Console/x64/7z.exe"            = "7z.exe"
  "CPP/7zip/UI/FileManager/x64/7zFM.exe"      = "7zFM.exe"
  "CPP/7zip/UI/GUI/x64/7zG.exe"               = "7zG.exe"
  "CPP/7zip/UI/Explorer/x64/7-zip.dll"        = "7-zip.dll"
  # 32-bit, as in 7-Zip's x64 setup: the context menu for 32-bit programs, the SFX modules
  # and the uninstaller
  "CPP/7zip/UI/Explorer/x86/7-zip.dll"        = "7-zip32.dll"
  "CPP/7zip/Bundles/SFXWin/x86/7z.sfx"        = "7z.sfx"
  "CPP/7zip/Bundles/SFXCon/x86/7zCon.sfx"     = "7zCon.sfx"
  "C/Util/7zipUninstall/x86/7zipUninstall.exe" = "Uninstall.exe"
  "DOC/License.txt"                           = "License.txt"
  "DOC/readme.txt"                            = "readme.txt"
}
$installer = Join-Path $root "C/Util/7zipInstall/x86/7zipInstall.exe"

$stage = Join-Path $OutDir "setup-files"
if (Test-Path $stage) { Remove-Item $stage -Recurse -Force }
New-Item -ItemType Directory -Force $stage | Out-Null
foreach ($src in $files.Keys) { Copy-Item (Join-Path $root $src) (Join-Path $stage $files[$src]) }

$zip = Join-Path $stage "7z.exe"   # next to its 7z.dll
& $zip x -y "-o$stage" $Upstream "Lang" "7-zip.chm" "History.txt" "descript.ion" | Out-Null
if ($LASTEXITCODE -ne 0 -or -not (Test-Path (Join-Path $stage "Lang\en.ttt"))) { throw "no language files in $Upstream" }

# The installer decodes only LZMA and BCJ2, not LZMA2.
$archive = Join-Path $OutDir "setup.7z"
if (Test-Path $archive) { Remove-Item $archive }
& $zip a -t7z -mx=9 -m0=LZMA -mf=BCJ2 $archive "$stage\*" | Out-Null
if ($LASTEXITCODE -ne 0) { throw "7-Zip failed to make $archive" }

$setup = Join-Path $OutDir "7zip-$Version-x64-setup.exe"
[IO.File]::WriteAllBytes($setup, [IO.File]::ReadAllBytes($installer) + [IO.File]::ReadAllBytes($archive))
Remove-Item $archive
$setup
