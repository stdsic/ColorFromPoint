// #define _DEBUG
#define _WIN32_WINNT 0x0A00
#include <windows.h>
#include <winspool.h>
#include <strsafe.h>
#include <math.h>
#include <lcms2.h>      // LittleCMS
#include <icm.h>        // WCS API          Mingw 환경에서는 제대로 동작하지 않는다. 최신 버전이 아니라 구조체 정의가 다르고 함수 시그니처가 변경되었다.
#include "Color.h"
#define CLASS_NAME		L"ColorFromPoint"
#define WM_CHANGEFOCUS	WM_USER+1
#define WM_MOUSEHOOK	WM_USER+321
#define WM_KEYBOARDHOOK	WM_USER+123
#define min(a,b)		(((a) < (b)) ? (a) : (b))
#define max(a,b)		(((a) < (b)) ? (b) : (a))
#define abs(a)			(((a) < 0) ? -(a) : (a))

#define EDGEFRAME		2
#define IDC_EDSTART		1025
#define IDC_LBSTART		2049
#define IDM_PROGRAM		4097
#define IDM_LINE		4098

#define OBSOLETE        0

// Update 25. 08.04
// Windows 디스플레이는 기본적으로 sRGB 색 공간을 사용한다.
// GetPixel을 이용해서 가져온 RGB값은 이미 디스플레이 드라이버와 OS가 감마 보정한 결과일 가능성이 높다.
// 따라서 GetPixel로 얻은 COLORREF 값은 sRGB 감마가 적용된 RGB 값으로 간주하는 것이 일반적이다.
// sRGB는 인쇄 전용 CMYK 변환 시스템과 호환성이 높은데, 일반적으로 상업용 프린터 드라이버나 인쇄 RIP(Raster Image Processor)들은 대부분 sRGB를 RGB 입력 표준으로 삼는다.
// Photoshop이나 Illustrator 같은 디자인 툴들도 CMYK 출력 시 내부적으로 sRGB를 기준으로 색상 의도를 계산한다.
// ICC 프로파일, 즉 프린터 프로파일도 대부분 sRGB 기준으로 설계되므로 선형 RGB 값은 직접 사용하는 것이 아니라 중간 과정에서만 사용된다.

// Update 25.08.18
// ICC 프로파일의 역할을 좀 더 알아보니 sRGB -> CMYK 변환 간 감마 보정 포함 여부와 잉크 특성, 종이 반사율 등을 모두 고려한다고 한다.
// 또, sRGB 값을 그대로 받아서 선형 RGB로 변환 후, 다시 CMYK로 변환한다고 한다.
// 잉크 혼합 기반의 색상 모델이라 선형적인 빛의 강도를 기준으로 계산해야 정확한 결과가 나온다.
// 실제 프린터에 출력될 색상값을 조사한다.
// CMYK16이나 CMYK_DBL로 변환하는 것도 가능하나, 굉장히 느려진다.
// 이 함수는 추후 기능 확장시 활용하기로 한다.

// Update 26.01.31
// ICC는 표준이 존재하지만 네트워크 참조 모델과 같이 참조일 뿐 장치마다 전부 다르다.
// 여기서 사용된 CMY 변환 공식도 아주 오래된 것이라 분명한 한계가 존재한다.
// 파란색(0,0,255)을 CMYK로 변환해보면 Magenta 값이 이상하다는 것을 알 수 있다.
// 유난히 높은 값(9x.xx)으로 조사되는데 이는 CMY 변환 공식의 한계이며,
// 이를 해결하려면 시스템에 설치된 장치와 icc 프로파일을 직접 조사하여 가져온 후 LittleCMS, WCS API 등을 이용해 적용해야 한다.
// 이는 추후 시간이 생기면 추가하기로 하고 당장은 CMYK 관련 함수와 코드를 사용하지 않기로 한다.

// Update 26.02.01
// 이전 프로젝트에서 만들어 둔 Color 클래스를 추가했다.
// RGB와 HSV간 변환을 수행하는 클래스이며 변환 생성자와 전역 연산자 오버로딩을 이용해 캡슐화했다.

// Update 26.02.01
// 웹까지 고려하여 HSL 색 공간도 추가하기로 결정했다.
// CSS에서 기본적으로 HSLA 색상 모델을 사용하기 때문에 확장한 김에 같이 해두는 것이 좋을 것 같다.
// HSL은 명도(L)를 기준으로 색을 표현함으로써 채도를 더 자연스럽게 조절하는 모델이다.
// 주로 웹에서 사용되며 HSV 모델과는 약간의 차이가 있다.
// HSV는 그래픽 툴(Photoshop)이나 게임(조명/광원 색상 변환, 그라데이션), 컴퓨터 비전(OpenCV) 등에서 쓰이는데
// HSV의 명도(V)는 일반적으로 생각하는 밝기(빛의 밝기)가 아니라 색의 밝기(색의 강도)를 나타낸다.
// 반면 HSL의 명도(L)는 우리가 일반적으로 생각하는 밝기(빛의 밝기)를 나타낸다고 볼 수 있다.
// OpenCV를 써본 사람은 특히 이해하기 쉬울 것이다.
// HSV에서 밝기를 조절하는 경우 밝아진다는 느낌보다는 색이 강렬해진다는 느낌을 준다.
// 애초에 그렇게 설계된 모델이지만 이로인해 톤 변화가 부자연스러워 약간의 괴리감이 있다.
// 이를 해결한 모델이 HSL이라고 생각하면 되며 명도(L)를 기준으로하여 채도(S, 색의 순도)를 결정한다.
// 변환식을 보면 색의 범위(Delta)를 명도(L)에 맞춰 보정한 값을 채도(S)로 쓴다는 것을 알 수 있다.

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK EditProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);

POINT GetWindowCenter(HWND hWnd);
BOOL SetWindowCenter(HWND hParent, HWND hWnd, LPRECT lpRect);
void GetRealDpi(HMONITOR hMonitor, float *XScale, float *YScale);
COLORREF GetAverageColor(HDC hdc, int x, int y, int rad);
bool IsColorDark(COLORREF color);
BOOL DrawBitmap(HDC hdc, int x, int y, HBITMAP hBitmap);
void ErrorMessage(LPCTSTR msg, ...);
void DebugMessage(LPCWSTR fmt, ...);

int APIENTRY wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int nCmdShow){
	HANDLE hMutex;
	hMutex = CreateMutex(NULL, FALSE, L"MyColorFromPointMutex");

	if(GetLastError() == ERROR_ALREADY_EXISTS){
		CloseHandle(hMutex);
		HWND hOnce = FindWindow(CLASS_NAME, NULL);
		if(hOnce){
			ShowWindowAsync(hOnce, SW_SHOWNORMAL);
			SetForegroundWindow(hOnce);
		}
		return 0;
	}

#ifdef _DEBUG
    AllocConsole();
#endif

	WNDCLASS wc = {
		CS_HREDRAW | CS_VREDRAW,
		WndProc,
		0,0,
		hInst,
		NULL, LoadCursor(NULL, IDC_ARROW),
		NULL,
		NULL,
		CLASS_NAME
	};
	RegisterClass(&wc);

	DWORD	dwStyle		= WS_OVERLAPPED,
			dwExStyle	= WS_EX_CLIENTEDGE;

	RECT crt;
	SetRect(&crt, 0,0, 500, 400);
	AdjustWindowRectEx(&crt, dwStyle, FALSE, dwExStyle);

	SetWindowCenter(NULL, NULL, &crt);

	HWND hWnd = CreateWindowEx(
			WS_EX_CLIENTEDGE,
			CLASS_NAME,
			CLASS_NAME,
			WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
			crt.left, crt.top, crt.right, crt.bottom,
			NULL,
			(HMENU)NULL,
			hInst,
			NULL
			);

	ShowWindow(hWnd, nCmdShow);

	MSG msg;
	while(GetMessage(&msg, nullptr, 0,0)){
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

    FreeConsole();
	return (int)msg.wParam;
}

typedef struct tag_MyRGB{
	float R;
	float G;
	float B;
}MyRGB;

void ToHexCode(COLORREF color, LPTSTR ret, int Size);
void ToHexCode(int Value, LPTSTR ret, int Size);
void ToHSVCode(float Value, LPTSTR ret, int Size);
void ToHSLCode(float Value, LPTSTR ret, int Size);

COLORREF ToCOLORREF(LPCTSTR HexCode);
MyRGB Normalize(COLORREF color);
MyRGB Normalize(int r, int g, int b);

float MyGetKValue(MyRGB rgb);
float LinearToSRGB(float Channel);
float SRGBToLinear(float Channel);

MyRGB ConvertToSRGB(MyRGB rgb);
MyRGB ConvertToLinearRGB(MyRGB srgb);

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam){
	const wchar_t	*wMyDll				= L"MyApiDll.dll",
					*wMyMouseProc		= L"MyMouseProc",
					*wMyKeyboardProc	= L"MyKeyboardProc",
					*wMyUtil			= L"MyInit";
	char			mMyDll[50],
					mMyMouseProc[50],
					mMyKeyboardProc[50],
					mMyUtil[50];
	static HDC		g_hScreenDC			= NULL;
	static RECT		g_rcMagnify			= {0,},
					g_rcRGB				= {0,},
					g_rcHSV			    = {0,},
					g_rcHSL			    = {0,},
					g_rcHex			    = {0,};
	static HDC		g_hMemDC			= NULL,
					g_hDrawMemDC		= NULL,
					g_hScreenMemDC		= NULL,
					g_hCaptureMemDC		= NULL;
	static HBITMAP	g_hBitmap			= NULL,
					g_hDrawBitmap		= NULL,
					g_hScreenBitmap		= NULL,
					g_hMagnifyCaptureBitmap = NULL;
	static HHOOK	g_hMouse			= NULL,
					g_hKeyboard			= NULL;
	static HMODULE	g_hModule			= NULL;
	static HOOKPROC	g_lpfnMouseProc		= NULL,
					g_lpfnKeyboardProc	= NULL;
	static float	g_Rate				= 2.0,
					g_XScale			= 1.0,
					g_YScale			= 1.0;
	static int		g_X					= 0,
					g_Y					= 0,
					g_iRadius			= 0;
	static HMONITOR	g_hCurrentMonitor	= NULL;

	static const int	nEdit			= 12,
						nList			= 1,
						nControls		= nList + nEdit,
						Padding			= 20;

	static HWND		hControls[nControls];
	static WNDPROC	OldEditProc;

	static HBRUSH hRedBrush, hGreenBrush, hBlueBrush, hBlackBrush;
	static COLORREF SelectColor, EllipseColor;
	static POINT Mouse, EllipseOrigin;
	static HPEN hWhitePen, hBlackPen;
	static BOOL bLine;

	void (*pInit)(HWND, HHOOK, HHOOK)	= NULL;

	RECT	crt, wrt, srt;
	BITMAP	bmp;
	DWORD	dwStyle, dwExStyle;

	POINT		Origin;
	COLORREF	color;
	TCHAR		HexCode[6];
	TCHAR		HSVCode[0x10];
	TCHAR		HSLCode[0x10];
	HMONITOR	hCurrentMonitor;
	int x, y, Width, iWidth, Height, iHeight, iRadius, ConvertLength;

	WNDCLASS wc;
	HDC hdc;

	HMENU hMenu, hPopupMenu;

	switch(iMessage){
		case WM_CREATE:
			try{
				g_hModule = LoadLibrary(wMyDll);
				if(g_hModule == NULL){ throw 1; }

				ConvertLength = WideCharToMultiByte(CP_ACP, 0, wMyMouseProc, -1, NULL, 0, NULL, NULL);
				WideCharToMultiByte(CP_ACP, 0, wMyMouseProc, -1, mMyMouseProc, ConvertLength, NULL, NULL);
				g_lpfnMouseProc = (HOOKPROC)GetProcAddress(g_hModule, mMyMouseProc);
				if(g_lpfnMouseProc == NULL){ throw 2; }

				g_hMouse = SetWindowsHookEx(WH_MOUSE_LL, g_lpfnMouseProc,g_hModule, 0);
				if(g_hMouse == NULL){ throw 3; }

				ConvertLength = WideCharToMultiByte(CP_ACP, 0, wMyKeyboardProc, -1, NULL, 0, NULL, NULL);
				WideCharToMultiByte(CP_ACP, 0, wMyKeyboardProc, -1, mMyKeyboardProc, ConvertLength, NULL, NULL);
				g_lpfnKeyboardProc = (HOOKPROC)GetProcAddress(g_hModule, mMyKeyboardProc);
				if(g_lpfnKeyboardProc == NULL){ throw 4; }

				g_hKeyboard = SetWindowsHookEx(WH_KEYBOARD_LL, g_lpfnKeyboardProc,g_hModule, 0);
				if(g_hKeyboard == NULL){ throw 5; }

				ConvertLength = WideCharToMultiByte(CP_ACP, 0, wMyUtil, -1, NULL, 0, NULL, NULL);
				WideCharToMultiByte(CP_ACP, 0, wMyUtil, -1, mMyUtil, ConvertLength, NULL, NULL);
				pInit = (void (*)(HWND, HHOOK, HHOOK))GetProcAddress(g_hModule, mMyUtil);
				if(pInit == NULL){ throw 6; }
				(*pInit)(hWnd, g_hMouse, g_hKeyboard);

			} catch (const int err){
				ErrorMessage(L"Init Failed");
				if(err != 1){
					FreeLibrary(g_hModule);
				}
				return -1;
			}

			SetRect(&g_rcMagnify, 0,0, 100, 100);

			color = ToCOLORREF(L"#c92519");
			hRedBrush = CreateSolidBrush(color);
			color = ToCOLORREF(L"#00A86B");
			hGreenBrush = CreateSolidBrush(color);
			color = ToCOLORREF(L"#0080ff");
			hBlueBrush = CreateSolidBrush(color);
			hBlackBrush = CreateSolidBrush(RGB(54, 69, 79));

			GetClassInfo(NULL, L"edit", &wc);
			wc.hInstance		= GetModuleHandle(NULL);
			wc.lpszClassName	= L"MyEditClass";
			OldEditProc			= wc.lpfnWndProc;
			wc.lpfnWndProc		= (WNDPROC)EditProc;
			RegisterClass(&wc);
			SetProp(hWnd, L"MyEditClassProc", (HANDLE)OldEditProc);
			for(int i=0; i<nEdit; i++){
				hControls[i] = CreateWindowEx(WS_EX_CLIENTEDGE, L"MyEditClass", TEXT(""), WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_RIGHT | ES_READONLY, 0,0,0,0, hWnd, (HMENU)(INT_PTR)(IDC_EDSTART + i), GetModuleHandle(NULL), NULL);
			}

			hControls[nControls - 1]= CreateWindowEx(WS_EX_CLIENTEDGE, L"listbox", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY | LBS_OWNERDRAWFIXED, 0,0,0,0, hWnd, (HMENU)(INT_PTR)IDC_LBSTART, GetModuleHandle(NULL), NULL);

			g_hScreenDC = GetDC(NULL);
			g_hScreenMemDC = CreateCompatibleDC(g_hScreenDC);
			hdc = GetDC(hWnd);
			g_hMemDC = CreateCompatibleDC(hdc);

			hWhitePen = CreatePen(PS_SOLID, 1, RGB(255,255,255));
			hBlackPen = CreatePen(PS_SOLID, 1, RGB(0,0,0));
			ReleaseDC(hWnd, hdc);

			bLine = FALSE;

			hMenu                   = CreateMenu();
			hPopupMenu              = CreatePopupMenu();
			AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)hPopupMenu, L"메뉴(&Menu)");
			AppendMenu(hPopupMenu, MF_STRING, IDM_PROGRAM, L"프로그램 소개");
			AppendMenu(hPopupMenu, MF_STRING | MF_UNCHECKED, IDM_LINE, L"보조선");
			SetMenu(hWnd, hMenu);

			SetTimer(hWnd, 1, 10, NULL);
			return 0;

		case WM_SIZE:
			if(wParam != SIZE_MINIMIZED){
				GetClientRect(hWnd, &crt);
				SetRect(&g_rcMagnify, 0,0, crt.right / 5, crt.bottom / 4);

				Width = 36;
                Height = 24;
				x = Padding * 3  + g_rcMagnify.right * 2;

				y = Padding;
				SetRect(&g_rcRGB, x, y, x + Width, y + Height);
				y = Padding + (g_rcMagnify.bottom - Height) / 2;
				SetRect(&g_rcHSV, x, y, x + Width, y + Height);
				y = Padding + g_rcMagnify.bottom - Height;
				SetRect(&g_rcHSL, x, y, x + Width, y + Height);
				int Gap = (g_rcMagnify.bottom - (Height * 3)) / 2;
				y = Padding + g_rcMagnify.bottom + Gap;
                SetRect(&g_rcHex, x, y, x + Width, y + Height);

				x = g_rcRGB.right + (Padding / 2);
				Width = (crt.right - x - (Padding / 2) * 3) / 3;

				iWidth = (LOWORD(lParam) - (Padding * 2 + g_rcMagnify.right)) / 2;
				iHeight = (HIWORD(lParam) - (y + Height + Padding)) / 2;
				g_iRadius = min(iWidth, iHeight) - Padding;
				EllipseOrigin.x = LOWORD(lParam) - iWidth;
				EllipseOrigin.y = HIWORD(lParam) - iHeight;

                SetRect(&srt, x, y, Width, Height);
                for(int i=0; i<3; i++){
                    y = Padding;
                    SetWindowPos(hControls[i], NULL, x, y, Width, Height, SWP_NOZORDER);

					y = Padding + (g_rcMagnify.bottom - Height) / 2;
                    SetWindowPos(hControls[i + 3], NULL, x, y, Width, Height, SWP_NOZORDER);

					y = Padding + g_rcMagnify.bottom - Height;
                    SetWindowPos(hControls[i + 6], NULL, x, y, Width, Height, SWP_NOZORDER);

                    y = Padding + g_rcMagnify.bottom + Gap;
                    SetWindowPos(hControls[i + 9], NULL, x, y, Width, Height, SWP_NOZORDER);

                    x += Width + (Padding / 2);
                }

				x = Padding;
				y = Padding * 2 + g_rcMagnify.bottom;
				Width = g_rcMagnify.right;
				Height = HIWORD(lParam) - y - Padding;
				SetWindowPos(hControls[nControls-1], NULL, x, y, Width, Height, SWP_NOZORDER);

				if(g_hMagnifyCaptureBitmap != NULL){
					DeleteObject(g_hMagnifyCaptureBitmap);
					g_hMagnifyCaptureBitmap = NULL;
				}

				if(g_hScreenBitmap != NULL){
					DeleteObject(g_hScreenBitmap);
					g_hScreenBitmap = NULL;
				}

				if(g_hDrawBitmap != NULL){
					DeleteObject(g_hDrawBitmap);
					g_hDrawBitmap = NULL;
				}

				if(g_hBitmap != NULL){
					DeleteObject(g_hBitmap);
					g_hBitmap = NULL;
				}

			}
			return 0;

		case WM_SETFOCUS:
            // 복사 가능하다는 것을 표시
			SetFocus(hControls[0]);
			return 0;

		case WM_GETMINMAXINFO:
			{
				LPMINMAXINFO lpmmi = (LPMINMAXINFO)lParam;

				SetRect(&crt, 0,0, 550, 420);
				dwStyle = GetWindowLongPtr(hWnd, GWL_STYLE);
				dwExStyle = GetWindowLongPtr(hWnd, GWL_EXSTYLE);
				AdjustWindowRectEx(&crt, dwStyle, GetMenu(hWnd) != NULL, dwExStyle);
				lpmmi->ptMinTrackSize.x = crt.right;
				lpmmi->ptMinTrackSize.y = crt.bottom;
			}
			return 0;

		case WM_MEASUREITEM:
			{
				LPMEASUREITEMSTRUCT lpmis = (LPMEASUREITEMSTRUCT)lParam;
				lpmis->itemHeight = 16;
			}
			return TRUE;

		case WM_DRAWITEM:
			{
				HBRUSH hBrush, hColorBrush, hColorOldBrush;

				LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
				if(lpdis->itemState & ODS_SELECTED){
					hBrush = GetSysColorBrush(COLOR_HIGHLIGHT);
				}else{
					hBrush = GetSysColorBrush(COLOR_BTNFACE);
				}

				FillRect(lpdis->hDC, &lpdis->rcItem, hBrush);

				color = (COLORREF)lpdis->itemData;
				hColorBrush = CreateSolidBrush(color);
				hColorOldBrush = (HBRUSH)SelectObject(lpdis->hDC, hColorBrush);

				Rectangle(lpdis->hDC, lpdis->rcItem.left + 5, lpdis->rcItem.top + 2, lpdis->rcItem.right - 5, lpdis->rcItem.bottom - 2);
				SelectObject(lpdis->hDC, hColorOldBrush);
				DeleteObject(hColorBrush);
			}
			return TRUE;

		case WM_COMMAND:
			switch(LOWORD(wParam)){
				case IDC_LBSTART:
					switch(HIWORD(wParam)){
						case LBN_SELCHANGE:
							{
								int idx	= SendMessage(hControls[nControls - 1], LB_GETCURSEL, 0,0);
								if(idx == LB_ERR){ return 0; }
								EllipseColor = (COLORREF)SendMessage(hControls[nControls - 1], LB_GETITEMDATA, idx, 0);

								int r = GetRValue(EllipseColor),
									g = GetGValue(EllipseColor),
									b = GetBValue(EllipseColor);

                                #if (OBSOLETE)
                                {
                                    // 26.01.31 Update
                                    // 오래된 CMY 변환 공식을 사용하다 보니 어쩔 수 없이 발생하는 한계인 것으로 보인다.
                                    // 인쇄 색공간(감산혼합: CMYK)은 모니터 색공간과 색을 만드는 방식 자체가 달라 변환에 한계가 있다.
                                    // 현대에는 ICC 표준에 맞게 프린터기 제조업체에서 icm, icc 파일을 제작하여 같이 배포한다.
                                    // 즉, CMYK는 장치에 의존적이고 장치마다 CMYK가 전부 다 다르다
                                    // 때문에, 설계상의 문제가 있던 것이므로 이를 전면 수정하기로 한다.
                                    // 추후 시간이 되면 HSV, HSL 색 공간을 추가로 제공하기로 한다.

                                    TCHAR Percentage[0x10];
                                    memset(Percentage, 0, sizeof(Percentage));
                                    MyCMYK cmyk = ToCMYKFromICC(r,g,b);
                                    StringCbPrintf(Percentage, sizeof(Percentage), L"%.2f", cmyk.C);
                                    SetDlgItemText(hWnd, IDC_EDSTART + 2, Percentage);

                                    StringCbPrintf(Percentage, sizeof(Percentage), L"%.2f", cmyk.M);
                                    SetDlgItemText(hWnd, IDC_EDSTART, Percentage);

                                    StringCbPrintf(Percentage, sizeof(Percentage), L"%.2f", cmyk.Y);
                                    SetDlgItemText(hWnd, IDC_EDSTART+1, Percentage);

                                    StringCbPrintf(Percentage, sizeof(Percentage), L"%.2f", cmyk.K);
                                    SetDlgItemText(hWnd, IDC_EDSTART+9, Percentage);
                                }
                                #endif
								
                                SetDlgItemInt(hWnd, (INT_PTR)(IDC_EDSTART + 0), r, FALSE);
                                SetDlgItemInt(hWnd, (INT_PTR)(IDC_EDSTART + 1), g, FALSE);
                                SetDlgItemInt(hWnd, (INT_PTR)(IDC_EDSTART + 2), b, FALSE);

                                COLORREF cColor = RGB(r,g,b);
                                Color MyColor(cColor);

                                MyColor.ToHSV();
                                memset(HSVCode, 0, sizeof(HSVCode));
                                ToHSVCode(MyColor._H, HSVCode, sizeof(HSVCode));
                                SetDlgItemText(hWnd, (INT_PTR)(IDC_EDSTART + 3), HSVCode); 
                                ToHSVCode(MyColor._S, HSVCode, sizeof(HSVCode));
                                SetDlgItemText(hWnd, (INT_PTR)(IDC_EDSTART + 4), HSVCode); 
                                ToHSVCode(MyColor._V, HSVCode, sizeof(HSVCode));
                                SetDlgItemText(hWnd, (INT_PTR)(IDC_EDSTART + 5), HSVCode); 

                                MyColor.ToHSL();
                                memset(HSVCode, 0, sizeof(HSLCode));
                                ToHSLCode(MyColor._H, HSLCode, sizeof(HSLCode));
                                SetDlgItemText(hWnd, (INT_PTR)(IDC_EDSTART + 6), HSLCode); 
                                ToHSLCode(MyColor._S, HSLCode, sizeof(HSLCode));
                                SetDlgItemText(hWnd, (INT_PTR)(IDC_EDSTART + 7), HSLCode); 
                                ToHSLCode(MyColor._L, HSLCode, sizeof(HSLCode));
                                SetDlgItemText(hWnd, (INT_PTR)(IDC_EDSTART + 8), HSLCode); 

                                memset(HexCode, 0, sizeof(HexCode));
                                ToHexCode(r, HexCode, sizeof(HexCode));
                                SetDlgItemText(hWnd, (INT_PTR)(IDC_EDSTART + 9), HexCode); 
                                ToHexCode(g, HexCode, sizeof(HexCode));
                                SetDlgItemText(hWnd, (INT_PTR)(IDC_EDSTART + 10), HexCode); 
                                ToHexCode(b, HexCode, sizeof(HexCode));
                                SetDlgItemText(hWnd, (INT_PTR)(IDC_EDSTART + 11), HexCode); 

								InvalidateRect(hWnd, NULL, FALSE);
							}
							break;
					}
					break;

				case IDM_PROGRAM:
					MessageBox(hWnd, L"프로그램 소개\r\n\r\n이 프로그램은 색상값 조사에 사용되는 컬러 픽커입니다.\r\n\r\n마우스 커서를 기준으로 일정한 크기의 영역을 캡처하여\r\n이미지 정보를 가져온 후 값을 추출할 색상 위에\r\n마우스 커서를 위치시켜 단축키로 색상을 추출할 수 있습니다.\r\n색상 추출은 픽셀 단위로만 가능합니다.\r\n\r\n단축키\r\n• Ctrl + Alt + 3 : 마우스 주변 영역을 캡처합니다. \r\n• Ctrl + Alt + 4 : 마우스 커서가 위치한 지점의 색상값을 추출합니다.\r\n• Alt + Wheel Up(Down) : 이미지를 확대하거나 축소할 수 있습니다.\r\n\r\nHSV / HSL\r\n위 프로그램에서 HSV / HSL은 [0,1]로 정규화된 범위를 갖습니다.\r\n\r\n• 색상(H) : 360°를 곱하여 색상각을 구할 수 있습니다.\r\n• 채도(S) : 100을 곱하여 백분율 값을 구할 수 있습니다.\r\n• 명도(V/L) : 100을 곱하여 백분율 값을 구할 수 있습니다.\r\n\r\n※ 참고\r\n색상값을 변환할 때 최근 변환한 색상을 리스트에 기록합니다.\r\n리스트에 기록된 색상을 선택하면 타원형 이미지에 색상을 적용하여 보여줍니다.", L"ColorFromPoint", MB_OK);
					break;

				case IDM_LINE:
					bLine = !bLine;
					break;
			}
			return 0;

		case WM_INITMENU:
			if(bLine){
				CheckMenuItem(GetSubMenu((HMENU)wParam, 0), IDM_LINE, MF_BYCOMMAND | MF_CHECKED);
			}else{
				CheckMenuItem(GetSubMenu((HMENU)wParam, 0), IDM_LINE, MF_BYCOMMAND | MF_UNCHECKED);
			}
			return 0;

		case WM_TIMER:
			switch(wParam){
				case 1:
					{
						GetClientRect(hWnd, &crt);
						hdc = GetDC(hWnd);
						if(g_hMemDC == NULL){
							g_hMemDC = CreateCompatibleDC(hdc);
						}

						if(g_hBitmap == NULL){
							g_hBitmap = CreateCompatibleBitmap(hdc, crt.right, crt.bottom);
						}

						HGDIOBJ hOld = SelectObject(g_hMemDC, g_hBitmap);
						FillRect(g_hMemDC, &crt, GetSysColorBrush(COLOR_BTNFACE));

						if(g_hScreenDC == NULL){
							g_hScreenDC = GetDC(NULL);
						}

						if(g_hScreenMemDC == NULL){
							g_hScreenMemDC = CreateCompatibleDC(g_hScreenDC);
						}

						if(g_hScreenBitmap == NULL){
							g_hScreenBitmap = CreateCompatibleBitmap(g_hScreenDC, g_rcMagnify.right, g_rcMagnify.bottom);
						}

						HGDIOBJ hScreenOld = SelectObject(g_hScreenMemDC, g_hScreenBitmap);
						GetObject(g_hScreenBitmap, sizeof(BITMAP), &bmp);
						BitBlt(
								g_hScreenMemDC,
								0, 0, bmp.bmWidth * g_XScale, bmp.bmHeight * g_YScale,
								g_hScreenDC,
								g_X - (bmp.bmWidth / g_Rate / 2), g_Y - (bmp.bmHeight / g_Rate / 2),
								SRCCOPY
							  );

						if(g_hDrawMemDC == NULL){
							g_hDrawMemDC = CreateCompatibleDC(hdc);
						}

						if(g_hDrawBitmap == NULL){
							g_hDrawBitmap = CreateCompatibleBitmap(hdc, g_rcMagnify.right, g_rcMagnify.bottom);
						}

						HGDIOBJ hDrawOld = SelectObject(g_hDrawMemDC, g_hDrawBitmap);
						GetObject(g_hDrawBitmap, sizeof(BITMAP), &bmp);
						SetStretchBltMode(g_hDrawMemDC, HALFTONE);
						StretchBlt(
								g_hDrawMemDC,
								0, 0, bmp.bmWidth * g_XScale, bmp.bmHeight * g_YScale,
								g_hScreenMemDC,
								0, 0, (bmp.bmWidth / g_Rate) * g_XScale, (bmp.bmHeight / g_Rate) * g_YScale,
								SRCCOPY
								);

						iWidth	= bmp.bmWidth;
						iHeight	= bmp.bmHeight;
						iRadius	= 2;

						Origin.x = iWidth / 2;
						Origin.y = iHeight / 2;

						color = GetAverageColor(g_hDrawMemDC, Origin.x, Origin.y, iRadius);

						HPEN hOldPen;
						if(IsColorDark(color)){
							hOldPen	= (HPEN)SelectObject(g_hDrawMemDC, hWhitePen);
						}else{
							hOldPen	= (HPEN)SelectObject(g_hDrawMemDC, hBlackPen);
						}

						SelectColor = GetPixel(g_hDrawMemDC, Origin.x, Origin.y);

						if(bLine){
							MoveToEx(g_hDrawMemDC, 0, Origin.y, NULL);
							LineTo(g_hDrawMemDC, iWidth, Origin.y);
							MoveToEx(g_hDrawMemDC, Origin.x, 0, NULL);
							LineTo(g_hDrawMemDC, Origin.x, iHeight);
						}

						HBRUSH hOldBrush = (HBRUSH)SelectObject(g_hDrawMemDC, (HBRUSH)GetStockObject(NULL_BRUSH));
						Ellipse(g_hDrawMemDC, Origin.x - iRadius, Origin.y - iRadius, Origin.x + iRadius, Origin.y + iRadius);
						SelectObject(g_hDrawMemDC, hOldBrush);
						SelectObject(g_hDrawMemDC, hOldPen);

						SelectObject(g_hDrawMemDC, hDrawOld);
						SelectObject(g_hScreenMemDC, hScreenOld);

						SetRect(&srt, Padding, Padding, Padding + g_rcMagnify.right, Padding + g_rcMagnify.bottom);
						InflateRect(&srt, EDGEFRAME, EDGEFRAME);
						DrawEdge(g_hMemDC, &srt, EDGE_SUNKEN, BF_RECT);

						InflateRect(&srt, -EDGEFRAME, -EDGEFRAME);
						if(g_hDrawBitmap != NULL){
							DrawBitmap(g_hMemDC, srt.left, srt.top, g_hDrawBitmap);
						}

						SetRect(&srt, Padding + srt.right, srt.top, Padding + srt.right + g_rcMagnify.right, srt.bottom);
						InflateRect(&srt, EDGEFRAME, EDGEFRAME);
						DrawEdge(g_hMemDC, &srt, EDGE_SUNKEN, BF_RECT);

						InflateRect(&srt, -EDGEFRAME, -EDGEFRAME);
						if(g_hMagnifyCaptureBitmap != NULL){
							DrawBitmap(g_hMemDC, srt.left, srt.top, g_hMagnifyCaptureBitmap);
						}

						DrawEdge(g_hMemDC, &g_rcRGB, EDGE_SUNKEN, BF_RECT);
						DrawEdge(g_hMemDC, &g_rcHSV, EDGE_SUNKEN, BF_RECT);
						DrawEdge(g_hMemDC, &g_rcHSL, EDGE_SUNKEN, BF_RECT);
						DrawEdge(g_hMemDC, &g_rcHex, EDGE_SUNKEN, BF_RECT);

                        int BkMode = SetBkMode(g_hMemDC, TRANSPARENT);
						CopyRect(&srt, &g_rcRGB);
						InflateRect(&srt, -EDGEFRAME, -EDGEFRAME);
                        DrawText(g_hMemDC, L"RGB", -1, &g_rcRGB, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

						CopyRect(&srt, &g_rcHSV);
						InflateRect(&srt, -EDGEFRAME, -EDGEFRAME);
                        DrawText(g_hMemDC, L"HSV", -1, &g_rcHSV, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

						CopyRect(&srt, &g_rcHSL);
						InflateRect(&srt, -EDGEFRAME, -EDGEFRAME);
                        DrawText(g_hMemDC, L"HSL", -1, &g_rcHSL, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

						CopyRect(&srt, &g_rcHex);
						InflateRect(&srt, -EDGEFRAME, -EDGEFRAME);
                        DrawText(g_hMemDC, L"HEX", -1, &g_rcHex, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

                        SetBkMode(g_hMemDC, BkMode);
						HBRUSH hEllipseBrush = CreateSolidBrush(EllipseColor),
							   hEllipseOldBrush	= (HBRUSH)SelectObject(g_hMemDC, hEllipseBrush);
						Ellipse(g_hMemDC, EllipseOrigin.x - g_iRadius, EllipseOrigin.y - g_iRadius, EllipseOrigin.x + g_iRadius, EllipseOrigin.y + g_iRadius);
						SelectObject(g_hMemDC, hEllipseOldBrush);
						DeleteObject(hEllipseBrush);

						GetObject(g_hBitmap, sizeof(BITMAP), &bmp);
						BitBlt(hdc, 0,0, bmp.bmWidth, bmp.bmHeight, g_hMemDC, 0,0, SRCCOPY);
						SelectObject(g_hMemDC, hOld);
						ReleaseDC(hWnd, hdc);
					}
					break;
			}
			return 0;
		case WM_PAINT:
			{
				PAINTSTRUCT ps;
				hdc = BeginPaint(hWnd, &ps);
				EndPaint(hWnd, &ps);
			}
			return 0;

		case WM_KEYBOARDHOOK:
			{
				KBDLLHOOKSTRUCT *ptr = (KBDLLHOOKSTRUCT*)lParam;

				switch(wParam){
					case WM_KEYUP:
					case WM_KEYDOWN:
						{
							WORD VKCode = ptr->vkCode,
								 KeyFlags = ptr->flags,
								 ScanCode = ptr->scanCode;

							BOOL bExtended,
								 bWasKeyDown,
								 bKeyReleased;

							// 확장 키(Numpad 등) 플래그 있을 시 0xE0이 접두(HIWORD)로 붙는다
							bExtended	= ((KeyFlags&& LLKHF_EXTENDED) == LLKHF_EXTENDED);
							if(bExtended){ ScanCode = MAKEWORD(ScanCode, 0xE0); }
							bWasKeyDown	= !(KeyFlags & LLKHF_UP);

							if(bWasKeyDown){
								switch(VKCode){
									case 0x33:
										if(GetKeyState(VK_CONTROL) & 0x8000 && GetKeyState(VK_MENU) & 0x8000){
											if(g_hMagnifyCaptureBitmap != NULL){
												DeleteObject(g_hMagnifyCaptureBitmap);
												g_hMagnifyCaptureBitmap = NULL;
											}

											hdc = GetDC(hWnd);
											if(g_hCaptureMemDC == NULL){
												g_hCaptureMemDC = CreateCompatibleDC(hdc);
											}

											GetObject(g_hScreenBitmap, sizeof(BITMAP), &bmp);
											g_hMagnifyCaptureBitmap = CreateCompatibleBitmap(hdc, bmp.bmWidth, bmp.bmHeight);
											HGDIOBJ hOld = SelectObject(g_hMemDC, g_hScreenBitmap);
											HGDIOBJ hTempOld = SelectObject(g_hCaptureMemDC, g_hMagnifyCaptureBitmap);

											SetStretchBltMode(g_hCaptureMemDC, HALFTONE);
											StretchBlt(
													g_hCaptureMemDC,
													0, 0, bmp.bmWidth * g_XScale, bmp.bmHeight * g_YScale,
													g_hMemDC,
													0, 0, (bmp.bmWidth / g_Rate) * g_XScale, (bmp.bmHeight / g_Rate) * g_YScale,
													SRCCOPY
													);

											SelectObject(g_hCaptureMemDC, hTempOld);
											SelectObject(g_hMemDC, hOld);
											ReleaseDC(hWnd, hdc);
										}
										break;

									case 0x34:
										if(GetKeyState(VK_CONTROL) & 0x8000 && GetKeyState(VK_MENU) & 0x8000){
											if(g_hDrawBitmap){
												SendMessage(hControls[nControls-1], LB_INSERTSTRING, 0, (LPARAM)SelectColor);
											}
										}
										break;

									default:
										break;
								}
							}
						}
						break;
				}
			}
			InvalidateRect(hWnd, NULL, FALSE);
			return 0;

		case WM_MOUSEHOOK:
			{
				MSLLHOOKSTRUCT*ptr = (MSLLHOOKSTRUCT*)lParam;
				Mouse.x = g_X = (int)(ptr->pt.x);
				Mouse.y = g_Y = (int)(ptr->pt.y);

				SHORT WheelDelta,
					  XButton;

				int	Lines		= 0,
					nScroll		= 0,
					WheelUnit	= 0;
				static int	SumDelta	= 0;

				switch(wParam){
					case WM_MOUSEMOVE:
						hCurrentMonitor = MonitorFromPoint(Mouse, MONITOR_DEFAULTTONEAREST);

						if(g_hCurrentMonitor != hCurrentMonitor){
							g_hCurrentMonitor = hCurrentMonitor;
							GetRealDpi(g_hCurrentMonitor, &g_XScale, &g_YScale);
						}
						break;

					case WM_MOUSEWHEEL:
						if(GetKeyState(VK_MENU) & 0x8000){
							if(g_hScreenBitmap != NULL){
								DeleteObject(g_hScreenBitmap);
								g_hScreenBitmap = NULL;
							}

							if(g_hDrawBitmap != NULL){
								DeleteObject(g_hDrawBitmap);
								g_hDrawBitmap = NULL;
							}

							if(g_hBitmap != NULL){
								DeleteObject(g_hBitmap);
								g_hBitmap = NULL;
							}

							nScroll			= 0;
							WheelDelta		= HIWORD(ptr->mouseData);

							SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &Lines, 0);
							// WHEEL_DELTA(120)
							WheelUnit		= WHEEL_DELTA / Lines;

							SumDelta		+= WheelDelta;
							nScroll			= SumDelta / WheelUnit;

							// 부호 상관없이 나머지 계산
							SumDelta		%= WheelUnit;

							int steps		= abs(nScroll);
							float factor	= 0.1f;
							if(nScroll > 0){
								g_Rate = max(1.f, min(5.f, g_Rate + factor * steps));
							}else{
								g_Rate = max(1.f, min(5.f, g_Rate - factor * steps));
							}
						}
						break;

					case WM_XBUTTONDOWN:
					case WM_XBUTTONUP:
						XButton = HIWORD(ptr->mouseData);
						if(XButton == XBUTTON1){

						}

						if(XButton == XBUTTON2){

						}
						break;

					default:
						break;
				}
			}
			InvalidateRect(hWnd, NULL, FALSE);
			return 0;

		case WM_CHANGEFOCUS:
			{
				HWND hPrevFocus		= (HWND)lParam;
				WPARAM KeyCode		= wParam;

                // 0 : LShift + Tab
                // 1 : Tab
                // 2 : Up
                // 3 : Down
                int Prev = -1,
                    Next = -1;

                for(int i=0; i<nEdit; i++){
                    if(hControls[i] == hPrevFocus){
                        Prev = i;
                        break;
                    }
                }

                switch(KeyCode){
                    case 0:
                        Next = (Prev - 1 + nEdit) % nEdit;
                        break;

                    case 1:
                        Next = (Prev + 1) % nEdit;
                        break;

                    case 2:
                        Next = (Prev - 3 + nEdit) % nEdit;
                        break;

                    case 3:
                        Next = (Prev + 3) % nEdit;
                        break;
                }

                SetFocus(hControls[Next]);
			}
			return 0;

		case WM_DESTROY:
			KillTimer(hWnd, 1);
			if(g_hMagnifyCaptureBitmap){ DeleteObject(g_hMagnifyCaptureBitmap); }
			if(g_hScreenBitmap){ DeleteObject(g_hScreenBitmap); }
			if(g_hDrawBitmap){ DeleteObject(g_hDrawBitmap); }
			if(g_hBitmap){ DeleteObject(g_hBitmap); }
			if(g_hCaptureMemDC){ DeleteDC(g_hCaptureMemDC); }
			if(g_hScreenMemDC){ DeleteDC(g_hScreenMemDC); }
			if(g_hDrawMemDC){ DeleteDC(g_hDrawMemDC); }
			if(g_hMemDC){ DeleteDC(g_hMemDC); }
			if(g_hMouse){ UnhookWindowsHookEx(g_hMouse); }
			if(g_hKeyboard){ UnhookWindowsHookEx(g_hKeyboard); }
			if(g_hModule){ FreeLibrary(g_hModule); }
			if(OldEditProc){
				for(int i=0; i<nEdit; i++){
					SetClassLongPtr(hControls[i], GCLP_WNDPROC, (LONG_PTR)OldEditProc);
				}
			}
			if(GetProp(hWnd, L"MyEditClassProc") != NULL){
				RemoveProp(hWnd, L"MyEditClassProc");
			}
			if(hRedBrush){ DeleteObject(hRedBrush); }
			if(hGreenBrush){ DeleteObject(hGreenBrush); }
			if(hBlueBrush){ DeleteObject(hBlueBrush); }
			if(hBlackBrush){ DeleteObject(hBlackBrush); }
			if(hWhitePen){ DeleteObject(hWhitePen); }
			if(hBlackPen){ DeleteObject(hBlackPen); }
			PostQuitMessage(0);
			return 0;
	}

	return (DefWindowProc(hWnd, iMessage, wParam, lParam));
}

POINT GetWindowCenter(HWND hWnd){
	RECT wrt;
	if(hWnd == NULL){ GetWindowRect(GetDesktopWindow(), &wrt); }
	else{ GetWindowRect(hWnd, &wrt); }

	int iWidth	= wrt.right - wrt.left;
	int iHeight	= wrt.bottom - wrt.top;

	iWidth /= 2;
	iHeight /=2;

	POINT Center = {iWidth, iHeight};

	return Center;
}

BOOL SetWindowCenter(HWND hParent, HWND hWnd, LPRECT lpRect){
	if(lpRect == NULL){ return FALSE; }
	if(hWnd != NULL){ GetWindowRect(hWnd, lpRect); }

	POINT Center = GetWindowCenter(hParent);

	int TargetWndWidth	= lpRect->right - lpRect->left;
	int TargetWndHeight = lpRect->bottom - lpRect->top;

	lpRect->left	= Center.x - (TargetWndWidth / 2);
	lpRect->top		= Center.y - (TargetWndHeight / 2);
	lpRect->right	= TargetWndWidth;
	lpRect->bottom	= TargetWndHeight;

	return TRUE;
}

void GetRealDpi(HMONITOR hMonitor, float *XScale, float *YScale){
	MONITORINFOEX Info = { sizeof(MONITORINFOEX) };
	GetMonitorInfo(hMonitor, &Info);

	DEVMODE DevMode = {.dmSize = sizeof(DEVMODE) };
	EnumDisplaySettings(Info.szDevice, ENUM_CURRENT_SETTINGS, &DevMode);

	RECT rt = Info.rcMonitor;

	float CurrentDpi = GetDpiForSystem() / USER_DEFAULT_SCREEN_DPI;
	*XScale = CurrentDpi / ((rt.right - rt.left) / (float)DevMode.dmPelsWidth);
	*YScale = CurrentDpi / ((rt.bottom - rt.top) / (float)DevMode.dmPelsHeight);
}

COLORREF GetAverageColor(HDC hdc, int x, int y, int rad){
	int	 r  = 0,
		 g	= 0,
		 b	= 0;

	int cnt = 0,
		SampleX[] = {x, x - rad, x + rad},
		SampleY[] = {y, y - rad, y + rad};

	COLORREF color;
	for (int i=0; i<3; i++){
		for (int j=0; j<3; j++){
			color = GetPixel(hdc, SampleX[i], SampleY[j]);
			r += GetRValue(color);
			g += GetGValue(color);
			b += GetBValue(color);
			++cnt;
		}
	}

	r /= cnt;
	g /= cnt;
	b /= cnt;

	return RGB(r, g, b);
}

// 0.5 미만 == 어두운 계열
bool IsColorDark(COLORREF color){
	int  r = GetRValue(color),
		 g = GetGValue(color),
		 b = GetBValue(color);

	// 가중 평균
	double brightness = (r * 0.299f + g * 0.587f + b * 0.114f) / 255.f;

	return brightness < 0.56f;
}

BOOL DrawBitmap(HDC hdc, int x, int y, HBITMAP hBitmap){
	if(hBitmap == NULL){return FALSE;}

	BITMAP	bmp;
	HDC		hMemDC = CreateCompatibleDC(hdc);
	GetObject(hBitmap, sizeof(BITMAP), &bmp);

	HGDIOBJ hOld = SelectObject(hMemDC, hBitmap);
	BitBlt(hdc, x, y, bmp.bmWidth, bmp.bmHeight, hMemDC, 0,0, SRCCOPY);

	SelectObject(hMemDC, hOld);
	DeleteDC(hMemDC);

	return TRUE;
}

void ErrorMessage(LPCTSTR msg, ...){
	LPVOID lpMsgBuf;
	DWORD dw = GetLastError(); 

	if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, NULL) == 0) {
		MessageBox(HWND_DESKTOP, L"DisplayText Error", TEXT("Warning"), MB_OK);
	}

	TCHAR buf[256];
	StringCbPrintf(buf, sizeof(buf), L"[%s(%d)]%s", msg, dw, lpMsgBuf);
	MessageBox(HWND_DESKTOP, (LPCTSTR)buf, L"Error", MB_ICONWARNING | MB_OK);
	LocalFree(lpMsgBuf);
}

void ToHexCode(COLORREF color, LPTSTR ret, int Size){
	StringCbPrintf(ret, Size, L"%02X%02X%02X", GetRValue(color), GetGValue(color), GetBValue(color));
}

void ToHexCode(int Value, LPTSTR ret, int Size){
	StringCbPrintf(ret, Size, L"%02X", Value);
}

void ToHSVCode(float Value, LPTSTR ret, int Size){
    StringCbPrintf(ret, Size, L"%05.4f", Value);
}

void ToHSLCode(float Value, LPTSTR ret, int Size){
    StringCbPrintf(ret, Size, L"%05.4f", Value);
}

COLORREF ToCOLORREF(LPCTSTR HexCode){
	TCHAR* ptr = (TCHAR*)HexCode;
	if(ptr[0] == '#'){ ptr++; }

	int i = 0,
		r = 0,
		g = 0,
		b = 0,
		Value = 0;

	for(ptr; *ptr && i<6; ptr++){
		if(*ptr >= '0' && *ptr <= '9'){
			Value = *ptr -'0';
		}
		if(*ptr >= 'A' && *ptr <= 'F'){
			Value = *ptr -'A' + 10;
		}
		if(*ptr >= 'a' && *ptr <= 'f'){
			Value = *ptr -'a' + 10;
		}

		if(i < 2){
			r = (r << 4) | Value;
		}else if(i <4){
			g = (g << 4) | Value;
		}else{
			b = (b << 4) | Value;
		}

		i++;
	}

	return RGB(r, g, b);
}

MyRGB Normalize(COLORREF color){
	// 0 ~ 1 : Normalization

	MyRGB rgb;
	rgb.R = GetRValue(color) / 255.f;
	rgb.G = GetGValue(color) / 255.f;
	rgb.B = GetBValue(color) / 255.f;
	return rgb;
}

MyRGB Normalize(int r, int g, int b){

	MyRGB rgb;
	rgb.R = max(0.0f, min(1.0f, r / 255.f));
	rgb.G = max(0.0f, min(1.0f, g / 255.f));
	rgb.B = max(0.0f, min(1.0f, b / 255.f));
	return rgb;
}

float LinearToSRGB(float Channel){
    if(Channel <= 0.0031308f){
        return 12.92f * Channel;
    }else{
        return 1.055f * powf(Channel, 1.0f / 2.4f) - 0.055f;
    }
}

MyRGB ConvertToSRGB(MyRGB rgb){
    MyRGB srgb = Normalize(rgb.R, rgb.G, rgb.B);
    srgb.R = LinearToSRGB(rgb.R);
    srgb.G = LinearToSRGB(rgb.G);
    srgb.B = LinearToSRGB(rgb.B);

    return srgb;
}

float SRGBToLinear(float Channel){
    if(Channel <= 0.04045f){
        return Channel / 12.92f;
    }else{
        return powf((Channel + 0.055f) / 1.055f, 2.4f);
    }
}

MyRGB ConvertToLinearRGB(MyRGB srgb){
    MyRGB Linear = Normalize(srgb.R, srgb.G, srgb.B);
    Linear.R = SRGBToLinear(srgb.R);
    Linear.G = SRGBToLinear(srgb.G);
    Linear.B = SRGBToLinear(srgb.B);

    return Linear;
}

LRESULT CALLBACK EditProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam){
	static CREATESTRUCT* cs;
	static WNDPROC OldEditProc;

	if(OldEditProc == NULL){
		OldEditProc = (WNDPROC)GetProp(GetParent(hWnd), L"MyEditClassProc");
	}

	switch(iMessage){
		case WM_LBUTTONDOWN:
			SetFocus(hWnd);
			return 0;

		case WM_SETFOCUS:
			SendMessage(hWnd, EM_SETSEL, 0, -1);
			break;

		case WM_CHAR:
		case WM_KEYUP:
		case WM_KEYDOWN:
			{
				WORD VKCode,
					 KeyFlags,
					 ScanCode,
					 RepeatCount;

				BOOL bExtended,
					 bWasKeyDown,
					 bKeyReleased;

				VKCode		= LOWORD(wParam);
				KeyFlags	= HIWORD(lParam);
				ScanCode	= LOBYTE(KeyFlags);
				bExtended	= ((KeyFlags&& KF_EXTENDED) == KF_EXTENDED);
				if(bExtended){ ScanCode = MAKEWORD(ScanCode, 0xE0); }

				bWasKeyDown = ((KeyFlags & KF_REPEAT) == KF_REPEAT);
				RepeatCount = LOWORD(lParam);
				bKeyReleased = ((KeyFlags & KF_UP) == KF_UP);

				if(!bKeyReleased){
					switch(VKCode){
						case VK_UP:
						case VK_DOWN:
						case VK_TAB:
							if(VKCode == VK_TAB){
								if(GetKeyState(VK_LSHIFT) & 0x8000){
									SendMessage(GetParent(hWnd), WM_CHANGEFOCUS, (WPARAM)0, (LPARAM)hWnd);
								}else{
									SendMessage(GetParent(hWnd), WM_CHANGEFOCUS, (WPARAM)1, (LPARAM)hWnd);
								}
							}else if(VKCode == VK_UP){
								SendMessage(GetParent(hWnd), WM_CHANGEFOCUS, (WPARAM)2, (LPARAM)hWnd);
							}else if(VKCode == VK_DOWN){
								SendMessage(GetParent(hWnd), WM_CHANGEFOCUS, (WPARAM)3, (LPARAM)hWnd);
							}
							return 0;

						default:
							break;
					}
				}
			}
			break;

		case WM_CREATE:
			cs = (CREATESTRUCT*)lParam;
	}

	return CallWindowProc(OldEditProc, hWnd, iMessage, wParam, lParam);
}

void DebugMessage(LPCWSTR fmt, ...){
    HANDLE hInput, hOutput, hError;

    hInput = GetStdHandle(STD_INPUT_HANDLE);
    hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    hError = GetStdHandle(STD_ERROR_HANDLE);

    WCHAR Debug[0x100];
    va_list arg;
    va_start(arg, fmt);
    wvsprintf(Debug, fmt, arg);
    va_end(arg);

    DWORD dwWritten;
    WriteConsole(hOutput, Debug, wcslen(Debug), &dwWritten, NULL);
}

#if (OBSOLETE) 
MyCMY GetCMY(MyRGB rgb, float K);
MyCMYK ToCMYK(int r, int g, int b);
MyCMYK ToCMYKFromICC(int r, int g, int b);
MyRGB ToRGB(MyCMYK cmyk);
COLORREF ToCOLORREF(MyCMYK cmyk);
HBRUSH CreateCMYKBrush(MyCMYK cmyk);
void ToHexCode(MyCMYK cmyk, LPTSTR ret, int Size);

typedef struct tag_MyCMY{
	float C;
	float M;
	float Y;
}MyCMY;

typedef struct tag_MyCMYK{
	float C;
	float M;
	float Y;
	float K;
}MyCMYK;

float MyGetKValue(MyRGB rgb) {
    // K = 1 - max(R',G',B')

    float K = 1.0f - max(rgb.R, max(rgb.G, rgb.B));
    return K;
}

MyCMY GetCMY(MyRGB rgb, float K) {
    // C = (1 - R' - K) / (1 - K)
    // M = (1 - G' - K) / (1 - K)
    // Y = (1 - B' - K) / (1 - K)

    MyCMY cmy = {0,};
    if(K < 1.0f){
        cmy.C = (1.f - rgb.R - K) / (1.f - K);
        cmy.M = (1.f - rgb.G - K) / (1.f - K);
        cmy.Y = (1.f - rgb.B - K) / (1.f - K);
    }else{
        cmy.C = 0.0f;
        cmy.M = 0.0f;
        cmy.Y = 0.0f;
    }

    return cmy;
}


MyCMYK ToCMYK(int r, int g, int b){
    MyRGB rgb = Normalize(r,g,b);
    // 프린터기는 sRGB -> RGB -> XYZ -> CMYK와 같이 네 가지 과정을 통해 CMYK로 변환한다.
    // 일반적인 컬러 픽커는 이를 고려하지 않으나 필요한 경우 윈도우 시스템이 제공하는 WCS API를 활용할 수 있다.

    rgb.R = SRGBToLinear(rgb.R);
    rgb.G = SRGBToLinear(rgb.G);
    rgb.B = SRGBToLinear(rgb.B);

    float K = MyGetKValue(rgb);
    MyCMY cmy = GetCMY(rgb, K);

    MyCMYK cmyk;
    cmyk.C = cmy.C * 100.f;
    cmyk.M = cmy.M * 100.f;
    cmyk.Y = cmy.Y * 100.f;
    cmyk.K = K * 100.f;

    return cmyk;
}

COLORREF ToCOLORREF(MyCMYK cmyk){
    // 0 ~ 1: 정규화 값 확인할 수 있도록 변환 공식 분할
    MyRGB rgb = ToRGB(cmyk);

    int r = (int)(rgb.R * 255.f),
        g = (int)(rgb.G * 255.f),
        b = (int)(rgb.B * 255.f);

    return RGB(r,g,b);
}

MyRGB ToRGB(MyCMYK cmyk){
    MyRGB rgb;

    // C' = C / 100
    // M' = M / 100
    // Y' = Y / 100
    // K' = K / 100
    float C = cmyk.C /  100.f,
          M = cmyk.M /  100.f,
          Y = cmyk.Y /  100.f,
          K = cmyk.K /  100.f;

    // R = (1 - C')(1 - K') * 255
    // G = (1 - C')(1 - K') * 255
    // B = (1 - C')(1 - K') * 255
    rgb.R = (1.f - C) * (1.f - K);
    rgb.G = (1.f - M) * (1.f - K);
    rgb.B = (1.f - Y) * (1.f - K);

    // rgb.R = LinearToSRGB(rgb.R);
    // rgb.G = LinearToSRGB(rgb.G);
    // rgb.B = LinearToSRGB(rgb.B);

    return rgb;
}

HBRUSH CreateCMYKBrush(MyCMYK cmyk){
    COLORREF color = ToCOLORREF(cmyk);
    return CreateSolidBrush(color);
}

void ToHexCode(MyCMYK cmyk, LPTSTR ret, int Size){
    COLORREF color = ToCOLORREF(cmyk);
    StringCbPrintf(ret, Size, L"%02X%02X%02X", GetRValue(color), GetGValue(color), GetBValue(color));
}

MyCMYK ToCMYKFromICC(int r, int g, int b){
    MyCMYK ret = {0,};

    BYTE rgb[3] = {(BYTE)r, (BYTE)g, (BYTE)b};
    BYTE cmyk[4] = {0,};

    cmsHPROFILE rgbProf = cmsCreate_sRGBProfile();
    if(!rgbProf){ return ret; }

    char iccPath[MAX_PATH] = {0,};
    DWORD iccLength = MAX_PATH;
    WCHAR PrinterName[MAX_PATH] = {0,};
    DWORD PrinterNameLength = MAX_PATH;

    BOOL bPrinter = GetDefaultPrinter(PrinterName, &PrinterNameLength);

    if(bPrinter){
        HDC hdc = CreateDC(NULL, PrinterName, NULL, NULL);
        if(hdc){
            GetICMProfileA(hdc, &iccLength, iccPath);
            DeleteDC(hdc);
        }
    }

    cmsHPROFILE cmykProf = NULL;

    if(strlen(iccPath) > 0){
        cmykProf = cmsOpenProfileFromFile(iccPath, "r");
    }else{
        WCHAR WindowsColorDir[MAX_PATH] = {0,};
        DWORD PathLength = MAX_PATH;
        if(GetColorDirectoryW(NULL, WindowsColorDir, &PathLength)){
            char ansiColorDir[MAX_PATH] = {0,};
            char fallbackPath[MAX_PATH] = {0,};
            WideCharToMultiByte(CP_ACP, 0, WindowsColorDir, -1, ansiColorDir, MAX_PATH, NULL, NULL);
            sprintf_s(fallbackPath, MAX_PATH, "%s\\sRGB Color Space Profile.icm", ansiColorDir);
            cmykProf = cmsOpenProfileFromFile(fallbackPath, "r");
        }
    }

    if(!cmykProf){
        cmsCloseProfile(rgbProf);
        return ret;
    }

    cmsColorSpaceSignature sig = cmsGetColorSpace(cmykProf);

    if(sig == cmsSigCmykData){
        cmsHTRANSFORM transform = cmsCreateTransform(rgbProf, TYPE_RGB_8, cmykProf, TYPE_CMYK_8, INTENT_PERCEPTUAL, 0);

        if(!transform){
            // DebugMessage(transform Is Null);
            cmsCloseProfile(rgbProf);
            cmsCloseProfile(cmykProf);
            return ret;
        }

        cmsDoTransform(transform, rgb, cmyk, 1);

        ret.C = cmyk[0] * 100.f / 255.f;
        ret.M = cmyk[1] * 100.f / 255.f;
        ret.Y = cmyk[2] * 100.f / 255.f;
        ret.K = cmyk[3] * 100.f / 255.f;

        cmsDeleteTransform(transform);
        cmsCloseProfile(rgbProf);
        cmsCloseProfile(cmykProf);

        return ret;
    }

    return ToCMYK(r,g,b);
}
#endif
