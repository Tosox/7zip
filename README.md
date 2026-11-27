# 7-Zip on GitHub
7-Zip website: [7-zip.org](https://7-zip.org)

## About this fork

This is a fork of [ip7z/7zip](https://github.com/ip7z/7zip) that lets external format plugins describe their own options,
so the **Add to Archive** dialog can show real controls for them instead of only the free-text *Parameters* box.

Changes from upstream:

- Format DLLs may export `GetFormatOptions()` with a small options schema.
  The Add dialog then shows a **Format options...** button for that format, and greys out
  the settings the schema says the format ignores. See [DOC/format-options.md](DOC/format-options.md).
- The *Parameters* box understands quotes: `name="C:\My Mods\file.ltx"` reaches the plugin as one value.

Everything is optional in both directions: plugins with the export still work in stock 7-Zip,
and this fork works with plugins that don't have it.

### Installing

Run `7zip-<version>-x64-setup.exe` from the releases. It installs over an existing 7-Zip, as
7-Zip's own setup does, and asks for a restart if Explorer has the old `7-zip.dll` loaded.
Or unpack the portable zip anywhere and run its `7zFM.exe`; it has no context menu.

### Releases

Versions are tagged `<upstream version>-fork.<n>`, for example `26.06-fork.1`. Pushing such a tag
builds the release; the tag's suffix goes into the version strings.
New upstream releases are merged into `main` (`git merge <upstream tag>`).

### Building on Windows

From an x64 Developer Command Prompt for Visual Studio, run `nmake PLATFORM=x64` in each of
`CPP\7zip\Bundles\Format7zF`, `CPP\7zip\UI\Console`, `CPP\7zip\UI\FileManager`, `CPP\7zip\UI\GUI`
and `CPP\7zip\UI\Explorer`. Output goes to the `x64` folder in each.

For the setup, also run, from an x86 Developer Command Prompt, `nmake PLATFORM=x86 Z7_64BIT_INSTALLER=1`
in `C\Util\7zipInstall` and `C\Util\7zipUninstall`, and `nmake PLATFORM=x86` in
`CPP\7zip\Bundles\SFXWin`, `CPP\7zip\Bundles\SFXCon` and `CPP\7zip\UI\Explorer`. Then
`.github\make-installer.ps1 -Version <version> -Upstream <official 7-Zip x64 setup>` builds it.

nmake does not track header dependencies: after changing a header, delete the `x64` folders
before rebuilding, or the binaries can crash.
