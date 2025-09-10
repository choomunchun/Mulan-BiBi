#include <Windows.h>
#include <windowsx.h>
#include <gl/GL.h>
#include <gl/GLU.h>
#include <cmath>
#include <vector>
#include <cstdio>
#include <algorithm> 

#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "Glu32.lib")

#define WINDOW_TITLE "Full Body Model Viewer"

// ===================================================================
//
// SECTION 1: MODEL & GLOBAL STATE
//
// ===================================================================

#define PI 3.1415926535f

// --- Global State ---
int   gWidth = 800;
int   gHeight = 600;

bool  gRunning = true;
bool  gShowWireframe = false;

bool  gLMBDown = false;
POINT gLastMouse = { 0, 0 };

// --- Camera State ---
struct Vec3 { float x, y, z; };
Vec3  gTarget = { 0.0f, 3.5f, 0.0f };
float gDist = 15.0f;
float gYaw = 0.2f;
float gPitch = 0.1f;

// --- Input State ---
bool keyW = false, keyS = false, keyA = false, keyD = false; // For Camera
bool keyUp = false, keydown = false, keyLeft = false, keyRight = false, keyShift = false; // For Character

LARGE_INTEGER gFreq = { 0 }, gPrev = { 0 };

// --- Proportions Control ---
#define LEG_SCALE 0.9f
#define BODY_SCALE 3.5f
#define ARM_SCALE 0.15f
#define HAND_SCALE 0.3f  // <-- NEW: Controls hand size relative to the arm

// --- Animation & Character State ---
Vec3  gCharacterPos = { 0.0f, 0.0f, 0.0f };
float gCharacterYaw = 0.0f;
float gWalkPhase = 0.0f;
float gMoveSpeed = 0.0f;
const float WALK_SPEED = 3.0f;
const float RUN_SPEED = 7.0f;
const float TURN_SPEED = 120.0f;

// --- Data Structures ---
struct Vec3f { float x, y, z; };
struct Tri { int a, b, c; };
struct JointPose { float torsoYaw, torsoPitch, torsoRoll; float headYaw, headPitch, headRoll; } g_pose = { 0,0,0, 0,0,0 };
struct HandJoint { Vec3 position; int parentIndex; };
struct ArmJoint { Vec3 position; int parentIndex; };

// --- Leg Data ---
const Vec3f gFootVertices[] = {
    {0.25f,0.0f,1.4f},{0.1f,0.0f,1.45f},{0.1f,0.25f,1.45f},{0.25f,0.25f,1.4f},{0.25f,0.0f,1.2f},{0.1f,0.0f,1.2f},{0.1f,0.3f,1.2f},{0.25f,0.3f,1.2f},{0.05f,0.0f,1.4f},{-0.1f,0.0f,1.35f},{-0.1f,0.2f,1.35f},{0.05f,0.2f,1.4f},{0.05f,0.0f,1.15f},{-0.1f,0.0f,1.15f},{-0.1f,0.25f,1.15f},{0.05f,0.25f,1.15f},{-0.15f,0.0f,1.3f},{-0.28f,0.0f,1.25f},{-0.28f,0.18f,1.25f},{-0.15f,0.18f,1.3f},{-0.15f,0.0f,1.1f},{-0.28f,0.0f,1.1f},{-0.28f,0.22f,1.1f},{-0.15f,0.22f,1.1f},{-0.32f,0.0f,1.2f},{-0.42f,0.0f,1.1f},{-0.42f,0.16f,1.1f},{-0.32f,0.16f,1.2f},{-0.32f,0.0f,1.0f},{-0.42f,0.0f,1.0f},{-0.42f,0.2f,1.0f},{-0.32f,0.2f,1.0f},{-0.46f,0.0f,1.0f},{-0.55f,0.0f,0.9f},{-0.55f,0.14f,0.9f},{-0.46f,0.14f,1.0f},{-0.46f,0.0f,0.8f},{-0.55f,0.0f,0.8f},{-0.55f,0.18f,0.8f},{-0.46f,0.18f,0.8f},{0.3f,0.0f,0.8f},{0.3f,0.4f,0.8f},{-0.6f,0.0f,0.6f},{-0.6f,0.3f,0.6f},{0.0f,0.9f,0.2f},{0.5f,0.8f,0.1f},{0.55f,0.9f,-0.5f},{0.0f,0.8f,-0.7f},{-0.55f,0.9f,-0.5f},{-0.5f,0.8f,0.1f},{0.0f,0.0f,0.2f},{0.4f,0.0f,-0.5f},{-0.4f,0.0f,-0.5f},{0.0f,2.0f,-0.1f},{-0.6f,1.9f,-0.3f},{-0.6f,2.2f,-0.9f},{0.0f,2.3f,-1.0f},{0.6f,2.2f,-0.9f},{0.6f,1.9f,-0.3f},{0.0f,3.5f,0.1f},{-0.5f,3.4f,-0.1f},{-0.55f,3.8f,-0.7f},{0.0f,3.9f,-0.8f},{0.55f,3.8f,-0.7f},{0.5f,3.4f,-0.1f},{0.0f,4.5f,0.3f},{-0.45f,4.4f,0.2f},{-0.5f,4.5f,-0.6f},{0.0f,4.4f,-0.7f},{0.5f,4.5f,-0.6f},{0.45f,4.4f,0.2f},{0.0f,6.0f,0.2f},{-0.7f,5.8f,0.0f},{-0.75f,6.2f,-0.5f},{0.0f,6.3f,-0.6f},{0.75f,6.2f,-0.5f},{0.7f,5.8f,0.0f},{0.0f,8.0f,0.1f},{-0.7f,7.9f,-0.1f},{-0.75f,8.1f,-0.4f},{0.0f,8.2f,-0.5f},{0.75f,8.1f,-0.4f},{0.7f,7.9f,-0.1f}
};
const int gFootQuads[][4] = {
    {0,1,2,3},{4,5,1,0},{7,6,2,3},{4,0,3,7},{5,4,7,6},{8,9,10,11},{12,13,9,8},{15,14,10,11},{12,8,11,15},{13,12,15,14},{16,17,18,19},{20,21,17,16},{23,22,18,19},{20,16,19,23},{21,20,23,22},{24,25,26,27},{28,29,25,24},{31,30,26,27},{28,24,27,31},{29,28,31,30},{32,33,34,35},{36,37,33,32},{39,38,34,35},{36,32,35,39},{37,36,39,38},{4,40,41,7},{12,4,7,15},{20,12,15,23},{28,20,23,31},{36,28,31,39},{39,36,42,43},{40,5,13,12},{5,4,12,-1},{13,21,20,12},{21,29,28,20},{29,37,36,28},{37,42,36,-1},{5,50,13,-1},{13,50,21,-1},{21,50,29,-1},{29,50,37,-1},{37,50,42,-1},{41,45,49,43},{41,44,45,-1},{15,14,49,45},{23,22,14,15},{31,30,22,23},{39,38,30,31},{43,38,39,-1},{43,49,38,-1},{44,53,58,45},{45,58,52,46},{46,52,51,47},{47,51,57,48},{48,57,54,49},{49,54,53,44},{50,52,51,-1},{40,41,45,46},{46,52,47},{40,46,45,-1},{42,43,49,48},{48,51,47,-1},{42,48,49,-1},{50,40,46,52},{50,42,48,51},{50,40,52,-1},{50,42,51,-1},{53,59,60,54},{54,60,61,55},{55,61,62,56},{56,62,63,57},{57,63,64,58},{58,64,59,53},{59,65,66,60},{60,66,67,61},{61,67,68,62},{62,68,69,63},{63,69,70,64},{64,70,65,59},{65,71,72,66},{66,72,73,67},{67,73,74,68},{68,74,75,69},{69,75,76,70},{70,76,71,65},{71,77,78,72},{72,78,79,73},{73,79,80,74},{74,80,81,75},{75,81,82,76},{76,82,77,71},{82,81,80,-1},{82,80,79,-1},{82,79,78,-1},{82,78,77,-1}
};
std::vector<Tri>   gTris;
std::vector<Vec3f> gVertexNormals;
Vec3f gModelCenter{ 0,0,0 };

// --- Body/Head Data ---
GLUquadric* g_headQuadric = nullptr;
#define C0 1.0000000f
#define S0 0.0000000f
#define C1 0.9238795f
#define S1 0.3826834f
#define C2 0.7071068f
#define S2 0.7071068f
#define C3 0.3826834f
#define S3 0.9238795f
#define C4 0.0000000f
#define S4 1.0000000f
#define C5 -0.3826834f
#define S5 0.9238795f
#define C6 -0.7071068f
#define S6 0.7071068f
#define C7 -0.9238795f
#define S7 0.3826834f
#define C8 -1.0000000f
#define S8 0.0000000f
#define C9 -0.9238795f
#define S9 -0.3826834f
#define C10 -0.7071068f
#define S10 -0.7071068f
#define C11 -0.3826834f
#define S11 -0.9238795f
#define C12 0.0000000f
#define S12 -1.0000000f
#define C13 0.3826834f
#define S13 -0.9238795f
#define C14 0.7071068f
#define S14 -0.7071068f
#define C15 0.9238795f
#define S15 -0.3826834f
const float segCos[] = { C0,C1,C2,C3,C4,C5,C6,C7,C8,C9,C10,C11,C12,C13,C14,C15 };
const float segSin[] = { S0,S1,S2,S3,S4,S5,S6,S7,S8,S9,S10,S11,S12,S13,S14,S15 };
#define R0 0.38f
#define R1 0.37f
#define R2 0.36f
#define R3 0.35f
#define R4 0.34f
#define R5 0.32f
#define R6 0.30f
#define R7 0.28f
#define R8 0.27f
#define R9 0.28f
#define R10 0.29f
#define R11 0.30f
#define R12 0.29f
#define R13 0.27f
#define R14 0.24f
#define R15 0.21f
#define R16 0.19f
#define R17 0.16f
#define R18 0.133f
#define Y0 0.00f
#define Y1 0.05f
#define Y2 0.10f
#define Y3 0.15f
#define Y4 0.20f
#define Y5 0.28f
#define Y6 0.36f
#define Y7 0.44f
#define Y8 0.52f
#define Y9 0.60f
#define Y10 0.68f
#define Y11 0.76f
#define Y12 0.82f
#define Y13 0.88f
#define Y14 0.92f
#define Y15 0.96f
#define Y16 0.98f
#define Y17 1.04f
#define Y18 1.10f
#define HEAD_CENTER_Y 1.30f
#define HEAD_RADIUS   0.24f
#define QUAD(r1,y1,r2,y2,cA,sA,cB,sB) glVertex3f((r1)*(cA),(y1),(r1)*(sA));glVertex3f((r2)*(cA),(y2),(r2)*(sA));glVertex3f((r2)*(cB),(y2),(r2)*(sB));glVertex3f((r1)*(cA),(y1),(r1)*(sA));glVertex3f((r2)*(cB),(y2),(r2)*(sB));glVertex3f((r1)*(cB),(y1),(r1)*(sB));
#define BAND(rA,yA,rB,yB) QUAD(rA,yA,rB,yB,C0,S0,C1,S1) QUAD(rA,yA,rB,yB,C1,S1,C2,S2) QUAD(rA,yA,rB,yB,C2,S2,C3,S3) QUAD(rA,yA,rB,yB,C3,S3,C4,S4) QUAD(rA,yA,rB,yB,C4,S4,C5,S5) QUAD(rA,yA,rB,yB,C5,S5,C6,S6) QUAD(rA,yA,rB,yB,C6,S6,C7,S7) QUAD(rA,yA,rB,yB,C7,S7,C8,S8) QUAD(rA,yA,rB,yB,C8,S8,C9,S9) QUAD(rA,yA,rB,yB,C9,S9,C10,S10) QUAD(rA,yA,rB,yB,C10,S10,C11,S11) QUAD(rA,yA,rB,yB,C11,S11,C12,S12) QUAD(rA,yA,rB,yB,C12,S12,C13,S13) QUAD(rA,yA,rB,yB,C13,S13,C14,S14) QUAD(rA,yA,rB,yB,C14,S14,C15,S15) QUAD(rA,yA,rB,yB,C15,S15,C0,S0)

// --- Arm/Hand Data ---
std::vector<HandJoint> g_HandJoints;
std::vector<HandJoint> g_HandJoints2;
std::vector<ArmJoint> g_ArmJoints;
std::vector<ArmJoint> g_ArmJoints2;

// ===================================================================
//
// SECTION 2: HELPER & SETUP FUNCTIONS
//
// ===================================================================

// --- Forward Declarations ---
LRESULT WINAPI WindowProcedure(HWND, UINT, WPARAM, LPARAM);
void display();
void updateCharacter(float dt);
void drawLeg(bool mirrorX, float swingAngle);
void drawBodyAndHead(float leftLegAngle, float rightLegAngle);
void drawSkirt(float leftLegAngle, float rightLegAngle);
void drawArmsAndHands(float leftArmAngle, float rightArmAngle);

// --- Math & Model Building ---
Vec3f sub(const Vec3f& p, const Vec3f& q) { return { p.x - q.x, p.y - q.y, p.z - q.z }; }
Vec3f cross(const Vec3f& a, const Vec3f& b) { return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x }; }
float dot(const Vec3f& a, const Vec3f& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
Vec3f normalize(const Vec3f& v) { float l = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z); return (l > 1e-6f) ? Vec3f{ v.x / l, v.y / l, v.z / l } : Vec3f{ 0,0,0 }; }
Vec3f faceNormal(int i0, int i1, int i2) { return normalize(cross(sub(gFootVertices[i1], gFootVertices[i0]), sub(gFootVertices[i2], gFootVertices[i0]))); }
Vec3f triCenter(int i0, int i1, int i2) { const Vec3f& v0 = gFootVertices[i0], & v1 = gFootVertices[i1], & v2 = gFootVertices[i2]; return { (v0.x + v1.x + v2.x) / 3.f,(v0.y + v1.y + v2.y) / 3.f,(v0.z + v1.z + v2.z) / 3.f }; }
void buildTriangles() {
    int nV = sizeof(gFootVertices) / sizeof(gFootVertices[0]); for (int i = 0; i < nV; ++i) { gModelCenter.x += gFootVertices[i].x; gModelCenter.y += gFootVertices[i].y; gModelCenter.z += gFootVertices[i].z; } gModelCenter.x /= nV; gModelCenter.y /= nV; gModelCenter.z /= nV; gTris.clear();
    for (const auto& q : gFootQuads) { if (q[3] == -1) { int a = q[0], b = q[1], c = q[2]; if (dot(faceNormal(a, b, c), sub(triCenter(a, b, c), gModelCenter)) < 0)std::swap(b, c); gTris.push_back({ a,b,c }); } else { int a = q[0], b = q[1], c = q[2], d = q[3]; {int i0 = a, i1 = b, i2 = d; if (dot(faceNormal(i0, i1, i2), sub(triCenter(i0, i1, i2), gModelCenter)) < 0)std::swap(i1, i2); gTris.push_back({ i0,i1,i2 }); } {int i0 = b, i1 = c, i2 = d; if (dot(faceNormal(i0, i1, i2), sub(triCenter(i0, i1, i2), gModelCenter)) < 0)std::swap(i1, i2); gTris.push_back({ i0,i1,i2 }); } } }
}
void computeVertexNormals() {
    int nV = sizeof(gFootVertices) / sizeof(gFootVertices[0]); gVertexNormals.assign(nV, { 0,0,0 });
    for (const auto& t : gTris) { Vec3f n = faceNormal(t.a, t.b, t.c); gVertexNormals[t.a].x += n.x; gVertexNormals[t.a].y += n.y; gVertexNormals[t.a].z += n.z; gVertexNormals[t.b].x += n.x; gVertexNormals[t.b].y += n.y; gVertexNormals[t.b].z += n.z; gVertexNormals[t.c].x += n.x; gVertexNormals[t.c].y += n.y; gVertexNormals[t.c].z += n.z; }
    for (int i = 0; i < nV; ++i)gVertexNormals[i] = normalize(gVertexNormals[i]);
}
static void InitializeHand() {
    g_HandJoints.clear();
    g_HandJoints.resize(21);
    g_HandJoints[0] = { {0.0f, 0.0f, 0.0f}, -1 };
    g_HandJoints[1] = { {0.6f, -0.1f, 0.4f}, 0 }; g_HandJoints[2] = { {0.9f, 0.0f, 0.7f}, 1 }; g_HandJoints[3] = { {1.1f, 0.1f, 0.9f}, 2 }; g_HandJoints[4] = { {1.2f, 0.2f, 1.1f}, 3 };
    g_HandJoints[5] = { {0.3f, 0.1f, 1.0f}, 0 }; g_HandJoints[6] = { {0.3f, 0.0f, 1.6f}, 5 }; g_HandJoints[7] = { {0.3f, 0.0f, 2.0f}, 6 }; g_HandJoints[8] = { {0.3f, 0.0f, 2.3f}, 7 };
    g_HandJoints[9] = { {0.0f, 0.1f, 1.1f}, 0 }; g_HandJoints[10] = { {0.0f, 0.0f, 1.8f}, 9 }; g_HandJoints[11] = { {0.0f, 0.0f, 2.3f}, 10 }; g_HandJoints[12] = { {0.0f, 0.0f, 2.6f}, 11 };
    g_HandJoints[13] = { {-0.3f, 0.1f, 1.0f}, 0 }; g_HandJoints[14] = { {-0.3f, 0.0f, 1.7f}, 13 }; g_HandJoints[15] = { {-0.3f, 0.0f, 2.1f}, 14 }; g_HandJoints[16] = { {-0.3f, 0.0f, 2.4f}, 15 };
    g_HandJoints[17] = { {-0.5f, 0.05f, 0.8f}, 0 }; g_HandJoints[18] = { {-0.5f, 0.0f, 1.3f}, 17 }; g_HandJoints[19] = { {-0.5f, 0.0f, 1.6f}, 18 }; g_HandJoints[20] = { {-0.5f, 0.0f, 1.8f}, 19 };
}
static void InitializeHand2() {
    g_HandJoints2.clear();
    g_HandJoints2.resize(21);
    float xOffset = 0.0f;
    float flipSign = -1.0f;
    g_HandJoints2[0] = { {xOffset + 0.0f * flipSign, 0.0f, 0.0f}, -1 };
    g_HandJoints2[1] = { {xOffset + 0.6f * flipSign, -0.1f, 0.4f}, 0 }; g_HandJoints2[2] = { {xOffset + 0.9f * flipSign, 0.0f, 0.7f}, 1 }; g_HandJoints2[3] = { {xOffset + 1.1f * flipSign, 0.1f, 0.9f}, 2 }; g_HandJoints2[4] = { {xOffset + 1.2f * flipSign, 0.2f, 1.1f}, 3 };
    g_HandJoints2[5] = { {xOffset + 0.3f * flipSign, 0.1f, 1.0f}, 0 }; g_HandJoints2[6] = { {xOffset + 0.3f * flipSign, 0.0f, 1.6f}, 5 }; g_HandJoints2[7] = { {xOffset + 0.3f * flipSign, 0.0f, 2.0f}, 6 }; g_HandJoints2[8] = { {xOffset + 0.3f * flipSign, 0.0f, 2.3f}, 7 };
    g_HandJoints2[9] = { {xOffset + 0.0f * flipSign, 0.1f, 1.1f}, 0 }; g_HandJoints2[10] = { {xOffset + 0.0f * flipSign, 0.0f, 1.8f}, 9 }; g_HandJoints2[11] = { {xOffset + 0.0f * flipSign, 0.0f, 2.3f}, 10 }; g_HandJoints2[12] = { {xOffset + 0.0f * flipSign, 0.0f, 2.6f}, 11 };
    g_HandJoints2[13] = { {xOffset + -0.3f * flipSign, 0.1f, 1.0f}, 0 }; g_HandJoints2[14] = { {xOffset + -0.3f * flipSign, 0.0f, 1.7f}, 13 }; g_HandJoints2[15] = { {xOffset + -0.3f * flipSign, 0.0f, 2.1f}, 14 }; g_HandJoints2[16] = { {xOffset + -0.3f * flipSign, 0.0f, 2.4f}, 15 };
    g_HandJoints2[17] = { {xOffset + -0.5f * flipSign, 0.05f, 0.8f}, 0 }; g_HandJoints2[18] = { {xOffset + -0.5f * flipSign, 0.0f, 1.3f}, 17 }; g_HandJoints2[19] = { {xOffset + -0.5f * flipSign, 0.0f, 1.6f}, 18 }; g_HandJoints2[20] = { {xOffset + -0.5f * flipSign, 0.0f, 1.8f}, 19 };
}
static void InitializeArm() {
    g_ArmJoints.clear();
    g_ArmJoints.resize(6);
    g_ArmJoints[0] = { {0.0f, 0.0f, 0.1f}, -1 };
    g_ArmJoints[1] = { {0.15f, 0.08f, 2.0f}, 0 };
    g_ArmJoints[2] = { {0.25f, 0.12f, 4.2f}, 1 };
    g_ArmJoints[3] = { {0.30f, 0.15f, 6.8f}, 2 };
    g_ArmJoints[4] = { {0.35f, 0.20f, 9.5f}, 3 };
    g_ArmJoints[5] = { {0.28f, 0.35f, 12.0f}, 4 };
}
static void InitializeArm2() {
    g_ArmJoints2.clear();
    g_ArmJoints2.resize(6);
    float xOffset = 0.0f;
    float flipSign = -1.0f;
    g_ArmJoints2[0] = { {xOffset + 0.0f * flipSign, 0.0f, 0.1f}, -1 };
    g_ArmJoints2[1] = { {xOffset + 0.15f * flipSign, 0.08f, 2.0f}, 0 };
    g_ArmJoints2[2] = { {xOffset + 0.25f * flipSign, 0.12f, 4.2f}, 1 };
    g_ArmJoints2[3] = { {xOffset + 0.30f * flipSign, 0.15f, 6.8f}, 2 };
    g_ArmJoints2[4] = { {xOffset + 0.35f * flipSign, 0.20f, 9.5f}, 3 };
    g_ArmJoints2[5] = { {xOffset + 0.28f * flipSign, 0.35f, 12.0f}, 4 };
}
void initializeCharacterParts() {
    buildTriangles();
    computeVertexNormals();
    InitializeHand();
    InitializeHand2();
    InitializeArm();
    InitializeArm2();
}

// ===================================================================
//
// SECTION 3: DRAWING ROUTINES
//
// ===================================================================

// --- Leg Drawing ---
void drawLeg(bool mirrorX, float swingAngle) {
    glPushMatrix();
    const float hipHeight = 8.2f * LEG_SCALE;
    glTranslatef(0.0f, hipHeight, 0.0f);
    glRotatef(swingAngle, 1.0f, 0.0f, 0.0f);
    glTranslatef(0.0f, -hipHeight, 0.0f);

    float mirror = mirrorX ? -1.0f : 1.0f;
    glColor3f(0.85f, 0.64f, 0.52f);
    glBegin(GL_TRIANGLES);
    for (const auto& t : gTris) {
        const Vec3f& n0 = gVertexNormals[t.a]; const Vec3f& v0 = gFootVertices[t.a];
        glNormal3f(n0.x * mirror, n0.y, n0.z); glVertex3f(v0.x * mirror, v0.y * LEG_SCALE, v0.z);
        const Vec3f& n1 = gVertexNormals[t.b]; const Vec3f& v1 = gFootVertices[t.b];
        glNormal3f(n1.x * mirror, n1.y, n1.z); glVertex3f(v1.x * mirror, v1.y * LEG_SCALE, v1.z);
        const Vec3f& n2 = gVertexNormals[t.c]; const Vec3f& v2 = gFootVertices[t.c];
        glNormal3f(n2.x * mirror, n2.y, n2.z); glVertex3f(v2.x * mirror, v2.y * LEG_SCALE, v2.z);
    }
    glEnd();
    glPopMatrix();
}

// --- Skirt Drawing ---
void drawSkirt(float leftLegAngle, float rightLegAngle) {
    const float topR = R8 + 0.1f; const float topY = Y8 - 0.51f;
    const float botR = 0.7f; const float botY = -1.7f;
    float deltaY = topY - botY; float deltaR = botR - topR;
    glColor3f(0.2f, 0.4f, 0.8f);
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < 16; ++i) {
        int next_i = (i + 1) % 16;
        float cA = segCos[i], sA = segSin[i];
        float cB = segCos[next_i], sB = segSin[next_i];
        Vec3f v_top_A = { topR * cA, topY, topR * sA }, v_bot_A = { botR * cA, botY, botR * sA };
        Vec3f v_top_B = { topR * cB, topY, topR * sB }, v_bot_B = { botR * cB, botY, botR * sB };

        float front_strength = 1.0f; float back_strength = 1.5f;
        float angleA = (v_bot_A.x <= 0) ? leftLegAngle : rightLegAngle;
        if (angleA > 0 && v_bot_A.z > 0) { v_bot_A.z += sin(angleA * PI / 180.0f) * front_strength; }
        else if (angleA < 0 && v_bot_A.z < 0) { v_bot_A.z += sin(angleA * PI / 180.0f) * back_strength; }
        float angleB = (v_bot_B.x <= 0) ? leftLegAngle : rightLegAngle;
        if (angleB > 0 && v_bot_B.z > 0) { v_bot_B.z += sin(angleB * PI / 180.0f) * front_strength; }
        else if (angleB < 0 && v_bot_B.z < 0) { v_bot_B.z += sin(angleB * PI / 180.0f) * back_strength; }

        Vec3f nA = normalize({ deltaY * cA, deltaR, deltaY * sA }), nB = normalize({ deltaY * cB, deltaR, deltaY * sB });
        glNormal3f(nA.x, nA.y, nA.z); glVertex3f(v_top_A.x, v_top_A.y, v_top_A.z);
        glNormal3f(nA.x, nA.y, nA.z); glVertex3f(v_bot_A.x, v_bot_A.y, v_bot_A.z);
        glNormal3f(nB.x, nB.y, nB.z); glVertex3f(v_bot_B.x, v_bot_B.y, v_bot_B.z);
        glNormal3f(nA.x, nA.y, nA.z); glVertex3f(v_top_A.x, v_top_A.y, v_top_A.z);
        glNormal3f(nB.x, nB.y, nB.z); glVertex3f(v_bot_B.x, v_bot_B.y, v_bot_B.z);
        glNormal3f(nB.x, nB.y, nB.z); glVertex3f(v_top_B.x, v_top_B.y, v_top_B.z);
    }
    glEnd();
}

// --- Body and Head Drawing ---
void drawFaceFeatures() {
    const float eyeY = HEAD_CENTER_Y + HEAD_RADIUS * 0.05f; const float eyeZ = HEAD_RADIUS * 0.86f; const float eyeR = 0.026f;
    glColor3f(0.1f, 0.1f, 0.1f);
    glBegin(GL_POLYGON); for (float a = 0; a < 6.28; a += 0.5)glVertex3f(-0.09f + eyeR * cos(a), eyeY + eyeR * sin(a), eyeZ); glEnd();
    glBegin(GL_POLYGON); for (float a = 0; a < 6.28; a += 0.5)glVertex3f(0.09f + eyeR * cos(a), eyeY + eyeR * sin(a), eyeZ); glEnd();
    glColor3f(0.8f, 0.2f, 0.2f);
    glBegin(GL_LINE_STRIP); glVertex3f(-0.08f, eyeY - 0.10f, eyeZ); glVertex3f(-0.05f, eyeY - 0.12f, eyeZ); glVertex3f(0.05f, eyeY - 0.12f, eyeZ); glVertex3f(0.08f, eyeY - 0.10f, eyeZ); glEnd();
}
void drawHeadSphere() {
    if (!g_headQuadric)return; glPushMatrix(); glColor3f(0.9f, 0.7f, 0.6f);
    GLdouble eq[4] = { 0,1,0,-Y18 }; glClipPlane(GL_CLIP_PLANE0, eq); glEnable(GL_CLIP_PLANE0);
    glTranslatef(0, HEAD_CENTER_Y, 0); gluSphere(g_headQuadric, HEAD_RADIUS, 32, 24);
    glDisable(GL_CLIP_PLANE0); glPopMatrix();
}
void drawTorso() {
    glColor3f(0.2f, 0.4f, 0.8f); glBegin(GL_TRIANGLES);
    BAND(R0, Y0, R1, Y1); BAND(R1, Y1, R2, Y2); BAND(R2, Y2, R3, Y3); BAND(R3, Y3, R4, Y4); BAND(R4, Y4, R5, Y5); BAND(R5, Y5, R6, Y6); BAND(R6, Y6, R7, Y7); BAND(R7, Y7, R8, Y8);
    BAND(R8, Y8, R9, Y9); BAND(R9, Y9, R10, Y10); BAND(R10, Y10, R11, Y11); BAND(R11, Y11, R12, Y12); BAND(R12, Y12, R13, Y13); BAND(R13, Y13, R14, Y14); BAND(R14, Y14, R15, Y15);
    glEnd();
    glColor3f(0.9f, 0.7f, 0.6f); glBegin(GL_TRIANGLES);
    BAND(R15, Y15, R16, Y16); BAND(R16, Y16, R17, Y17); BAND(R17, Y17, R18, Y18);
    glEnd(); drawHeadSphere();
}

// --- Arm and Hand Drawing (Your Original Functions) ---
static void drawRobotJoint(const Vec3& pos, float size) {
    glPushMatrix();
    glTranslatef(pos.x, pos.y, pos.z);
    float half = size * 0.5f;
    glBegin(GL_TRIANGLES);
    glNormal3f(0, 0, 1);
    glVertex3f(-half, -half, half); glVertex3f(half, -half, half); glVertex3f(half, half, half);
    glVertex3f(-half, -half, half); glVertex3f(half, half, half); glVertex3f(-half, half, half);
    glNormal3f(0, 0, -1);
    glVertex3f(half, -half, -half); glVertex3f(-half, -half, -half); glVertex3f(-half, half, -half);
    glVertex3f(half, -half, -half); glVertex3f(-half, half, -half); glVertex3f(half, half, -half);
    glNormal3f(1, 0, 0);
    glVertex3f(half, -half, half); glVertex3f(half, -half, -half); glVertex3f(half, half, -half);
    glVertex3f(half, -half, half); glVertex3f(half, half, -half); glVertex3f(half, half, half);
    glNormal3f(-1, 0, 0);
    glVertex3f(-half, -half, -half); glVertex3f(-half, -half, half); glVertex3f(-half, half, half);
    glVertex3f(-half, -half, -half); glVertex3f(-half, half, half); glVertex3f(-half, half, -half);
    glNormal3f(0, 1, 0);
    glVertex3f(-half, half, half); glVertex3f(half, half, half); glVertex3f(half, half, -half);
    glVertex3f(-half, half, half); glVertex3f(half, half, -half); glVertex3f(-half, half, -half);
    glNormal3f(0, -1, 0);
    glVertex3f(-half, -half, -half); glVertex3f(half, -half, -half); glVertex3f(half, -half, half);
    glVertex3f(-half, -half, -half); glVertex3f(half, -half, half); glVertex3f(-half, -half, half);
    glEnd();
    glPopMatrix();
}
static void drawAnatomicalArmSegment(const Vec3& start, const Vec3& end, float baseWidth, float baseHeight, float muscleWidth, float boneWidth) {
    Vec3 dir = { end.x - start.x, end.y - start.y, end.z - start.z };
    float length = sqrtf(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
    if (length < 0.001f) return;
    dir.x /= length; dir.y /= length; dir.z /= length;

    glPushMatrix();
    glTranslatef(start.x, start.y, start.z);
    float pitch = asinf(-dir.y) * 180.0f / PI;
    float yaw = atan2f(dir.x, dir.z) * 180.0f / PI;
    glRotatef(yaw, 0, 1, 0);
    glRotatef(pitch, 1, 0, 0);

    glColor3f(0.85f, 0.64f, 0.52f);

    float skinHalf = baseWidth * 0.5f;
    float skinHeight = baseHeight * 0.5f;
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < 12; i++) {
        float angle1 = (float)i * 2.0f * PI / 12.0f;
        float angle2 = (float)(i + 1) * 2.0f * PI / 12.0f;
        float x1 = cosf(angle1) * skinHalf;
        float y1 = sinf(angle1) * skinHeight;
        float x2 = cosf(angle2) * skinHalf;
        float y2 = sinf(angle2) * skinHeight;
        glNormal3f(cosf(angle1), sinf(angle1), 0);
        glVertex3f(x1, y1, 0);
        glVertex3f(x2, y2, 0);
        glVertex3f(x1, y1, length);
        glVertex3f(x1, y1, length);
        glVertex3f(x2, y2, 0);
        glVertex3f(x2, y2, length);
    }
    glEnd();
    glPopMatrix();
}
static void drawLowPolyArm(const std::vector<ArmJoint>& armJoints) {
    if (armJoints.empty()) return;
    drawAnatomicalArmSegment({ 0,0,0 }, armJoints[1].position, 0.9f, 0.7f, 0.7f, 0.5f);
    drawAnatomicalArmSegment(armJoints[1].position, armJoints[2].position, 1.0f, 0.8f, 0.8f, 0.6f);
    drawAnatomicalArmSegment(armJoints[2].position, armJoints[3].position, 1.1f, 0.9f, 0.9f, 0.65f);
    drawAnatomicalArmSegment(armJoints[3].position, armJoints[4].position, 1.05f, 0.85f, 0.85f, 0.7f);
    drawAnatomicalArmSegment(armJoints[4].position, armJoints[5].position, 1.3f, 1.0f, 1.1f, 0.8f);

    for (size_t i = 1; i < armJoints.size(); ++i) {
        drawRobotJoint(armJoints[i].position, 0.15f + i * 0.03f);
    }
}
static void drawSkinnedPalm(const std::vector<HandJoint>& joints) {
    Vec3 wrist = joints[0].position;
    Vec3 thumbBase = joints[1].position;
    Vec3 indexBase = joints[5].position;
    Vec3 middleBase = joints[9].position;
    Vec3 ringBase = joints[13].position;
    Vec3 pinkyBase = joints[17].position;
    Vec3 palmVerts[] = {
        {wrist.x - 0.5f, wrist.y, wrist.z - 0.1f},
        {wrist.x + 0.5f, wrist.y, wrist.z - 0.1f},
        {pinkyBase.x - 0.05f, pinkyBase.y + 0.05f, pinkyBase.z - 0.4f},
        {ringBase.x - 0.05f, ringBase.y + 0.08f, ringBase.z - 0.4f},
        {middleBase.x - 0.05f, middleBase.y + 0.1f, middleBase.z - 0.4f},
        {indexBase.x - 0.05f, indexBase.y + 0.08f, indexBase.z - 0.4f},
        {thumbBase.x - 0.1f, thumbBase.y + 0.0f, thumbBase.z - 0.3f}
    };

    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0, 1, 0.2f);
    glVertex3f(wrist.x, wrist.y + 0.1f, wrist.z + 0.2f);
    for (int i = 0; i < 7; ++i) { glVertex3f(palmVerts[i].x, palmVerts[i].y, palmVerts[i].z); }
    glVertex3f(palmVerts[0].x, palmVerts[0].y, palmVerts[0].z);
    glEnd();

    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0, -1, -0.2f);
    glVertex3f(wrist.x, wrist.y - 0.3f, wrist.z);
    glVertex3f(palmVerts[0].x, palmVerts[0].y - 0.4f, palmVerts[0].z);
    for (int i = 6; i >= 0; --i) { glVertex3f(palmVerts[i].x, palmVerts[i].y - 0.4f, palmVerts[i].z); }
    glEnd();
}
static void drawLowPolyFinger(const std::vector<HandJoint>& joints, int mcp, int pip, int dip, int tip) {
    drawAnatomicalArmSegment(joints[mcp].position, joints[pip].position, 0.18, 0.18, 0.18, 0.18);
    drawAnatomicalArmSegment(joints[pip].position, joints[dip].position, 0.15, 0.15, 0.15, 0.15);
    drawAnatomicalArmSegment(joints[dip].position, joints[tip].position, 0.12, 0.12, 0.12, 0.12);
}
static void drawLowPolyThumb(const std::vector<HandJoint>& joints) {
    drawAnatomicalArmSegment(joints[1].position, joints[2].position, 0.20, 0.20, 0.20, 0.20);
    drawAnatomicalArmSegment(joints[2].position, joints[3].position, 0.17, 0.17, 0.17, 0.17);
    drawAnatomicalArmSegment(joints[3].position, joints[4].position, 0.14, 0.14, 0.14, 0.14);
}
void drawArmsAndHands(float leftArmAngle, float rightArmAngle) {
    glColor3f(0.85f, 0.64f, 0.52f);

    // --- Draw Left Arm ---
    glPushMatrix();
    glTranslatef(-R16 - 0.2f, Y16, 0.5f);
    glRotatef(-45.0f, 0.0f, 1.0f, 0.0f);
    glRotatef(15.0f, 1.0f, 0.0f, 0.0f);
    glRotatef(leftArmAngle, 1.0f, 0.0f, 0.0f);

    // Draw the arm with ARM_SCALE
    glPushMatrix();
    glScalef(ARM_SCALE, ARM_SCALE, ARM_SCALE);
    drawLowPolyArm(g_ArmJoints);
    glPopMatrix();

    // Move to the wrist position, taking ARM_SCALE into account
    Vec3 leftWristPos = g_ArmJoints.back().position;
    glTranslatef(leftWristPos.x * ARM_SCALE, leftWristPos.y * ARM_SCALE, leftWristPos.z * ARM_SCALE);

    // Draw the hand with HAND_SCALE
    glPushMatrix();
    glScalef(HAND_SCALE, HAND_SCALE, HAND_SCALE);
    drawSkinnedPalm(g_HandJoints);
    drawLowPolyThumb(g_HandJoints);
    drawLowPolyFinger(g_HandJoints, 5, 6, 7, 8);
    drawLowPolyFinger(g_HandJoints, 9, 10, 11, 12);
    drawLowPolyFinger(g_HandJoints, 13, 14, 15, 16);
    drawLowPolyFinger(g_HandJoints, 17, 18, 19, 20);
    glPopMatrix();

    glPopMatrix();

    // --- Draw Right Arm ---
    glPushMatrix();
    glTranslatef(R16 + 0.2f, Y16, 0.5f);
    glRotatef(45.0f, 0.0f, 1.0f, 0.0f);
    glRotatef(15.0f, 1.0f, 0.0f, 0.0f);
    glRotatef(rightArmAngle, 1.0f, 0.0f, 0.0f);

    // Draw the arm with ARM_SCALE
    glPushMatrix();
    glScalef(ARM_SCALE, ARM_SCALE, ARM_SCALE);
    drawLowPolyArm(g_ArmJoints2);
    glPopMatrix();

    // Move to the wrist position, taking ARM_SCALE into account
    Vec3 rightWristPos = g_ArmJoints2.back().position;
    glTranslatef(rightWristPos.x * ARM_SCALE, rightWristPos.y * ARM_SCALE, rightWristPos.z * ARM_SCALE);

    // Draw the hand with HAND_SCALE
    glPushMatrix();
    glScalef(HAND_SCALE, HAND_SCALE, HAND_SCALE);
    drawSkinnedPalm(g_HandJoints2);
    drawLowPolyThumb(g_HandJoints2);
    drawLowPolyFinger(g_HandJoints2, 5, 6, 7, 8);
    drawLowPolyFinger(g_HandJoints2, 9, 10, 11, 12);
    drawLowPolyFinger(g_HandJoints2, 13, 14, 15, 16);
    drawLowPolyFinger(g_HandJoints2, 17, 18, 19, 20);
    glPopMatrix();

    glPopMatrix();
}
// --- Main Character Drawing ---
void drawBodyAndHead(float leftLegAngle, float rightLegAngle) {
    glPushMatrix();
    glTranslatef(0.0f, 8.2f * LEG_SCALE, 0.0f);
    glScalef(BODY_SCALE, BODY_SCALE, BODY_SCALE);
    glRotatef(g_pose.torsoYaw, 0, 1, 0); glRotatef(g_pose.torsoPitch, 1, 0, 0); glRotatef(g_pose.torsoRoll, 0, 0, 1);

    drawArmsAndHands(-leftLegAngle, -rightLegAngle);
    drawTorso();
    drawSkirt(leftLegAngle, rightLegAngle);

    glDisable(GL_LIGHTING); glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glPushMatrix();
    glTranslatef(0, HEAD_CENTER_Y, 0);
    glRotatef(g_pose.headYaw, 0, 1, 0); glRotatef(g_pose.headPitch, 1, 0, 0); glRotatef(g_pose.headRoll, 0, 0, 1);
    glTranslatef(0, -HEAD_CENTER_Y, 0);
    drawFaceFeatures();
    glPopMatrix();
    glEnable(GL_LIGHTING);
    glPopMatrix();
}

void display() {
    glClearColor(0.6f, 0.3f, 0.7f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluPerspective(45.0, (double)gWidth / gHeight, 0.1, 100.0);

    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    Vec3 eye; { eye.x = gTarget.x + gDist * cos(gPitch) * sin(gYaw); eye.y = gTarget.y + gDist * sin(gPitch); eye.z = gTarget.z + gDist * cos(gPitch) * cos(gYaw); }
    gluLookAt(eye.x, eye.y, eye.z, gTarget.x, gTarget.y, gTarget.z, 0.0, 1.0, 0.0);

    glPolygonMode(GL_FRONT_AND_BACK, gShowWireframe ? GL_LINE : GL_FILL);

    glEnable(GL_LIGHTING); glEnable(GL_LIGHT0); glEnable(GL_COLOR_MATERIAL);
    float lightPos[] = { 20.f,20.f,30.f,1.f }; float ambientLight[] = { 0.4f,0.4f,0.4f,1.f }; float diffuseLight[] = { 0.7f,0.7f,0.7f,1.f };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos); glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight); glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);

    glPushMatrix();
    glTranslatef(gCharacterPos.x, gCharacterPos.y, gCharacterPos.z);
    glRotatef(gCharacterYaw, 0.0f, 1.0f, 0.0f);

    float legSwing = 0.0f;
    float animSpeed = keyShift ? 1.8f : 1.0f;
    if (abs(gMoveSpeed) > 0.1f) {
        legSwing = 25.0f * sin(gWalkPhase * animSpeed);
    }
    float leftLegSwing = -legSwing;
    float rightLegSwing = legSwing;

    glPushMatrix(); glTranslatef(-0.4f, 0, 0); drawLeg(false, leftLegSwing); glPopMatrix();
    glPushMatrix(); glTranslatef(0.4f, 0, 0); drawLeg(true, rightLegSwing); glPopMatrix();
    drawBodyAndHead(leftLegSwing, rightLegSwing);

    glPopMatrix();
}

// ===================================================================
//
// SECTION 4: WIN32 APPLICATION FRAMEWORK
//
// ===================================================================

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nCmdShow) {
    WNDCLASSEX wc{}; wc.cbSize = sizeof(WNDCLASSEX); wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC; wc.lpfnWndProc = WindowProcedure; wc.hInstance = GetModuleHandle(NULL); wc.lpszClassName = WINDOW_TITLE;
    if (!RegisterClassEx(&wc))return 0;
    HWND hWnd = CreateWindow(WINDOW_TITLE, WINDOW_TITLE, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, gWidth, gHeight, NULL, NULL, wc.hInstance, NULL);
    if (!hWnd)return 0;
    HDC hdc = GetDC(hWnd);
    { PIXELFORMATDESCRIPTOR pfd{}; pfd.nSize = sizeof(pfd); pfd.nVersion = 1; pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER; pfd.iPixelType = PFD_TYPE_RGBA; pfd.cColorBits = 32; pfd.cDepthBits = 24; pfd.iLayerType = PFD_MAIN_PLANE; int pf = ChoosePixelFormat(hdc, &pfd); SetPixelFormat(hdc, pf, &pfd); }
    HGLRC rc = wglCreateContext(hdc); wglMakeCurrent(hdc, rc);
    ShowWindow(hWnd, nCmdShow); UpdateWindow(hWnd);

    glEnable(GL_DEPTH_TEST); glEnable(GL_NORMALIZE);
    initializeCharacterParts();
    g_headQuadric = gluNewQuadric(); if (g_headQuadric)gluQuadricNormals(g_headQuadric, GLU_SMOOTH); if (g_headQuadric)gluQuadricDrawStyle(g_headQuadric, gShowWireframe ? GLU_LINE : GLU_FILL);
    QueryPerformanceFrequency(&gFreq); QueryPerformanceCounter(&gPrev);

    MSG msg{};
    while (gRunning) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) { if (msg.message == WM_QUIT)gRunning = false; TranslateMessage(&msg); DispatchMessage(&msg); }
        LARGE_INTEGER now; QueryPerformanceCounter(&now); float dt = float(now.QuadPart - gPrev.QuadPart) / float(gFreq.QuadPart); gPrev = now;
        if (dt > 0.1f) dt = 0.1f;

        updateCharacter(dt);

        { Vec3 eye; {eye.x = gTarget.x + gDist * cos(gPitch) * sin(gYaw); eye.y = gTarget.y + gDist * sin(gPitch); eye.z = gTarget.z + gDist * cos(gPitch) * cos(gYaw); } Vec3 f = { gTarget.x - eye.x,gTarget.y - eye.y,gTarget.z - eye.z }; float fl = sqrt(f.x * f.x + f.y * f.y + f.z * f.z); if (fl > 1e-6) { f.x /= fl; f.y /= fl; f.z /= fl; } Vec3 r = { f.z,0,-f.x }; float moveStep = gDist * 0.8f * dt; if (keyW)gTarget.y += moveStep; if (keyS)gTarget.y -= moveStep; if (keyA) { gTarget.x -= r.x * moveStep; gTarget.z -= r.z * moveStep; }if (keyD) { gTarget.x += r.x * moveStep; gTarget.z += r.z * moveStep; } }
        display(); SwapBuffers(hdc);
    }

    if (g_headQuadric) { gluDeleteQuadric(g_headQuadric); g_headQuadric = nullptr; } wglMakeCurrent(NULL, NULL); wglDeleteContext(rc); ReleaseDC(hWnd, hdc); UnregisterClass(WINDOW_TITLE, wc.hInstance);
    return 0;
}

void updateCharacter(float dt) {
    float targetSpeed = 0.0f;
    if (keyUp) { targetSpeed = keyShift ? RUN_SPEED : WALK_SPEED; }
    if (keydown) { targetSpeed = -(keyShift ? RUN_SPEED : WALK_SPEED); }
    gMoveSpeed = targetSpeed;
    if (keyLeft) { gCharacterYaw += TURN_SPEED * dt; }
    if (keyRight) { gCharacterYaw -= TURN_SPEED * dt; }
    if (abs(gMoveSpeed) > 0.01f) {
        float angleRad = gCharacterYaw * PI / 180.0f;
        gCharacterPos.x -= sin(angleRad) * gMoveSpeed * dt;
        gCharacterPos.z -= cos(angleRad) * gMoveSpeed * dt;
        float phaseSpeed = keyShift ? 10.0f : 5.0f;
        gWalkPhase += gMoveSpeed * dt * phaseSpeed / (keyShift ? 2.5f : 2.0f);
    }
}

LRESULT WINAPI WindowProcedure(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_DESTROY: PostQuitMessage(0); return 0;
    case WM_SIZE: gWidth = LOWORD(lParam); gHeight = HIWORD(lParam); if (gWidth <= 0)gWidth = 1; if (gHeight <= 0)gHeight = 1; glViewport(0, 0, gWidth, gHeight); return 0;
    case WM_LBUTTONDOWN: gLMBDown = true; SetCapture(hWnd); gLastMouse.x = LOWORD(lParam); gLastMouse.y = HIWORD(lParam); return 0;
    case WM_LBUTTONUP: gLMBDown = false; ReleaseCapture(); return 0;
    case WM_MOUSEMOVE: if (gLMBDown) { int mx = LOWORD(lParam), my = HIWORD(lParam); int dx = mx - gLastMouse.x, dy = my - gLastMouse.y; gLastMouse.x = mx; gLastMouse.y = my; gYaw += dx * 0.005f; gPitch -= dy * 0.005f; if (gPitch > 1.55f)gPitch = 1.55f; if (gPitch < -1.55f)gPitch = -1.55f; } return 0;
    case WM_MOUSEWHEEL: {short delta = GET_WHEEL_DELTA_WPARAM(wParam); gDist *= (1.0f - (delta / 120.0f) * 0.1f); if (gDist < 2.0f)gDist = 2.0f; if (gDist > 100.0f)gDist = 100.0f; } return 0;
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)PostQuitMessage(0);
        else if (wParam == 'R') { gYaw = 0.2f; gPitch = 0.1f; gDist = 15.0f; gTarget = { 0.0f,3.5f,0.0f }; gCharacterPos = { 0,0,0 }; gCharacterYaw = 0; }
        else if (wParam == '1') { gShowWireframe = !gShowWireframe; if (g_headQuadric)gluQuadricDrawStyle(g_headQuadric, gShowWireframe ? GLU_LINE : GLU_FILL); }
        else if (wParam == 'W') keyW = true; else if (wParam == 'S') keyS = true;
        else if (wParam == 'A') keyA = true; else if (wParam == 'D') keyD = true;
        else if (wParam == VK_UP) keyUp = true; else if (wParam == VK_DOWN) keydown = true;
        else if (wParam == VK_LEFT) keyLeft = true; else if (wParam == VK_RIGHT) keyRight = true;
        else if (wParam == VK_SHIFT) keyShift = true;
        return 0;
    case WM_KEYUP:
        if (wParam == 'W') keyW = false; else if (wParam == 'S') keyS = false;
        else if (wParam == 'A') keyA = false; else if (wParam == 'D') keyD = false;
        else if (wParam == VK_UP) keyUp = false; else if (wParam == VK_DOWN) keydown = false;
        else if (wParam == VK_LEFT) keyLeft = false; else if (wParam == VK_RIGHT) keyRight = false;
        else if (wParam == VK_SHIFT) keyShift = false;
        return 0;
    default: return DefWindowProc(hWnd, msg, wParam, lParam);
    }
}
