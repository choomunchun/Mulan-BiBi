#include <Windows.h>
#include <gl/GL.h>
#include <gl/GLU.h>
#include <cmath>

#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "Glu32.lib")

#define WINDOW_TITLE "Phase 1: Legacy OpenGL Demo (Orbit Camera + Cube)"

// --------------------- 全局状态 ---------------------
int   gWidth = 800;
int   gHeight = 600;

bool  gRunning = true;
bool  gWireframe = false;

bool  gLMBDown = false;
POINT gLastMouse = { 0, 0 };

// 轨迹球相机参数（右手系）
struct Vec3 { float x, y, z; };
Vec3  gTarget = { 0.0f, 0.8f, 0.0f }; // 目标点
float gYaw = 0.0f;               // 水平角（弧度）
float gPitch = 0.3f;               // 俯仰角（弧度），限制在 (-89°, 89°)
float gDist = 5.0f;               // 眼到目标距离

// 键盘连续按压
bool keyW = false, keyS = false, keyA = false, keyD = false;

// 定时（平滑移动）
LARGE_INTEGER gFreq = { 0 }, gPrev = { 0 };

// --------------------- 前置声明 ---------------------
LRESULT WINAPI WindowProcedure(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
bool initPixelFormat(HDC hdc);
void display();
void updateCameraByKeys(float dt);
void computeEye(Vec3& eye);
void drawAxes(float len = 2.0f);
void drawUnitCube();

// --------------------- WinMain ---------------------
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nCmdShow)
{
    WNDCLASSEX wc{};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = WindowProcedure;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = WINDOW_TITLE;

    if (!RegisterClassEx(&wc)) return 0;

    HWND hWnd = CreateWindow(
        WINDOW_TITLE, WINDOW_TITLE, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, gWidth, gHeight,
        NULL, NULL, wc.hInstance, NULL
    );
    if (!hWnd) return 0;

    HDC hdc = GetDC(hWnd);
    if (!initPixelFormat(hdc)) return 0;

    HGLRC rc = wglCreateContext(hdc);
    if (!wglMakeCurrent(hdc, rc)) return 0;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    // 初始 GL 状态
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);

    // 定时器
    QueryPerformanceFrequency(&gFreq);
    QueryPerformanceCounter(&gPrev);

    MSG msg{};
    while (gRunning)
    {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT) gRunning = false;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // 计算 dt
        LARGE_INTEGER now; QueryPerformanceCounter(&now);
        float dt = float(now.QuadPart - gPrev.QuadPart) / float(gFreq.QuadPart);
        gPrev = now;

        updateCameraByKeys(dt);

        display();
        SwapBuffers(hdc);
    }

    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(rc);
    ReleaseDC(hWnd, hdc);
    UnregisterClass(WINDOW_TITLE, wc.hInstance);
    return 0;
}

// --------------------- 窗口过程 ---------------------
LRESULT WINAPI WindowProcedure(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_SIZE:
        gWidth = LOWORD(lParam);
        gHeight = HIWORD(lParam);
        if (gWidth <= 0)  gWidth = 1;
        if (gHeight <= 0) gHeight = 1;
        glViewport(0, 0, gWidth, gHeight);
        return 0;

    case WM_LBUTTONDOWN:
        gLMBDown = true;
        SetCapture(hWnd);
        gLastMouse.x = LOWORD(lParam);
        gLastMouse.y = HIWORD(lParam);
        return 0;

    case WM_LBUTTONUP:
        gLMBDown = false;
        ReleaseCapture();
        return 0;

    case WM_MOUSEMOVE:
        if (gLMBDown)
        {
            int mx = LOWORD(lParam);
            int my = HIWORD(lParam);
            int dx = mx - gLastMouse.x;
            int dy = my - gLastMouse.y;
            gLastMouse.x = mx;
            gLastMouse.y = my;

            const float sens = 0.005f; // 鼠标灵敏度
            gYaw += dx * sens;
            gPitch -= dy * sens;

            // 限制俯仰角，避免翻折
            const float limit = 1.55334f; // ~89度
            if (gPitch > limit) gPitch = limit;
            if (gPitch < -limit) gPitch = -limit;
        }
        return 0;

    case WM_MOUSEWHEEL:
    {
        short delta = GET_WHEEL_DELTA_WPARAM(wParam); // 每格 ±120
        float zoomFactor = 1.0f - (delta / 120.0f) * 0.1f; // 每格 ±10%
        gDist *= zoomFactor;
        if (gDist < 1.0f) gDist = 1.0f;
        if (gDist > 50.0f) gDist = 50.0f;
        return 0;
    }

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) PostQuitMessage(0);
        else if (wParam == 'R') { gYaw = 0.0f; gPitch = 0.3f; gDist = 5.0f; gTarget = { 0.0f,0.8f,0.0f }; }
        else if (wParam == VK_F1) { gWireframe = !gWireframe; glPolygonMode(GL_FRONT_AND_BACK, gWireframe ? GL_LINE : GL_FILL); }
        else if (wParam == 'W')  keyW = true;
        else if (wParam == 'S')  keyS = true;
        else if (wParam == 'A')  keyA = true;
        else if (wParam == 'D')  keyD = true;
        return 0;

    case WM_KEYUP:
        if (wParam == 'W') keyW = false;
        else if (wParam == 'S') keyS = false;
        else if (wParam == 'A') keyA = false;
        else if (wParam == 'D') keyD = false;
        return 0;

    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
}

// --------------------- OpenGL 像素格式 ---------------------
bool initPixelFormat(HDC hdc)
{
    PIXELFORMATDESCRIPTOR pfd{};
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cAlphaBits = 8;
    pfd.cDepthBits = 24;
    pfd.iLayerType = PFD_MAIN_PLANE;

    int pf = ChoosePixelFormat(hdc, &pfd);
    if (pf == 0) return false;
    return SetPixelFormat(hdc, pf, &pfd) == TRUE;
}

// --------------------- 相机与输入 ---------------------
void updateCameraByKeys(float dt)
{
    // 由当前 yaw/pitch 计算“视线前向”与“右向”
    Vec3 eye; computeEye(eye);
    Vec3 forward = { gTarget.x - eye.x, gTarget.y - eye.y, gTarget.z - eye.z };
    // 归一化
    float fl = std::sqrt(forward.x * forward.x + forward.y * forward.y + forward.z * forward.z);
    if (fl > 1e-6f) { forward.x /= fl; forward.y /= fl; forward.z /= fl; }
    // 右向 = forward × worldUp
    Vec3 worldUp = { 0.0f, 1.0f, 0.0f };
    Vec3 right = {
        forward.y * worldUp.z - forward.z * worldUp.y,
        forward.z * worldUp.x - forward.x * worldUp.z,
        forward.x * worldUp.y - forward.y * worldUp.x
    };
    float rl = std::sqrt(right.x * right.x + right.y * right.y + right.z * right.z);
    if (rl > 1e-6f) { right.x /= rl; right.y /= rl; right.z /= rl; }

    // 平移速度与缩放相关
    float speed = gDist * 0.8f; // 基础速率
    float step = speed * dt;

    if (keyW) { gTarget.x += forward.x * step; gTarget.y += forward.y * step; gTarget.z += forward.z * step; }
    if (keyS) { gTarget.x -= forward.x * step; gTarget.y -= forward.y * step; gTarget.z -= forward.z * step; }
    if (keyA) { gTarget.x -= right.x * step;   gTarget.y -= right.y * step;   gTarget.z -= right.z * step; }
    if (keyD) { gTarget.x += right.x * step;   gTarget.y += right.y * step;   gTarget.z += right.z * step; }
}

void computeEye(Vec3& eye)
{
    // 球坐标换算到世界坐标（右手系，Z 轴指向屏幕外）
    float cp = std::cos(gPitch), sp = std::sin(gPitch);
    float sy = std::sin(gYaw), cy = std::cos(gYaw);

    eye.x = gTarget.x + gDist * cp * sy;
    eye.y = gTarget.y + gDist * sp;
    eye.z = gTarget.z + gDist * cp * cy; // yaw=0 时在 +Z 方向
}

// --------------------- 绘制 ---------------------
void display()
{
    glClearColor(0.05f, 0.07f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // --- 投影 ---
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    double aspect = (gHeight > 0) ? (double)gWidth / (double)gHeight : 1.3333;
    gluPerspective(45.0, aspect, 0.1, 100.0);

    // --- 视图 ---
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    Vec3 eye; computeEye(eye);
    gluLookAt(eye.x, eye.y, eye.z,
        gTarget.x, gTarget.y, gTarget.z,
        0.0, 1.0, 0.0);

    // --- 坐标轴 ---
    drawAxes(2.5f);

    // --- 模型矩阵（演示用：轻微自转，可按需移除） ---
    glPushMatrix();
    glTranslatef(0.0f, 0.8f, 0.0f); // 让立方体稍微离地
    static float autoRot = 0.0f;
    autoRot += 0.2f;
    if (autoRot > 360.0f) autoRot -= 360.0f;
    glRotatef(autoRot, 0.0f, 1.0f, 0.0f);

    drawUnitCube();

    glPopMatrix();
}

void drawAxes(float len)
{
    glDisable(GL_LIGHTING);
    glBegin(GL_LINES);
    // X - 红
    glColor3f(1, 0, 0); glVertex3f(0, 0, 0); glVertex3f(len, 0, 0);
    // Y - 绿
    glColor3f(0, 1, 0); glVertex3f(0, 0, 0); glVertex3f(0, len, 0);
    // Z - 蓝
    glColor3f(0, 0, 1); glVertex3f(0, 0, 0); glVertex3f(0, 0, len);
    glEnd();
}

void drawUnitCube()
{
    // 以原点为中心的 1x1x1 立方体（边长 1），这里放大到 1.6
    const float s = 0.8f;

    glBegin(GL_QUADS);
    // 前面 (Z+)
    glColor3f(0.9f, 0.3f, 0.3f);
    glVertex3f(-s, -s, s); glVertex3f(s, -s, s); glVertex3f(s, s, s); glVertex3f(-s, s, s);
    // 后面 (Z-)
    glColor3f(0.3f, 0.3f, 0.9f);
    glVertex3f(-s, -s, -s); glVertex3f(-s, s, -s); glVertex3f(s, s, -s); glVertex3f(s, -s, -s);
    // 左面 (X-)
    glColor3f(0.3f, 0.9f, 0.3f);
    glVertex3f(-s, -s, -s); glVertex3f(-s, -s, s); glVertex3f(-s, s, s); glVertex3f(-s, s, -s);
    // 右面 (X+)
    glColor3f(0.9f, 0.9f, 0.3f);
    glVertex3f(s, -s, -s); glVertex3f(s, s, -s); glVertex3f(s, s, s); glVertex3f(s, -s, s);
    // 上面 (Y+)
    glColor3f(0.3f, 0.9f, 0.9f);
    glVertex3f(-s, s, s); glVertex3f(s, s, s); glVertex3f(s, s, -s); glVertex3f(-s, s, -s);
    // 下面 (Y-)
    glColor3f(0.9f, 0.3f, 0.9f);
    glVertex3f(-s, -s, s); glVertex3f(-s, -s, -s); glVertex3f(s, -s, -s); glVertex3f(s, -s, s);
    glEnd();
}
