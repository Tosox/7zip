// FormatOptions.cpp

#include "StdAfx.h"

#include "FormatOptions.h"

static bool IsSpaceChar(wchar_t c)
{
  return c == ' ' || c == '\t';
}

static void TrimLine(UString &s)
{
  s.Trim();
  while (!s.IsEmpty() && s.Back() == '\r')
  {
    s.DeleteBack();
    s.Trim();
  }
}

static void SplitByBar(const UString &s, UStringVector &parts)
{
  parts.Clear();
  if (s.IsEmpty())
    return;
  UString part;
  for (unsigned i = 0; i < s.Len(); i++)
  {
    const wchar_t c = s[i];
    if (c != '|')
    {
      part += c;
      continue;
    }
    part.Trim();
    parts.Add(part);
    part.Empty();
  }
  part.Trim();
  parts.Add(part);
}

static int FindValue(const UStringVector &values, const UString &value)
{
  FOR_VECTOR (i, values)
    if (values[i].IsEqualTo_NoCase(value))
      return (int)i;
  return -1;
}

static NFormatOptionType::EEnum ParseType(const UString &s)
{
  if (s.IsEqualTo_Ascii_NoCase("bool")) return NFormatOptionType::kBool;
  if (s.IsEqualTo_Ascii_NoCase("enum")) return NFormatOptionType::kEnum;
  if (s.IsEqualTo_Ascii_NoCase("file")) return NFormatOptionType::kFile;
  if (s.IsEqualTo_Ascii_NoCase("uint")) return NFormatOptionType::kUInt;
  return NFormatOptionType::kString; // unknown types fall back to a text field
}

bool ParseFormatOptions(const UString &text, CObjectVector<CFormatOption> &options,
    UStringVector *ignore)
{
  options.Clear();
  if (ignore)
    ignore->Clear();
  CFormatOption *cur = NULL;

  unsigned pos = 0;
  while (pos < text.Len())
  {
    int end = text.Find(L'\n', pos);
    if (end < 0)
      end = (int)text.Len();
    UString line;
    line.SetFrom(text.Ptr(pos), (unsigned)end - pos);
    pos = (unsigned)end + 1;
    TrimLine(line);

    if (line.IsEmpty() || line[0] == ';' || line[0] == '#')
      continue;

    if (line[0] == '[')
    {
      cur = NULL;
      if (line.Len() < 3 || line.Back() != ']')
        continue;
      UString name;
      name.SetFrom(line.Ptr(1), line.Len() - 2);
      name.Trim();
      if (name.IsEmpty())
        continue;
      cur = &options.AddNew();
      cur->Name = name;
      continue;
    }

    const int eq = line.Find(L'=');
    if (eq <= 0)
      continue;
    UString key, value;
    key.SetFrom(line, (unsigned)eq);
    key.Trim();
    value = line.Ptr((unsigned)eq + 1);
    value.Trim();

    if (!cur)
    {
      if (key.IsEqualTo_Ascii_NoCase("version") && value != L"1")
      {
        options.Clear();
        if (ignore)
          ignore->Clear();
        return false;
      }
      if (key.IsEqualTo_Ascii_NoCase("ignore") && ignore)
        SplitByBar(value, *ignore);
      continue;
    }

    if (key.IsEqualTo_Ascii_NoCase("type"))
      cur->Type = ParseType(value);
    else if (key.IsEqualTo_Ascii_NoCase("label"))
      cur->Label = value;
    else if (key.IsEqualTo_Ascii_NoCase("tip"))
      cur->Tip = value;
    else if (key.IsEqualTo_Ascii_NoCase("default"))
      cur->Default = value;
    else if (key.IsEqualTo_Ascii_NoCase("values"))
      SplitByBar(value, cur->Values);
    else if (key.IsEqualTo_Ascii_NoCase("labels"))
      SplitByBar(value, cur->Labels);
    else if (key.IsEqualTo_Ascii_NoCase("filter"))
      SplitByBar(value, cur->Filter);
    else if (key.IsEqualTo_Ascii_NoCase("new_values"))
      SplitByBar(value, cur->NewValues);
    else if (key.IsEqualTo_Ascii_NoCase("new_default"))
      cur->NewDefault = value;
    // unknown keys are ignored, so later schema versions can add keys
  }

  FOR_VECTOR (i, options)
  {
    CFormatOption &o = options[i];
    if (o.Label.IsEmpty())
      o.Label = o.Name;
    if (o.Type == NFormatOptionType::kEnum && o.Values.IsEmpty())
      o.Type = NFormatOptionType::kString;
    if (o.Type == NFormatOptionType::kEnum && o.Default.IsEmpty())
      o.Default = o.Values[0];
    if (o.Labels.Size() != o.Values.Size())
      o.Labels.Clear();
    if ((o.Filter.Size() & 1) != 0)
      o.Filter.DeleteBack();

    if (o.Type != NFormatOptionType::kEnum || FindValue(o.Values, o.NewDefault) < 0)
      o.NewDefault.Empty();
    if (o.Type != NFormatOptionType::kEnum)
      o.NewValues.Clear();
    for (unsigned k = 0; k < o.NewValues.Size();)
      if (FindValue(o.Values, o.NewValues[k]) < 0)
        o.NewValues.Delete(k);
      else
        k++;
    // The new archive's default must be one of the values it shows.
    if (FindValue(o.NewValues, o.NewDefault.IsEmpty() ? o.Default : o.NewDefault) < 0)
      o.NewValues.Clear();
  }
  return true;
}

void ApplyNewArchive(CObjectVector<CFormatOption> &options)
{
  FOR_VECTOR (i, options)
  {
    CFormatOption &o = options[i];
    if (!o.NewDefault.IsEmpty())
      o.Default = o.NewDefault;
    if (o.NewValues.IsEmpty())
      continue;
    UStringVector labels;
    FOR_VECTOR (k, o.NewValues)
    {
      const int index = FindValue(o.Values, o.NewValues[k]);
      o.NewValues[k] = o.Values[(unsigned)index]; // the spelling from values=
      if (!o.Labels.IsEmpty())
        labels.Add(o.Labels[(unsigned)index]);
    }
    o.Values = o.NewValues;
    o.Labels = labels;
  }
}

void SplitQuotedOptions(const UString &src, UStringVector &tokens)
{
  tokens.Clear();
  UString token;
  bool inQuotes = false;
  for (unsigned i = 0; i < src.Len(); i++)
  {
    const wchar_t c = src[i];
    if (c == '"')
    {
      inQuotes = !inQuotes;
      continue;
    }
    if (!inQuotes && IsSpaceChar(c))
    {
      if (!token.IsEmpty())
        tokens.Add(token);
      token.Empty();
      continue;
    }
    token += c;
  }
  if (!token.IsEmpty())
    tokens.Add(token);
}

void AppendOptionToken(UString &params, const UString &token)
{
  if (token.IsEmpty())
    return;
  if (!params.IsEmpty() && !IsSpaceChar(params.Back()))
    params.Add_Space();

  bool needQuotes = false;
  for (unsigned i = 0; i < token.Len(); i++)
    if (IsSpaceChar(token[i]))
      needQuotes = true;
  if (!needQuotes)
  {
    params += token;
    return;
  }

  const int eq = token.Find(L'=');
  const unsigned valuePos = (eq < 0) ? 0 : (unsigned)eq + 1;
  UString name;
  name.SetFrom(token, valuePos);
  params += name;
  params += L'"';
  params += token.Ptr(valuePos);
  params += L'"';
}

void ParseOptionToken(const UString &token, UString &name, UString &value)
{
  unsigned start = 0;
  if (token.Len() > 2 && token[0] == '-' && (token[1] == 'm' || token[1] == 'M'))
    start = 2;
  const UString t (token.Ptr(start));
  value.Empty();
  const int eq = t.Find(L'=');
  if (eq >= 0)
  {
    name.SetFrom(t, (unsigned)eq);
    value = t.Ptr((unsigned)eq + 1);
    return;
  }
  name = t;
  if (!name.IsEmpty() && (name.Back() == '+' || name.Back() == '-'))
  {
    value += name.Back();
    name.DeleteBack();
  }
}
