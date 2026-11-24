# 7-Zip on GitHub
7-Zip website: [7-zip.org](https://7-zip.org)

## About this fork

This is a fork of [ip7z/7zip](https://github.com/ip7z/7zip) that lets external format plugins
(DLLs in the `Formats` folder) describe their own options, so the **Add to Archive** dialog can show
real controls for them instead of only the free-text *Parameters* box.

Changes from upstream:

- Format DLLs may export `GetFormatOptions()` with a small options schema.
  The Add dialog then shows a **Format options...** button for that format.
  See [DOC/format-options.md](DOC/format-options.md).
- The *Parameters* box understands quotes: `name="C:\My Mods\file.ltx"` reaches the plugin as one value.
- Windows x64 CI that builds `7z.exe`, `7z.dll`, `7zFM.exe`, `7zG.exe` and `7-zip.dll` into a portable zip.

Everything is optional in both directions: plugins with the export still work in stock 7-Zip,
and this fork works with plugins that don't have it.

Versions are tagged `<upstream version>-fork.<n>`, for example `26.06-fork.1`.
New upstream releases are merged into `main` (`git merge <upstream tag>`).

### Building on Windows

From an x64 Developer Command Prompt for Visual Studio, run `nmake PLATFORM=x64` in each of
`CPP\7zip\Bundles\Format7zF`, `CPP\7zip\UI\Console`, `CPP\7zip\UI\FileManager`, `CPP\7zip\UI\GUI`
and `CPP\7zip\UI\Explorer`. Output goes to the `x64` folder in each.

nmake does not track header dependencies: after changing a header, delete the `x64` folders
before rebuilding, or the binaries can crash.
