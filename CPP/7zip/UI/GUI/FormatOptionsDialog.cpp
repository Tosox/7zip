// FormatOptionsDialog.cpp

#include "StdAfx.h"

#include "../../../Windows/CommonDialog.h"
#include "../../../Windows/Control/ComboBox.h"

#include "../../Common/MethodProps.h"

#include "FormatOptionsDialog.h"

using namespace NWindows;

// Layout in dialog units. kContentWidth must match xc in FormatOptionsDialog.rc.
static const int kMargin = 8;
static const int kContentWidth = 272;
static const int kLabelWidth = 96;
static const int kGap = 4;
static const int kControlX = kMargin + kLabelWidth + kGap;
static const int kControlWidth = kContentWidth - kLabelWidth - kGap;
static const int kBrowseWidth = 20;
static const int kRowHeight = 18;
static const int kEditHeight = 14;
static const int kComboListHeight = 120;

// Option i uses control ID (kFirstId + 2*i); a file option's browse button uses (kFirstId + 2*i + 1).
static const unsigned kFirstId = 1000;

static bool GetBoolDefault(const CFormatOption &o)
{
  bool b = false;
  if (!o.Default.IsEmpty() && !StringToBool(o.Default, b))
    b = false;
  return b;
}

// Returns the value from the last token with this option's name, or the option's default.
static UString GetInitialValue(const CFormatOption &o, const UStringVector &tokens, bool &found)
{
  UString value = o.Default;
  found = false;
  FOR_VECTOR (i, tokens)
  {
    UString name, v;
    ParseOptionToken(tokens[i], name, v);
    if (name.IsEqualTo_NoCase(o.Name))
    {
      value = v;
      found = true;
    }
  }
  return value;
}

RECT CFormatOptionsDialog::DluRect(int x, int y, int width, int height)
{
  RECT r;
  r.left = x;
  r.top = y;
  r.right = x + width;
  r.bottom = y + height;
  MapRect(&r);
  return r;
}

HWND CFormatOptionsDialog::AddControl(LPCWSTR className, LPCWSTR text, DWORD style, DWORD exStyle, const RECT &r, unsigned id)
{
  const HWND w = ::CreateWindowExW(exStyle, className, text, WS_CHILD | WS_VISIBLE | style,
      r.left, r.top, r.right - r.left, r.bottom - r.top,
      *this, (HMENU)(UINT_PTR)id, (HINSTANCE)GetLongPtr(GWLP_HINSTANCE), NULL);
  if (!w)
    return NULL;
  ::SendMessageW(w, WM_SETFONT, (WPARAM)_font, TRUE);
  // keep the tab order: rows first, then the OK / Cancel buttons from the template
  ::SetWindowPos(w, _lastControl ? _lastControl : HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
  _lastControl = w;
  return w;
}

bool CFormatOptionsDialog::OnInit()
{
  const CObjectVector<CFormatOption> &options = *_options;
  _font = (HFONT)SendMsg(WM_GETFONT, 0, 0);

  UStringVector tokens;
  SplitQuotedOptions(Params, tokens);

  const HWND tooltip = ::CreateWindowExW(0, TOOLTIPS_CLASSW, NULL,
      WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
      CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
      *this, NULL, (HINSTANCE)GetLongPtr(GWLP_HINSTANCE), NULL);
  if (tooltip)
    ::SendMessageW(tooltip, TTM_SETMAXTIPWIDTH, 0, 400);

  _controls.Clear();
  FOR_VECTOR (i, options)
  {
    const CFormatOption &o = options[i];
    const int y = kMargin + (int)i * kRowHeight;
    const unsigned id = kFirstId + i * 2;
    bool found;
    const UString value = GetInitialValue(o, tokens, found);
    HWND ctrl = NULL;

    if (o.Type == NFormatOptionType::kBool)
    {
      ctrl = AddControl(L"BUTTON", o.Label, BS_AUTOCHECKBOX | WS_TABSTOP, 0,
          DluRect(kMargin, y + 2, kContentWidth, 10), id);
      bool b = GetBoolDefault(o);
      if (found && !StringToBool(value, b))
        b = GetBoolDefault(o);
      CheckButton(id, b);
    }
    else
    {
      UString label = o.Label;
      label += L':';
      AddControl(L"STATIC", label, SS_LEFT | SS_NOPREFIX, 0,
          DluRect(kMargin, y + 2, kLabelWidth, 8), (unsigned)0xFFFF);

      if (o.Type == NFormatOptionType::kEnum)
      {
        ctrl = AddControl(L"COMBOBOX", NULL, CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, 0,
            DluRect(kControlX, y, kControlWidth, kComboListHeight), id);
        NControl::CComboBox combo;
        combo.Attach(ctrl);
        int sel = 0;
        FOR_VECTOR (k, o.Values)
        {
          combo.AddString(o.Labels.IsEmpty() ? o.Values[k] : o.Labels[k]);
          if (o.Values[k].IsEqualTo_NoCase(value))
            sel = (int)k;
        }
        combo.SetCurSel(sel);
      }
      else
      {
        const bool isFile = (o.Type == NFormatOptionType::kFile);
        DWORD style = ES_AUTOHSCROLL | WS_TABSTOP;
        if (o.Type == NFormatOptionType::kUInt)
          style |= ES_NUMBER;
        const int width = isFile ? kControlWidth - kBrowseWidth - kGap : kControlWidth;
        ctrl = AddControl(L"EDIT", value, style, WS_EX_CLIENTEDGE,
            DluRect(kControlX, y, width, kEditHeight), id);
        if (isFile)
          AddControl(L"BUTTON", L"...", BS_PUSHBUTTON | WS_TABSTOP, 0,
              DluRect(kControlX + width + kGap, y, kBrowseWidth, kEditHeight), id + 1);
      }
    }

    _controls.Add(ctrl);

    if (tooltip && ctrl && !o.Tip.IsEmpty())
    {
      TTTOOLINFOW ti;
      memset(&ti, 0, sizeof(ti));
      ti.cbSize = sizeof(ti);
      ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
      ti.hwnd = *this;
      ti.uId = (UINT_PTR)ctrl;
      ti.lpszText = const_cast<LPWSTR>(o.Tip.Ptr()); // the tooltip copies the text
      ::SendMessageW(tooltip, TTM_ADDTOOLW, 0, (LPARAM)&ti);
    }
  }

  // grow the dialog to fit the rows and move OK / Cancel below them
  const RECT growRect = DluRect(0, 0, 0, (int)options.Size() * kRowHeight);
  const int grow = growRect.bottom;
  {
    RECT wr;
    GetWindowRect(&wr);
    Move(wr.left, wr.top - grow / 2, wr.right - wr.left, wr.bottom - wr.top + grow);
    const unsigned kButtons[] = { IDOK, IDCANCEL };
    for (unsigned k = 0; k < Z7_ARRAY_SIZE(kButtons); k++)
    {
      RECT r;
      GetClientRectOfItem(kButtons[k], r);
      r.top += grow;
      r.bottom += grow;
      MoveItem_RECT(kButtons[k], r);
    }
  }
  NormalizePosition();

  if (!_controls.IsEmpty() && _controls[0])
  {
    SendMsg_NextDlgCtl_HWND(_controls[0]);
    return false; // focus is already set
  }
  return CModalDialog::OnInit();
}

void CFormatOptionsDialog::Browse(unsigned index)
{
  const CFormatOption &o = (*_options)[index];
  CWindow edit(_controls[index]);

  CCommonDialogInfo info;
  info.hwndOwner = *this;
  info.lpstrTitle = o.Label;
  edit.GetText(info.FilePath);

  UStringVector filters = o.Filter;
  filters.Add(UString(L"All Files"));
  filters.Add(UString(L"*.*"));

  if (info.CommonDlg_BrowseForFile(NULL, filters))
    edit.SetText(info.FilePath);
}

bool CFormatOptionsDialog::OnButtonClicked(unsigned buttonID, HWND buttonHWND)
{
  if (buttonID >= kFirstId && ((buttonID - kFirstId) & 1) != 0)
  {
    const unsigned index = (buttonID - kFirstId) / 2;
    if (index < _options->Size())
    {
      Browse(index);
      return true;
    }
  }
  return CModalDialog::OnButtonClicked(buttonID, buttonHWND);
}

void CFormatOptionsDialog::OnOK()
{
  const CObjectVector<CFormatOption> &options = *_options;

  UStringVector tokens;
  SplitQuotedOptions(Params, tokens);

  // keep tokens the schema doesn't know about, in their original order
  UString result;
  FOR_VECTOR (t, tokens)
  {
    UString name, v;
    ParseOptionToken(tokens[t], name, v);
    bool known = false;
    FOR_VECTOR (i, options)
      if (name.IsEqualTo_NoCase(options[i].Name))
        known = true;
    if (!known)
      AppendOptionToken(result, tokens[t]);
  }

  // then add options that differ from their defaults
  FOR_VECTOR (i, options)
  {
    const CFormatOption &o = options[i];
    const unsigned id = kFirstId + i * 2;
    UString value;
    bool isDefault;

    if (o.Type == NFormatOptionType::kBool)
    {
      const bool b = IsButtonCheckedBool(id);
      value = b ? L"on" : L"off";
      isDefault = (b == GetBoolDefault(o));
    }
    else if (o.Type == NFormatOptionType::kEnum)
    {
      NControl::CComboBox combo;
      combo.Attach(_controls[i]);
      const int sel = combo.GetCurSel();
      value = (sel >= 0 && (unsigned)sel < o.Values.Size()) ? o.Values[(unsigned)sel] : o.Default;
      isDefault = value.IsEqualTo_NoCase(o.Default);
    }
    else
    {
      GetItemText(id, value);
      value.Trim();
      isDefault = (value == o.Default);
    }

    if (isDefault)
      continue;
    UString token = o.Name;
    token += L'=';
    token += value;
    AppendOptionToken(result, token);
  }

  Params = result;
  CModalDialog::OnOK();
}
