# Format options for external format plugins

This fork lets a format DLL in the `Formats` folder describe its own update options.
The **Add to Archive** dialog then shows a **Format options...** button next to
*Parameters* for that format, and a small dialog with one control per option.

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

INI-like text. Lines starting with `;` or `#` are comments.
Each `[section]` is one option, and the section name is the property name that
`ISetProperties::SetProperties` receives. Keys:

| Key       | Meaning |
|-----------|---------|
| `type`    | `bool`, `enum`, `string`, `file` or `uint`. Unknown types are shown as `string`. |
| `label`   | Text shown in the dialog. Defaults to the option name. |
| `tip`     | Tooltip text. |
| `default` | The value the plugin uses when the option is not given. The dialog writes an option only when it differs from this. For `enum` it defaults to the first value. |
| `values`  | `enum` only: values separated by `\|`. |
| `labels`  | `enum` only: display names for `values`, same count, separated by `\|`. |
| `filter`  | `file` only: `description\|masks` pairs for the open-file dialog, e.g. `LTX files\|*.ltx`. "All Files" is always added. |

Unknown keys are ignored. A top-level `version=` other than `1` makes the fork ignore
the whole schema, so a later version can change the format without breaking older builds.

```ini
version=1

[compress]
type=enum
values=auto|on|off
labels=Auto|Always|Never
default=auto
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

Options apply when an archive is created or updated through the Add dialog.
The File Manager does not pass options when it opens an archive.
