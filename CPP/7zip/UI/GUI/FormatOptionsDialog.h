// FormatOptionsDialog.h

#ifndef ZIP7_INC_FORMAT_OPTIONS_DIALOG_H
#define ZIP7_INC_FORMAT_OPTIONS_DIALOG_H

#include "../../../Windows/Control/Dialog.h"

#include "../Common/FormatOptions.h"

#include "FormatOptionsDialogRes.h"

// Shows one control per option declared by a format DLL and
// merges the chosen values into the Add dialog's Parameters text.
class CFormatOptionsDialog: public NWindows::NControl::CModalDialog
{
  const CObjectVector<CFormatOption> *_options;
  CRecordVector<HWND> _controls; // one input control per option
  HWND _lastControl;
  HFONT _font;

  RECT DluRect(int x, int y, int width, int height);
  HWND AddControl(LPCWSTR className, LPCWSTR text, DWORD style, DWORD exStyle, const RECT &r, unsigned id);
  void Browse(unsigned index);

  virtual bool OnInit() Z7_override;
  virtual bool OnButtonClicked(unsigned buttonID, HWND buttonHWND) Z7_override;
  virtual void OnOK() Z7_override;

public:
  UString Params; // [in, out] Parameters text

  INT_PTR Create(HWND wndParent = NULL)
  {
    return CModalDialog::Create(IDD_FORMAT_OPTIONS, wndParent);
  }

  CFormatOptionsDialog(const CObjectVector<CFormatOption> *options):
      _options(options),
      _lastControl(NULL),
      _font(NULL)
      {}
};

#endif
