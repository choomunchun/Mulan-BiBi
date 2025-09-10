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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, BMP.bmWidth, BMP.bmHeight, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, BMP.bmBits);
    
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
            
            textureData[index] = (unsigned char)(220 + variation * 35);     // R
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
void drawShoulderSockets();
void drawCubeShoulder(float x, float y, float z, float width, float height, float depth);
void drawSphereShoulder(float x, float y, float z, float radius);

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
    
    // Based on the hand diagram with 21 joints (0-20)
    // Starting from a cube approach - step 1: basic proportions
    
    // WRIST (0) - base of the cube
    g_HandJoints[0] = {{0.0f, 0.0f, 0.0f}, -1};
    
    // Step 2: Make it narrower and position for palm geometry
    // THUMB chain (1-4) - positioned on the diagonal face (thumb base)
    g_HandJoints[1] = {{0.6f, -0.1f, 0.4f}, 0};   // THUMB_CMC - moved to side for thumb base
    g_HandJoints[2] = {{0.9f, 0.0f, 0.7f}, 1};    // THUMB_MCP
    g_HandJoints[3] = {{1.1f, 0.1f, 0.9f}, 2};    // THUMB_IP
    g_HandJoints[4] = {{1.2f, 0.2f, 1.1f}, 3};    // THUMB_TIP
    
    // Step 3: Add loop cuts - fingers positioned in upper part
    // First loop cut area - knuckles
    // INDEX FINGER chain (5-8) - upper part positioning
    g_HandJoints[5] = {{0.3f, 0.1f, 1.0f}, 0};    // INDEX_FINGER_MCP
    g_HandJoints[6] = {{0.3f, 0.0f, 1.6f}, 5};    // INDEX_FINGER_PIP
    g_HandJoints[7] = {{0.3f, 0.0f, 2.0f}, 6};    // INDEX_FINGER_DIP
    g_HandJoints[8] = {{0.3f, 0.0f, 2.3f}, 7};    // INDEX_FINGER_TIP
    
    // MIDDLE FINGER chain (9-12) - longest finger
    g_HandJoints[9] = {{0.0f, 0.1f, 1.1f}, 0};    // MIDDLE_FINGER_MCP
    g_HandJoints[10] = {{0.0f, 0.0f, 1.8f}, 9};   // MIDDLE_FINGER_PIP
    g_HandJoints[11] = {{0.0f, 0.0f, 2.3f}, 10};  // MIDDLE_FINGER_DIP
    g_HandJoints[12] = {{0.0f, 0.0f, 2.6f}, 11};  // MIDDLE_FINGER_TIP
    
    // RING FINGER chain (13-16)
    g_HandJoints[13] = {{-0.3f, 0.1f, 1.0f}, 0};  // RING_FINGER_MCP
    g_HandJoints[14] = {{-0.3f, 0.0f, 1.7f}, 13}; // RING_FINGER_PIP
    g_HandJoints[15] = {{-0.3f, 0.0f, 2.1f}, 14}; // RING_FINGER_DIP
    g_HandJoints[16] = {{-0.3f, 0.0f, 2.4f}, 15}; // RING_FINGER_TIP
    
    // PINKY chain (17-20) - shortest finger
    g_HandJoints[17] = {{-0.5f, 0.05f, 0.8f}, 0};  // PINKY_MCP - lower position
    g_HandJoints[18] = {{-0.5f, 0.0f, 1.3f}, 17};  // PINKY_PIP
    g_HandJoints[19] = {{-0.5f, 0.0f, 1.6f}, 18};  // PINKY_DIP
    g_HandJoints[20] = {{-0.5f, 0.0f, 1.8f}, 19};  // PINKY_TIP
}

// Initialize second hand (mirrored and positioned to the right)
static void InitializeHand2() {
    g_HandJoints2.clear();
    g_HandJoints2.resize(21);
    
    // Offset the second hand to the right and flip it horizontally
    float xOffset = 0.0f; // No offset since we'll position it properly in the body
    float flipSign = -1.0f; // Flip X coordinates to mirror the hand
    
    // WRIST (0) - base of the cube
    g_HandJoints2[0] = {{xOffset + 0.0f * flipSign, 0.0f, 0.0f}, -1};
    
    // THUMB chain (1-4) - mirrored to opposite side
    g_HandJoints2[1] = {{xOffset + 0.6f * flipSign, -0.1f, 0.4f}, 0};   // THUMB_CMC
    g_HandJoints2[2] = {{xOffset + 0.9f * flipSign, 0.0f, 0.7f}, 1};    // THUMB_MCP
    g_HandJoints2[3] = {{xOffset + 1.1f * flipSign, 0.1f, 0.9f}, 2};    // THUMB_IP
    g_HandJoints2[4] = {{xOffset + 1.2f * flipSign, 0.2f, 1.1f}, 3};    // THUMB_TIP
    
    // INDEX FINGER chain (5-8) - mirrored
    g_HandJoints2[5] = {{xOffset + 0.3f * flipSign, 0.1f, 1.0f}, 0};    // INDEX_FINGER_MCP
    g_HandJoints2[6] = {{xOffset + 0.3f * flipSign, 0.0f, 1.6f}, 5};    // INDEX_FINGER_PIP
    g_HandJoints2[7] = {{xOffset + 0.3f * flipSign, 0.0f, 2.0f}, 6};    // INDEX_FINGER_DIP
    g_HandJoints2[8] = {{xOffset + 0.3f * flipSign, 0.0f, 2.3f}, 7};    // INDEX_FINGER_TIP
    
    // MIDDLE FINGER chain (9-12) - mirrored
    g_HandJoints2[9] = {{xOffset + 0.0f * flipSign, 0.1f, 1.1f}, 0};    // MIDDLE_FINGER_MCP
    g_HandJoints2[10] = {{xOffset + 0.0f * flipSign, 0.0f, 1.8f}, 9};   // MIDDLE_FINGER_PIP
    g_HandJoints2[11] = {{xOffset + 0.0f * flipSign, 0.0f, 2.3f}, 10};  // MIDDLE_FINGER_DIP
    g_HandJoints2[12] = {{xOffset + 0.0f * flipSign, 0.0f, 2.6f}, 11};  // MIDDLE_FINGER_TIP
    
    // RING FINGER chain (13-16) - mirrored
    g_HandJoints2[13] = {{xOffset + -0.3f * flipSign, 0.1f, 1.0f}, 0};  // RING_FINGER_MCP
    g_HandJoints2[14] = {{xOffset + -0.3f * flipSign, 0.0f, 1.7f}, 13}; // RING_FINGER_PIP
    g_HandJoints2[15] = {{xOffset + -0.3f * flipSign, 0.0f, 2.1f}, 14}; // RING_FINGER_DIP
    g_HandJoints2[16] = {{xOffset + -0.3f * flipSign, 0.0f, 2.4f}, 15}; // RING_FINGER_TIP
    
    // PINKY chain (17-20) - mirrored
    g_HandJoints2[17] = {{xOffset + -0.5f * flipSign, 0.05f, 0.8f}, 0};  // PINKY_MCP
    g_HandJoints2[18] = {{xOffset + -0.5f * flipSign, 0.0f, 1.3f}, 17};  // PINKY_PIP
    g_HandJoints2[19] = {{xOffset + -0.5f * flipSign, 0.0f, 1.6f}, 18};  // PINKY_DIP
    g_HandJoints2[20] = {{xOffset + -0.5f * flipSign, 0.0f, 1.8f}, 19};  // PINKY_TIP
}

// ====================== Arm Creation ======================
static void InitializeArm() {
    g_ArmJoints.clear();
    g_ArmJoints.resize(6);  // 6 joints to match anatomy structure
    
    // ARM_WRIST (0) - attachment point to hand wrist
    g_ArmJoints[0] = {{0.0f, 0.0f, 0.1f}, -1};    // Connection point
    
    // ARM_LOWER_FOREARM (1)
    g_ArmJoints[1] = {{0.15f, 0.08f, 2.0f}, 0};   
    
    // ARM_MID_FOREARM (2) - mid-section of forearm
    g_ArmJoints[2] = {{0.25f, 0.12f, 4.2f}, 1};   
    
    // ARM_ELBOW (3) - elbow transition point
    g_ArmJoints[3] = {{0.30f, 0.15f, 6.8f}, 2};   
    
    // ARM_UPPER_ARM (4)
    g_ArmJoints[4] = {{0.35f, 0.20f, 9.5f}, 3};   
    
    // ARM_SHOULDER (5) - shoulder connection
    g_ArmJoints[5] = {{0.28f, 0.35f, 12.0f}, 4};  
}

// Initialize second arm (mirrored)
static void InitializeArm2() {
    g_ArmJoints2.clear();
    g_ArmJoints2.resize(6);  // 6 joints to match anatomy structure
    
    // Mirror the arm to match the second hand position
    float xOffset = 0.0f; // No offset since we'll position it properly in the body
    float flipSign = -1.0f; // Flip X coordinates to mirror the arm
    
    // ARM_WRIST (0) - attachment point to second hand wrist
    g_ArmJoints2[0] = {{xOffset + 0.0f * flipSign, 0.0f, 0.1f}, -1};    // Connection point
    
    // ARM_LOWER_FOREARM (1) - mirrored
    g_ArmJoints2[1] = {{xOffset + 0.15f * flipSign, 0.08f, 2.0f}, 0};   
    
    // ARM_MID_FOREARM (2) - mirrored
    g_ArmJoints2[2] = {{xOffset + 0.25f * flipSign, 0.12f, 4.2f}, 1};   
    
    // ARM_ELBOW (3) - mirrored
    g_ArmJoints2[3] = {{xOffset + 0.30f * flipSign, 0.15f, 6.8f}, 2};   
    
    // ARM_UPPER_ARM (4) - mirrored
    g_ArmJoints2[4] = {{xOffset + 0.35f * flipSign, 0.20f, 9.5f}, 3};   
    
    // ARM_SHOULDER (5) - mirrored
    g_ArmJoints2[5] = {{xOffset + 0.28f * flipSign, 0.35f, 12.0f}, 4};  
}
void initializeCharacterParts() {
    buildTriangles();
    computeVertexNormals();
    InitializeHand();
    InitializeHand2();
    InitializeArm();
    InitializeArm2();
    
    // Initialize hand texture
    g_HandTexture = loadTexture("skin.bmp");
    if (g_HandTexture == 0) {
        // If texture file not found, create procedural skin texture
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
            
            glNormal3f(x0/radius, y0/radius, z0/radius);
            glVertex3f(x0, y0, z0);
            glNormal3f(x1/radius, y1/radius, z1/radius);
            glVertex3f(x1, y1, z1);
        }
        glEnd();
    }
    
    glPopMatrix();
}

// Draw anatomically complex arm segment with texture
static void drawAnatomicalArmSegment(const Vec3& start, const Vec3& end, float baseWidth, float baseHeight, 
                                   float muscleWidth, float boneWidth) {
    Vec3 dir = {end.x - start.x, end.y - start.y, end.z - start.z};
    float length = sqrtf(dir.x*dir.x + dir.y*dir.y + dir.z*dir.z);
    
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
    Vec3 dir = {end.x - start.x, end.y - start.y, end.z - start.z};
    float length = sqrtf(dir.x*dir.x + dir.y*dir.y + dir.z*dir.z);
    
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
        Vec3 normal1 = {cosf(angle1), sinf(angle1), 0};
        Vec3 normal2 = {cosf(angle2), sinf(angle2), 0};
        
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
        float lat0 = PI * (0.0f + (float)i / stacks / 2.0f);     // 0 to PI/2 (top half only)
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
            Vec3 n00 = {x00/radius, y0/radius, z00/radius};
            Vec3 n01 = {x01/radius, y0/radius, z01/radius};
            Vec3 n10 = {x10/radius, y1/radius, z10/radius};
            Vec3 n11 = {x11/radius, y1/radius, z11/radius};
            
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
    } else {
        glDisable(GL_TEXTURE_2D);
        glColor3f(0.85f, 0.64f, 0.52f); // Skin color for non-textured mode
    }
    
    // Draw arm segments scaled to match palm size
    Vec3 shoulderSocket = {0.0f, 0.0f, 0.0f};  // Start from shoulder socket
    Vec3 wristConnect = {0.0f, 0.0f, 0.05f};  // Connection to hand
    
    // Add shoulder connector piece
    drawAnatomicalArmSegment(shoulderSocket, {0.0f, -0.1f, 0.0f}, 1.2f, 1.0f, 1.0f, 0.8f);
    
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
    palmGrid[0][0] = {wrist.x - 0.6f, wrist.y - 0.02f, wrist.z - 0.15f};   // Far pinky edge
    palmGrid[0][1] = {wrist.x - 0.42f, wrist.y - 0.01f, wrist.z - 0.13f};  // Pinky-ring gap
    palmGrid[0][2] = {wrist.x - 0.26f, wrist.y + 0.0f, wrist.z - 0.11f};   // Ring area
    palmGrid[0][3] = {wrist.x - 0.12f, wrist.y + 0.01f, wrist.z - 0.1f};   // Left-center
    palmGrid[0][4] = {wrist.x + 0.0f, wrist.y + 0.02f, wrist.z - 0.09f};   // True center
    palmGrid[0][5] = {wrist.x + 0.12f, wrist.y + 0.01f, wrist.z - 0.1f};   // Right-center
    palmGrid[0][6] = {wrist.x + 0.26f, wrist.y + 0.0f, wrist.z - 0.11f};   // Index area
    palmGrid[0][7] = {wrist.x + 0.42f, wrist.y - 0.01f, wrist.z - 0.13f};  // Index-thumb gap
    palmGrid[0][8] = {wrist.x + 0.6f, wrist.y - 0.02f, wrist.z - 0.15f};   // Far thumb edge
    
    // Layer 1: Lower palm transition
    palmGrid[1][0] = {wrist.x - 0.52f, wrist.y + 0.08f, wrist.z + 0.05f};
    palmGrid[1][1] = {wrist.x - 0.38f, wrist.y + 0.09f, wrist.z + 0.08f};
    palmGrid[1][2] = {wrist.x - 0.24f, wrist.y + 0.1f, wrist.z + 0.1f};
    palmGrid[1][3] = {wrist.x - 0.1f, wrist.y + 0.11f, wrist.z + 0.12f};
    palmGrid[1][4] = {wrist.x + 0.0f, wrist.y + 0.12f, wrist.z + 0.13f};
    palmGrid[1][5] = {wrist.x + 0.1f, wrist.y + 0.11f, wrist.z + 0.12f};
    palmGrid[1][6] = {wrist.x + 0.24f, wrist.y + 0.1f, wrist.z + 0.1f};
    palmGrid[1][7] = {wrist.x + 0.38f, wrist.y + 0.09f, wrist.z + 0.08f};
    palmGrid[1][8] = {wrist.x + 0.52f, wrist.y + 0.08f, wrist.z + 0.05f};
    
    // Layer 2: Lower-mid palm
    palmGrid[2][0] = {wrist.x - 0.48f, wrist.y + 0.16f, wrist.z + 0.2f};
    palmGrid[2][1] = {wrist.x - 0.35f, wrist.y + 0.17f, wrist.z + 0.22f};
    palmGrid[2][2] = {wrist.x - 0.22f, wrist.y + 0.18f, wrist.z + 0.24f};
    palmGrid[2][3] = {wrist.x - 0.08f, wrist.y + 0.19f, wrist.z + 0.25f};
    palmGrid[2][4] = {wrist.x + 0.0f, wrist.y + 0.2f, wrist.z + 0.26f};
    palmGrid[2][5] = {wrist.x + 0.08f, wrist.y + 0.19f, wrist.z + 0.25f};
    palmGrid[2][6] = {wrist.x + 0.22f, wrist.y + 0.18f, wrist.z + 0.24f};
    palmGrid[2][7] = {wrist.x + 0.35f, wrist.y + 0.17f, wrist.z + 0.22f};
    palmGrid[2][8] = {wrist.x + 0.48f, wrist.y + 0.16f, wrist.z + 0.2f};
    
    // Layer 3: True mid palm
    palmGrid[3][0] = {wrist.x - 0.44f, wrist.y + 0.25f, wrist.z + 0.35f};
    palmGrid[3][1] = {wrist.x - 0.32f, wrist.y + 0.26f, wrist.z + 0.37f};
    palmGrid[3][2] = {wrist.x - 0.2f, wrist.y + 0.27f, wrist.z + 0.38f};
    palmGrid[3][3] = {wrist.x - 0.06f, wrist.y + 0.28f, wrist.z + 0.39f};
    palmGrid[3][4] = {wrist.x + 0.0f, wrist.y + 0.29f, wrist.z + 0.4f};
    palmGrid[3][5] = {wrist.x + 0.06f, wrist.y + 0.28f, wrist.z + 0.39f};
    palmGrid[3][6] = {wrist.x + 0.2f, wrist.y + 0.27f, wrist.z + 0.38f};
    palmGrid[3][7] = {wrist.x + 0.32f, wrist.y + 0.26f, wrist.z + 0.37f};
    palmGrid[3][8] = {wrist.x + 0.44f, wrist.y + 0.25f, wrist.z + 0.35f};
    
    // Layer 4: Upper palm (approaching knuckles)
    palmGrid[4][0] = {wrist.x - 0.4f, wrist.y + 0.35f, wrist.z + 0.5f};
    palmGrid[4][1] = {wrist.x - 0.28f, wrist.y + 0.36f, wrist.z + 0.52f};
    palmGrid[4][2] = {wrist.x - 0.16f, wrist.y + 0.37f, wrist.z + 0.53f};
    palmGrid[4][3] = {wrist.x - 0.04f, wrist.y + 0.38f, wrist.z + 0.54f};
    palmGrid[4][4] = {wrist.x + 0.0f, wrist.y + 0.39f, wrist.z + 0.55f};
    palmGrid[4][5] = {wrist.x + 0.04f, wrist.y + 0.38f, wrist.z + 0.54f};
    palmGrid[4][6] = {wrist.x + 0.16f, wrist.y + 0.37f, wrist.z + 0.53f};
    palmGrid[4][7] = {wrist.x + 0.28f, wrist.y + 0.36f, wrist.z + 0.52f};
    palmGrid[4][8] = {wrist.x + 0.4f, wrist.y + 0.35f, wrist.z + 0.5f};
    
    // Layer 5: Pre-knuckle area (precision positioning)
    palmGrid[5][0] = {pinkyBase.x - 0.02f, pinkyBase.y + 0.02f, pinkyBase.z - 0.3f};
    palmGrid[5][1] = {(pinkyBase.x + ringBase.x) * 0.5f - 0.015f, (pinkyBase.y + ringBase.y) * 0.5f + 0.04f, (pinkyBase.z + ringBase.z) * 0.5f - 0.3f};
    palmGrid[5][2] = {ringBase.x - 0.015f, ringBase.y + 0.06f, ringBase.z - 0.3f};
    palmGrid[5][3] = {(ringBase.x + middleBase.x) * 0.5f - 0.01f, (ringBase.y + middleBase.y) * 0.5f + 0.075f, (ringBase.z + middleBase.z) * 0.5f - 0.3f};
    palmGrid[5][4] = {middleBase.x - 0.01f, middleBase.y + 0.09f, middleBase.z - 0.3f};
    palmGrid[5][5] = {(middleBase.x + indexBase.x) * 0.5f - 0.01f, (middleBase.y + indexBase.y) * 0.5f + 0.075f, (middleBase.z + indexBase.z) * 0.5f - 0.3f};
    palmGrid[5][6] = {indexBase.x - 0.015f, indexBase.y + 0.06f, indexBase.z - 0.3f};
    palmGrid[5][7] = {(indexBase.x + thumbBase.x) * 0.7f - 0.02f, (indexBase.y + thumbBase.y) * 0.7f + 0.03f, (indexBase.z + thumbBase.z) * 0.7f - 0.25f};
    palmGrid[5][8] = {thumbBase.x - 0.05f, thumbBase.y + 0.01f, thumbBase.z - 0.2f};
    
    // Layer 6: Direct finger connections (seamless transitions)
    palmGrid[6][0] = {pinkyBase.x, pinkyBase.y - 0.05f, pinkyBase.z - 0.08f};
    palmGrid[6][1] = {(pinkyBase.x + ringBase.x) * 0.5f, (pinkyBase.y + ringBase.y) * 0.5f - 0.06f, (pinkyBase.z + ringBase.z) * 0.5f - 0.08f};
    palmGrid[6][2] = {ringBase.x, ringBase.y - 0.06f, ringBase.z - 0.08f};
    palmGrid[6][3] = {(ringBase.x + middleBase.x) * 0.5f, (ringBase.y + middleBase.y) * 0.5f - 0.07f, (ringBase.z + middleBase.z) * 0.5f - 0.08f};
    palmGrid[6][4] = {middleBase.x, middleBase.y - 0.08f, middleBase.z - 0.08f};
    palmGrid[6][5] = {(middleBase.x + indexBase.x) * 0.5f, (middleBase.y + indexBase.y) * 0.5f - 0.07f, (middleBase.z + indexBase.z) * 0.5f - 0.08f};
    palmGrid[6][6] = {indexBase.x, indexBase.y - 0.06f, indexBase.z - 0.08f};
    palmGrid[6][7] = {(indexBase.x + thumbBase.x) * 0.6f, (indexBase.y + thumbBase.y) * 0.6f - 0.05f, (indexBase.z + thumbBase.z) * 0.6f - 0.06f};
    palmGrid[6][8] = {thumbBase.x, thumbBase.y - 0.04f, thumbBase.z - 0.05f};
    
    // Create back surface vertices (mirrored below palm)
    Vec3 backGrid[GRID_HEIGHT][GRID_WIDTH];
    for (int row = 0; row < GRID_HEIGHT; row++) {
        for (int col = 0; col < GRID_WIDTH; col++) {
            backGrid[row][col] = {palmGrid[row][col].x, palmGrid[row][col].y - 0.35f, palmGrid[row][col].z};
        }
    }
    
    // ULTRA-DENSE MESH RENDERING - Complete coverage with no gaps
    glBegin(GL_TRIANGLES);
    
    // PALM SURFACE (top) - Complete quad grid tessellation
    Vec3 normalUp = {0, 1, 0.15f};
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
    Vec3 normalDown = {0, -1, -0.15f};
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
        Vec3 edge1 = {palmGrid[row + 1][0].x - palmGrid[row][0].x, palmGrid[row + 1][0].y - palmGrid[row][0].y, palmGrid[row + 1][0].z - palmGrid[row][0].z};
        Vec3 edge2 = {backGrid[row][0].x - palmGrid[row][0].x, backGrid[row][0].y - palmGrid[row][0].y, backGrid[row][0].z - palmGrid[row][0].z};
        Vec3 normal = {edge1.y * edge2.z - edge1.z * edge2.y, edge1.z * edge2.x - edge1.x * edge2.z, edge1.x * edge2.y - edge1.y * edge2.x};
        float len = sqrtf(normal.x*normal.x + normal.y*normal.y + normal.z*normal.z);
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
        Vec3 edge1 = {palmGrid[row + 1][GRID_WIDTH - 1].x - palmGrid[row][GRID_WIDTH - 1].x, palmGrid[row + 1][GRID_WIDTH - 1].y - palmGrid[row][GRID_WIDTH - 1].y, palmGrid[row + 1][GRID_WIDTH - 1].z - palmGrid[row][GRID_WIDTH - 1].z};
        Vec3 edge2 = {backGrid[row][GRID_WIDTH - 1].x - palmGrid[row][GRID_WIDTH - 1].x, backGrid[row][GRID_WIDTH - 1].y - palmGrid[row][GRID_WIDTH - 1].y, backGrid[row][GRID_WIDTH - 1].z - palmGrid[row][GRID_WIDTH - 1].z};
        Vec3 normal = {edge2.y * edge1.z - edge2.z * edge1.y, edge2.z * edge1.x - edge2.x * edge1.z, edge2.x * edge1.y - edge2.y * edge1.x};
        float len = sqrtf(normal.x*normal.x + normal.y*normal.y + normal.z*normal.z);
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
        Vec3 edge1 = {palmGrid[0][col + 1].x - palmGrid[0][col].x, palmGrid[0][col + 1].y - palmGrid[0][col].y, palmGrid[0][col + 1].z - palmGrid[0][col].z};
        Vec3 edge2 = {backGrid[0][col].x - palmGrid[0][col].x, backGrid[0][col].y - palmGrid[0][col].y, backGrid[0][col].z - palmGrid[0][col].z};
        Vec3 normal = {edge2.y * edge1.z - edge2.z * edge1.y, edge2.z * edge1.x - edge2.x * edge1.z, edge2.x * edge1.y - edge2.y * edge1.x};
        float len = sqrtf(normal.x*normal.x + normal.y*normal.y + normal.z*normal.z);
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
        Vec3 edge1 = {palmGrid[GRID_HEIGHT - 1][col + 1].x - palmGrid[GRID_HEIGHT - 1][col].x, palmGrid[GRID_HEIGHT - 1][col + 1].y - palmGrid[GRID_HEIGHT - 1][col].y, palmGrid[GRID_HEIGHT - 1][col + 1].z - palmGrid[GRID_HEIGHT - 1][col].z};
        Vec3 edge2 = {backGrid[GRID_HEIGHT - 1][col].x - palmGrid[GRID_HEIGHT - 1][col].x, backGrid[GRID_HEIGHT - 1][col].y - palmGrid[GRID_HEIGHT - 1][col].y, backGrid[GRID_HEIGHT - 1][col].z - palmGrid[GRID_HEIGHT - 1][col].z};
        Vec3 normal = {edge1.y * edge2.z - edge1.z * edge2.y, edge1.z * edge2.x - edge1.x * edge2.z, edge1.x * edge2.y - edge1.y * edge2.x};
        float len = sqrtf(normal.x*normal.x + normal.y*normal.y + normal.z*normal.z);
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
        Vec3 edge1 = {topVerts[nextI]->x - topVerts[i]->x, topVerts[nextI]->y - topVerts[i]->y, topVerts[nextI]->z - topVerts[i]->z};
        Vec3 edge2 = {bottomVerts[i]->x - topVerts[i]->x, bottomVerts[i]->y - topVerts[i]->y, bottomVerts[i]->z - topVerts[i]->z};
        Vec3 normal = {edge1.y * edge2.z - edge1.z * edge2.y, edge1.z * edge2.x - edge1.x * edge2.z, edge1.x * edge2.y - edge1.y * edge2.x};
        float len = sqrtf(normal.x*normal.x + normal.y*normal.y + normal.z*normal.z);
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
    palmGrid[0][0] = {wrist.x - 0.6f, wrist.y - 0.02f, wrist.z - 0.15f};
    palmGrid[0][1] = {wrist.x - 0.42f, wrist.y - 0.01f, wrist.z - 0.13f};
    palmGrid[0][2] = {wrist.x - 0.26f, wrist.y + 0.0f, wrist.z - 0.11f};
    palmGrid[0][3] = {wrist.x - 0.12f, wrist.y + 0.01f, wrist.z - 0.1f};
    palmGrid[0][4] = {wrist.x + 0.0f, wrist.y + 0.02f, wrist.z - 0.09f};
    palmGrid[0][5] = {wrist.x + 0.12f, wrist.y + 0.01f, wrist.z - 0.1f};
    palmGrid[0][6] = {wrist.x + 0.26f, wrist.y + 0.0f, wrist.z - 0.11f};
    palmGrid[0][7] = {wrist.x + 0.42f, wrist.y - 0.01f, wrist.z - 0.13f};
    palmGrid[0][8] = {wrist.x + 0.6f, wrist.y - 0.02f, wrist.z - 0.15f};
    
    // Layer 1: Lower palm transition
    palmGrid[1][0] = {wrist.x - 0.52f, wrist.y + 0.08f, wrist.z + 0.05f};
    palmGrid[1][1] = {wrist.x - 0.38f, wrist.y + 0.09f, wrist.z + 0.08f};
    palmGrid[1][2] = {wrist.x - 0.24f, wrist.y + 0.1f, wrist.z + 0.1f};
    palmGrid[1][3] = {wrist.x - 0.1f, wrist.y + 0.11f, wrist.z + 0.12f};
    palmGrid[1][4] = {wrist.x + 0.0f, wrist.y + 0.12f, wrist.z + 0.13f};
    palmGrid[1][5] = {wrist.x + 0.1f, wrist.y + 0.11f, wrist.z + 0.12f};
    palmGrid[1][6] = {wrist.x + 0.24f, wrist.y + 0.1f, wrist.z + 0.1f};
    palmGrid[1][7] = {wrist.x + 0.38f, wrist.y + 0.09f, wrist.z + 0.08f};
    palmGrid[1][8] = {wrist.x + 0.52f, wrist.y + 0.08f, wrist.z + 0.05f};
    
    // Layer 2: Lower-mid palm
    palmGrid[2][0] = {wrist.x - 0.48f, wrist.y + 0.16f, wrist.z + 0.2f};
    palmGrid[2][1] = {wrist.x - 0.35f, wrist.y + 0.17f, wrist.z + 0.22f};
    palmGrid[2][2] = {wrist.x - 0.22f, wrist.y + 0.18f, wrist.z + 0.24f};
    palmGrid[2][3] = {wrist.x - 0.08f, wrist.y + 0.19f, wrist.z + 0.25f};
    palmGrid[2][4] = {wrist.x + 0.0f, wrist.y + 0.2f, wrist.z + 0.26f};
    palmGrid[2][5] = {wrist.x + 0.08f, wrist.y + 0.19f, wrist.z + 0.25f};
    palmGrid[2][6] = {wrist.x + 0.22f, wrist.y + 0.18f, wrist.z + 0.24f};
    palmGrid[2][7] = {wrist.x + 0.35f, wrist.y + 0.17f, wrist.z + 0.22f};
    palmGrid[2][8] = {wrist.x + 0.48f, wrist.y + 0.16f, wrist.z + 0.2f};
    
    // Layer 3: True mid palm
    palmGrid[3][0] = {wrist.x - 0.44f, wrist.y + 0.25f, wrist.z + 0.35f};
    palmGrid[3][1] = {wrist.x - 0.32f, wrist.y + 0.26f, wrist.z + 0.37f};
    palmGrid[3][2] = {wrist.x - 0.2f, wrist.y + 0.27f, wrist.z + 0.38f};
    palmGrid[3][3] = {wrist.x - 0.06f, wrist.y + 0.28f, wrist.z + 0.39f};
    palmGrid[3][4] = {wrist.x + 0.0f, wrist.y + 0.29f, wrist.z + 0.4f};
    palmGrid[3][5] = {wrist.x + 0.06f, wrist.y + 0.28f, wrist.z + 0.39f};
    palmGrid[3][6] = {wrist.x + 0.2f, wrist.y + 0.27f, wrist.z + 0.38f};
    palmGrid[3][7] = {wrist.x + 0.32f, wrist.y + 0.26f, wrist.z + 0.37f};
    palmGrid[3][8] = {wrist.x + 0.44f, wrist.y + 0.25f, wrist.z + 0.35f};
    
    // Layer 4: Upper palm (approaching knuckles)
    palmGrid[4][0] = {wrist.x - 0.4f, wrist.y + 0.35f, wrist.z + 0.5f};
    palmGrid[4][1] = {wrist.x - 0.28f, wrist.y + 0.36f, wrist.z + 0.52f};
    palmGrid[4][2] = {wrist.x - 0.16f, wrist.y + 0.37f, wrist.z + 0.53f};
    palmGrid[4][3] = {wrist.x - 0.04f, wrist.y + 0.38f, wrist.z + 0.54f};
    palmGrid[4][4] = {wrist.x + 0.0f, wrist.y + 0.39f, wrist.z + 0.55f};
    palmGrid[4][5] = {wrist.x + 0.04f, wrist.y + 0.38f, wrist.z + 0.54f};
    palmGrid[4][6] = {wrist.x + 0.16f, wrist.y + 0.37f, wrist.z + 0.53f};
    palmGrid[4][7] = {wrist.x + 0.28f, wrist.y + 0.36f, wrist.z + 0.52f};
    palmGrid[4][8] = {wrist.x + 0.4f, wrist.y + 0.35f, wrist.z + 0.5f};
    
    // Layer 5: Pre-knuckle area (precision positioning)
    palmGrid[5][0] = {pinkyBase.x - 0.02f, pinkyBase.y + 0.02f, pinkyBase.z - 0.3f};
    palmGrid[5][1] = {(pinkyBase.x + ringBase.x) * 0.5f - 0.015f, (pinkyBase.y + ringBase.y) * 0.5f + 0.04f, (pinkyBase.z + ringBase.z) * 0.5f - 0.3f};
    palmGrid[5][2] = {ringBase.x - 0.015f, ringBase.y + 0.06f, ringBase.z - 0.3f};
    palmGrid[5][3] = {(ringBase.x + middleBase.x) * 0.5f - 0.01f, (ringBase.y + middleBase.y) * 0.5f + 0.075f, (ringBase.z + middleBase.z) * 0.5f - 0.3f};
    palmGrid[5][4] = {middleBase.x - 0.01f, middleBase.y + 0.09f, middleBase.z - 0.3f};
    palmGrid[5][5] = {(middleBase.x + indexBase.x) * 0.5f - 0.01f, (middleBase.y + indexBase.y) * 0.5f + 0.075f, (middleBase.z + indexBase.z) * 0.5f - 0.3f};
    palmGrid[5][6] = {indexBase.x - 0.015f, indexBase.y + 0.06f, indexBase.z - 0.3f};
    palmGrid[5][7] = {(indexBase.x + thumbBase.x) * 0.7f - 0.02f, (indexBase.y + thumbBase.y) * 0.7f + 0.03f, (indexBase.z + thumbBase.z) * 0.7f - 0.25f};
    palmGrid[5][8] = {thumbBase.x - 0.05f, thumbBase.y + 0.01f, thumbBase.z - 0.2f};
    
    // Layer 6: Direct finger connections (seamless transitions)
    palmGrid[6][0] = {pinkyBase.x, pinkyBase.y - 0.05f, pinkyBase.z - 0.08f};
    palmGrid[6][1] = {(pinkyBase.x + ringBase.x) * 0.5f, (pinkyBase.y + ringBase.y) * 0.5f - 0.06f, (pinkyBase.z + ringBase.z) * 0.5f - 0.08f};
    palmGrid[6][2] = {ringBase.x, ringBase.y - 0.06f, ringBase.z - 0.08f};
    palmGrid[6][3] = {(ringBase.x + middleBase.x) * 0.5f, (ringBase.y + middleBase.y) * 0.5f - 0.07f, (ringBase.z + middleBase.z) * 0.5f - 0.08f};
    palmGrid[6][4] = {middleBase.x, middleBase.y - 0.08f, middleBase.z - 0.08f};
    palmGrid[6][5] = {(middleBase.x + indexBase.x) * 0.5f, (middleBase.y + indexBase.y) * 0.5f - 0.07f, (middleBase.z + indexBase.z) * 0.5f - 0.08f};
    palmGrid[6][6] = {indexBase.x, indexBase.y - 0.06f, indexBase.z - 0.08f};
    palmGrid[6][7] = {(indexBase.x + thumbBase.x) * 0.6f, (indexBase.y + thumbBase.y) * 0.6f - 0.05f, (indexBase.z + thumbBase.z) * 0.6f - 0.06f};
    palmGrid[6][8] = {thumbBase.x, thumbBase.y - 0.04f, thumbBase.z - 0.05f};
    
    // Create back surface vertices (mirrored below palm)
    Vec3 backGrid[GRID_HEIGHT][GRID_WIDTH];
    for (int row = 0; row < GRID_HEIGHT; row++) {
        for (int col = 0; col < GRID_WIDTH; col++) {
            backGrid[row][col] = {palmGrid[row][col].x, palmGrid[row][col].y - 0.35f, palmGrid[row][col].z};
        }
    }
    
    // IDENTICAL ULTRA-DENSE MESH RENDERING as palm 1
    glBegin(GL_TRIANGLES);
    
    // PALM SURFACE (top) - Complete quad grid tessellation
    Vec3 normalUp = {0, 1, 0.15f};
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
    Vec3 normalDown = {0, -1, -0.15f};
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
    // Draw main torso body (clothing)
    glColor3f(0.2f, 0.4f, 0.8f); glBegin(GL_TRIANGLES);
    BAND(R0, Y0, R1, Y1); BAND(R1, Y1, R2, Y2); BAND(R2, Y2, R3, Y3); BAND(R3, Y3, R4, Y4); BAND(R4, Y4, R5, Y5); BAND(R5, Y5, R6, Y6); BAND(R6, Y6, R7, Y7); BAND(R7, Y7, R8, Y8);
    BAND(R8, Y8, R9, Y9); BAND(R9, Y9, R10, Y10); BAND(R10, Y10, R11, Y11); BAND(R11, Y11, R12, Y12); BAND(R12, Y12, R13, Y13); BAND(R13, Y13, R14, Y14); BAND(R14, Y14, R15, Y15);
    glEnd();
    
    // Draw skin areas (neck) and enhanced shoulders
    glColor3f(0.9f, 0.7f, 0.6f); glBegin(GL_TRIANGLES);
    BAND(R15, Y15, R16, Y16); BAND(R16, Y16, R17, Y17); BAND(R17, Y17, R18, Y18);
    glEnd();
    
    // Add shoulder socket geometry for better arm attachment
    drawShoulderSockets();
    
    drawHeadSphere();
}

// Draw enhanced shoulder sockets for natural arm attachment
void drawShoulderSockets() {
    glColor3f(0.9f, 0.7f, 0.6f); // Skin color to match neck
    
    // Shoulder parameters
    const float shoulderRadius = 0.12f;
    const float shoulderHeight = Y16;
    const float shoulderOffset = R16 + 0.08f; // Extend slightly beyond torso radius
    const int shoulderSides = 8;
    
    // Draw left shoulder socket
    glPushMatrix();
    glTranslatef(-shoulderOffset, shoulderHeight, 0.0f);
    
    glBegin(GL_TRIANGLES);
    // Create rounded shoulder socket
    for (int i = 0; i < shoulderSides; i++) {
        float angle1 = (float)i * 2.0f * PI / shoulderSides;
        float angle2 = (float)(i + 1) * 2.0f * PI / shoulderSides;
        
        float x1 = shoulderRadius * cosf(angle1);
        float y1 = shoulderRadius * sinf(angle1) * 0.6f; // Flattened vertically
        float z1 = shoulderRadius * sinf(angle1) * 0.8f; // Slightly compressed
        
        float x2 = shoulderRadius * cosf(angle2);
        float y2 = shoulderRadius * sinf(angle2) * 0.6f;
        float z2 = shoulderRadius * sinf(angle2) * 0.8f;
        
        // Calculate normals
        Vec3 normal1 = {cosf(angle1), sinf(angle1) * 0.6f, sinf(angle1) * 0.8f};
        Vec3 normal2 = {cosf(angle2), sinf(angle2) * 0.6f, sinf(angle2) * 0.8f};
        float len1 = sqrtf(normal1.x*normal1.x + normal1.y*normal1.y + normal1.z*normal1.z);
        float len2 = sqrtf(normal2.x*normal2.x + normal2.y*normal2.y + normal2.z*normal2.z);
        if (len1 > 0.001f) { normal1.x /= len1; normal1.y /= len1; normal1.z /= len1; }
        if (len2 > 0.001f) { normal2.x /= len2; normal2.y /= len2; normal2.z /= len2; }
        
        // Triangle 1: center -> point1 -> point2
        glNormal3f(0, 1, 0);
        glVertex3f(0, 0, 0); // Center
        glNormal3f(normal1.x, normal1.y, normal1.z);
        glVertex3f(x1, y1, z1);
        glNormal3f(normal2.x, normal2.y, normal2.z);
        glVertex3f(x2, y2, z2);
        
        // Add connection to torso
        glNormal3f(0.8f, 0, 0); // Point toward torso center
        glVertex3f(0, 0, 0);
        glVertex3f(x1 * 0.5f, y1 * 0.5f + 0.02f, z1 * 0.5f); // Blend to torso
        glVertex3f(x2 * 0.5f, y2 * 0.5f + 0.02f, z2 * 0.5f);
    }
    glEnd();
    glPopMatrix();
    
    // Draw right shoulder socket (mirrored)
    glPushMatrix();
    glTranslatef(shoulderOffset, shoulderHeight, 0.0f);
    
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < shoulderSides; i++) {
        float angle1 = (float)i * 2.0f * PI / shoulderSides;
        float angle2 = (float)(i + 1) * 2.0f * PI / shoulderSides;
        
        float x1 = -shoulderRadius * cosf(angle1); // Mirrored X
        float y1 = shoulderRadius * sinf(angle1) * 0.6f;
        float z1 = shoulderRadius * sinf(angle1) * 0.8f;
        
        float x2 = -shoulderRadius * cosf(angle2); // Mirrored X
        float y2 = shoulderRadius * sinf(angle2) * 0.6f;
        float z2 = shoulderRadius * sinf(angle2) * 0.8f;
        
        // Calculate normals (mirrored)
        Vec3 normal1 = {-cosf(angle1), sinf(angle1) * 0.6f, sinf(angle1) * 0.8f};
        Vec3 normal2 = {-cosf(angle2), sinf(angle2) * 0.6f, sinf(angle2) * 0.8f};
        float len1 = sqrtf(normal1.x*normal1.x + normal1.y*normal1.y + normal1.z*normal1.z);
        float len2 = sqrtf(normal2.x*normal2.x + normal2.y*normal2.y + normal2.z*normal2.z);
        if (len1 > 0.001f) { normal1.x /= len1; normal1.y /= len1; normal1.z /= len1; }
        if (len2 > 0.001f) { normal2.x /= len2; normal2.y /= len2; normal2.z /= len2; }
        
        // Triangle 1: center -> point2 -> point1 (reversed winding for right side)
        glNormal3f(0, 1, 0);
        glVertex3f(0, 0, 0); // Center
        glNormal3f(normal2.x, normal2.y, normal2.z);
        glVertex3f(x2, y2, z2);
        glNormal3f(normal1.x, normal1.y, normal1.z);
        glVertex3f(x1, y1, z1);
        
        // Add connection to torso
        glNormal3f(-0.8f, 0, 0); // Point toward torso center (mirrored)
        glVertex3f(0, 0, 0);
        glVertex3f(x2 * 0.5f, y2 * 0.5f + 0.02f, z2 * 0.5f); // Blend to torso
        glVertex3f(x1 * 0.5f, y1 * 0.5f + 0.02f, z1 * 0.5f);
    }
    glEnd();
    glPopMatrix();
}

// Draw Minecraft-style cube shoulder
void drawCubeShoulder(float x, float y, float z, float width, float height, float depth) {
    glPushMatrix();
    glTranslatef(x, y, z);
    
    glColor3f(0.9f, 0.7f, 0.6f); // Skin color
    
    float w = width / 2.0f;
    float h = height / 2.0f;
    float d = depth / 2.0f;
    
    glBegin(GL_QUADS);
    
    // Front face
    glNormal3f(0.0f, 0.0f, 1.0f);
    glVertex3f(-w, -h, d);
    glVertex3f(w, -h, d);
    glVertex3f(w, h, d);
    glVertex3f(-w, h, d);
    
    // Back face
    glNormal3f(0.0f, 0.0f, -1.0f);
    glVertex3f(-w, -h, -d);
    glVertex3f(-w, h, -d);
    glVertex3f(w, h, -d);
    glVertex3f(w, -h, -d);
    
    // Left face
    glNormal3f(-1.0f, 0.0f, 0.0f);
    glVertex3f(-w, -h, -d);
    glVertex3f(-w, -h, d);
    glVertex3f(-w, h, d);
    glVertex3f(-w, h, -d);
    
    // Right face
    glNormal3f(1.0f, 0.0f, 0.0f);
    glVertex3f(w, -h, -d);
    glVertex3f(w, h, -d);
    glVertex3f(w, h, d);
    glVertex3f(w, -h, d);
    
    // Top face
    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(-w, h, -d);
    glVertex3f(-w, h, d);
    glVertex3f(w, h, d);
    glVertex3f(w, h, -d);
    
    // Bottom face
    glNormal3f(0.0f, -1.0f, 0.0f);
    glVertex3f(-w, -h, -d);
    glVertex3f(w, -h, -d);
    glVertex3f(w, -h, d);
    glVertex3f(-w, -h, d);
    
    glEnd();
    glPopMatrix();
}

// Draw spherical shoulder for more realistic upper arm connection
void drawSphereShoulder(float x, float y, float z, float radius) {
    glPushMatrix();
    glTranslatef(x, y, z);
    
    glColor3f(0.9f, 0.7f, 0.6f); // Skin color
    
    // Draw sphere using triangular strips for smooth surface
    const int slices = 16;  // More slices for smoother sphere
    const int stacks = 12;  // More stacks for better quality
    
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
            
            // Calculate normals for lighting
            glNormal3f(x0/radius, y0/radius, z0/radius);
            glVertex3f(x0, y0, z0);
            glNormal3f(x1/radius, y1/radius, z1/radius);
            glVertex3f(x1, y1, z1);
        }
        glEnd();
    }
    
    glPopMatrix();
}

// --- Arm and Hand Drawing ---
void drawArmsAndHands(float leftArmAngle, float rightArmAngle) {
    // Enable texturing if available
    if (g_TextureEnabled && g_HandTexture != 0) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, g_HandTexture);
        glColor3f(1.0f, 1.0f, 1.0f); // White to show texture clearly
    } else {
        glDisable(GL_TEXTURE_2D);
        glColor3f(0.85f, 0.64f, 0.52f); // Skin color
    }

    // --- Draw Left Arm ---
    glPushMatrix();
    // Attach to left spherical shoulder - connect to the outer edge of the sphere
    float shoulderRadius = 0.25f; // Match the updated spherical shoulder radius
    float shoulderXOffset = R16 + shoulderRadius + 0.05f; // Match the updated offset
    float armConnectionOffset = shoulderXOffset + shoulderRadius; // Connect to outer edge of sphere
    glTranslatef(-armConnectionOffset, Y16 + 0.05f, 0.0f); // Match updated Y position
    
    // Position arm to hang naturally down at the side with palm facing out
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);    // Pitch arm down to hang naturally
    glRotatef(0.0f, 0.0f, 1.0f, 0.0f);     // No yaw rotation
    glRotatef(20.0f, 0.0f, 0.0f, 1.0f);    // Roll to orient palm outward (changed from -20 to +20)
    

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
    // Rotate the left hand to face palm outward
    glRotatef(90.0f, 0.0f, 0.0f, 1.0f);    // Rotate hand 180 degrees around Z-axis to face palm outward
    glScalef(HAND_SCALE, HAND_SCALE, HAND_SCALE);
    
    // Re-enable texture for hand
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

    glPopMatrix();

    // --- Draw Right Arm ---
    glPushMatrix();
    // Attach to right shoulder edge - connect to the outer edge of the realistic shoulder
    glTranslatef(armConnectionOffset, Y16 + 0.05f, 0.0f); // Match updated Y position
    
    // Position arm to hang naturally down at the side with palm facing out
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);    // Pitch arm down to hang naturally
    glRotatef(0.0f, 0.0f, 1.0f, 0.0f);     // No yaw rotation
    glRotatef(-20.0f, 0.0f, 0.0f, 1.0f);   // Roll to orient palm outward (changed from +20 to -20)

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
    // Rotate the right hand to face palm outward
    glRotatef(-90.0f, 0.0f, 0.0f, 1.0f);   // Rotate hand -90 degrees around Y-axis to face palm outward
    glScalef(HAND_SCALE, HAND_SCALE, HAND_SCALE);
    
    // Re-enable texture for hand
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

    glPopMatrix();

    // Disable texture after drawing hands
    glDisable(GL_TEXTURE_2D);
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
    
    // Draw spherical shoulders for realistic upper arm connection
    float shoulderRadius = 0.25f;  // Increased radius for longer/wider shoulders
    float shoulderY = Y16 + 0.05f; // Slightly raised for better connection to neck
    float shoulderXOffset = R16 + shoulderRadius + 0.05f; // Position based on radius
    
    // Left spherical shoulder
    drawSphereShoulder(-shoulderXOffset, shoulderY, 0.0f, shoulderRadius);
    
    // Right spherical shoulder
    drawSphereShoulder(shoulderXOffset, shoulderY, 0.0f, shoulderRadius);

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