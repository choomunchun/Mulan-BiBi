#include <Windows.h>
#include <gl/GL.h>
#include <gl/GLU.h>
#define _USE_MATH_DEFINES
#include <cmath>

#include "spear.h"
#include "shield.h"

#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "Glu32.lib")

#define WINDOW_TITLE "Spear and Shield"

// Global State
int   gWidth = 800;
int   gHeight = 600;
bool  gRunning = true;
bool  gShowWireframe = false;
bool  gLMBDown = false;
POINT gLastMouse = { 0, 0 };
float gYaw = -0.5f;
float gPitch = 0.5f;
float gDist = 15.0f;

// Forward Declarations for main file functions
LRESULT WINAPI WindowProcedure(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
void display();

// Main Application
bool initPixelFormat(HDC hdc)
{
    PIXELFORMATDESCRIPTOR pfd{};
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR); pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA; pfd.cColorBits = 32; pfd.cDepthBits = 24;
    pfd.iLayerType = PFD_MAIN_PLANE;
    int pf = ChoosePixelFormat(hdc, &pfd);
    return (pf != 0 && SetPixelFormat(hdc, pf, &pfd));
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    WNDCLASSEX wc{};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = WindowProcedure;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = WINDOW_TITLE;
    if (!RegisterClassEx(&wc)) return 0;

    HWND hWnd = CreateWindow(WINDOW_TITLE, WINDOW_TITLE, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, gWidth, gHeight,
        NULL, NULL, wc.hInstance, NULL);

    HDC hdc = GetDC(hWnd);
    if (!initPixelFormat(hdc)) return 0;

    HGLRC rc = wglCreateContext(hdc);
    if (!wglMakeCurrent(hdc, rc)) return 0;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);

    MSG msg{};
    while (gRunning)
    {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT) gRunning = false;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        display();
        SwapBuffers(hdc);
    }

    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(rc);
    ReleaseDC(hWnd, hdc);
    UnregisterClass(WINDOW_TITLE, wc.hInstance);
    return 0;
}

LRESULT WINAPI WindowProcedure(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_DESTROY: PostQuitMessage(0); return 0;
    case WM_SIZE:
        gWidth = LOWORD(lParam); gHeight = HIWORD(lParam);
        glViewport(0, 0, gWidth, gHeight);
        return 0;
    case WM_LBUTTONDOWN:
        gLMBDown = true; SetCapture(hWnd);
        gLastMouse.x = LOWORD(lParam); gLastMouse.y = HIWORD(lParam);
        return 0;
    case WM_LBUTTONUP:
        gLMBDown = false; ReleaseCapture(); return 0;
    case WM_MOUSEMOVE:
        if (gLMBDown) {
            int mx = LOWORD(lParam), my = HIWORD(lParam);
            int dx = mx - gLastMouse.x, dy = my - gLastMouse.y;
            gLastMouse.x = mx; gLastMouse.y = my;
            gYaw += dx * 0.005f; gPitch -= dy * 0.005f;
            if (gPitch > 1.55f) gPitch = 1.55f; if (gPitch < -1.55f) gPitch = -1.55f;
        }
        return 0;
    case WM_MOUSEWHEEL: {
        short delta = GET_WHEEL_DELTA_WPARAM(wParam);
        gDist *= (1.0f - (delta / 120.0f) * 0.1f);
        if (gDist < 2.0f) gDist = 2.0f; if (gDist > 50.0f) gDist = 50.0f;
        return 0;
    }
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) PostQuitMessage(0);
        else if (wParam == '1') { gShowWireframe = !gShowWireframe; }
        return 0;
    default: return DefWindowProc(hWnd, msg, wParam, lParam);
    }
}

void display()
{
    glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (gHeight > 0) ? (double)gWidth / gHeight : 1.0, 0.1, 100.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    float camX = gDist * cos(gPitch) * sin(gYaw);
    float camY = gDist * sin(gPitch);
    float camZ = gDist * cos(gPitch) * cos(gYaw);
    gluLookAt(camX, camY, camZ, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);

    glPolygonMode(GL_FRONT_AND_BACK, gShowWireframe ? GL_LINE : GL_FILL);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    float lightPos[] = { 10.f, 10.f, 15.f, 1.f };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

    // --- Draw Spear ---
    glPushMatrix();
    glTranslatef(2.0f, -5.0f, 0.0f);
    drawSpear();
    glPopMatrix();

    // --- Draw Shield ---
    glPushMatrix();
    glTranslatef(-2.5f, 0.0f, 0.0f);
    glRotatef(90, 0.0f, 1.0f, 0.0f);
    glRotatef(-15, 1.0f, 0.0f, 0.0f);
    drawShield();
    glPopMatrix();

    glDisable(GL_LIGHTING);
}
