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

  CFormatOption(): Type(NFormatOptionType::kString) {}
};

/* Parses schema text. Returns false (and no options) if the schema
   declares a version other than 1. */
bool ParseFormatOptions(const UString &text, CObjectVector<CFormatOption> &options);

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
