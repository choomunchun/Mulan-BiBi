#include <Windows.h>
#include <gl/GL.h>
#include <gl/GLU.h>
#include <cmath>
#include <vector>
#include <cstdio>
#include <algorithm> // For std::swap

#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "Glu32.lib")

#define WINDOW_TITLE "Complete Foot Model (Final)"

// --------------------- Global State ---------------------
int   gWidth = 800;
int   gHeight = 600;

bool  gRunning = true;
bool  gShowWireframe = false;

bool  gLMBDown = false;
POINT gLastMouse = { 0, 0 };

struct Vec3 { float x, y, z; };

Vec3  gTarget = { 0.0f, 0.7f, 0.0f };
float gYaw = 0.2f;
float gPitch = 0.1f;
float gDist = 5.0f;

bool keyW = false, keyS = false, keyA = false, keyD = false;

LARGE_INTEGER gFreq = { 0 }, gPrev = { 0 };

// --------------------- Vertex/Face Data ---------------------
struct Vec3f { float x, y, z; };

// --- 顶点数据与上一版本相同 ---
const Vec3f gFootVertices[] = {
    // Toes (5 distinct toes)
    {0.25f, 0.0f, 1.4f}, {0.1f, 0.0f, 1.45f}, {0.1f, 0.25f, 1.45f}, {0.25f, 0.25f, 1.4f}, // 0-3 Big Toe Tip
    {0.25f, 0.0f, 1.2f}, {0.1f, 0.0f, 1.2f}, {0.1f, 0.3f, 1.2f}, {0.25f, 0.3f, 1.2f}, // 4-7 Big Toe Knuckle
    {0.05f, 0.0f, 1.4f}, {-0.1f, 0.0f, 1.35f}, {-0.1f, 0.2f, 1.35f}, {0.05f, 0.2f, 1.4f}, // 8-11 2nd Toe Tip
    {0.05f, 0.0f, 1.15f}, {-0.1f, 0.0f, 1.15f}, {-0.1f, 0.25f, 1.15f}, {0.05f, 0.25f, 1.15f}, // 12-15 2nd Toe Knuckle
    {-0.15f, 0.0f, 1.3f}, {-0.28f, 0.0f, 1.25f}, {-0.28f, 0.18f, 1.25f}, {-0.15f, 0.18f, 1.3f}, // 16-19 3rd Toe Tip
    {-0.15f, 0.0f, 1.1f}, {-0.28f, 0.0f, 1.1f}, {-0.28f, 0.22f, 1.1f}, {-0.15f, 0.22f, 1.1f}, // 20-23 3rd Toe Knuckle
    {-0.32f, 0.0f, 1.2f}, {-0.42f, 0.0f, 1.1f}, {-0.42f, 0.16f, 1.1f}, {-0.32f, 0.16f, 1.2f}, // 24-27 4th Toe Tip
    {-0.32f, 0.0f, 1.0f}, {-0.42f, 0.0f, 1.0f}, {-0.42f, 0.2f, 1.0f}, {-0.32f, 0.2f, 1.0f}, // 28-31 4th Toe Knuckle
    {-0.46f, 0.0f, 1.0f}, {-0.55f, 0.0f, 0.9f}, {-0.55f, 0.14f, 0.9f}, {-0.46f, 0.14f, 1.0f}, // 32-35 5th Toe Tip
    {-0.46f, 0.0f, 0.8f}, {-0.55f, 0.0f, 0.8f}, {-0.55f, 0.18f, 0.8f}, {-0.46f, 0.18f, 0.8f}, // 36-39 5th Toe Knuckle

    // Foot Body Sides
    {0.3f, 0.0f, 0.8f}, {0.3f, 0.4f, 0.8f}, // 40-41 (Side of Big Toe)
    {-0.6f, 0.0f, 0.6f}, {-0.6f, 0.3f, 0.6f}, // 42-43 (Side of Pinky)

    // Lower Ankle Ring / Heel connection points
    {0.0f, 0.9f, 0.2f}, {0.5f, 0.8f, 0.1f}, {0.55f, 0.9f, -0.5f}, {0.0f, 0.8f, -0.7f}, {-0.55f, 0.9f, -0.5f}, {-0.5f, 0.8f, 0.1f}, // 44-49

    // Sole Vertices
    {0.0f, 0.0f, 0.2f}, {0.4f, 0.0f, -0.5f}, {-0.4f, 0.0f, -0.5f}, // 50-52

    // Intermediate Ring (Top of Foot - Metatarsals)
    {0.35f, 0.6f, 0.5f}, {0.1f, 0.7f, 0.4f}, {-0.15f, 0.65f, 0.4f}, {-0.35f, 0.5f, 0.4f}, {-0.5f, 0.45f, 0.3f}, // 53-57

    // Intermediate Ring (Bottom of Foot - Ball)
    {0.2f, 0.0f, 0.7f}, {0.0f, 0.0f, 0.65f}, {-0.2f, 0.0f, 0.6f}, {-0.4f, 0.0f, 0.5f}, // 58-61

    // Upper Ankle Vertices
    {0.0f, 1.2f, 0.2f}, {0.3f, 1.1f, 0.1f}, {0.35f, 1.2f, -0.4f}, {0.0f, 1.1f, -0.6f}, {-0.35f, 1.2f, -0.4f}, {-0.3f, 1.1f, 0.1f}, // 62-67

    // Side Wall Vertices
    {0.4f, 0.4f, 0.0f}, // 68 - Inner side wall center
    {-0.45f, 0.4f, 0.0f},// 69 - Outer side wall center

    // Sole Arch Vertices
    {0.2f, 0.0f, -0.1f}, // 70 - Inner arch
    {-0.2f, 0.0f, -0.1f},// 71 - Outer arch

    // Heel Curve Vertices
    {0.3f, 0.4f, -0.7f}, // 72 - Inner heel curve
    {0.0f, 0.3f, -0.85f},// 73 - Back heel curve
    {-0.3f, 0.4f, -0.7f} // 74 - Outer heel curve
};


// --- MODIFICATION START: Final adjustments to gFootQuads to patch side walls ---
const int gFootQuads[][4] = {
    // Toes
    {0,1,2,3}, {4,5,1,0}, {7,6,2,3}, {4,0,3,7}, {5,4,7,6},
    {8,9,10,11}, {12,13,9,8}, {15,14,10,11}, {12,8,11,15}, {13,12,15,14},
    {16,17,18,19}, {20,21,17,16}, {23,22,18,19}, {20,16,19,23}, {21,20,23,22},
    {24,25,26,27}, {28,29,25,24}, {31,30,26,27}, {28,24,27,31}, {29,28,31,30},
    {32,33,34,35}, {36,37,33,32}, {39,38,34,35}, {36,32,35,39}, {37,36,39,38},

    // === FOOT BODY ===
    // Close final gaps on the sides near the toes
    {40, 41, 7, 5},   // Inner side wall next to big toe
    {42, 43, 39, 37}, // Outer side wall next to pinky toe

    // Top of Foot
    {7, 15, 54, 53}, {15, 23, 55, 54}, {23, 31, 56, 55}, {31, 39, 57, 56},
    {41, 7, 53, -1}, {43, 39, 57, -1},
    {53, 54, 44, 45}, {54, 55, 49, 44}, {55, 56, 48, 49}, {56, 57, 48, -1},

    // Sole of Foot with Arch
    {5, 13, 59, 58}, {13, 21, 60, 59}, {21, 29, 61, 60},
    {29, 37, 61, -1}, {37, 42, 61, -1},
    {40, 5, 58, -1},
    {58, 59, 70, 50}, {59, 60, 71, 70}, {60, 61, 52, 71},
    {70, 71, 52, 51}, {50, 70, 51, -1},

    // Detailed Side Walls
    {41, 53, 68, -1}, {53, 45, 68, -1}, {40, 41, 68, -1},
    {43, 57, 69, -1}, {57, 48, 69, -1}, {42, 43, 69, -1},

    // Detailed Heel
    {51, 73, 72, -1}, {52, 74, 73, 51},
    {46, 47, 72, -1}, {72, 47, 73, -1}, {73, 47, 74, -1}, {48, 47, 74, -1},

    // === ANKLE STRUCTURE ===
    {45, 44, 62, 63}, {44, 49, 67, 62}, {49, 48, 66, 67},
    {48, 47, 65, 66}, {47, 46, 64, 65}, {46, 45, 63, 64},
    {62, 63, 64, 65}, {62, 65, 66, 67},

    // FINAL PATCHES to close side wall holes
    {68, 45, 46, 72}, {68, 72, 51, -1}, {68, 51, 70, -1}, {68, 70, 50, 40},
    {69, 48, 47, 74}, {69, 74, 52, -1}, {69, 52, 71, -1}, {69, 71, 61, 42},

    // === NEW FINAL PATCHES for Gaps Between Toes ===
    {7, 15, 13, 5},
    {15, 23, 21, 13},
    {23, 31, 29, 21},
    {31, 39, 37, 29}
};
// --- MODIFICATION END ---


// --------------------- Triangulation + Normal Calculation ---------------------
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
        bool invalid = false;
        for (int i = 0; i < 4; ++i) {
            if (q[i] != -1 && (q[i] < 0 || q[i] >= nV)) {
                invalid = true;
                break;
            }
        }
        if (invalid) continue;

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

// --------------------- Forward Declarations ---------------------
LRESULT WINAPI WindowProcedure(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
bool initPixelFormat(HDC hdc);
void display();
void updateCameraByKeys(float dt);
void computeEye(Vec3& eye);
void drawFoot(bool mirrorX);

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

// --------------------- Window Procedure ---------------------
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
        if (gDist < 1.0f) gDist = 1.0f;
        if (gDist > 30.0f) gDist = 30.0f;
        return 0;
    }

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) PostQuitMessage(0);
        else if (wParam == 'R') { gYaw = 0.2f; gPitch = 0.1f; gDist = 5.0f; gTarget = { 0.0f, 0.7f, 0.0f }; }
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

// --------------------- OpenGL Pixel Format ---------------------
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

// --------------------- Camera Control ---------------------
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

    if (keyW) { gTarget.y += moveStep; }
    if (keyS) { gTarget.y -= moveStep; }

    if (keyA) { gTarget.x -= right.x * moveStep;   gTarget.y -= right.y * moveStep;   gTarget.z -= right.z * moveStep; }
    if (keyD) { gTarget.x += right.x * moveStep;   gTarget.y += right.y * moveStep;   gTarget.z += right.z * moveStep; }
}

// --------------------- Drawing ---------------------
void drawFoot(bool mirrorX)
{
    float mirror = mirrorX ? -1.0f : 1.0f;

    glBegin(GL_TRIANGLES);
    for (const auto& t : gTris)
    {
        const Vec3f& n0 = gVertexNormals[t.a];
        const Vec3f& v0 = gFootVertices[t.a];
        glNormal3f(n0.x * mirror, n0.y, n0.z);
        glVertex3f(v0.x * mirror, v0.y, v0.z);

        const Vec3f& n1 = gVertexNormals[t.b];
        const Vec3f& v1 = gFootVertices[t.b];
        glNormal3f(n1.x * mirror, n1.y, n1.z);
        glVertex3f(v1.x * mirror, v1.y, v1.z);

        const Vec3f& n2 = gVertexNormals[t.c];
        const Vec3f& v2 = gFootVertices[t.c];
        glNormal3f(n2.x * mirror, n2.y, n2.z);
        glVertex3f(v2.x * mirror, v2.y, v2.z);
    }
    glEnd();
}

void display()
{
    glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
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

    glDisable(GL_CULL_FACE);

    if (!gShowWireframe) { glEnable(GL_POLYGON_OFFSET_FILL); glPolygonOffset(1.f, 1.f); }

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    float lightPos[] = { 10.f, 10.f, 15.f, 1.f };
    float ambientLight[] = { 0.4f, 0.4f, 0.4f, 1.f };
    float diffuseLight[] = { 0.6f, 0.6f, 0.6f, 1.f };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);

    glColor3f(0.8f, 0.8f, 0.8f);

    // --- Draw Left Foot ---
    glPushMatrix();
    glTranslatef(-0.75f, 0.0f, 0.0f);
    drawFoot(false);
    glPopMatrix();

    // --- Draw Right Foot ---
    glPushMatrix();
    glTranslatef(0.75f, 0.0f, 0.0f);
    drawFoot(true);
    glPopMatrix();


    glDisable(GL_LIGHTING);
    if (!gShowWireframe) glDisable(GL_POLYGON_OFFSET_FILL);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}
