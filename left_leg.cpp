
#include <Windows.h>
#include <gl/GL.h>
#include <gl/GLU.h>
#include <cmath>
#include <vector>
#include <cstdio>
#include <algorithm> // For std::swap

#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "Glu32.lib")

#define WINDOW_TITLE "Anatomically-Correct Leg Viewer (Final Rebuild)"

// --------------------- 全局状态 ---------------------
int   gWidth = 800;
int   gHeight = 600;

bool  gRunning = true;
bool  gShowWireframe = false;

bool  gLMBDown = false;
POINT gLastMouse = { 0, 0 };

struct Vec3 { float x, y, z; };
Vec3  gTarget = { 0.0f, 4.0f, 0.0f };
float gYaw = 0.2f;
float gPitch = 0.1f;
float gDist = 14.0f;

bool keyW = false, keyS = false, keyA = false, keyD = false;

LARGE_INTEGER gFreq = { 0 }, gPrev = { 0 };

// --------------------- 顶点/面数据 (全新重构) ---------------------
struct Vec3f { float x, y, z; };

// 顶点 (全新、系统化构建)
const Vec3f gFootVertices[] = {
    // Toes (5 distinct toes)
    // Big Toe
    {0.25f, 0.0f, 1.4f}, {0.1f, 0.0f, 1.45f}, {0.1f, 0.25f, 1.45f}, {0.25f, 0.25f, 1.4f}, // 0-3 Tip
    {0.25f, 0.0f, 1.2f}, {0.1f, 0.0f, 1.2f}, {0.1f, 0.3f, 1.2f}, {0.25f, 0.3f, 1.2f}, // 4-7 Knuckle
    // 2nd Toe
    {0.05f, 0.0f, 1.4f}, {-0.1f, 0.0f, 1.35f}, {-0.1f, 0.2f, 1.35f}, {0.05f, 0.2f, 1.4f}, // 8-11 Tip
    {0.05f, 0.0f, 1.15f}, {-0.1f, 0.0f, 1.15f}, {-0.1f, 0.25f, 1.15f}, {0.05f, 0.25f, 1.15f}, // 12-15 Knuckle
    // 3rd Toe
    {-0.15f, 0.0f, 1.3f}, {-0.28f, 0.0f, 1.25f}, {-0.28f, 0.18f, 1.25f}, {-0.15f, 0.18f, 1.3f}, // 16-19 Tip
    {-0.15f, 0.0f, 1.1f}, {-0.28f, 0.0f, 1.1f}, {-0.28f, 0.22f, 1.1f}, {-0.15f, 0.22f, 1.1f}, // 20-23 Knuckle
    // 4th Toe
    {-0.32f, 0.0f, 1.2f}, {-0.42f, 0.0f, 1.1f}, {-0.42f, 0.16f, 1.1f}, {-0.32f, 0.16f, 1.2f}, // 24-27 Tip
    {-0.32f, 0.0f, 1.0f}, {-0.42f, 0.0f, 1.0f}, {-0.42f, 0.2f, 1.0f}, {-0.32f, 0.2f, 1.0f}, // 28-31 Knuckle
    // 5th Toe
    {-0.46f, 0.0f, 1.0f}, {-0.55f, 0.0f, 0.9f}, {-0.55f, 0.14f, 0.9f}, {-0.46f, 0.14f, 1.0f}, // 32-35 Tip
    {-0.46f, 0.0f, 0.8f}, {-0.55f, 0.0f, 0.8f}, {-0.55f, 0.18f, 0.8f}, {-0.46f, 0.18f, 0.8f}, // 36-39 Knuckle

    // Foot Body
    {0.3f, 0.0f, 0.8f}, {0.3f, 0.4f, 0.8f}, // 40-41 (Side of Big Toe)
    {-0.6f, 0.0f, 0.6f}, {-0.6f, 0.3f, 0.6f}, // 42-43 (Side of Pinky)

    // Arch and Ankle Ring
    {0.0f, 0.9f, 0.2f}, {0.5f, 0.8f, 0.1f}, {0.55f, 0.9f, -0.5f}, {0.0f, 0.8f, -0.7f}, {-0.55f, 0.9f, -0.5f}, {-0.5f, 0.8f, 0.1f}, // 44-49
    {0.0f, 0.0f, 0.2f}, {0.4f, 0.0f, -0.5f}, {-0.4f, 0.0f, -0.5f}, // 50-52 (Sole)

    // Leg Rings
    // Ring 4: Lower Calf
    {0.0f, 2.0f, -0.1f}, {-0.6f, 1.9f, -0.3f}, {-0.6f, 2.2f, -0.9f}, {0.0f, 2.3f, -1.0f}, {0.6f, 2.2f, -0.9f}, {0.6f, 1.9f, -0.3f}, // 53-58
    // Ring 5: Upper Calf / Shin
    {0.0f, 3.5f, 0.1f}, {-0.5f, 3.4f, -0.1f}, {-0.55f, 3.8f, -0.7f}, {0.0f, 3.9f, -0.8f}, {0.55f, 3.8f, -0.7f}, {0.5f, 3.4f, -0.1f}, // 59-64
    // Ring 6: Knee
    {0.0f, 4.5f, 0.3f}, {-0.45f, 4.4f, 0.2f}, {-0.5f, 4.5f, -0.6f}, {0.0f, 4.4f, -0.7f}, {0.5f, 4.5f, -0.6f}, {0.45f, 4.4f, 0.2f}, // 65-70
    // Ring 7: Lower Thigh
    {0.0f, 6.0f, 0.2f}, {-0.7f, 5.8f, 0.0f}, {-0.75f, 6.2f, -0.5f}, {0.0f, 6.3f, -0.6f}, {0.75f, 6.2f, -0.5f}, {0.7f, 5.8f, 0.0f}, // 71-76
    // Ring 8: Upper Thigh
    {0.0f, 8.0f, 0.1f}, {-0.7f, 7.9f, -0.1f}, {-0.75f, 8.1f, -0.4f}, {0.0f, 8.2f, -0.5f}, {0.75f, 8.1f, -0.4f}, {0.7f, 7.9f, -0.1f}, // 77-82
};

// 面 (全新、系统化连接)
const int gFootQuads[][4] = {
    // Toes
    {0,1,2,3}, {4,5,1,0}, {7,6,2,3}, {4,0,3,7}, {5,4,7,6}, // Big Toe
    {8,9,10,11}, {12,13,9,8}, {15,14,10,11}, {12,8,11,15}, {13,12,15,14}, // 2nd Toe
    {16,17,18,19}, {20,21,17,16}, {23,22,18,19}, {20,16,19,23}, {21,20,23,22}, // 3rd Toe
    {24,25,26,27}, {28,29,25,24}, {31,30,26,27}, {28,24,27,31}, {29,28,31,30}, // 4th Toe
    {32,33,34,35}, {36,37,33,32}, {39,38,34,35}, {36,32,35,39}, {37,36,39,38}, // 5th Toe

    // Connect Toes to Foot Body
    {4, 40, 41, 7}, {12, 4, 7, 15}, {20, 12, 15, 23}, {28, 20, 23, 31}, {36, 28, 31, 39},
    {39, 36, 42, 43},
    {40, 5, 13, 12}, {5, 4, 12, -1}, // Sole connections
    {13,21,20,12}, {21,29,28,20}, {29,37,36,28}, {37,42,36,-1},
    {5, 50, 13, -1}, {13, 50, 21, -1}, {21, 50, 29, -1}, {29, 50, 37, -1}, {37, 50, 42, -1},


    // Top Foot to Ankle
    {41, 45, 49, 43}, {41, 44, 45, -1},
    {15, 14, 49, 45}, {23, 22, 14, 15}, {31, 30, 22, 23}, {39, 38, 30, 31},
    {43, 38, 39, -1}, {43, 49, 38, -1},

    // Ankle Ring to Leg
    {44, 53, 58, 45}, {45, 58, 52, 46}, {46, 52, 51, 47}, {47, 51, 57, 48}, {48, 57, 54, 49}, {49, 54, 53, 44},
    // Sole to Ankle
    {50, 52, 51, -1}, {40, 41, 45, 46}, {46, 52, 47}, {40, 46, 45, -1},
    {42, 43, 49, 48}, {48, 51, 47, -1}, {42, 48, 49, -1},
    {50, 40, 46, 52}, {50, 42, 48, 51}, {50, 40, 52,-1},{50, 42, 51, -1},


    // --- Leg (same connection logic as before, but with new indices) ---
    {53, 59, 60, 54}, {54, 60, 61, 55}, {55, 61, 62, 56}, {56, 62, 63, 57}, {57, 63, 64, 58}, {58, 64, 59, 53},
    {59, 65, 66, 60}, {60, 66, 67, 61}, {61, 67, 68, 62}, {62, 68, 69, 63}, {63, 69, 70, 64}, {64, 70, 65, 59},
    {65, 71, 72, 66}, {66, 72, 73, 67}, {67, 73, 74, 68}, {68, 74, 75, 69}, {69, 75, 76, 70}, {70, 76, 71, 65},
    {71, 77, 78, 72}, {72, 78, 79, 73}, {73, 79, 80, 74}, {74, 80, 81, 75}, {75, 81, 82, 76}, {76, 82, 77, 71},
    // Thigh Cap
    {82, 81, 80, -1}, {82, 80, 79, -1}, {82, 79, 78, -1}, {82, 78, 77, -1},
};

// --------------------- 三角化 + 法线计算 ---------------------
struct Tri { int a, b, c; };
std::vector<Tri>   gTris;
std::vector<Vec3f> gVertexNormals;
Vec3f gModelCenter{ 0,0,0 };

Vec3f sub(const Vec3f& p, const Vec3f& q) { return { p.x - q.x, p.y - q.y, p.z - q.z }; }
Vec3f cross(const Vec3f& a, const Vec3f& b) {
    return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
}
float dot(const Vec3f& a, const Vec3f& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
Vec3f normalize(const Vec3f& v) {
    float l = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    return (l > 1e-6f) ? Vec3f{ v.x / l, v.y / l, v.z / l } : Vec3f{ 0,0,0 };
}

Vec3f faceNormal(int i0, int i1, int i2) {
    const Vec3f& v0 = gFootVertices[i0];
    const Vec3f& v1 = gFootVertices[i1];
    const Vec3f& v2 = gFootVertices[i2];
    return normalize(cross(sub(v1, v0), sub(v2, v0)));
}

Vec3f triCenter(int i0, int i1, int i2) {
    const Vec3f& v0 = gFootVertices[i0];
    const Vec3f& v1 = gFootVertices[i1];
    const Vec3f& v2 = gFootVertices[i2];
    return { (v0.x + v1.x + v2.x) / 3.0f, (v0.y + v1.y + v2.y) / 3.0f, (v0.z + v1.z + v2.z) / 3.0f };
}

void buildTriangles()
{
    int nV = sizeof(gFootVertices) / sizeof(gFootVertices[0]);
    gModelCenter = { 0,0,0 };
    for (int i = 0; i < nV; ++i) { gModelCenter.x += gFootVertices[i].x; gModelCenter.y += gFootVertices[i].y; gModelCenter.z += gFootVertices[i].z; }
    gModelCenter.x /= nV; gModelCenter.y /= nV; gModelCenter.z /= nV;

    gTris.clear();
    int nF = sizeof(gFootQuads) / sizeof(gFootQuads[0]);
    for (int f = 0; f < nF; ++f) {
        const int* q = gFootQuads[f];
        if (q[3] == -1) {
            int a = q[0], b = q[1], c = q[2];
            Vec3f n = faceNormal(a, b, c);
            Vec3f center = triCenter(a, b, c);
            Vec3f outDir = sub(center, gModelCenter);
            if (dot(n, outDir) < 0) std::swap(b, c);
            gTris.push_back({ a,b,c });
        }
        else {
            int a = q[0], b = q[1], c = q[2], d = q[3];
            {
                int i0 = a, i1 = b, i2 = d;
                Vec3f n = faceNormal(i0, i1, i2);
                Vec3f center = triCenter(i0, i1, i2);
                Vec3f outDir = sub(center, gModelCenter);
                if (dot(n, outDir) < 0) std::swap(i1, i2);
                gTris.push_back({ i0,i1,i2 });
            }
            {
                int i0 = b, i1 = c, i2 = d;
                Vec3f n = faceNormal(i0, i1, i2);
                Vec3f center = triCenter(i0, i1, i2);
                Vec3f outDir = sub(center, gModelCenter);
                if (dot(n, outDir) < 0) std::swap(i1, i2);
                gTris.push_back({ i0,i1,i2 });
            }
        }
    }
}

void computeVertexNormals() {
    int nV = sizeof(gFootVertices) / sizeof(gFootVertices[0]);
    gVertexNormals.assign(nV, { 0.0f, 0.0f, 0.0f });

    for (const auto& t : gTris) {
        Vec3f n = faceNormal(t.a, t.b, t.c);
        gVertexNormals[t.a].x += n.x; gVertexNormals[t.a].y += n.y; gVertexNormals[t.a].z += n.z;
        gVertexNormals[t.b].x += n.x; gVertexNormals[t.b].y += n.y; gVertexNormals[t.b].z += n.z;
        gVertexNormals[t.c].x += n.x; gVertexNormals[t.c].y += n.y; gVertexNormals[t.c].z += n.z;
    }

    for (int i = 0; i < nV; ++i) {
        gVertexNormals[i] = normalize(gVertexNormals[i]);
    }
}

// --------------------- 前置声明 ---------------------
LRESULT WINAPI WindowProcedure(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
bool initPixelFormat(HDC hdc);
void display();
void updateCameraByKeys(float dt);
void computeEye(Vec3& eye);

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

    HWND hWnd = CreateWindow(WINDOW_TITLE, WINDOW_TITLE, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, gWidth, gHeight,
        NULL, NULL, wc.hInstance, NULL);
    if (!hWnd) return 0;

    HDC hdc = GetDC(hWnd);
    if (!initPixelFormat(hdc)) return 0;

    HGLRC rc = wglCreateContext(hdc);
    if (!wglMakeCurrent(hdc, rc)) return 0;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);

    buildTriangles();
    computeVertexNormals();

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
    case WM_DESTROY: PostQuitMessage(0); return 0;

    case WM_SIZE:
        gWidth = LOWORD(lParam);
        gHeight = HIWORD(lParam);
        if (gWidth <= 0)  gWidth = 1;
        if (gHeight <= 0) gHeight = 1;
        glViewport(0, 0, gWidth, gHeight);
        return 0;

    case WM_LBUTTONDOWN:
        gLMBDown = true; SetCapture(hWnd);
        gLastMouse.x = LOWORD(lParam); gLastMouse.y = HIWORD(lParam);
        return 0;

    case WM_LBUTTONUP:
        gLMBDown = false; ReleaseCapture(); return 0;

    case WM_MOUSEMOVE:
        if (gLMBDown)
        {
            int mx = LOWORD(lParam), my = HIWORD(lParam);
            int dx = mx - gLastMouse.x, dy = my - gLastMouse.y;
            gLastMouse.x = mx; gLastMouse.y = my;
            const float sens = 0.005f;
            gYaw += dx * sens;
            gPitch -= dy * sens;
            const float limit = 1.55334f;
            if (gPitch > limit) gPitch = limit;
            if (gPitch < -limit) gPitch = -limit;
        }
        return 0;

    case WM_MOUSEWHEEL:
    {
        short delta = GET_WHEEL_DELTA_WPARAM(wParam);
        float zoomFactor = 1.0f - (delta / 120.0f) * 0.1f;
        gDist *= zoomFactor;
        if (gDist < 2.0f) gDist = 2.0f;
        if (gDist > 50.0f) gDist = 50.0f;
        return 0;
    }

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) PostQuitMessage(0);
        else if (wParam == 'R') { gYaw = 0.2f; gPitch = 0.1f; gDist = 14.0f; gTarget = { 0.0f, 4.0f, 0.0f }; }
        else if (wParam == '1') { gShowWireframe = !gShowWireframe; }
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

    default: return DefWindowProc(hWnd, msg, wParam, lParam);
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

// --------------------- 摄像机控制 ---------------------
void computeEye(Vec3& eye)
{
    float cp = std::cos(gPitch), sp = std::sin(gPitch);
    float sy = std::sin(gYaw), cy = std::cos(gYaw);
    eye.x = gTarget.x + gDist * cp * sy;
    eye.y = gTarget.y + gDist * sp;
    eye.z = gTarget.z + gDist * cp * cy;
}

void updateCameraByKeys(float dt)
{
    Vec3 eye; computeEye(eye);
    Vec3 forward = { gTarget.x - eye.x, gTarget.y - eye.y, gTarget.z - eye.z };
    float fl = std::sqrt(forward.x * forward.x + forward.y * forward.y + forward.z * forward.z);
    if (fl > 1e-6f) { forward.x /= fl; forward.y /= fl; forward.z /= fl; }

    Vec3 worldUp = { 0.0f, 1.0f, 0.0f };
    Vec3 right = {
        forward.y * worldUp.z - forward.z * worldUp.y,
        forward.z * worldUp.x - forward.x * worldUp.z,
        forward.x * worldUp.y - forward.y * worldUp.x
    };
    float rl = std::sqrt(right.x * right.x + right.y * right.y + right.z * right.z);
    if (rl > 1e-6f) { right.x /= rl; right.y /= rl; right.z /= rl; }

    float moveSpeed = gDist * 0.8f;
    float moveStep = moveSpeed * dt;

    // W/S for Up/Down
    if (keyW) { gTarget.y += moveStep; }
    if (keyS) { gTarget.y -= moveStep; }

    // A/D for Panning (strafing)
    if (keyA) { gTarget.x -= right.x * moveStep;   gTarget.y -= right.y * moveStep;   gTarget.z -= right.z * moveStep; }
    if (keyD) { gTarget.x += right.x * moveStep;   gTarget.y += right.y * moveStep;   gTarget.z += right.z * moveStep; }
}

// --------------------- 绘制 ---------------------
void display()
{
    glClearColor(0.1f, 0.1f, 0.12f, 1.0f); // 深色背景以突出模型
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    double aspect = (gHeight > 0) ? (double)gWidth / (double)gHeight : 1.3333;
    gluPerspective(45.0, aspect, 0.1, 100.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    Vec3 eye; computeEye(eye);
    gluLookAt(eye.x, eye.y, eye.z, gTarget.x, gTarget.y, gTarget.z, 0.0, 1.0, 0.0);

    glPolygonMode(GL_FRONT_AND_BACK, gShowWireframe ? GL_LINE : GL_FILL);

    // 关闭背面剔除
    glDisable(GL_CULL_FACE);

    if (!gShowWireframe) { glEnable(GL_POLYGON_OFFSET_FILL); glPolygonOffset(1.f, 1.f); }

    // 光照 (移除高光)
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    float lightPos[] = { 10.f, 10.f, 15.f, 1.f };
    float ambientLight[] = { 0.4f, 0.4f, 0.4f, 1.f }; // 增强环境光
    float diffuseLight[] = { 0.6f, 0.6f, 0.6f, 1.f }; // 减弱漫反射光
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);

    // 移除材质属性设置，使用纯色
    glColor3f(0.8f, 0.8f, 0.8f); // 中性灰色

    // 绘制模型
    glPushMatrix();
    glTranslatef(0.0f, -4.5f, 0.0f); // 调整模型位置以适应新尺寸

    glBegin(GL_TRIANGLES);
    for (const auto& t : gTris)
    {
        const Vec3f& n0 = gVertexNormals[t.a];
        const Vec3f& v0 = gFootVertices[t.a];
        glNormal3f(n0.x, n0.y, n0.z);
        glVertex3f(v0.x, v0.y, v0.z);

        const Vec3f& n1 = gVertexNormals[t.b];
        const Vec3f& v1 = gFootVertices[t.b];
        glNormal3f(n1.x, n1.y, n1.z);
        glVertex3f(v1.x, v1.y, v1.z);

        const Vec3f& n2 = gVertexNormals[t.c];
        const Vec3f& v2 = gFootVertices[t.c];
        glNormal3f(n2.x, n2.y, n2.z);
        glVertex3f(v2.x, v2.y, v2.z);
    }
    glEnd();

    glPopMatrix();

    glDisable(GL_LIGHTING);
    if (!gShowWireframe) glDisable(GL_POLYGON_OFFSET_FILL);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

