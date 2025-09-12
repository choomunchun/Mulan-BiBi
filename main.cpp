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

// Define GL_BGR if not available
#ifndef GL_BGR
#define GL_BGR 0x80E0
#endif

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
#define LEG_SCALE 0.9f  // Shorter legs for better body-to-leg ratio
#define BODY_SCALE 3.5f
#define ARM_SCALE 0.10f
#define HAND_SCALE 0.15f

// --- Animation & Character State ---
Vec3  gCharacterPos = { 0.0f, 0.0f, 0.0f };
float gCharacterYaw = 0.0f;
float gWalkPhase = 0.0f;
float gMoveSpeed = 0.0f;
const float WALK_SPEED = 3.0f;
const float RUN_SPEED = 1.0f;
const float TURN_SPEED = 120.0f;

// =========================================================
// == 📍 NEW: ANIMATION AMPLITUDE CONTROLS ==
// =========================================================
const float WALK_THIGH_SWING = 10.0f;    // Walking thigh swing angle
const float RUN_THIGH_SWING = 25.0f;     // Running thigh swing angle
const float WALK_ARM_SWING = 35.0f;      // Walking arm swing angle
const float RUN_ARM_SWING = 55.0f;       // Running arm swing angle
const float BODY_BOB_AMOUNT = 0.03f;     // How much the body moves up and down
const float PLANTED_LEG_FLEX_AMOUNT = 15.0f; // How much the planted leg bends to absorb impact
const float SWINGING_LEG_KNEE_BEND = -30.0f; // How much the forward-swinging leg bends at the knee

// --- Data Structures ---
struct Vec3f { float x, y, z; };
struct Tri { int a, b, c; };
struct JointPose { float torsoYaw, torsoPitch, torsoRoll; float headYaw, headPitch, headRoll; } g_pose = { 0,0,0, 0,0,0 };
struct HandJoint { Vec3 position; int parentIndex; };
struct ArmJoint { Vec3 position; int parentIndex; };

// --- Texture Support ---
GLuint g_HandTexture = 0; // texture name
BITMAP BMP; // bitmap structure
HBITMAP hBMP = NULL; // bitmap handle

// Texture enabled flag
bool g_TextureEnabled = true;

// =========================== Texture Support ===========================
// Texture loading function
GLuint loadTexture(LPCSTR filename) {
    GLuint texture = 0;

    // Initialize texture info
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    HBITMAP hBMP = (HBITMAP)LoadImageA(GetModuleHandle(NULL), filename, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION | LR_LOADFROMFILE);

    if (hBMP == NULL) {
        // If file not found, create a simple procedural texture
        return 0; // Return 0 to indicate no texture
    }

    GetObject(hBMP, sizeof(BMP), &BMP);

    // Assign texture to polygon
    glEnable(GL_TEXTURE_2D);
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, BMP.bmWidth, BMP.bmHeight, 0, GL_BGR, GL_UNSIGNED_BYTE, BMP.bmBits);

    DeleteObject(hBMP);
    return texture;
}

// Create a simple procedural skin texture in memory
GLuint createSkinTexture() {
    const int texWidth = 64;
    const int texHeight = 64;
    unsigned char* textureData = new unsigned char[texWidth * texHeight * 3];

    // Create a simple skin-colored pattern
    for (int y = 0; y < texHeight; y++) {
        for (int x = 0; x < texWidth; x++) {
            int index = (y * texWidth + x) * 3;

            // Base skin color with some variation
            float variation = (sinf(x * 0.2f) * cosf(y * 0.15f) + 1.0f) * 0.1f;

            textureData[index] = (unsigned char)(220 + variation * 35);      // R
            textureData[index + 1] = (unsigned char)(180 + variation * 25); // G  
            textureData[index + 2] = (unsigned char)(140 + variation * 20); // B
        }
    }

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texWidth, texHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, textureData);

    delete[] textureData;
    return texture;
}
// =============================================================

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
#define R0 0.50f  // Hip area - wider for better leg connection
#define R1 0.49f
#define R2 0.48f
#define R3 0.47f
#define R4 0.46f
#define R5 0.43f  // Waist area - slightly narrower for feminine shape
#define R6 0.40f
#define R7 0.37f
#define R8 0.35f  // Lower torso - wider for natural leg attachment
#define R9 0.37f
#define R10 0.39f
#define R11 0.41f  // Upper torso - broader for mature proportions
#define R12 0.45f  // Upper torso - broader for strong shoulders
#define R13 0.44f  // Pre-shoulder area - smooth transition
#define R14 0.43f  // Shoulder area - gentle rise
#define R15 0.40f  // Shoulder peak - feminine curve
#define R16 0.28f  // Upper shoulder slope - slimmer feminine transition
#define R17 0.16f  // Lower shoulder slope - elegant curve to neck
#define R18 0.09f  // Neck base - slim feminine neck
#define R19 0.07f  // Upper neck - connects to head
#define Y0 0.00f
#define Y1 0.08f
#define Y2 0.16f
#define Y3 0.24f
#define Y4 0.32f
#define Y5 0.45f
#define Y6 0.58f
#define Y7 0.70f
#define Y8 0.83f
#define Y9 0.96f
#define Y10 1.09f
#define Y11 1.22f
#define Y12 1.31f
#define Y13 1.41f
#define Y14 1.47f
#define Y15 1.54f
#define Y16 1.58f
#define Y17 1.66f  // Upper shoulder slope - feminine curve
#define Y18 1.76f  // Lower shoulder slope - elegant transition
#define Y19 1.88f  // Upper neck - connects to head
#define HEAD_CENTER_Y 1.92f  // Perfectly centered on torso middle, sits naturally on neck
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
void drawLegModel(bool mirrorX); // New helper function to draw the leg mesh
void drawLegModel(bool mirrorX); // Add this new declaration
void drawLeg(bool mirrorX, float thighAngle, float kneeAngle); // Modify this line
void drawBodyAndHead(float leftLegAngle, float rightLegAngle, float leftArmAngle, float rightArmAngle); // Updated signature
void drawSkirt(float leftLegAngle, float rightLegAngle);
void drawArmsAndHands(float leftArmAngle, float rightArmAngle);
void drawInternalShoulderJoints();
void drawCurvedBand(float rA, float yA, float rB, float yB);
void drawCustomFaceShape();
void drawMulanHead();
void drawMulanFace();
void drawMulanEyes();
void drawMulanEyebrows();
void drawMulanNose();
void drawMulanMouth();
void drawMulanHair();

// --- Math & Model Building ---
Vec3f sub(const Vec3f& p, const Vec3f& q) { return { p.x - q.x, p.y - q.y, p.z - q.z }; }
Vec3f cross(const Vec3f& a, const Vec3f& b) { return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x }; }
float dot(const Vec3f& a, const Vec3f& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
Vec3f normalize(const Vec3f& v) { float l = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z); return (l > 1e-6f) ? Vec3f{ v.x / l, v.y / l, v.z / l } : Vec3f{ 0,0,0 }; }
Vec3f faceNormal(int i0, int i1, int i2) { return normalize(cross(sub(gFootVertices[i1], gFootVertices[i0]), sub(gFootVertices[i2], gFootVertices[i0]))); }
Vec3f triCenter(int i0, int i1, int i2) { const Vec3f& v0 = gFootVertices[i0], & v1 = gFootVertices[i1], & v2 = gFootVertices[i2]; return { (v0.x + v1.x + v2.x) / 3.f,(v0.y + v1.y + v2.y) / 3.f,(v0.z + v1.z + v2.z) / 3.f }; }
void buildTriangles() {
    int nV = sizeof(gFootVertices) / sizeof(gFootVertices[0]); for (int i = 0; i < nV; ++i) { gModelCenter.x += gFootVertices[i].x; gModelCenter.y += gFootVertices[i].y; gModelCenter.z += gFootVertices[i].z; } gModelCenter.x /= nV; gModelCenter.y /= nV; gModelCenter.z /= nV; gTris.clear();
    for (const auto& q : gFootQuads) { if (q[3] == -1) { int a = q[0], b = q[1], c = q[2]; if (dot(faceNormal(a, b, c), sub(triCenter(a, b, c), gModelCenter)) < 0)std::swap(b, c); gTris.push_back({ a,b,c }); } else { int a = q[0], b = q[1], c = q[2], d = q[3]; { int i0 = a, i1 = b, i2 = d; if (dot(faceNormal(i0, i1, i2), sub(triCenter(i0, i1, i2), gModelCenter)) < 0)std::swap(i1, i2); gTris.push_back({ i0,i1,i2 }); } { int i0 = b, i1 = c, i2 = d; if (dot(faceNormal(i0, i1, i2), sub(triCenter(i0, i1, i2), gModelCenter)) < 0)std::swap(i1, i2); gTris.push_back({ i0,i1,i2 }); } } }
}
void computeVertexNormals() {
    int nV = sizeof(gFootVertices) / sizeof(gFootVertices[0]); gVertexNormals.assign(nV, { 0,0,0 });
    for (const auto& t : gTris) { Vec3f n = faceNormal(t.a, t.b, t.c); gVertexNormals[t.a].x += n.x; gVertexNormals[t.a].y += n.y; gVertexNormals[t.a].z += n.z; gVertexNormals[t.b].x += n.x; gVertexNormals[t.b].y += n.y; gVertexNormals[t.b].z += n.z; gVertexNormals[t.c].x += n.x; gVertexNormals[t.c].y += n.y; gVertexNormals[t.c].z += n.z; }
    for (int i = 0; i < nV; ++i)gVertexNormals[i] = normalize(gVertexNormals[i]);
}
static void InitializeHand() {
    g_HandJoints.clear();
    g_HandJoints.resize(21);

    // WRIST (0)
    g_HandJoints[0] = { {0.0f, 0.0f, 0.0f}, -1 };

    // THUMB chain (1-4)
    g_HandJoints[1] = { {0.6f, -0.1f, 0.4f}, 0 };
    g_HandJoints[2] = { {0.9f, 0.0f, 0.7f}, 1 };
    g_HandJoints[3] = { {1.1f, 0.1f, 0.9f}, 2 };
    g_HandJoints[4] = { {1.2f, 0.2f, 1.1f}, 3 };

    // INDEX FINGER chain (5-8)
    g_HandJoints[5] = { {0.3f, 0.1f, 1.0f}, 0 };
    g_HandJoints[6] = { {0.3f, 0.0f, 1.6f}, 5 };
    g_HandJoints[7] = { {0.3f, 0.0f, 2.0f}, 6 };
    g_HandJoints[8] = { {0.3f, 0.0f, 2.3f}, 7 };

    // MIDDLE FINGER chain (9-12)
    g_HandJoints[9] = { {0.0f, 0.1f, 1.1f}, 0 };
    g_HandJoints[10] = { {0.0f, 0.0f, 1.8f}, 9 };
    g_HandJoints[11] = { {0.0f, 0.0f, 2.3f}, 10 };
    g_HandJoints[12] = { {0.0f, 0.0f, 2.6f}, 11 };

    // RING FINGER chain (13-16)
    g_HandJoints[13] = { {-0.3f, 0.1f, 1.0f}, 0 };
    g_HandJoints[14] = { {-0.3f, 0.0f, 1.7f}, 13 };
    g_HandJoints[15] = { {-0.3f, 0.0f, 2.1f}, 14 };
    g_HandJoints[16] = { {-0.3f, 0.0f, 2.4f}, 15 };

    // PINKY chain (17-20)
    g_HandJoints[17] = { {-0.5f, 0.05f, 0.8f}, 0 };
    g_HandJoints[18] = { {-0.5f, 0.0f, 1.3f}, 17 };
    g_HandJoints[19] = { {-0.5f, 0.0f, 1.6f}, 18 };
    g_HandJoints[20] = { {-0.5f, 0.0f, 1.8f}, 19 };
}

static void InitializeHand2() {
    g_HandJoints2.clear();
    g_HandJoints2.resize(21);
    float flipSign = -1.0f;

    g_HandJoints2[0] = { {0.0f * flipSign, 0.0f, 0.0f}, -1 };
    g_HandJoints2[1] = { {0.6f * flipSign, -0.1f, 0.4f}, 0 };
    g_HandJoints2[2] = { {0.9f * flipSign, 0.0f, 0.7f}, 1 };
    g_HandJoints2[3] = { {1.1f * flipSign, 0.1f, 0.9f}, 2 };
    g_HandJoints2[4] = { {1.2f * flipSign, 0.2f, 1.1f}, 3 };
    g_HandJoints2[5] = { {0.3f * flipSign, 0.1f, 1.0f}, 0 };
    g_HandJoints2[6] = { {0.3f * flipSign, 0.0f, 1.6f}, 5 };
    g_HandJoints2[7] = { {0.3f * flipSign, 0.0f, 2.0f}, 6 };
    g_HandJoints2[8] = { {0.3f * flipSign, 0.0f, 2.3f}, 7 };
    g_HandJoints2[9] = { {0.0f * flipSign, 0.1f, 1.1f}, 0 };
    g_HandJoints2[10] = { {0.0f * flipSign, 0.0f, 1.8f}, 9 };
    g_HandJoints2[11] = { {0.0f * flipSign, 0.0f, 2.3f}, 10 };
    g_HandJoints2[12] = { {0.0f * flipSign, 0.0f, 2.6f}, 11 };
    g_HandJoints2[13] = { {-0.3f * flipSign, 0.1f, 1.0f}, 0 };
    g_HandJoints2[14] = { {-0.3f * flipSign, 0.0f, 1.7f}, 13 };
    g_HandJoints2[15] = { {-0.3f * flipSign, 0.0f, 2.1f}, 14 };
    g_HandJoints2[16] = { {-0.3f * flipSign, 0.0f, 2.4f}, 15 };
    g_HandJoints2[17] = { {-0.5f * flipSign, 0.05f, 0.8f}, 0 };
    g_HandJoints2[18] = { {-0.5f * flipSign, 0.0f, 1.3f}, 17 };
    g_HandJoints2[19] = { {-0.5f * flipSign, 0.0f, 1.6f}, 18 };
    g_HandJoints2[20] = { {-0.5f * flipSign, 0.0f, 1.8f}, 19 };
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
    float flipSign = -1.0f;
    g_ArmJoints2[0] = { {0.0f * flipSign, 0.0f, 0.1f}, -1 };
    g_ArmJoints2[1] = { {0.15f * flipSign, 0.08f, 2.0f}, 0 };
    g_ArmJoints2[2] = { {0.25f * flipSign, 0.12f, 4.2f}, 1 };
    g_ArmJoints2[3] = { {0.30f * flipSign, 0.15f, 6.8f}, 2 };
    g_ArmJoints2[4] = { {0.35f * flipSign, 0.20f, 9.5f}, 3 };
    g_ArmJoints2[5] = { {0.28f * flipSign, 0.35f, 12.0f}, 4 };
}

void initializeCharacterParts() {
    buildTriangles();
    computeVertexNormals();
    InitializeHand();
    InitializeHand2();
    InitializeArm();
    InitializeArm2();
    g_HandTexture = loadTexture("skin.bmp");
    if (g_HandTexture == 0) {
        g_HandTexture = createSkinTexture();
    }
}

// =============== ADVANCED HAND & ARM DRAWING FUNCTIONS ===============

// Draw a sphere at a joint position
static void drawJoint(const Vec3& pos, float radius) {
    glPushMatrix();
    glTranslatef(pos.x, pos.y, pos.z);

    // Simple sphere using triangular strips
    const int slices = 8;
    const int stacks = 6;

    for (int i = 0; i < stacks; ++i) {
        float lat0 = PI * (-0.5f + (float)i / stacks);
        float lat1 = PI * (-0.5f + (float)(i + 1) / stacks);
        float y0 = radius * sinf(lat0);
        float y1 = radius * sinf(lat1);
        float r0 = radius * cosf(lat0);
        float r1 = radius * cosf(lat1);

        glBegin(GL_TRIANGLE_STRIP);
        for (int j = 0; j <= slices; ++j) {
            float lng = 2 * PI * (float)j / slices;
            float x0 = r0 * cosf(lng);
            float z0 = r0 * sinf(lng);
            float x1 = r1 * cosf(lng);
            float z1 = r1 * sinf(lng);

            glNormal3f(x0 / radius, y0 / radius, z0 / radius);
            glVertex3f(x0, y0, z0);
            glNormal3f(x1 / radius, y1 / radius, z1 / radius);
            glVertex3f(x1, y1, z1);
        }
        glEnd();
    }

    glPopMatrix();
}

// Draw anatomically complex arm segment with texture
static void drawAnatomicalArmSegment(const Vec3& start, const Vec3& end, float baseWidth, float baseHeight,
    float muscleWidth, float boneWidth) {
    Vec3 dir = { end.x - start.x, end.y - start.y, end.z - start.z };
    float length = sqrtf(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);

    if (length < 0.001f) return;

    // Normalize direction
    dir.x /= length; dir.y /= length; dir.z /= length;

    glPushMatrix();
    glTranslatef(start.x, start.y, start.z);

    // Align with direction vector
    float pitch = asinf(-dir.y) * 180.0f / PI;
    float yaw = atan2f(dir.x, dir.z) * 180.0f / PI;
    glRotatef(yaw, 0, 1, 0);
    glRotatef(pitch, 1, 0, 0);

    // Create smooth elliptical outer surface with texture mapping
    float skinHalf = baseWidth * 0.5f;
    float skinHeight = baseHeight * 0.5f;

    glBegin(GL_TRIANGLES);
    for (int i = 0; i < 12; i++) {
        float angle1 = (float)i * 2.0f * PI / 12.0f;
        float angle2 = (float)(i + 1) * 2.0f * PI / 12.0f;

        // Elliptical cross-section for natural arm shape
        float x1 = cosf(angle1) * skinHalf;
        float y1 = sinf(angle1) * skinHeight;
        float x2 = cosf(angle2) * skinHalf;
        float y2 = sinf(angle2) * skinHeight;

        // Texture coordinates for skin surface
        float u1 = (float)i / 12.0f;
        float u2 = (float)(i + 1) / 12.0f;

        // Smooth surface triangles with texture mapping
        glNormal3f(cosf(angle1), sinf(angle1), 0);
        glTexCoord2f(u1, 0.0f); glVertex3f(x1, y1, 0);
        glTexCoord2f(u2, 0.0f); glVertex3f(x2, y2, 0);
        glTexCoord2f(u1, 1.0f); glVertex3f(x1, y1, length);

        glTexCoord2f(u1, 1.0f); glVertex3f(x1, y1, length);
        glTexCoord2f(u2, 0.0f); glVertex3f(x2, y2, 0);
        glTexCoord2f(u2, 1.0f); glVertex3f(x2, y2, length);
    }
    glEnd();

    glPopMatrix();
}

// Draw a robot joint cube with texture
static void drawRobotJoint(const Vec3& pos, float size) {
    glPushMatrix();
    glTranslatef(pos.x, pos.y, pos.z);

    float half = size * 0.5f;

    glBegin(GL_TRIANGLES);

    // Front face
    glNormal3f(0, 0, 1);
    glTexCoord2f(0, 0); glVertex3f(-half, -half, half);
    glTexCoord2f(1, 0); glVertex3f(half, -half, half);
    glTexCoord2f(1, 1); glVertex3f(half, half, half);

    glTexCoord2f(0, 0); glVertex3f(-half, -half, half);
    glTexCoord2f(1, 1); glVertex3f(half, half, half);
    glTexCoord2f(0, 1); glVertex3f(-half, half, half);

    // Back face
    glNormal3f(0, 0, -1);
    glTexCoord2f(0, 0); glVertex3f(half, -half, -half);
    glTexCoord2f(1, 0); glVertex3f(-half, -half, -half);
    glTexCoord2f(1, 1); glVertex3f(-half, half, -half);

    glTexCoord2f(0, 0); glVertex3f(half, -half, -half);
    glTexCoord2f(1, 1); glVertex3f(-half, half, -half);
    glTexCoord2f(0, 1); glVertex3f(half, half, -half);

    // Right face
    glNormal3f(1, 0, 0);
    glTexCoord2f(0, 0); glVertex3f(half, -half, half);
    glTexCoord2f(1, 0); glVertex3f(half, -half, -half);
    glTexCoord2f(1, 1); glVertex3f(half, half, -half);

    glTexCoord2f(0, 0); glVertex3f(half, -half, half);
    glTexCoord2f(1, 1); glVertex3f(half, half, -half);
    glTexCoord2f(0, 1); glVertex3f(half, half, half);

    // Left face
    glNormal3f(-1, 0, 0);
    glTexCoord2f(0, 0); glVertex3f(-half, -half, -half);
    glTexCoord2f(1, 0); glVertex3f(-half, -half, half);
    glTexCoord2f(1, 1); glVertex3f(-half, half, half);

    glTexCoord2f(0, 0); glVertex3f(-half, -half, -half);
    glTexCoord2f(1, 1); glVertex3f(-half, half, half);
    glTexCoord2f(0, 1); glVertex3f(-half, half, -half);

    // Top face
    glNormal3f(0, 1, 0);
    glTexCoord2f(0, 0); glVertex3f(-half, half, half);
    glTexCoord2f(1, 0); glVertex3f(half, half, half);
    glTexCoord2f(1, 1); glVertex3f(half, half, -half);

    glTexCoord2f(0, 0); glVertex3f(-half, half, half);
    glTexCoord2f(1, 1); glVertex3f(half, half, -half);
    glTexCoord2f(0, 1); glVertex3f(-half, half, -half);

    // Bottom face
    glNormal3f(0, -1, 0);
    glTexCoord2f(0, 0); glVertex3f(-half, -half, -half);
    glTexCoord2f(1, 0); glVertex3f(half, -half, -half);
    glTexCoord2f(1, 1); glVertex3f(half, -half, half);

    glTexCoord2f(0, 0); glVertex3f(-half, -half, -half);
    glTexCoord2f(1, 1); glVertex3f(half, -half, half);
    glTexCoord2f(0, 1); glVertex3f(-half, -half, half);

    glEnd();
    glPopMatrix();
}

// Draw a low-poly finger segment with texture
static void drawLowPolyFingerSegment(const Vec3& start, const Vec3& end, float radius) {
    Vec3 dir = { end.x - start.x, end.y - start.y, end.z - start.z };
    float length = sqrtf(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);

    if (length < 0.001f) return;

    // Normalize direction
    dir.x /= length; dir.y /= length; dir.z /= length;

    glPushMatrix();
    glTranslatef(start.x, start.y, start.z);

    // Align with direction vector
    float pitch = asinf(-dir.y) * 180.0f / PI;
    float yaw = atan2f(dir.x, dir.z) * 180.0f / PI;
    glRotatef(yaw, 0, 1, 0);
    glRotatef(pitch, 1, 0, 0);

    // Create organic cylindrical segment - 8 sides for rounded appearance
    const int sides = 8;
    float angleStep = 2.0f * PI / sides;

    glBegin(GL_TRIANGLES);

    // Cylindrical sides
    for (int i = 0; i < sides; i++) {
        float angle1 = i * angleStep;
        float angle2 = ((i + 1) % sides) * angleStep;

        float x1 = radius * cosf(angle1);
        float y1 = radius * sinf(angle1);
        float x2 = radius * cosf(angle2);
        float y2 = radius * sinf(angle2);

        // Normal for this face (pointing outward from cylinder axis)
        Vec3 normal1 = { cosf(angle1), sinf(angle1), 0 };
        Vec3 normal2 = { cosf(angle2), sinf(angle2), 0 };

        // First triangle
        glNormal3f(normal1.x, normal1.y, normal1.z);
        glTexCoord2f((float)i / sides, 0.0f); glVertex3f(x1, y1, 0);
        glTexCoord2f((float)i / sides, 1.0f); glVertex3f(x1, y1, length);

        glNormal3f(normal2.x, normal2.y, normal2.z);
        glTexCoord2f((float)(i + 1) / sides, 0.0f); glVertex3f(x2, y2, 0);

        // Second triangle
        glNormal3f(normal1.x, normal1.y, normal1.z);
        glTexCoord2f((float)i / sides, 1.0f); glVertex3f(x1, y1, length);

        glNormal3f(normal2.x, normal2.y, normal2.z);
        glTexCoord2f((float)(i + 1) / sides, 1.0f); glVertex3f(x2, y2, length);
        glTexCoord2f((float)(i + 1) / sides, 0.0f); glVertex3f(x2, y2, 0);
    }

    glEnd();
    glPopMatrix();
}

// Draw a half sphere fingertip with texture coordinates
static void drawFingertip(const Vec3& position, float radius) {
    glPushMatrix();
    glTranslatef(position.x, position.y, position.z);

    // Create half sphere - top hemisphere only
    const int slices = 8;  // Horizontal divisions
    const int stacks = 4;  // Vertical divisions (half of full sphere)

    glBegin(GL_TRIANGLES);

    // Generate half sphere geometry
    for (int i = 0; i < stacks; i++) {
        float lat0 = PI * (0.0f + (float)i / stacks / 2.0f);      // 0 to PI/2 (top half only)
        float lat1 = PI * (0.0f + (float)(i + 1) / stacks / 2.0f);

        float y0 = radius * sinf(lat0);  // Height
        float y1 = radius * sinf(lat1);
        float r0 = radius * cosf(lat0);  // Radius at this height
        float r1 = radius * cosf(lat1);

        for (int j = 0; j < slices; j++) {
            float lng0 = 2 * PI * (float)j / slices;
            float lng1 = 2 * PI * (float)(j + 1) / slices;

            // Calculate vertices
            float x00 = r0 * cosf(lng0), z00 = r0 * sinf(lng0);
            float x01 = r0 * cosf(lng1), z01 = r0 * sinf(lng1);
            float x10 = r1 * cosf(lng0), z10 = r1 * sinf(lng0);
            float x11 = r1 * cosf(lng1), z11 = r1 * sinf(lng1);

            // Calculate normals (pointing outward)
            Vec3 n00 = { x00 / radius, y0 / radius, z00 / radius };
            Vec3 n01 = { x01 / radius, y0 / radius, z01 / radius };
            Vec3 n10 = { x10 / radius, y1 / radius, z10 / radius };
            Vec3 n11 = { x11 / radius, y1 / radius, z11 / radius };

            // Calculate texture coordinates
            float u0 = (float)j / slices;
            float u1 = (float)(j + 1) / slices;
            float v0 = (float)i / stacks;
            float v1 = (float)(i + 1) / stacks;

            // First triangle
            glNormal3f(n00.x, n00.y, n00.z);
            glTexCoord2f(u0, v0); glVertex3f(x00, y0, z00);

            glNormal3f(n10.x, n10.y, n10.z);
            glTexCoord2f(u0, v1); glVertex3f(x10, y1, z10);

            glNormal3f(n01.x, n01.y, n01.z);
            glTexCoord2f(u1, v0); glVertex3f(x01, y0, z01);

            // Second triangle
            glNormal3f(n01.x, n01.y, n01.z);
            glTexCoord2f(u1, v0); glVertex3f(x01, y0, z01);

            glNormal3f(n10.x, n10.y, n10.z);
            glTexCoord2f(u0, v1); glVertex3f(x10, y1, z10);

            glNormal3f(n11.x, n11.y, n11.z);
            glTexCoord2f(u1, v1); glVertex3f(x11, y1, z11);
        }
    }

    glEnd();
    glPopMatrix();
}

// Draw complete anatomically complex arm with texture for hand 1
static void drawLowPolyArm(const std::vector<ArmJoint>& armJoints) {
    if (armJoints.empty()) return;

    // Enable texturing for the entire arm to match hand
    if (g_TextureEnabled) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, g_HandTexture);
        glColor3f(1.0f, 1.0f, 1.0f); // White to show texture clearly
    }
    else {
        glDisable(GL_TEXTURE_2D);
        glColor3f(0.85f, 0.64f, 0.52f); // Skin color for non-textured mode
    }

    // Draw arm segments scaled to match palm size
    Vec3 shoulderSocket = { 0.0f, 0.0f, 0.0f };  // Start from shoulder socket
    Vec3 wristConnect = { 0.0f, 0.0f, 0.05f };  // Connection to hand

    // Add shoulder connector piece
    drawAnatomicalArmSegment(shoulderSocket, { 0.0f, -0.1f, 0.0f }, 1.2f, 1.0f, 1.0f, 0.8f);

    drawAnatomicalArmSegment(wristConnect, armJoints[1].position, 0.9f, 0.7f, 0.7f, 0.5f);
    drawAnatomicalArmSegment(armJoints[1].position, armJoints[2].position, 1.0f, 0.8f, 0.8f, 0.6f);
    drawAnatomicalArmSegment(armJoints[2].position, armJoints[3].position, 1.1f, 0.9f, 0.9f, 0.65f);
    drawAnatomicalArmSegment(armJoints[3].position, armJoints[4].position, 1.05f, 0.85f, 0.85f, 0.7f);
    drawAnatomicalArmSegment(armJoints[4].position, armJoints[5].position, 1.3f, 1.0f, 1.1f, 0.8f);

    // Draw joints
    for (size_t i = 1; i < armJoints.size(); ++i) {
        Vec3 pos = armJoints[i].position;
        float jointSize = 0.15f + i * 0.02f;
        drawRobotJoint(pos, jointSize);
    }
}

// Draw enhanced palm with comprehensive finger connections like main_hand.cpp
static void drawSkinnedPalm(const std::vector<HandJoint>& joints) {
    Vec3 wrist = joints[0].position;
    Vec3 thumbBase = joints[1].position;
    Vec3 indexBase = joints[5].position;
    Vec3 middleBase = joints[9].position;
    Vec3 ringBase = joints[13].position;
    Vec3 pinkyBase = joints[17].position;

    // ULTRA-DENSE PALM MESH - Complete coverage with no gaps
    const int GRID_WIDTH = 9;   // More columns for complete coverage
    const int GRID_HEIGHT = 7;  // More layers for smooth transitions
    Vec3 palmGrid[GRID_HEIGHT][GRID_WIDTH];

    // Layer 0: Wrist base (extended coverage)
    palmGrid[0][0] = { wrist.x - 0.6f, wrist.y - 0.02f, wrist.z - 0.15f };   // Far pinky edge
    palmGrid[0][1] = { wrist.x - 0.42f, wrist.y - 0.01f, wrist.z - 0.13f };  // Pinky-ring gap
    palmGrid[0][2] = { wrist.x - 0.26f, wrist.y + 0.0f, wrist.z - 0.11f };   // Ring area
    palmGrid[0][3] = { wrist.x - 0.12f, wrist.y + 0.01f, wrist.z - 0.1f };   // Left-center
    palmGrid[0][4] = { wrist.x + 0.0f, wrist.y + 0.02f, wrist.z - 0.09f };   // True center
    palmGrid[0][5] = { wrist.x + 0.12f, wrist.y + 0.01f, wrist.z - 0.1f };   // Right-center
    palmGrid[0][6] = { wrist.x + 0.26f, wrist.y + 0.0f, wrist.z - 0.11f };   // Index area
    palmGrid[0][7] = { wrist.x + 0.42f, wrist.y - 0.01f, wrist.z - 0.13f };  // Index-thumb gap
    palmGrid[0][8] = { wrist.x + 0.6f, wrist.y - 0.02f, wrist.z - 0.15f };   // Far thumb edge

    // Layer 1: Lower palm transition
    palmGrid[1][0] = { wrist.x - 0.52f, wrist.y + 0.08f, wrist.z + 0.05f };
    palmGrid[1][1] = { wrist.x - 0.38f, wrist.y + 0.09f, wrist.z + 0.08f };
    palmGrid[1][2] = { wrist.x - 0.24f, wrist.y + 0.1f, wrist.z + 0.1f };
    palmGrid[1][3] = { wrist.x - 0.1f, wrist.y + 0.11f, wrist.z + 0.12f };
    palmGrid[1][4] = { wrist.x + 0.0f, wrist.y + 0.12f, wrist.z + 0.13f };
    palmGrid[1][5] = { wrist.x + 0.1f, wrist.y + 0.11f, wrist.z + 0.12f };
    palmGrid[1][6] = { wrist.x + 0.24f, wrist.y + 0.1f, wrist.z + 0.1f };
    palmGrid[1][7] = { wrist.x + 0.38f, wrist.y + 0.09f, wrist.z + 0.08f };
    palmGrid[1][8] = { wrist.x + 0.52f, wrist.y + 0.08f, wrist.z + 0.05f };

    // Layer 2: Lower-mid palm
    palmGrid[2][0] = { wrist.x - 0.48f, wrist.y + 0.16f, wrist.z + 0.2f };
    palmGrid[2][1] = { wrist.x - 0.35f, wrist.y + 0.17f, wrist.z + 0.22f };
    palmGrid[2][2] = { wrist.x - 0.22f, wrist.y + 0.18f, wrist.z + 0.24f };
    palmGrid[2][3] = { wrist.x - 0.08f, wrist.y + 0.19f, wrist.z + 0.25f };
    palmGrid[2][4] = { wrist.x + 0.0f, wrist.y + 0.2f, wrist.z + 0.26f };
    palmGrid[2][5] = { wrist.x + 0.08f, wrist.y + 0.19f, wrist.z + 0.25f };
    palmGrid[2][6] = { wrist.x + 0.22f, wrist.y + 0.18f, wrist.z + 0.24f };
    palmGrid[2][7] = { wrist.x + 0.35f, wrist.y + 0.17f, wrist.z + 0.22f };
    palmGrid[2][8] = { wrist.x + 0.48f, wrist.y + 0.16f, wrist.z + 0.2f };

    // Layer 3: True mid palm
    palmGrid[3][0] = { wrist.x - 0.44f, wrist.y + 0.25f, wrist.z + 0.35f };
    palmGrid[3][1] = { wrist.x - 0.32f, wrist.y + 0.26f, wrist.z + 0.37f };
    palmGrid[3][2] = { wrist.x - 0.2f, wrist.y + 0.27f, wrist.z + 0.38f };
    palmGrid[3][3] = { wrist.x - 0.06f, wrist.y + 0.28f, wrist.z + 0.39f };
    palmGrid[3][4] = { wrist.x + 0.0f, wrist.y + 0.29f, wrist.z + 0.4f };
    palmGrid[3][5] = { wrist.x + 0.06f, wrist.y + 0.28f, wrist.z + 0.39f };
    palmGrid[3][6] = { wrist.x + 0.2f, wrist.y + 0.27f, wrist.z + 0.38f };
    palmGrid[3][7] = { wrist.x + 0.32f, wrist.y + 0.26f, wrist.z + 0.37f };
    palmGrid[3][8] = { wrist.x + 0.44f, wrist.y + 0.25f, wrist.z + 0.35f };

    // Layer 4: Upper palm (approaching knuckles)
    palmGrid[4][0] = { wrist.x - 0.4f, wrist.y + 0.35f, wrist.z + 0.5f };
    palmGrid[4][1] = { wrist.x - 0.28f, wrist.y + 0.36f, wrist.z + 0.52f };
    palmGrid[4][2] = { wrist.x - 0.16f, wrist.y + 0.37f, wrist.z + 0.53f };
    palmGrid[4][3] = { wrist.x - 0.04f, wrist.y + 0.38f, wrist.z + 0.54f };
    palmGrid[4][4] = { wrist.x + 0.0f, wrist.y + 0.39f, wrist.z + 0.55f };
    palmGrid[4][5] = { wrist.x + 0.04f, wrist.y + 0.38f, wrist.z + 0.54f };
    palmGrid[4][6] = { wrist.x + 0.16f, wrist.y + 0.37f, wrist.z + 0.53f };
    palmGrid[4][7] = { wrist.x + 0.28f, wrist.y + 0.36f, wrist.z + 0.52f };
    palmGrid[4][8] = { wrist.x + 0.4f, wrist.y + 0.35f, wrist.z + 0.5f };

    // Layer 5: Pre-knuckle area (precision positioning)
    palmGrid[5][0] = { pinkyBase.x - 0.02f, pinkyBase.y + 0.02f, pinkyBase.z - 0.3f };
    palmGrid[5][1] = { (pinkyBase.x + ringBase.x) * 0.5f - 0.015f, (pinkyBase.y + ringBase.y) * 0.5f + 0.04f, (pinkyBase.z + ringBase.z) * 0.5f - 0.3f };
    palmGrid[5][2] = { ringBase.x - 0.015f, ringBase.y + 0.06f, ringBase.z - 0.3f };
    palmGrid[5][3] = { (ringBase.x + middleBase.x) * 0.5f - 0.01f, (ringBase.y + middleBase.y) * 0.5f + 0.075f, (ringBase.z + middleBase.z) * 0.5f - 0.3f };
    palmGrid[5][4] = { middleBase.x - 0.01f, middleBase.y + 0.09f, middleBase.z - 0.3f };
    palmGrid[5][5] = { (middleBase.x + indexBase.x) * 0.5f - 0.01f, (middleBase.y + indexBase.y) * 0.5f + 0.075f, (middleBase.z + indexBase.z) * 0.5f - 0.3f };
    palmGrid[5][6] = { indexBase.x - 0.015f, indexBase.y + 0.06f, indexBase.z - 0.3f };
    palmGrid[5][7] = { (indexBase.x + thumbBase.x) * 0.7f - 0.02f, (indexBase.y + thumbBase.y) * 0.7f + 0.03f, (indexBase.z + thumbBase.z) * 0.7f - 0.25f };
    palmGrid[5][8] = { thumbBase.x - 0.05f, thumbBase.y + 0.01f, thumbBase.z - 0.2f };

    // Layer 6: Direct finger connections (seamless transitions)
    palmGrid[6][0] = { pinkyBase.x, pinkyBase.y - 0.05f, pinkyBase.z - 0.08f };
    palmGrid[6][1] = { (pinkyBase.x + ringBase.x) * 0.5f, (pinkyBase.y + ringBase.y) * 0.5f - 0.06f, (pinkyBase.z + ringBase.z) * 0.5f - 0.08f };
    palmGrid[6][2] = { ringBase.x, ringBase.y - 0.06f, ringBase.z - 0.08f };
    palmGrid[6][3] = { (ringBase.x + middleBase.x) * 0.5f, (ringBase.y + middleBase.y) * 0.5f - 0.07f, (ringBase.z + middleBase.z) * 0.5f - 0.08f };
    palmGrid[6][4] = { middleBase.x, middleBase.y - 0.08f, middleBase.z - 0.08f };
    palmGrid[6][5] = { (middleBase.x + indexBase.x) * 0.5f, (middleBase.y + indexBase.y) * 0.5f - 0.07f, (middleBase.z + indexBase.z) * 0.5f - 0.08f };
    palmGrid[6][6] = { indexBase.x, indexBase.y - 0.06f, indexBase.z - 0.08f };
    palmGrid[6][7] = { (indexBase.x + thumbBase.x) * 0.6f, (indexBase.y + thumbBase.y) * 0.6f - 0.05f, (indexBase.z + thumbBase.z) * 0.6f - 0.06f };
    palmGrid[6][8] = { thumbBase.x, thumbBase.y - 0.04f, thumbBase.z - 0.05f };

    // Create back surface vertices (mirrored below palm)
    Vec3 backGrid[GRID_HEIGHT][GRID_WIDTH];
    for (int row = 0; row < GRID_HEIGHT; row++) {
        for (int col = 0; col < GRID_WIDTH; col++) {
            backGrid[row][col] = { palmGrid[row][col].x, palmGrid[row][col].y - 0.35f, palmGrid[row][col].z };
        }
    }

    // ULTRA-DENSE MESH RENDERING - Complete coverage with no gaps
    glBegin(GL_TRIANGLES);

    // PALM SURFACE (top) - Complete quad grid tessellation
    Vec3 normalUp = { 0, 1, 0.15f };
    glNormal3f(normalUp.x, normalUp.y, normalUp.z);

    // Draw complete grid as triangulated quads
    for (int row = 0; row < GRID_HEIGHT - 1; row++) {
        for (int col = 0; col < GRID_WIDTH - 1; col++) {
            // Calculate texture coordinates for seamless mapping
            float u1 = (float)col / (GRID_WIDTH - 1);
            float u2 = (float)(col + 1) / (GRID_WIDTH - 1);
            float v1 = (float)row / (GRID_HEIGHT - 1);
            float v2 = (float)(row + 1) / (GRID_HEIGHT - 1);

            // Quad triangle 1: Current -> Right -> Below-Right
            glTexCoord2f(u1, v1); glVertex3f(palmGrid[row][col].x, palmGrid[row][col].y, palmGrid[row][col].z);
            glTexCoord2f(u2, v1); glVertex3f(palmGrid[row][col + 1].x, palmGrid[row][col + 1].y, palmGrid[row][col + 1].z);
            glTexCoord2f(u2, v2); glVertex3f(palmGrid[row + 1][col + 1].x, palmGrid[row + 1][col + 1].y, palmGrid[row + 1][col + 1].z);

            // Quad triangle 2: Current -> Below-Right -> Below
            glTexCoord2f(u1, v1); glVertex3f(palmGrid[row][col].x, palmGrid[row][col].y, palmGrid[row][col].z);
            glTexCoord2f(u2, v2); glVertex3f(palmGrid[row + 1][col + 1].x, palmGrid[row + 1][col + 1].y, palmGrid[row + 1][col + 1].z);
            glTexCoord2f(u1, v2); glVertex3f(palmGrid[row + 1][col].x, palmGrid[row + 1][col].y, palmGrid[row + 1][col].z);
        }
    }

    // BACK SURFACE (bottom) - Mirrored grid
    Vec3 normalDown = { 0, -1, -0.15f };
    glNormal3f(normalDown.x, normalDown.y, normalDown.z);

    for (int row = 0; row < GRID_HEIGHT - 1; row++) {
        for (int col = 0; col < GRID_WIDTH - 1; col++) {
            float u1 = (float)col / (GRID_WIDTH - 1);
            float u2 = (float)(col + 1) / (GRID_WIDTH - 1);
            float v1 = (float)row / (GRID_HEIGHT - 1);
            float v2 = (float)(row + 1) / (GRID_HEIGHT - 1);

            // Reverse winding for back face
            glTexCoord2f(u1, v1); glVertex3f(backGrid[row][col].x, backGrid[row][col].y, backGrid[row][col].z);
            glTexCoord2f(u2, v2); glVertex3f(backGrid[row + 1][col + 1].x, backGrid[row + 1][col + 1].y, backGrid[row + 1][col + 1].z);
            glTexCoord2f(u2, v1); glVertex3f(backGrid[row][col + 1].x, backGrid[row][col + 1].y, backGrid[row][col + 1].z);

            glTexCoord2f(u1, v1); glVertex3f(backGrid[row][col].x, backGrid[row][col].y, backGrid[row][col].z);
            glTexCoord2f(u1, v2); glVertex3f(backGrid[row + 1][col].x, backGrid[row + 1][col].y, backGrid[row + 1][col].z);
            glTexCoord2f(u2, v2); glVertex3f(backGrid[row + 1][col + 1].x, backGrid[row + 1][col + 1].y, backGrid[row + 1][col + 1].z);
        }
    }

    // SIDE WALLS - Connect palm to back surface edges
    // Left edge wall
    for (int row = 0; row < GRID_HEIGHT - 1; row++) {
        Vec3 edge1 = { palmGrid[row + 1][0].x - palmGrid[row][0].x, palmGrid[row + 1][0].y - palmGrid[row][0].y, palmGrid[row + 1][0].z - palmGrid[row][0].z };
        Vec3 edge2 = { backGrid[row][0].x - palmGrid[row][0].x, backGrid[row][0].y - palmGrid[row][0].y, backGrid[row][0].z - palmGrid[row][0].z };
        Vec3 normal = { edge1.y * edge2.z - edge1.z * edge2.y, edge1.z * edge2.x - edge1.x * edge2.z, edge1.x * edge2.y - edge1.y * edge2.x };
        float len = sqrtf(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
        if (len > 0.001f) { normal.x /= len; normal.y /= len; normal.z /= len; }

        glNormal3f(normal.x, normal.y, normal.z);
        float v1 = (float)row / (GRID_HEIGHT - 1);
        float v2 = (float)(row + 1) / (GRID_HEIGHT - 1);

        glTexCoord2f(0.0f, v1); glVertex3f(palmGrid[row][0].x, palmGrid[row][0].y, palmGrid[row][0].z);
        glTexCoord2f(1.0f, v1); glVertex3f(backGrid[row][0].x, backGrid[row][0].y, backGrid[row][0].z);
        glTexCoord2f(0.0f, v2); glVertex3f(palmGrid[row + 1][0].x, palmGrid[row + 1][0].y, palmGrid[row + 1][0].z);

        glTexCoord2f(1.0f, v1); glVertex3f(backGrid[row][0].x, backGrid[row][0].y, backGrid[row][0].z);
        glTexCoord2f(1.0f, v2); glVertex3f(backGrid[row + 1][0].x, backGrid[row + 1][0].y, backGrid[row + 1][0].z);
        glTexCoord2f(0.0f, v2); glVertex3f(palmGrid[row + 1][0].x, palmGrid[row + 1][0].y, palmGrid[row + 1][0].z);
    }

    // Right edge wall
    for (int row = 0; row < GRID_HEIGHT - 1; row++) {
        Vec3 edge1 = { palmGrid[row + 1][GRID_WIDTH - 1].x - palmGrid[row][GRID_WIDTH - 1].x, palmGrid[row + 1][GRID_WIDTH - 1].y - palmGrid[row][GRID_WIDTH - 1].y, palmGrid[row + 1][GRID_WIDTH - 1].z - palmGrid[row][GRID_WIDTH - 1].z };
        Vec3 edge2 = { backGrid[row][GRID_WIDTH - 1].x - palmGrid[row][GRID_WIDTH - 1].x, backGrid[row][GRID_WIDTH - 1].y - palmGrid[row][GRID_WIDTH - 1].y, backGrid[row][GRID_WIDTH - 1].z - palmGrid[row][GRID_WIDTH - 1].z };
        Vec3 normal = { edge2.y * edge1.z - edge2.z * edge1.y, edge2.z * edge1.x - edge2.x * edge1.z, edge2.x * edge1.y - edge2.y * edge1.x };
        float len = sqrtf(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
        if (len > 0.001f) { normal.x /= len; normal.y /= len; normal.z /= len; }

        glNormal3f(normal.x, normal.y, normal.z);
        float v1 = (float)row / (GRID_HEIGHT - 1);
        float v2 = (float)(row + 1) / (GRID_HEIGHT - 1);

        glTexCoord2f(0.0f, v1); glVertex3f(palmGrid[row][GRID_WIDTH - 1].x, palmGrid[row][GRID_WIDTH - 1].y, palmGrid[row][GRID_WIDTH - 1].z);
        glTexCoord2f(0.0f, v2); glVertex3f(palmGrid[row + 1][GRID_WIDTH - 1].x, palmGrid[row + 1][GRID_WIDTH - 1].y, palmGrid[row + 1][GRID_WIDTH - 1].z);
        glTexCoord2f(1.0f, v1); glVertex3f(backGrid[row][GRID_WIDTH - 1].x, backGrid[row][GRID_WIDTH - 1].y, backGrid[row][GRID_WIDTH - 1].z);

        glTexCoord2f(1.0f, v1); glVertex3f(backGrid[row][GRID_WIDTH - 1].x, backGrid[row][GRID_WIDTH - 1].y, backGrid[row][GRID_WIDTH - 1].z);
        glTexCoord2f(0.0f, v2); glVertex3f(palmGrid[row + 1][GRID_WIDTH - 1].x, palmGrid[row + 1][GRID_WIDTH - 1].y, palmGrid[row + 1][GRID_WIDTH - 1].z);
        glTexCoord2f(1.0f, v2); glVertex3f(backGrid[row + 1][GRID_WIDTH - 1].x, backGrid[row + 1][GRID_WIDTH - 1].y, backGrid[row + 1][GRID_WIDTH - 1].z);
    }

    // Top edge wall (wrist side)
    for (int col = 0; col < GRID_WIDTH - 1; col++) {
        Vec3 edge1 = { palmGrid[0][col + 1].x - palmGrid[0][col].x, palmGrid[0][col + 1].y - palmGrid[0][col].y, palmGrid[0][col + 1].z - palmGrid[0][col].z };
        Vec3 edge2 = { backGrid[0][col].x - palmGrid[0][col].x, backGrid[0][col].y - palmGrid[0][col].y, backGrid[0][col].z - palmGrid[0][col].z };
        Vec3 normal = { edge2.y * edge1.z - edge2.z * edge1.y, edge2.z * edge1.x - edge2.x * edge1.z, edge2.x * edge1.y - edge2.y * edge2.x };
        float len = sqrtf(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
        if (len > 0.001f) { normal.x /= len; normal.y /= len; normal.z /= len; }

        glNormal3f(normal.x, normal.y, normal.z);
        float u1 = (float)col / (GRID_WIDTH - 1);
        float u2 = (float)(col + 1) / (GRID_WIDTH - 1);

        glTexCoord2f(u1, 0.0f); glVertex3f(palmGrid[0][col].x, palmGrid[0][col].y, palmGrid[0][col].z);
        glTexCoord2f(u2, 0.0f); glVertex3f(palmGrid[0][col + 1].x, palmGrid[0][col + 1].y, palmGrid[0][col + 1].z);
        glTexCoord2f(u1, 1.0f); glVertex3f(backGrid[0][col].x, backGrid[0][col].y, backGrid[0][col].z);

        glTexCoord2f(u2, 0.0f); glVertex3f(palmGrid[0][col + 1].x, palmGrid[0][col + 1].y, palmGrid[0][col + 1].z);
        glTexCoord2f(u2, 1.0f); glVertex3f(backGrid[0][col + 1].x, backGrid[0][col + 1].y, backGrid[0][col + 1].z);
        glTexCoord2f(u1, 1.0f); glVertex3f(backGrid[0][col].x, backGrid[0][col].y, backGrid[0][col].z);
    }

    // Bottom edge wall (finger side)
    for (int col = 0; col < GRID_WIDTH - 1; col++) {
        Vec3 edge1 = { palmGrid[GRID_HEIGHT - 1][col + 1].x - palmGrid[GRID_HEIGHT - 1][col].x, palmGrid[GRID_HEIGHT - 1][col + 1].y - palmGrid[GRID_HEIGHT - 1][col].y, palmGrid[GRID_HEIGHT - 1][col + 1].z - palmGrid[GRID_HEIGHT - 1][col].z };
        Vec3 edge2 = { backGrid[GRID_HEIGHT - 1][col].x - palmGrid[GRID_HEIGHT - 1][col].x, backGrid[GRID_HEIGHT - 1][col].y - palmGrid[GRID_HEIGHT - 1][col].y, backGrid[GRID_HEIGHT - 1][col].z - palmGrid[GRID_HEIGHT - 1][col].z };
        Vec3 normal = { edge1.y * edge2.z - edge1.z * edge2.y, edge1.z * edge2.x - edge1.x * edge2.z, edge1.x * edge2.y - edge1.y * edge2.x };
        float len = sqrtf(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
        if (len > 0.001f) { normal.x /= len; normal.y /= len; normal.z /= len; }

        glNormal3f(normal.x, normal.y, normal.z);
        float u1 = (float)col / (GRID_WIDTH - 1);
        float u2 = (float)(col + 1) / (GRID_WIDTH - 1);

        glTexCoord2f(u1, 0.0f); glVertex3f(palmGrid[GRID_HEIGHT - 1][col].x, palmGrid[GRID_HEIGHT - 1][col].y, palmGrid[GRID_HEIGHT - 1][col].z);
        glTexCoord2f(u1, 1.0f); glVertex3f(backGrid[GRID_HEIGHT - 1][col].x, backGrid[GRID_HEIGHT - 1][col].y, backGrid[GRID_HEIGHT - 1][col].z);
        glTexCoord2f(u2, 0.0f); glVertex3f(palmGrid[GRID_HEIGHT - 1][col + 1].x, palmGrid[GRID_HEIGHT - 1][col + 1].y, palmGrid[GRID_HEIGHT - 1][col + 1].z);

        glTexCoord2f(u2, 0.0f); glVertex3f(palmGrid[GRID_HEIGHT - 1][col + 1].x, palmGrid[GRID_HEIGHT - 1][col + 1].y, palmGrid[GRID_HEIGHT - 1][col + 1].z);
        glTexCoord2f(u1, 1.0f); glVertex3f(backGrid[GRID_HEIGHT - 1][col].x, backGrid[GRID_HEIGHT - 1][col].y, backGrid[GRID_HEIGHT - 1][col].z);
        glTexCoord2f(u2, 1.0f); glVertex3f(backGrid[GRID_HEIGHT - 1][col + 1].x, backGrid[GRID_HEIGHT - 1][col + 1].y, backGrid[GRID_HEIGHT - 1][col + 1].z);
    }

    glEnd();
}

// Enhanced palm connection sides for seamless finger joints  
static void drawPalmSides(const std::vector<Vec3*>& bottomVerts, const std::vector<Vec3*>& topVerts) {
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < (int)bottomVerts.size(); i++) {
        int nextI = (i + 1) % (int)bottomVerts.size();
        Vec3 edge1 = { topVerts[nextI]->x - topVerts[i]->x, topVerts[nextI]->y - topVerts[i]->y, topVerts[nextI]->z - topVerts[i]->z };
        Vec3 edge2 = { bottomVerts[i]->x - topVerts[i]->x, bottomVerts[i]->y - topVerts[i]->y, bottomVerts[i]->z - topVerts[i]->z };
        Vec3 normal = { edge1.y * edge2.z - edge1.z * edge2.y, edge1.z * edge2.x - edge1.x * edge2.z, edge1.x * edge2.y - edge1.y * edge2.x };
        float len = sqrtf(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
        if (len > 0.001f) { normal.x /= len; normal.y /= len; normal.z /= len; }

        glNormal3f(normal.x, normal.y, normal.z);
        float u1 = (float)i / (float)bottomVerts.size();
        float u2 = (float)nextI / (float)bottomVerts.size();

        glTexCoord2f(u1, 0.0f); glVertex3f(topVerts[i]->x, topVerts[i]->y, topVerts[i]->z);
        glTexCoord2f(u1, 1.0f); glVertex3f(bottomVerts[i]->x, bottomVerts[i]->y, bottomVerts[i]->z);
        glTexCoord2f(u2, 0.0f); glVertex3f(topVerts[nextI]->x, topVerts[nextI]->y, topVerts[nextI]->z);

        glTexCoord2f(u1, 1.0f); glVertex3f(bottomVerts[i]->x, bottomVerts[i]->y, bottomVerts[i]->z);
        glTexCoord2f(u2, 1.0f); glVertex3f(bottomVerts[nextI]->x, bottomVerts[nextI]->y, bottomVerts[nextI]->z);
        glTexCoord2f(u2, 0.0f); glVertex3f(topVerts[nextI]->x, topVerts[nextI]->y, topVerts[nextI]->z);
    }
    glEnd();
}

// Draw enhanced palm with ultra-dense mesh for complete coverage (hand 2)
static void drawSkinnedPalm2(const std::vector<HandJoint>& joints) {
    Vec3 wrist = joints[0].position;
    Vec3 thumbBase = joints[1].position;
    Vec3 indexBase = joints[5].position;
    Vec3 middleBase = joints[9].position;
    Vec3 ringBase = joints[13].position;
    Vec3 pinkyBase = joints[17].position;

    // IDENTICAL ULTRA-DENSE PALM MESH as palm 1 for consistency
    const int GRID_WIDTH = 9;
    const int GRID_HEIGHT = 7;
    Vec3 palmGrid[GRID_HEIGHT][GRID_WIDTH];

    // Use exact same grid structure as palm 1 for consistent coverage

    // Layer 0: Wrist base (extended coverage)
    palmGrid[0][0] = { wrist.x - 0.6f, wrist.y - 0.02f, wrist.z - 0.15f };
    palmGrid[0][1] = { wrist.x - 0.42f, wrist.y - 0.01f, wrist.z - 0.13f };
    palmGrid[0][2] = { wrist.x - 0.26f, wrist.y + 0.0f, wrist.z - 0.11f };
    palmGrid[0][3] = { wrist.x - 0.12f, wrist.y + 0.01f, wrist.z - 0.1f };
    palmGrid[0][4] = { wrist.x + 0.0f, wrist.y + 0.02f, wrist.z - 0.09f };
    palmGrid[0][5] = { wrist.x + 0.12f, wrist.y + 0.01f, wrist.z - 0.1f };
    palmGrid[0][6] = { wrist.x + 0.26f, wrist.y + 0.0f, wrist.z - 0.11f };
    palmGrid[0][7] = { wrist.x + 0.42f, wrist.y - 0.01f, wrist.z - 0.13f };
    palmGrid[0][8] = { wrist.x + 0.6f, wrist.y - 0.02f, wrist.z - 0.15f };

    // Layer 1: Lower palm transition
    palmGrid[1][0] = { wrist.x - 0.52f, wrist.y + 0.08f, wrist.z + 0.05f };
    palmGrid[1][1] = { wrist.x - 0.38f, wrist.y + 0.09f, wrist.z + 0.08f };
    palmGrid[1][2] = { wrist.x - 0.24f, wrist.y + 0.1f, wrist.z + 0.1f };
    palmGrid[1][3] = { wrist.x - 0.1f, wrist.y + 0.11f, wrist.z + 0.12f };
    palmGrid[1][4] = { wrist.x + 0.0f, wrist.y + 0.12f, wrist.z + 0.13f };
    palmGrid[1][5] = { wrist.x + 0.1f, wrist.y + 0.11f, wrist.z + 0.12f };
    palmGrid[1][6] = { wrist.x + 0.24f, wrist.y + 0.1f, wrist.z + 0.1f };
    palmGrid[1][7] = { wrist.x + 0.38f, wrist.y + 0.09f, wrist.z + 0.08f };
    palmGrid[1][8] = { wrist.x + 0.52f, wrist.y + 0.08f, wrist.z + 0.05f };

    // Layer 2: Lower-mid palm
    palmGrid[2][0] = { wrist.x - 0.48f, wrist.y + 0.16f, wrist.z + 0.2f };
    palmGrid[2][1] = { wrist.x - 0.35f, wrist.y + 0.17f, wrist.z + 0.22f };
    palmGrid[2][2] = { wrist.x - 0.22f, wrist.y + 0.18f, wrist.z + 0.24f };
    palmGrid[2][3] = { wrist.x - 0.08f, wrist.y + 0.19f, wrist.z + 0.25f };
    palmGrid[2][4] = { wrist.x + 0.0f, wrist.y + 0.2f, wrist.z + 0.26f };
    palmGrid[2][5] = { wrist.x + 0.08f, wrist.y + 0.19f, wrist.z + 0.25f };
    palmGrid[2][6] = { wrist.x + 0.22f, wrist.y + 0.18f, wrist.z + 0.24f };
    palmGrid[2][7] = { wrist.x + 0.35f, wrist.y + 0.17f, wrist.z + 0.22f };
    palmGrid[2][8] = { wrist.x + 0.48f, wrist.y + 0.16f, wrist.z + 0.2f };

    // Layer 3: True mid palm
    palmGrid[3][0] = { wrist.x - 0.44f, wrist.y + 0.25f, wrist.z + 0.35f };
    palmGrid[3][1] = { wrist.x - 0.32f, wrist.y + 0.26f, wrist.z + 0.37f };
    palmGrid[3][2] = { wrist.x - 0.2f, wrist.y + 0.27f, wrist.z + 0.38f };
    palmGrid[3][3] = { wrist.x - 0.06f, wrist.y + 0.28f, wrist.z + 0.39f };
    palmGrid[3][4] = { wrist.x + 0.0f, wrist.y + 0.29f, wrist.z + 0.4f };
    palmGrid[3][5] = { wrist.x + 0.06f, wrist.y + 0.28f, wrist.z + 0.39f };
    palmGrid[3][6] = { wrist.x + 0.2f, wrist.y + 0.27f, wrist.z + 0.38f };
    palmGrid[3][7] = { wrist.x + 0.32f, wrist.y + 0.26f, wrist.z + 0.37f };
    palmGrid[3][8] = { wrist.x + 0.44f, wrist.y + 0.25f, wrist.z + 0.35f };

    // Layer 4: Upper palm (approaching knuckles)
    palmGrid[4][0] = { wrist.x - 0.4f, wrist.y + 0.35f, wrist.z + 0.5f };
    palmGrid[4][1] = { wrist.x - 0.28f, wrist.y + 0.36f, wrist.z + 0.52f };
    palmGrid[4][2] = { wrist.x - 0.16f, wrist.y + 0.37f, wrist.z + 0.53f };
    palmGrid[4][3] = { wrist.x - 0.04f, wrist.y + 0.38f, wrist.z + 0.54f };
    palmGrid[4][4] = { wrist.x + 0.0f, wrist.y + 0.39f, wrist.z + 0.55f };
    palmGrid[4][5] = { wrist.x + 0.04f, wrist.y + 0.38f, wrist.z + 0.54f };
    palmGrid[4][6] = { wrist.x + 0.16f, wrist.y + 0.37f, wrist.z + 0.53f };
    palmGrid[4][7] = { wrist.x + 0.28f, wrist.y + 0.36f, wrist.z + 0.52f };
    palmGrid[4][8] = { wrist.x + 0.4f, wrist.y + 0.35f, wrist.z + 0.5f };

    // Layer 5: Pre-knuckle area (precision positioning)
    palmGrid[5][0] = { pinkyBase.x - 0.02f, pinkyBase.y + 0.02f, pinkyBase.z - 0.3f };
    palmGrid[5][1] = { (pinkyBase.x + ringBase.x) * 0.5f - 0.015f, (pinkyBase.y + ringBase.y) * 0.5f + 0.04f, (pinkyBase.z + ringBase.z) * 0.5f - 0.3f };
    palmGrid[5][2] = { ringBase.x - 0.015f, ringBase.y + 0.06f, ringBase.z - 0.3f };
    palmGrid[5][3] = { (ringBase.x + middleBase.x) * 0.5f - 0.01f, (ringBase.y + middleBase.y) * 0.5f + 0.075f, (ringBase.z + middleBase.z) * 0.5f - 0.3f };
    palmGrid[5][4] = { middleBase.x - 0.01f, middleBase.y + 0.09f, middleBase.z - 0.3f };
    palmGrid[5][5] = { (middleBase.x + indexBase.x) * 0.5f - 0.01f, (middleBase.y + indexBase.y) * 0.5f + 0.075f, (middleBase.z + indexBase.z) * 0.5f - 0.3f };
    palmGrid[5][6] = { indexBase.x - 0.015f, indexBase.y + 0.06f, indexBase.z - 0.3f };
    palmGrid[5][7] = { (indexBase.x + thumbBase.x) * 0.7f - 0.02f, (indexBase.y + thumbBase.y) * 0.7f + 0.03f, (indexBase.z + thumbBase.z) * 0.7f - 0.25f };
    palmGrid[5][8] = { thumbBase.x - 0.05f, thumbBase.y + 0.01f, thumbBase.z - 0.2f };

    // Layer 6: Direct finger connections (seamless transitions)
    palmGrid[6][0] = { pinkyBase.x, pinkyBase.y - 0.05f, pinkyBase.z - 0.08f };
    palmGrid[6][1] = { (pinkyBase.x + ringBase.x) * 0.5f, (pinkyBase.y + ringBase.y) * 0.5f - 0.06f, (pinkyBase.z + ringBase.z) * 0.5f - 0.08f };
    palmGrid[6][2] = { ringBase.x, ringBase.y - 0.06f, ringBase.z - 0.08f };
    palmGrid[6][3] = { (ringBase.x + middleBase.x) * 0.5f, (ringBase.y + middleBase.y) * 0.5f - 0.07f, (ringBase.z + middleBase.z) * 0.5f - 0.08f };
    palmGrid[6][4] = { middleBase.x, middleBase.y - 0.08f, middleBase.z - 0.08f };
    palmGrid[6][5] = { (middleBase.x + indexBase.x) * 0.5f, (middleBase.y + indexBase.y) * 0.5f - 0.07f, (middleBase.z + indexBase.z) * 0.5f - 0.08f };
    palmGrid[6][6] = { indexBase.x, indexBase.y - 0.06f, indexBase.z - 0.08f };
    palmGrid[6][7] = { (indexBase.x + thumbBase.x) * 0.6f, (indexBase.y + thumbBase.y) * 0.6f - 0.05f, (indexBase.z + thumbBase.z) * 0.6f - 0.06f };
    palmGrid[6][8] = { thumbBase.x, thumbBase.y - 0.04f, thumbBase.z - 0.05f };

    // Create back surface vertices (mirrored below palm)
    Vec3 backGrid[GRID_HEIGHT][GRID_WIDTH];
    for (int row = 0; row < GRID_HEIGHT; row++) {
        for (int col = 0; col < GRID_WIDTH; col++) {
            backGrid[row][col] = { palmGrid[row][col].x, palmGrid[row][col].y - 0.35f, palmGrid[row][col].z };
        }
    }

    // IDENTICAL ULTRA-DENSE MESH RENDERING as palm 1
    glBegin(GL_TRIANGLES);

    // PALM SURFACE (top) - Complete quad grid tessellation
    Vec3 normalUp = { 0, 1, 0.15f };
    glNormal3f(normalUp.x, normalUp.y, normalUp.z);

    // Draw complete grid as triangulated quads
    for (int row = 0; row < GRID_HEIGHT - 1; row++) {
        for (int col = 0; col < GRID_WIDTH - 1; col++) {
            float u1 = (float)col / (GRID_WIDTH - 1);
            float u2 = (float)(col + 1) / (GRID_WIDTH - 1);
            float v1 = (float)row / (GRID_HEIGHT - 1);
            float v2 = (float)(row + 1) / (GRID_HEIGHT - 1);

            // Quad triangle 1
            glTexCoord2f(u1, v1); glVertex3f(palmGrid[row][col].x, palmGrid[row][col].y, palmGrid[row][col].z);
            glTexCoord2f(u2, v1); glVertex3f(palmGrid[row][col + 1].x, palmGrid[row][col + 1].y, palmGrid[row][col + 1].z);
            glTexCoord2f(u2, v2); glVertex3f(palmGrid[row + 1][col + 1].x, palmGrid[row + 1][col + 1].y, palmGrid[row + 1][col + 1].z);

            // Quad triangle 2
            glTexCoord2f(u1, v1); glVertex3f(palmGrid[row][col].x, palmGrid[row][col].y, palmGrid[row][col].z);
            glTexCoord2f(u2, v2); glVertex3f(palmGrid[row + 1][col + 1].x, palmGrid[row + 1][col + 1].y, palmGrid[row + 1][col + 1].z);
            glTexCoord2f(u1, v2); glVertex3f(palmGrid[row + 1][col].x, palmGrid[row + 1][col].y, palmGrid[row + 1][col].z);
        }
    }

    // BACK SURFACE (bottom) - Mirrored grid with reverse winding
    Vec3 normalDown = { 0, -1, -0.15f };
    glNormal3f(normalDown.x, normalDown.y, normalDown.z);

    for (int row = 0; row < GRID_HEIGHT - 1; row++) {
        for (int col = 0; col < GRID_WIDTH - 1; col++) {
            float u1 = (float)col / (GRID_WIDTH - 1);
            float u2 = (float)(col + 1) / (GRID_WIDTH - 1);
            float v1 = (float)row / (GRID_HEIGHT - 1);
            float v2 = (float)(row + 1) / (GRID_HEIGHT - 1);

            // Reverse winding for back face
            glTexCoord2f(u1, v1); glVertex3f(backGrid[row][col].x, backGrid[row][col].y, backGrid[row][col].z);
            glTexCoord2f(u2, v2); glVertex3f(backGrid[row + 1][col + 1].x, backGrid[row + 1][col + 1].y, backGrid[row + 1][col + 1].z);
            glTexCoord2f(u2, v1); glVertex3f(backGrid[row][col + 1].x, backGrid[row][col + 1].y, backGrid[row][col + 1].z);

            glTexCoord2f(u1, v1); glVertex3f(backGrid[row][col].x, backGrid[row][col].y, backGrid[row][col].z);
            glTexCoord2f(u1, v2); glVertex3f(backGrid[row + 1][col].x, backGrid[row + 1][col].y, backGrid[row + 1][col].z);
            glTexCoord2f(u2, v2); glVertex3f(backGrid[row + 1][col + 1].x, backGrid[row + 1][col + 1].y, backGrid[row + 1][col + 1].z);
        }
    }

    glEnd();
}

// Draw a complete low-poly finger with fingertip for hand 1
static void drawLowPolyFinger(const std::vector<HandJoint>& joints, int mcpIdx, int pipIdx, int dipIdx, int tipIdx) {
    if (mcpIdx >= 0 && pipIdx >= 0) {
        drawLowPolyFingerSegment(joints[mcpIdx].position, joints[pipIdx].position, 0.18f);
    }
    if (pipIdx >= 0 && dipIdx >= 0) {
        drawLowPolyFingerSegment(joints[pipIdx].position, joints[dipIdx].position, 0.15f);
    }
    if (dipIdx >= 0 && tipIdx >= 0) {
        drawLowPolyFingerSegment(joints[dipIdx].position, joints[tipIdx].position, 0.12f);
        drawFingertip(joints[tipIdx].position, 0.12f);
    }
}

// Draw low-poly thumb with fingertip for hand 1
static void drawLowPolyThumb(const std::vector<HandJoint>& joints) {
    drawLowPolyFingerSegment(joints[1].position, joints[2].position, 0.20f);
    drawLowPolyFingerSegment(joints[2].position, joints[3].position, 0.17f);
    drawLowPolyFingerSegment(joints[3].position, joints[4].position, 0.14f);
    drawFingertip(joints[4].position, 0.14f);
}

// leg

// Helper function to make leg cross-sections more round
Vec3f makeRoundedVertex(const Vec3f& originalVertex) {
    Vec3f rounded = originalVertex;
    
    // Get the radius from center at this Y level
    float radius = sqrtf(originalVertex.x * originalVertex.x + originalVertex.z * originalVertex.z);
    
    // Calculate the angle around the Y axis
    float angle = atan2f(originalVertex.z, originalVertex.x);
    
    // Determine target radius based on Y position for different leg sections (slimmer)
    float targetRadius;
    if (originalVertex.y < 2.0f) {
        // Foot area - keep original shape for foot details
        return originalVertex;
    } else if (originalVertex.y < 4.0f) {
        // Ankle/lower calf - gradually round and slim
        targetRadius = 0.2f + (originalVertex.y - 2.0f) * 0.05f; // 0.2 to 0.3 (was 0.3 to 0.5)
    } else if (originalVertex.y < 6.0f) {
        // Mid calf - rounder but slimmer
        targetRadius = 0.3f + (originalVertex.y - 4.0f) * 0.05f; // 0.3 to 0.4 (was 0.5 to 0.6)
    } else {
        // Upper leg/thigh - roundest but much slimmer
        targetRadius = 0.4f + (originalVertex.y - 6.0f) * 0.03f; // 0.4 to 0.46+ (was 0.6 to 0.7+)
    }
    
    // If original radius is very small, don't modify much (preserve center line)
    if (radius < 0.1f) {
        return originalVertex;
    }
    
    // Apply circular transformation
    float blendFactor = 0.7f; // How much to round (0.7 = 70% round, 30% original)
    float newRadius = radius * (1.0f - blendFactor) + targetRadius * blendFactor;
    
    // Reconstruct position with new radius
    rounded.x = newRadius * cosf(angle);
    rounded.z = newRadius * sinf(angle);
    // Keep original Y coordinate
    rounded.y = originalVertex.y;
    
    return rounded;
}

// This helper just draws the raw leg model triangles with rounded cross-sections.
void drawLegModel(bool mirrorX) {
    float mirror = mirrorX ? -1.0f : 1.0f;
    glColor3f(0.85f, 0.64f, 0.52f);
    glBegin(GL_TRIANGLES);
    for (const auto& t : gTris) {
        // Apply rounding to vertices
        const Vec3f& n0 = gVertexNormals[t.a]; 
        Vec3f v0 = makeRoundedVertex(gFootVertices[t.a]);
        glNormal3f(n0.x * mirror, n0.y, n0.z); glVertex3f(v0.x * mirror, v0.y * LEG_SCALE, v0.z);
        
        const Vec3f& n1 = gVertexNormals[t.b]; 
        Vec3f v1 = makeRoundedVertex(gFootVertices[t.b]);
        glNormal3f(n1.x * mirror, n1.y, n1.z); glVertex3f(v1.x * mirror, v1.y * LEG_SCALE, v1.z);
        
        const Vec3f& n2 = gVertexNormals[t.c]; 
        Vec3f v2 = makeRoundedVertex(gFootVertices[t.c]);
        glNormal3f(n2.x * mirror, n2.y, n2.z); glVertex3f(v2.x * mirror, v2.y * LEG_SCALE, v2.z);
    }
    glEnd();
}

// MODIFIED: This function now takes separate thigh and knee angles.
void drawLeg(bool mirrorX, float thighAngle, float kneeAngle) {
    // Hip connects at the top of the leg mesh (8.2f scaled)
    // This matches the body positioning in drawBodyAndHead
    const float hipHeight = 8.2f * LEG_SCALE; // Top of leg mesh connects to body bottom
    const float kneeHeight = 3.9f * LEG_SCALE; // The Y-coordinate of the knee joint

    // --- PART 1: Draw Upper Leg (Thigh) ---
    glPushMatrix();
    {
        // 1. Rotate the thigh from the hip
        glTranslatef(0.0f, hipHeight, 0.0f);
        glRotatef(thighAngle, 1.0f, 0.0f, 0.0f);
        glTranslatef(0.0f, -hipHeight, 0.0f);

        // 2. Clip away the lower leg (everything below the knee)
        GLdouble clipPlaneUpper[] = { 0.0, 1.0, 0.0, -kneeHeight };
        glClipPlane(GL_CLIP_PLANE0, clipPlaneUpper);
        glEnable(GL_CLIP_PLANE0);

        drawLegModel(mirrorX);

        glDisable(GL_CLIP_PLANE0);
    }
    glPopMatrix();

    // --- PART 2: Draw Lower Leg (Calf and Foot) ---
    glPushMatrix();
    {
        // 1. First, apply the same hip rotation to the entire leg
        glTranslatef(0.0f, hipHeight, 0.0f);
        glRotatef(thighAngle, 1.0f, 0.0f, 0.0f);
        glTranslatef(0.0f, -hipHeight, 0.0f);

        // 2. Then, apply the knee bend rotation around the knee joint
        glTranslatef(0.0f, kneeHeight, 0.0f);
        glRotatef(kneeAngle, 1.0f, 0.0f, 0.0f);
        glTranslatef(0.0f, -kneeHeight, 0.0f);

        // 3. Clip away the upper leg (everything above the knee)
        GLdouble clipPlaneLower[] = { 0.0, -1.0, 0.0, kneeHeight };
        glClipPlane(GL_CLIP_PLANE0, clipPlaneLower);
        glEnable(GL_CLIP_PLANE0);

        drawLegModel(mirrorX);

        glDisable(GL_CLIP_PLANE0);
    }
    glPopMatrix();
}
// --- Skirt Drawing ---
void drawSkirt(float leftLegAngle, float rightLegAngle) {
    const float topR = R8 + 0.11f;
    const float topY = Y8 - 0.51f;
    const float botR = 0.65f;  // Reduced from 0.82f for shorter skirt
    const float botY = -0.8f;  // Raised from -1.7f for shorter length
    float deltaY = topY - botY;
    float deltaR = botR - topR;

    // You can still adjust this strength value
    float front_strength = 1.5f;
    float back_strength = 2.5f; // Adjusted this for the new logic

    glColor3f(0.2f, 0.4f, 0.8f);
    glBegin(GL_TRIANGLES);

    for (int i = 0; i < 32; ++i) {
        // --- Calculate interpolated vertex A ---
        int idx_A0 = i / 2;
        int idx_A1 = (idx_A0 + 1) % 16;
        float interp_A = (i % 2) * 0.5f;
        float cA = segCos[idx_A0] * (1.0f - interp_A) + segCos[idx_A1] * interp_A;
        float sA = segSin[idx_A0] * (1.0f - interp_A) + segSin[idx_A1] * interp_A;

        // --- Calculate interpolated vertex B ---
        int next_i = (i + 1) % 32;
        int idx_B0 = next_i / 2;
        int idx_B1 = (idx_B0 + 1) % 16;
        float interp_B = (next_i % 2) * 0.5f;
        float cB = segCos[idx_B0] * (1.0f - interp_B) + segCos[idx_B1] * interp_B;
        float sB = segSin[idx_B0] * (1.0f - interp_B) + segSin[idx_B1] * interp_B;

        Vec3f v_top_A = { topR * cA, topY, topR * sA };
        Vec3f v_bot_A = { botR * cA, botY, botR * sA };
        Vec3f v_top_B = { topR * cB, topY, topR * sB };
        Vec3f v_bot_B = { botR * cB, botY, botR * sB };

        // --- MODIFIED DEFORMATION LOGIC ---
        float angleA = (v_bot_A.x <= 0) ? leftLegAngle : rightLegAngle;
        if (angleA > 0) { // Leg is swinging forward
            float front_weight = (sA > 0.0f) ? sA : 0.0f; // Weight is 1 at front, 0 at sides
            v_bot_A.z += sin(angleA * PI / 180.0f) * front_strength * front_weight;
        }
        else { // Leg is swinging backward
            float back_weight = (-sA > 0.0f) ? -sA : 0.0f; // Weight is 1 at back, 0 at sides
            v_bot_A.z += sin(angleA * PI / 180.0f) * back_strength * back_weight;
        }

        float angleB = (v_bot_B.x <= 0) ? leftLegAngle : rightLegAngle;
        if (angleB > 0) { // Leg is swinging forward
            float front_weight = (sB > 0.0f) ? sB : 0.0f;
            v_bot_B.z += sin(angleB * PI / 180.0f) * front_strength * front_weight;
        }
        else { // Leg is swinging backward
            float back_weight = (-sB > 0.0f) ? -sB : 0.0f;
            v_bot_B.z += sin(angleB * PI / 180.0f) * back_strength * back_weight;
        }

        // --- End of modification ---

        Vec3f nA = normalize({ deltaY * cA, deltaR, deltaY * sA });
        Vec3f nB = normalize({ deltaY * cB, deltaR, deltaY * sB });

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
// Draw Mulan's detailed facial features
void drawMulanEyes() {
    const float headR = HEAD_RADIUS;
    const float eyeY = headR * 0.2f; // Position relative to head center
    const float eyeZ = headR * 0.95f; // Moved forward to face surface
    const float eyeSpacing = 0.1f; // Slightly wider spacing
    
    // Eye whites (almond shaped for East Asian features)
    glColor3f(0.95f, 0.95f, 0.95f);
    
    // Left eye white
    glBegin(GL_POLYGON);
    glVertex3f(-eyeSpacing - 0.04f, eyeY + 0.02f, eyeZ);
    glVertex3f(-eyeSpacing - 0.02f, eyeY + 0.035f, eyeZ);
    glVertex3f(-eyeSpacing + 0.02f, eyeY + 0.035f, eyeZ);
    glVertex3f(-eyeSpacing + 0.04f, eyeY + 0.02f, eyeZ);
    glVertex3f(-eyeSpacing + 0.02f, eyeY - 0.02f, eyeZ);
    glVertex3f(-eyeSpacing - 0.02f, eyeY - 0.02f, eyeZ);
    glEnd();
    
    // Right eye white
    glBegin(GL_POLYGON);
    glVertex3f(eyeSpacing - 0.04f, eyeY + 0.02f, eyeZ);
    glVertex3f(eyeSpacing - 0.02f, eyeY + 0.035f, eyeZ);
    glVertex3f(eyeSpacing + 0.02f, eyeY + 0.035f, eyeZ);
    glVertex3f(eyeSpacing + 0.04f, eyeY + 0.02f, eyeZ);
    glVertex3f(eyeSpacing + 0.02f, eyeY - 0.02f, eyeZ);
    glVertex3f(eyeSpacing - 0.02f, eyeY - 0.02f, eyeZ);
    glEnd();
    
    // Dark brown irises
    glColor3f(0.2f, 0.1f, 0.05f);
    glBegin(GL_POLYGON);
    for (float a = 0; a < 6.28f; a += 0.3f) {
        glVertex3f(-eyeSpacing + 0.025f * cos(a), eyeY + 0.025f * sin(a), eyeZ + 0.001f);
    }
    glEnd();
    
    glBegin(GL_POLYGON);
    for (float a = 0; a < 6.28f; a += 0.3f) {
        glVertex3f(eyeSpacing + 0.025f * cos(a), eyeY + 0.025f * sin(a), eyeZ + 0.001f);
    }
    glEnd();
    
    // Black pupils
    glColor3f(0.0f, 0.0f, 0.0f);
    glBegin(GL_POLYGON);
    for (float a = 0; a < 6.28f; a += 0.5f) {
        glVertex3f(-eyeSpacing + 0.012f * cos(a), eyeY + 0.012f * sin(a), eyeZ + 0.002f);
    }
    glEnd();
    
    glBegin(GL_POLYGON);
    for (float a = 0; a < 6.28f; a += 0.5f) {
        glVertex3f(eyeSpacing + 0.012f * cos(a), eyeY + 0.012f * sin(a), eyeZ + 0.002f);
    }
    glEnd();
    
    // Eye highlights
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_POLYGON);
    for (float a = 0; a < 6.28f; a += 1.0f) {
        glVertex3f(-eyeSpacing - 0.008f + 0.004f * cos(a), eyeY + 0.008f + 0.004f * sin(a), eyeZ + 0.003f);
    }
    glEnd();
    
    glBegin(GL_POLYGON);
    for (float a = 0; a < 6.28f; a += 1.0f) {
        glVertex3f(eyeSpacing - 0.008f + 0.004f * cos(a), eyeY + 0.008f + 0.004f * sin(a), eyeZ + 0.003f);
    }
    glEnd();
}

void drawMulanEyebrows() {
    const float headR = HEAD_RADIUS;
    const float browY = headR * 0.32f; // Position relative to head center
    const float browZ = headR * 0.92f; // Moved forward to face surface
    const float eyeSpacing = 0.1f; // Match eye spacing
    
    // Dark brown eyebrows
    glColor3f(0.15f, 0.1f, 0.05f);
    glLineWidth(3.0f);
    
    // Left eyebrow
    glBegin(GL_LINE_STRIP);
    glVertex3f(-eyeSpacing - 0.04f, browY, browZ);
    glVertex3f(-eyeSpacing - 0.02f, browY + 0.015f, browZ);
    glVertex3f(-eyeSpacing, browY + 0.018f, browZ);
    glVertex3f(-eyeSpacing + 0.02f, browY + 0.012f, browZ);
    glVertex3f(-eyeSpacing + 0.04f, browY + 0.005f, browZ);
    glEnd();
    
    // Right eyebrow
    glBegin(GL_LINE_STRIP);
    glVertex3f(eyeSpacing - 0.04f, browY + 0.005f, browZ);
    glVertex3f(eyeSpacing - 0.02f, browY + 0.012f, browZ);
    glVertex3f(eyeSpacing, browY + 0.018f, browZ);
    glVertex3f(eyeSpacing + 0.02f, browY + 0.015f, browZ);
    glVertex3f(eyeSpacing + 0.04f, browY, browZ);
    glEnd();
    
    glLineWidth(1.0f);
}

void drawMulanNose() {
    const float headR = HEAD_RADIUS;
    const float noseY = -headR * 0.02f; // Position relative to head center
    const float noseZ = headR * 0.98f; // Moved forward to face surface
    
    // Nose bridge (subtle for East Asian features)
    glColor3f(0.85f, 0.65f, 0.55f); // Slightly darker than skin
    glLineWidth(2.0f);
    
    glBegin(GL_LINE_STRIP);
    glVertex3f(0.0f, noseY + 0.03f, noseZ);
    glVertex3f(0.0f, noseY, noseZ + 0.01f);
    glVertex3f(0.0f, noseY - 0.01f, noseZ);
    glEnd();
    
    // Nostrils
    glColor3f(0.3f, 0.2f, 0.15f);
    glBegin(GL_POINTS);
    glVertex3f(-0.008f, noseY - 0.015f, noseZ);
    glVertex3f(0.008f, noseY - 0.015f, noseZ);
    glEnd();
    
    glLineWidth(1.0f);
}

void drawMulanMouth() {
    const float headR = HEAD_RADIUS;
    const float mouthY = -headR * 0.3f; // Position relative to head center
    const float mouthZ = headR * 0.95f; // Moved forward to face surface
    
    // Natural lip color
    glColor3f(0.8f, 0.4f, 0.4f);
    glLineWidth(2.5f);
    
    // Upper lip curve
    glBegin(GL_LINE_STRIP);
    glVertex3f(-0.025f, mouthY + 0.005f, mouthZ);
    glVertex3f(-0.012f, mouthY + 0.012f, mouthZ);
    glVertex3f(0.0f, mouthY + 0.008f, mouthZ);
    glVertex3f(0.012f, mouthY + 0.012f, mouthZ);
    glVertex3f(0.025f, mouthY + 0.005f, mouthZ);
    glEnd();
    
    // Lower lip
    glBegin(GL_LINE_STRIP);
    glVertex3f(-0.025f, mouthY + 0.005f, mouthZ);
    glVertex3f(-0.012f, mouthY - 0.005f, mouthZ);
    glVertex3f(0.0f, mouthY - 0.008f, mouthZ);
    glVertex3f(0.012f, mouthY - 0.005f, mouthZ);
    glVertex3f(0.025f, mouthY + 0.005f, mouthZ);
    glEnd();
    
    glLineWidth(1.0f);
}

void drawMulanHair() {
    const float headR = HEAD_RADIUS;
    const int segments = 32;
    
    glColor3f(0.05f, 0.05f, 0.1f); // Dark black hair
    
    // --- Part 1: Voluminous Hair Cap ---
    // A slightly larger cap to give the hair body.
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < segments; i++) {
        float angle1 = (float)i / segments * 2.0f * PI;
        float angle2 = (float)(i + 1) / segments * 2.0f * PI;
        
        float capRadius = headR * 1.15f; // Increased to avoid clipping
        float x1 = capRadius * cos(angle1);
        float z1 = capRadius * sin(angle1);
        float x2 = capRadius * cos(angle2);
        float z2 = capRadius * sin(angle2);
        
        // A high center point for volume
        Vec3f top_center = {0.0f, headR * 1.25f, 0.0f}; // Slightly higher
        // The edge of the cap - positioned outward from head surface
        Vec3f edge_v1 = {x1, headR * 0.75f, z1};
        Vec3f edge_v2 = {x2, headR * 0.75f, z2};

        // Normal for the triangle
        Vec3f n = normalize(cross(sub(edge_v1, top_center), sub(edge_v2, top_center)));
        glNormal3f(n.x, n.y, n.z);

        glVertex3f(top_center.x, top_center.y, top_center.z);
        glVertex3f(edge_v1.x, edge_v1.y, edge_v1.z);
        glVertex3f(edge_v2.x, edge_v2.y, edge_v2.z);
    }
    glEnd();
    
    // --- Part 2: Main Bob Shape (Sides & Back) ---
    // Chin-length hair with an inward curve.
    glBegin(GL_QUADS);
    for (int i = 0; i < segments; i++) {
        float angle1 = (float)i / segments * 2.0f * PI;
        float angle2 = (float)(i + 1) / segments * 2.0f * PI;

        // Skip the very front to leave space for bangs
        if (sin(angle1) > 0.3f && fabsf(cos(angle1)) < 0.8f) {
            continue;
        }

        // Top of the strand - positioned outward from head
        float topRadius = headR * 1.15f; // Increased to clear head surface
        float topY = headR * 0.75f;
        float x1_top = topRadius * cos(angle1);
        float z1_top = topRadius * sin(angle1);
        float x2_top = topRadius * cos(angle2);
        float z2_top = topRadius * sin(angle2);

        // Bottom of the strand with different lengths
        float bottomRadius = headR * 0.95f; // Less inward curve to avoid clipping
        float bottomY = -headR * 0.5f; // Default chin-length
        
        // Make sides very short (undercut style)
        if (fabsf(cos(angle1)) > 0.5f) { // Left and right sides
            bottomY = headR * 0.1f; // Very short - just below ear level
            bottomRadius = headR * 1.25f; // Keep close to head
        }
        // Make back much longer (shoulder-length)
        else if (sin(angle1) < -0.3) { // Back of the head
            bottomY = -headR * 1.0f; // Much longer back - shoulder length
            bottomRadius = headR * 1.25f; // Less aggressive inward curve for longer hair
        }

        float x1_bot = bottomRadius * cos(angle1);
        float z1_bot = bottomRadius * sin(angle1);
        float x2_bot = bottomRadius * cos(angle2);
        float z2_bot = bottomRadius * sin(angle2);

        Vec3f v_top1 = {x1_top, topY, z1_top};
        Vec3f v_top2 = {x2_top, topY, z2_top};
        Vec3f v_bot1 = {x1_bot, bottomY, z1_bot};
        Vec3f v_bot2 = {x2_bot, bottomY, z2_bot};

        Vec3f n = normalize(cross(sub(v_top2, v_top1), sub(v_bot1, v_top1)));
        glNormal3f(n.x, n.y, n.z);

        glVertex3f(v_top1.x, v_top1.y, v_top1.z);
        glVertex3f(v_top2.x, v_top2.y, v_top2.z);
        glVertex3f(v_bot2.x, v_bot2.y, v_bot2.z);
        glVertex3f(v_bot1.x, v_bot1.y, v_bot1.z);
    }
    glEnd();

    // --- Part 3: French Fringe (Bangs) ---
    // Straight-cut bangs across the forehead.
    glBegin(GL_QUADS);
    for (int i = 0; i < segments; i++) {
        float angle1 = (float)i / segments * 2.0f * PI;
        float angle2 = (float)(i + 1) / segments * 2.0f * PI;

        // Only draw bangs in the front center section - no side bangs
        if (sin(angle1) < 0.4f || fabsf(cos(angle1)) > 0.6f) {
             continue;
        }

        // Top of bangs - positioned outward from head
        float topRadius = headR * 1.15f; // Increased to clear head surface
        float topY = headR * 0.8f;
        float x1_top = topRadius * cos(angle1);
        float z1_top = topRadius * sin(angle1);
        float x2_top = topRadius * cos(angle2);
        float z2_top = topRadius * sin(angle2);

        // Bottom of bangs (just above eyebrows)
        float bottomRadius = headR * 1.08f; // Slightly outward to avoid clipping
        float bottomY = headR * 0.3f; // Eyebrow level is browY = headR * 0.32f
        float x1_bot = bottomRadius * cos(angle1);
        float z1_bot = bottomRadius * sin(angle1);
        float x2_bot = bottomRadius * cos(angle2);
        float z2_bot = bottomRadius * sin(angle2);

        Vec3f v_top1 = {x1_top, topY, z1_top};
        Vec3f v_top2 = {x2_top, topY, z2_top};
        Vec3f v_bot1 = {x1_bot, bottomY, z1_bot};
        Vec3f v_bot2 = {x2_bot, bottomY, z2_bot};
        
        Vec3f n = normalize(cross(sub(v_top2, v_top1), sub(v_bot1, v_top1)));
        glNormal3f(n.x, n.y, n.z);

        glVertex3f(v_top1.x, v_top1.y, v_top1.z);
        glVertex3f(v_top2.x, v_top2.y, v_top2.z);
        glVertex3f(v_bot2.x, v_bot2.y, v_bot2.z);
        glVertex3f(v_bot1.x, v_bot1.y, v_bot1.z);
    }
    glEnd();
}

void drawMulanFace() {
    drawMulanHair();
    drawMulanEyebrows();
    drawMulanEyes();
    drawMulanNose();
    drawMulanMouth();
}

// Draw custom U-shaped face geometry
void drawCustomFaceShape() {
    // Define face parameters - much larger to match original sphere size
    const float faceWidth = HEAD_RADIUS * 1.1f;
    const float faceHeight = HEAD_RADIUS * 1.4f;
    const float faceDepth = HEAD_RADIUS * 0.9f;
    const int segments = 20; // Horizontal segments
    const int layers = 16;   // Vertical layers
    
    glBegin(GL_TRIANGLES);
    
    for (int layer = 0; layer < layers - 1; layer++) {
        float y1 = ((float)layer / (layers - 1)) * 2.0f - 1.0f; // -1 to 1
        float y2 = ((float)(layer + 1) / (layers - 1)) * 2.0f - 1.0f;
        
        // Convert to actual Y coordinates
        float actualY1 = y1 * faceHeight * 0.5f;
        float actualY2 = y2 * faceHeight * 0.5f;
        
        // Calculate face width scaling based on height for U-shaped jaw
        float widthScale1, widthScale2;
        
        if (y1 < -0.2f) { // Lower jaw area - create U-shape
            float jawFactor = (y1 + 1.0f) / 0.8f; // 0 at bottom, 1 at jaw line
            widthScale1 = 0.5f + 0.5f * jawFactor * jawFactor; // Curved U-shape, less extreme
        } else if (y1 < 0.4f) { // Mid face - full width
            widthScale1 = 1.0f;
        } else { // Upper face/forehead
            float foreheadFactor = (y1 - 0.4f) / 0.6f;
            widthScale1 = 1.0f - 0.1f * foreheadFactor; // Slightly narrower forehead
        }
        
        if (y2 < -0.2f) { // Lower jaw area - create U-shape
            float jawFactor = (y2 + 1.0f) / 0.8f;
            widthScale2 = 0.5f + 0.5f * jawFactor * jawFactor; // Curved U-shape, less extreme
        } else if (y2 < 0.4f) { // Mid face - full width
            widthScale2 = 1.0f;
        } else { // Upper face/forehead
            float foreheadFactor = (y2 - 0.4f) / 0.6f;
            widthScale2 = 1.0f - 0.1f * foreheadFactor; // Slightly narrower forehead
        }
        
        // Calculate depth scaling for more natural face shape
        float depthScale1 = 1.0f - 0.1f * fabsf(y1); // Less flattening for better proportions
        float depthScale2 = 1.0f - 0.1f * fabsf(y2);
        
        for (int seg = 0; seg < segments; seg++) {
            float angle1 = (float)seg / segments * 2.0f * PI;
            float angle2 = (float)(seg + 1) / segments * 2.0f * PI;
            
            // Calculate vertex positions for current layer
            float x1_1 = cos(angle1) * faceWidth * widthScale1;
            float z1_1 = sin(angle1) * faceDepth * depthScale1;
            float x1_2 = cos(angle2) * faceWidth * widthScale1;
            float z1_2 = sin(angle2) * faceDepth * depthScale1;
            
            // Calculate vertex positions for next layer
            float x2_1 = cos(angle1) * faceWidth * widthScale2;
            float z2_1 = sin(angle1) * faceDepth * depthScale2;
            float x2_2 = cos(angle2) * faceWidth * widthScale2;
            float z2_2 = sin(angle2) * faceDepth * depthScale2;
            
            // Calculate normals for smooth shading
            Vec3f normal1 = normalize({x1_1, actualY1 * 0.3f, z1_1});
            Vec3f normal2 = normalize({x1_2, actualY1 * 0.3f, z1_2});
            Vec3f normal3 = normalize({x2_1, actualY2 * 0.3f, z2_1});
            Vec3f normal4 = normalize({x2_2, actualY2 * 0.3f, z2_2});
            
            // First triangle
            glNormal3f(normal1.x, normal1.y, normal1.z);
            glVertex3f(x1_1, actualY1, z1_1);
            glNormal3f(normal3.x, normal3.y, normal3.z);
            glVertex3f(x2_1, actualY2, z2_1);
            glNormal3f(normal2.x, normal2.y, normal2.z);
            glVertex3f(x1_2, actualY1, z1_2);
            
            // Second triangle
            glNormal3f(normal2.x, normal2.y, normal2.z);
            glVertex3f(x1_2, actualY1, z1_2);
            glNormal3f(normal3.x, normal3.y, normal3.z);
            glVertex3f(x2_1, actualY2, z2_1);
            glNormal3f(normal4.x, normal4.y, normal4.z);
            glVertex3f(x2_2, actualY2, z2_2);
        }
    }
    
    // Cap the top and bottom
    // Top cap
    for (int seg = 0; seg < segments; seg++) {
        float angle1 = (float)seg / segments * 2.0f * PI;
        float angle2 = (float)(seg + 1) / segments * 2.0f * PI;
        
        float topWidthScale = 0.9f; // Slightly narrower at top
        float x1 = cos(angle1) * faceWidth * topWidthScale;
        float z1 = sin(angle1) * faceDepth * 0.8f;
        float x2 = cos(angle2) * faceWidth * topWidthScale;
        float z2 = sin(angle2) * faceDepth * 0.8f;
        
        glNormal3f(0, 1, 0);
        glVertex3f(0, faceHeight * 0.5f, 0);
        glVertex3f(x1, faceHeight * 0.5f, z1);
        glVertex3f(x2, faceHeight * 0.5f, z2);
    }
    
    // Bottom cap (chin area)
    for (int seg = 0; seg < segments; seg++) {
        float angle1 = (float)seg / segments * 2.0f * PI;
        float angle2 = (float)(seg + 1) / segments * 2.0f * PI;
        
        float bottomWidthScale = 0.5f; // Less narrow at chin for better proportions
        float x1 = cos(angle1) * faceWidth * bottomWidthScale;
        float z1 = sin(angle1) * faceDepth * 0.6f;
        float x2 = cos(angle2) * faceWidth * bottomWidthScale;
        float z2 = sin(angle2) * faceDepth * 0.6f;
        
        glNormal3f(0, -1, 0);
        glVertex3f(0, -faceHeight * 0.5f, 0);
        glVertex3f(x2, -faceHeight * 0.5f, z2);
        glVertex3f(x1, -faceHeight * 0.5f, z1);
    }
    
    glEnd();
}

void drawMulanHead() {
    glPushMatrix();
    
    // Position head to sit directly on top of neck sphere at Y19 level
    // Head bottom should touch neck top, overlapping slightly for seamless connection
    glTranslatef(0.0f, Y19 + HEAD_RADIUS * 0.7f, 0.0f);
    
    // Mulan's skin tone (East Asian complexion)
    glColor3f(0.92f, 0.76f, 0.65f);
    
    // Draw custom U-shaped face instead of sphere
    drawCustomFaceShape();
    
    // Add head bottom cap to seal connection with neck
    const float headBottomRadius = HEAD_RADIUS * 0.3f; // Smaller radius for neck connection
    const float headBottomY = -HEAD_RADIUS * 0.7f; // Bottom of head
    
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < 16; i++) {
        float angle1 = (float)i / 16.0f * 2.0f * PI;
        float angle2 = (float)(i + 1) / 16.0f * 2.0f * PI;
        
        // Create smooth bottom surface
        Vec3f center = {0.0f, headBottomY, 0.0f};
        Vec3f v1 = {headBottomRadius * cos(angle1), headBottomY, headBottomRadius * sin(angle1)};
        Vec3f v2 = {headBottomRadius * cos(angle2), headBottomY, headBottomRadius * sin(angle2)};
        
        // Normal pointing downward (since this is the bottom of head)
        glNormal3f(0, -1, 0);
        
        // Triangle: center -> v2 -> v1 (clockwise from below for correct winding)
        glVertex3f(center.x, center.y, center.z);
        glVertex3f(v2.x, v2.y, v2.z);
        glVertex3f(v1.x, v1.y, v1.z);
    }
    glEnd();
    
    // Draw detailed facial features
    drawMulanFace();
    
    glPopMatrix();
}
void drawTorso() {
    // Draw main torso body (clothing) with smooth shading
    glColor3f(0.2f, 0.4f, 0.8f);
    glShadeModel(GL_SMOOTH); // Enable smooth shading for seamless appearance
    glBegin(GL_TRIANGLES);
    drawCurvedBand(R0, Y0, R1, Y1);
    drawCurvedBand(R1, Y1, R2, Y2);
    drawCurvedBand(R2, Y2, R3, Y3);
    drawCurvedBand(R3, Y3, R4, Y4);
    drawCurvedBand(R4, Y4, R5, Y5);
    drawCurvedBand(R5, Y5, R6, Y6);
    drawCurvedBand(R6, Y6, R7, Y7);
    drawCurvedBand(R7, Y7, R8, Y8);
    drawCurvedBand(R8, Y8, R9, Y9);
    drawCurvedBand(R9, Y9, R10, Y10);
    drawCurvedBand(R10, Y10, R11, Y11);
    drawCurvedBand(R11, Y11, R12, Y12);
    drawCurvedBand(R12, Y12, R13, Y13);
    drawCurvedBand(R13, Y13, R14, Y14);
    drawCurvedBand(R14, Y14, R15, Y15);
    drawCurvedBand(R15, Y15, R16, Y16); // Extended torso to include shoulder transition
    glEnd();
    
    // Add bottom cap to close the torso (prevent seeing through from below)
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < 16; ++i) {
        float cA = segCos[i];
        float sA = segSin[i];
        float cB = segCos[(i + 1) % 16];
        float sB = segSin[(i + 1) % 16];
        
        // Create bottom surface at Y0 level with R0 radius
        Vec3f center = {0.0f, Y0, 0.0f};
        Vec3f v1 = {R0 * cA, Y0, R0 * sA};
        Vec3f v2 = {R0 * cB, Y0, R0 * sB};
        
        // Apply same curvature as torso
        if (sA < 0) v1.z *= 0.5f; // Back curvature
        if (sB < 0) v2.z *= 0.5f; // Back curvature
        
        // Normal pointing downward (since this is the bottom)
        glNormal3f(0, -1, 0);
        
        // Triangle: center -> v1 -> v2 (clockwise from below)
        glVertex3f(center.x, center.y, center.z);
        glVertex3f(v2.x, v2.y, v2.z); // Reversed order for correct winding
        glVertex3f(v1.x, v1.y, v1.z);
    }
    glEnd();
    
    glShadeModel(GL_FLAT); // Reset to default shading

    // Draw skin areas (shoulder slope and neck) with smooth shading - feminine curves
    // All bands are automatically centered on torso middle (X=0, Z=0)
    glColor3f(0.9f, 0.7f, 0.6f);
    glShadeModel(GL_SMOOTH); // Smooth shading for seamless shoulder-neck transition
    glBegin(GL_TRIANGLES);
    drawCurvedBand(R16, Y16, R17, Y17); // Upper shoulder slope - elegant feminine curve
    drawCurvedBand(R17, Y17, R18, Y18); // Lower shoulder slope to neck base
    drawCurvedBand(R18, Y18, R19, Y19); // Neck - slim and graceful, centered on torso
    glEnd();
    
    // Add neck top cap to close the neck (smooth transition to head)
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < 16; ++i) {
        float cA = segCos[i];
        float sA = segSin[i];
        float cB = segCos[(i + 1) % 16];
        float sB = segSin[(i + 1) % 16];
        
        // Create top surface at Y19 level with R19 radius
        Vec3f center = {0.0f, Y19, 0.0f};
        Vec3f v1 = {R19 * cA, Y19, R19 * sA};
        Vec3f v2 = {R19 * cB, Y19, R19 * sB};
        
        // Normal pointing upward (since this is the top of neck)
        glNormal3f(0, 1, 0);
        
        // Triangle: center -> v1 -> v2 (counter-clockwise from above)
        glVertex3f(center.x, center.y, center.z);
        glVertex3f(v1.x, v1.y, v1.z);
        glVertex3f(v2.x, v2.y, v2.z);
    }
    glEnd();
    
    glShadeModel(GL_FLAT); // Reset to default shading

    // Draw internal ball-and-socket joints (hidden inside torso and arms)
    drawInternalShoulderJoints();

    drawMulanHead();
}

// Draw internal ball-and-socket joints (hidden inside torso and arms)
void drawInternalShoulderJoints() {
    const float shoulderOffsetTorso = R14 * 0.90f;  // Ball position lowered to R14 level
    const float shoulderOffsetArm = R14 * 0.85f; // Socket position at lower torso connection
    const float jointHeight = Y14; // Position lowered to R14 level for better integration
    const float ballRadius = 0.08f;
    const float socketRadius = 0.10f;
    
    // --- Left Shoulder Ball (inside blue torso R14) ---
    glPushMatrix();
    glColor3f(0.2f, 0.4f, 0.8f); // Match blue torso color
    glTranslatef(-shoulderOffsetTorso, jointHeight, 0.1f); // Positioned at blue torso boundary
    
    // Clip ball to only show inside torso (hide external part)
    GLdouble clipPlaneLeft[] = { -1.0, 0.0, 0.0, -shoulderOffsetTorso + 0.1f };
    glClipPlane(GL_CLIP_PLANE0, clipPlaneLeft);
    glEnable(GL_CLIP_PLANE0);
    
    // Draw ball hemisphere (only internal part visible)
    GLUquadric* ballQuadric = gluNewQuadric();
    if (ballQuadric) {
        gluSphere(ballQuadric, ballRadius, 16, 16);
        gluDeleteQuadric(ballQuadric);
    }
    glDisable(GL_CLIP_PLANE0);
    glPopMatrix();
    
    // --- Right Shoulder Ball (inside blue torso R14) ---
    glPushMatrix();
    glColor3f(0.2f, 0.4f, 0.8f); // Match blue torso color
    glTranslatef(shoulderOffsetTorso, jointHeight, 0.1f); // Positioned at blue torso boundary
    
    // Clip ball to only show inside torso
    GLdouble clipPlaneRight[] = { 1.0, 0.0, 0.0, -shoulderOffsetTorso + 0.1f };
    glClipPlane(GL_CLIP_PLANE0, clipPlaneRight);
    glEnable(GL_CLIP_PLANE0);
    
    if (ballQuadric) {
        ballQuadric = gluNewQuadric();
        gluSphere(ballQuadric, ballRadius, 16, 16);
        gluDeleteQuadric(ballQuadric);
    }
    glDisable(GL_CLIP_PLANE0);
    glPopMatrix();
}

// Original shoulder socket function (now unused)
void drawShoulderSocketsOLD() {
    glColor3f(0.9f, 0.7f, 0.6f); // Skin color to match neck

    // --- Shoulder parameters integrated with torso ---
    const float shoulderRadius = 0.12f;     // Smaller radius to blend better
    const float shoulderHeight = Y15;       // Position at torso shoulder level
    const float shoulderOffset = R14 * 0.85f; // Slightly inward for better integration
    const int slices = 12;                  
    const int stacks = 4;                   // Fewer stacks for flatter integration

    // --- Draw Left Shoulder Socket (clipped to blend with torso) ---
    glPushMatrix();
    glTranslatef(-shoulderOffset, shoulderHeight, 0.0f);
    
    // Clip to blend seamlessly with torso
    GLdouble clipPlane[] = { 1.0, 0.0, 0.0, shoulderOffset * 0.3f }; 
    glClipPlane(GL_CLIP_PLANE0, clipPlane); 
    glEnable(GL_CLIP_PLANE0);

    glBegin(GL_TRIANGLES);
    // Generate a hemisphere (top half of a sphere)
    for (int i = 0; i < stacks; i++) {
        float lat0 = PI * (0.0f + (float)i / stacks / 2.0f);      // Latitude from 0 to PI/2 (top)
        float lat1 = PI * (0.0f + (float)(i + 1) / stacks / 2.0f);

        float y0 = shoulderRadius * sinf(lat0);
        float y1 = shoulderRadius * sinf(lat1);
        float r0 = shoulderRadius * cosf(lat0);
        float r1 = shoulderRadius * cosf(lat1);

        for (int j = 0; j < slices; j++) {
            float lng0 = 2 * PI * (float)j / slices;
            float lng1 = 2 * PI * (float)(j + 1) / slices;

            // Vertices of the quad for this segment
            float x00 = r0 * cosf(lng0); float z00 = r0 * sinf(lng0);
            float x01 = r0 * cosf(lng1); float z01 = r0 * sinf(lng1);
            float x10 = r1 * cosf(lng0); float z10 = r1 * sinf(lng0);
            float x11 = r1 * cosf(lng1); float z11 = r1 * sinf(lng1);

            // Normals for smooth lighting (points outward from the center)
            Vec3f n00 = normalize({ x00, y0, z00 });
            Vec3f n01 = normalize({ x01, y0, z01 });
            Vec3f n10 = normalize({ x10, y1, z10 });
            Vec3f n11 = normalize({ x11, y1, z11 });

            // First Triangle
            glNormal3f(n00.x, n00.y, n00.z); glVertex3f(x00, y0, z00);
            glNormal3f(n10.x, n10.y, n10.z); glVertex3f(x10, y1, z10);
            glNormal3f(n01.x, n01.y, n01.z); glVertex3f(x01, y0, z01);

            // Second Triangle
            glNormal3f(n01.x, n01.y, n01.z); glVertex3f(x01, y0, z01);
            glNormal3f(n10.x, n10.y, n10.z); glVertex3f(x10, y1, z10);
            glNormal3f(n11.x, n11.y, n11.z); glVertex3f(x11, y1, z11);
        }
    }
    glEnd();
    glDisable(GL_CLIP_PLANE0); // Disable clipping after left shoulder
    glPopMatrix();

    // --- Draw Right Shoulder Socket (mirrored and clipped) ---
    glPushMatrix();
    glTranslatef(shoulderOffset, shoulderHeight, 0.0f);
    
    // Clip to blend seamlessly with torso (mirrored)
    GLdouble clipPlaneRight[] = { -1.0, 0.0, 0.0, shoulderOffset * 0.3f }; 
    glClipPlane(GL_CLIP_PLANE0, clipPlaneRight); 
    glEnable(GL_CLIP_PLANE0);

    glBegin(GL_TRIANGLES);
    // Generate a hemisphere, mirrored on the X-axis
    for (int i = 0; i < stacks; i++) {
        float lat0 = PI * (0.0f + (float)i / stacks / 2.0f);
        float lat1 = PI * (0.0f + (float)(i + 1) / stacks / 2.0f);

        float y0 = shoulderRadius * sinf(lat0);
        float y1 = shoulderRadius * sinf(lat1);
        float r0 = shoulderRadius * cosf(lat0);
        float r1 = shoulderRadius * cosf(lat1);

        for (int j = 0; j < slices; j++) {
            float lng0 = 2 * PI * (float)j / slices;
            float lng1 = 2 * PI * (float)(j + 1) / slices;

            // Vertices mirrored on X
            float x00 = -r0 * cosf(lng0); float z00 = r0 * sinf(lng0);
            float x01 = -r0 * cosf(lng1); float z01 = r0 * sinf(lng1);
            float x10 = -r1 * cosf(lng0); float z10 = r1 * sinf(lng0);
            float x11 = -r1 * cosf(lng1); float z11 = r1 * sinf(lng1);

            // Normals mirrored on X for correct lighting
            Vec3f n00 = normalize({ x00, y0, z00 });
            Vec3f n01 = normalize({ x01, y0, z01 });
            Vec3f n10 = normalize({ x10, y1, z10 });
            Vec3f n11 = normalize({ x11, y1, z11 });

            // First Triangle (winding order reversed for correct facing)
            glNormal3f(n00.x, n00.y, n00.z); glVertex3f(x00, y0, z00);
            glNormal3f(n01.x, n01.y, n01.z); glVertex3f(x01, y0, z01);
            glNormal3f(n10.x, n10.y, n10.z); glVertex3f(x10, y1, z10);

            // Second Triangle (winding order reversed)
            glNormal3f(n01.x, n01.y, n01.z); glVertex3f(x01, y0, z01);
            glNormal3f(n11.x, n11.y, n11.z); glVertex3f(x11, y1, z11);
            glNormal3f(n10.x, n10.y, n10.z); glVertex3f(x10, y1, z10);
        }
    }
    glEnd();
    glDisable(GL_CLIP_PLANE0); // Disable clipping after right shoulder
    glPopMatrix();
}
// --- Arm and Hand Drawing ---
void drawArmsAndHands(float leftArmAngle, float rightArmAngle) {
    // Default arm pose - arms extended outward from sides
    float leftShoulderPitch = 90.0f + leftArmAngle;
    float rightShoulderPitch = 90.0f + rightArmAngle;
    float leftShoulderYaw = -20.0f;   // Left arm rotated to the left
    float rightShoulderYaw = 20.0f;   // Right arm rotated to the right
    float leftShoulderRoll = -40.0f;  // Roll left arm outward more
    float rightShoulderRoll = 40.0f;  // Roll right arm outward more
    float defaultElbowBend = 20.0f;
    float defaultWristPitch = 0.0f;
    float defaultWristYaw = 0.0f;
    float defaultWristRoll = 0.0f;

    // Enable texturing if available
    if (g_TextureEnabled && g_HandTexture != 0) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, g_HandTexture);
        glColor3f(1.0f, 1.0f, 1.0f); // White to show texture clearly
    }
    else {
        glDisable(GL_TEXTURE_2D);
        glColor3f(0.85f, 0.64f, 0.52f); // Skin color
    }

    // --- Draw Left Arm (connected to shoulder peak R15) ---
    glPushMatrix();
    {
        float shoulderOffset = R14 * 1.05f; // Lowered to R14 level with clearance
        glTranslatef(-shoulderOffset, Y14, 0.1f); // Position at lowered shoulder level
        glRotatef(leftShoulderPitch, 1.0f, 0.0f, 0.0f);
        glRotatef(leftShoulderYaw, 0.0f, 1.0f, 0.0f);
        glRotatef(leftShoulderRoll, 0.0f, 0.0f, 1.0f);

        // Draw internal shoulder socket (connected to shoulder band R15)
        glPushMatrix();
        glColor3f(0.85f, 0.64f, 0.52f); // Match arm color
        glTranslatef(0.0f, 0.0f, -0.05f); // Socket position aligned with shoulder band connection
        
        // Clip socket to only show inside arm (hide external part)
        GLdouble clipPlaneArmLeft[] = { 1.0, 0.0, 0.0, 0.1f };
        glClipPlane(GL_CLIP_PLANE1, clipPlaneArmLeft);
        glEnable(GL_CLIP_PLANE1);
        
        // Draw internal socket cavity
        GLUquadric* socketQuadric = gluNewQuadric();
        if (socketQuadric) {
            gluQuadricOrientation(socketQuadric, GLU_INSIDE); // Inward-facing for socket
            gluSphere(socketQuadric, 0.10f, 12, 12);
            gluDeleteQuadric(socketQuadric);
        }
        glDisable(GL_CLIP_PLANE1);
        glPopMatrix();

        glPushMatrix();
        glScalef(ARM_SCALE * 0.9f, ARM_SCALE * 0.9f, ARM_SCALE * 0.9f); // Slightly smaller to avoid clipping
        drawLowPolyArm(g_ArmJoints);
        glPopMatrix();

        Vec3 elbowPos = g_ArmJoints[3].position;
        glTranslatef(elbowPos.x * ARM_SCALE, elbowPos.y * ARM_SCALE, elbowPos.z * ARM_SCALE);
        glRotatef(defaultElbowBend - 15.0f, 1.0f, 0.0f, 0.0f);
        glTranslatef(-elbowPos.x * ARM_SCALE, -elbowPos.y * ARM_SCALE, -elbowPos.z * ARM_SCALE);

        Vec3 leftWristPos = g_ArmJoints.back().position;
        glTranslatef(leftWristPos.x * ARM_SCALE, leftWristPos.y * ARM_SCALE, leftWristPos.z * ARM_SCALE);
        glRotatef(defaultWristPitch, 1.0f, 0.0f, 0.0f);
        glRotatef(defaultWristYaw, 0.0f, 1.0f, 0.0f);
        glRotatef(defaultWristRoll, 0.0f, 0.0f, 1.0f);

        glPushMatrix();
        // Connection to arm wrist - move hand higher to link with arm
        glTranslatef(0.0f, 0.05f, -0.10f);  // Position hand higher to connect with arm end
        glScalef(HAND_SCALE * 1.0f, HAND_SCALE * 1.0f, HAND_SCALE * 1.0f); // Full scale for better connection
        if (g_TextureEnabled && g_HandTexture != 0) {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, g_HandTexture);
            glColor3f(1.0f, 1.0f, 1.0f);
        }

        drawSkinnedPalm(g_HandJoints);
        drawLowPolyThumb(g_HandJoints);
        drawLowPolyFinger(g_HandJoints, 5, 6, 7, 8);
        drawLowPolyFinger(g_HandJoints, 9, 10, 11, 12);
        drawLowPolyFinger(g_HandJoints, 13, 14, 15, 16);
        drawLowPolyFinger(g_HandJoints, 17, 18, 19, 20);
        glPopMatrix();
    }
    glPopMatrix();

    // --- Draw Right Arm (connected to shoulder peak R15) ---
    glPushMatrix();
    {
        float shoulderOffset = R14 * 1.05f; // Lowered to R14 level with clearance
        glTranslatef(shoulderOffset, Y14, 0.1f); // Position at lowered shoulder level
        glRotatef(rightShoulderPitch, 1.0f, 0.0f, 0.0f);
        glRotatef(rightShoulderYaw, 0.0f, 1.0f, 0.0f);
        glRotatef(rightShoulderRoll, 0.0f, 0.0f, 1.0f);

        // Draw internal shoulder socket (connected to shoulder band R15)
        glPushMatrix();
        glColor3f(0.85f, 0.64f, 0.52f); // Match arm color
        glTranslatef(0.0f, 0.0f, -0.05f); // Socket position aligned with shoulder band connection
        
        // Clip socket to only show inside arm (hide external part)  
        GLdouble clipPlaneArmRight[] = { -1.0, 0.0, 0.0, 0.1f };
        glClipPlane(GL_CLIP_PLANE1, clipPlaneArmRight);
        glEnable(GL_CLIP_PLANE1);
        
        // Draw internal socket cavity
        GLUquadric* socketQuadric = gluNewQuadric();
        if (socketQuadric) {
            gluQuadricOrientation(socketQuadric, GLU_INSIDE); // Inward-facing for socket
            gluSphere(socketQuadric, 0.10f, 12, 12);
            gluDeleteQuadric(socketQuadric);
        }
        glDisable(GL_CLIP_PLANE1);
        glPopMatrix();

        glPushMatrix();
        glScalef(ARM_SCALE * 0.9f, ARM_SCALE * 0.9f, ARM_SCALE * 0.9f); // Slightly smaller to avoid clipping
        drawLowPolyArm(g_ArmJoints2);
        glPopMatrix();

        Vec3 rightElbowPos = g_ArmJoints2[3].position;
        glTranslatef(rightElbowPos.x * ARM_SCALE, rightElbowPos.y * ARM_SCALE, rightElbowPos.z * ARM_SCALE);
        glRotatef(defaultElbowBend - 15.0f, 1.0f, 0.0f, 0.0f);
        glTranslatef(-rightElbowPos.x * ARM_SCALE, -rightElbowPos.y * ARM_SCALE, -rightElbowPos.z * ARM_SCALE);

        Vec3 rightWristPos = g_ArmJoints2.back().position;
        glTranslatef(rightWristPos.x * ARM_SCALE, rightWristPos.y * ARM_SCALE, rightWristPos.z * ARM_SCALE);
        glRotatef(defaultWristPitch, 1.0f, 0.0f, 0.0f);
        glRotatef(defaultWristYaw, 0.0f, 1.0f, 0.0f);
        glRotatef(defaultWristRoll, 0.0f, 0.0f, 1.0f);

        glPushMatrix();
        // Connection to arm wrist - move hand higher to link with arm
        glTranslatef(0.0f, 0.05f, -0.10f);  // Position hand higher to connect with arm end
        glScalef(HAND_SCALE * 1.0f, HAND_SCALE * 1.0f, HAND_SCALE * 1.0f); // Full scale for better connection
        if (g_TextureEnabled && g_HandTexture != 0) {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, g_HandTexture);
            glColor3f(1.0f, 1.0f, 1.0f);
        }

        drawSkinnedPalm2(g_HandJoints2);
        drawLowPolyThumb(g_HandJoints2);
        drawLowPolyFinger(g_HandJoints2, 5, 6, 7, 8);
        drawLowPolyFinger(g_HandJoints2, 9, 10, 11, 12);
        drawLowPolyFinger(g_HandJoints2, 13, 14, 15, 16);
        drawLowPolyFinger(g_HandJoints2, 17, 18, 19, 20);
        glPopMatrix();
    }
    glPopMatrix();

    // Disable texture after drawing hands
    glDisable(GL_TEXTURE_2D);
}
// --- Main Character Drawing ---
void drawBodyAndHead(float leftLegAngle, float rightLegAngle, float leftArmAngle, float rightArmAngle) {
    glPushMatrix();
    // Position body to connect seamlessly with scaled leg tops
    // Leg mesh max Y is 8.2f, scaled by LEG_SCALE gives the connection point
    glTranslatef(0.0f, 8.2f * LEG_SCALE, 0.0f); // Connect at top of scaled legs
    glScalef(BODY_SCALE, BODY_SCALE, BODY_SCALE);
    glRotatef(g_pose.torsoYaw, 0, 1, 0); glRotatef(g_pose.torsoPitch, 1, 0, 0); glRotatef(g_pose.torsoRoll, 0, 0, 1);

    drawArmsAndHands(leftArmAngle, rightArmAngle); // Pass the arm angles down
    drawTorso();
    // drawSkirt(leftLegAngle, rightLegAngle); // Skirt removed

    glDisable(GL_LIGHTING); glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glPushMatrix();
    glTranslatef(0, HEAD_CENTER_Y, 0);
    glRotatef(g_pose.headYaw, 0, 1, 0); glRotatef(g_pose.headPitch, 1, 0, 0); glRotatef(g_pose.headRoll, 0, 0, 1);
    glTranslatef(0, -HEAD_CENTER_Y, 0);
    // Face features are now drawn as part of drawMulanHead()
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

    // =========================================================
    // == 📍 FINAL CORRECTED ANIMATION LOGIC ==
    // =========================================================
    float bodyBob = 0.0f;
    float leftThighAngle = 0.0f, rightThighAngle = 0.0f;
    float leftKneeAngle = 0.0f, rightKneeAngle = 0.0f;
    float leftArmSwing = 0.0f, rightArmSwing = 0.0f;

    if (fabsf(gMoveSpeed) > 0.1f) {
        float animSpeed = keyShift ? 1.8f : 1.0f;
        float phase = gWalkPhase * animSpeed;

        // Use sin for forward/back motion. This is the main driver of the cycle.
        float sinPhase = sin(phase);

        // 1. Calculate Body Bob (vertical movement)
        // Body is lowest at contact (phase 0, PI, 2PI) and highest mid-swing.
        bodyBob = (cos(phase * 2.0f) * -0.5f + 0.5f) * -BODY_BOB_AMOUNT;

        // 2. Calculate Thigh and Arm Swing
        float thighSwingAmplitude = keyShift ? RUN_THIGH_SWING : WALK_THIGH_SWING;
        float armSwingAmplitude = keyShift ? RUN_ARM_SWING : WALK_ARM_SWING;

        rightThighAngle = -sinPhase * thighSwingAmplitude;
        leftThighAngle = sinPhase * thighSwingAmplitude;
        rightArmSwing = sinPhase * armSwingAmplitude;
        leftArmSwing = -sinPhase * armSwingAmplitude;

        // 3. --- CORRECTED KNEE LOGIC ---
        // This logic bends the knee most when the thigh is in the middle of its forward swing.

        // Right Leg Knee: Its forward swing is when sinPhase is negative.
        if (rightThighAngle > 0) { // Leg is moving forward
            float bendFactor = 1.0f - fabsf(sinPhase); // 1 at peak swing, 0 at center
            rightKneeAngle = -(1.0f - bendFactor * bendFactor) * SWINGING_LEG_KNEE_BEND;
        }
        else { // Leg is moving backward
            rightKneeAngle = -(fabsf(sinPhase)) * PLANTED_LEG_FLEX_AMOUNT;
        }

        // Left Leg Knee: Its forward swing is when sinPhase is positive.
        if (leftThighAngle > 0) { // Leg is moving forward
            float bendFactor = 1.0f - fabsf(sinPhase);
            leftKneeAngle = -(1.0f - bendFactor * bendFactor) * SWINGING_LEG_KNEE_BEND;
        }
        else { // Leg is moving backward
            leftKneeAngle = -(fabsf(sinPhase)) * PLANTED_LEG_FLEX_AMOUNT;
        }
    }

    glPushMatrix();
    glTranslatef(gCharacterPos.x, gCharacterPos.y + bodyBob, gCharacterPos.z); // Apply body bob
    glRotatef(gCharacterYaw, 0.0f, 1.0f, 0.0f);

    // Draw legs with better proportions
    // Hip width should match the body radius at hip level (R8 * BODY_SCALE)
    // Position legs at natural hip socket locations for proper human proportions
    float hipWidth = R8 * BODY_SCALE * 0.8f; // Slightly wider hip positioning for shorter legs
    
    glPushMatrix(); 
    glTranslatef(-hipWidth, 0.3f, 0.5f); // Translated up to stick to torso bottom
    drawLeg(false, leftThighAngle, leftKneeAngle); 
    glPopMatrix();
    
    glPushMatrix(); 
    glTranslatef(hipWidth, 0.3f, 0.5f); // Translated up to stick to torso bottom
    drawLeg(true, rightThighAngle, rightKneeAngle); 
    glPopMatrix();

    // Pass thigh angles for skirt and arm angles for arms
    drawBodyAndHead(leftThighAngle, rightThighAngle, leftArmSwing, rightArmSwing);

    glPopMatrix();
}
// ===================================================================
//
// SECTION 4: WIN32 APPLICATION FRAMEWORK
//
// ===================================================================

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nCmdShow) {
    WNDCLASSEXA wc{}; wc.cbSize = sizeof(WNDCLASSEXA); wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC; wc.lpfnWndProc = WindowProcedure; wc.hInstance = GetModuleHandle(NULL); wc.lpszClassName = WINDOW_TITLE;
    if (!RegisterClassExA(&wc))return 0;
    HWND hWnd = CreateWindowA(WINDOW_TITLE, WINDOW_TITLE, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, gWidth, gHeight, NULL, NULL, wc.hInstance, NULL);
    if (!hWnd)return 0;
    HDC hdc = GetDC(hWnd);
    { PIXELFORMATDESCRIPTOR pfd{}; pfd.nSize = sizeof(pfd); pfd.nVersion = 1; pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER; pfd.iPixelType = PFD_TYPE_RGBA; pfd.cColorBits = 32; pfd.cDepthBits = 24; pfd.iLayerType = PFD_MAIN_PLANE; int pf = ChoosePixelFormat(hdc, &pfd); SetPixelFormat(hdc, pf, &pfd); }
    HGLRC rc = wglCreateContext(hdc); wglMakeCurrent(hdc, rc);
    ShowWindow(hWnd, nCmdShow); UpdateWindow(hWnd);

    glEnable(GL_DEPTH_TEST); glEnable(GL_NORMALIZE);
    initializeCharacterParts();
    QueryPerformanceFrequency(&gFreq); QueryPerformanceCounter(&gPrev);

    MSG msg{};
    while (gRunning) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) { if (msg.message == WM_QUIT)gRunning = false; TranslateMessage(&msg); DispatchMessage(&msg); }
        LARGE_INTEGER now; QueryPerformanceCounter(&now); float dt = float(now.QuadPart - gPrev.QuadPart) / float(gFreq.QuadPart); gPrev = now;
        if (dt > 0.1f) dt = 0.1f;

        updateCharacter(dt);

        { Vec3 eye; { eye.x = gTarget.x + gDist * cos(gPitch) * sin(gYaw); eye.y = gTarget.y + gDist * sin(gPitch); eye.z = gTarget.z + gDist * cos(gPitch) * cos(gYaw); } Vec3 f = { gTarget.x - eye.x,gTarget.y - eye.y,gTarget.z - eye.z }; float fl = sqrt(f.x * f.x + f.y * f.y + f.z * f.z); if (fl > 1e-6) { f.x /= fl; f.y /= fl; f.z /= fl; } Vec3 r = { f.z,0,-f.x }; float moveStep = gDist * 0.8f * dt; if (keyW)gTarget.y += moveStep; if (keyS)gTarget.y -= moveStep; if (keyA) { gTarget.x -= r.x * moveStep; gTarget.z -= r.z * moveStep; }if (keyD) { gTarget.x += r.x * moveStep; gTarget.z += r.z * moveStep; } }
        display(); SwapBuffers(hdc);
    }

    wglMakeCurrent(NULL, NULL); wglDeleteContext(rc); ReleaseDC(hWnd, hdc); UnregisterClassA(WINDOW_TITLE, wc.hInstance);
    return 0;
}

void updateCharacter(float dt) {
    float targetSpeed = 0.0f;
    if (keyUp) { targetSpeed = keyShift ? RUN_SPEED : WALK_SPEED; }
    if (keydown) { targetSpeed = -(keyShift ? RUN_SPEED : WALK_SPEED); }
    gMoveSpeed = targetSpeed;
    if (keyLeft) { gCharacterYaw += TURN_SPEED * dt; }
    if (keyRight) { gCharacterYaw -= TURN_SPEED * dt; }
    if (fabsf(gMoveSpeed) > 0.01f) {
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
    case WM_MOUSEWHEEL: { short delta = GET_WHEEL_DELTA_WPARAM(wParam); gDist *= (1.0f - (delta / 120.0f) * 0.1f); if (gDist < 2.0f)gDist = 2.0f; if (gDist > 100.0f)gDist = 100.0f; } return 0;
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)PostQuitMessage(0);
        else if (wParam == 'R') { gYaw = 0.2f; gPitch = 0.1f; gDist = 15.0f; gTarget = { 0.0f,3.5f,0.0f }; gCharacterPos = { 0,0,0 }; gCharacterYaw = 0; }
        else if (wParam == '1') { gShowWireframe = !gShowWireframe; }
        else if (wParam == '2') { g_TextureEnabled = !g_TextureEnabled; } // Toggle hand texture
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

void drawCurvedBand(float rA, float yA, float rB, float yB) {
	for (int i = 0; i < 16; ++i) {
		float cA = segCos[i];
		float sA = segSin[i];
		float cB = segCos[(i + 1) % 16];
		float sB = segSin[(i + 1) % 16];

		// Base vertices
		Vec3f v1A = { rA * cA, yA, rA * sA };
		Vec3f v2A = { rB * cA, yB, rB * sA };
		Vec3f v1B = { rA * cB, yA, rA * sB };
		Vec3f v2B = { rB * cB, yB, rB * sB };

		// Apply curvature
		if (sA < 0) { // Back
			v1A.z *= 0.5f;
			v2A.z *= 0.5f;
		}
		if (sB < 0) { // Back
			v1B.z *= 0.5f;
			v2B.z *= 0.5f;
		}

		Vec3f nA = normalize(cross(sub(v2A, v1A), sub(v1B, v1A)));
		Vec3f nB = normalize(cross(sub(v2B, v1B), sub(v1A, v1B)));

		glNormal3f(nA.x, nA.y, nA.z);
		glVertex3f(v1A.x, v1A.y, v1A.z);
		glVertex3f(v2A.x, v2A.y, v2A.z);
		glVertex3f(v2B.x, v2B.y, v2B.z);

		glNormal3f(nB.x, nB.y, nB.z);
		glVertex3f(v1A.x, v1A.y, v1A.z);
		glVertex3f(v2B.x, v2B.y, v2B.z);
		glVertex3f(v1B.x, v1B.y, v1B.z);
	}
}
