#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../mat2png/palette.hpp"
#include "../mat2png/parser.hpp"
#include "../mat2png/rasterizer.hpp"
#include "../img2mat/converter.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>

#include <string>
#include <vector>
#include <queue>
#include <unordered_map>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <algorithm>

// Enable modern visual styles
#pragma comment(linker, "\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

namespace {

enum class ActiveTab {
    MatrixToPng = 0,
    PngToPixelArt = 1
};

// UI Control IDs
constexpr int ID_TAB_MAT2PNG      = 1000;
constexpr int ID_TAB_PNG2MAT      = 1001;

// Tab 1: Matriz -> PNG
constexpr int ID_BTN_PASTE        = 1002;
constexpr int ID_BTN_FIX_MATRIX   = 1003;
constexpr int ID_BTN_CROP_TAB1    = 1004;
constexpr int ID_BTN_REMBG_TAB1   = 1005;
constexpr int ID_BTN_CLEAR        = 1006;
constexpr int ID_BTN_SAMPLE32     = 1007;
constexpr int ID_BTN_SAMPLE16     = 1008;
constexpr int ID_EDIT_MATRIX      = 1009;

// Tab 2: PNG -> Pixel Art
constexpr int ID_BTN_TAB2_IMPORT  = 1010;
constexpr int ID_BTN_RES16        = 1011;
constexpr int ID_BTN_RES32        = 1012;
constexpr int ID_BTN_RES64        = 1013;
constexpr int ID_CHK_TAB2_CROP    = 1014;
constexpr int ID_BTN_TAB2_CROP    = 1015;
constexpr int ID_BTN_TAB2_REMBG   = 1016;
constexpr int ID_BTN_TAB2_COPYMAT = 1017;
constexpr int ID_BTN_TAB2_TO_EDIT = 1018;

// Shared Canvas Controls
constexpr int ID_CANVAS           = 1030;
constexpr int ID_COMBO_SCALE      = 1031;
constexpr int ID_CHK_GRID         = 1032;
constexpr int ID_BTN_EXPORT_HD    = 1033;
constexpr int ID_BTN_EXPORT_1X    = 1034;
constexpr int ID_BTN_COPY_IMG     = 1035;
constexpr int ID_PALETTE_BAR      = 1036;

// Dark Theme Colors
const COLORREF COLOR_BG_MAIN     = RGB(22, 24, 34);     // Deep Slate
const COLORREF COLOR_BG_PANEL    = RGB(30, 33, 46);     // Dark Card
const COLORREF COLOR_BG_EDIT     = RGB(15, 16, 23);     // Editor Dark
const COLORREF COLOR_TEXT_EDIT   = RGB(225, 230, 242);  // Soft White
const COLORREF COLOR_ACCENT_BLUE = RGB(99, 155, 255);   // Arcane Blue
const COLORREF COLOR_SUCCESS     = RGB(153, 229, 80);   // Lime Green
const COLORREF COLOR_WARNING     = RGB(247, 118, 34);   // Flame Orange
const COLORREF COLOR_ERROR       = RGB(217, 87, 99);    // Coral Red
const COLORREF COLOR_TEXT_MUTED  = RGB(132, 126, 135);  // Stone Grey

// Active Tab State
ActiveTab g_activeTab = ActiveTab::MatrixToPng;

// Global Windows & Handles
HWND g_hWndMain          = nullptr;

// Tab Selector Buttons
HWND g_hBtnTabMat2Png    = nullptr;
HWND g_hBtnTabPng2Mat    = nullptr;

// Tab 1 Controls
HWND g_hBtnPaste         = nullptr;
HWND g_hBtnFix           = nullptr;
HWND g_hBtnCropTab1      = nullptr;
HWND g_hBtnRemBg1        = nullptr;
HWND g_hBtnClear         = nullptr;
HWND g_hBtnSample32      = nullptr;
HWND g_hBtnSample16      = nullptr;
HWND g_hEditMatrix       = nullptr;
HWND g_hStatusMatrix     = nullptr;

// Tab 2 Controls
HWND g_hTab2BtnImport    = nullptr;
HWND g_hTab2LblFile      = nullptr;
HWND g_hTab2LblResHeader = nullptr;
HWND g_hTab2BtnRes16     = nullptr;
HWND g_hTab2BtnRes32     = nullptr;
HWND g_hTab2BtnRes64     = nullptr;
HWND g_hTab2ChkCrop      = nullptr;
HWND g_hTab2BtnCrop      = nullptr;
HWND g_hTab2BtnRemBg     = nullptr;
HWND g_hTab2LblBgTip     = nullptr;
HWND g_hTab2LblStats     = nullptr;
HWND g_hTab2BtnCopyMat   = nullptr;
HWND g_hTab2BtnToEditor  = nullptr;

// Shared Canvas Controls
HWND g_hCanvas           = nullptr;
HWND g_hStatusPixel      = nullptr;
HWND g_hComboScale       = nullptr;
HWND g_hChkGrid          = nullptr;
HWND g_hPaletteBar       = nullptr;

HBRUSH g_hBrushMain      = nullptr;
HBRUSH g_hBrushPanel     = nullptr;
HBRUSH g_hBrushEdit      = nullptr;
HFONT  g_hFontMono       = nullptr;
HFONT  g_hFontUI         = nullptr;
HFONT  g_hFontUIBold     = nullptr;
HFONT  g_hFontSmall      = nullptr;

// State Variables
spritex::tools::Matrix g_matrix;
bool g_matrixValid = false;
bool g_matrixWasRepaired = false;
std::string g_matrixWarning;
std::string g_parseError;
int g_currentScale = 16;
bool g_showGrid = false;
int g_hoverPixelX = -1;
int g_hoverPixelY = -1;
int g_hoverSwatch = -1;
double g_lastRenderMs = 0.0;

int g_targetResolution = 32;
std::string g_lastImportedFile;

// Embedded Samples
const char* const SAMPLE_32X32 =
"[00,00,00,00,00,00,00,00,00,00,00,00,00,28,27,27,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00]\r\n"
"[00,00,00,00,00,00,00,00,00,00,00,00,28,27,27,27,27,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00]\r\n"
"[00,00,00,00,00,00,00,00,00,00,00,28,27,27,27,27,27,01,00,00,00,00,00,00,00,00,00,00,00,00,00,00]\r\n"
"[00,00,00,00,00,00,00,00,00,00,01,27,27,27,27,27,01,01,00,00,00,00,00,00,00,00,00,00,00,00,00,00]\r\n"
"[00,00,00,00,00,00,00,00,00,01,22,22,21,20,01,01,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00]\r\n"
"[00,00,00,00,00,00,00,00,01,22,21,21,20,22,24,01,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00]\r\n"
"[00,00,00,00,00,00,00,01,22,21,21,20,22,24,24,01,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00]\r\n"
"[00,00,00,00,00,00,00,01,24,01,01,01,01,01,24,01,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00]\r\n"
"[00,00,00,00,00,00,00,01,24,19,18,18,19,01,24,01,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00]\r\n"
"[00,00,00,00,00,00,00,01,24,24,24,24,24,24,25,01,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00]\r\n"
"[00,00,00,00,00,01,01,01,24,25,01,01,25,24,01,01,01,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00]\r\n"
"[00,00,00,00,01,09,21,22,01,01,27,27,01,01,22,21,09,01,00,00,00,00,00,00,00,00,00,00,00,00,00,00]\r\n"
"[00,00,00,01,09,22,22,24,01,27,27,27,27,01,24,22,22,09,01,00,00,00,00,00,00,00,00,00,00,00,00,00]\r\n"
"[00,00,00,01,24,24,25,01,22,21,20,22,24,22,01,25,24,24,01,00,00,00,00,00,00,00,00,00,00,00,00,00]\r\n"
"[00,00,01,09,01,01,01,22,21,20,09,09,20,22,24,01,01,01,09,01,00,00,00,00,00,00,00,00,00,00,00,00]\r\n"
"[00,00,01,22,01,27,01,22,20,09,09,09,09,20,24,01,27,01,22,01,00,00,00,00,00,00,00,00,00,00,00,00]\r\n"
"[00,00,01,24,01,27,01,24,22,20,09,09,20,22,25,01,27,01,24,01,00,00,00,00,00,00,00,00,00,00,00,00]\r\n"
"[00,00,01,25,01,27,01,25,24,22,22,22,22,24,25,01,27,01,25,01,00,00,00,00,00,00,00,00,00,00,00,00]\r\n"
"[00,00,00,01,01,27,01,01,04,04,04,04,04,04,01,01,27,01,01,00,00,00,00,00,00,00,00,00,00,00,00,00]\r\n"
"[00,00,00,00,01,27,27,01,09,09,09,09,09,09,01,27,27,01,00,00,00,00,00,00,00,00,00,00,00,00,00,00]\r\n"
"[00,00,00,00,01,27,27,01,04,04,04,04,04,04,01,27,27,01,00,00,00,00,00,00,00,00,00,00,00,00,00,00]\r\n"
"[00,00,00,00,01,27,27,01,22,20,01,01,20,22,01,27,27,01,00,00,00,00,00,00,00,00,00,00,00,00,00,00]\r\n"
"[00,00,00,00,01,27,01,22,21,24,01,01,24,21,22,01,27,01,00,00,00,00,00,00,00,00,00,00,00,00,00,00]\r\n"
"[00,00,00,00,01,27,01,22,20,24,01,01,24,20,22,01,27,01,00,00,00,00,00,00,00,00,00,00,00,00,00,00]\r\n"
"[00,00,00,00,01,01,01,24,22,25,01,01,25,22,24,01,01,01,00,00,00,00,00,00,00,00,00,00,00,00,00,00]\r\n"
"[00,00,00,00,00,00,01,24,22,25,01,01,25,22,24,01,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00]\r\n"
"[00,00,00,00,00,00,01,25,24,02,01,01,02,24,25,01,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00]\r\n"
"[00,00,00,00,00,00,01,25,25,02,01,01,02,25,25,01,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00]\r\n"
"[00,00,00,00,00,00,01,04,05,05,01,01,05,05,04,01,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00]\r\n"
"[00,00,00,00,00,00,01,04,04,01,01,01,01,04,04,01,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00]\r\n"
"[00,00,00,00,00,00,01,01,01,01,00,00,01,01,01,01,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00]\r\n"
"[00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00]";

const char* const SAMPLE_16X16 =
"[0, 0, 0, 0, 0, 0, 5, 5, 5, 0, 0, 0, 0, 0, 0, 0]\r\n"
"[0, 0, 0, 0, 0, 1, 5, 5, 1, 0, 0, 0, 0, 0, 0, 0]\r\n"
"[0, 0, 0, 0, 1, 3, 2, 2, 3, 1, 0, 0, 0, 0, 0, 0]\r\n"
"[0, 0, 0, 0, 1, 2, 7, 7, 2, 1, 0, 0, 0, 0, 0, 0]\r\n"
"[0, 0, 0, 0, 1, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0, 0]\r\n"
"[0, 0, 0, 1, 3, 2, 4, 4, 2, 3, 1, 0, 0, 0, 0, 0]\r\n"
"[0, 0, 1, 3, 2, 6, 6, 6, 6, 2, 3, 1, 0, 0, 0, 0]\r\n"
"[0, 1, 3, 3, 2, 2, 2, 2, 2, 2, 3, 3, 1, 0, 0, 0]\r\n"
"[0, 1, 4, 1, 2, 6, 5, 5, 6, 2, 1, 4, 1, 0, 0, 0]\r\n"
"[0, 0, 1, 1, 2, 2, 5, 5, 2, 2, 1, 1, 0, 0, 0, 0]\r\n"
"[0, 0, 0, 1, 2, 2, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0]\r\n"
"[0, 0, 0, 1, 4, 2, 1, 1, 2, 4, 1, 0, 0, 0, 0, 0]\r\n"
"[0, 0, 0, 1, 2, 2, 1, 1, 2, 2, 1, 0, 0, 0, 0, 0]\r\n"
"[0, 0, 0, 1, 2, 4, 1, 1, 4, 2, 1, 0, 0, 0, 0, 0]\r\n"
"[0, 0, 0, 1, 4, 4, 1, 1, 4, 4, 1, 0, 0, 0, 0, 0]\r\n"
"[0, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0]";

// Forward Declarations
LRESULT CALLBACK WndProcMain(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WndProcCanvas(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WndProcPalette(HWND, UINT, WPARAM, LPARAM);
void SwitchTab(ActiveTab newTab);
void UpdateTab2Stats(const std::string& filePath, int origW, int origH, int targetRes, size_t uniqueColors, double timeMs);
void UpdateResolutionButtonsUI(int res);
void SetResolution(int res);
void RemoveBackground();
bool AutoCropMatrix(spritex::tools::Matrix& mat, int margin = 1);
void UpdateMatrixFromEditor();
void ExportPNG(int scale);
void CopyImageToClipboard();

// Helper to set clipboard text
void SetClipboardText(const std::string& text) {
    if (OpenClipboard(g_hWndMain)) {
        EmptyClipboard();
        HGLOBAL hGlob = GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
        if (hGlob) {
            char* pData = (char*)GlobalLock(hGlob);
            memcpy(pData, text.c_str(), text.size() + 1);
            GlobalUnlock(hGlob);
            SetClipboardData(CF_TEXT, hGlob);
        }
        CloseClipboard();
    }
}

// Helper to get clipboard text
std::string GetClipboardText() {
    std::string result;
    if (OpenClipboard(g_hWndMain)) {
        HANDLE hData = GetClipboardData(CF_TEXT);
        if (hData) {
            char* pText = (char*)GlobalLock(hData);
            if (pText) {
                result = pText;
                GlobalUnlock(hData);
            }
        }
        CloseClipboard();
    }
    return result;
}

} // namespace

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Initialize common controls
    INITCOMMONCONTROLSEX icc = {};
    icc.dwSize = sizeof(icc);
    icc.dwICC  = ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icc);

    // Create GDI Brushes & Fonts
    g_hBrushMain  = CreateSolidBrush(COLOR_BG_MAIN);
    g_hBrushPanel = CreateSolidBrush(COLOR_BG_PANEL);
    g_hBrushEdit  = CreateSolidBrush(COLOR_BG_EDIT);

    g_hFontMono   = CreateFontA(-13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, "Consolas");
    g_hFontUI     = CreateFontA(-13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                CLEARTYPE_QUALITY, VARIABLE_PITCH | FF_SWISS, "Segoe UI");
    g_hFontUIBold = CreateFontA(-13, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                CLEARTYPE_QUALITY, VARIABLE_PITCH | FF_SWISS, "Segoe UI");
    g_hFontSmall  = CreateFontA(-11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                CLEARTYPE_QUALITY, VARIABLE_PITCH | FF_SWISS, "Segoe UI");

    // Register Window Classes
    WNDCLASSEXA wcMain = {};
    wcMain.cbSize        = sizeof(wcMain);
    wcMain.style         = CS_HREDRAW | CS_VREDRAW;
    wcMain.lpfnWndProc   = WndProcMain;
    wcMain.hInstance     = hInstance;
    wcMain.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wcMain.hbrBackground = g_hBrushMain;
    wcMain.lpszClassName = "SpritexMainClass";
    RegisterClassExA(&wcMain);

    WNDCLASSEXA wcCanvas = {};
    wcCanvas.cbSize        = sizeof(wcCanvas);
    wcCanvas.style         = CS_HREDRAW | CS_VREDRAW;
    wcCanvas.lpfnWndProc   = WndProcCanvas;
    wcCanvas.hInstance     = hInstance;
    wcCanvas.hCursor       = LoadCursor(NULL, IDC_CROSS);
    wcCanvas.hbrBackground = g_hBrushPanel;
    wcCanvas.lpszClassName = "SpritexCanvasClass";
    RegisterClassExA(&wcCanvas);

    WNDCLASSEXA wcPalette = {};
    wcPalette.cbSize        = sizeof(wcPalette);
    wcPalette.style         = CS_HREDRAW | CS_VREDRAW;
    wcPalette.lpfnWndProc   = WndProcPalette;
    wcPalette.hInstance     = hInstance;
    wcPalette.hCursor       = LoadCursor(NULL, IDC_HAND);
    wcPalette.hbrBackground = g_hBrushMain;
    wcPalette.lpszClassName = "SpritexPaletteClass";
    RegisterClassExA(&wcPalette);

    // Create Main Window
    g_hWndMain = CreateWindowExA(
        0, "SpritexMainClass",
        "spritex Studio - Visualizador & Editor de Pixel Art (DB32)",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, 1180, 840,
        NULL, NULL, hInstance, NULL
    );

    if (!g_hWndMain) return 1;

    ShowWindow(g_hWndMain, nCmdShow);
    UpdateWindow(g_hWndMain);

    // Preload 32x32 sample
    SetWindowTextA(g_hEditMatrix, SAMPLE_32X32);
    UpdateMatrixFromEditor();
    SwitchTab(ActiveTab::MatrixToPng);

    // Message Loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Cleanup GDI objects
    DeleteObject(g_hBrushMain);
    DeleteObject(g_hBrushPanel);
    DeleteObject(g_hBrushEdit);
    DeleteObject(g_hFontMono);
    DeleteObject(g_hFontUI);
    DeleteObject(g_hFontUIBold);
    DeleteObject(g_hFontSmall);

    return (int)msg.wParam;
}

namespace {

LRESULT CALLBACK WndProcMain(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            HINSTANCE hInst = ((LPCREATESTRUCT)lParam)->hInstance;

            // ==========================================
            // TOP TAB SELECTOR BAR
            // ==========================================
            g_hBtnTabMat2Png = CreateWindowExA(0, "BUTTON", "[ Matriz -> PNG ]",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                20, 12, 175, 30, hWnd, (HMENU)ID_TAB_MAT2PNG, hInst, NULL);
            SendMessage(g_hBtnTabMat2Png, WM_SETFONT, (WPARAM)g_hFontUIBold, TRUE);

            g_hBtnTabPng2Mat = CreateWindowExA(0, "BUTTON", "PNG -> Pixel Art",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                200, 12, 175, 30, hWnd, (HMENU)ID_TAB_PNG2MAT, hInst, NULL);
            SendMessage(g_hBtnTabPng2Mat, WM_SETFONT, (WPARAM)g_hFontUIBold, TRUE);

            // ==========================================
            // TAB 1 CONTROLS: MATRIZ -> PNG
            // ==========================================
            g_hBtnPaste = CreateWindowExA(0, "BUTTON", "Colar",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                20, 50, 48, 28, hWnd, (HMENU)ID_BTN_PASTE, hInst, NULL);
            SendMessage(g_hBtnPaste, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);

            g_hBtnFix = CreateWindowExA(0, "BUTTON", "Ajustar",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                72, 50, 60, 28, hWnd, (HMENU)ID_BTN_FIX_MATRIX, hInst, NULL);
            SendMessage(g_hBtnFix, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);

            g_hBtnCropTab1 = CreateWindowExA(0, "BUTTON", "Enquadrar",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                136, 50, 78, 28, hWnd, (HMENU)ID_BTN_CROP_TAB1, hInst, NULL);
            SendMessage(g_hBtnCropTab1, WM_SETFONT, (WPARAM)g_hFontUIBold, TRUE);

            g_hBtnRemBg1 = CreateWindowExA(0, "BUTTON", "Rem. Fundo",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                218, 50, 85, 28, hWnd, (HMENU)ID_BTN_REMBG_TAB1, hInst, NULL);
            SendMessage(g_hBtnRemBg1, WM_SETFONT, (WPARAM)g_hFontUIBold, TRUE);

            g_hBtnClear = CreateWindowExA(0, "BUTTON", "Limpar",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                307, 50, 52, 28, hWnd, (HMENU)ID_BTN_CLEAR, hInst, NULL);
            SendMessage(g_hBtnClear, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);

            g_hBtnSample32 = CreateWindowExA(0, "BUTTON", "Ex. 32x",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                363, 50, 68, 28, hWnd, (HMENU)ID_BTN_SAMPLE32, hInst, NULL);
            SendMessage(g_hBtnSample32, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);

            g_hBtnSample16 = CreateWindowExA(0, "BUTTON", "Ex. 16x",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                435, 50, 68, 28, hWnd, (HMENU)ID_BTN_SAMPLE16, hInst, NULL);
            SendMessage(g_hBtnSample16, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);

            // Multiline Matrix Editor
            g_hEditMatrix = CreateWindowExA(
                WS_EX_CLIENTEDGE, "EDIT", "",
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_NOHIDESEL,
                20, 86, 520, 534, hWnd, (HMENU)ID_EDIT_MATRIX, hInst, NULL
            );
            SendMessage(g_hEditMatrix, WM_SETFONT, (WPARAM)g_hFontMono, TRUE);

            g_hStatusMatrix = CreateWindowExA(
                0, "STATIC", "Aguardando entrada...",
                WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP,
                20, 626, 520, 24, hWnd, NULL, hInst, NULL
            );
            SendMessage(g_hStatusMatrix, WM_SETFONT, (WPARAM)g_hFontUIBold, TRUE);

            // ==========================================
            // TAB 2 CONTROLS: PNG -> PIXEL ART STUDIO
            // ==========================================
            g_hTab2BtnImport = CreateWindowExA(0, "BUTTON", "Carregar Imagem (PNG, JPG, BMP)",
                WS_CHILD | BS_PUSHBUTTON,
                20, 50, 520, 36, hWnd, (HMENU)ID_BTN_TAB2_IMPORT, hInst, NULL);
            SendMessage(g_hTab2BtnImport, WM_SETFONT, (WPARAM)g_hFontUIBold, TRUE);

            g_hTab2LblFile = CreateWindowExA(0, "STATIC", "Nenhuma imagem importada ainda.",
                WS_CHILD | SS_LEFTNOWORDWRAP,
                20, 92, 520, 20, hWnd, NULL, hInst, NULL);
            SendMessage(g_hTab2LblFile, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);

            g_hTab2LblResHeader = CreateWindowExA(0, "STATIC", "Resolucao Alvo da Pixel Art:",
                WS_CHILD | SS_LEFTNOWORDWRAP,
                20, 116, 520, 20, hWnd, NULL, hInst, NULL);
            SendMessage(g_hTab2LblResHeader, WM_SETFONT, (WPARAM)g_hFontUIBold, TRUE);

            g_hTab2BtnRes16 = CreateWindowExA(0, "BUTTON", "16x16",
                WS_CHILD | BS_PUSHBUTTON,
                20, 140, 160, 30, hWnd, (HMENU)ID_BTN_RES16, hInst, NULL);
            SendMessage(g_hTab2BtnRes16, WM_SETFONT, (WPARAM)g_hFontUIBold, TRUE);

            g_hTab2BtnRes32 = CreateWindowExA(0, "BUTTON", "[ 32x32 ]",
                WS_CHILD | BS_PUSHBUTTON,
                190, 140, 160, 30, hWnd, (HMENU)ID_BTN_RES32, hInst, NULL);
            SendMessage(g_hTab2BtnRes32, WM_SETFONT, (WPARAM)g_hFontUIBold, TRUE);

            g_hTab2BtnRes64 = CreateWindowExA(0, "BUTTON", "64x64",
                WS_CHILD | BS_PUSHBUTTON,
                360, 140, 160, 30, hWnd, (HMENU)ID_BTN_RES64, hInst, NULL);
            SendMessage(g_hTab2BtnRes64, WM_SETFONT, (WPARAM)g_hFontUIBold, TRUE);

            g_hTab2ChkCrop = CreateWindowExA(0, "BUTTON", "Auto-Enquadrar Silhueta (Cortar bordas vazias e ampliar)",
                WS_CHILD | BS_AUTOCHECKBOX,
                20, 178, 520, 24, hWnd, (HMENU)ID_CHK_TAB2_CROP, hInst, NULL);
            SendMessage(g_hTab2ChkCrop, WM_SETFONT, (WPARAM)g_hFontUIBold, TRUE);
            SendMessage(g_hTab2ChkCrop, BM_SETCHECK, BST_CHECKED, 0);

            g_hTab2BtnCrop = CreateWindowExA(0, "BUTTON", "Auto-Enquadrar Silhueta",
                WS_CHILD | BS_PUSHBUTTON,
                20, 206, 255, 32, hWnd, (HMENU)ID_BTN_TAB2_CROP, hInst, NULL);
            SendMessage(g_hTab2BtnCrop, WM_SETFONT, (WPARAM)g_hFontUIBold, TRUE);

            g_hTab2BtnRemBg = CreateWindowExA(0, "BUTTON", "Remover Fundo Externo",
                WS_CHILD | BS_PUSHBUTTON,
                285, 206, 255, 32, hWnd, (HMENU)ID_BTN_TAB2_REMBG, hInst, NULL);
            SendMessage(g_hTab2BtnRemBg, WM_SETFONT, (WPARAM)g_hFontUIBold, TRUE);

            g_hTab2LblBgTip = CreateWindowExA(0, "STATIC",
                "Dica: O Auto-Enquadramento amplia o personagem ocupando a tela com proporcao 1:1.\nO botao Remover Fundo apaga o fundo sem danificar reflexos brancos ou detalhes do sprite.",
                WS_CHILD,
                20, 244, 520, 36, hWnd, NULL, hInst, NULL);
            SendMessage(g_hTab2LblBgTip, WM_SETFONT, (WPARAM)g_hFontSmall, TRUE);

            g_hTab2LblStats = CreateWindowExA(0, "STATIC",
                "Estatisticas da Pixelizacao:\r\n\r\n"
                "  * Nenhuma imagem carregada ainda.\r\n"
                "  * Clique no botao acima para importar qualquer imagem PNG, JPG ou BMP.\r\n"
                "  * O spritex vai auto-enquadrar e quantizar para a paleta DawnBringer 32!",
                WS_CHILD,
                20, 288, 520, 130, hWnd, NULL, hInst, NULL);
            SendMessage(g_hTab2LblStats, WM_SETFONT, (WPARAM)g_hFontMono, TRUE);

            g_hTab2BtnCopyMat = CreateWindowExA(0, "BUTTON", "Copiar Matriz Gerada [Tokens]",
                WS_CHILD | BS_PUSHBUTTON,
                20, 426, 520, 36, hWnd, (HMENU)ID_BTN_TAB2_COPYMAT, hInst, NULL);
            SendMessage(g_hTab2BtnCopyMat, WM_SETFONT, (WPARAM)g_hFontUIBold, TRUE);

            g_hTab2BtnToEditor = CreateWindowExA(0, "BUTTON", "Abrir Matriz no Editor de Codigo (Aba 1)",
                WS_CHILD | BS_PUSHBUTTON,
                20, 470, 520, 36, hWnd, (HMENU)ID_BTN_TAB2_TO_EDIT, hInst, NULL);
            SendMessage(g_hTab2BtnToEditor, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);

            // ==========================================
            // SHARED RIGHT CONTROLS: CANVAS & EXPORTS
            // ==========================================
            HWND hLblScale = CreateWindowExA(0, "STATIC", "Escala:",
                WS_CHILD | WS_VISIBLE, 560, 16, 45, 24, hWnd, NULL, hInst, NULL);
            SendMessage(hLblScale, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);

            g_hComboScale = CreateWindowExA(0, "COMBOBOX", "",
                WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
                610, 13, 95, 200, hWnd, (HMENU)ID_COMBO_SCALE, hInst, NULL);
            SendMessage(g_hComboScale, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);
            SendMessage(g_hComboScale, CB_ADDSTRING, 0, (LPARAM)"1x (Nativo)");
            SendMessage(g_hComboScale, CB_ADDSTRING, 0, (LPARAM)"4x");
            SendMessage(g_hComboScale, CB_ADDSTRING, 0, (LPARAM)"8x");
            SendMessage(g_hComboScale, CB_ADDSTRING, 0, (LPARAM)"12x");
            SendMessage(g_hComboScale, CB_ADDSTRING, 0, (LPARAM)"16x (HD)");
            SendMessage(g_hComboScale, CB_ADDSTRING, 0, (LPARAM)"20x");
            SendMessage(g_hComboScale, CB_ADDSTRING, 0, (LPARAM)"24x");
            SendMessage(g_hComboScale, CB_SETCURSEL, 4, 0); // 16x default

            g_hChkGrid = CreateWindowExA(0, "BUTTON", "Grade de Pixels",
                WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                715, 16, 110, 24, hWnd, (HMENU)ID_CHK_GRID, hInst, NULL);
            SendMessage(g_hChkGrid, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);

            HWND hBtnExpHD = CreateWindowExA(0, "BUTTON", "Salvar PNG (HD)",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                835, 12, 105, 30, hWnd, (HMENU)ID_BTN_EXPORT_HD, hInst, NULL);
            SendMessage(hBtnExpHD, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);

            HWND hBtnExp1X = CreateWindowExA(0, "BUTTON", "Salvar PNG (1x)",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                945, 12, 100, 30, hWnd, (HMENU)ID_BTN_EXPORT_1X, hInst, NULL);
            SendMessage(hBtnExp1X, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);

            HWND hBtnCopy = CreateWindowExA(0, "BUTTON", "Copiar Imagem",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                1050, 12, 100, 30, hWnd, (HMENU)ID_BTN_COPY_IMG, hInst, NULL);
            SendMessage(hBtnCopy, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);

            // Custom Canvas Window
            g_hCanvas = CreateWindowExA(
                WS_EX_CLIENTEDGE, "SpritexCanvasClass", "",
                WS_CHILD | WS_VISIBLE,
                560, 50, 590, 570, hWnd, (HMENU)ID_CANVAS, hInst, NULL
            );

            // Inspector Pixel Status Label
            g_hStatusPixel = CreateWindowExA(
                0, "STATIC", "Mova o cursor sobre a imagem para inspecionar pixels.",
                WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP,
                560, 626, 590, 24, hWnd, NULL, hInst, NULL
            );
            SendMessage(g_hStatusPixel, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);

            // Palette Strip Bar
            g_hPaletteBar = CreateWindowExA(
                0, "SpritexPaletteClass", "",
                WS_CHILD | WS_VISIBLE,
                20, 665, 1130, 85, hWnd, (HMENU)ID_PALETTE_BAR, hInst, NULL
            );

            return 0;
        }

        case WM_SIZE: {
            int width  = LOWORD(lParam);
            int height = HIWORD(lParam);
            if (width < 600 || height < 400) break;

            int leftWidth = (width - 60) / 2;
            int rightLeft = leftWidth + 40;
            int rightWidth = width - rightLeft - 20;

            // Tab bar buttons:
            SetWindowPos(g_hBtnTabMat2Png, NULL, 20, 12, 175, 30, SWP_NOZORDER);
            SetWindowPos(g_hBtnTabPng2Mat, NULL, 200, 12, 175, 30, SWP_NOZORDER);

            // Tab 1 controls:
            SetWindowPos(g_hBtnPaste, NULL, 20, 50, 48, 28, SWP_NOZORDER);
            SetWindowPos(g_hBtnFix, NULL, 72, 50, 60, 28, SWP_NOZORDER);
            SetWindowPos(g_hBtnCropTab1, NULL, 136, 50, 78, 28, SWP_NOZORDER);
            SetWindowPos(g_hBtnRemBg1, NULL, 218, 50, 85, 28, SWP_NOZORDER);
            SetWindowPos(g_hBtnClear, NULL, 307, 50, 52, 28, SWP_NOZORDER);
            SetWindowPos(g_hBtnSample32, NULL, 363, 50, 68, 28, SWP_NOZORDER);
            SetWindowPos(g_hBtnSample16, NULL, 435, 50, 68, 28, SWP_NOZORDER);

            int editorTop = 86;
            int editorHeight = height - 200;
            SetWindowPos(g_hEditMatrix, NULL, 20, editorTop, leftWidth, editorHeight, SWP_NOZORDER);
            SetWindowPos(g_hStatusMatrix, NULL, 20, editorTop + editorHeight + 6, leftWidth, 24, SWP_NOZORDER);

            // Tab 2 controls:
            SetWindowPos(g_hTab2BtnImport, NULL, 20, 50, leftWidth, 36, SWP_NOZORDER);
            SetWindowPos(g_hTab2LblFile, NULL, 20, 92, leftWidth, 20, SWP_NOZORDER);
            SetWindowPos(g_hTab2LblResHeader, NULL, 20, 116, leftWidth, 20, SWP_NOZORDER);

            int resBtnW = (leftWidth - 20) / 3;
            SetWindowPos(g_hTab2BtnRes16, NULL, 20, 140, resBtnW, 30, SWP_NOZORDER);
            SetWindowPos(g_hTab2BtnRes32, NULL, 20 + resBtnW + 10, 140, resBtnW, 30, SWP_NOZORDER);
            SetWindowPos(g_hTab2BtnRes64, NULL, 20 + 2 * (resBtnW + 10), 140, resBtnW, 30, SWP_NOZORDER);

            SetWindowPos(g_hTab2ChkCrop, NULL, 20, 178, leftWidth, 24, SWP_NOZORDER);

            int halfBtnW = (leftWidth - 10) / 2;
            SetWindowPos(g_hTab2BtnCrop, NULL, 20, 206, halfBtnW, 32, SWP_NOZORDER);
            SetWindowPos(g_hTab2BtnRemBg, NULL, 20 + halfBtnW + 10, 206, halfBtnW, 32, SWP_NOZORDER);

            SetWindowPos(g_hTab2LblBgTip, NULL, 20, 244, leftWidth, 36, SWP_NOZORDER);
            SetWindowPos(g_hTab2LblStats, NULL, 20, 288, leftWidth, 130, SWP_NOZORDER);
            SetWindowPos(g_hTab2BtnCopyMat, NULL, 20, 426, leftWidth, 36, SWP_NOZORDER);
            SetWindowPos(g_hTab2BtnToEditor, NULL, 20, 470, leftWidth, 36, SWP_NOZORDER);

            // Right side (Canvas & controls):
            SetWindowPos(g_hCanvas, NULL, rightLeft, 50, rightWidth, editorHeight + 36, SWP_NOZORDER);
            SetWindowPos(g_hStatusPixel, NULL, rightLeft, editorTop + editorHeight + 6, rightWidth, 24, SWP_NOZORDER);

            SetWindowPos(g_hPaletteBar, NULL, 20, height - 90, width - 40, 80, SWP_NOZORDER);
            InvalidateRect(hWnd, NULL, TRUE);
            break;
        }

        case WM_CTLCOLOREDIT: {
            HDC hdcEdit = (HDC)wParam;
            SetTextColor(hdcEdit, COLOR_TEXT_EDIT);
            SetBkColor(hdcEdit, COLOR_BG_EDIT);
            return (LRESULT)g_hBrushEdit;
        }

        case WM_CTLCOLORSTATIC: {
            HDC hdcStatic = (HDC)wParam;
            HWND hStatic = (HWND)lParam;
            SetBkMode(hdcStatic, TRANSPARENT);

            if (hStatic == g_hStatusMatrix) {
                if (g_matrixValid) {
                    if (g_matrixWasRepaired) {
                        SetTextColor(hdcStatic, COLOR_WARNING);
                    } else {
                        SetTextColor(hdcStatic, COLOR_SUCCESS);
                    }
                } else {
                    SetTextColor(hdcStatic, COLOR_ERROR);
                }
            } else if (hStatic == g_hStatusPixel) {
                SetTextColor(hdcStatic, COLOR_ACCENT_BLUE);
            } else if (hStatic == g_hTab2LblFile || hStatic == g_hTab2LblResHeader) {
                SetTextColor(hdcStatic, COLOR_ACCENT_BLUE);
            } else if (hStatic == g_hTab2LblStats) {
                SetTextColor(hdcStatic, COLOR_TEXT_EDIT);
            } else if (hStatic == g_hTab2LblBgTip) {
                SetTextColor(hdcStatic, COLOR_TEXT_MUTED);
            } else if (hStatic == g_hTab2ChkCrop) {
                SetTextColor(hdcStatic, COLOR_TEXT_EDIT);
            } else {
                SetTextColor(hdcStatic, COLOR_TEXT_EDIT);
            }
            return (LRESULT)g_hBrushMain;
        }

        case WM_COMMAND: {
            int wmId    = LOWORD(wParam);
            int wmEvent = HIWORD(wParam);

            if (wmId == ID_TAB_MAT2PNG) {
                SwitchTab(ActiveTab::MatrixToPng);
            } else if (wmId == ID_TAB_PNG2MAT) {
                SwitchTab(ActiveTab::PngToPixelArt);
            } else if (wmId == ID_EDIT_MATRIX && wmEvent == EN_CHANGE) {
                UpdateMatrixFromEditor();
            } else if (wmId == ID_BTN_TAB2_IMPORT) {
                char szFile[MAX_PATH] = "";
                OPENFILENAMEA ofn = {};
                ofn.lStructSize = sizeof(OPENFILENAMEA);
                ofn.hwndOwner   = hWnd;
                ofn.lpstrFilter = "Imagens (*.png;*.jpg;*.jpeg;*.bmp)\0*.png;*.jpg;*.jpeg;*.bmp\0Todos os Arquivos (*.*)\0*.*\0";
                ofn.lpstrFile   = szFile;
                ofn.nMaxFile    = sizeof(szFile);
                ofn.lpstrTitle  = "Selecionar Imagem para Converter em Pixel Art (DB32)";
                ofn.Flags       = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

                if (GetOpenFileNameA(&ofn)) {
                    g_lastImportedFile = szFile;
                    spritex::tools::QuantizerOptions qOpt;
                    qOpt.targetWidth = g_targetResolution;
                    qOpt.targetHeight = g_targetResolution;
                    qOpt.mode = spritex::tools::DownsampleMode::Modal;
                    qOpt.keyWhite = true;
                    qOpt.outline = false;
                    qOpt.despeckle = true;
                    qOpt.autoCrop = (SendMessage(g_hTab2ChkCrop, BM_GETCHECK, 0, 0) == BST_CHECKED);
                    qOpt.cropMargin = 1;

                    auto tStart = std::chrono::high_resolution_clock::now();
                    auto cRes = spritex::tools::ImageConverter::from_file(szFile, qOpt);
                    auto tEnd = std::chrono::high_resolution_clock::now();
                    double timeMs = std::chrono::duration<double, std::milli>(tEnd - tStart).count();

                    if (cRes.success) {
                        std::string matStr = spritex::tools::MatrixParser::format_matrix(cRes.matrix);
                        SetWindowTextA(g_hEditMatrix, matStr.c_str());
                        UpdateMatrixFromEditor();

                        if (g_targetResolution == 64 && g_currentScale > 8) {
                            g_currentScale = 8;
                            SendMessage(g_hComboScale, CB_SETCURSEL, 2, 0); // 8x
                        } else if (g_targetResolution == 16 && g_currentScale < 16) {
                            g_currentScale = 16;
                            SendMessage(g_hComboScale, CB_SETCURSEL, 4, 0); // 16x
                        }

                        UpdateTab2Stats(szFile, cRes.originalWidth, cRes.originalHeight, g_targetResolution, cRes.uniqueColors, timeMs);

                        std::stringstream ss;
                        ss << "Imagem pixelizada: " << cRes.originalWidth << "x" << cRes.originalHeight
                           << " px -> " << g_targetResolution << "x" << g_targetResolution
                           << (qOpt.autoCrop ? " [Auto-Enquadrada] (" : " (") << cRes.uniqueColors << " cores DB32)";
                        SetWindowTextA(g_hStatusPixel, ss.str().c_str());
                    } else {
                        MessageBoxA(hWnd, cRes.error.c_str(), "Erro na Conversao", MB_ICONERROR);
                    }
                }
            } else if (wmId == ID_BTN_RES16) {
                SetResolution(16);
            } else if (wmId == ID_BTN_RES32) {
                SetResolution(32);
            } else if (wmId == ID_BTN_RES64) {
                SetResolution(64);
            } else if (wmId == ID_BTN_TAB2_CROP) {
                // Auto-crop button in Tab 2
                if (!g_lastImportedFile.empty()) {
                    spritex::tools::QuantizerOptions qOpt;
                    qOpt.targetWidth = g_targetResolution;
                    qOpt.targetHeight = g_targetResolution;
                    qOpt.mode = spritex::tools::DownsampleMode::Modal;
                    qOpt.keyWhite = true;
                    qOpt.outline = false;
                    qOpt.despeckle = true;
                    qOpt.autoCrop = true;
                    qOpt.cropMargin = 1;

                    auto tStart = std::chrono::high_resolution_clock::now();
                    auto cRes = spritex::tools::ImageConverter::from_file(g_lastImportedFile, qOpt);
                    auto tEnd = std::chrono::high_resolution_clock::now();
                    double timeMs = std::chrono::duration<double, std::milli>(tEnd - tStart).count();

                    if (cRes.success) {
                        std::string matStr = spritex::tools::MatrixParser::format_matrix(cRes.matrix);
                        SetWindowTextA(g_hEditMatrix, matStr.c_str());
                        UpdateMatrixFromEditor();
                        UpdateTab2Stats(g_lastImportedFile, cRes.originalWidth, cRes.originalHeight, g_targetResolution, cRes.uniqueColors, timeMs);
                        SetWindowTextA(g_hStatusPixel, "Silhueta auto-enquadrada a partir da imagem original HD!");
                    }
                } else if (g_matrixValid && !g_matrix.empty()) {
                    if (AutoCropMatrix(g_matrix, 1)) {
                        std::string matStr = spritex::tools::MatrixParser::format_matrix(g_matrix);
                        SetWindowTextA(g_hEditMatrix, matStr.c_str());
                        UpdateMatrixFromEditor();
                        SetWindowTextA(g_hStatusPixel, "Matriz auto-enquadrada e centralizada com sucesso!");
                    } else {
                        SetWindowTextA(g_hStatusPixel, "A pixel art ja esta ocupando todo o espaco disponivel.");
                    }
                } else {
                    MessageBoxA(hWnd, "Nenhuma imagem ou matriz carregada para enquadrar.", "Atencao", MB_ICONWARNING);
                }
            } else if (wmId == ID_BTN_CROP_TAB1) {
                // Auto-crop button in Tab 1 (Matrix editor)
                if (g_matrixValid && !g_matrix.empty()) {
                    if (AutoCropMatrix(g_matrix, 1)) {
                        std::string matStr = spritex::tools::MatrixParser::format_matrix(g_matrix);
                        SetWindowTextA(g_hEditMatrix, matStr.c_str());
                        UpdateMatrixFromEditor();
                        SetWindowTextA(g_hStatusPixel, "Matriz enquadrada e ampliada mantendo proporcao 1:1!");
                    } else {
                        SetWindowTextA(g_hStatusPixel, "A matriz ja esta enquadrada no tamanho maximo.");
                    }
                } else {
                    MessageBoxA(hWnd, "Insira uma matriz valida antes de enquadrar.", "Atencao", MB_ICONWARNING);
                }
            } else if (wmId == ID_BTN_TAB2_REMBG || wmId == ID_BTN_REMBG_TAB1) {
                RemoveBackground();
            } else if (wmId == ID_BTN_TAB2_COPYMAT) {
                if (g_matrixValid && !g_matrix.empty()) {
                    std::string matStr = spritex::tools::MatrixParser::format_matrix(g_matrix);
                    SetClipboardText(matStr);
                    SetWindowTextA(g_hStatusPixel, "Matriz de tokens copiada para a Area de Transferencia!");
                } else {
                    MessageBoxA(hWnd, "Nenhuma matriz valida disponivel para copiar.", "Atencao", MB_ICONWARNING);
                }
            } else if (wmId == ID_BTN_TAB2_TO_EDIT) {
                SwitchTab(ActiveTab::MatrixToPng);
                SetWindowTextA(g_hStatusPixel, "Matriz aberta no Editor de Codigo para edicao pixel a pixel.");
            } else if (wmId == ID_BTN_PASTE) {
                std::string clip = GetClipboardText();
                if (!clip.empty()) {
                    g_lastImportedFile.clear();
                    SetWindowTextA(g_hEditMatrix, clip.c_str());
                    UpdateMatrixFromEditor();
                }
            } else if (wmId == ID_BTN_SAMPLE32) {
                g_lastImportedFile.clear();
                SetWindowTextA(g_hEditMatrix, SAMPLE_32X32);
                UpdateMatrixFromEditor();
                SetResolution(32);
            } else if (wmId == ID_BTN_SAMPLE16) {
                g_lastImportedFile.clear();
                SetWindowTextA(g_hEditMatrix, SAMPLE_16X16);
                UpdateMatrixFromEditor();
                SetResolution(16);
            } else if (wmId == ID_BTN_FIX_MATRIX) {
                if (g_matrixValid && !g_matrix.empty()) {
                    std::string formatted = spritex::tools::MatrixParser::format_matrix(g_matrix);
                    SetWindowTextA(g_hEditMatrix, formatted.c_str());
                    UpdateMatrixFromEditor();
                }
            } else if (wmId == ID_BTN_CLEAR) {
                g_lastImportedFile.clear();
                SetWindowTextA(g_hEditMatrix, "");
                UpdateMatrixFromEditor();
            } else if (wmId == ID_COMBO_SCALE && wmEvent == CBN_SELCHANGE) {
                int cur = (int)SendMessage(g_hComboScale, CB_GETCURSEL, 0, 0);
                const int scales[] = {1, 4, 8, 12, 16, 20, 24};
                if (cur >= 0 && cur < 7) {
                    g_currentScale = scales[cur];
                    InvalidateRect(g_hCanvas, NULL, FALSE);
                }
            } else if (wmId == ID_CHK_GRID) {
                g_showGrid = (SendMessage(g_hChkGrid, BM_GETCHECK, 0, 0) == BST_CHECKED);
                InvalidateRect(g_hCanvas, NULL, FALSE);
            } else if (wmId == ID_BTN_EXPORT_HD) {
                ExportPNG(16);
            } else if (wmId == ID_BTN_EXPORT_1X) {
                ExportPNG(1);
            } else if (wmId == ID_BTN_COPY_IMG) {
                CopyImageToClipboard();
            }
            break;
        }

        case WM_DESTROY: {
            PostQuitMessage(0);
            return 0;
        }
    }
    return DefWindowProcA(hWnd, uMsg, wParam, lParam);
}

// Switch between Tab 1 and Tab 2
void SwitchTab(ActiveTab newTab) {
    g_activeTab = newTab;

    if (newTab == ActiveTab::MatrixToPng) {
        SetWindowTextA(g_hBtnTabMat2Png, "[ Matriz -> PNG ]");
        SetWindowTextA(g_hBtnTabPng2Mat, "PNG -> Pixel Art");
    } else {
        SetWindowTextA(g_hBtnTabMat2Png, "Matriz -> PNG");
        SetWindowTextA(g_hBtnTabPng2Mat, "[ PNG -> Pixel Art ]");
    }

    int show1 = (newTab == ActiveTab::MatrixToPng) ? SW_SHOW : SW_HIDE;
    if (g_hBtnPaste) ShowWindow(g_hBtnPaste, show1);
    if (g_hBtnFix) ShowWindow(g_hBtnFix, show1);
    if (g_hBtnCropTab1) ShowWindow(g_hBtnCropTab1, show1);
    if (g_hBtnRemBg1) ShowWindow(g_hBtnRemBg1, show1);
    if (g_hBtnClear) ShowWindow(g_hBtnClear, show1);
    if (g_hBtnSample32) ShowWindow(g_hBtnSample32, show1);
    if (g_hBtnSample16) ShowWindow(g_hBtnSample16, show1);
    if (g_hEditMatrix) ShowWindow(g_hEditMatrix, show1);
    if (g_hStatusMatrix) ShowWindow(g_hStatusMatrix, show1);

    int show2 = (newTab == ActiveTab::PngToPixelArt) ? SW_SHOW : SW_HIDE;
    if (g_hTab2BtnImport) ShowWindow(g_hTab2BtnImport, show2);
    if (g_hTab2LblFile) ShowWindow(g_hTab2LblFile, show2);
    if (g_hTab2LblResHeader) ShowWindow(g_hTab2LblResHeader, show2);
    if (g_hTab2BtnRes16) ShowWindow(g_hTab2BtnRes16, show2);
    if (g_hTab2BtnRes32) ShowWindow(g_hTab2BtnRes32, show2);
    if (g_hTab2BtnRes64) ShowWindow(g_hTab2BtnRes64, show2);
    if (g_hTab2ChkCrop) ShowWindow(g_hTab2ChkCrop, show2);
    if (g_hTab2BtnCrop) ShowWindow(g_hTab2BtnCrop, show2);
    if (g_hTab2BtnRemBg) ShowWindow(g_hTab2BtnRemBg, show2);
    if (g_hTab2LblBgTip) ShowWindow(g_hTab2LblBgTip, show2);
    if (g_hTab2LblStats) ShowWindow(g_hTab2LblStats, show2);
    if (g_hTab2BtnCopyMat) ShowWindow(g_hTab2BtnCopyMat, show2);
    if (g_hTab2BtnToEditor) ShowWindow(g_hTab2BtnToEditor, show2);

    InvalidateRect(g_hWndMain, NULL, TRUE);
}

// Update Tab 2 stats display card
void UpdateTab2Stats(const std::string& filePath, int origW, int origH, int targetRes, size_t uniqueColors, double timeMs) {
    if (!g_hTab2LblStats) return;

    std::string fileName = filePath;
    size_t lastSlash = fileName.find_last_of("\\/");
    if (lastSlash != std::string::npos) {
        fileName = fileName.substr(lastSlash + 1);
    }

    std::stringstream ssFile;
    ssFile << "Arquivo Carregado: " << fileName << " (" << origW << "x" << origH << " px)";
    SetWindowTextA(g_hTab2LblFile, ssFile.str().c_str());

    std::stringstream ss;
    ss << "Estatisticas da Pixelizacao:\r\n\r\n"
       << "  * Resolucao Original:  " << origW << " x " << origH << " px (32-bit RGBA)\r\n"
       << "  * Pixel Art Gerada:    " << targetRes << " x " << targetRes << " (" << (targetRes * targetRes) << " tokens)\r\n"
       << "  * Cores DawnBringer32: " << uniqueColors << " / 32 cores unicas\r\n"
       << "  * Algoritmo:           Modal Sampling + Auto-Crop CIE76\r\n"
       << "  * Tempo de Execucao:   " << std::fixed << std::setprecision(2) << timeMs << " ms";
    SetWindowTextA(g_hTab2LblStats, ss.str().c_str());
}

// Auto-crop bounding box of non-zero tokens and fit inside matrix
bool AutoCropMatrix(spritex::tools::Matrix& mat, int margin) {
    if (mat.empty()) return false;

    // Detect background token: check corner tokens
    uint8_t c1 = mat.at(0, 0);
    uint8_t c2 = mat.at(mat.width - 1, 0);
    uint8_t c3 = mat.at(0, mat.height - 1);
    uint8_t c4 = mat.at(mat.width - 1, mat.height - 1);
    uint8_t bgToken = (c1 == c2 && c1 == c3) ? c1 : 0;

    int minX = (int)mat.width, maxX = -1;
    int minY = (int)mat.height, maxY = -1;

    for (int y = 0; y < (int)mat.height; ++y) {
        for (int x = 0; x < (int)mat.width; ++x) {
            uint8_t t = mat.at(x, y);
            if (t != 0 && t != bgToken) {
                if (x < minX) minX = x;
                if (x > maxX) maxX = x;
                if (y < minY) minY = y;
                if (y > maxY) maxY = y;
            }
        }
    }

    if (minX > maxX || minY > maxY) return false;

    int bboxW = maxX - minX + 1;
    int bboxH = maxY - minY + 1;

    // If it already occupies almost the whole matrix, no scaling needed
    if (bboxW >= (int)mat.width - 2 && bboxH >= (int)mat.height - 2) {
        return false;
    }

    int targetW = (int)mat.width;
    int targetH = (int)mat.height;
    int availW = std::max(1, targetW - 2 * margin);
    int availH = std::max(1, targetH - 2 * margin);

    float scale = std::min((float)availW / bboxW, (float)availH / bboxH);
    int newW = std::clamp((int)std::round(bboxW * scale), 1, targetW);
    int newH = std::clamp((int)std::round(bboxH * scale), 1, targetH);

    int offsetX = (targetW - newW) / 2;
    int offsetY = (targetH - newH) / 2;

    spritex::tools::Matrix newMat;
    newMat.width = targetW;
    newMat.height = targetH;
    newMat.data.resize(targetW * targetH, 0);

    for (int y = 0; y < newH; ++y) {
        int srcY = minY + y * bboxH / newH;
        if (srcY >= (int)mat.height) srcY = (int)mat.height - 1;
        for (int x = 0; x < newW; ++x) {
            int srcX = minX + x * bboxW / newW;
            if (srcX >= (int)mat.width) srcX = (int)mat.width - 1;

            newMat.data[(offsetY + y) * targetW + (offsetX + x)] = mat.at(srcX, srcY);
        }
    }

    mat = newMat;
    return true;
}

// Parse text from editor and update internal matrix state
void UpdateMatrixFromEditor() {
    int len = GetWindowTextLengthA(g_hEditMatrix);
    if (len == 0) {
        g_matrix = {};
        g_matrixValid = false;
        g_matrixWasRepaired = false;
        SetWindowTextA(g_hStatusMatrix, "Nenhuma matriz inserida.");
        InvalidateRect(g_hCanvas, NULL, TRUE);
        InvalidateRect(g_hStatusMatrix, NULL, TRUE);
        return;
    }

    std::string text(len + 1, '\0');
    GetWindowTextA(g_hEditMatrix, &text[0], len + 1);
    text.resize(len);

    auto startT = std::chrono::high_resolution_clock::now();
    auto res = spritex::tools::MatrixParser::parse(text, true); // autoRepair = true
    auto endT = std::chrono::high_resolution_clock::now();
    g_lastRenderMs = std::chrono::duration<double, std::milli>(endT - startT).count();

    if (res.success) {
        g_matrix = res.matrix;
        g_matrixValid = true;
        g_matrixWasRepaired = res.wasRepaired;
        g_matrixWarning = res.warning;
        g_parseError.clear();

        if (g_matrix.width == g_matrix.height && (g_matrix.width == 16 || g_matrix.width == 32 || g_matrix.width == 64)) {
            g_targetResolution = (int)g_matrix.width;
            UpdateResolutionButtonsUI(g_targetResolution);
        }

        // Count unique colors
        std::array<bool, 256> seen{};
        size_t unique = 0;
        for (uint8_t t : g_matrix.data) {
            if (!seen[t]) { seen[t] = true; unique++; }
        }

        std::stringstream ss;
        if (res.wasRepaired) {
            ss << "[AVISO AUTO-AJUSTADO] " << res.warning << " | "
               << g_matrix.width << "x" << g_matrix.height
               << " | " << unique << " cores DB32 (clique em 'Auto-Ajustar' para corrigir o texto)";
        } else {
            ss << "[OK] " << g_matrix.width << "x" << g_matrix.height
               << " (" << (g_matrix.width * g_matrix.height) << " tokens) | "
               << unique << " cores DB32 | Parse: " << std::fixed << std::setprecision(2) << g_lastRenderMs << " ms";
        }
        SetWindowTextA(g_hStatusMatrix, ss.str().c_str());
    } else {
        g_matrixValid = false;
        g_matrixWasRepaired = false;
        g_parseError = res.error;
        std::string err = "[ERRO] " + res.error;
        SetWindowTextA(g_hStatusMatrix, err.c_str());
    }

    InvalidateRect(g_hCanvas, NULL, FALSE);
    InvalidateRect(g_hStatusMatrix, NULL, TRUE);
}

// Canvas Window Procedure (Double-Buffered Pixel Art Rendering)
LRESULT CALLBACK WndProcCanvas(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_MOUSEMOVE: {
            int mx = GET_X_LPARAM(lParam);
            int my = GET_Y_LPARAM(lParam);

            if (!g_matrixValid || g_matrix.empty()) {
                g_hoverPixelX = -1;
                g_hoverPixelY = -1;
                break;
            }

            RECT rc;
            GetClientRect(hWnd, &rc);
            int spriteW = (int)g_matrix.width * g_currentScale;
            int spriteH = (int)g_matrix.height * g_currentScale;
            int offsetX = (rc.right - spriteW) / 2;
            int offsetY = (rc.bottom - spriteH) / 2;

            int px = (mx - offsetX) / g_currentScale;
            int py = (my - offsetY) / g_currentScale;

            if (px >= 0 && px < (int)g_matrix.width && py >= 0 && py < (int)g_matrix.height) {
                if (px != g_hoverPixelX || py != g_hoverPixelY) {
                    g_hoverPixelX = px;
                    g_hoverPixelY = py;

                    uint8_t token = g_matrix.at(px, py);
                    auto c = spritex::tools::Palette32::get(token);
                    const char* name = spritex::tools::Palette32::get_name(token);

                    std::stringstream ss;
                    ss << "Pixel: (" << px << ", " << py << ") | "
                       << "Token: " << (int)token << " | "
                       << name;
                    if (c.a == 0) {
                        ss << " (Transparente)";
                    } else {
                        ss << " (#" << std::hex << std::uppercase
                           << std::setw(2) << std::setfill('0') << (int)c.r
                           << std::setw(2) << std::setfill('0') << (int)c.g
                           << std::setw(2) << std::setfill('0') << (int)c.b << ")";
                    }
                    SetWindowTextA(g_hStatusPixel, ss.str().c_str());
                }
            } else {
                if (g_hoverPixelX != -1 || g_hoverPixelY != -1) {
                    g_hoverPixelX = -1;
                    g_hoverPixelY = -1;
                    SetWindowTextA(g_hStatusPixel, "Mova o cursor sobre o sprite para inspecionar pixels.");
                }
            }
            break;
        }

        case WM_RBUTTONDOWN: {
            int mx = GET_X_LPARAM(lParam);
            int my = GET_Y_LPARAM(lParam);

            if (!g_matrixValid || g_matrix.empty()) break;

            RECT rc;
            GetClientRect(hWnd, &rc);
            int spriteW = (int)g_matrix.width * g_currentScale;
            int spriteH = (int)g_matrix.height * g_currentScale;
            int offsetX = (rc.right - spriteW) / 2;
            int offsetY = (rc.bottom - spriteH) / 2;

            int px = (mx - offsetX) / g_currentScale;
            int py = (my - offsetY) / g_currentScale;

            if (px >= 0 && px < (int)g_matrix.width && py >= 0 && py < (int)g_matrix.height) {
                uint8_t targetToken = g_matrix.at(px, py);
                if (targetToken != 0) {
                    std::vector<bool> visited(g_matrix.width * g_matrix.height, false);
                    std::queue<std::pair<int, int>> q;

                    visited[py * g_matrix.width + px] = true;
                    q.push({px, py});

                    size_t count = 0;
                    while (!q.empty()) {
                        auto p = q.front();
                        int cx = p.first;
                        int cy = p.second;
                        q.pop();

                        g_matrix.data[cy * g_matrix.width + cx] = 0;
                        count++;

                        const int dx[] = {0, 0, -1, 1};
                        const int dy[] = {-1, 1, 0, 0};
                        for (int i = 0; i < 4; ++i) {
                            int nx = cx + dx[i];
                            int ny = cy + dy[i];
                            if (nx >= 0 && nx < (int)g_matrix.width && ny >= 0 && ny < (int)g_matrix.height) {
                                size_t nidx = ny * g_matrix.width + nx;
                                if (!visited[nidx] && g_matrix.at(nx, ny) == targetToken) {
                                    visited[nidx] = true;
                                    q.push({nx, ny});
                                }
                            }
                        }
                    }

                    std::string newText = spritex::tools::MatrixParser::format_matrix(g_matrix);
                    SetWindowTextA(g_hEditMatrix, newText.c_str());
                    UpdateMatrixFromEditor();

                    const char* name = spritex::tools::Palette32::get_name(targetToken);
                    std::string msg = "Regiao da cor '" + std::string(name) + "' (" +
                                      std::to_string(count) + " pixels) convertida para Transparente!";
                    SetWindowTextA(g_hStatusPixel, msg.c_str());
                }
            }
            break;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            RECT rc;
            GetClientRect(hWnd, &rc);
            int w = rc.right - rc.left;
            int h = rc.bottom - rc.top;

            // Double buffering
            HDC hdcMem = CreateCompatibleDC(hdc);
            HBITMAP hbmMem = CreateCompatibleBitmap(hdc, w, h);
            HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMem);

            // Dark canvas background
            FillRect(hdcMem, &rc, g_hBrushPanel);

            if (g_matrixValid && !g_matrix.empty()) {
                int spriteW = (int)g_matrix.width * g_currentScale;
                int spriteH = (int)g_matrix.height * g_currentScale;
                int offsetX = (w - spriteW) / 2;
                int offsetY = (h - spriteH) / 2;

                // Checkerboard pattern colors for transparency
                COLORREF cLight = RGB(38, 42, 58);
                COLORREF cDark  = RGB(28, 31, 44);
                HBRUSH hBrLight = CreateSolidBrush(cLight);
                HBRUSH hBrDark  = CreateSolidBrush(cDark);

                // Grid pen
                HPEN hPenGrid = CreatePen(PS_SOLID, 1, RGB(45, 50, 70));

                int checkSize = std::max(4, g_currentScale / 2);

                for (size_t y = 0; y < g_matrix.height; ++y) {
                    for (size_t x = 0; x < g_matrix.width; ++x) {
                        int rx = offsetX + (int)x * g_currentScale;
                        int ry = offsetY + (int)y * g_currentScale;
                        RECT rPixel = {rx, ry, rx + g_currentScale, ry + g_currentScale};

                        uint8_t token = g_matrix.at(x, y);
                        auto color = spritex::tools::Palette32::get(token);

                        if (color.a == 0) {
                            // Render Aseprite-like checkerboard
                            for (int cy = ry; cy < ry + g_currentScale; cy += checkSize) {
                                for (int cx = rx; cx < rx + g_currentScale; cx += checkSize) {
                                    RECT rCheck = {
                                        cx, cy,
                                        std::min(cx + checkSize, rx + g_currentScale),
                                        std::min(cy + checkSize, ry + g_currentScale)
                                    };
                                    bool isLight = ((cx / checkSize) + (cy / checkSize)) % 2 == 0;
                                    FillRect(hdcMem, &rCheck, isLight ? hBrLight : hBrDark);
                                }
                            }
                        } else {
                            HBRUSH hBrPixel = CreateSolidBrush(RGB(color.r, color.g, color.b));
                            FillRect(hdcMem, &rPixel, hBrPixel);
                            DeleteObject(hBrPixel);
                        }

                        // Pixel Grid
                        if (g_showGrid && g_currentScale >= 6) {
                            HPEN hOldPen = (HPEN)SelectObject(hdcMem, hPenGrid);
                            SelectObject(hdcMem, GetStockObject(NULL_BRUSH));
                            Rectangle(hdcMem, rPixel.left, rPixel.top, rPixel.right + 1, rPixel.bottom + 1);
                            SelectObject(hdcMem, hOldPen);
                        }
                    }
                }

                // Sprite border outline
                HPEN hPenBorder = CreatePen(PS_SOLID, 1, COLOR_ACCENT_BLUE);
                HPEN hOldP = (HPEN)SelectObject(hdcMem, hPenBorder);
                SelectObject(hdcMem, GetStockObject(NULL_BRUSH));
                Rectangle(hdcMem, offsetX - 1, offsetY - 1, offsetX + spriteW + 1, offsetY + spriteH + 1);
                SelectObject(hdcMem, hOldP);
                DeleteObject(hPenBorder);

                DeleteObject(hBrLight);
                DeleteObject(hBrDark);
                DeleteObject(hPenGrid);
            } else {
                // Empty state text
                SetBkMode(hdcMem, TRANSPARENT);
                SelectObject(hdcMem, g_hFontUI);
                SetTextColor(hdcMem, COLOR_TEXT_MUTED);
                std::string msg = g_matrixValid ? "Nenhuma matriz carregada" : ("Erro de sintaxe:\n" + g_parseError);
                DrawTextA(hdcMem, msg.c_str(), -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            }

            // Blit to screen
            BitBlt(hdc, 0, 0, w, h, hdcMem, 0, 0, SRCCOPY);

            SelectObject(hdcMem, hbmOld);
            DeleteObject(hbmMem);
            DeleteDC(hdcMem);

            EndPaint(hWnd, &ps);
            return 0;
        }
    }
    return DefWindowProcA(hWnd, uMsg, wParam, lParam);
}

// Bottom DB32 Interactive Palette Bar
LRESULT CALLBACK WndProcPalette(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_MOUSEMOVE: {
            int mx = GET_X_LPARAM(lParam);
            int swatch = mx / 34; // 32px swatch + 2px gap
            if (swatch >= 0 && swatch < (int)spritex::tools::Palette32::SIZE) {
                if (swatch != g_hoverSwatch) {
                    g_hoverSwatch = swatch;
                    auto c = spritex::tools::Palette32::get((uint8_t)swatch);
                    const char* name = spritex::tools::Palette32::get_name((uint8_t)swatch);

                    std::stringstream ss;
                    ss << "Paleta: Token [" << swatch << "] = " << name;
                    if (c.a == 0) ss << " (Transparente)";
                    else {
                        ss << " (#" << std::hex << std::uppercase
                           << std::setw(2) << std::setfill('0') << (int)c.r
                           << std::setw(2) << std::setfill('0') << (int)c.g
                           << std::setw(2) << std::setfill('0') << (int)c.b << ")";
                    }
                    ss << " - Clique para copiar o token!";
                    SetWindowTextA(g_hStatusPixel, ss.str().c_str());
                    InvalidateRect(hWnd, NULL, FALSE);
                }
            } else {
                g_hoverSwatch = -1;
            }
            break;
        }

        case WM_LBUTTONDOWN: {
            int mx = GET_X_LPARAM(lParam);
            int swatch = mx / 34;
            if (swatch >= 0 && swatch < (int)spritex::tools::Palette32::SIZE) {
                std::string tokenStr = std::to_string(swatch);
                SetClipboardText(tokenStr);

                std::string msg = "Token " + tokenStr + " copiado para a area de transferencia!";
                SetWindowTextA(g_hStatusPixel, msg.c_str());
            }
            break;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            RECT rc;
            GetClientRect(hWnd, &rc);
            int w = rc.right - rc.left;
            int h = rc.bottom - rc.top;

            HDC hdcMem = CreateCompatibleDC(hdc);
            HBITMAP hbmMem = CreateCompatibleBitmap(hdc, w, h);
            HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMem);

            FillRect(hdcMem, &rc, g_hBrushMain);

            // Title label
            SetBkMode(hdcMem, TRANSPARENT);
            SelectObject(hdcMem, g_hFontUIBold);
            SetTextColor(hdcMem, COLOR_TEXT_MUTED);
            TextOutA(hdcMem, 4, 2, "PALETA MASTER DB32 (Clique em uma cor para copiar o token):", 59);

            for (size_t i = 0; i < spritex::tools::Palette32::SIZE; ++i) {
                int sx = 4 + (int)i * 34;
                int sy = 22;
                RECT rSwatch = {sx, sy, sx + 32, sy + 32};

                auto c = spritex::tools::Palette32::get((uint8_t)i);

                if (c.a == 0) {
                    // Checkered pattern for transparent swatch
                    HBRUSH hBr1 = CreateSolidBrush(RGB(50, 50, 70));
                    HBRUSH hBr2 = CreateSolidBrush(RGB(30, 30, 45));
                    RECT r1 = {sx, sy, sx + 16, sy + 16};
                    RECT r2 = {sx + 16, sy, sx + 32, sy + 16};
                    RECT r3 = {sx, sy + 16, sx + 16, sy + 32};
                    RECT r4 = {sx + 16, sy + 16, sx + 32, sy + 32};
                    FillRect(hdcMem, &r1, hBr1);
                    FillRect(hdcMem, &r2, hBr2);
                    FillRect(hdcMem, &r3, hBr2);
                    FillRect(hdcMem, &r4, hBr1);
                    DeleteObject(hBr1);
                    DeleteObject(hBr2);
                } else {
                    HBRUSH hBr = CreateSolidBrush(RGB(c.r, c.g, c.b));
                    FillRect(hdcMem, &rSwatch, hBr);
                    DeleteObject(hBr);
                }

                // Swatch border
                HPEN hPen = CreatePen(PS_SOLID, 1, ((int)i == g_hoverSwatch) ? RGB(255, 255, 255) : RGB(55, 60, 80));
                HPEN hOld = (HPEN)SelectObject(hdcMem, hPen);
                SelectObject(hdcMem, GetStockObject(NULL_BRUSH));
                Rectangle(hdcMem, rSwatch.left, rSwatch.top, rSwatch.right, rSwatch.bottom);
                SelectObject(hdcMem, hOld);
                DeleteObject(hPen);

                // Token Number text below
                SelectObject(hdcMem, g_hFontSmall);
                SetTextColor(hdcMem, COLOR_TEXT_MUTED);
                std::string numStr = std::to_string(i);
                RECT rNum = {sx, sy + 34, sx + 32, sy + 48};
                DrawTextA(hdcMem, numStr.c_str(), -1, &rNum, DT_CENTER | DT_SINGLELINE);
            }

            BitBlt(hdc, 0, 0, w, h, hdcMem, 0, 0, SRCCOPY);

            SelectObject(hdcMem, hbmOld);
            DeleteObject(hbmMem);
            DeleteDC(hdcMem);

            EndPaint(hWnd, &ps);
            return 0;
        }
    }
    return DefWindowProcA(hWnd, uMsg, wParam, lParam);
}

// Automatically detects background color and flood-fills outer perimeter to transparent (0)
void RemoveBackground() {
    if (!g_matrixValid || g_matrix.empty()) {
        MessageBoxA(g_hWndMain, "Nenhuma matriz valida carregada para remover o fundo.", "Atencao", MB_ICONWARNING);
        return;
    }

    // 1. Detect background token from the outer corners & borders
    uint8_t c1 = g_matrix.at(0, 0);
    uint8_t c2 = g_matrix.at(g_matrix.width - 1, 0);
    uint8_t c3 = g_matrix.at(0, g_matrix.height - 1);
    uint8_t c4 = g_matrix.at(g_matrix.width - 1, g_matrix.height - 1);

    std::unordered_map<uint8_t, int> counts;
    counts[c1]++; counts[c2]++; counts[c3]++; counts[c4]++;

    uint8_t bgToken = c1;
    int maxCorner = 0;
    for (const auto& pair : counts) {
        if (pair.second > maxCorner) {
            maxCorner = pair.second;
            bgToken = pair.first;
        }
    }

    // If dominant corner is already transparent (0), look for another border token
    if (bgToken == 0) {
        std::unordered_map<uint8_t, int> borderCounts;
        for (size_t x = 0; x < g_matrix.width; ++x) {
            uint8_t t1 = g_matrix.at(x, 0);
            uint8_t t2 = g_matrix.at(x, g_matrix.height - 1);
            if (t1 != 0) borderCounts[t1]++;
            if (t2 != 0) borderCounts[t2]++;
        }
        for (size_t y = 0; y < g_matrix.height; ++y) {
            uint8_t t1 = g_matrix.at(0, y);
            uint8_t t2 = g_matrix.at(g_matrix.width - 1, y);
            if (t1 != 0) borderCounts[t1]++;
            if (t2 != 0) borderCounts[t2]++;
        }

        if (borderCounts.empty()) {
            MessageBoxA(g_hWndMain, "O fundo da imagem ja e 100% transparente (Token 0).", "Informacao", MB_ICONINFORMATION);
            return;
        }

        int maxB = 0;
        for (const auto& pair : borderCounts) {
            if (pair.second > maxB) {
                maxB = pair.second;
                bgToken = pair.first;
            }
        }
    }

    // 2. Flood-Fill from all 4 outer borders for pixels matching bgToken
    std::vector<bool> visited(g_matrix.width * g_matrix.height, false);
    std::queue<std::pair<int, int>> q;

    auto pushIfBorderBg = [&](int x, int y) {
        size_t idx = y * g_matrix.width + x;
        if (!visited[idx] && g_matrix.at(x, y) == bgToken) {
            visited[idx] = true;
            q.push({x, y});
        }
    };

    for (size_t x = 0; x < g_matrix.width; ++x) {
        pushIfBorderBg((int)x, 0);
        pushIfBorderBg((int)x, (int)g_matrix.height - 1);
    }
    for (size_t y = 0; y < g_matrix.height; ++y) {
        pushIfBorderBg(0, (int)y);
        pushIfBorderBg((int)g_matrix.width - 1, (int)y);
    }

    size_t removedCount = 0;
    while (!q.empty()) {
        auto p = q.front();
        int cx = p.first;
        int cy = p.second;
        q.pop();

        g_matrix.data[cy * g_matrix.width + cx] = 0;
        removedCount++;

        const int dx[] = {0, 0, -1, 1};
        const int dy[] = {-1, 1, 0, 0};
        for (int i = 0; i < 4; ++i) {
            int nx = cx + dx[i];
            int ny = cy + dy[i];
            if (nx >= 0 && nx < (int)g_matrix.width && ny >= 0 && ny < (int)g_matrix.height) {
                size_t nidx = ny * g_matrix.width + nx;
                if (!visited[nidx] && g_matrix.at(nx, ny) == bgToken) {
                    visited[nidx] = true;
                    q.push({nx, ny});
                }
            }
        }
    }

    // 3. Update editor text and canvas
    std::string newText = spritex::tools::MatrixParser::format_matrix(g_matrix);
    SetWindowTextA(g_hEditMatrix, newText.c_str());
    UpdateMatrixFromEditor();

    const char* colorName = spritex::tools::Palette32::get_name(bgToken);
    std::string msg = "Fundo removido: " + std::to_string(removedCount) +
                      " pixels externos da cor '" + colorName +
                      "' (Token " + std::to_string(bgToken) + ") convertidos para Transparente (0)!";
    SetWindowTextA(g_hStatusPixel, msg.c_str());
}

// Update Resolution Buttons Visual Indication
void UpdateResolutionButtonsUI(int res) {
    if (g_hTab2BtnRes16) SetWindowTextA(g_hTab2BtnRes16, (res == 16) ? "[ 16x16 ]" : "16x16");
    if (g_hTab2BtnRes32) SetWindowTextA(g_hTab2BtnRes32, (res == 32) ? "[ 32x32 ]" : "32x32");
    if (g_hTab2BtnRes64) SetWindowTextA(g_hTab2BtnRes64, (res == 64) ? "[ 64x64 ]" : "64x64");
}

// Set target resolution (16x16, 32x32, 64x64) and convert or resample
void SetResolution(int res) {
    if (res != 16 && res != 32 && res != 64) return;
    g_targetResolution = res;
    UpdateResolutionButtonsUI(res);

    // 1. If an image was previously imported, re-process from original file at new resolution
    if (!g_lastImportedFile.empty()) {
        spritex::tools::QuantizerOptions qOpt;
        qOpt.targetWidth = res;
        qOpt.targetHeight = res;
        qOpt.mode = spritex::tools::DownsampleMode::Modal;
        qOpt.keyWhite = true;
        qOpt.outline = false;
        qOpt.despeckle = true;
        qOpt.autoCrop = (SendMessage(g_hTab2ChkCrop, BM_GETCHECK, 0, 0) == BST_CHECKED);
        qOpt.cropMargin = 1;

        auto tStart = std::chrono::high_resolution_clock::now();
        auto cRes = spritex::tools::ImageConverter::from_file(g_lastImportedFile, qOpt);
        auto tEnd = std::chrono::high_resolution_clock::now();
        double timeMs = std::chrono::duration<double, std::milli>(tEnd - tStart).count();

        if (cRes.success) {
            std::string matStr = spritex::tools::MatrixParser::format_matrix(cRes.matrix);
            SetWindowTextA(g_hEditMatrix, matStr.c_str());
            UpdateMatrixFromEditor();

            if (res == 64 && g_currentScale > 8) {
                g_currentScale = 8;
                SendMessage(g_hComboScale, CB_SETCURSEL, 2, 0); // 8x
            } else if (res == 16 && g_currentScale < 16) {
                g_currentScale = 16;
                SendMessage(g_hComboScale, CB_SETCURSEL, 4, 0); // 16x
            }

            UpdateTab2Stats(g_lastImportedFile, cRes.originalWidth, cRes.originalHeight, res, cRes.uniqueColors, timeMs);

            std::stringstream ss;
            ss << "Imagem re-pixelizada para " << res << "x" << res << " (" << cRes.uniqueColors << " cores DB32)";
            SetWindowTextA(g_hStatusPixel, ss.str().c_str());
            return;
        }
    }

    // 2. If no image was imported, but a valid matrix is in editor, resample using nearest-neighbor
    if (g_matrixValid && !g_matrix.empty() && ((int)g_matrix.width != res || (int)g_matrix.height != res)) {
        spritex::tools::Matrix resampled;
        resampled.width = res;
        resampled.height = res;
        resampled.data.resize(res * res, 0);

        for (int y = 0; y < res; ++y) {
            int sy = std::clamp((int)(y * g_matrix.height / res), 0, (int)g_matrix.height - 1);
            for (int x = 0; x < res; ++x) {
                int sx = std::clamp((int)(x * g_matrix.width / res), 0, (int)g_matrix.width - 1);
                resampled.data[y * res + x] = g_matrix.at(sx, sy);
            }
        }

        std::string matStr = spritex::tools::MatrixParser::format_matrix(resampled);
        SetWindowTextA(g_hEditMatrix, matStr.c_str());
        UpdateMatrixFromEditor();

        if (res == 64 && g_currentScale > 8) {
            g_currentScale = 8;
            SendMessage(g_hComboScale, CB_SETCURSEL, 2, 0); // 8x
        } else if (res == 16 && g_currentScale < 16) {
            g_currentScale = 16;
            SendMessage(g_hComboScale, CB_SETCURSEL, 4, 0); // 16x
        }

        std::stringstream ss;
        ss << "Pixel art redimensionada para " << res << "x" << res;
        SetWindowTextA(g_hStatusPixel, ss.str().c_str());
    } else {
        std::stringstream ss;
        ss << "Resolucao de saida selecionada: " << res << "x" << res << ". Importe uma imagem ou cole uma matriz.";
        SetWindowTextA(g_hStatusPixel, ss.str().c_str());
    }
}

// Export PNG via file dialog
void ExportPNG(int scale) {
    if (!g_matrixValid || g_matrix.empty()) {
        MessageBoxA(g_hWndMain, "Por favor, insira uma matriz valida antes de exportar.", "Atencao", MB_ICONWARNING);
        return;
    }

    char szFile[MAX_PATH] = "sprite.png";
    OPENFILENAMEA ofn = {};
    ofn.lStructSize     = sizeof(OPENFILENAMEA);
    ofn.hwndOwner       = g_hWndMain;
    ofn.lpstrFilter     = "Imagens PNG (*.png)\0*.png\0Todos os Arquivos (*.*)\0*.*\0";
    ofn.lpstrFile       = szFile;
    ofn.nMaxFile        = sizeof(szFile);
    ofn.lpstrTitle      = "Salvar Imagem PNG";
    ofn.Flags           = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt     = "png";

    if (GetSaveFileNameA(&ofn)) {
        spritex::tools::Rasterizer::Options opt;
        opt.scale = scale;

        auto res = spritex::tools::Rasterizer::to_png(g_matrix, szFile, opt);
        if (res.success) {
            std::string msg = "PNG exportado com sucesso!\n\nArquivo: " + std::string(szFile) +
                              "\nDimensoes: " + std::to_string(res.outputWidth) + "x" + std::to_string(res.outputHeight) + " px";
            MessageBoxA(g_hWndMain, msg.c_str(), "Sucesso", MB_ICONINFORMATION);
        } else {
            MessageBoxA(g_hWndMain, res.errorMessage.c_str(), "Erro na Exportacao", MB_ICONERROR);
        }
    }
}

// Copy DIB to Windows Clipboard
void CopyImageToClipboard() {
    if (!g_matrixValid || g_matrix.empty()) {
        MessageBoxA(g_hWndMain, "Nenhuma matriz valida disponivel para copiar.", "Atencao", MB_ICONWARNING);
        return;
    }

    int scale = g_currentScale;
    int outW = (int)g_matrix.width * scale;
    int outH = (int)g_matrix.height * scale;

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       = outW;
    bmi.bmiHeader.biHeight      = outH; // Bottom-up for CF_DIB
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biBitCount    = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    size_t dibSize = sizeof(BITMAPINFOHEADER) + (outW * outH * 4);

    if (OpenClipboard(g_hWndMain)) {
        EmptyClipboard();

        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, dibSize);
        if (hMem) {
            uint8_t* pDest = (uint8_t*)GlobalLock(hMem);
            memcpy(pDest, &bmi.bmiHeader, sizeof(BITMAPINFOHEADER));
            uint32_t* pPixels = (uint32_t*)(pDest + sizeof(BITMAPINFOHEADER));

            for (int y = outH - 1; y >= 0; --y) {
                int srcY = (outH - 1 - y) / scale;
                for (int x = 0; x < outW; ++x) {
                    int srcX = x / scale;
                    uint8_t token = g_matrix.at(srcX, srcY);
                    auto c = spritex::tools::Palette32::get(token);

                    if (c.a == 0) {
                        *pPixels++ = 0x00000000;
                    } else {
                        // BGRA format for CF_DIB
                        uint32_t val = (0xFF << 24) | (c.r << 16) | (c.g << 8) | c.b;
                        *pPixels++ = val;
                    }
                }
            }

            GlobalUnlock(hMem);
            SetClipboardData(CF_DIB, hMem);
            SetWindowTextA(g_hStatusPixel, "Imagem copiada para a Area de Transferencia (CF_DIB)!");
        }
        CloseClipboard();
    }
}

} // namespace
