#include <Windows.h>
#include <gl/GL.h>
#include <gl/GLU.h>
#define _USE_MATH_DEFINES
#include <cmath>
#include <vector>
#include <cstdio>
#include <algorithm>

#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "Glu32.lib")

#define WINDOW_TITLE "Rounded Connected Leg Model (with Your Foot)"

// --------------------- Global State ---------------------
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
float gDist = 18.0f;

bool keyW = false, keyS = false, keyA = false, keyD = false;

LARGE_INTEGER gFreq = { 0 }, gPrev = { 0 };

// --------------------- Vertex/Face Data ---------------------
struct Vec3f { float x, y, z; };

std::vector<Vec3f> gAllVertices;
std::vector<std::vector<int>> gAllQuads;
struct Tri { int a, b, c; };
std::vector<Tri>   gTris;
std::vector<Vec3f> gVertexNormals;
Vec3f gModelCenter{ 0,0,0 };

// --------------------- YOUR FOOT (vertices + faces) ---------------------
// (Exactly the foot you supplied; unchanged)
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
    {0.4f, 0.4f, 0.0f},  // 68 - Inner side wall center
    {-0.45f, 0.4f, 0.0f},// 69 - Outer side wall center

    // Sole Arch Vertices
    {0.2f, 0.0f, -0.1f}, // 70 - Inner arch
    {-0.2f, 0.0f, -0.1f},// 71 - Outer arch

    // Heel Curve Vertices
    {0.3f, 0.4f, -0.7f}, // 72 - Inner heel curve
    {0.0f, 0.3f, -0.85f},// 73 - Back heel curve
    {-0.3f, 0.4f, -0.7f} // 74 - Outer heel curve
};

const int gFootQuads[][4] = {
    // Toes
    {0,1,2,3}, {4,5,1,0}, {7,6,2,3}, {4,0,3,7}, {5,4,7,6},
    {8,9,10,11}, {12,13,9,8}, {15,14,10,11}, {12,8,11,15}, {13,12,15,14},
    {16,17,18,19}, {20,21,17,16}, {23,22,18,19}, {20,16,19,23}, {21,20,23,22},
    {24,25,26,27}, {28,29,25,24}, {31,30,26,27}, {28,24,27,31}, {29,28,31,30},
    {32,33,34,35}, {36,37,33,32}, {39,38,34,35}, {36,32,35,39}, {37,36,39,38},

    // === FOOT BODY ===
    {40, 41, 7, 5},   // Inner side near big toe
    {42, 43, 39, 37}, // Outer side near pinky

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

    // FINAL PATCHES (toe gaps + side walls)
    {68, 45, 46, 72}, {68, 72, 51, -1}, {68, 51, 70, -1}, {68, 70, 50, 40},
    {69, 48, 47, 74}, {69, 74, 52, -1}, {69, 52, 71, -1}, {69, 71, 61, 42},
    {7, 15, 13, 5}, {15, 23, 21, 13}, {23, 31, 29, 21}, {31, 39, 37, 29}
};

const int gNumFootVertices = sizeof(gFootVertices) / sizeof(gFootVertices[0]);
const int gNumFootQuads    = sizeof(gFootQuads)    / sizeof(gFootQuads[0]);

// --------------------- Math Helpers ---------------------
Vec3f sub(const Vec3f& p, const Vec3f& q) { return { p.x - q.x, p.y - q.y, p.z - q.z }; }
Vec3f cross(const Vec3f& a, const Vec3f& b) {
    return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
}
float dot(const Vec3f& a, const Vec3f& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

Vec3f normalize(const Vec3f& v) {
    float l = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    return (l > 1e-6f) ? Vec3f{ v.x / l, v.y / l, v.z / l } : Vec3f{ 0,0,0 };
}

Vec3 normalize(const Vec3& v) {
    float l = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    return (l > 1e-6f) ? Vec3{ v.x / l, v.y / l, v.z / l } : Vec3{ 0,0,0 };
}

std::vector<Vec3f> generateRing(float cx, float cy, float cz, float radiusX, float radiusZ, int numSegments, float startAngle = 0.0f) {
    std::vector<Vec3f> ringVertices;
    for (int i = 0; i < numSegments; ++i) {
        float angle = startAngle + 2.0f * (float)M_PI * i / numSegments;
        ringVertices.push_back({ cx + radiusX * std::sin(angle), cy, cz + radiusZ * std::cos(angle) });
    }
    return ringVertices;
}

void addRingQuads(int ring1StartIdx, int ring2StartIdx, int numSegments) {
    for (int i = 0; i < numSegments; ++i) {
        int i0 = ring1StartIdx + i;
        int i1 = ring1StartIdx + (i + 1) % numSegments;
        int i2 = ring2StartIdx + (i + 1) % numSegments;
        int i3 = ring2StartIdx + i;
        gAllQuads.push_back({ i0, i1, i2, i3 });
    }
}

inline Vec3f lerp(const Vec3f& a, const Vec3f& b, float t) {
    return { a.x + (b.x - a.x)*t, a.y + (b.y - a.y)*t, a.z + (b.z - a.z)*t };
}
inline Vec3f rotateY(const Vec3f& v, float ang) {
    float s = std::sin(ang), c = std::cos(ang);
    return { c*v.x + s*v.z, v.y, -s*v.x + c*v.z };
}

// --------------------- Build Mesh (Leg + Your Foot) ---------------------
void buildLegMesh() {
    gAllVertices.clear();
    gAllQuads.clear();

    const int legSegments = 12;
    int currentVertexIndex = 0;

    // ----- Ankle Ring (12) -----
    int ankleRingIdx = currentVertexIndex;
    auto ankleRing = generateRing(0.0f, 0.9f, -0.3f, 0.5f, 0.4f, legSegments);
    gAllVertices.insert(gAllVertices.end(), ankleRing.begin(), ankleRing.end());
    currentVertexIndex += legSegments;

    // ----- Lower Shin -----
    int lowerShinRingIdx = currentVertexIndex;
    auto lowerShinRing = generateRing(0.0f, 1.45f, -0.2f, 0.55f, 0.45f, legSegments);
    gAllVertices.insert(gAllVertices.end(), lowerShinRing.begin(), lowerShinRing.end());
    currentVertexIndex += legSegments;
    addRingQuads(ankleRingIdx, lowerShinRingIdx, legSegments);

    // ----- Lower Calf -----
    int lowerCalfRingIdx = currentVertexIndex;
    auto lowerCalfRing = generateRing(0.0f, 2.0f, -0.2f, 0.6f, 0.5f, legSegments);
    gAllVertices.insert(gAllVertices.end(), lowerCalfRing.begin(), lowerCalfRing.end());
    currentVertexIndex += legSegments;
    addRingQuads(lowerShinRingIdx, lowerCalfRingIdx, legSegments);

    // ----- Mid Calf -----
    int midCalfRingIdx = currentVertexIndex;
    auto midCalfRing = generateRing(0.0f, 2.75f, 0.0f, 0.7f, 0.6f, legSegments);
    gAllVertices.insert(gAllVertices.end(), midCalfRing.begin(), midCalfRing.end());
    currentVertexIndex += legSegments;
    addRingQuads(lowerCalfRingIdx, midCalfRingIdx, legSegments);

    // ----- Upper Calf -----
    int upperCalfRingIdx = currentVertexIndex;
    auto upperCalfRing = generateRing(0.0f, 3.5f, 0.1f, 0.65f, 0.55f, legSegments);
    gAllVertices.insert(gAllVertices.end(), upperCalfRing.begin(), upperCalfRing.end());
    currentVertexIndex += legSegments;
    addRingQuads(midCalfRingIdx, upperCalfRingIdx, legSegments);

    // ----- Knee -----
    int kneeRingIdx = currentVertexIndex;
    auto kneeRing = generateRing(0.0f, 4.5f, 0.2f, 0.7f, 0.6f, legSegments);
    gAllVertices.insert(gAllVertices.end(), kneeRing.begin(), kneeRing.end());
    currentVertexIndex += legSegments;
    addRingQuads(upperCalfRingIdx, kneeRingIdx, legSegments);

    // ----- Lower Thigh -----
    int lowerThighRingIdx = currentVertexIndex;
    auto lowerThighRing = generateRing(0.0f, 5.5f, 0.1f, 0.8f, 0.7f, legSegments);
    gAllVertices.insert(gAllVertices.end(), lowerThighRing.begin(), lowerThighRing.end());
    currentVertexIndex += legSegments;
    addRingQuads(kneeRingIdx, lowerThighRingIdx, legSegments);

    // ----- Mid Thigh -----
    int midThighRingIdx = currentVertexIndex;
    auto midThighRing = generateRing(0.0f, 6.5f, 0.0f, 0.9f, 0.8f, legSegments);
    gAllVertices.insert(gAllVertices.end(), midThighRing.begin(), midThighRing.end());
    currentVertexIndex += legSegments;
    addRingQuads(lowerThighRingIdx, midThighRingIdx, legSegments);

    // ----- Upper Thigh -----
    int upperThighRingIdx = currentVertexIndex;
    auto upperThighRing = generateRing(0.0f, 7.5f, -0.1f, 0.95f, 0.85f, legSegments);
    gAllVertices.insert(gAllVertices.end(), upperThighRing.begin(), upperThighRing.end());
    currentVertexIndex += legSegments;
    addRingQuads(midThighRingIdx, upperThighRingIdx, legSegments);

    // ----- Hip -----
    int hipRingIdx = currentVertexIndex;
    auto hipRing = generateRing(0.0f, 8.5f, -0.2f, 1.0f, 0.9f, legSegments);
    gAllVertices.insert(gAllVertices.end(), hipRing.begin(), hipRing.end());
    currentVertexIndex += legSegments;
    addRingQuads(upperThighRingIdx, hipRingIdx, legSegments);

    // ----- Top Cap -----
    int topCenterIdx = currentVertexIndex++;
    gAllVertices.push_back({ 0.0f, 8.7f, -0.2f });
    for (int i = 0; i < legSegments; ++i) {
        gAllQuads.push_back({ hipRingIdx + i, hipRingIdx + (i + 1) % legSegments, topCenterIdx, -1 });
    }

    // ====== APPEND YOUR FOOT (transform + adapter ring) ======
    const float footScaleX = 0.95f;
    const float footScaleZ = 0.95f;
    const float footLift   = 0.0f;   // bring the foot’s own ankle ring to y≈0.9
    const float footShiftZ = -0.15f;  // tuck toward the shin
    const float footYaw    = 0.0f;    // Y-rotation if you want toe-out/in

    int footStartIdx = currentVertexIndex;
    for (int i = 0; i < gNumFootVertices; ++i) {
        Vec3f v = gFootVertices[i];
        v.x *= footScaleX;
        v.z *= footScaleZ;
        v = rotateY(v, footYaw);
        v.y += footLift;
        v.z += footShiftZ;
        gAllVertices.push_back(v);
        ++currentVertexIndex;
    }

    // Add foot faces (all quads/tri fans)
    for (int i = 0; i < gNumFootQuads; ++i) {
        std::vector<int> quad;
        for (int j = 0; j < 4; ++j) {
            int idx = gFootQuads[i][j];
            if (idx == -1) break;
            quad.push_back(footStartIdx + idx);
        }
        gAllQuads.push_back(quad);
    }

    // Build a 12-vertex adapter from foot ring (indices 44..49) and stitch to ankleRingIdx
    int footRingStart = footStartIdx + 44;
    int footRingCount = 6;

    int adapterStartIdx = currentVertexIndex;
    for (int i = 0; i < 12; ++i) {
        float s = (i / 12.0f) * footRingCount;
        int k = (int)std::floor(s);
        float t = s - k;
        int k0 = (k) % footRingCount;
        int k1 = (k + 1) % footRingCount;

        const Vec3f& a = gAllVertices[footRingStart + k0];
        const Vec3f& b = gAllVertices[footRingStart + k1];
        Vec3f sample = {
            a.x + (b.x - a.x) * t,
            a.y + (b.y - a.y) * t,
            a.z + (b.z - a.z) * t
        };

        gAllVertices.push_back(sample);
        ++currentVertexIndex;
    }

    // Now stitch leg ankle ring (ankleRingIdx, 12 verts) to adapter ring (12 verts)
    addRingQuads(ankleRingIdx, adapterStartIdx, 12);
}

// --------------------- Triangles & Normals ---------------------
Vec3f faceNormal(int i0, int i1, int i2) {
    const Vec3f& v0 = gAllVertices[i0];
    const Vec3f& v1 = gAllVertices[i1];
    const Vec3f& v2 = gAllVertices[i2];
    return normalize(cross(sub(v1, v0), sub(v2, v0)));
}

Vec3f triCenter(int i0, int i1, int i2) {
    const Vec3f& v0 = gAllVertices[i0];
    const Vec3f& v1 = gAllVertices[i1];
    const Vec3f& v2 = gAllVertices[i2];
    return { (v0.x + v1.x + v2.x) / 3.0f, (v0.y + v1.y + v2.y) / 3.0f, (v0.z + v1.z + v2.z) / 3.0f };
}

void buildTriangles()
{
    size_t nV = gAllVertices.size();
    gModelCenter = { 0,0,0 };
    for (size_t i = 0; i < nV; ++i) {
        gModelCenter.x += gAllVertices[i].x;
        gModelCenter.y += gAllVertices[i].y;
        gModelCenter.z += gAllVertices[i].z;
    }
    if (nV > 0) {
        gModelCenter.x /= nV; gModelCenter.y /= nV; gModelCenter.z /= nV;
    }

    gTris.clear();
    for (const auto& q : gAllQuads) {
        if (q.size() == 3 || (q.size() == 4 && q[3] == -1)) {
            int a = q[0], b = q[1], c = q[2];
            if (dot(faceNormal(a, b, c), sub(triCenter(a, b, c), gModelCenter)) < 0) std::swap(b, c);
            gTris.push_back({ a,b,c });
        }
        else if (q.size() == 4) {
            int a = q[0], b = q[1], c = q[2], d = q[3];
            {
                int i0 = a, i1 = b, i2 = d;
                if (dot(faceNormal(i0, i1, i2), sub(triCenter(i0, i1, i2), gModelCenter)) < 0) std::swap(i1, i2);
                gTris.push_back({ i0,i1,i2 });
            }
            {
                int i0 = b, i1 = c, i2 = d;
                if (dot(faceNormal(i0, i1, i2), sub(triCenter(i0, i1, i2), gModelCenter)) < 0) std::swap(i1, i2);
                gTris.push_back({ i0,i1,i2 });
            }
        }
    }
}

void computeVertexNormals() {
    size_t nV = gAllVertices.size();
    if (nV == 0) return;
    gVertexNormals.assign(nV, { 0.0f, 0.0f, 0.0f });

    for (const auto& t : gTris) {
        Vec3f n = faceNormal(t.a, t.b, t.c);
        gVertexNormals[t.a].x += n.x; gVertexNormals[t.a].y += n.y; gVertexNormals[t.a].z += n.z;
        gVertexNormals[t.b].x += n.x; gVertexNormals[t.b].y += n.y; gVertexNormals[t.b].z += n.z;
        gVertexNormals[t.c].x += n.x; gVertexNormals[t.c].y += n.y; gVertexNormals[t.c].z += n.z;
    }
    for (size_t i = 0; i < nV; ++i) gVertexNormals[i] = normalize(gVertexNormals[i]);
}

// --------------------- Forward Declarations ---------------------
LRESULT WINAPI WindowProcedure(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
bool initPixelFormat(HDC hdc);
void display();
void updateCameraByKeys(float dt);
void computeEye(Vec3& eye);
void drawLeg(bool mirrorX);

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

    HDC hdc = GetDC(hWnd);
    if (!initPixelFormat(hdc)) return 0;

    HGLRC rc = wglCreateContext(hdc);
    if (!wglMakeCurrent(hdc, rc)) return 0;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);

    buildLegMesh();
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
        if (dt > 0.1f) dt = 0.1f;

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
        gWidth = LOWORD(lParam); gHeight = HIWORD(lParam);
        if (gWidth <= 0) gWidth = 1; if (gHeight <= 0) gHeight = 1;
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
        else if (wParam == 'R') { gYaw = 0.2f; gPitch = 0.1f; gDist = 18.0f; gTarget = { 0.0f, 4.0f, 0.0f }; }
        else if (wParam == '1') { gShowWireframe = !gShowWireframe; }
        else if (wParam == 'W') keyW = true; else if (wParam == 'S') keyS = true;
        else if (wParam == 'A') keyA = true; else if (wParam == 'D') keyD = true;
        return 0;
    case WM_KEYUP:
        if (wParam == 'W') keyW = false; else if (wParam == 'S') keyS = false;
        else if (wParam == 'A') keyA = false; else if (wParam == 'D') keyD = false;
        return 0;
    default: return DefWindowProc(hWnd, msg, wParam, lParam);
    }
}

// --------------------- OpenGL Pixel Format ---------------------
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

// --------------------- Camera Control ---------------------
void computeEye(Vec3& eye) {
    eye.x = gTarget.x + gDist * std::cos(gPitch) * std::sin(gYaw);
    eye.y = gTarget.y + gDist * std::sin(gPitch);
    eye.z = gTarget.z + gDist * std::cos(gPitch) * std::cos(gYaw);
}

void updateCameraByKeys(float dt) {
    Vec3 eye; computeEye(eye);
    Vec3 f = { gTarget.x - eye.x, 0, gTarget.z - eye.z };
    f = normalize(f);
    Vec3 r = { f.z, 0, -f.x };
    float moveStep = gDist * 0.8f * dt;
    if (keyW) { gTarget.y += moveStep; } if (keyS) { gTarget.y -= moveStep; }
    if (keyA) { gTarget.x -= r.x * moveStep; gTarget.z -= r.z * moveStep; }
    if (keyD) { gTarget.x += r.x * moveStep; gTarget.z += r.z * moveStep; }
}

// --------------------- Drawing ---------------------
void drawLeg(bool mirrorX)
{
    float mirror = mirrorX ? -1.0f : 1.0f;

    glBegin(GL_TRIANGLES);
    for (const auto& t : gTris)
    {
        const Vec3f& n0 = gVertexNormals[t.a];
        const Vec3f& v0 = gAllVertices[t.a];
        glNormal3f(n0.x * mirror, n0.y, n0.z);
        glVertex3f(v0.x * mirror, v0.y, v0.z);

        const Vec3f& n1 = gVertexNormals[t.b];
        const Vec3f& v1 = gAllVertices[t.b];
        glNormal3f(n1.x * mirror, n1.y, n1.z);
        glVertex3f(v1.x * mirror, v1.y, v1.z);

        const Vec3f& n2 = gVertexNormals[t.c];
        const Vec3f& v2 = gAllVertices[t.c];
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
    gluPerspective(45.0, (gHeight > 0) ? (double)gWidth / gHeight : 1.0, 0.1, 100.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    Vec3 eye; computeEye(eye);
    gluLookAt(eye.x, eye.y, eye.z, gTarget.x, gTarget.y, gTarget.z, 0.0, 1.0, 0.0);

    glPolygonMode(GL_FRONT_AND_BACK, gShowWireframe ? GL_LINE : GL_FILL);

    // Lighting
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

    // --- Draw Left Leg ---
    glPushMatrix();
    glTranslatef(0.0f, -4.5f, 0.0f);
    glTranslatef(-0.75f, 0.0f, 0.0f);
    drawLeg(false);
    glPopMatrix();

    // --- Draw Right Leg ---
    glPushMatrix();
    glTranslatef(0.0f, -4.5f, 0.0f);
    glTranslatef(0.75f, 0.0f, 0.0f);
    drawLeg(true);
    glPopMatrix();

    glDisable(GL_LIGHTING);
}
