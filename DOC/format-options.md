# Format options for external format plugins

This fork lets a format DLL in the `Formats` folder describe its own update options.
The **Add to Archive** dialog then shows a **Format options...** button next to
*Parameters* for that format, and a small dialog with one control per option.
The DLL can also name the Add dialog's settings that it doesn't use, and the dialog greys them out.

The mechanism is optional on both sides:

- Stock 7-Zip never looks for the export, so a plugin that has it still works there
  (options are typed into *Parameters* as before).
- This fork shows no button for plugins without the export.

## The export

```cpp
extern "C" __declspec(dllexport)
HRESULT WINAPI GetFormatOptions(UInt32 formatIndex, PROPVARIANT *value);
```

- `formatIndex` is the same index that `GetHandlerProperty2` receives.
- Return `S_OK` with a `VT_BSTR` holding the schema text, or `VT_EMPTY` when the format has no options.
- The typedef is `Func_GetFormatOptions` in `CPP/7zip/Archive/IArchive.h`.

If the DLL uses a `.def` file (for example `Archive2.def`), `__declspec(dllexport)` adds this
export next to the ones the `.def` file lists.

## Schema text (version 1)

INI-like text. Lines starting with `;` or `#` are comments. Unknown keys are ignored, so later
versions can add keys without breaking older builds.

```ini
version=1
ignore=level|threads|memory|volumes

[compress]
type=enum
values=auto|on|off
labels=Keep existing|Compress|Store
default=auto
new_values=on|off
new_default=on
label=Compression

[userdata]
type=file
label=Userdata file
filter=LTX files|*.ltx
tip=Copied into the archive's userdata block

[keep_duplicates]
type=bool
default=off
label=Keep duplicate paths
```

### Top-level keys

These come before the first `[section]`.

| Key       | Meaning |
|-----------|---------|
| `version` | `1`. Any other value makes the fork ignore the whole schema. |
| `ignore`  | The Add dialog's settings that the format doesn't use, separated by `\|`: `level`, `threads`, `memory` and `volumes`. The dialog greys them out. |

7-Zip still passes the values of `level`, `threads` and `memory` (`x`, `mt`, `memuse`), so the
plugin must keep accepting them. `volumes` also clears *Split to volumes*, so the dialog writes
one file; use it for formats whose readers can't open split archives. The command line's `-v`
still splits.

A schema with `ignore=` and no options greys the settings out without showing the
**Format options...** button.

### Options

Each `[section]` is one option, and the section name is the property name that
`ISetProperties::SetProperties` receives.

| Key           | Meaning |
|---------------|---------|
| `type`        | `bool`, `enum`, `string`, `file` or `uint`. Unknown types are shown as `string`. |
| `label`       | Text shown in the dialog. Defaults to the option name. |
| `tip`         | Tooltip text. |
| `default`     | The value the plugin uses when the option is not given. The dialog writes an option only when it differs from this. For `enum` it defaults to the first value. |
| `values`      | `enum` only: values separated by `\|`. |
| `labels`      | `enum` only: display names for `values`, same count, separated by `\|`. |
| `filter`      | `file` only: `description\|masks` pairs for the open-file dialog, e.g. `LTX files\|*.ltx`. "All Files" is always added. |
| `new_values`  | `enum` only: the values shown for a new archive, separated by `\|`. Labels still come from `labels`. Default: all `values`. |
| `new_default` | `enum` only: the value the plugin uses for a new archive when the option is not given. Default: `default`. |

### New archives

`new_values` and `new_default` are for a `default` such as `auto` that means "like the archive
being updated". For a new archive it means something fixed, which the dialog can show instead:
in the example above, a new archive offers *Compress* and *Store*, with *Compress* selected.

The dialog uses them when the archive doesn't exist at the moment **Format options...** is
pressed. Keeping `new_default` writes nothing to *Parameters*, so the remembered *Parameters*
don't force that value on later updates.

The fork ignores values that aren't in `values`, a `new_default` that isn't in `values`, and a
`new_values` that leaves out the new archive's default (`new_default`, or else `default`).

## What the plugin receives

The dialog only edits the *Parameters* text, so values arrive through the normal
`SetProperties` path, exactly as if the user had typed them:

- `bool` is written as `name=on` or `name=off`. Parse it with `PROPVARIANT_to_bool`
  (`CPP/7zip/Common/MethodProps.cpp`), which also accepts `name`, `name+` and `name-`.
- Any value made only of digits arrives as `VT_UI4` (or `VT_UI8`), whatever its `type`;
  this includes `enum` values such as `2215`. Everything else arrives as `VT_BSTR`.
- Values containing spaces are quoted in *Parameters* (`name="C:\My Mods\ud.ltx"`).
  The fork strips the quotes, so the plugin receives the plain value.
  Stock 7-Zip does not: it splits *Parameters* at every space.
- Tokens in *Parameters* that the schema does not know are kept as they are.
- So is a value that a `bool` or `enum` control can't show (`dbver=9999`, `flag=yes`),
  unless the user changes that control, so the plugin can still reject or accept it.

The values are remembered per format together with the rest of *Parameters*.

Options apply when an archive is created or updated through the Add dialog. Changes made in
the File Manager without it (dragging files in, deleting, renaming, editing with F4) pass no
options, so the plugin uses its defaults.
