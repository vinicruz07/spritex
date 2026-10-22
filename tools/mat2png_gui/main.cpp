#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../mat2png/palette.hpp"
#include "../mat2png/parser.hpp"
#include "../mat2png/rasterizer.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>

#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <chrono>

// Enable modern visual styles
#pragma comment(linker, "\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

namespace {

// UI Control IDs
constexpr int ID_BTN_PASTE     = 1001;
constexpr int ID_BTN_SAMPLE32  = 1002;
constexpr int ID_BTN_SAMPLE16  = 1003;
constexpr int ID_BTN_CLEAR     = 1004;
constexpr int ID_EDIT_MATRIX   = 1005;
constexpr int ID_CANVAS        = 1006;
constexpr int ID_COMBO_SCALE   = 1007;
constexpr int ID_CHK_GRID      = 1008;
constexpr int ID_BTN_EXPORT_HD = 1009;
constexpr int ID_BTN_EXPORT_1X = 1010;
constexpr int ID_BTN_COPY_IMG  = 1011;
constexpr int ID_PALETTE_BAR   = 1012;
constexpr int ID_BTN_FIX_MATRIX = 1013;

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

// Global Windows & Handles
HWND g_hWndMain       = nullptr;
HWND g_hEditMatrix    = nullptr;
HWND g_hCanvas        = nullptr;
HWND g_hStatusMatrix  = nullptr;
HWND g_hStatusPixel   = nullptr;
HWND g_hComboScale    = nullptr;
HWND g_hChkGrid       = nullptr;
HWND g_hPaletteBar    = nullptr;

HBRUSH g_hBrushMain   = nullptr;
HBRUSH g_hBrushPanel  = nullptr;
HBRUSH g_hBrushEdit   = nullptr;
HFONT  g_hFontMono    = nullptr;
HFONT  g_hFontUI      = nullptr;
HFONT  g_hFontUIBold  = nullptr;
HFONT  g_hFontSmall   = nullptr;

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

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    // Set Windows console/process code page
    SetConsoleOutputCP(CP_UTF8);

    // Initialize Common Controls
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icex);

    // Create Dark Brushes
    g_hBrushMain  = CreateSolidBrush(COLOR_BG_MAIN);
    g_hBrushPanel = CreateSolidBrush(COLOR_BG_PANEL);
    g_hBrushEdit  = CreateSolidBrush(COLOR_BG_EDIT);

    // Create Fonts
    g_hFontMono   = CreateFontA(-13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, "Consolas");
    g_hFontUI     = CreateFontA(-13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                CLEARTYPE_QUALITY, VARIABLE_PITCH | FF_SWISS, "Segoe UI");
    g_hFontUIBold = CreateFontA(-14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                CLEARTYPE_QUALITY, VARIABLE_PITCH | FF_SWISS, "Segoe UI");
    g_hFontSmall  = CreateFontA(-11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                CLEARTYPE_QUALITY, VARIABLE_PITCH | FF_SWISS, "Segoe UI");

    // Register Window Classes
    WNDCLASSEXA wcMain = {};
    wcMain.cbSize        = sizeof(WNDCLASSEXA);
    wcMain.lpfnWndProc   = WndProcMain;
    wcMain.hInstance     = hInstance;
    wcMain.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wcMain.hbrBackground = g_hBrushMain;
    wcMain.lpszClassName = "SpritexMainClass";
    RegisterClassExA(&wcMain);

    WNDCLASSEXA wcCanvas = {};
    wcCanvas.cbSize        = sizeof(WNDCLASSEXA);
    wcCanvas.lpfnWndProc   = WndProcCanvas;
    wcCanvas.hInstance     = hInstance;
    wcCanvas.hCursor       = LoadCursor(NULL, IDC_CROSS);
    wcCanvas.hbrBackground = g_hBrushPanel;
    wcCanvas.lpszClassName = "SpritexCanvasClass";
    RegisterClassExA(&wcCanvas);

    WNDCLASSEXA wcPalette = {};
    wcPalette.cbSize        = sizeof(WNDCLASSEXA);
    wcPalette.lpfnWndProc   = WndProcPalette;
    wcPalette.hInstance     = hInstance;
    wcPalette.hCursor       = LoadCursor(NULL, IDC_HAND);
    wcPalette.hbrBackground = g_hBrushPanel;
    wcPalette.lpszClassName = "SpritexPaletteClass";
    RegisterClassExA(&wcPalette);

    // Create Main Window
    g_hWndMain = CreateWindowExA(
        0, "SpritexMainClass",
        "spritex Studio - Visualizador & Editor Interativo de Pixel Art (DB32)",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, 1160, 820,
        NULL, NULL, hInstance, NULL
    );

    if (!g_hWndMain) return 1;

    ShowWindow(g_hWndMain, nCmdShow);
    UpdateWindow(g_hWndMain);

    // Preload 32x32 sample
    SetWindowTextA(g_hEditMatrix, SAMPLE_32X32);
    UpdateMatrixFromEditor();

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

            // 1. Left Action Buttons
            HWND hBtnPaste = CreateWindowExA(0, "BUTTON", "Colar",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                20, 20, 85, 30, hWnd, (HMENU)ID_BTN_PASTE, hInst, NULL);
            SendMessage(hBtnPaste, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);

            HWND hBtn32 = CreateWindowExA(0, "BUTTON", "Exemplo 32x32",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                110, 20, 95, 30, hWnd, (HMENU)ID_BTN_SAMPLE32, hInst, NULL);
            SendMessage(hBtn32, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);

            HWND hBtn16 = CreateWindowExA(0, "BUTTON", "Exemplo 16x16",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                210, 20, 95, 30, hWnd, (HMENU)ID_BTN_SAMPLE16, hInst, NULL);
            SendMessage(hBtn16, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);

            HWND hBtnFix = CreateWindowExA(0, "BUTTON", "Auto-Ajustar",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                310, 20, 95, 30, hWnd, (HMENU)ID_BTN_FIX_MATRIX, hInst, NULL);
            SendMessage(hBtnFix, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);

            HWND hBtnClr = CreateWindowExA(0, "BUTTON", "Limpar",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                410, 20, 65, 30, hWnd, (HMENU)ID_BTN_CLEAR, hInst, NULL);
            SendMessage(hBtnClr, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);

            // 2. Multiline Editor
            g_hEditMatrix = CreateWindowExA(
                WS_EX_CLIENTEDGE, "EDIT", "",
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_NOHIDESEL,
                20, 60, 480, 560, hWnd, (HMENU)ID_EDIT_MATRIX, hInst, NULL
            );
            SendMessage(g_hEditMatrix, WM_SETFONT, (WPARAM)g_hFontMono, TRUE);

            // 3. Status Label under editor
            g_hStatusMatrix = CreateWindowExA(
                0, "STATIC", "Aguardando entrada...",
                WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP,
                20, 630, 480, 24, hWnd, NULL, hInst, NULL
            );
            SendMessage(g_hStatusMatrix, WM_SETFONT, (WPARAM)g_hFontUIBold, TRUE);

            // 4. Right Canvas Controls
            HWND hLblScale = CreateWindowExA(0, "STATIC", "Escala:",
                WS_CHILD | WS_VISIBLE, 520, 25, 45, 24, hWnd, NULL, hInst, NULL);
            SendMessage(hLblScale, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);

            g_hComboScale = CreateWindowExA(0, "COMBOBOX", "",
                WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
                570, 22, 95, 200, hWnd, (HMENU)ID_COMBO_SCALE, hInst, NULL);
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
                675, 25, 110, 24, hWnd, (HMENU)ID_CHK_GRID, hInst, NULL);
            SendMessage(g_hChkGrid, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);

            HWND hBtnExpHD = CreateWindowExA(0, "BUTTON", "Salvar PNG (HD)",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                795, 20, 105, 30, hWnd, (HMENU)ID_BTN_EXPORT_HD, hInst, NULL);
            SendMessage(hBtnExpHD, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);

            HWND hBtnExp1X = CreateWindowExA(0, "BUTTON", "Salvar PNG (1x)",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                905, 20, 100, 30, hWnd, (HMENU)ID_BTN_EXPORT_1X, hInst, NULL);
            SendMessage(hBtnExp1X, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);

            HWND hBtnCopy = CreateWindowExA(0, "BUTTON", "Copiar Imagem",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                1010, 20, 100, 30, hWnd, (HMENU)ID_BTN_COPY_IMG, hInst, NULL);
            SendMessage(hBtnCopy, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);

            // 5. Custom Canvas Window
            g_hCanvas = CreateWindowExA(
                WS_EX_CLIENTEDGE, "SpritexCanvasClass", "",
                WS_CHILD | WS_VISIBLE,
                520, 60, 600, 560, hWnd, (HMENU)ID_CANVAS, hInst, NULL
            );

            // 6. Inspector Pixel Status Label
            g_hStatusPixel = CreateWindowExA(
                0, "STATIC", "Mova o cursor sobre a imagem para inspecionar pixels.",
                WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP,
                520, 630, 600, 24, hWnd, NULL, hInst, NULL
            );
            SendMessage(g_hStatusPixel, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);

            // 7. Palette Strip Bar
            g_hPaletteBar = CreateWindowExA(
                0, "SpritexPaletteClass", "",
                WS_CHILD | WS_VISIBLE,
                20, 665, 1100, 85, hWnd, (HMENU)ID_PALETTE_BAR, hInst, NULL
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
            int editorHeight = height - 190;

            SetWindowPos(g_hEditMatrix, NULL, 20, 60, leftWidth, editorHeight, SWP_NOZORDER);
            SetWindowPos(g_hStatusMatrix, NULL, 20, 60 + editorHeight + 8, leftWidth, 24, SWP_NOZORDER);

            SetWindowPos(g_hCanvas, NULL, rightLeft, 60, rightWidth, editorHeight, SWP_NOZORDER);
            SetWindowPos(g_hStatusPixel, NULL, rightLeft, 60 + editorHeight + 8, rightWidth, 24, SWP_NOZORDER);

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
            } else {
                SetTextColor(hdcStatic, COLOR_TEXT_EDIT);
            }
            return (LRESULT)g_hBrushMain;
        }

        case WM_COMMAND: {
            int wmId    = LOWORD(wParam);
            int wmEvent = HIWORD(wParam);

            if (wmId == ID_EDIT_MATRIX && wmEvent == EN_CHANGE) {
                UpdateMatrixFromEditor();
            } else if (wmId == ID_BTN_PASTE) {
                std::string clip = GetClipboardText();
                if (!clip.empty()) {
                    SetWindowTextA(g_hEditMatrix, clip.c_str());
                    UpdateMatrixFromEditor();
                }
            } else if (wmId == ID_BTN_SAMPLE32) {
                SetWindowTextA(g_hEditMatrix, SAMPLE_32X32);
                UpdateMatrixFromEditor();
            } else if (wmId == ID_BTN_SAMPLE16) {
                SetWindowTextA(g_hEditMatrix, SAMPLE_16X16);
                UpdateMatrixFromEditor();
            } else if (wmId == ID_BTN_FIX_MATRIX) {
                if (g_matrixValid && !g_matrix.empty()) {
                    std::string formatted = spritex::tools::MatrixParser::format_matrix(g_matrix);
                    SetWindowTextA(g_hEditMatrix, formatted.c_str());
                    UpdateMatrixFromEditor();
                }
            } else if (wmId == ID_BTN_CLEAR) {
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

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcA(hWnd, uMsg, wParam, lParam);
}

void UpdateMatrixFromEditor() {
    int len = GetWindowTextLengthA(g_hEditMatrix);
    if (len == 0) {
        g_matrix = spritex::tools::Matrix();
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

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            RECT rc;
            GetClientRect(hWnd, &rc);
            int w = rc.right - rc.left;
            int h = rc.bottom - rc.top;

            // Create memory DC for flicker-free double buffering
            HDC hdcMem = CreateCompatibleDC(hdc);
            HBITMAP hbmMem = CreateCompatibleBitmap(hdc, w, h);
            HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMem);

            // Fill canvas background
            HBRUSH hBrCanvas = CreateSolidBrush(RGB(28, 30, 42));
            FillRect(hdcMem, &rc, hBrCanvas);
            DeleteObject(hBrCanvas);

            if (g_matrixValid && !g_matrix.empty()) {
                int spriteW = (int)g_matrix.width * g_currentScale;
                int spriteH = (int)g_matrix.height * g_currentScale;
                int offsetX = (w - spriteW) / 2;
                int offsetY = (h - spriteH) / 2;

                // 1. Draw Checkerboard background for transparency
                int checkSize = 16;
                HBRUSH hBrCheck1 = CreateSolidBrush(RGB(38, 41, 56));
                HBRUSH hBrCheck2 = CreateSolidBrush(RGB(48, 52, 70));

                for (int cy = offsetY; cy < offsetY + spriteH; cy += checkSize) {
                    for (int cx = offsetX; cx < offsetX + spriteW; cx += checkSize) {
                        RECT rCheck = {
                            cx, cy,
                            std::min(cx + checkSize, offsetX + spriteW),
                            std::min(cy + checkSize, offsetY + spriteH)
                        };
                        int checkX = (cx - offsetX) / checkSize;
                        int checkY = (cy - offsetY) / checkSize;
                        FillRect(hdcMem, &rCheck, ((checkX + checkY) % 2 == 0) ? hBrCheck1 : hBrCheck2);
                    }
                }
                DeleteObject(hBrCheck1);
                DeleteObject(hBrCheck2);

                // 2. Prepare 32-bit BGRA top-down DIB for StretchDIBits
                BITMAPINFO bmi = {};
                bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
                bmi.bmiHeader.biWidth       = (LONG)g_matrix.width;
                bmi.bmiHeader.biHeight      = -((LONG)g_matrix.height); // Negative = Top-Down
                bmi.bmiHeader.biPlanes      = 1;
                bmi.bmiHeader.biBitCount    = 32;
                bmi.bmiHeader.biCompression = BI_RGB;

                std::vector<uint32_t> bgraPixels(g_matrix.width * g_matrix.height);
                for (size_t y = 0; y < g_matrix.height; ++y) {
                    for (size_t x = 0; x < g_matrix.width; ++x) {
                        uint8_t token = g_matrix.at(x, y);
                        auto c = spritex::tools::Palette32::get(token);
                        size_t idx = y * g_matrix.width + x;

                        if (c.a == 0) {
                            // Transparent pixel: blend with checkerboard
                            bool isEven = (((x * g_currentScale / checkSize) + (y * g_currentScale / checkSize)) % 2 == 0);
                            uint8_t bg = isEven ? 45 : 55;
                            bgraPixels[idx] = (255 << 24) | (bg << 16) | (bg << 8) | bg;
                        } else {
                            bgraPixels[idx] = (c.a << 24) | (c.r << 16) | (c.g << 8) | c.b;
                        }
                    }
                }

                // Render crisp nearest-neighbor sprite
                SetStretchBltMode(hdcMem, COLORONCOLOR);
                StretchDIBits(
                    hdcMem,
                    offsetX, offsetY, spriteW, spriteH,
                    0, 0, (int)g_matrix.width, (int)g_matrix.height,
                    bgraPixels.data(),
                    &bmi,
                    DIB_RGB_COLORS,
                    SRCCOPY
                );

                // 3. Pixel Grid Overlay
                if (g_showGrid && g_currentScale >= 4) {
                    HPEN hPenGrid = CreatePen(PS_SOLID, 1, RGB(65, 70, 95));
                    HPEN hPenOld = (HPEN)SelectObject(hdcMem, hPenGrid);

                    for (size_t x = 0; x <= g_matrix.width; ++x) {
                        int gx = offsetX + (int)x * g_currentScale;
                        MoveToEx(hdcMem, gx, offsetY, NULL);
                        LineTo(hdcMem, gx, offsetY + spriteH);
                    }
                    for (size_t y = 0; y <= g_matrix.height; ++y) {
                        int gy = offsetY + (int)y * g_currentScale;
                        MoveToEx(hdcMem, offsetX, gy, NULL);
                        LineTo(hdcMem, offsetX + spriteW, gy);
                    }

                    SelectObject(hdcMem, hPenOld);
                    DeleteObject(hPenGrid);
                }

                // 4. Subtle Border around sprite
                HPEN hPenBorder = CreatePen(PS_SOLID, 1, RGB(99, 155, 255));
                HPEN hOldPen = (HPEN)SelectObject(hdcMem, hPenBorder);
                HBRUSH hNullBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
                HBRUSH hOldBrush = (HBRUSH)SelectObject(hdcMem, hNullBrush);
                Rectangle(hdcMem, offsetX - 1, offsetY - 1, offsetX + spriteW + 1, offsetY + spriteH + 1);
                SelectObject(hdcMem, hOldBrush);
                SelectObject(hdcMem, hOldPen);
                DeleteObject(hPenBorder);

            } else {
                // Empty or Error state message
                SetBkMode(hdcMem, TRANSPARENT);
                SelectObject(hdcMem, g_hFontUIBold);
                SetTextColor(hdcMem, RGB(120, 125, 145));

                std::string msg = g_matrixValid ? "Cole uma matriz para visualizar" : ("Erro: " + g_parseError);
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

            // Draw swatches
            SelectObject(hdcMem, g_hFontSmall);

            for (uint8_t i = 0; i < spritex::tools::Palette32::SIZE; ++i) {
                int sx = 4 + (int)i * 34;
                int sy = 24;
                RECT rSwatch = { sx, sy, sx + 32, sy + 36 };

                auto c = spritex::tools::Palette32::get(i);

                if (c.a == 0) {
                    // Transparent checkerboard
                    HBRUSH hB1 = CreateSolidBrush(RGB(60, 60, 60));
                    HBRUSH hB2 = CreateSolidBrush(RGB(100, 100, 100));
                    RECT r1 = { sx, sy, sx + 16, sy + 18 };
                    RECT r2 = { sx + 16, sy, sx + 32, sy + 18 };
                    RECT r3 = { sx, sy + 18, sx + 16, sy + 36 };
                    RECT r4 = { sx + 16, sy + 18, sx + 32, sy + 36 };
                    FillRect(hdcMem, &r1, hB1);
                    FillRect(hdcMem, &r2, hB2);
                    FillRect(hdcMem, &r3, hB2);
                    FillRect(hdcMem, &r4, hB1);
                    DeleteObject(hB1);
                    DeleteObject(hB2);
                } else {
                    HBRUSH hBr = CreateSolidBrush(RGB(c.r, c.g, c.b));
                    FillRect(hdcMem, &rSwatch, hBr);
                    DeleteObject(hBr);
                }

                // Swatch border
                COLORREF borderCol = (i == g_hoverSwatch) ? RGB(255, 255, 255) : RGB(40, 44, 60);
                HPEN hPen = CreatePen(PS_SOLID, (i == g_hoverSwatch ? 2 : 1), borderCol);
                HPEN hOldPen = (HPEN)SelectObject(hdcMem, hPen);
                HBRUSH hNull = (HBRUSH)GetStockObject(NULL_BRUSH);
                HBRUSH hOldBr = (HBRUSH)SelectObject(hdcMem, hNull);
                Rectangle(hdcMem, rSwatch.left, rSwatch.top, rSwatch.right, rSwatch.bottom);
                SelectObject(hdcMem, hOldBr);
                SelectObject(hdcMem, hOldPen);
                DeleteObject(hPen);

                // Token Number text
                std::string num = std::to_string(i);
                int luminance = (c.r * 299 + c.g * 587 + c.b * 114) / 1000;
                SetTextColor(hdcMem, (c.a > 0 && luminance > 130) ? RGB(0, 0, 0) : RGB(255, 255, 255));
                DrawTextA(hdcMem, num.c_str(), -1, &rSwatch, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
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

    size_t dataSize = outW * outH * 4;
    HGLOBAL hMem = GlobalAlloc(GHND, sizeof(BITMAPINFOHEADER) + dataSize);
    if (!hMem) return;

    uint8_t* pMem = (uint8_t*)GlobalLock(hMem);
    memcpy(pMem, &bmi.bmiHeader, sizeof(BITMAPINFOHEADER));
    uint8_t* pPixels = pMem + sizeof(BITMAPINFOHEADER);

    for (int y = 0; y < outH; ++y) {
        int srcY = (int)g_matrix.height - 1 - (y / scale);
        for (int x = 0; x < outW; ++x) {
            int srcX = x / scale;
            uint8_t token = g_matrix.at(srcX, srcY);
            auto c = spritex::tools::Palette32::get(token);
            size_t idx = (y * outW + x) * 4;

            if (c.a == 0) {
                // Background dark for transparent pixels
                pPixels[idx + 0] = 30;
                pPixels[idx + 1] = 33;
                pPixels[idx + 2] = 46;
                pPixels[idx + 3] = 255;
            } else {
                pPixels[idx + 0] = c.b;
                pPixels[idx + 1] = c.g;
                pPixels[idx + 2] = c.r;
                pPixels[idx + 3] = c.a;
            }
        }
    }
    GlobalUnlock(hMem);

    if (OpenClipboard(g_hWndMain)) {
        EmptyClipboard();
        SetClipboardData(CF_DIB, hMem);
        CloseClipboard();
        MessageBoxA(g_hWndMain, "Imagem copiada para a Area de Transferencia!\nVoce ja pode colar diretamente no Discord, Photoshop, Paint, etc.", "Copiado", MB_ICONINFORMATION);
    }
}

} // namespace
