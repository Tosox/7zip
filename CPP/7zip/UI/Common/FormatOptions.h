// FormatOptions.h
// Options that an external format DLL describes through its optional
// GetFormatOptions() export. See DOC/format-options.md.

#ifndef ZIP7_INC_FORMAT_OPTIONS_H
#define ZIP7_INC_FORMAT_OPTIONS_H

#include "../../../Common/MyString.h"

namespace NFormatOptionType
{
  enum EEnum
  {
    kString,
    kBool,
    kEnum,
    kFile,
    kUInt
  };
}

struct CFormatOption
{
  NFormatOptionType::EEnum Type;
  UString Name;
  UString Label;
  UString Tip;
  UString Default;
  UStringVector Values;   // kEnum: values written to Parameters
  UStringVector Labels;   // kEnum: display names (same count as Values, or empty)
  UStringVector Filter;   // kFile: pairs of (description, masks)
  UStringVector NewValues; // kEnum: the values shown for a new archive (empty: all)
  UString NewDefault;      // kEnum: the default for a new archive (empty: Default)

  CFormatOption(): Type(NFormatOptionType::kString) {}
};

/* Parses schema text. Returns false (and no options) if the schema
   declares a version other than 1.
   (ignore), if given, receives the top-level "ignore=" list: the Add dialog's
   settings that the format doesn't use ("level", "threads", "memory"). */
bool ParseFormatOptions(const UString &text, CObjectVector<CFormatOption> &options,
    UStringVector *ignore = NULL);

/* Turns the options into what the dialog shows for a new archive:
   NewValues replace Values (with their labels) and NewDefault replaces Default. */
void ApplyNewArchive(CObjectVector<CFormatOption> &options);

/* Splits Parameters text at spaces and tabs.
   Characters inside "..." are kept together and the quotes are removed. */
void SplitQuotedOptions(const UString &src, UStringVector &tokens);

/* Appends a "name=value" or "name" token to (params),
   quoting the value if it contains spaces or tabs. */
void AppendOptionToken(UString &params, const UString &token);

/* Splits a token into name and value.
   "name=value" -> value; "name" -> ""; "name+" -> "+"; "name-" -> "-".
   A leading "-m" (CLI style) is ignored. */
void ParseOptionToken(const UString &token, UString &name, UString &value);

#endif
