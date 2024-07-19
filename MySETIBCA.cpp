//
// MySETIBCA, an application for decoding, encoding message images using 
// a block cellular automata like what was used in the 'A Sign inSpace' project message
// 
// MySETIBCA.cpp
// (C) 2023, Mark Stegall
// Author: Mark Stegall
//
// The MySETIviewer app as the initial starting skeleton for the MySETIBCA app
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
// Background:
// From the 'A Sign in Space' website, https://asignin.space/:
// A Sign in Space is an interdisciplinary project by media artist Daniela de Paulis,
// in collaboration with the SETI Institute, the European Space Agency,
// the Green Bank Observatory and INAF, the Italian National Institute for Astrophysics.
// The project consists in transmitting a simulated extraterrestrial message as part
// of a live performance, using an ESA spacecraft as celestial source.The objective
// of the project is to involve the world - wide Search for Extraterrestrial
// Intelligence community, professionals from different fieldsand the broader public
// in the reception, decodingand interpretation of the message.This process will
// require global cooperation, bridging a conversation around the topics of SETI,
// space researchand society, across multiple culturesand fields of expertise.
// https://www.seti.org/event/sign-space
// 
// The message was transmitted from the ESA's ExoMars Trace Gas Orbiter (TGO)
//   on May 24 at 19:16 UTC/12:15 pm PDT.
// 
// It was received by three radio telescopes on earth May 24,2023.
// 
// A group of individuals in the Discord 'A Sign in Space' channel
// unscrambled the message from the radio telemetry.
// The message published as Data17.bin is the correctly transcribed
// bitstream of the message payload given to ESA.
// 
// The next step in the problem is the decoding of the payload bitstream into
// the next level of the message.  A header, 256x256 bit 'starmap', footer were decoded shortly.
// 
// The starmap required further decoding.  A year later it was successfully decoded by
// John the Decoder (Discord user name) into a 256x256 5 amino acid map and confirmed
// by the 'A Sign in Space' group as the correct decode of the starmap.
// This involved using a 2D Margolus block cellular automata with a CCW transition ruleset
// and a x+1,y+1 translation rule.
// The image was obtained by running the BCA on the starmap for 6625 iterations.
// The number 6625 can be found in the footer as 5*5*5*53 which is translated from the 5x1s,5x0s
// 5x1s,53x0s in the footer.  The CCW rule is found in the header.
// 
// MySETIBCA application
// This application can be used to run a BCA on an image backwards or forwards,
// display and save the results.
// 
// It also allows for image files to be overlayed onto the BCA image for comparison.
// This includes changing the alignment of a file image to the BCA image.
// This also includes is a Major/Minor grid display so that the blocks in the image can
// be easily displayed.  The BCA image and other files images are loaded as layers that
// can be assigned colors and enabled or disabled.
// 
// The MySETI programs is a set of tools to help in this process.
// This conssists of:
//      MySETIapp    Addresses various messgae extraction and image manipultation
//                   methods the have been tried or are being tried for the purpose
//                   of further decoding the 'A Sign In Space' message
//      MySETIviewer Viewer program to load the image files (.raw, .bmp(8 bit)) that the MySETIapp   
//                   application generates.  It allows the overlay, alignment, and
//                   display of theses file with an interactrive GUI.
//      MySETIBCA    Block cellular automata program for decoding/encoding messages for the A Sign In Space
//                   project
//
// 
// V1.0.0	2024-06-21	Initial release
// V1.0.1   2024-06-23  Fixed code error for return of type void instead of BOOL
//                        when calling MargolusBCAp1p1()
//                      Fixed installer conifiguration that kept 'repairing' the .cfg and .ini when 
//                        the shortcut were used
//                      Changed BCA layer raw file input of color based raw image(3 frames)
//                        to single single binary frame
// V1.1.0   2024-06-28  Added ASIS receive/send message.
//                          Recieve reads the ASIS .bin bitstream file and saves a decoded
//                          .raw image file and a .bmp file
//                          Send will encode a .raw image file or .bmp file that is 256x256
//                          and save it as a ASIS bitstream file.  (pixels that have a 
//                          monochromatic or chromatic brightness <50 are considered 0,
//                          >= 50 are considered 1)
//                      Added even/odd controls to Margolus BCA, this shows wether the next
//                          iteration will be an even (0,0) or odd grid (1,1).  Also allows
//                          user to change the current selelction.  Note:  ASIS uses an even
//                          starting grid.
//                      Allow also the use of .bmp files instead of .raw files for the BCA input
//                      Automatically save of BMP and (optionally) PNG file when image file is saved
//                      Corrected handling of BMP files
//                      Corrected potential memory leak on WM_DESTROY
// V1.1.2   2024-07-10  Changed default size of display to be 256x256, save/restore user settings
//                      Added histogram of # bits set in 2x2 block for image
//                      Added current non zero bit count to BCA dialog
//                      Send ASIS message dialog saves the number of iterations as default for next time
//                      Allow override of iterations specfied in the Receive ASIS message
//                      Allow generation of footer based on # iterations selected rather than use a file
//                      Correction, Enable Grid should have initialized to disabled.
//                      Correction, filename extension  for send message corrected to default of .bin
//                      Correction, removed bit order flag from bitstream conversion, we already know ASIS bit order
//                      Correction, default filename for footer was the header file
//                      Correction, Receive ASIS message starting EvenStep = FALSE when # of iterations is even
//                      Correction, Enable Grid had inconsistencies
// V1.1.3   20204-07-18 Correction, Allow 0 iterations in ASIS message both receive and send
// 
//  This appliction stores user parameters in a Windows style .ini file
//  The MySETIBCA.ini file must be in the same directory as the exectable
//
//  A lot of this application code is really to support the framework
//  of the application for the user interface.
// 
//  The application code that does the image overlays and formatting
//  are implemented in the Display class and the Layers class:
//  Display.h,Display.cpp, Layers.h, Layers.cpp
// 
// The application code that does the actual display are implemented in:
// ImageDialog.cpp, ImageDlg.cpp
// 
// The actual block cellular automata user imterface is implemented in:
// CAdialogs.cpp
// 
// The actual block cellular automata user imterface is implemented in:
// CAdialogs.cpp
// 
// The processing algorithms for the block cellular automata is implement in:
// CA.cpp
// 
// The first layer in the image is alsways the BCA image layer and can not be deleted
// If the BCA image is not loaded then the first layer will display in the layers list
// as BCA image not loaded.
// 
#include "framework.h"
#include <atlstr.h>
#include <strsafe.h>
#include <atlpath.h>
#include <string.h>
#include "AppErrors.h"
#include "MySETIBCA.h"
#include "Layers.h"
#include "Display.h"
#include "ImageDialog.h"
#include "AppFunctions.h"
#include "imageheader.h"
#include "FileFunctions.h"
#include "CA.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// working filenames
WCHAR szBMPFilename[MAX_PATH] = L"";    // used to save last results file
WCHAR szCurrentFilename[MAX_PATH] = L"";    // used to save last results file
WCHAR szTempDir[MAX_PATH] = L"";        // used as a temporary storage location

// global Class declarations
Layers* ImageLayers = NULL;     // This class loads the image layers, combines them into the Overlay image
Display* Displays = NULL;       // This class creates a refernce image based on the display format paraneters
                                // and then inserts the Overlay image to create the Dislay image
ImageDialog* ImgDlg = NULL;     // This class is used to support displaying the Display image in a window
                                // on the desktop.  This also includes scaling and panning of the displayed image

// global flags
BOOL AutoPNG = FALSE;                // generate a PNG file when a BMP file is saved
BOOL KeepOpen = TRUE;           // keep Display and Layers dialog open when clicking OK or Cancel
BOOL ShowStatusBar = TRUE;     // Display Status bar

                               // handles for modeless dialogs and windows
HWND hwndImage = NULL;   // Handle for modeless Image Dialog window (this displays the image in a window)
HWND hwndDisplay = NULL; // Handle for the Display dialog (sets the grid and gap, and
                         //    creates the Display image used in the Image Dialog
HWND hwndLayers = NULL;  // Handle for the Layers dialog, this creates the Overlayed image from
                         //    the specified layer, colors amd position,  the overlay image is
                         //    then used in the Display dialog
HWND hwndMargolusBCA = NULL;  // Handle for the MargolusBCA dialog

// used by the color picker dialog custom color list
COLORREF CustomColorTable[16] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };    // array of custom colors

// The following is from the version information in the resource file
CString strProductName;
CString strProductVersion;
CString strName;
CString strCopyright;
CString strAppNameEXE;
CString strAppNameINI;

// handle to the main application window
//  needed to do things like check or uncheck a menu item in the main app
HWND hwndMain = NULL;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

// Declaration for callback dialog procedures in other modules
INT_PTR CALLBACK    AboutDlg(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    SettingsDisplayDlg(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    SettingsLayersDlg(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    SettingsGlobalDlg(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    ImageDlg(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    Text2StreamDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK    BitImageDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK    MargolusBCADlg(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    ReceiveASISdlg(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    SendASISdlg(HWND, UINT, WPARAM, LPARAM);

//*******************************************************************************
//
// int APIENTRY wWinMain
//
//*******************************************************************************
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);

    // 
    // Get version information, executable name including path to executable location
    if (!GetProductAndVersion(&strProductName,
        &strProductVersion,
        &strName,
        &strCopyright,
        &strAppNameEXE))
        return FALSE;

    // create INI file name for application settings
    // The INI file has the same name as the executable.
    // It is located in the same directory as the exectable
    CPath path(strAppNameEXE);
    path.RenameExtension(_T(".ini"));
    strAppNameINI.SetString(path);

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_MYSETIBCA, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MYSETIBCA));

    MSG msg;
    BOOL bRet;

    // Main message loop:
    while ((bRet = GetMessage(&msg, nullptr, 0, 0))!=0)
    {
        if (bRet == -1) {
            // error handling if required
        }
        else {
            // Modeless dialog need to process messages intended for the modeless dialog only.
            // Check that message is not for each modeless dialog box.
            
            // Modeless Image Dialog window
            if (IsWindow(hwndImage) && IsDialogMessage(hwndImage, &msg)) {
                continue;
            }

            // Modeless Display window
            if (IsWindow(hwndDisplay) && IsDialogMessage(hwndDisplay, &msg)) {
                continue;
            }

            // Modeless Layer window
            if (IsWindow(hwndLayers) && IsDialogMessage(hwndLayers, &msg)) {
                continue;
            }

            // Modeless Margolus BCA window
            if (IsWindow(hwndMargolusBCA) && IsDialogMessage(hwndMargolusBCA, &msg)) {
                continue;
            }

            // process all other applications messages
            if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }

    return (int) msg.wParam;
}


//*******************************************************************************
//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.  This is not the same as a c++ class
//
//*******************************************************************************
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MYSETIBCA));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_MYSETIBCA);
    wcex.lpszClassName  = szWindowClass;  // loaded from the resource file IDC_MYSETIBCA string
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_MYSETIBCA));

    return RegisterClassExW(&wcex);
}

//*******************************************************************************
//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
//*******************************************************************************
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   // Just using default size and position since this will be overwritten
   // when restorewindow is called
   HWND hWnd = CreateWindowW(szWindowClass, szTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, // initial x,y position
        CW_USEDEFAULT, 0, // initial x,ysize
        nullptr, nullptr,
        hInstance, nullptr);

   if (!hWnd)
   {
       // if can not create window just die
      return FALSE;
   }

   int ResetWindows = GetPrivateProfileInt(L"GlobalSettings", L"ResetWindows", 0, (LPCTSTR)strAppNameINI);

   // restore main window position from last execution
   if (!ResetWindows) {
       CString csString = L"MainWindow";
       RestoreWindowPlacement(hWnd, csString);
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   // load globals

   // this must be done before ImageDialog class created
   ShowStatusBar = GetPrivateProfileInt(L"SettingsGlobalDlg", L"ShowStatusBar", 1, (LPCTSTR)strAppNameINI);

   hwndMain = hWnd;
   ImgDlg = new ImageDialog;
   ImageLayers = new Layers;
   Displays = new Display;

   // create display window
   hwndImage = CreateDialog(hInst, MAKEINTRESOURCE(IDD_IMAGE), hwndMain, ImageDlg);

   // strings
   WCHAR szString[MAX_PATH];

   GetPrivateProfileString(L"SettingsGlobalDlg", L"TempDir", L"", szString, MAX_PATH, (LPCTSTR)strAppNameINI);
   wcscpy_s(szTempDir, szString);

   GetPrivateProfileString(L"SettingsGlobalDlg", L"CurrentFIlename", L"", szString, MAX_PATH, (LPCTSTR)strAppNameINI);
   wcscpy_s(szCurrentFilename, szString);

   // variables
   AutoPNG = GetPrivateProfileInt(L"SettingsGlobalDlg", L"AutoPNG", 1, (LPCTSTR)strAppNameINI);
   KeepOpen = GetPrivateProfileInt(L"SettingsGlobalDlg", L"KeepOpen", 0, (LPCTSTR)strAppNameINI);
   int ydir = GetPrivateProfileInt(L"SettingsGlobalDlg", L"yposDir", 1, (LPCTSTR)strAppNameINI);
   ImageLayers->SetYdir(ydir);

   for (int i = 0; i < 16; i++) {
       WCHAR CustomColor[20];
       swprintf_s(CustomColor, 20, L"CustomColorTable%d", i);
       CustomColorTable[i] = (COLORREF) GetPrivateProfileInt(L"SettingsDlg", CustomColor, 0, (LPCTSTR)strAppNameINI);
   }

   // These are Display class settings
   COLORREF BackGroundColor = (COLORREF)GetPrivateProfileInt(L"SettingsDisplayDlg", L"rgbBackground", 131586, (LPCTSTR)strAppNameINI);
   COLORREF GapMajorColor = (COLORREF)GetPrivateProfileInt(L"SettingsDisplayDlg", L"rgbGapMajor", 2621521, (LPCTSTR)strAppNameINI);
   COLORREF GapMinorColor = (COLORREF)GetPrivateProfileInt(L"SettingsDisplayDlg", L"rgbGapMinor", 1973790, (LPCTSTR)strAppNameINI);
   Displays->SetColors(BackGroundColor, GapMajorColor, GapMinorColor);

   int ValueX, ValueY;
   ValueX = GetPrivateProfileInt(L"SettingsDisplayDlg", L"GridXmajor", 8, (LPCTSTR)strAppNameINI);
   ValueY = GetPrivateProfileInt(L"SettingsDisplayDlg", L"GridYmajor", 8, (LPCTSTR)strAppNameINI);
   Displays->SetGridMajor(ValueX, ValueY);

   ValueX = GetPrivateProfileInt(L"SettingsDisplayDlg", L"GridXminor", 4, (LPCTSTR)strAppNameINI);
   ValueY = GetPrivateProfileInt(L"SettingsDisplayDlg", L"GridYminor", 4, (LPCTSTR)strAppNameINI);
   Displays->SetGridMinor(ValueX, ValueY);

   ValueX = GetPrivateProfileInt(L"SettingsDisplayDlg", L"GapXmajor", 1, (LPCTSTR)strAppNameINI);
   ValueY = GetPrivateProfileInt(L"SettingsDisplayDlg", L"GapYmajor", 1, (LPCTSTR)strAppNameINI);
   Displays->SetGapMajor(ValueX, ValueY);

   ValueX = GetPrivateProfileInt(L"SettingsDisplayDlg", L"GapXminor", 1, (LPCTSTR)strAppNameINI);
   ValueY = GetPrivateProfileInt(L"SettingsDisplayDlg", L"GapYminor", 1, (LPCTSTR)strAppNameINI);
   Displays->SetGapMinor(ValueX, ValueY);

   int Enable = GetPrivateProfileInt(L"SettingsDisplayDlg", L"EnableGrid", 0, (LPCTSTR)strAppNameINI);
   Displays->EnableGrid(Enable);

   // These are Layer class settings
   COLORREF Color;
   // This is the color of the overlay background anywhere it hasn't been replaced by image data
   Color = (COLORREF) GetPrivateProfileInt(L"LayersDlg", L"BackGround", 2763306, (LPCTSTR)strAppNameINI);
   ImageLayers->SetBackgroundColor(Color);
   Color = (COLORREF)GetPrivateProfileInt(L"LayersDlg", L"OverlayColor", 65793, (LPCTSTR)strAppNameINI);
   ImageLayers->SetOverlayColor(Color);
   Color = (COLORREF)GetPrivateProfileInt(L"LayersDlg", L"DefaultLayerColor", 16777215, (LPCTSTR)strAppNameINI);
   ImageLayers->SetDefaultLayerColor(Color);

   // restore default MinOverlaySize
   int x, y;
   x = GetPrivateProfileInt(L"SettingsDlg", L"MinOverlaySizeX", 256, (LPCTSTR)strAppNameINI);
   y = GetPrivateProfileInt(L"SettingsDlg", L"MinOverlaySizeY", 256, (LPCTSTR)strAppNameINI);
   ImageLayers->SetMinOverlaySize(x, y);

   // add reserved layer 0;
   ImageLayers->AddLayer(NULL);

   hwndDisplay = CreateDialog(hInst, MAKEINTRESOURCE(IDD_SETTINGS_DISPLAY), hWnd, SettingsDisplayDlg);
   // Initially hide display dialog
   ShowWindow(hwndDisplay, SW_HIDE);

   if (GetPrivateProfileInt(L"SettingsGlobalDlg", L"StartLast", 0, (LPCTSTR)strAppNameINI) != 0) {
       // load the last layer cinfiguration
       WCHAR szString[MAX_PATH];

       GetPrivateProfileString(L"GlobalSettings", L"LastConfigFile", L"", szString, MAX_PATH, (LPCTSTR)strAppNameINI);
       //wcscpy_s(ImageLayers->ConfigurationFile, MAX_PATH, szString);
       int iRes = ImageLayers->LoadConfiguration(szString);
       if (iRes != APP_SUCCESS) {
           wcscpy_s(ImageLayers->ConfigurationFile, MAX_PATH, L"");
       }
   }
   else {
       wcscpy_s(ImageLayers->ConfigurationFile, MAX_PATH, L"");
   }

   hwndLayers = CreateDialog(hInst, MAKEINTRESOURCE(IDD_SETTINGS_LAYERS), hWnd, SettingsLayersDlg);
   ShowWindow(hwndLayers, SW_HIDE);

   hwndMargolusBCA = CreateDialog(hInst, MAKEINTRESOURCE(IDD_CA_MBCA), hWnd, MargolusBCADlg);

   // clear windows reset
   WritePrivateProfileString(L"GlobalSettings", L"ResetWindows", L"0", (LPCTSTR)strAppNameINI);
   return TRUE;
}

//*******************************************************************************
//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
//*******************************************************************************
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        // Parse the menu selections:
        switch (wmId)
        {

// File menu

        case IDM_LOAD_CONFIGURATION:
        {
            int iRes;
            WCHAR szFilename[MAX_PATH];

            PWSTR pszFilename;
            COMDLG_FILTERSPEC AllType[] =
            {
                 { L"Image files", L"*.cfg" },
                 { L"All Files", L"*.*" },
            };

            if (!CCFileOpen(hWnd, ImageLayers->ConfigurationFile, &pszFilename, FALSE, 2, AllType, L".cfg")) {
                break;
            }

            wcscpy_s(szFilename, pszFilename);
            CoTaskMemFree(pszFilename);

            iRes = ImageLayers->LoadConfiguration(szFilename);
            if (iRes != APP_SUCCESS) {
                MessageBox(hWnd, L"Load configuration file failed\nCheck file for correct filenames in file", L"Layers", MB_OK);
                break;
            }
            Displays->LoadConfiguration(szFilename);
            SendMessage(hwndDisplay, WM_COMMAND, ID_UPDATE,0); // do not apply
            SendMessage(hwndLayers, WM_COMMAND, ID_UPDATE, 1); // apply 
            
            break;
        }

        case IDM_SAVE_CONFIGURATION:
        {
            int iRes;

            if (ImageLayers->GetNumLayers() == 0) {
                MessageBox(hWnd, L"No layers loaded", L"Layers", MB_OK);
                break;
            }

            if (wcslen(ImageLayers->ConfigurationFile) != 0) {
                iRes = ImageLayers->SaveConfiguration();
                iRes = Displays->SaveConfiguration(ImageLayers->ConfigurationFile);
                if (iRes == APP_SUCCESS) {
                    break;
                }
            }
            // go ahead with IDM_SAVE_AS_CONFIGURATION
        }
        case IDM_SAVE_AS_CONFIGURATION:
        {
            int iRes;

            if (ImageLayers->GetNumLayers() == 0) {
                MessageBox(hWnd, L"No layers loaded", L"Layers", MB_OK);
                break;
            }

            WCHAR szFilename[MAX_PATH];

            PWSTR pszFilename;
            COMDLG_FILTERSPEC AllType[] =
            {
                 { L"Image files", L"*.cfg" },
                 { L"All Files", L"*.*" },
            };

            if (!CCFileSave(hWnd, ImageLayers->ConfigurationFile, &pszFilename, FALSE, 2, AllType, L".cfg")) {
                break;
            }

            wcscpy_s(szFilename, pszFilename);
            CoTaskMemFree(pszFilename);

            iRes = ImageLayers->SaveConfiguration(szFilename);
            if (iRes == APP_SUCCESS) {
                wcscpy_s(ImageLayers->ConfigurationFile, MAX_PATH, szFilename);
            }
            iRes = Displays->SaveConfiguration(szFilename);
            break;
        }

        case IDM_LAYER_SAVEBMP:
        {
            if (ImageLayers->GetNumLayers() == 0) {
                MessageBox(hWnd, L"No layers, nothing to save", L"Layers", MB_OK); 
                break;
            }

            if (!ImageLayers->OverlayValid) {
                MessageBox(hWnd, L"Overlay is not valid to save\nmake sure to 'Apply' current layers configuration", L"Layers", MB_OK);
                break;
            }

            PWSTR pszFilename;
            COMDLG_FILTERSPEC AllType[] =
            {
                 { L"BMP files", L"*.bmp" },
                 { L"All Files", L"*.*" }
            };

            if (!CCFileSave(hWnd, szCurrentFilename, &pszFilename, FALSE, 2, AllType, L".bmp")) {
                break;
            }

            wcscpy_s(szCurrentFilename, pszFilename);
            CoTaskMemFree(pszFilename);

            int iRes;
            iRes = ImageLayers->SaveBMP(szCurrentFilename);
            if (iRes != APP_SUCCESS) {
                MessageBox(hWnd, L"Save Failed", L"Layers", MB_OK);
            }
            break;
        }

        case IDM_DISPLAY_SAVEBMP:
        {
            if (ImageLayers->GetNumLayers() == 0) {
                MessageBox(hWnd, L"No layers, nothing to save", L"Display", MB_OK);
                break;
            }

            PWSTR pszFilename;
            COMDLG_FILTERSPEC AllType[] =
            {
                 { L"BMP files", L"*.bmp" },
                 { L"All Files", L"*.*" }
            };

            if (!CCFileSave(hWnd, szCurrentFilename, &pszFilename, FALSE, 2, AllType, L".bmp")) {
                break;
            }

            wcscpy_s(szCurrentFilename, pszFilename);
            CoTaskMemFree(pszFilename);

            int iRes;
            iRes = Displays->SaveBMP(szCurrentFilename, 0);
            if (iRes != APP_SUCCESS) {
                MessageBox(hWnd, L"Save Failed", L"Layers", MB_OK);
            }
            break;
        }

        case IDM_REFERENCE_SAVEBMP:
        {
            PWSTR pszFilename;
            COMDLG_FILTERSPEC AllType[] =
            {
                 { L"BMP files", L"*.bmp" },
                 { L"All Files", L"*.*" }
            };

            if (!CCFileSave(hWnd, szCurrentFilename, &pszFilename, FALSE, 2, AllType, L".bmp")) {
                break;
            }

            wcscpy_s(szCurrentFilename, pszFilename);
            CoTaskMemFree(pszFilename);

            int iRes;
            iRes = Displays->SaveBMP(szCurrentFilename, 1);
            if (iRes != APP_SUCCESS) {
                MessageBox(hWnd, L"Save Failed", L"Layers", MB_OK);
            }
            break;
        }

        case IDM_BITTOOLS_TEXT2BITSTREAM:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_BITTOOLS_TEXT2STREAM), hWnd, Text2StreamDlg);
            break;

        case IDM_BITTOOLS_BINARYIMAGE:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_BITTOOLS_BINARYIMAGE), hWnd, BitImageDlg);
            break;

        case IDM_EXIT:
        {
            if (hwndImage) {
                // save window position/size data
                CString csString = L"ImageWindow";
                SaveWindowPlacement(hwndImage, csString);
                // this must be done to ensure Direct2D factory is released
                DestroyWindow(hwndImage);
            }
            DestroyWindow(hWnd);
            break;
        }

// Cellular Automata menu

        case IDM_CA_MBCA:
            if (hwndMargolusBCA) {
                ShowWindow(hwndMargolusBCA, SW_SHOW);
            }

            break;

// ASIS menu 
        case IDM_RECEIVE_ASIS:
        {
            DialogBox(hInst, MAKEINTRESOURCE(IDD_RECEIVE_ASIS), hWnd, ReceiveASISdlg);
            break;
        }

        case IDM_SEND_ASIS:
        {
            DialogBox(hInst, MAKEINTRESOURCE(IDD_SEND_ASIS), hWnd, SendASISdlg);
            break;
        }

// Settings menu

        case IDM_SETTINGS_GLOBAL:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_SETTINGS_GLOBAL), hWnd, SettingsGlobalDlg);
            break;

        case IDM_SETTINGS_DISPLAY:
        {
            if (wcslen(szTempDir) == 0) {
                MessageBox(hWnd, L"Working file folder needs to be set\nin Settings->Global first", L"Layers", MB_OK);
                break;
            }
            if (hwndDisplay) {
                ShowWindow(hwndDisplay, SW_SHOW);
            }
            
            break;
        }

        case IDM_SETTINGS_LAYERS:
        {
            if (wcslen(szTempDir) == 0) {
                MessageBox(hWnd, L"Working file folder needs to be set\nin Settings->Global first", L"Layers", MB_OK);
                break;
            }
            if (hwndLayers) {
                ShowWindow(hwndLayers, SW_SHOW);
            }

            //DialogBox(hInst, MAKEINTRESOURCE(IDD_SETTINGS_LAYERS), hWnd, SettingsLayersDlg);
            break;
        }

        case IDM_SETTINGS_RESET_WINDOWS:
            WritePrivateProfileString(L"GlobalSettings", L"ResetWindows", L"1", (LPCTSTR)strAppNameINI);
            break;

// Help menu

        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, AboutDlg);
            break;


        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }

        break;
    } // This is the end of WM_COMMAND
  
    case WM_TIMER:
        if (BCArunning == 0) {
            KillTimer(hWnd, IDT_BCA_RUN_TIMER);
        }
        if (BCArunning == 1) {
            PostMessage(hwndMargolusBCA, WM_COMMAND, IDC_STEP_FORWARD, 0);
        }
        else {
            PostMessage(hwndMargolusBCA, WM_COMMAND, IDC_STEP_BACKWARD, 0);
        }
        return 0;

    case WM_CLOSE:
    {
        if (hwndImage) {
            // save window position/size data
            CString csString = L"ImageWindow";
            SaveWindowPlacement(hwndImage, csString);
            // this must be done to ensure Direct2D factory is released
            DestroyWindow(hwndImage);
        }
        if (hwndDisplay) {
            // save window position/size data for the Image Dialog window
            CString csString = L"ImageDisplay";
            SaveWindowPlacement(hwndDisplay, csString);
            DestroyWindow(hwndDisplay);
        }
        if (hwndLayers) {
            // save window position/size data for the Image Dialog window
            CString csString = L"LayerWindow";
            SaveWindowPlacement(hwndLayers, csString);
            DestroyWindow(hwndLayers);
        }
        if (hwndMargolusBCA) {
            // save window position/size data for the Margolus BCA Dialog window
            CString csString = L"MargolusBCAwindow";
            SaveWindowPlacement(hwndMargolusBCA, csString);
            DestroyWindow(hwndMargolusBCA);
        }

        DestroyWindow(hWnd);
        break;
    }

    case WM_DESTROY:
    {   
        WritePrivateProfileString(L"GlobalSettings", L"CurrentFilename", szCurrentFilename, (LPCTSTR)strAppNameINI);
        WritePrivateProfileString(L"GlobalSettings", L"LastConfigFile", ImageLayers->ConfigurationFile, (LPCTSTR)strAppNameINI);
        // save window position/size data for the Main window
        CString csString = L"MainWindow";
        SaveWindowPlacement(hWnd, csString);

        // kill run BCA timer if still running
        KillTimer(hwndMain, IDT_BCA_RUN_TIMER);

        // release Image Dialog
        if (hwndImage) {
            // save window position/size data for the Image Dialog window
            CString csString = L"ImageWindow";
            SaveWindowPlacement(hwndImage, csString);
            DestroyWindow(hwndImage);
        }

        if (hwndDisplay) {
            // save window position/size data for the Image Dialog window
            CString csString = L"ImageDisplay";
            SaveWindowPlacement(hwndDisplay, csString);
            DestroyWindow(hwndDisplay);
        }

        if (hwndLayers) {
            // save window position/size data for the Image Dialog window
            CString csString = L"LayerWindow";
            SaveWindowPlacement(hwndDisplay, csString);
            DestroyWindow(hwndLayers);
        }

        if (hwndMargolusBCA) {
            // save window position/size data for the Margolus BCA Dialog window
            CString csString = L"MargolusBCAwindow";
            SaveWindowPlacement(hwndMargolusBCA, csString);
            DestroyWindow(hwndMargolusBCA);
        }

        // delete the global classes;
        if (ImageLayers != NULL) delete ImageLayers;
        if (Displays != NULL) delete Displays;
        if (ImgDlg != NULL) delete ImgDlg;

        PostQuitMessage(0);
        break;
    }

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);

    }
    return 0;
}

