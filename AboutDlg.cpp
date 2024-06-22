//
// MySETIBCA, an application for decoding, encoding message images using 
// a block cellular automata like what was used in the 'A Sign inSpace' project message
// 
// AboutDlg.cpp
// (C) 2023, Mark Stegall
// Author: Mark Stegall
//
// This file is part of MySETIBCA.
//
// MySETIBCA is free software : you can redistribute it and /or modify it under
// the terms of the GNU General Public License as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.
//
// MySETIBCA is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
// without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
// You should have received a copy of the GNU General Public License along with MySETIBCA.
// If not, see < https://www.gnu.org/licenses/>. 
// 
// V1.0.0	2024-06-21	Initial release
//
// About box handler
//
//  This module is a copy of the AboutDlg module used in MySETIviewer and customized
//  for this application

#include "framework.h"
#include "MySETIBCA.h"
#include <libloaderapi.h>
#include <Windows.h>
#include <shobjidl.h>
#include <winver.h>
#include <vector>
#include <atlstr.h>
#include <strsafe.h>
#include "Globals.h"
#include "AppFunctions.h"
#include "imageheader.h"
#include "FileFunctions.h"

// Message handler for about box.
INT_PTR CALLBACK AboutDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        if (GetProductAndVersion(&strProductName,
            &strProductVersion,
            &strName,
            &strCopyright,
            &strAppNameEXE))
        {
            const WCHAR* pszProductName = (const WCHAR*)strProductName;
            const WCHAR* pszProductVersion = (const WCHAR*)strProductVersion;
            const WCHAR* pszName = (const WCHAR*)strName;
            const WCHAR* pszCopyright = (const WCHAR*)strCopyright;

            SetDlgItemText(hDlg, IDC_ABOUT_NAME, pszProductName);
            SetDlgItemText(hDlg, IDC_ABOUT_VERSION, pszProductVersion);
            SetDlgItemText(hDlg, IDC_ABOUT_LICENSE, pszName);
            SetDlgItemText(hDlg, IDC_ABOUT_COPYRIGHT, pszCopyright);
        }
        else
        {
            SetDlgItemText(hDlg, IDC_ABOUT_NAME, (LPCWSTR)L"MySETIBCA");
            SetDlgItemText(hDlg, IDC_ABOUT_VERSION, (LPCWSTR)L"1.1.0.1");
            SetDlgItemText(hDlg, IDC_ABOUT_AUTHOR, (LPCWSTR)L"GNU GPL V3.0 or later");
            SetDlgItemText(hDlg, IDC_ABOUT_COPYRIGHT, (LPCWSTR)L"(C) 2023, Mark Stegall");
        }

        SetDlgItemText(hDlg, IDC_INI_FILE, strAppNameINI);
        SetDlgItemText(hDlg, IDC_EXE_FILE, strAppNameEXE);

        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
