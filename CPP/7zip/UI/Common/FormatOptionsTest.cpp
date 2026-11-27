// FormatOptionsTest.cpp
// Self-check for FormatOptions.cpp. Not part of any 7-Zip binary; built and run by CI:
//   cl -nologo -EHsc -W4 -WX FormatOptionsTest.cpp FormatOptions.cpp
//      ..\..\..\Common\MyString.cpp ..\..\..\Common\IntToString.cpp user32.lib oleaut32.lib

#include "StdAfx.h"

#include <stdio.h>

#include "FormatOptions.h"

static int g_Failures = 0;

#define CHECK(cond) do { if (!(cond)) { printf("FAIL line %d: %s\n", __LINE__, #cond); g_Failures++; } } while (0)

static UString Join(const UStringVector &v)
{
  UString s;
  FOR_VECTOR (i, v)
  {
    s += L'<';
    s += v[i];
    s += L'>';
  }
  return s;
}

static void TestParse()
{
  CObjectVector<CFormatOption> opts;
  const UString text (
      L"version=1\r\n"
      L"; comment\r\n"
      L"[compress]\r\n"
      L"type=enum\r\n"
      L"values=auto|on|off\r\n"
      L"labels=Auto|Always|Never\r\n"
      L"default=auto\r\n"
      L"label=Compression\r\n"
      L"future_key=ignored\r\n"
      L"[ xdb_ud ]\n"
      L"type=file\n"
      L"filter=LTX files|*.ltx|odd\n"
      L"[flag]\n"
      L"type=bool\n"
      L"[bad_enum]\n"
      L"type=enum\n"
      L"labels=A|B\n"
      L"[mode]\n"
      L"type=enum\n"
      L"values=ltx|bin\n"
      L"[weird]\n"
      L"type=color\n"
      L"[]\n"
      L"orphan=1\n");
  CHECK(ParseFormatOptions(text, opts));
  CHECK(opts.Size() == 6);
  if (opts.Size() != 6)
    return;

  CHECK(opts[0].Name == L"compress");
  CHECK(opts[0].Type == NFormatOptionType::kEnum);
  CHECK(Join(opts[0].Values) == L"<auto><on><off>");
  CHECK(Join(opts[0].Labels) == L"<Auto><Always><Never>");
  CHECK(opts[0].Default == L"auto");
  CHECK(opts[0].Label == L"Compression");

  CHECK(opts[1].Name == L"xdb_ud");
  CHECK(opts[1].Type == NFormatOptionType::kFile);
  CHECK(Join(opts[1].Filter) == L"<LTX files><*.ltx>"); // odd trailing entry dropped
  CHECK(opts[1].Label == L"xdb_ud");                    // label falls back to name

  CHECK(opts[2].Type == NFormatOptionType::kBool);
  CHECK(opts[3].Type == NFormatOptionType::kString);    // enum without values
  CHECK(opts[3].Labels.IsEmpty());
  CHECK(opts[4].Type == NFormatOptionType::kEnum);
  CHECK(opts[4].Default == L"ltx");                     // enum default falls back to first value
  CHECK(opts[5].Type == NFormatOptionType::kString);    // unknown type

  CHECK(!ParseFormatOptions(UString(L"version=2\n[a]\ntype=bool\n"), opts));
  CHECK(opts.IsEmpty());
  CHECK(ParseFormatOptions(UString(L""), opts));
  CHECK(opts.IsEmpty());

  UStringVector ignore;
  CHECK(ParseFormatOptions(UString(L"version=1\nignore= level | threads|memory\n[a]\nignore=x\n"), opts, &ignore));
  CHECK(Join(ignore) == L"<level><threads><memory>"); // a section's ignore= is not the format's
  CHECK(!ParseFormatOptions(UString(L"ignore=level\nversion=2\n"), opts, &ignore));
  CHECK(ignore.IsEmpty());
  CHECK(ParseFormatOptions(UString(L"[a]\n"), opts, &ignore));
  CHECK(ignore.IsEmpty());
}

static void TestNewArchive()
{
  CObjectVector<CFormatOption> opts;
  CHECK(ParseFormatOptions(UString(
      L"[both]\ntype=enum\nvalues=auto|on|off\nlabels=Keep|On|Off\ndefault=auto\nnew_values=OFF|on|bogus\nnew_default=on\n"
      L"[default_only]\ntype=enum\nvalues=auto|a|b\nnew_default=b\n"
      L"[values_only]\ntype=enum\nvalues=auto|a|b\nnew_values=a|auto\n"
      L"[no_default]\ntype=enum\nvalues=auto|a|b\nnew_values=a|b\n"
      L"[bad_default]\ntype=enum\nvalues=auto|a|b\nnew_values=a|b\nnew_default=c\n"
      L"[flag]\ntype=bool\nnew_default=on\nnew_values=on\n"), opts));
  CHECK(opts.Size() == 6);
  if (opts.Size() != 6)
    return;
  CHECK(Join(opts[0].NewValues) == L"<OFF><on>");   // unknown values dropped
  CHECK(opts[3].NewValues.IsEmpty());                // leaves out the default without new_default
  CHECK(opts[4].NewDefault.IsEmpty());               // not one of the values
  CHECK(opts[4].NewValues.IsEmpty());
  CHECK(opts[5].NewDefault.IsEmpty() && opts[5].NewValues.IsEmpty()); // enum only

  ApplyNewArchive(opts);
  CHECK(Join(opts[0].Values) == L"<off><on>");       // spelled as in values=
  CHECK(Join(opts[0].Labels) == L"<Off><On>");
  CHECK(opts[0].Default == L"on");
  CHECK(Join(opts[1].Values) == L"<auto><a><b>");
  CHECK(opts[1].Default == L"b");
  CHECK(Join(opts[2].Values) == L"<a><auto>");
  CHECK(opts[2].Labels.IsEmpty());
  CHECK(opts[2].Default == L"auto");
  CHECK(Join(opts[3].Values) == L"<auto><a><b>");
  CHECK(opts[3].Default == L"auto");
}

static void TestSplit()
{
  UStringVector t;
  SplitQuotedOptions(UString(L"  a=1   b \t c=\"x y\"  \"d=e f\" g=\"\" "), t);
  CHECK(Join(t) == L"<a=1><b><c=x y><d=e f><g=>");

  SplitQuotedOptions(UString(L"x=\"unterminated value"), t);
  CHECK(Join(t) == L"<x=unterminated value>");

  SplitQuotedOptions(UString(L""), t);
  CHECK(t.IsEmpty());
}

static void TestAppendAndRoundTrip()
{
  UString p;
  AppendOptionToken(p, UString(L"a=1"));
  AppendOptionToken(p, UString(L"path=C:\\my dir\\u.ltx"));
  AppendOptionToken(p, UString(L"flag"));
  AppendOptionToken(p, UString(L""));
  CHECK(p == L"a=1 path=\"C:\\my dir\\u.ltx\" flag");

  UStringVector t;
  SplitQuotedOptions(p, t);
  CHECK(Join(t) == L"<a=1><path=C:\\my dir\\u.ltx><flag>");
}

static void TestParseToken()
{
  UString n, v;
  ParseOptionToken(UString(L"name=val=ue"), n, v);
  CHECK(n == L"name" && v == L"val=ue");
  ParseOptionToken(UString(L"-mx=9"), n, v);
  CHECK(n == L"x" && v == L"9");
  ParseOptionToken(UString(L"flag"), n, v);
  CHECK(n == L"flag" && v.IsEmpty());
  ParseOptionToken(UString(L"flag-"), n, v);
  CHECK(n == L"flag" && v == L"-");
  ParseOptionToken(UString(L"flag+"), n, v);
  CHECK(n == L"flag" && v == L"+");
}

int main()
{
  TestParse();
  TestNewArchive();
  TestSplit();
  TestAppendAndRoundTrip();
  TestParseToken();
  if (g_Failures != 0)
  {
    printf("%d check(s) failed\n", g_Failures);
    return 1;
  }
  printf("FormatOptionsTest: all checks passed\n");
  return 0;
}
