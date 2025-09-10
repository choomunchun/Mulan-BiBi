// Build: Visual Studio (Win32/x64) — link opengl32.lib
// Hand Drawing with OpenGL using glVertex3f

#define NOMINMAX
#include <Windows.h>
#include <windowsx.h>  // GET_X_LPARAM, GET_Y_LPARAM, GET_WHEEL_DELTA_WPARAM
#include <gl/GL.h>
#include <gl/GLU.h>
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")

#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#endif
#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))
#endif
#ifndef GET_WHEEL_DELTA_WPARAM
#define GET_WHEEL_DELTA_WPARAM(wp) ((short)HIWORD(wp))
#endif

#include <vector>
#include <cmath>

// =========================== Config ===========================
static const char* kWindowTitle = "Hand Drawing with OpenGL";

// =========================== Texture Support ===========================
GLuint g_HandTexture = 0; // texture name
BITMAP BMP; // bitmap structure
HBITMAP hBMP = NULL; // bitmap handle

// Texture loading function
GLuint loadTexture(LPCSTR filename) {
    GLuint texture = 0;
    
    // Initialize texture info
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    HBITMAP hBMP = (HBITMAP)LoadImage(GetModuleHandle(NULL), filename, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION | LR_LOADFROMFILE);
    
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
            float variation = (sin(x * 0.2f) * cos(y * 0.15f) + 1.0f) * 0.1f;
            
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

// =========================== Types ============================
struct Vec3 { float x = 0, y = 0, z = 0; };

// Hand joint structure based on the diagram
struct HandJoint {
    Vec3 position;
    int parentIndex;  // -1 for root joints
};

// Arm joint structure for low-poly arm
struct ArmJoint {
    Vec3 position;
    int parentIndex;  // -1 for root joints
};

// =============================================================

// ====================== Global Win32/GL =======================
HWND  g_hWnd = nullptr;
HDC   g_hDC = nullptr;
HGLRC g_hRC = nullptr;
int   g_Width = 1280, g_Height = 720;

// Camera/orbit/pan/zoom
float g_CamDist = 5.0f;
float g_OrbitYawDeg = 0.0f;
float g_OrbitPitchDeg = 15.0f;
float g_PanX = 0.0f, g_PanY = 0.0f;

bool  g_Wireframe = false;
bool  g_TextureEnabled = true;

// Mouse
bool  g_LButtonDown = false;
int   g_LastMouseX = 0, g_LastMouseY = 0;

// Hand data - 21 joints as shown in the diagram
std::vector<HandJoint> g_HandJoints;
std::vector<HandJoint> g_HandJoints2; // Second hand

// Arm data - 4 joints for low-poly arm (shoulder, elbow, wrist connection)
std::vector<ArmJoint> g_ArmJoints;
std::vector<ArmJoint> g_ArmJoints2; // Second arm
// =============================================================

// ========================= Math utils =========================
static const float kPI = 3.14159265358979323846f;

static void PerspectiveGL(double fovY, double aspect, double zNear, double zFar) {
    double fH = std::tan(fovY * 0.5 * 3.14159265358979323846 / 180.0) * zNear;
    double fW = fH * aspect;
    glFrustum(-fW, fW, -fH, fH, zNear, zFar);
}
// =============================================================

// ====================== Hand Creation ======================
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
    float xOffset = 4.0f; // Position to the right of first hand
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
    g_ArmJoints.resize(6);  // 6 joints to match Mulan OBJ anatomy structure
    
    // Restructured arm design following Mulan OBJ file vertex coordinates
    // Y-axis: body height, X-axis: width, Z-axis: depth
    // Scaling coordinates to match our hand size (original model is much larger)
    
    // ARM_WRIST (0) - attachment point to hand wrist
    g_ArmJoints[0] = {{0.0f, 0.0f, -0.1f}, -1};    // Connection point
    
    // ARM_LOWER_FOREARM (1) - based on Bip001_L_Forearm_038 vertices
    g_ArmJoints[1] = {{0.15f, 0.08f, -2.0f}, 0};   // Scaled from ~1.5, ~21-23, ~0.3-0.7
    
    // ARM_MID_FOREARM (2) - mid-section of forearm
    g_ArmJoints[2] = {{0.25f, 0.12f, -4.2f}, 1};   // Natural progression
    
    // ARM_ELBOW (3) - elbow transition point
    g_ArmJoints[3] = {{0.30f, 0.15f, -6.8f}, 2};   // Slight outward curve at elbow
    
    // ARM_UPPER_ARM (4) - based on Bip001_L_UpperArm_037 vertices  
    g_ArmJoints[4] = {{0.35f, 0.20f, -9.5f}, 3};   // Scaled from ~2.0-2.9, ~22-24, ~0.3-0.8
    
    // ARM_SHOULDER (5) - based on Bip001_L_Clavicle_036 connection
    g_ArmJoints[5] = {{0.28f, 0.35f, -12.0f}, 4};  // Shoulder connection with natural rise
}

// Initialize second arm (mirrored and positioned for the second hand)
static void InitializeArm2() {
    g_ArmJoints2.clear();
    g_ArmJoints2.resize(6);  // 6 joints to match Mulan OBJ anatomy structure
    
    // Mirror the arm to match the second hand position
    float xOffset = 4.0f; // Position to match second hand
    float flipSign = -1.0f; // Flip X coordinates to mirror the arm
    
    // ARM_WRIST (0) - attachment point to second hand wrist
    g_ArmJoints2[0] = {{xOffset + 0.0f * flipSign, 0.0f, -0.1f}, -1};    // Connection point
    
    // ARM_LOWER_FOREARM (1) - mirrored
    g_ArmJoints2[1] = {{xOffset + 0.15f * flipSign, 0.08f, -2.0f}, 0};   
    
    // ARM_MID_FOREARM (2) - mirrored
    g_ArmJoints2[2] = {{xOffset + 0.25f * flipSign, 0.12f, -4.2f}, 1};   
    
    // ARM_ELBOW (3) - mirrored
    g_ArmJoints2[3] = {{xOffset + 0.30f * flipSign, 0.15f, -6.8f}, 2};   
    
    // ARM_UPPER_ARM (4) - mirrored
    g_ArmJoints2[4] = {{xOffset + 0.35f * flipSign, 0.20f, -9.5f}, 3};   
    
    // ARM_SHOULDER (5) - mirrored
    g_ArmJoints2[5] = {{xOffset + 0.28f * flipSign, 0.35f, -12.0f}, 4};  
}
// =============================================================

// ===================== OpenGL setup/draw ======================
static bool initPixelFormat(HDC hdc) {
    PIXELFORMATDESCRIPTOR pfd = { sizeof(PIXELFORMATDESCRIPTOR) };
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cDepthBits = 24;
    pfd.iLayerType = PFD_MAIN_PLANE;
    int pf = ChoosePixelFormat(hdc, &pfd);
    if (!pf) return false;
    if (!SetPixelFormat(hdc, pf, &pfd)) return false;
    return true;
}

static void applyPolygonMode() {
    glPolygonMode(GL_FRONT_AND_BACK, g_Wireframe ? GL_LINE : GL_FILL);
}

static void initGL() {
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    GLfloat lightPos[4] = { 2.0f, 2.0f, 2.0f, 0.0f };
    GLfloat lightCol[4] = { 1, 1, 1, 1 };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightCol);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightCol);

    GLfloat matDiff[4] = { 0.8f, 0.7f, 0.6f, 1 };
    GLfloat matSpec[4] = { 0.3f, 0.3f, 0.3f, 1 };
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, matDiff);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, matSpec);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 32.0f);

    // Initialize texture
    g_HandTexture = loadTexture("skin.bmp");
    if (g_HandTexture == 0) {
        // If texture file not found, create procedural skin texture
        g_HandTexture = createSkinTexture();
    }

    applyPolygonMode();
}

static void resizeGL(int w, int h) {
    g_Width = (w > 0) ? w : 1;
    g_Height = (h > 0) ? h : 1;
    glViewport(0, 0, g_Width, g_Height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    PerspectiveGL(45.0, double(g_Width) / double(g_Height), 0.1, 100.0);
    glMatrixMode(GL_MODELVIEW);
}

// Draw a sphere at a joint position
static void drawJoint(const Vec3& pos, float radius) {
    glPushMatrix();
    glTranslatef(pos.x, pos.y, pos.z);
    
    // Simple sphere using triangular strips
    const int slices = 8;
    const int stacks = 6;
    
    for (int i = 0; i < stacks; ++i) {
        float lat0 = kPI * (-0.5f + (float)i / stacks);
        float lat1 = kPI * (-0.5f + (float)(i + 1) / stacks);
        float y0 = radius * sinf(lat0);
        float y1 = radius * sinf(lat1);
        float r0 = radius * cosf(lat0);
        float r1 = radius * cosf(lat1);
        
        glBegin(GL_TRIANGLE_STRIP);
        for (int j = 0; j <= slices; ++j) {
            float lng = 2 * kPI * (float)j / slices;
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

// Draw a bone between two joints
static void drawBone(const Vec3& start, const Vec3& end, float radius) {
    Vec3 dir = {end.x - start.x, end.y - start.y, end.z - start.z};
    float length = sqrtf(dir.x*dir.x + dir.y*dir.y + dir.z*dir.z);
    
    if (length < 0.001f) return;
    
    // Normalize direction
    dir.x /= length; dir.y /= length; dir.z /= length;
    
    glPushMatrix();
    glTranslatef(start.x, start.y, start.z);
    
    // Align with direction vector
    float pitch = asinf(-dir.y) * 180.0f / kPI;
    float yaw = atan2f(dir.x, dir.z) * 180.0f / kPI;
    glRotatef(yaw, 0, 1, 0);
    glRotatef(pitch, 1, 0, 0);
    
    // Draw cylinder
    const int sides = 8;
    glBegin(GL_TRIANGLE_STRIP);
    for (int i = 0; i <= sides; ++i) {
        float angle = 2 * kPI * i / sides;
        float x = radius * cosf(angle);
        float y = radius * sinf(angle);
        
        glNormal3f(cosf(angle), sinf(angle), 0);
        glVertex3f(x, y, 0);
        glVertex3f(x, y, length);
    }
    glEnd();
    
    glPopMatrix();
}

// Helper function to create a skin segment between two joints
static void drawFingerSegment(const Vec3& joint1, const Vec3& joint2, float radius1, float radius2) {
    Vec3 dir = {joint2.x - joint1.x, joint2.y - joint1.y, joint2.z - joint1.z};
    float length = sqrtf(dir.x*dir.x + dir.y*dir.y + dir.z*dir.z);
    
    if (length < 0.001f) return;
    
    // Normalize direction
    dir.x /= length; dir.y /= length; dir.z /= length;
    
    glPushMatrix();
    glTranslatef(joint1.x, joint1.y, joint1.z);
    
    // Align with direction vector
    float pitch = asinf(-dir.y) * 180.0f / kPI;
    float yaw = atan2f(dir.x, dir.z) * 180.0f / kPI;
    glRotatef(yaw, 0, 1, 0);
    glRotatef(pitch, 1, 0, 0);
    
    // Draw textured tapered cylinder skin using quad strips
    const int sides = 12; // More sides for smoother skin
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= sides; ++i) {
        float angle = 2 * kPI * i / sides;
        float x1 = radius1 * cosf(angle);
        float y1 = radius1 * sinf(angle);
        float x2 = radius2 * cosf(angle);
        float y2 = radius2 * sinf(angle);
        
        // Add texture coordinates for finger skin
        float u = (float)i / sides; // Wrap around finger
        float v1 = 0.0f; // Start of segment
        float v2 = 1.0f; // End of segment
        
        glNormal3f(cosf(angle), sinf(angle), 0);
        glTexCoord2f(u, v1); glVertex3f(x1, y1, 0);
        glTexCoord2f(u, v2); glVertex3f(x2, y2, length);
    }
    glEnd();
    
    // Draw textured caps
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0, 0, -1);
    glTexCoord2f(0.5f, 0.5f); glVertex3f(0, 0, 0); // Center texture coordinate
    for (int i = 0; i <= sides; ++i) {
        float angle = 2 * kPI * i / sides;
        float x = radius1 * cosf(angle);
        float y = radius1 * sinf(angle);
        float u = 0.5f + 0.5f * cosf(angle);
        float v = 0.5f + 0.5f * sinf(angle);
        glTexCoord2f(u, v); glVertex3f(x, y, 0);
    }
    glEnd();
    
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0, 0, 1);
    glTexCoord2f(0.5f, 0.5f); glVertex3f(0, 0, length); // Center texture coordinate
    for (int i = sides; i >= 0; --i) {
        float angle = 2 * kPI * i / sides;
        float x = radius2 * cosf(angle);
        float y = radius2 * sinf(angle);
        float u = 0.5f + 0.5f * cosf(angle);
        float v = 0.5f + 0.5f * sinf(angle);
        glTexCoord2f(u, v); glVertex3f(x, y, length);
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
        float lat0 = kPI * (0.0f + (float)i / stacks / 2.0f);     // 0 to PI/2 (top half only)
        float lat1 = kPI * (0.0f + (float)(i + 1) / stacks / 2.0f);
        
        float y0 = radius * sinf(lat0);  // Height
        float y1 = radius * sinf(lat1);
        float r0 = radius * cosf(lat0);  // Radius at this height
        float r1 = radius * cosf(lat1);
        
        for (int j = 0; j < slices; j++) {
            float lng0 = 2 * kPI * (float)j / slices;
            float lng1 = 2 * kPI * (float)(j + 1) / slices;
            
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
    
    // Draw flat base to close the hemisphere
    Vec3 normalDown = {0, -1, 0};
    glNormal3f(normalDown.x, normalDown.y, normalDown.z);
    for (int j = 0; j < slices; j++) {
        float lng0 = 2 * kPI * (float)j / slices;
        float lng1 = 2 * kPI * (float)(j + 1) / slices;
        
        float x0 = radius * cosf(lng0), z0 = radius * sinf(lng0);
        float x1 = radius * cosf(lng1), z1 = radius * sinf(lng1);
        
        glTexCoord2f(0.5f, 0.5f); glVertex3f(0, 0, 0);
        glTexCoord2f(x0 * 0.5f + 0.5f, z0 * 0.5f + 0.5f); glVertex3f(x0, 0, z0);
        glTexCoord2f(x1 * 0.5f + 0.5f, z1 * 0.5f + 0.5f); glVertex3f(x1, 0, z1);
    }
    
    glEnd();
    glPopMatrix();
}

// Draw a low-poly finger segment (simple box) with texture coordinates
static void drawLowPolyFingerSegment(const Vec3& start, const Vec3& end, float radius) {
    Vec3 dir = {end.x - start.x, end.y - start.y, end.z - start.z};
    float length = sqrtf(dir.x*dir.x + dir.y*dir.y + dir.z*dir.z);
    
    if (length < 0.001f) return;
    
    // Normalize direction
    dir.x /= length; dir.y /= length; dir.z /= length;
    
    glPushMatrix();
    glTranslatef(start.x, start.y, start.z);
    
    // Align with direction vector
    float pitch = asinf(-dir.y) * 180.0f / kPI;
    float yaw = atan2f(dir.x, dir.z) * 180.0f / kPI;
    glRotatef(yaw, 0, 1, 0);
    glRotatef(pitch, 1, 0, 0);
    
    // Create organic cylindrical segment - 8 sides for rounded appearance
    const int sides = 8;
    float angleStep = 2.0f * kPI / sides;
    
    glBegin(GL_TRIANGLES);
    
    // End caps (rounded)
    // Start cap
    Vec3 normalStart = {0, 0, -1};
    glNormal3f(normalStart.x, normalStart.y, normalStart.z);
    for (int i = 0; i < sides; i++) {
        float angle1 = i * angleStep;
        float angle2 = ((i + 1) % sides) * angleStep;
        
        float x1 = radius * cosf(angle1);
        float y1 = radius * sinf(angle1);
        float x2 = radius * cosf(angle2);
        float y2 = radius * sinf(angle2);
        
        glTexCoord2f(0.5f, 0.5f); glVertex3f(0, 0, 0);
        glTexCoord2f(x1 * 0.5f + 0.5f, y1 * 0.5f + 0.5f); glVertex3f(x1, y1, 0);
        glTexCoord2f(x2 * 0.5f + 0.5f, y2 * 0.5f + 0.5f); glVertex3f(x2, y2, 0);
    }
    
    // End cap
    Vec3 normalEnd = {0, 0, 1};
    glNormal3f(normalEnd.x, normalEnd.y, normalEnd.z);
    for (int i = 0; i < sides; i++) {
        float angle1 = i * angleStep;
        float angle2 = ((i + 1) % sides) * angleStep;
        
        float x1 = radius * cosf(angle1);
        float y1 = radius * sinf(angle1);
        float x2 = radius * cosf(angle2);
        float y2 = radius * sinf(angle2);
        
        glTexCoord2f(0.5f, 0.5f); glVertex3f(0, 0, length);
        glTexCoord2f(x2 * 0.5f + 0.5f, y2 * 0.5f + 0.5f); glVertex3f(x2, y2, length);
        glTexCoord2f(x1 * 0.5f + 0.5f, y1 * 0.5f + 0.5f); glVertex3f(x1, y1, length);
    }
    
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

// Draw a complete low-poly finger with fingertip for hand 1
static void drawLowPolyFinger(int mcpIdx, int pipIdx, int dipIdx, int tipIdx) {
    if (mcpIdx >= 0 && pipIdx >= 0) {
        // Proximal phalanx (largest segment) - bigger radius
        drawLowPolyFingerSegment(g_HandJoints[mcpIdx].position, g_HandJoints[pipIdx].position, 0.18f);
    }
    if (pipIdx >= 0 && dipIdx >= 0) {
        // Middle phalanx - bigger radius
        drawLowPolyFingerSegment(g_HandJoints[pipIdx].position, g_HandJoints[dipIdx].position, 0.15f);
    }
    if (dipIdx >= 0 && tipIdx >= 0) {
        // Distal phalanx (smallest segment) - bigger radius
        drawLowPolyFingerSegment(g_HandJoints[dipIdx].position, g_HandJoints[tipIdx].position, 0.12f);
        
        // Add fingertip - half sphere at the tip
        drawFingertip(g_HandJoints[tipIdx].position, 0.12f);
    }
}

// Draw a complete low-poly finger with fingertip for hand 2
static void drawLowPolyFinger2(int mcpIdx, int pipIdx, int dipIdx, int tipIdx) {
    if (mcpIdx >= 0 && pipIdx >= 0) {
        // Proximal phalanx (largest segment) - bigger radius
        drawLowPolyFingerSegment(g_HandJoints2[mcpIdx].position, g_HandJoints2[pipIdx].position, 0.18f);
    }
    if (pipIdx >= 0 && dipIdx >= 0) {
        // Middle phalanx - bigger radius
        drawLowPolyFingerSegment(g_HandJoints2[pipIdx].position, g_HandJoints2[dipIdx].position, 0.15f);
    }
    if (dipIdx >= 0 && tipIdx >= 0) {
        // Distal phalanx (smallest segment) - bigger radius
        drawLowPolyFingerSegment(g_HandJoints2[dipIdx].position, g_HandJoints2[tipIdx].position, 0.12f);
        
        // Add fingertip - half sphere at the tip
        drawFingertip(g_HandJoints2[tipIdx].position, 0.12f);
    }
}

// Draw low-poly thumb with bigger dimensions and fingertip for hand 1
static void drawLowPolyThumb() {
    drawLowPolyFingerSegment(g_HandJoints[1].position, g_HandJoints[2].position, 0.20f); // CMC to MCP - bigger
    drawLowPolyFingerSegment(g_HandJoints[2].position, g_HandJoints[3].position, 0.17f); // MCP to IP - bigger
    drawLowPolyFingerSegment(g_HandJoints[3].position, g_HandJoints[4].position, 0.14f); // IP to tip - bigger
    
    // Add thumb tip - half sphere at the thumb tip
    drawFingertip(g_HandJoints[4].position, 0.14f);
}

// Draw low-poly thumb with bigger dimensions and fingertip for hand 2
static void drawLowPolyThumb2() {
    drawLowPolyFingerSegment(g_HandJoints2[1].position, g_HandJoints2[2].position, 0.20f); // CMC to MCP - bigger
    drawLowPolyFingerSegment(g_HandJoints2[2].position, g_HandJoints2[3].position, 0.17f); // MCP to IP - bigger
    drawLowPolyFingerSegment(g_HandJoints2[3].position, g_HandJoints2[4].position, 0.14f); // IP to tip - bigger
    
    // Add thumb tip - half sphere at the thumb tip
    drawFingertip(g_HandJoints2[4].position, 0.14f);
}

// ====================== Arm Drawing Functions ======================

// Draw a low-poly cube/box arm segment with texture coordinates (robot style)
static void drawRobotArmSegment(const Vec3& start, const Vec3& end, float width, float height) {
    Vec3 dir = {end.x - start.x, end.y - start.y, end.z - start.z};
    float length = sqrtf(dir.x*dir.x + dir.y*dir.y + dir.z*dir.z);
    
    if (length < 0.001f) return;
    
    // Normalize direction
    dir.x /= length; dir.y /= length; dir.z /= length;
    
    glPushMatrix();
    glTranslatef(start.x, start.y, start.z);
    
    // Align with direction vector
    float pitch = asinf(-dir.y) * 180.0f / kPI;
    float yaw = atan2f(dir.x, dir.z) * 180.0f / kPI;
    glRotatef(yaw, 0, 1, 0);
    glRotatef(pitch, 1, 0, 0);
    
    // Create rectangular box segment (robot arm style)
    float halfWidth = width * 0.5f;
    float halfHeight = height * 0.5f;
    
    glBegin(GL_TRIANGLES);
    
    // Front face (+X side)
    glNormal3f(1, 0, 0);
    glTexCoord2f(0, 0); glVertex3f(halfWidth, -halfHeight, 0);
    glTexCoord2f(1, 0); glVertex3f(halfWidth, -halfHeight, length);
    glTexCoord2f(1, 1); glVertex3f(halfWidth, halfHeight, length);
    
    glTexCoord2f(0, 0); glVertex3f(halfWidth, -halfHeight, 0);
    glTexCoord2f(1, 1); glVertex3f(halfWidth, halfHeight, length);
    glTexCoord2f(0, 1); glVertex3f(halfWidth, halfHeight, 0);
    
    // Back face (-X side)
    glNormal3f(-1, 0, 0);
    glTexCoord2f(0, 0); glVertex3f(-halfWidth, -halfHeight, length);
    glTexCoord2f(1, 0); glVertex3f(-halfWidth, -halfHeight, 0);
    glTexCoord2f(1, 1); glVertex3f(-halfWidth, halfHeight, 0);
    
    glTexCoord2f(0, 0); glVertex3f(-halfWidth, -halfHeight, length);
    glTexCoord2f(1, 1); glVertex3f(-halfWidth, halfHeight, 0);
    glTexCoord2f(0, 1); glVertex3f(-halfWidth, halfHeight, length);
    
    // Top face (+Y side)
    glNormal3f(0, 1, 0);
    glTexCoord2f(0, 0); glVertex3f(-halfWidth, halfHeight, 0);
    glTexCoord2f(1, 0); glVertex3f(halfWidth, halfHeight, 0);
    glTexCoord2f(1, 1); glVertex3f(halfWidth, halfHeight, length);
    
    glTexCoord2f(0, 0); glVertex3f(-halfWidth, halfHeight, 0);
    glTexCoord2f(1, 1); glVertex3f(halfWidth, halfHeight, length);
    glTexCoord2f(0, 1); glVertex3f(-halfWidth, halfHeight, length);
    
    // Bottom face (-Y side)
    glNormal3f(0, -1, 0);
    glTexCoord2f(0, 0); glVertex3f(-halfWidth, -halfHeight, length);
    glTexCoord2f(1, 0); glVertex3f(halfWidth, -halfHeight, length);
    glTexCoord2f(1, 1); glVertex3f(halfWidth, -halfHeight, 0);
    
    glTexCoord2f(0, 0); glVertex3f(-halfWidth, -halfHeight, length);
    glTexCoord2f(1, 1); glVertex3f(halfWidth, -halfHeight, 0);
    glTexCoord2f(0, 1); glVertex3f(-halfWidth, -halfHeight, 0);
    
    // End cap (+Z side)
    glNormal3f(0, 0, 1);
    glTexCoord2f(0, 0); glVertex3f(-halfWidth, -halfHeight, length);
    glTexCoord2f(1, 0); glVertex3f(-halfWidth, halfHeight, length);
    glTexCoord2f(1, 1); glVertex3f(halfWidth, halfHeight, length);
    
    glTexCoord2f(0, 0); glVertex3f(-halfWidth, -halfHeight, length);
    glTexCoord2f(1, 1); glVertex3f(halfWidth, halfHeight, length);
    glTexCoord2f(0, 1); glVertex3f(halfWidth, -halfHeight, length);
    
    // Start cap (-Z side)
    glNormal3f(0, 0, -1);
    glTexCoord2f(0, 0); glVertex3f(-halfWidth, -halfHeight, 0);
    glTexCoord2f(1, 0); glVertex3f(halfWidth, -halfHeight, 0);
    glTexCoord2f(1, 1); glVertex3f(halfWidth, halfHeight, 0);
    
    glTexCoord2f(0, 0); glVertex3f(-halfWidth, -halfHeight, 0);
    glTexCoord2f(1, 1); glVertex3f(halfWidth, halfHeight, 0);
    glTexCoord2f(0, 1); glVertex3f(-halfWidth, halfHeight, 0);
    
    glEnd();
    glPopMatrix();
}

// Draw anatomically complex arm segment inspired by Mulan OBJ vertex groups - WITH TEXTURE
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
    float pitch = asinf(-dir.y) * 180.0f / kPI;
    float yaw = atan2f(dir.x, dir.z) * 180.0f / kPI;
    glRotatef(yaw, 0, 1, 0);
    glRotatef(pitch, 1, 0, 0);
    
    // Layer 1: Base bone structure (like Mulan OBJ core) - WITH TEXTURE
    glColor3f(0.65f, 0.65f, 0.65f); // Bone color
    float boneHalf = boneWidth * 0.5f;
    
    glBegin(GL_TRIANGLES);
    // Draw octagonal bone core for realism with texture mapping
    for (int i = 0; i < 8; i++) {
        float angle1 = (float)i * 2.0f * kPI / 8.0f;
        float angle2 = (float)(i + 1) * 2.0f * kPI / 8.0f;
        
        float x1 = cosf(angle1) * boneHalf;
        float y1 = sinf(angle1) * boneHalf;
        float x2 = cosf(angle2) * boneHalf;
        float y2 = sinf(angle2) * boneHalf;
        
        // Texture coordinates for bone surface
        float u1 = (float)i / 8.0f;
        float u2 = (float)(i + 1) / 8.0f;
        
        // Side faces with texture mapping
        glNormal3f(cosf(angle1), sinf(angle1), 0);
        glTexCoord2f(u1, 0.0f); glVertex3f(x1, y1, 0);
        glTexCoord2f(u2, 0.0f); glVertex3f(x2, y2, 0);
        glTexCoord2f(u1, 1.0f); glVertex3f(x1, y1, length);
        
        glTexCoord2f(u1, 1.0f); glVertex3f(x1, y1, length);
        glTexCoord2f(u2, 0.0f); glVertex3f(x2, y2, 0);
        glTexCoord2f(u2, 1.0f); glVertex3f(x2, y2, length);
    }
    glEnd();
    
    // Layer 2: Muscle groups (inspired by Mulan OBJ muscle definition) - WITH TEXTURE
    glColor3f(0.6f, 0.55f, 0.55f); // Muscle color
    float muscleHalf = muscleWidth * 0.5f;
    
    // Draw 4 major muscle groups around the bone with texture mapping
    for (int muscle = 0; muscle < 4; muscle++) {
        float muscleAngle = (float)muscle * kPI * 0.5f; // 90-degree intervals
        float muscleX = cosf(muscleAngle) * muscleHalf;
        float muscleY = sinf(muscleAngle) * muscleHalf;
        
        // Muscle bulge with anatomical tapering
        float muscleStart = muscleHalf * 0.8f; // Tapered start
        float muscleMid = muscleHalf * 1.0f;   // Full bulge
        float muscleEnd = muscleHalf * 0.9f;   // Tapered end
        
        glBegin(GL_TRIANGLES);
        
        // Muscle segment with natural curve and texture mapping
        for (int seg = 0; seg < 6; seg++) {
            float z1 = (float)seg / 6.0f * length;
            float z2 = (float)(seg + 1) / 6.0f * length;
            
            // Calculate muscle bulge along length (realistic muscle shape)
            float bulge1 = muscleStart + (muscleMid - muscleStart) * sinf((float)seg / 6.0f * kPI);
            float bulge2 = muscleStart + (muscleMid - muscleStart) * sinf((float)(seg + 1) / 6.0f * kPI);
            
            float x1 = cosf(muscleAngle) * bulge1;
            float y1 = sinf(muscleAngle) * bulge1;
            float x2 = cosf(muscleAngle) * bulge2;
            float y2 = sinf(muscleAngle) * bulge2;
            
            // Texture coordinates for muscle surface
            float v1 = (float)seg / 6.0f;
            float v2 = (float)(seg + 1) / 6.0f;
            float u = (float)muscle / 4.0f;
            
            // Connect to bone center for realistic attachment with texture
            glNormal3f(cosf(muscleAngle), sinf(muscleAngle), 0);
            glTexCoord2f(u, v1); glVertex3f(0, 0, z1);
            glTexCoord2f(u + 0.25f, v1); glVertex3f(x1, y1, z1);
            glTexCoord2f(u + 0.25f, v2); glVertex3f(x2, y2, z2);
            
            glTexCoord2f(u, v1); glVertex3f(0, 0, z1);
            glTexCoord2f(u + 0.25f, v2); glVertex3f(x2, y2, z2);
            glTexCoord2f(u, v2); glVertex3f(0, 0, z2);
        }
        glEnd();
    }
    
    // Layer 3: Outer skin/armor layer (complete coverage like Mulan OBJ surface) - WITH TEXTURE
    glColor3f(0.7f, 0.7f, 0.7f); // Skin/armor color
    float skinHalf = baseWidth * 0.5f;
    float skinHeight = baseHeight * 0.5f;
    
    glBegin(GL_TRIANGLES);
    // Create smooth elliptical outer surface with texture mapping
    for (int i = 0; i < 12; i++) {
        float angle1 = (float)i * 2.0f * kPI / 12.0f;
        float angle2 = (float)(i + 1) * 2.0f * kPI / 12.0f;
        
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

// Draw complete anatomically complex arm following Mulan OBJ structure - PALM-FITTED WITH TEXTURE for hand 1
static void drawLowPolyArm() {
    if (g_ArmJoints.empty()) return;
    
    // Enable texturing for the entire arm to match hand
    if (g_TextureEnabled) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, g_HandTexture);
        glColor3f(1.0f, 1.0f, 1.0f); // White to show texture clearly
    } else {
        glDisable(GL_TEXTURE_2D);
        glColor3f(0.9f, 0.8f, 0.7f); // Skin color for non-textured mode
    }
    
    // Draw arm segments scaled to match palm size (palm width = 1.0f, so arm should be proportional)
    // Palm-fitted dimensions based on human anatomy ratios
    
    // Wrist to lower forearm - matches palm width (1.0f) with natural taper
    Vec3 wristConnect = {0.0f, 0.0f, 0.05f};  // Connection to hand
    drawAnatomicalArmSegment(wristConnect, g_ArmJoints[1].position, 
                           0.9f, 0.7f,    // Base: 90% of palm width, 70% height
                           0.7f,          // Muscle: 70% of palm width
                           0.5f);         // Bone: 50% of palm width
    
    // Lower forearm segment - slightly wider than wrist, proportional to palm
    Vec3 lowerForearmOverlap = {0.12f, 0.06f, -1.9f};  // Natural overlap
    drawAnatomicalArmSegment(lowerForearmOverlap, g_ArmJoints[2].position,
                           1.0f, 0.8f,    // Same as palm width
                           0.8f,          // 80% muscle coverage
                           0.6f);         // 60% bone core
    
    // Mid forearm segment - peak forearm, slightly larger than palm
    Vec3 midForearmOverlap = {0.22f, 0.10f, -4.1f};  // Continued curve
    drawAnatomicalArmSegment(midForearmOverlap, g_ArmJoints[3].position,
                           1.1f, 0.9f,    // 110% of palm for muscle bulk
                           0.9f,          // Strong forearm muscles
                           0.65f);        // Substantial bone
    
    // Elbow segment - transition joint, similar to palm width
    Vec3 elbowOverlap = {0.28f, 0.13f, -6.7f};  // Natural elbow curve
    drawAnatomicalArmSegment(elbowOverlap, g_ArmJoints[4].position,
                           1.05f, 0.85f,  // Slightly larger than palm
                           0.85f,         // Joint muscle support
                           0.7f);         // Major joint bone
    
    // Upper arm segment - larger than palm (natural arm proportions)
    Vec3 upperArmOverlap = {0.32f, 0.18f, -9.4f};  // Upper arm positioning
    drawAnatomicalArmSegment(upperArmOverlap, g_ArmJoints[5].position,
                           1.3f, 1.0f,    // 130% of palm width (realistic upper arm)
                           1.1f,          // Major muscle groups (biceps/triceps)
                           0.8f);         // Humerus bone
    
    // Draw palm-proportioned joints with realistic scaling and texture
    if (g_TextureEnabled) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, g_HandTexture);
        glColor3f(1.0f, 1.0f, 1.0f);
    } else {
        glDisable(GL_TEXTURE_2D);
    }
    
    // Joint sizing based on palm proportions (palm = 1.0f reference)
    for (int joint = 1; joint < 6; joint++) {
        Vec3 pos = g_ArmJoints[joint].position;
        float palmRatio = 0.15f; // 15% of palm width as base joint size
        
        // Joint core (bone/cartilage) - proportional to palm
        if (!g_TextureEnabled) glColor3f(0.8f, 0.8f, 0.8f);
        drawRobotJoint(pos, palmRatio + joint * 0.02f);
        
        // Joint capsule (ligaments/tendons) - slightly larger
        if (!g_TextureEnabled) glColor3f(0.7f, 0.65f, 0.6f);
        drawRobotJoint(pos, palmRatio * 1.5f + joint * 0.025f);
        
        // Joint housing (external structure) - palm-fitted
        if (!g_TextureEnabled) glColor3f(0.65f, 0.65f, 0.65f);
        drawRobotJoint(pos, palmRatio * 2.0f + joint * 0.03f);
    }
    
    // Add anatomical details scaled to palm size with texture
    if (g_TextureEnabled) {
        glColor3f(0.8f, 0.8f, 0.8f); // Lighter for textured tendons
    } else {
        glColor3f(0.5f, 0.5f, 0.6f); // Tendon/nerve color
    }
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    
    // Draw tendon lines proportional to palm width
    for (int i = 1; i < 5; i++) {
        Vec3 start = g_ArmJoints[i].position;
        Vec3 end = g_ArmJoints[i + 1].position;
        
        // Main tendon line (20% of palm width offset)
        float tendonOffset = 0.2f; // 20% of palm width
        glVertex3f(start.x + tendonOffset, start.y, start.z);
        glVertex3f(end.x + tendonOffset, end.y, end.z);
        
        // Secondary tendon line
        glVertex3f(start.x - tendonOffset, start.y, start.z);
        glVertex3f(end.x - tendonOffset, end.y, end.z);
    }
    
    glEnd();
    glLineWidth(1.0f);
}

// Draw complete anatomically complex arm following Mulan OBJ structure - PALM-FITTED WITH TEXTURE for hand 2
static void drawLowPolyArm2() {
    if (g_ArmJoints2.empty()) return;
    
    // Enable texturing for the entire arm to match hand
    if (g_TextureEnabled) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, g_HandTexture);
        glColor3f(1.0f, 1.0f, 1.0f); // White to show texture clearly
    } else {
        glDisable(GL_TEXTURE_2D);
        glColor3f(0.9f, 0.8f, 0.7f); // Skin color for non-textured mode
    }
    
    // Draw arm segments scaled to match palm size for second hand
    float xOffset = 4.0f; // Match second hand offset
    
    // Wrist to lower forearm - matches palm width (1.0f) with natural taper
    Vec3 wristConnect2 = {xOffset, 0.0f, 0.05f};  // Connection to second hand
    drawAnatomicalArmSegment(wristConnect2, g_ArmJoints2[1].position, 
                           0.9f, 0.7f,    // Base: 90% of palm width, 70% height
                           0.7f,          // Muscle: 70% of palm width
                           0.5f);         // Bone: 50% of palm width
    
    // Lower forearm segment - slightly wider than wrist, proportional to palm
    Vec3 lowerForearmOverlap2 = {xOffset - 0.12f, 0.06f, -1.9f};  // Natural overlap (mirrored)
    drawAnatomicalArmSegment(lowerForearmOverlap2, g_ArmJoints2[2].position,
                           1.0f, 0.8f,    // Same as palm width
                           0.8f,          // 80% muscle coverage
                           0.6f);         // 60% bone core
    
    // Mid forearm segment - peak forearm, slightly larger than palm
    Vec3 midForearmOverlap2 = {xOffset - 0.22f, 0.10f, -4.1f};  // Continued curve (mirrored)
    drawAnatomicalArmSegment(midForearmOverlap2, g_ArmJoints2[3].position,
                           1.1f, 0.9f,    // 110% of palm for muscle bulk
                           0.9f,          // Strong forearm muscles
                           0.65f);        // Substantial bone
    
    // Elbow segment - transition joint, similar to palm width
    Vec3 elbowOverlap2 = {xOffset - 0.28f, 0.13f, -6.7f};  // Natural elbow curve (mirrored)
    drawAnatomicalArmSegment(elbowOverlap2, g_ArmJoints2[4].position,
                           1.05f, 0.85f,  // Slightly larger than palm
                           0.85f,         // Joint muscle support
                           0.7f);         // Major joint bone
    
    // Upper arm segment - larger than palm (natural arm proportions)
    Vec3 upperArmOverlap2 = {xOffset - 0.32f, 0.18f, -9.4f};  // Upper arm positioning (mirrored)
    drawAnatomicalArmSegment(upperArmOverlap2, g_ArmJoints2[5].position,
                           1.3f, 1.0f,    // 130% of palm width (realistic upper arm)
                           1.1f,          // Major muscle groups (biceps/triceps)
                           0.8f);         // Humerus bone
    
    // Draw palm-proportioned joints with realistic scaling and texture
    if (g_TextureEnabled) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, g_HandTexture);
        glColor3f(1.0f, 1.0f, 1.0f);
    } else {
        glDisable(GL_TEXTURE_2D);
    }
    
    // Joint sizing based on palm proportions (palm = 1.0f reference)
    for (int joint = 1; joint < 6; joint++) {
        Vec3 pos = g_ArmJoints2[joint].position;
        float palmRatio = 0.15f; // 15% of palm width as base joint size
        
        // Joint core (bone/cartilage) - proportional to palm
        if (!g_TextureEnabled) glColor3f(0.8f, 0.8f, 0.8f);
        drawRobotJoint(pos, palmRatio + joint * 0.02f);
        
        // Joint capsule (ligaments/tendons) - slightly larger
        if (!g_TextureEnabled) glColor3f(0.7f, 0.65f, 0.6f);
        drawRobotJoint(pos, palmRatio * 1.5f + joint * 0.025f);
        
        // Joint housing (external structure) - palm-fitted
        if (!g_TextureEnabled) glColor3f(0.65f, 0.65f, 0.65f);
        drawRobotJoint(pos, palmRatio * 2.0f + joint * 0.03f);
    }
    
    // Add anatomical details scaled to palm size with texture
    if (g_TextureEnabled) {
        glColor3f(0.8f, 0.8f, 0.8f); // Lighter for textured tendons
    } else {
        glColor3f(0.5f, 0.5f, 0.6f); // Tendon/nerve color
    }
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    
    // Draw tendon lines proportional to palm width
    for (int i = 1; i < 5; i++) {
        Vec3 start = g_ArmJoints2[i].position;
        Vec3 end = g_ArmJoints2[i + 1].position;
        
        // Main tendon line (20% of palm width offset)
        float tendonOffset = 0.2f; // 20% of palm width
        glVertex3f(start.x + tendonOffset, start.y, start.z);
        glVertex3f(end.x + tendonOffset, end.y, end.z);
        
        // Secondary tendon line
        glVertex3f(start.x - tendonOffset, start.y, start.z);
        glVertex3f(end.x - tendonOffset, end.y, end.z);
    }
    
    glEnd();
    glLineWidth(1.0f);
}

// Draw palm with complete polygonal coverage like a real hand for hand 1
static void drawSkinnedPalm() {
    Vec3 wrist = g_HandJoints[0].position;
    Vec3 thumbBase = g_HandJoints[1].position;
    Vec3 indexBase = g_HandJoints[5].position;
    Vec3 middleBase = g_HandJoints[9].position;
    Vec3 ringBase = g_HandJoints[13].position;
    Vec3 pinkyBase = g_HandJoints[17].position;
    
    // COMPREHENSIVE PALM TOPOLOGY - DENSE MESH for complete coverage visualization
    
    // Base vertices (wrist area) - wider for natural proportions
    Vec3 palmBase1 = {wrist.x - 0.5f, wrist.y, wrist.z - 0.1f};    // Pinky side base
    Vec3 palmBase2 = {wrist.x + 0.5f, wrist.y, wrist.z - 0.1f};    // Thumb side base  
    Vec3 palmBase3 = {wrist.x - 0.16f, wrist.y, wrist.z - 0.1f};   // Center-left base
    Vec3 palmBase4 = {wrist.x + 0.16f, wrist.y, wrist.z - 0.1f};   // Center-right base
    Vec3 palmBase5 = {wrist.x, wrist.y, wrist.z - 0.1f};           // Center base
    
    // Lower palm vertices (first elevation) - more subdivisions
    Vec3 palmLow1 = {wrist.x - 0.45f, wrist.y + 0.12f, wrist.z + 0.15f};  // Pinky side low
    Vec3 palmLow2 = {wrist.x + 0.45f, wrist.y + 0.12f, wrist.z + 0.15f};  // Thumb side low
    Vec3 palmLow3 = {wrist.x - 0.25f, wrist.y + 0.12f, wrist.z + 0.15f};  // Left-center low
    Vec3 palmLow4 = {wrist.x - 0.08f, wrist.y + 0.12f, wrist.z + 0.15f};  // Center-left low
    Vec3 palmLow5 = {wrist.x + 0.08f, wrist.y + 0.12f, wrist.z + 0.15f};  // Center-right low
    Vec3 palmLow6 = {wrist.x + 0.25f, wrist.y + 0.12f, wrist.z + 0.15f};  // Right-center low
    
    // Middle palm vertices (second elevation) - dense mesh
    Vec3 palmMid1 = {wrist.x - 0.4f, wrist.y + 0.25f, wrist.z + 0.35f};   // Pinky side mid
    Vec3 palmMid2 = {wrist.x + 0.4f, wrist.y + 0.25f, wrist.z + 0.35f};   // Thumb side mid
    Vec3 palmMid3 = {wrist.x - 0.2f, wrist.y + 0.25f, wrist.z + 0.35f};   // Left-center mid
    Vec3 palmMid4 = {wrist.x - 0.06f, wrist.y + 0.25f, wrist.z + 0.35f};  // Center-left mid
    Vec3 palmMid5 = {wrist.x + 0.06f, wrist.y + 0.25f, wrist.z + 0.35f};  // Center-right mid
    Vec3 palmMid6 = {wrist.x + 0.2f, wrist.y + 0.25f, wrist.z + 0.35f};   // Right-center mid
    
    // Upper-middle palm vertices (third elevation) - additional layer for density
    Vec3 palmUpMid1 = {wrist.x - 0.35f, wrist.y + 0.4f, wrist.z + 0.55f};   // Pinky side up-mid
    Vec3 palmUpMid2 = {wrist.x + 0.35f, wrist.y + 0.4f, wrist.z + 0.55f};   // Thumb side up-mid
    Vec3 palmUpMid3 = {wrist.x - 0.15f, wrist.y + 0.4f, wrist.z + 0.55f};   // Left-center up-mid
    Vec3 palmUpMid4 = {wrist.x - 0.05f, wrist.y + 0.4f, wrist.z + 0.55f};   // Center-left up-mid
    Vec3 palmUpMid5 = {wrist.x + 0.05f, wrist.y + 0.4f, wrist.z + 0.55f};   // Center-right up-mid
    Vec3 palmUpMid6 = {wrist.x + 0.15f, wrist.y + 0.4f, wrist.z + 0.55f};   // Right-center up-mid
    
    // Upper palm vertices (knuckle area) - dense knuckle coverage
    Vec3 palmHigh1 = {pinkyBase.x - 0.05f, pinkyBase.y + 0.05f, pinkyBase.z - 0.4f};    // Pinky knuckle area
    Vec3 palmHigh2 = {ringBase.x - 0.05f, ringBase.y + 0.08f, ringBase.z - 0.4f};      // Ring knuckle area
    Vec3 palmHigh3 = {middleBase.x - 0.05f, middleBase.y + 0.1f, middleBase.z - 0.4f}; // Middle knuckle area
    Vec3 palmHigh4 = {indexBase.x - 0.05f, indexBase.y + 0.08f, indexBase.z - 0.4f};   // Index knuckle area
    Vec3 palmHigh5 = {thumbBase.x - 0.1f, thumbBase.y + 0.0f, thumbBase.z - 0.3f};     // Thumb knuckle area
    
    // Pre-knuckle vertices (between upper palm and knuckles) - smooth transition
    Vec3 preKnuckle1 = {pinkyBase.x - 0.03f, pinkyBase.y + 0.03f, pinkyBase.z - 0.25f};
    Vec3 preKnuckle2 = {ringBase.x - 0.03f, ringBase.y + 0.05f, ringBase.z - 0.25f};
    Vec3 preKnuckle3 = {middleBase.x - 0.03f, middleBase.y + 0.07f, middleBase.z - 0.25f};
    Vec3 preKnuckle4 = {indexBase.x - 0.03f, indexBase.y + 0.05f, indexBase.z - 0.25f};
    Vec3 preKnuckle5 = {thumbBase.x - 0.06f, thumbBase.y - 0.02f, thumbBase.z - 0.15f};
    
    // Finger connection vertices (direct connections to finger bases)
    Vec3 fingerConn1 = {pinkyBase.x, pinkyBase.y - 0.08f, pinkyBase.z - 0.1f};   // Pinky connection
    Vec3 fingerConn2 = {ringBase.x, ringBase.y - 0.08f, ringBase.z - 0.1f};     // Ring connection
    Vec3 fingerConn3 = {middleBase.x, middleBase.y - 0.08f, middleBase.z - 0.1f}; // Middle connection
    Vec3 fingerConn4 = {indexBase.x, indexBase.y - 0.08f, indexBase.z - 0.1f};   // Index connection
    Vec3 fingerConn5 = {thumbBase.x, thumbBase.y - 0.05f, thumbBase.z - 0.05f};  // Thumb connection
    
    // BACK OF HAND vertices (create solid volume) - Dense mesh to match front
    Vec3 backBase1 = {palmBase1.x, palmBase1.y - 0.4f, palmBase1.z};
    Vec3 backBase2 = {palmBase2.x, palmBase2.y - 0.4f, palmBase2.z};
    Vec3 backBase3 = {palmBase3.x, palmBase3.y - 0.4f, palmBase3.z};
    Vec3 backBase4 = {palmBase4.x, palmBase4.y - 0.4f, palmBase4.z};
    Vec3 backBase5 = {palmBase5.x, palmBase5.y - 0.4f, palmBase5.z};
    
    Vec3 backLow1 = {palmLow1.x, palmLow1.y - 0.4f, palmLow1.z};
    Vec3 backLow2 = {palmLow2.x, palmLow2.y - 0.4f, palmLow2.z};
    Vec3 backLow3 = {palmLow3.x, palmLow3.y - 0.4f, palmLow3.z};
    Vec3 backLow4 = {palmLow4.x, palmLow4.y - 0.4f, palmLow4.z};
    Vec3 backLow5 = {palmLow5.x, palmLow5.y - 0.4f, palmLow5.z};
    Vec3 backLow6 = {palmLow6.x, palmLow6.y - 0.4f, palmLow6.z};
    
    Vec3 backMid1 = {palmMid1.x, palmMid1.y - 0.4f, palmMid1.z};
    Vec3 backMid2 = {palmMid2.x, palmMid2.y - 0.4f, palmMid2.z};
    Vec3 backMid3 = {palmMid3.x, palmMid3.y - 0.4f, palmMid3.z};
    Vec3 backMid4 = {palmMid4.x, palmMid4.y - 0.4f, palmMid4.z};
    Vec3 backMid5 = {palmMid5.x, palmMid5.y - 0.4f, palmMid5.z};
    Vec3 backMid6 = {palmMid6.x, palmMid6.y - 0.4f, palmMid6.z};
    
    Vec3 backUpMid1 = {palmUpMid1.x, palmUpMid1.y - 0.4f, palmUpMid1.z};
    Vec3 backUpMid2 = {palmUpMid2.x, palmUpMid2.y - 0.4f, palmUpMid2.z};
    Vec3 backUpMid3 = {palmUpMid3.x, palmUpMid3.y - 0.4f, palmUpMid3.z};
    Vec3 backUpMid4 = {palmUpMid4.x, palmUpMid4.y - 0.4f, palmUpMid4.z};
    Vec3 backUpMid5 = {palmUpMid5.x, palmUpMid5.y - 0.4f, palmUpMid5.z};
    Vec3 backUpMid6 = {palmUpMid6.x, palmUpMid6.y - 0.4f, palmUpMid6.z};
    
    Vec3 backHigh1 = {palmHigh1.x, palmHigh1.y - 0.4f, palmHigh1.z};
    Vec3 backHigh2 = {palmHigh2.x, palmHigh2.y - 0.4f, palmHigh2.z};
    Vec3 backHigh3 = {palmHigh3.x, palmHigh3.y - 0.4f, palmHigh3.z};
    Vec3 backHigh4 = {palmHigh4.x, palmHigh4.y - 0.4f, palmHigh4.z};
    Vec3 backHigh5 = {palmHigh5.x, palmHigh5.y - 0.4f, palmHigh5.z};
    
    Vec3 backPreKnuckle1 = {preKnuckle1.x, preKnuckle1.y - 0.4f, preKnuckle1.z};
    Vec3 backPreKnuckle2 = {preKnuckle2.x, preKnuckle2.y - 0.4f, preKnuckle2.z};
    Vec3 backPreKnuckle3 = {preKnuckle3.x, preKnuckle3.y - 0.4f, preKnuckle3.z};
    Vec3 backPreKnuckle4 = {preKnuckle4.x, preKnuckle4.y - 0.4f, preKnuckle4.z};
    Vec3 backPreKnuckle5 = {preKnuckle5.x, preKnuckle5.y - 0.4f, preKnuckle5.z};
    
    // START DRAWING DENSE PALM TOPOLOGY - Maximum polygon coverage
    glBegin(GL_TRIANGLES);
    
    // PALM SURFACE (top) - DENSE MESH for complete visualization
    Vec3 normalUp = {0, 1, 0};
    glNormal3f(normalUp.x, normalUp.y, normalUp.z);
    
    // DENSE LAYER 1: Base to Low palm (6x6 subdivisions)
    // Row 1: Left sections
    glTexCoord2f(0.0f, 0.0f); glVertex3f(palmBase1.x, palmBase1.y, palmBase1.z);
    glTexCoord2f(0.2f, 0.0f); glVertex3f(palmBase3.x, palmBase3.y, palmBase3.z);
    glTexCoord2f(0.2f, 0.16f); glVertex3f(palmLow3.x, palmLow3.y, palmLow3.z);
    
    glTexCoord2f(0.2f, 0.16f); glVertex3f(palmLow3.x, palmLow3.y, palmLow3.z);
    glTexCoord2f(0.0f, 0.16f); glVertex3f(palmLow1.x, palmLow1.y, palmLow1.z);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(palmBase1.x, palmBase1.y, palmBase1.z);
    
    glTexCoord2f(0.2f, 0.0f); glVertex3f(palmBase3.x, palmBase3.y, palmBase3.z);
    glTexCoord2f(0.35f, 0.0f); glVertex3f(palmBase5.x, palmBase5.y, palmBase5.z);
    glTexCoord2f(0.35f, 0.16f); glVertex3f(palmLow4.x, palmLow4.y, palmLow4.z);
    
    glTexCoord2f(0.35f, 0.16f); glVertex3f(palmLow4.x, palmLow4.y, palmLow4.z);
    glTexCoord2f(0.2f, 0.16f); glVertex3f(palmLow3.x, palmLow3.y, palmLow3.z);
    glTexCoord2f(0.2f, 0.0f); glVertex3f(palmBase3.x, palmBase3.y, palmBase3.z);
    
    glTexCoord2f(0.35f, 0.0f); glVertex3f(palmBase5.x, palmBase5.y, palmBase5.z);
    glTexCoord2f(0.5f, 0.0f); glVertex3f(palmBase4.x, palmBase4.y, palmBase4.z);
    glTexCoord2f(0.5f, 0.16f); glVertex3f(palmLow5.x, palmLow5.y, palmLow5.z);
    
    glTexCoord2f(0.5f, 0.16f); glVertex3f(palmLow5.x, palmLow5.y, palmLow5.z);
    glTexCoord2f(0.35f, 0.16f); glVertex3f(palmLow4.x, palmLow4.y, palmLow4.z);
    glTexCoord2f(0.35f, 0.0f); glVertex3f(palmBase5.x, palmBase5.y, palmBase5.z);
    
    glTexCoord2f(0.5f, 0.0f); glVertex3f(palmBase4.x, palmBase4.y, palmBase4.z);
    glTexCoord2f(0.8f, 0.0f); glVertex3f(palmBase2.x, palmBase2.y, palmBase2.z);
    glTexCoord2f(0.8f, 0.16f); glVertex3f(palmLow6.x, palmLow6.y, palmLow6.z);
    
    glTexCoord2f(0.8f, 0.16f); glVertex3f(palmLow6.x, palmLow6.y, palmLow6.z);
    glTexCoord2f(0.5f, 0.16f); glVertex3f(palmLow5.x, palmLow5.y, palmLow5.z);
    glTexCoord2f(0.5f, 0.0f); glVertex3f(palmBase4.x, palmBase4.y, palmBase4.z);
    
    glTexCoord2f(0.8f, 0.0f); glVertex3f(palmBase2.x, palmBase2.y, palmBase2.z);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(palmBase2.x, palmBase2.y, palmBase2.z);
    glTexCoord2f(1.0f, 0.16f); glVertex3f(palmLow2.x, palmLow2.y, palmLow2.z);
    
    glTexCoord2f(1.0f, 0.16f); glVertex3f(palmLow2.x, palmLow2.y, palmLow2.z);
    glTexCoord2f(0.8f, 0.16f); glVertex3f(palmLow6.x, palmLow6.y, palmLow6.z);
    glTexCoord2f(0.8f, 0.0f); glVertex3f(palmBase2.x, palmBase2.y, palmBase2.z);
    
    // DENSE LAYER 2: Low to Mid palm (6x6 subdivisions)
    glTexCoord2f(0.0f, 0.16f); glVertex3f(palmLow1.x, palmLow1.y, palmLow1.z);
    glTexCoord2f(0.2f, 0.16f); glVertex3f(palmLow3.x, palmLow3.y, palmLow3.z);
    glTexCoord2f(0.2f, 0.33f); glVertex3f(palmMid3.x, palmMid3.y, palmMid3.z);
    
    glTexCoord2f(0.2f, 0.33f); glVertex3f(palmMid3.x, palmMid3.y, palmMid3.z);
    glTexCoord2f(0.0f, 0.33f); glVertex3f(palmMid1.x, palmMid1.y, palmMid1.z);
    glTexCoord2f(0.0f, 0.16f); glVertex3f(palmLow1.x, palmLow1.y, palmLow1.z);
    
    glTexCoord2f(0.2f, 0.16f); glVertex3f(palmLow3.x, palmLow3.y, palmLow3.z);
    glTexCoord2f(0.35f, 0.16f); glVertex3f(palmLow4.x, palmLow4.y, palmLow4.z);
    glTexCoord2f(0.35f, 0.33f); glVertex3f(palmMid4.x, palmMid4.y, palmMid4.z);
    
    glTexCoord2f(0.35f, 0.33f); glVertex3f(palmMid4.x, palmMid4.y, palmMid4.z);
    glTexCoord2f(0.2f, 0.33f); glVertex3f(palmMid3.x, palmMid3.y, palmMid3.z);
    glTexCoord2f(0.2f, 0.16f); glVertex3f(palmLow3.x, palmLow3.y, palmLow3.z);
    
    glTexCoord2f(0.35f, 0.16f); glVertex3f(palmLow4.x, palmLow4.y, palmLow4.z);
    glTexCoord2f(0.5f, 0.16f); glVertex3f(palmLow5.x, palmLow5.y, palmLow5.z);
    glTexCoord2f(0.5f, 0.33f); glVertex3f(palmMid5.x, palmMid5.y, palmMid5.z);
    
    glTexCoord2f(0.5f, 0.33f); glVertex3f(palmMid5.x, palmMid5.y, palmMid5.z);
    glTexCoord2f(0.35f, 0.33f); glVertex3f(palmMid4.x, palmMid4.y, palmMid4.z);
    glTexCoord2f(0.35f, 0.16f); glVertex3f(palmLow4.x, palmLow4.y, palmLow4.z);
    
    glTexCoord2f(0.5f, 0.16f); glVertex3f(palmLow5.x, palmLow5.y, palmLow5.z);
    glTexCoord2f(0.8f, 0.16f); glVertex3f(palmLow6.x, palmLow6.y, palmLow6.z);
    glTexCoord2f(0.8f, 0.33f); glVertex3f(palmMid6.x, palmMid6.y, palmMid6.z);
    
    glTexCoord2f(0.8f, 0.33f); glVertex3f(palmMid6.x, palmMid6.y, palmMid6.z);
    glTexCoord2f(0.5f, 0.33f); glVertex3f(palmMid5.x, palmMid5.y, palmMid5.z);
    glTexCoord2f(0.5f, 0.16f); glVertex3f(palmLow5.x, palmLow5.y, palmLow5.z);
    
    glTexCoord2f(0.8f, 0.16f); glVertex3f(palmLow6.x, palmLow6.y, palmLow6.z);
    glTexCoord2f(1.0f, 0.16f); glVertex3f(palmLow2.x, palmLow2.y, palmLow2.z);
    glTexCoord2f(1.0f, 0.33f); glVertex3f(palmMid2.x, palmMid2.y, palmMid2.z);
    
    glTexCoord2f(1.0f, 0.33f); glVertex3f(palmMid2.x, palmMid2.y, palmMid2.z);
    glTexCoord2f(0.8f, 0.33f); glVertex3f(palmMid6.x, palmMid6.y, palmMid6.z);
    glTexCoord2f(0.8f, 0.16f); glVertex3f(palmLow6.x, palmLow6.y, palmLow6.z);
    
    // DENSE LAYER 3: Mid to UpMid palm (6x6 subdivisions)
    glTexCoord2f(0.0f, 0.33f); glVertex3f(palmMid1.x, palmMid1.y, palmMid1.z);
    glTexCoord2f(0.2f, 0.33f); glVertex3f(palmMid3.x, palmMid3.y, palmMid3.z);
    glTexCoord2f(0.2f, 0.5f); glVertex3f(palmUpMid3.x, palmUpMid3.y, palmUpMid3.z);
    
    glTexCoord2f(0.2f, 0.5f); glVertex3f(palmUpMid3.x, palmUpMid3.y, palmUpMid3.z);
    glTexCoord2f(0.0f, 0.5f); glVertex3f(palmUpMid1.x, palmUpMid1.y, palmUpMid1.z);
    glTexCoord2f(0.0f, 0.33f); glVertex3f(palmMid1.x, palmMid1.y, palmMid1.z);
    
    glTexCoord2f(0.2f, 0.33f); glVertex3f(palmMid3.x, palmMid3.y, palmMid3.z);
    glTexCoord2f(0.35f, 0.33f); glVertex3f(palmMid4.x, palmMid4.y, palmMid4.z);
    glTexCoord2f(0.35f, 0.5f); glVertex3f(palmUpMid4.x, palmUpMid4.y, palmUpMid4.z);
    
    glTexCoord2f(0.35f, 0.5f); glVertex3f(palmUpMid4.x, palmUpMid4.y, palmUpMid4.z);
    glTexCoord2f(0.2f, 0.5f); glVertex3f(palmUpMid3.x, palmUpMid3.y, palmUpMid3.z);
    glTexCoord2f(0.2f, 0.33f); glVertex3f(palmMid3.x, palmMid3.y, palmMid3.z);
    
    // LAYER 3: Mid to High palm (knuckle areas)
    glTexCoord2f(0.0f, 0.5f); glVertex3f(palmMid1.x, palmMid1.y, palmMid1.z);
    glTexCoord2f(0.2f, 0.75f); glVertex3f(palmHigh1.x, palmHigh1.y, palmHigh1.z);
    glTexCoord2f(0.4f, 0.75f); glVertex3f(palmHigh2.x, palmHigh2.y, palmHigh2.z);
    
    glTexCoord2f(0.4f, 0.75f); glVertex3f(palmHigh2.x, palmHigh2.y, palmHigh2.z);
    glTexCoord2f(0.33f, 0.5f); glVertex3f(palmMid3.x, palmMid3.y, palmMid3.z);
    glTexCoord2f(0.0f, 0.5f); glVertex3f(palmMid1.x, palmMid1.y, palmMid1.z);
    
    glTexCoord2f(0.33f, 0.5f); glVertex3f(palmMid3.x, palmMid3.y, palmMid3.z);
    glTexCoord2f(0.4f, 0.75f); glVertex3f(palmHigh2.x, palmHigh2.y, palmHigh2.z);
    glTexCoord2f(0.6f, 0.75f); glVertex3f(palmHigh3.x, palmHigh3.y, palmHigh3.z);
    
    glTexCoord2f(0.6f, 0.75f); glVertex3f(palmHigh3.x, palmHigh3.y, palmHigh3.z);
    glTexCoord2f(0.66f, 0.5f); glVertex3f(palmMid4.x, palmMid4.y, palmMid4.z);
    glTexCoord2f(0.33f, 0.5f); glVertex3f(palmMid3.x, palmMid3.y, palmMid3.z);
    
    glTexCoord2f(0.66f, 0.5f); glVertex3f(palmMid4.x, palmMid4.y, palmMid4.z);
    glTexCoord2f(0.6f, 0.75f); glVertex3f(palmHigh3.x, palmHigh3.y, palmHigh3.z);
    glTexCoord2f(0.8f, 0.75f); glVertex3f(palmHigh4.x, palmHigh4.y, palmHigh4.z);
    
    glTexCoord2f(0.8f, 0.75f); glVertex3f(palmHigh4.x, palmHigh4.y, palmHigh4.z);
    glTexCoord2f(1.0f, 0.5f); glVertex3f(palmMid2.x, palmMid2.y, palmMid2.z);
    glTexCoord2f(0.66f, 0.5f); glVertex3f(palmMid4.x, palmMid4.y, palmMid4.z);
    
    // Thumb area connection
    glTexCoord2f(0.8f, 0.75f); glVertex3f(palmHigh4.x, palmHigh4.y, palmHigh4.z);
    glTexCoord2f(1.0f, 0.75f); glVertex3f(palmHigh5.x, palmHigh5.y, palmHigh5.z);
    glTexCoord2f(1.0f, 0.5f); glVertex3f(palmMid2.x, palmMid2.y, palmMid2.z);
    
    // LAYER 4: High palm to finger connections
    glTexCoord2f(0.2f, 0.75f); glVertex3f(palmHigh1.x, palmHigh1.y, palmHigh1.z);
    glTexCoord2f(0.2f, 1.0f); glVertex3f(fingerConn1.x, fingerConn1.y, fingerConn1.z);
    glTexCoord2f(0.4f, 1.0f); glVertex3f(fingerConn2.x, fingerConn2.y, fingerConn2.z);
    
    glTexCoord2f(0.4f, 1.0f); glVertex3f(fingerConn2.x, fingerConn2.y, fingerConn2.z);
    glTexCoord2f(0.4f, 0.75f); glVertex3f(palmHigh2.x, palmHigh2.y, palmHigh2.z);
    glTexCoord2f(0.2f, 0.75f); glVertex3f(palmHigh1.x, palmHigh1.y, palmHigh1.z);
    
    glTexCoord2f(0.4f, 0.75f); glVertex3f(palmHigh2.x, palmHigh2.y, palmHigh2.z);
    glTexCoord2f(0.4f, 1.0f); glVertex3f(fingerConn2.x, fingerConn2.y, fingerConn2.z);
    glTexCoord2f(0.6f, 1.0f); glVertex3f(fingerConn3.x, fingerConn3.y, fingerConn3.z);
    
    glTexCoord2f(0.6f, 1.0f); glVertex3f(fingerConn3.x, fingerConn3.y, fingerConn3.z);
    glTexCoord2f(0.6f, 0.75f); glVertex3f(palmHigh3.x, palmHigh3.y, palmHigh3.z);
    glTexCoord2f(0.4f, 0.75f); glVertex3f(palmHigh2.x, palmHigh2.y, palmHigh2.z);
    
    glTexCoord2f(0.6f, 0.75f); glVertex3f(palmHigh3.x, palmHigh3.y, palmHigh3.z);
    glTexCoord2f(0.6f, 1.0f); glVertex3f(fingerConn3.x, fingerConn3.y, fingerConn3.z);
    glTexCoord2f(0.8f, 1.0f); glVertex3f(fingerConn4.x, fingerConn4.y, fingerConn4.z);
    
    glTexCoord2f(0.8f, 1.0f); glVertex3f(fingerConn4.x, fingerConn4.y, fingerConn4.z);
    glTexCoord2f(0.8f, 0.75f); glVertex3f(palmHigh4.x, palmHigh4.y, palmHigh4.z);
    glTexCoord2f(0.6f, 0.75f); glVertex3f(palmHigh3.x, palmHigh3.y, palmHigh3.z);
    
    glTexCoord2f(0.8f, 0.75f); glVertex3f(palmHigh4.x, palmHigh4.y, palmHigh4.z);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(fingerConn5.x, fingerConn5.y, fingerConn5.z);
    glTexCoord2f(1.0f, 0.75f); glVertex3f(palmHigh5.x, palmHigh5.y, palmHigh5.z);
    
    // LAYER 5: Finger connections to actual finger bases
    Vec3 normalConnect = {0, 0.6f, 0.8f};
    glNormal3f(normalConnect.x, normalConnect.y, normalConnect.z);
    
    glTexCoord2f(0.2f, 1.0f); glVertex3f(fingerConn1.x, fingerConn1.y, fingerConn1.z);
    glTexCoord2f(0.2f, 1.0f); glVertex3f(pinkyBase.x, pinkyBase.y, pinkyBase.z);
    glTexCoord2f(0.4f, 1.0f); glVertex3f(ringBase.x, ringBase.y, ringBase.z);
    
    glTexCoord2f(0.4f, 1.0f); glVertex3f(ringBase.x, ringBase.y, ringBase.z);
    glTexCoord2f(0.4f, 1.0f); glVertex3f(fingerConn2.x, fingerConn2.y, fingerConn2.z);
    glTexCoord2f(0.2f, 1.0f); glVertex3f(fingerConn1.x, fingerConn1.y, fingerConn1.z);
    
    glTexCoord2f(0.4f, 1.0f); glVertex3f(fingerConn2.x, fingerConn2.y, fingerConn2.z);
    glTexCoord2f(0.4f, 1.0f); glVertex3f(ringBase.x, ringBase.y, ringBase.z);
    glTexCoord2f(0.6f, 1.0f); glVertex3f(middleBase.x, middleBase.y, middleBase.z);
    
    glTexCoord2f(0.6f, 1.0f); glVertex3f(middleBase.x, middleBase.y, middleBase.z);
    glTexCoord2f(0.6f, 1.0f); glVertex3f(fingerConn3.x, fingerConn3.y, fingerConn3.z);
    glTexCoord2f(0.4f, 1.0f); glVertex3f(fingerConn2.x, fingerConn2.y, fingerConn2.z);
    
    glTexCoord2f(0.6f, 1.0f); glVertex3f(fingerConn3.x, fingerConn3.y, fingerConn3.z);
    glTexCoord2f(0.6f, 1.0f); glVertex3f(middleBase.x, middleBase.y, middleBase.z);
    glTexCoord2f(0.8f, 1.0f); glVertex3f(indexBase.x, indexBase.y, indexBase.z);
    
    glTexCoord2f(0.8f, 1.0f); glVertex3f(indexBase.x, indexBase.y, indexBase.z);
    glTexCoord2f(0.8f, 1.0f); glVertex3f(fingerConn4.x, fingerConn4.y, fingerConn4.z);
    glTexCoord2f(0.6f, 1.0f); glVertex3f(fingerConn3.x, fingerConn3.y, fingerConn3.z);
    
    glTexCoord2f(0.8f, 1.0f); glVertex3f(fingerConn4.x, fingerConn4.y, fingerConn4.z);
    glTexCoord2f(0.8f, 1.0f); glVertex3f(indexBase.x, indexBase.y, indexBase.z);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(thumbBase.x, thumbBase.y, thumbBase.z);
    
    glTexCoord2f(1.0f, 1.0f); glVertex3f(thumbBase.x, thumbBase.y, thumbBase.z);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(fingerConn5.x, fingerConn5.y, fingerConn5.z);
    glTexCoord2f(0.8f, 1.0f); glVertex3f(fingerConn4.x, fingerConn4.y, fingerConn4.z);
    
    // BACK OF HAND SURFACE (bottom) - Complete coverage with layered topology
    Vec3 normalDown = {0, -1, 0};
    glNormal3f(normalDown.x, normalDown.y, normalDown.z);
    
    // Mirror the top topology on the bottom for solid volume
    // Base to Low back layer
    glTexCoord2f(0.0f, 0.0f); glVertex3f(backBase1.x, backBase1.y, backBase1.z);
    glTexCoord2f(0.0f, 0.25f); glVertex3f(backLow1.x, backLow1.y, backLow1.z);
    glTexCoord2f(0.33f, 0.25f); glVertex3f(backLow3.x, backLow3.y, backLow3.z);
    
    glTexCoord2f(0.33f, 0.25f); glVertex3f(backLow3.x, backLow3.y, backLow3.z);
    glTexCoord2f(0.33f, 0.0f); glVertex3f(backBase3.x, backBase3.y, backBase3.z);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(backBase1.x, backBase1.y, backBase1.z);
    
    glTexCoord2f(0.33f, 0.0f); glVertex3f(backBase3.x, backBase3.y, backBase3.z);
    glTexCoord2f(0.33f, 0.25f); glVertex3f(backLow3.x, backLow3.y, backLow3.z);
    glTexCoord2f(0.66f, 0.25f); glVertex3f(backLow4.x, backLow4.y, backLow4.z);
    
    glTexCoord2f(0.66f, 0.25f); glVertex3f(backLow4.x, backLow4.y, backLow4.z);
    glTexCoord2f(0.66f, 0.0f); glVertex3f(backBase2.x, backBase2.y, backBase2.z);
    glTexCoord2f(0.33f, 0.0f); glVertex3f(backBase3.x, backBase3.y, backBase3.z);
    
    glTexCoord2f(0.66f, 0.0f); glVertex3f(backBase2.x, backBase2.y, backBase2.z);
    glTexCoord2f(0.66f, 0.25f); glVertex3f(backLow4.x, backLow4.y, backLow4.z);
    glTexCoord2f(1.0f, 0.25f); glVertex3f(backLow2.x, backLow2.y, backLow2.z);
    
    glTexCoord2f(1.0f, 0.25f); glVertex3f(backLow2.x, backLow2.y, backLow2.z);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(backBase2.x, backBase2.y, backBase2.z);
    glTexCoord2f(0.66f, 0.0f); glVertex3f(backBase2.x, backBase2.y, backBase2.z);
    
    // Low to Mid back layer
    glTexCoord2f(0.0f, 0.25f); glVertex3f(backLow1.x, backLow1.y, backLow1.z);
    glTexCoord2f(0.0f, 0.5f); glVertex3f(backMid1.x, backMid1.y, backMid1.z);
    glTexCoord2f(0.33f, 0.5f); glVertex3f(backMid3.x, backMid3.y, backMid3.z);
    
    glTexCoord2f(0.33f, 0.5f); glVertex3f(backMid3.x, backMid3.y, backMid3.z);
    glTexCoord2f(0.33f, 0.25f); glVertex3f(backLow3.x, backLow3.y, backLow3.z);
    glTexCoord2f(0.0f, 0.25f); glVertex3f(backLow1.x, backLow1.y, backLow1.z);
    
    glTexCoord2f(0.33f, 0.25f); glVertex3f(backLow3.x, backLow3.y, backLow3.z);
    glTexCoord2f(0.33f, 0.5f); glVertex3f(backMid3.x, backMid3.y, backMid3.z);
    glTexCoord2f(0.66f, 0.5f); glVertex3f(backMid4.x, backMid4.y, backMid4.z);
    
    glTexCoord2f(0.66f, 0.5f); glVertex3f(backMid4.x, backMid4.y, backMid4.z);
    glTexCoord2f(0.66f, 0.25f); glVertex3f(backLow4.x, backLow4.y, backLow4.z);
    glTexCoord2f(0.33f, 0.25f); glVertex3f(backLow3.x, backLow3.y, backLow3.z);
    
    glTexCoord2f(0.66f, 0.25f); glVertex3f(backLow4.x, backLow4.y, backLow4.z);
    glTexCoord2f(0.66f, 0.5f); glVertex3f(backMid4.x, backMid4.y, backMid4.z);
    glTexCoord2f(1.0f, 0.5f); glVertex3f(backMid2.x, backMid2.y, backMid2.z);
    
    glTexCoord2f(1.0f, 0.5f); glVertex3f(backMid2.x, backMid2.y, backMid2.z);
    glTexCoord2f(1.0f, 0.25f); glVertex3f(backLow2.x, backLow2.y, backLow2.z);
    glTexCoord2f(0.66f, 0.25f); glVertex3f(backLow4.x, backLow4.y, backLow4.z);
    
    // Mid to High back layer (knuckles area)
    glTexCoord2f(0.0f, 0.5f); glVertex3f(backMid1.x, backMid1.y, backMid1.z);
    glTexCoord2f(0.2f, 0.75f); glVertex3f(backHigh1.x, backHigh1.y, backHigh1.z);
    glTexCoord2f(0.33f, 0.5f); glVertex3f(backMid3.x, backMid3.y, backMid3.z);
    
    glTexCoord2f(0.33f, 0.5f); glVertex3f(backMid3.x, backMid3.y, backMid3.z);
    glTexCoord2f(0.2f, 0.75f); glVertex3f(backHigh1.x, backHigh1.y, backHigh1.z);
    glTexCoord2f(0.4f, 0.75f); glVertex3f(backHigh2.x, backHigh2.y, backHigh2.z);
    
    glTexCoord2f(0.33f, 0.5f); glVertex3f(backMid3.x, backMid3.y, backMid3.z);
    glTexCoord2f(0.4f, 0.75f); glVertex3f(backHigh2.x, backHigh2.y, backHigh2.z);
    glTexCoord2f(0.6f, 0.75f); glVertex3f(backHigh3.x, backHigh3.y, backHigh3.z);
    
    glTexCoord2f(0.6f, 0.75f); glVertex3f(backHigh3.x, backHigh3.y, backHigh3.z);
    glTexCoord2f(0.66f, 0.5f); glVertex3f(backMid4.x, backMid4.y, backMid4.z);
    glTexCoord2f(0.33f, 0.5f); glVertex3f(backMid3.x, backMid3.y, backMid3.z);
    
    glTexCoord2f(0.66f, 0.5f); glVertex3f(backMid4.x, backMid4.y, backMid4.z);
    glTexCoord2f(0.6f, 0.75f); glVertex3f(backHigh3.x, backHigh3.y, backHigh3.z);
    glTexCoord2f(0.8f, 0.75f); glVertex3f(backHigh4.x, backHigh4.y, backHigh4.z);
    
    glTexCoord2f(0.8f, 0.75f); glVertex3f(backHigh4.x, backHigh4.y, backHigh4.z);
    glTexCoord2f(1.0f, 0.5f); glVertex3f(backMid2.x, backMid2.y, backMid2.z);
    glTexCoord2f(0.66f, 0.5f); glVertex3f(backMid4.x, backMid4.y, backMid4.z);
    
    glTexCoord2f(0.8f, 0.75f); glVertex3f(backHigh4.x, backHigh4.y, backHigh4.z);
    glTexCoord2f(1.0f, 0.75f); glVertex3f(backHigh5.x, backHigh5.y, backHigh5.z);
    glTexCoord2f(1.0f, 0.5f); glVertex3f(backMid2.x, backMid2.y, backMid2.z);
    
    // SIDE WALLS - Connect top and bottom surfaces for solid volume
    Vec3 normalSide;
    
    // Left side wall (pinky side)
    normalSide = {-1, 0, 0};
    glNormal3f(normalSide.x, normalSide.y, normalSide.z);
    
    glTexCoord2f(0.0f, 0.0f); glVertex3f(palmBase1.x, palmBase1.y, palmBase1.z);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(backBase1.x, backBase1.y, backBase1.z);
    glTexCoord2f(0.0f, 0.25f); glVertex3f(backLow1.x, backLow1.y, backLow1.z);
    
    glTexCoord2f(0.0f, 0.25f); glVertex3f(backLow1.x, backLow1.y, backLow1.z);
    glTexCoord2f(0.0f, 0.25f); glVertex3f(palmLow1.x, palmLow1.y, palmLow1.z);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(palmBase1.x, palmBase1.y, palmBase1.z);
    
    glTexCoord2f(0.0f, 0.25f); glVertex3f(palmLow1.x, palmLow1.y, palmLow1.z);
    glTexCoord2f(0.0f, 0.25f); glVertex3f(backLow1.x, backLow1.y, backLow1.z);
    glTexCoord2f(0.0f, 0.5f); glVertex3f(backMid1.x, backMid1.y, backMid1.z);
    
    glTexCoord2f(0.0f, 0.5f); glVertex3f(backMid1.x, backMid1.y, backMid1.z);
    glTexCoord2f(0.0f, 0.5f); glVertex3f(palmMid1.x, palmMid1.y, palmMid1.z);
    glTexCoord2f(0.0f, 0.25f); glVertex3f(palmLow1.x, palmLow1.y, palmLow1.z);
    
    glTexCoord2f(0.0f, 0.5f); glVertex3f(palmMid1.x, palmMid1.y, palmMid1.z);
    glTexCoord2f(0.0f, 0.5f); glVertex3f(backMid1.x, backMid1.y, backMid1.z);
    glTexCoord2f(0.2f, 0.75f); glVertex3f(backHigh1.x, backHigh1.y, backHigh1.z);
    
    glTexCoord2f(0.2f, 0.75f); glVertex3f(backHigh1.x, backHigh1.y, backHigh1.z);
    glTexCoord2f(0.2f, 0.75f); glVertex3f(palmHigh1.x, palmHigh1.y, palmHigh1.z);
    glTexCoord2f(0.0f, 0.5f); glVertex3f(palmMid1.x, palmMid1.y, palmMid1.z);
    
    // Right side wall (thumb side)
    normalSide = {1, 0, 0};
    glNormal3f(normalSide.x, normalSide.y, normalSide.z);
    
    glTexCoord2f(1.0f, 0.0f); glVertex3f(palmBase2.x, palmBase2.y, palmBase2.z);
    glTexCoord2f(1.0f, 0.25f); glVertex3f(palmLow2.x, palmLow2.y, palmLow2.z);
    glTexCoord2f(1.0f, 0.25f); glVertex3f(backLow2.x, backLow2.y, backLow2.z);
    
    glTexCoord2f(1.0f, 0.25f); glVertex3f(backLow2.x, backLow2.y, backLow2.z);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(backBase2.x, backBase2.y, backBase2.z);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(palmBase2.x, palmBase2.y, palmBase2.z);
    
    glTexCoord2f(1.0f, 0.25f); glVertex3f(palmLow2.x, palmLow2.y, palmLow2.z);
    glTexCoord2f(1.0f, 0.5f); glVertex3f(palmMid2.x, palmMid2.y, palmMid2.z);
    glTexCoord2f(1.0f, 0.5f); glVertex3f(backMid2.x, backMid2.y, backMid2.z);
    
    glTexCoord2f(1.0f, 0.5f); glVertex3f(backMid2.x, backMid2.y, backMid2.z);
    glTexCoord2f(1.0f, 0.25f); glVertex3f(backLow2.x, backLow2.y, backLow2.z);
    glTexCoord2f(1.0f, 0.25f); glVertex3f(palmLow2.x, palmLow2.y, palmLow2.z);
    
    glTexCoord2f(1.0f, 0.5f); glVertex3f(palmMid2.x, palmMid2.y, palmMid2.z);
    glTexCoord2f(1.0f, 0.75f); glVertex3f(palmHigh5.x, palmHigh5.y, palmHigh5.z);
    glTexCoord2f(1.0f, 0.75f); glVertex3f(backHigh5.x, backHigh5.y, backHigh5.z);
    
    glTexCoord2f(1.0f, 0.75f); glVertex3f(backHigh5.x, backHigh5.y, backHigh5.z);
    glTexCoord2f(1.0f, 0.5f); glVertex3f(backMid2.x, backMid2.y, backMid2.z);
    glTexCoord2f(1.0f, 0.5f); glVertex3f(palmMid2.x, palmMid2.y, palmMid2.z);
    
    // Front wall (wrist edge)
    normalSide = {0, 0, -1};
    glNormal3f(normalSide.x, normalSide.y, normalSide.z);
    
    glTexCoord2f(0.0f, 0.0f); glVertex3f(palmBase1.x, palmBase1.y, palmBase1.z);
    glTexCoord2f(0.5f, 0.0f); glVertex3f(palmBase3.x, palmBase3.y, palmBase3.z);
    glTexCoord2f(0.5f, 0.0f); glVertex3f(backBase3.x, backBase3.y, backBase3.z);
    
    glTexCoord2f(0.5f, 0.0f); glVertex3f(backBase3.x, backBase3.y, backBase3.z);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(backBase1.x, backBase1.y, backBase1.z);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(palmBase1.x, palmBase1.y, palmBase1.z);
    
    glTexCoord2f(0.5f, 0.0f); glVertex3f(palmBase3.x, palmBase3.y, palmBase3.z);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(palmBase2.x, palmBase2.y, palmBase2.z);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(backBase2.x, backBase2.y, backBase2.z);
    
    glTexCoord2f(1.0f, 0.0f); glVertex3f(backBase2.x, backBase2.y, backBase2.z);
    glTexCoord2f(0.5f, 0.0f); glVertex3f(backBase3.x, backBase3.y, backBase3.z);
    glTexCoord2f(0.5f, 0.0f); glVertex3f(palmBase3.x, palmBase3.y, palmBase3.z);
    
    // INTEGRATED PALM SURFACE - Natural hand skin feel
    Vec3 normalPalm = {0, 0.8f, 0.6f}; // More natural normal direction
    glNormal3f(normalPalm.x, normalPalm.y, normalPalm.z);
    
    // Create a naturally integrated palm surface that feels like real hand skin
    // Positions should blend seamlessly with existing hand geometry
    
    // Define integrated palm vertices (natural positioning, not layered on top)
    float naturalBlend = 0.02f; // Subtle forward positioning for natural feel
    float seamlessHeight = 0.01f; // Minimal height adjustment for seamless integration
    
    // Natural palm base (wrist area) - seamlessly integrated
    Vec3 naturalBase1 = {palmBase1.x - 0.05f, palmBase1.y + seamlessHeight, palmBase1.z + naturalBlend};
    Vec3 naturalBase2 = {palmBase2.x + 0.05f, palmBase2.y + seamlessHeight, palmBase2.z + naturalBlend};
    Vec3 naturalBase3 = {palmBase3.x - 0.02f, palmBase3.y + seamlessHeight, palmBase3.z + naturalBlend};
    Vec3 naturalBase4 = {palmBase4.x + 0.02f, palmBase4.y + seamlessHeight, palmBase4.z + naturalBlend};
    Vec3 naturalBase5 = {palmBase5.x, palmBase5.y + seamlessHeight, palmBase5.z + naturalBlend};
    
    // Natural knuckle area (where fingers connect) - integrated with finger bases
    Vec3 naturalKnuckle1 = {pinkyBase.x - 0.02f, pinkyBase.y + seamlessHeight*2, pinkyBase.z + naturalBlend*0.5f};
    Vec3 naturalKnuckle2 = {ringBase.x - 0.01f, ringBase.y + seamlessHeight*2, ringBase.z + naturalBlend*0.5f};
    Vec3 naturalKnuckle3 = {middleBase.x, middleBase.y + seamlessHeight*2, middleBase.z + naturalBlend*0.5f};
    Vec3 naturalKnuckle4 = {indexBase.x + 0.01f, indexBase.y + seamlessHeight*2, indexBase.z + naturalBlend*0.5f};
    Vec3 naturalKnuckle5 = {thumbBase.x + 0.02f, thumbBase.y + seamlessHeight*1.5f, thumbBase.z + naturalBlend*0.3f};
    
    // Natural mid-palm area - smooth transition between base and knuckles
    Vec3 naturalMid1 = {palmMid1.x - 0.03f, palmMid1.y + seamlessHeight*1.5f, palmMid1.z + naturalBlend*0.8f};
    Vec3 naturalMid2 = {palmMid2.x + 0.03f, palmMid2.y + seamlessHeight*1.5f, palmMid2.z + naturalBlend*0.8f};
    Vec3 naturalMid3 = {palmMid3.x - 0.015f, palmMid3.y + seamlessHeight*1.5f, palmMid3.z + naturalBlend*0.8f};
    Vec3 naturalMid4 = {palmMid4.x - 0.005f, palmMid4.y + seamlessHeight*1.5f, palmMid4.z + naturalBlend*0.8f};
    Vec3 naturalMid5 = {palmMid5.x + 0.005f, palmMid5.y + seamlessHeight*1.5f, palmMid5.z + naturalBlend*0.8f};
    Vec3 naturalMid6 = {palmMid6.x + 0.015f, palmMid6.y + seamlessHeight*1.5f, palmMid6.z + naturalBlend*0.8f};
    
    // SOLID FRONT SURFACE TRIANGLES - Complete coverage
    
    // Base to Mid front coverage
    // NATURAL PALM SURFACE TRIANGULATION - Seamless integration
    glColor4f(0.9f, 0.7f, 0.6f, 1.0f); // Natural hand color
    
    // Base palm surface with natural skin feel (not layered on top)
    glTexCoord2f(0.0f, 0.0f); glVertex3f(naturalBase1.x, naturalBase1.y, naturalBase1.z);
    glTexCoord2f(0.33f, 0.0f); glVertex3f(naturalBase3.x, naturalBase3.y, naturalBase3.z);
    glTexCoord2f(0.33f, 0.5f); glVertex3f(naturalMid3.x, naturalMid3.y, naturalMid3.z);
    
    glTexCoord2f(0.33f, 0.5f); glVertex3f(naturalMid3.x, naturalMid3.y, naturalMid3.z);
    glTexCoord2f(0.0f, 0.5f); glVertex3f(naturalMid1.x, naturalMid1.y, naturalMid1.z);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(naturalBase1.x, naturalBase1.y, naturalBase1.z);
    
    glTexCoord2f(0.33f, 0.0f); glVertex3f(naturalBase3.x, naturalBase3.y, naturalBase3.z);
    glTexCoord2f(0.5f, 0.0f); glVertex3f(naturalBase5.x, naturalBase5.y, naturalBase5.z);
    glTexCoord2f(0.5f, 0.5f); glVertex3f(naturalMid4.x, naturalMid4.y, naturalMid4.z);
    
    glTexCoord2f(0.5f, 0.5f); glVertex3f(naturalMid4.x, naturalMid4.y, naturalMid4.z);
    glTexCoord2f(0.33f, 0.5f); glVertex3f(naturalMid3.x, naturalMid3.y, naturalMid3.z);
    glTexCoord2f(0.33f, 0.0f); glVertex3f(naturalBase3.x, naturalBase3.y, naturalBase3.z);
    
    glTexCoord2f(0.5f, 0.0f); glVertex3f(naturalBase5.x, naturalBase5.y, naturalBase5.z);
    glTexCoord2f(0.66f, 0.0f); glVertex3f(naturalBase4.x, naturalBase4.y, naturalBase4.z);
    glTexCoord2f(0.66f, 0.5f); glVertex3f(naturalMid5.x, naturalMid5.y, naturalMid5.z);
    
    glTexCoord2f(0.66f, 0.5f); glVertex3f(naturalMid5.x, naturalMid5.y, naturalMid5.z);
    glTexCoord2f(0.5f, 0.5f); glVertex3f(naturalMid4.x, naturalMid4.y, naturalMid4.z);
    glTexCoord2f(0.5f, 0.0f); glVertex3f(naturalBase5.x, naturalBase5.y, naturalBase5.z);
    
    glTexCoord2f(0.66f, 0.0f); glVertex3f(naturalBase4.x, naturalBase4.y, naturalBase4.z);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(naturalBase2.x, naturalBase2.y, naturalBase2.z);
    glTexCoord2f(1.0f, 0.5f); glVertex3f(naturalMid6.x, naturalMid6.y, naturalMid6.z);
    
    glTexCoord2f(1.0f, 0.5f); glVertex3f(naturalMid6.x, naturalMid6.y, naturalMid6.z);
    glTexCoord2f(0.66f, 0.5f); glVertex3f(naturalMid5.x, naturalMid5.y, naturalMid5.z);
    glTexCoord2f(0.66f, 0.0f); glVertex3f(naturalBase4.x, naturalBase4.y, naturalBase4.z);
    
    glTexCoord2f(1.0f, 0.0f); glVertex3f(naturalBase2.x, naturalBase2.y, naturalBase2.z);
    glTexCoord2f(1.0f, 0.5f); glVertex3f(naturalMid2.x, naturalMid2.y, naturalMid2.z);
    glTexCoord2f(1.0f, 0.5f); glVertex3f(naturalMid6.x, naturalMid6.y, naturalMid6.z);
    
    // Natural knuckle transitions (seamless with finger connections)
    glTexCoord2f(0.0f, 0.5f); glVertex3f(naturalMid1.x, naturalMid1.y, naturalMid1.z);
    glTexCoord2f(0.2f, 0.8f); glVertex3f(naturalKnuckle1.x, naturalKnuckle1.y, naturalKnuckle1.z);
    glTexCoord2f(0.33f, 0.5f); glVertex3f(naturalMid3.x, naturalMid3.y, naturalMid3.z);
    
    glTexCoord2f(0.33f, 0.5f); glVertex3f(naturalMid3.x, naturalMid3.y, naturalMid3.z);
    glTexCoord2f(0.2f, 0.8f); glVertex3f(naturalKnuckle1.x, naturalKnuckle1.y, naturalKnuckle1.z);
    glTexCoord2f(0.4f, 0.8f); glVertex3f(naturalKnuckle2.x, naturalKnuckle2.y, naturalKnuckle2.z);
    
    glTexCoord2f(0.33f, 0.5f); glVertex3f(naturalMid3.x, naturalMid3.y, naturalMid3.z);
    glTexCoord2f(0.4f, 0.8f); glVertex3f(naturalKnuckle2.x, naturalKnuckle2.y, naturalKnuckle2.z);
    glTexCoord2f(0.5f, 0.5f); glVertex3f(naturalMid4.x, naturalMid4.y, naturalMid4.z);
    
    glTexCoord2f(0.5f, 0.5f); glVertex3f(naturalMid4.x, naturalMid4.y, naturalMid4.z);
    glTexCoord2f(0.4f, 0.8f); glVertex3f(naturalKnuckle2.x, naturalKnuckle2.y, naturalKnuckle2.z);
    glTexCoord2f(0.6f, 0.8f); glVertex3f(naturalKnuckle3.x, naturalKnuckle3.y, naturalKnuckle3.z);
    
    glTexCoord2f(0.5f, 0.5f); glVertex3f(naturalMid4.x, naturalMid4.y, naturalMid4.z);
    glTexCoord2f(0.6f, 0.8f); glVertex3f(naturalKnuckle3.x, naturalKnuckle3.y, naturalKnuckle3.z);
    glTexCoord2f(0.66f, 0.5f); glVertex3f(naturalMid5.x, naturalMid5.y, naturalMid5.z);
    
    glTexCoord2f(0.66f, 0.5f); glVertex3f(naturalMid5.x, naturalMid5.y, naturalMid5.z);
    glTexCoord2f(0.6f, 0.8f); glVertex3f(naturalKnuckle3.x, naturalKnuckle3.y, naturalKnuckle3.z);
    glTexCoord2f(0.8f, 0.8f); glVertex3f(naturalKnuckle4.x, naturalKnuckle4.y, naturalKnuckle4.z);
    
    glTexCoord2f(0.66f, 0.5f); glVertex3f(naturalMid5.x, naturalMid5.y, naturalMid5.z);
    glTexCoord2f(0.8f, 0.8f); glVertex3f(naturalKnuckle4.x, naturalKnuckle4.y, naturalKnuckle4.z);
    glTexCoord2f(1.0f, 0.5f); glVertex3f(naturalMid6.x, naturalMid6.y, naturalMid6.z);
    
    glTexCoord2f(1.0f, 0.5f); glVertex3f(naturalMid6.x, naturalMid6.y, naturalMid6.z);
    glTexCoord2f(0.8f, 0.8f); glVertex3f(naturalKnuckle4.x, naturalKnuckle4.y, naturalKnuckle4.z);
    glTexCoord2f(1.0f, 0.8f); glVertex3f(naturalKnuckle5.x, naturalKnuckle5.y, naturalKnuckle5.z);
    
    glTexCoord2f(1.0f, 0.5f); glVertex3f(naturalMid6.x, naturalMid6.y, naturalMid6.z);
    glTexCoord2f(1.0f, 0.8f); glVertex3f(naturalKnuckle5.x, naturalKnuckle5.y, naturalKnuckle5.z);
    glTexCoord2f(1.0f, 0.5f); glVertex3f(naturalMid2.x, naturalMid2.y, naturalMid2.z);
    
    // Additional smooth surface coverage for seamless integration
    glTexCoord2f(0.2f, 0.2f); glVertex3f(naturalBase5.x, naturalBase5.y, naturalBase5.z);
    glTexCoord2f(0.4f, 0.4f); glVertex3f(naturalMid3.x, naturalMid3.y, naturalMid3.z);
    glTexCoord2f(0.6f, 0.4f); glVertex3f(naturalMid4.x, naturalMid4.y, naturalMid4.z);
    
    glTexCoord2f(0.6f, 0.4f); glVertex3f(naturalMid4.x, naturalMid4.y, naturalMid4.z);
    glTexCoord2f(0.8f, 0.4f); glVertex3f(naturalMid5.x, naturalMid5.y, naturalMid5.z);
    glTexCoord2f(0.5f, 0.2f); glVertex3f(naturalBase5.x, naturalBase5.y, naturalBase5.z);
    
    glTexCoord2f(0.5f, 0.2f); glVertex3f(naturalBase5.x, naturalBase5.y, naturalBase5.z);
    glTexCoord2f(0.8f, 0.4f); glVertex3f(naturalMid5.x, naturalMid5.y, naturalMid5.z);
    glTexCoord2f(0.8f, 0.2f); glVertex3f(naturalBase4.x, naturalBase4.y, naturalBase4.z);
    
    glEnd();
}

// Draw palm with complete polygonal coverage like a real hand for hand 2
static void drawSkinnedPalm2() {
    Vec3 wrist = g_HandJoints2[0].position;
    Vec3 thumbBase = g_HandJoints2[1].position;
    Vec3 indexBase = g_HandJoints2[5].position;
    Vec3 middleBase = g_HandJoints2[9].position;
    Vec3 ringBase = g_HandJoints2[13].position;
    Vec3 pinkyBase = g_HandJoints2[17].position;
    
    // COMPREHENSIVE PALM TOPOLOGY - DENSE MESH for complete coverage visualization
    
    // Base vertices (wrist area) - wider for natural proportions
    Vec3 palmBase1 = {wrist.x - 0.5f, wrist.y, wrist.z - 0.1f};    // Pinky side base
    Vec3 palmBase2 = {wrist.x + 0.5f, wrist.y, wrist.z - 0.1f};    // Thumb side base  
    Vec3 palmBase3 = {wrist.x - 0.16f, wrist.y, wrist.z - 0.1f};   // Center-left base
    Vec3 palmBase4 = {wrist.x + 0.16f, wrist.y, wrist.z - 0.1f};   // Center-right base
    Vec3 palmBase5 = {wrist.x, wrist.y, wrist.z - 0.1f};           // Center base
    
    // Lower palm vertices (first elevation) - more subdivisions
    Vec3 palmLow1 = {wrist.x - 0.45f, wrist.y + 0.12f, wrist.z + 0.15f};  // Pinky side low
    Vec3 palmLow2 = {wrist.x + 0.45f, wrist.y + 0.12f, wrist.z + 0.15f};  // Thumb side low
    Vec3 palmLow3 = {wrist.x - 0.25f, wrist.y + 0.12f, wrist.z + 0.15f};  // Left-center low
    Vec3 palmLow4 = {wrist.x - 0.08f, wrist.y + 0.12f, wrist.z + 0.15f};  // Center-left low
    Vec3 palmLow5 = {wrist.x + 0.08f, wrist.y + 0.12f, wrist.z + 0.15f};  // Center-right low
    Vec3 palmLow6 = {wrist.x + 0.25f, wrist.y + 0.12f, wrist.z + 0.15f};  // Right-center low
    
    // Middle palm vertices (second elevation) - dense mesh
    Vec3 palmMid1 = {wrist.x - 0.4f, wrist.y + 0.25f, wrist.z + 0.35f};   // Pinky side mid
    Vec3 palmMid2 = {wrist.x + 0.4f, wrist.y + 0.25f, wrist.z + 0.35f};   // Thumb side mid
    Vec3 palmMid3 = {wrist.x - 0.2f, wrist.y + 0.25f, wrist.z + 0.35f};   // Left-center mid
    Vec3 palmMid4 = {wrist.x - 0.06f, wrist.y + 0.25f, wrist.z + 0.35f};  // Center-left mid
    Vec3 palmMid5 = {wrist.x + 0.06f, wrist.y + 0.25f, wrist.z + 0.35f};  // Center-right mid
    Vec3 palmMid6 = {wrist.x + 0.2f, wrist.y + 0.25f, wrist.z + 0.35f};   // Right-center mid
    
    // Upper-middle palm vertices (third elevation) - additional layer for density
    Vec3 palmUpMid1 = {wrist.x - 0.35f, wrist.y + 0.4f, wrist.z + 0.55f};   // Pinky side up-mid
    Vec3 palmUpMid2 = {wrist.x + 0.35f, wrist.y + 0.4f, wrist.z + 0.55f};   // Thumb side up-mid
    Vec3 palmUpMid3 = {wrist.x - 0.15f, wrist.y + 0.4f, wrist.z + 0.55f};   // Left-center up-mid
    Vec3 palmUpMid4 = {wrist.x - 0.05f, wrist.y + 0.4f, wrist.z + 0.55f};   // Center-left up-mid
    Vec3 palmUpMid5 = {wrist.x + 0.05f, wrist.y + 0.4f, wrist.z + 0.55f};   // Center-right up-mid
    Vec3 palmUpMid6 = {wrist.x + 0.15f, wrist.y + 0.4f, wrist.z + 0.55f};   // Right-center up-mid
    
    // Upper palm vertices (knuckle area) - dense knuckle coverage
    Vec3 palmHigh1 = {pinkyBase.x - 0.05f, pinkyBase.y + 0.05f, pinkyBase.z - 0.4f};    // Pinky knuckle area
    Vec3 palmHigh2 = {ringBase.x - 0.05f, ringBase.y + 0.08f, ringBase.z - 0.4f};      // Ring knuckle area
    Vec3 palmHigh3 = {middleBase.x - 0.05f, middleBase.y + 0.1f, middleBase.z - 0.4f}; // Middle knuckle area
    Vec3 palmHigh4 = {indexBase.x - 0.05f, indexBase.y + 0.08f, indexBase.z - 0.4f};   // Index knuckle area
    Vec3 palmHigh5 = {thumbBase.x - 0.1f, thumbBase.y + 0.0f, thumbBase.z - 0.3f};     // Thumb knuckle area
    
    // Pre-knuckle vertices (between upper palm and knuckles) - smooth transition
    Vec3 preKnuckle1 = {pinkyBase.x - 0.03f, pinkyBase.y + 0.03f, pinkyBase.z - 0.25f};
    Vec3 preKnuckle2 = {ringBase.x - 0.03f, ringBase.y + 0.05f, ringBase.z - 0.25f};
    Vec3 preKnuckle3 = {middleBase.x - 0.03f, middleBase.y + 0.07f, middleBase.z - 0.25f};
    Vec3 preKnuckle4 = {indexBase.x - 0.03f, indexBase.y + 0.05f, indexBase.z - 0.25f};
    Vec3 preKnuckle5 = {thumbBase.x - 0.06f, thumbBase.y - 0.02f, thumbBase.z - 0.15f};
    
    // Finger connection vertices (direct connections to finger bases)
    Vec3 fingerConn1 = {pinkyBase.x, pinkyBase.y - 0.08f, pinkyBase.z - 0.1f};   // Pinky connection
    Vec3 fingerConn2 = {ringBase.x, ringBase.y - 0.08f, ringBase.z - 0.1f};     // Ring connection
    Vec3 fingerConn3 = {middleBase.x, middleBase.y - 0.08f, middleBase.z - 0.1f}; // Middle connection
    Vec3 fingerConn4 = {indexBase.x, indexBase.y - 0.08f, indexBase.z - 0.1f};   // Index connection
    Vec3 fingerConn5 = {thumbBase.x, thumbBase.y - 0.05f, thumbBase.z - 0.05f};  // Thumb connection
    
    // BACK OF HAND vertices (create solid volume) - Dense mesh to match front
    Vec3 backBase1 = {palmBase1.x, palmBase1.y - 0.4f, palmBase1.z};
    Vec3 backBase2 = {palmBase2.x, palmBase2.y - 0.4f, palmBase2.z};
    Vec3 backBase3 = {palmBase3.x, palmBase3.y - 0.4f, palmBase3.z};
    Vec3 backBase4 = {palmBase4.x, palmBase4.y - 0.4f, palmBase4.z};
    Vec3 backBase5 = {palmBase5.x, palmBase5.y - 0.4f, palmBase5.z};
    
    Vec3 backLow1 = {palmLow1.x, palmLow1.y - 0.4f, palmLow1.z};
    Vec3 backLow2 = {palmLow2.x, palmLow2.y - 0.4f, palmLow2.z};
    Vec3 backLow3 = {palmLow3.x, palmLow3.y - 0.4f, palmLow3.z};
    Vec3 backLow4 = {palmLow4.x, palmLow4.y - 0.4f, palmLow4.z};
    Vec3 backLow5 = {palmLow5.x, palmLow5.y - 0.4f, palmLow5.z};
    Vec3 backLow6 = {palmLow6.x, palmLow6.y - 0.4f, palmLow6.z};
    
    Vec3 backMid1 = {palmMid1.x, palmMid1.y - 0.4f, palmMid1.z};
    Vec3 backMid2 = {palmMid2.x, palmMid2.y - 0.4f, palmMid2.z};
    Vec3 backMid3 = {palmMid3.x, palmMid3.y - 0.4f, palmMid3.z};
    Vec3 backMid4 = {palmMid4.x, palmMid4.y - 0.4f, palmMid4.z};
    Vec3 backMid5 = {palmMid5.x, palmMid5.y - 0.4f, palmMid5.z};
    Vec3 backMid6 = {palmMid6.x, palmMid6.y - 0.4f, palmMid6.z};
    
    Vec3 backUpMid1 = {palmUpMid1.x, palmUpMid1.y - 0.4f, palmUpMid1.z};
    Vec3 backUpMid2 = {palmUpMid2.x, palmUpMid2.y - 0.4f, palmUpMid2.z};
    Vec3 backUpMid3 = {palmUpMid3.x, palmUpMid3.y - 0.4f, palmUpMid3.z};
    Vec3 backUpMid4 = {palmUpMid4.x, palmUpMid4.y - 0.4f, palmUpMid4.z};
    Vec3 backUpMid5 = {palmUpMid5.x, palmUpMid5.y - 0.4f, palmUpMid5.z};
    Vec3 backUpMid6 = {palmUpMid6.x, palmUpMid6.y - 0.4f, palmUpMid6.z};
    
    Vec3 backHigh1 = {palmHigh1.x, palmHigh1.y - 0.4f, palmHigh1.z};
    Vec3 backHigh2 = {palmHigh2.x, palmHigh2.y - 0.4f, palmHigh2.z};
    Vec3 backHigh3 = {palmHigh3.x, palmHigh3.y - 0.4f, palmHigh3.z};
    Vec3 backHigh4 = {palmHigh4.x, palmHigh4.y - 0.4f, palmHigh4.z};
    Vec3 backHigh5 = {palmHigh5.x, palmHigh5.y - 0.4f, palmHigh5.z};
    
    Vec3 backPreKnuckle1 = {preKnuckle1.x, preKnuckle1.y - 0.4f, preKnuckle1.z};
    Vec3 backPreKnuckle2 = {preKnuckle2.x, preKnuckle2.y - 0.4f, preKnuckle2.z};
    Vec3 backPreKnuckle3 = {preKnuckle3.x, preKnuckle3.y - 0.4f, preKnuckle3.z};
    Vec3 backPreKnuckle4 = {preKnuckle4.x, preKnuckle4.y - 0.4f, preKnuckle4.z};
    Vec3 backPreKnuckle5 = {preKnuckle5.x, preKnuckle5.y - 0.4f, preKnuckle5.z};
    
    // START DRAWING DENSE PALM TOPOLOGY - Maximum polygon coverage
    glBegin(GL_TRIANGLES);
    
    // PALM SURFACE (top) - DENSE MESH for complete visualization
    Vec3 normalUp = {0, 1, 0};
    glNormal3f(normalUp.x, normalUp.y, normalUp.z);
    
    // DENSE LAYER 1: Base to Low palm (6x6 subdivisions)
    // Row 1: Left sections
    glTexCoord2f(0.0f, 0.0f); glVertex3f(palmBase1.x, palmBase1.y, palmBase1.z);
    glTexCoord2f(0.2f, 0.0f); glVertex3f(palmBase3.x, palmBase3.y, palmBase3.z);
    glTexCoord2f(0.2f, 0.16f); glVertex3f(palmLow3.x, palmLow3.y, palmLow3.z);
    
    glTexCoord2f(0.2f, 0.16f); glVertex3f(palmLow3.x, palmLow3.y, palmLow3.z);
    glTexCoord2f(0.0f, 0.16f); glVertex3f(palmLow1.x, palmLow1.y, palmLow1.z);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(palmBase1.x, palmBase1.y, palmBase1.z);
    
    glTexCoord2f(0.2f, 0.0f); glVertex3f(palmBase3.x, palmBase3.y, palmBase3.z);
    glTexCoord2f(0.35f, 0.0f); glVertex3f(palmBase5.x, palmBase5.y, palmBase5.z);
    glTexCoord2f(0.35f, 0.16f); glVertex3f(palmLow4.x, palmLow4.y, palmLow4.z);
    
    glTexCoord2f(0.35f, 0.16f); glVertex3f(palmLow4.x, palmLow4.y, palmLow4.z);
    glTexCoord2f(0.2f, 0.16f); glVertex3f(palmLow3.x, palmLow3.y, palmLow3.z);
    glTexCoord2f(0.2f, 0.0f); glVertex3f(palmBase3.x, palmBase3.y, palmBase3.z);
    
    glTexCoord2f(0.35f, 0.0f); glVertex3f(palmBase5.x, palmBase5.y, palmBase5.z);
    glTexCoord2f(0.5f, 0.0f); glVertex3f(palmBase4.x, palmBase4.y, palmBase4.z);
    glTexCoord2f(0.5f, 0.16f); glVertex3f(palmLow5.x, palmLow5.y, palmLow5.z);
    
    glTexCoord2f(0.5f, 0.16f); glVertex3f(palmLow5.x, palmLow5.y, palmLow5.z);
    glTexCoord2f(0.35f, 0.16f); glVertex3f(palmLow4.x, palmLow4.y, palmLow4.z);
    glTexCoord2f(0.35f, 0.0f); glVertex3f(palmBase5.x, palmBase5.y, palmBase5.z);
    
    glTexCoord2f(0.5f, 0.0f); glVertex3f(palmBase4.x, palmBase4.y, palmBase4.z);
    glTexCoord2f(0.8f, 0.0f); glVertex3f(palmBase2.x, palmBase2.y, palmBase2.z);
    glTexCoord2f(0.8f, 0.16f); glVertex3f(palmLow6.x, palmLow6.y, palmLow6.z);
    
    glTexCoord2f(0.8f, 0.16f); glVertex3f(palmLow6.x, palmLow6.y, palmLow6.z);
    glTexCoord2f(0.5f, 0.16f); glVertex3f(palmLow5.x, palmLow5.y, palmLow5.z);
    glTexCoord2f(0.5f, 0.0f); glVertex3f(palmBase4.x, palmBase4.y, palmBase4.z);
    
    glTexCoord2f(0.8f, 0.0f); glVertex3f(palmBase2.x, palmBase2.y, palmBase2.z);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(palmBase2.x, palmBase2.y, palmBase2.z);
    glTexCoord2f(1.0f, 0.16f); glVertex3f(palmLow2.x, palmLow2.y, palmLow2.z);
    
    glTexCoord2f(1.0f, 0.16f); glVertex3f(palmLow2.x, palmLow2.y, palmLow2.z);
    glTexCoord2f(0.8f, 0.16f); glVertex3f(palmLow6.x, palmLow6.y, palmLow6.z);
    glTexCoord2f(0.8f, 0.0f); glVertex3f(palmBase2.x, palmBase2.y, palmBase2.z);
    
    // Continue with additional layers to match first palm exactly...
    // DENSE LAYER 2: Low to Mid palm (6x6 subdivisions)
    glTexCoord2f(0.0f, 0.16f); glVertex3f(palmLow1.x, palmLow1.y, palmLow1.z);
    glTexCoord2f(0.2f, 0.16f); glVertex3f(palmLow3.x, palmLow3.y, palmLow3.z);
    glTexCoord2f(0.2f, 0.33f); glVertex3f(palmMid3.x, palmMid3.y, palmMid3.z);
    
    glTexCoord2f(0.2f, 0.33f); glVertex3f(palmMid3.x, palmMid3.y, palmMid3.z);
    glTexCoord2f(0.0f, 0.33f); glVertex3f(palmMid1.x, palmMid1.y, palmMid1.z);
    glTexCoord2f(0.0f, 0.16f); glVertex3f(palmLow1.x, palmLow1.y, palmLow1.z);
    
    glTexCoord2f(0.2f, 0.16f); glVertex3f(palmLow3.x, palmLow3.y, palmLow3.z);
    glTexCoord2f(0.35f, 0.16f); glVertex3f(palmLow4.x, palmLow4.y, palmLow4.z);
    glTexCoord2f(0.35f, 0.33f); glVertex3f(palmMid4.x, palmMid4.y, palmMid4.z);
    
    glTexCoord2f(0.35f, 0.33f); glVertex3f(palmMid4.x, palmMid4.y, palmMid4.z);
    glTexCoord2f(0.2f, 0.33f); glVertex3f(palmMid3.x, palmMid3.y, palmMid3.z);
    glTexCoord2f(0.2f, 0.16f); glVertex3f(palmLow3.x, palmLow3.y, palmLow3.z);
    
    glTexCoord2f(0.35f, 0.16f); glVertex3f(palmLow4.x, palmLow4.y, palmLow4.z);
    glTexCoord2f(0.5f, 0.16f); glVertex3f(palmLow5.x, palmLow5.y, palmLow5.z);
    glTexCoord2f(0.5f, 0.33f); glVertex3f(palmMid5.x, palmMid5.y, palmMid5.z);
    
    glTexCoord2f(0.5f, 0.33f); glVertex3f(palmMid5.x, palmMid5.y, palmMid5.z);
    glTexCoord2f(0.35f, 0.33f); glVertex3f(palmMid4.x, palmMid4.y, palmMid4.z);
    glTexCoord2f(0.35f, 0.16f); glVertex3f(palmLow4.x, palmLow4.y, palmLow4.z);
    
    glTexCoord2f(0.5f, 0.16f); glVertex3f(palmLow5.x, palmLow5.y, palmLow5.z);
    glTexCoord2f(0.8f, 0.16f); glVertex3f(palmLow6.x, palmLow6.y, palmLow6.z);
    glTexCoord2f(0.8f, 0.33f); glVertex3f(palmMid6.x, palmMid6.y, palmMid6.z);
    
    glTexCoord2f(0.8f, 0.33f); glVertex3f(palmMid6.x, palmMid6.y, palmMid6.z);
    glTexCoord2f(0.5f, 0.33f); glVertex3f(palmMid5.x, palmMid5.y, palmMid5.z);
    glTexCoord2f(0.5f, 0.16f); glVertex3f(palmLow5.x, palmLow5.y, palmLow5.z);
    
    glTexCoord2f(0.8f, 0.16f); glVertex3f(palmLow6.x, palmLow6.y, palmLow6.z);
    glTexCoord2f(1.0f, 0.16f); glVertex3f(palmLow2.x, palmLow2.y, palmLow2.z);
    glTexCoord2f(1.0f, 0.33f); glVertex3f(palmMid2.x, palmMid2.y, palmMid2.z);
    
    glTexCoord2f(1.0f, 0.33f); glVertex3f(palmMid2.x, palmMid2.y, palmMid2.z);
    glTexCoord2f(0.8f, 0.33f); glVertex3f(palmMid6.x, palmMid6.y, palmMid6.z);
    glTexCoord2f(0.8f, 0.16f); glVertex3f(palmLow6.x, palmLow6.y, palmLow6.z);
    
    // DENSE LAYER 3: Mid to UpMid palm (6x6 subdivisions)
    glTexCoord2f(0.0f, 0.33f); glVertex3f(palmMid1.x, palmMid1.y, palmMid1.z);
    glTexCoord2f(0.2f, 0.33f); glVertex3f(palmMid3.x, palmMid3.y, palmMid3.z);
    glTexCoord2f(0.2f, 0.5f); glVertex3f(palmUpMid3.x, palmUpMid3.y, palmUpMid3.z);
    
    glTexCoord2f(0.2f, 0.5f); glVertex3f(palmUpMid3.x, palmUpMid3.y, palmUpMid3.z);
    glTexCoord2f(0.0f, 0.5f); glVertex3f(palmUpMid1.x, palmUpMid1.y, palmUpMid1.z);
    glTexCoord2f(0.0f, 0.33f); glVertex3f(palmMid1.x, palmMid1.y, palmMid1.z);
    
    glTexCoord2f(0.2f, 0.33f); glVertex3f(palmMid3.x, palmMid3.y, palmMid3.z);
    glTexCoord2f(0.35f, 0.33f); glVertex3f(palmMid4.x, palmMid4.y, palmMid4.z);
    glTexCoord2f(0.35f, 0.5f); glVertex3f(palmUpMid4.x, palmUpMid4.y, palmUpMid4.z);
    
    glTexCoord2f(0.35f, 0.5f); glVertex3f(palmUpMid4.x, palmUpMid4.y, palmUpMid4.z);
    glTexCoord2f(0.2f, 0.5f); glVertex3f(palmUpMid3.x, palmUpMid3.y, palmUpMid3.z);
    glTexCoord2f(0.2f, 0.33f); glVertex3f(palmMid3.x, palmMid3.y, palmMid3.z);
    
    // LAYER 3: Mid to High palm (knuckle areas)
    glTexCoord2f(0.0f, 0.5f); glVertex3f(palmMid1.x, palmMid1.y, palmMid1.z);
    glTexCoord2f(0.2f, 0.75f); glVertex3f(palmHigh1.x, palmHigh1.y, palmHigh1.z);
    glTexCoord2f(0.4f, 0.75f); glVertex3f(palmHigh2.x, palmHigh2.y, palmHigh2.z);
    
    glTexCoord2f(0.4f, 0.75f); glVertex3f(palmHigh2.x, palmHigh2.y, palmHigh2.z);
    glTexCoord2f(0.33f, 0.5f); glVertex3f(palmMid3.x, palmMid3.y, palmMid3.z);
    glTexCoord2f(0.0f, 0.5f); glVertex3f(palmMid1.x, palmMid1.y, palmMid1.z);
    
    glTexCoord2f(0.33f, 0.5f); glVertex3f(palmMid3.x, palmMid3.y, palmMid3.z);
    glTexCoord2f(0.4f, 0.75f); glVertex3f(palmHigh2.x, palmHigh2.y, palmHigh2.z);
    glTexCoord2f(0.6f, 0.75f); glVertex3f(palmHigh3.x, palmHigh3.y, palmHigh3.z);
    
    glTexCoord2f(0.6f, 0.75f); glVertex3f(palmHigh3.x, palmHigh3.y, palmHigh3.z);
    glTexCoord2f(0.66f, 0.5f); glVertex3f(palmMid4.x, palmMid4.y, palmMid4.z);
    glTexCoord2f(0.33f, 0.5f); glVertex3f(palmMid3.x, palmMid3.y, palmMid3.z);
    
    glTexCoord2f(0.66f, 0.5f); glVertex3f(palmMid4.x, palmMid4.y, palmMid4.z);
    glTexCoord2f(0.6f, 0.75f); glVertex3f(palmHigh3.x, palmHigh3.y, palmHigh3.z);
    glTexCoord2f(0.8f, 0.75f); glVertex3f(palmHigh4.x, palmHigh4.y, palmHigh4.z);
    
    glTexCoord2f(0.8f, 0.75f); glVertex3f(palmHigh4.x, palmHigh4.y, palmHigh4.z);
    glTexCoord2f(1.0f, 0.5f); glVertex3f(palmMid2.x, palmMid2.y, palmMid2.z);
    glTexCoord2f(0.66f, 0.5f); glVertex3f(palmMid4.x, palmMid4.y, palmMid4.z);
    
    // Thumb area connection
    glTexCoord2f(0.8f, 0.75f); glVertex3f(palmHigh4.x, palmHigh4.y, palmHigh4.z);
    glTexCoord2f(1.0f, 0.75f); glVertex3f(palmHigh5.x, palmHigh5.y, palmHigh5.z);
    glTexCoord2f(1.0f, 0.5f); glVertex3f(palmMid2.x, palmMid2.y, palmMid2.z);
    
    // LAYER 4: High palm to finger connections
    glTexCoord2f(0.2f, 0.75f); glVertex3f(palmHigh1.x, palmHigh1.y, palmHigh1.z);
    glTexCoord2f(0.2f, 1.0f); glVertex3f(fingerConn1.x, fingerConn1.y, fingerConn1.z);
    glTexCoord2f(0.4f, 1.0f); glVertex3f(fingerConn2.x, fingerConn2.y, fingerConn2.z);
    
    glTexCoord2f(0.4f, 1.0f); glVertex3f(fingerConn2.x, fingerConn2.y, fingerConn2.z);
    glTexCoord2f(0.4f, 0.75f); glVertex3f(palmHigh2.x, palmHigh2.y, palmHigh2.z);
    glTexCoord2f(0.2f, 0.75f); glVertex3f(palmHigh1.x, palmHigh1.y, palmHigh1.z);
    
    glTexCoord2f(0.4f, 0.75f); glVertex3f(palmHigh2.x, palmHigh2.y, palmHigh2.z);
    glTexCoord2f(0.4f, 1.0f); glVertex3f(fingerConn2.x, fingerConn2.y, fingerConn2.z);
    glTexCoord2f(0.6f, 1.0f); glVertex3f(fingerConn3.x, fingerConn3.y, fingerConn3.z);
    
    glTexCoord2f(0.6f, 1.0f); glVertex3f(fingerConn3.x, fingerConn3.y, fingerConn3.z);
    glTexCoord2f(0.6f, 0.75f); glVertex3f(palmHigh3.x, palmHigh3.y, palmHigh3.z);
    glTexCoord2f(0.4f, 0.75f); glVertex3f(palmHigh2.x, palmHigh2.y, palmHigh2.z);
    
    glTexCoord2f(0.6f, 0.75f); glVertex3f(palmHigh3.x, palmHigh3.y, palmHigh3.z);
    glTexCoord2f(0.6f, 1.0f); glVertex3f(fingerConn3.x, fingerConn3.y, fingerConn3.z);
    glTexCoord2f(0.8f, 1.0f); glVertex3f(fingerConn4.x, fingerConn4.y, fingerConn4.z);
    
    glTexCoord2f(0.8f, 1.0f); glVertex3f(fingerConn4.x, fingerConn4.y, fingerConn4.z);
    glTexCoord2f(0.8f, 0.75f); glVertex3f(palmHigh4.x, palmHigh4.y, palmHigh4.z);
    glTexCoord2f(0.6f, 0.75f); glVertex3f(palmHigh3.x, palmHigh3.y, palmHigh3.z);
    
    glTexCoord2f(0.8f, 0.75f); glVertex3f(palmHigh4.x, palmHigh4.y, palmHigh4.z);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(fingerConn5.x, fingerConn5.y, fingerConn5.z);
    glTexCoord2f(1.0f, 0.75f); glVertex3f(palmHigh5.x, palmHigh5.y, palmHigh5.z);
    
    // LAYER 5: Finger connections to actual finger bases
    Vec3 normalConnect = {0, 0.6f, 0.8f};
    glNormal3f(normalConnect.x, normalConnect.y, normalConnect.z);
    
    glTexCoord2f(0.2f, 1.0f); glVertex3f(fingerConn1.x, fingerConn1.y, fingerConn1.z);
    glTexCoord2f(0.2f, 1.0f); glVertex3f(pinkyBase.x, pinkyBase.y, pinkyBase.z);
    glTexCoord2f(0.4f, 1.0f); glVertex3f(ringBase.x, ringBase.y, ringBase.z);
    
    glTexCoord2f(0.4f, 1.0f); glVertex3f(ringBase.x, ringBase.y, ringBase.z);
    glTexCoord2f(0.4f, 1.0f); glVertex3f(fingerConn2.x, fingerConn2.y, fingerConn2.z);
    glTexCoord2f(0.2f, 1.0f); glVertex3f(fingerConn1.x, fingerConn1.y, fingerConn1.z);
    
    glTexCoord2f(0.4f, 1.0f); glVertex3f(fingerConn2.x, fingerConn2.y, fingerConn2.z);
    glTexCoord2f(0.4f, 1.0f); glVertex3f(ringBase.x, ringBase.y, ringBase.z);
    glTexCoord2f(0.6f, 1.0f); glVertex3f(middleBase.x, middleBase.y, middleBase.z);
    
    glTexCoord2f(0.6f, 1.0f); glVertex3f(middleBase.x, middleBase.y, middleBase.z);
    glTexCoord2f(0.6f, 1.0f); glVertex3f(fingerConn3.x, fingerConn3.y, fingerConn3.z);
    glTexCoord2f(0.4f, 1.0f); glVertex3f(fingerConn2.x, fingerConn2.y, fingerConn2.z);
    
    glTexCoord2f(0.6f, 1.0f); glVertex3f(fingerConn3.x, fingerConn3.y, fingerConn3.z);
    glTexCoord2f(0.6f, 1.0f); glVertex3f(middleBase.x, middleBase.y, middleBase.z);
    glTexCoord2f(0.8f, 1.0f); glVertex3f(indexBase.x, indexBase.y, indexBase.z);
    
    glTexCoord2f(0.8f, 1.0f); glVertex3f(indexBase.x, indexBase.y, indexBase.z);
    glTexCoord2f(0.8f, 1.0f); glVertex3f(fingerConn4.x, fingerConn4.y, fingerConn4.z);
    glTexCoord2f(0.6f, 1.0f); glVertex3f(fingerConn3.x, fingerConn3.y, fingerConn3.z);
    
    glTexCoord2f(0.8f, 1.0f); glVertex3f(fingerConn4.x, fingerConn4.y, fingerConn4.z);
    glTexCoord2f(0.8f, 1.0f); glVertex3f(indexBase.x, indexBase.y, indexBase.z);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(thumbBase.x, thumbBase.y, thumbBase.z);
    
    glTexCoord2f(1.0f, 1.0f); glVertex3f(thumbBase.x, thumbBase.y, thumbBase.z);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(fingerConn5.x, fingerConn5.y, fingerConn5.z);
    glTexCoord2f(0.8f, 1.0f); glVertex3f(fingerConn4.x, fingerConn4.y, fingerConn4.z);
    
    // BACK OF HAND SURFACE (bottom) - Complete coverage with layered topology
    Vec3 normalDown = {0, -1, 0};
    glNormal3f(normalDown.x, normalDown.y, normalDown.z);
    
    // Mirror the top topology on the bottom for solid volume
    // Base to Low back layer
    glTexCoord2f(0.0f, 0.0f); glVertex3f(backBase1.x, backBase1.y, backBase1.z);
    glTexCoord2f(0.0f, 0.25f); glVertex3f(backLow1.x, backLow1.y, backLow1.z);
    glTexCoord2f(0.33f, 0.25f); glVertex3f(backLow3.x, backLow3.y, backLow3.z);
    
    glTexCoord2f(0.33f, 0.25f); glVertex3f(backLow3.x, backLow3.y, backLow3.z);
    glTexCoord2f(0.33f, 0.0f); glVertex3f(backBase3.x, backBase3.y, backBase3.z);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(backBase1.x, backBase1.y, backBase1.z);
    
    glTexCoord2f(0.33f, 0.0f); glVertex3f(backBase3.x, backBase3.y, backBase3.z);
    glTexCoord2f(0.33f, 0.25f); glVertex3f(backLow3.x, backLow3.y, backLow3.z);
    glTexCoord2f(0.66f, 0.25f); glVertex3f(backLow4.x, backLow4.y, backLow4.z);
    
    glTexCoord2f(0.66f, 0.25f); glVertex3f(backLow4.x, backLow4.y, backLow4.z);
    glTexCoord2f(0.66f, 0.0f); glVertex3f(backBase2.x, backBase2.y, backBase2.z);
    glTexCoord2f(0.33f, 0.0f); glVertex3f(backBase3.x, backBase3.y, backBase3.z);
    
    glTexCoord2f(0.66f, 0.0f); glVertex3f(backBase2.x, backBase2.y, backBase2.z);
    glTexCoord2f(0.66f, 0.25f); glVertex3f(backLow4.x, backLow4.y, backLow4.z);
    glTexCoord2f(1.0f, 0.25f); glVertex3f(backLow2.x, backLow2.y, backLow2.z);
    
    glTexCoord2f(1.0f, 0.25f); glVertex3f(backLow2.x, backLow2.y, backLow2.z);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(backBase2.x, backBase2.y, backBase2.z);
    glTexCoord2f(0.66f, 0.0f); glVertex3f(backBase2.x, backBase2.y, backBase2.z);
    
    // Low to Mid back layer
    glTexCoord2f(0.0f, 0.25f); glVertex3f(backLow1.x, backLow1.y, backLow1.z);
    glTexCoord2f(0.0f, 0.5f); glVertex3f(backMid1.x, backMid1.y, backMid1.z);
    glTexCoord2f(0.33f, 0.5f); glVertex3f(backMid3.x, backMid3.y, backMid3.z);
    
    glTexCoord2f(0.33f, 0.5f); glVertex3f(backMid3.x, backMid3.y, backMid3.z);
    glTexCoord2f(0.33f, 0.25f); glVertex3f(backLow3.x, backLow3.y, backLow3.z);
    glTexCoord2f(0.0f, 0.25f); glVertex3f(backLow1.x, backLow1.y, backLow1.z);
    
    glTexCoord2f(0.33f, 0.25f); glVertex3f(backLow3.x, backLow3.y, backLow3.z);
    glTexCoord2f(0.33f, 0.5f); glVertex3f(backMid3.x, backMid3.y, backMid3.z);
    glTexCoord2f(0.66f, 0.5f); glVertex3f(backMid4.x, backMid4.y, backMid4.z);
    
    glTexCoord2f(0.66f, 0.5f); glVertex3f(backMid4.x, backMid4.y, backMid4.z);
    glTexCoord2f(0.66f, 0.25f); glVertex3f(backLow4.x, backLow4.y, backLow4.z);
    glTexCoord2f(0.33f, 0.25f); glVertex3f(backLow3.x, backLow3.y, backLow3.z);
    
    glTexCoord2f(0.66f, 0.25f); glVertex3f(backLow4.x, backLow4.y, backLow4.z);
    glTexCoord2f(0.66f, 0.5f); glVertex3f(backMid4.x, backMid4.y, backMid4.z);
    glTexCoord2f(1.0f, 0.5f); glVertex3f(backMid2.x, backMid2.y, backMid2.z);
    
    glTexCoord2f(1.0f, 0.5f); glVertex3f(backMid2.x, backMid2.y, backMid2.z);
    glTexCoord2f(1.0f, 0.25f); glVertex3f(backLow2.x, backLow2.y, backLow2.z);
    glTexCoord2f(0.66f, 0.25f); glVertex3f(backLow4.x, backLow4.y, backLow4.z);
    
    // Mid to High back layer (knuckles area)
    glTexCoord2f(0.0f, 0.5f); glVertex3f(backMid1.x, backMid1.y, backMid1.z);
    glTexCoord2f(0.2f, 0.75f); glVertex3f(backHigh1.x, backHigh1.y, backHigh1.z);
    glTexCoord2f(0.33f, 0.5f); glVertex3f(backMid3.x, backMid3.y, backMid3.z);
    
    glTexCoord2f(0.33f, 0.5f); glVertex3f(backMid3.x, backMid3.y, backMid3.z);
    glTexCoord2f(0.2f, 0.75f); glVertex3f(backHigh1.x, backHigh1.y, backHigh1.z);
    glTexCoord2f(0.4f, 0.75f); glVertex3f(backHigh2.x, backHigh2.y, backHigh2.z);
    
    glTexCoord2f(0.33f, 0.5f); glVertex3f(backMid3.x, backMid3.y, backMid3.z);
    glTexCoord2f(0.4f, 0.75f); glVertex3f(backHigh2.x, backHigh2.y, backHigh2.z);
    glTexCoord2f(0.6f, 0.75f); glVertex3f(backHigh3.x, backHigh3.y, backHigh3.z);
    
    glTexCoord2f(0.6f, 0.75f); glVertex3f(backHigh3.x, backHigh3.y, backHigh3.z);
    glTexCoord2f(0.66f, 0.5f); glVertex3f(backMid4.x, backMid4.y, backMid4.z);
    glTexCoord2f(0.33f, 0.5f); glVertex3f(backMid3.x, backMid3.y, backMid3.z);
    
    glTexCoord2f(0.66f, 0.5f); glVertex3f(backMid4.x, backMid4.y, backMid4.z);
    glTexCoord2f(0.6f, 0.75f); glVertex3f(backHigh3.x, backHigh3.y, backHigh3.z);
    glTexCoord2f(0.8f, 0.75f); glVertex3f(backHigh4.x, backHigh4.y, backHigh4.z);
    
    glTexCoord2f(0.8f, 0.75f); glVertex3f(backHigh4.x, backHigh4.y, backHigh4.z);
    glTexCoord2f(1.0f, 0.5f); glVertex3f(backMid2.x, backMid2.y, backMid2.z);
    glTexCoord2f(0.66f, 0.5f); glVertex3f(backMid4.x, backMid4.y, backMid4.z);
    
    glTexCoord2f(0.8f, 0.75f); glVertex3f(backHigh4.x, backHigh4.y, backHigh4.z);
    glTexCoord2f(1.0f, 0.75f); glVertex3f(backHigh5.x, backHigh5.y, backHigh5.z);
    glTexCoord2f(1.0f, 0.5f); glVertex3f(backMid2.x, backMid2.y, backMid2.z);
    
    // SIDE WALLS - Connect top and bottom surfaces for solid volume
    Vec3 normalSide;
    
    // Left side wall (pinky side)
    normalSide = {-1, 0, 0};
    glNormal3f(normalSide.x, normalSide.y, normalSide.z);
    
    glTexCoord2f(0.0f, 0.0f); glVertex3f(palmBase1.x, palmBase1.y, palmBase1.z);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(backBase1.x, backBase1.y, backBase1.z);
    glTexCoord2f(0.0f, 0.25f); glVertex3f(backLow1.x, backLow1.y, backLow1.z);
    
    glTexCoord2f(0.0f, 0.25f); glVertex3f(backLow1.x, backLow1.y, backLow1.z);
    glTexCoord2f(0.0f, 0.25f); glVertex3f(palmLow1.x, palmLow1.y, palmLow1.z);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(palmBase1.x, palmBase1.y, palmBase1.z);
    
    glTexCoord2f(0.0f, 0.25f); glVertex3f(palmLow1.x, palmLow1.y, palmLow1.z);
    glTexCoord2f(0.0f, 0.25f); glVertex3f(backLow1.x, backLow1.y, backLow1.z);
    glTexCoord2f(0.0f, 0.5f); glVertex3f(backMid1.x, backMid1.y, backMid1.z);
    
    glTexCoord2f(0.0f, 0.5f); glVertex3f(backMid1.x, backMid1.y, backMid1.z);
    glTexCoord2f(0.0f, 0.5f); glVertex3f(palmMid1.x, palmMid1.y, palmMid1.z);
    glTexCoord2f(0.0f, 0.25f); glVertex3f(palmLow1.x, palmLow1.y, palmLow1.z);
    
    glTexCoord2f(0.0f, 0.5f); glVertex3f(palmMid1.x, palmMid1.y, palmMid1.z);
    glTexCoord2f(0.0f, 0.5f); glVertex3f(backMid1.x, backMid1.y, backMid1.z);
    glTexCoord2f(0.2f, 0.75f); glVertex3f(backHigh1.x, backHigh1.y, backHigh1.z);
    
    glTexCoord2f(0.2f, 0.75f); glVertex3f(backHigh1.x, backHigh1.y, backHigh1.z);
    glTexCoord2f(0.2f, 0.75f); glVertex3f(palmHigh1.x, palmHigh1.y, palmHigh1.z);
    glTexCoord2f(0.0f, 0.5f); glVertex3f(palmMid1.x, palmMid1.y, palmMid1.z);
    
    // Right side wall (thumb side)
    normalSide = {1, 0, 0};
    glNormal3f(normalSide.x, normalSide.y, normalSide.z);
    
    glTexCoord2f(1.0f, 0.0f); glVertex3f(palmBase2.x, palmBase2.y, palmBase2.z);
    glTexCoord2f(1.0f, 0.25f); glVertex3f(palmLow2.x, palmLow2.y, palmLow2.z);
    glTexCoord2f(1.0f, 0.25f); glVertex3f(backLow2.x, backLow2.y, backLow2.z);
    
    glTexCoord2f(1.0f, 0.25f); glVertex3f(backLow2.x, backLow2.y, backLow2.z);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(backBase2.x, backBase2.y, backBase2.z);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(palmBase2.x, palmBase2.y, palmBase2.z);
    
    glTexCoord2f(1.0f, 0.25f); glVertex3f(palmLow2.x, palmLow2.y, palmLow2.z);
    glTexCoord2f(1.0f, 0.5f); glVertex3f(palmMid2.x, palmMid2.y, palmMid2.z);
    glTexCoord2f(1.0f, 0.5f); glVertex3f(backMid2.x, backMid2.y, backMid2.z);
    
    glTexCoord2f(1.0f, 0.5f); glVertex3f(backMid2.x, backMid2.y, backMid2.z);
    glTexCoord2f(1.0f, 0.25f); glVertex3f(backLow2.x, backLow2.y, backLow2.z);
    glTexCoord2f(1.0f, 0.25f); glVertex3f(palmLow2.x, palmLow2.y, palmLow2.z);
    
    glTexCoord2f(1.0f, 0.5f); glVertex3f(palmMid2.x, palmMid2.y, palmMid2.z);
    glTexCoord2f(1.0f, 0.75f); glVertex3f(palmHigh5.x, palmHigh5.y, palmHigh5.z);
    glTexCoord2f(1.0f, 0.75f); glVertex3f(backHigh5.x, backHigh5.y, backHigh5.z);
    
    glTexCoord2f(1.0f, 0.75f); glVertex3f(backHigh5.x, backHigh5.y, backHigh5.z);
    glTexCoord2f(1.0f, 0.5f); glVertex3f(backMid2.x, backMid2.y, backMid2.z);
    glTexCoord2f(1.0f, 0.5f); glVertex3f(palmMid2.x, palmMid2.y, palmMid2.z);
    
    // Front wall (wrist edge)
    normalSide = {0, 0, -1};
    glNormal3f(normalSide.x, normalSide.y, normalSide.z);
    
    glTexCoord2f(0.0f, 0.0f); glVertex3f(palmBase1.x, palmBase1.y, palmBase1.z);
    glTexCoord2f(0.5f, 0.0f); glVertex3f(palmBase3.x, palmBase3.y, palmBase3.z);
    glTexCoord2f(0.5f, 0.0f); glVertex3f(backBase3.x, backBase3.y, backBase3.z);
    
    glTexCoord2f(0.5f, 0.0f); glVertex3f(backBase3.x, backBase3.y, backBase3.z);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(backBase1.x, backBase1.y, backBase1.z);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(palmBase1.x, palmBase1.y, palmBase1.z);
    
    glTexCoord2f(0.5f, 0.0f); glVertex3f(palmBase3.x, palmBase3.y, palmBase3.z);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(palmBase2.x, palmBase2.y, palmBase2.z);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(backBase2.x, backBase2.y, backBase2.z);
    
    glTexCoord2f(1.0f, 0.0f); glVertex3f(backBase2.x, backBase2.y, backBase2.z);
    glTexCoord2f(0.5f, 0.0f); glVertex3f(backBase3.x, backBase3.y, backBase3.z);
    glTexCoord2f(0.5f, 0.0f); glVertex3f(palmBase3.x, palmBase3.y, palmBase3.z);
    
    // INTEGRATED PALM SURFACE - Natural hand skin feel
    Vec3 normalPalm = {0, 0.8f, 0.6f}; // More natural normal direction
    glNormal3f(normalPalm.x, normalPalm.y, normalPalm.z);
    
    // Create a naturally integrated palm surface that feels like real hand skin
    // Positions should blend seamlessly with existing hand geometry
    
    // Define integrated palm vertices (natural positioning, not layered on top)
    float naturalBlend = 0.02f; // Subtle forward positioning for natural feel
    float seamlessHeight = 0.01f; // Minimal height adjustment for seamless integration
    
    // Natural palm base (wrist area) - seamlessly integrated
    Vec3 naturalBase1 = {palmBase1.x - 0.05f, palmBase1.y + seamlessHeight, palmBase1.z + naturalBlend};
    Vec3 naturalBase2 = {palmBase2.x + 0.05f, palmBase2.y + seamlessHeight, palmBase2.z + naturalBlend};
    Vec3 naturalBase3 = {palmBase3.x - 0.02f, palmBase3.y + seamlessHeight, palmBase3.z + naturalBlend};
    Vec3 naturalBase4 = {palmBase4.x + 0.02f, palmBase4.y + seamlessHeight, palmBase4.z + naturalBlend};
    Vec3 naturalBase5 = {palmBase5.x, palmBase5.y + seamlessHeight, palmBase5.z + naturalBlend};
    
    // Natural knuckle area (where fingers connect) - integrated with finger bases
    Vec3 naturalKnuckle1 = {pinkyBase.x - 0.02f, pinkyBase.y + seamlessHeight*2, pinkyBase.z + naturalBlend*0.5f};
    Vec3 naturalKnuckle2 = {ringBase.x - 0.01f, ringBase.y + seamlessHeight*2, ringBase.z + naturalBlend*0.5f};
    Vec3 naturalKnuckle3 = {middleBase.x, middleBase.y + seamlessHeight*2, middleBase.z + naturalBlend*0.5f};
    Vec3 naturalKnuckle4 = {indexBase.x + 0.01f, indexBase.y + seamlessHeight*2, indexBase.z + naturalBlend*0.5f};
    Vec3 naturalKnuckle5 = {thumbBase.x + 0.02f, thumbBase.y + seamlessHeight*1.5f, thumbBase.z + naturalBlend*0.3f};
    
    // Natural mid-palm area - smooth transition between base and knuckles
    Vec3 naturalMid1 = {palmMid1.x - 0.03f, palmMid1.y + seamlessHeight*1.5f, palmMid1.z + naturalBlend*0.8f};
    Vec3 naturalMid2 = {palmMid2.x + 0.03f, palmMid2.y + seamlessHeight*1.5f, palmMid2.z + naturalBlend*0.8f};
    Vec3 naturalMid3 = {palmMid3.x - 0.015f, palmMid3.y + seamlessHeight*1.5f, palmMid3.z + naturalBlend*0.8f};
    Vec3 naturalMid4 = {palmMid4.x - 0.005f, palmMid4.y + seamlessHeight*1.5f, palmMid4.z + naturalBlend*0.8f};
    Vec3 naturalMid5 = {palmMid5.x + 0.005f, palmMid5.y + seamlessHeight*1.5f, palmMid5.z + naturalBlend*0.8f};
    Vec3 naturalMid6 = {palmMid6.x + 0.015f, palmMid6.y + seamlessHeight*1.5f, palmMid6.z + naturalBlend*0.8f};
    
    // SOLID FRONT SURFACE TRIANGLES - Complete coverage
    
    // Base to Mid front coverage
    // NATURAL PALM SURFACE TRIANGULATION - Seamless integration
    glColor4f(0.9f, 0.7f, 0.6f, 1.0f); // Natural hand color
    
    // Base palm surface with natural skin feel (not layered on top)
    glTexCoord2f(0.0f, 0.0f); glVertex3f(naturalBase1.x, naturalBase1.y, naturalBase1.z);
    glTexCoord2f(0.33f, 0.0f); glVertex3f(naturalBase3.x, naturalBase3.y, naturalBase3.z);
    glTexCoord2f(0.33f, 0.5f); glVertex3f(naturalMid3.x, naturalMid3.y, naturalMid3.z);
    
    glTexCoord2f(0.33f, 0.5f); glVertex3f(naturalMid3.x, naturalMid3.y, naturalMid3.z);
    glTexCoord2f(0.0f, 0.5f); glVertex3f(naturalMid1.x, naturalMid1.y, naturalMid1.z);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(naturalBase1.x, naturalBase1.y, naturalBase1.z);
    
    glTexCoord2f(0.33f, 0.0f); glVertex3f(naturalBase3.x, naturalBase3.y, naturalBase3.z);
    glTexCoord2f(0.5f, 0.0f); glVertex3f(naturalBase5.x, naturalBase5.y, naturalBase5.z);
    glTexCoord2f(0.5f, 0.5f); glVertex3f(naturalMid4.x, naturalMid4.y, naturalMid4.z);
    
    glTexCoord2f(0.5f, 0.5f); glVertex3f(naturalMid4.x, naturalMid4.y, naturalMid4.z);
    glTexCoord2f(0.33f, 0.5f); glVertex3f(naturalMid3.x, naturalMid3.y, naturalMid3.z);
    glTexCoord2f(0.33f, 0.0f); glVertex3f(naturalBase3.x, naturalBase3.y, naturalBase3.z);
    
    glTexCoord2f(0.5f, 0.0f); glVertex3f(naturalBase5.x, naturalBase5.y, naturalBase5.z);
    glTexCoord2f(0.66f, 0.0f); glVertex3f(naturalBase4.x, naturalBase4.y, naturalBase4.z);
    glTexCoord2f(0.66f, 0.5f); glVertex3f(naturalMid5.x, naturalMid5.y, naturalMid5.z);
    
    glTexCoord2f(0.66f, 0.5f); glVertex3f(naturalMid5.x, naturalMid5.y, naturalMid5.z);
    glTexCoord2f(0.5f, 0.5f); glVertex3f(naturalMid4.x, naturalMid4.y, naturalMid4.z);
    glTexCoord2f(0.5f, 0.0f); glVertex3f(naturalBase5.x, naturalBase5.y, naturalBase5.z);
    
    glTexCoord2f(0.66f, 0.0f); glVertex3f(naturalBase4.x, naturalBase4.y, naturalBase4.z);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(naturalBase2.x, naturalBase2.y, naturalBase2.z);
    glTexCoord2f(1.0f, 0.5f); glVertex3f(naturalMid6.x, naturalMid6.y, naturalMid6.z);
    
    glTexCoord2f(1.0f, 0.5f); glVertex3f(naturalMid6.x, naturalMid6.y, naturalMid6.z);
    glTexCoord2f(0.66f, 0.5f); glVertex3f(naturalMid5.x, naturalMid5.y, naturalMid5.z);
    glTexCoord2f(0.66f, 0.0f); glVertex3f(naturalBase4.x, naturalBase4.y, naturalBase4.z);
    
    glTexCoord2f(1.0f, 0.0f); glVertex3f(naturalBase2.x, naturalBase2.y, naturalBase2.z);
    glTexCoord2f(1.0f, 0.5f); glVertex3f(naturalMid2.x, naturalMid2.y, naturalMid2.z);
    glTexCoord2f(1.0f, 0.5f); glVertex3f(naturalMid6.x, naturalMid6.y, naturalMid6.z);
    
    // Natural knuckle transitions (seamless with finger connections)
    glTexCoord2f(0.0f, 0.5f); glVertex3f(naturalMid1.x, naturalMid1.y, naturalMid1.z);
    glTexCoord2f(0.2f, 0.8f); glVertex3f(naturalKnuckle1.x, naturalKnuckle1.y, naturalKnuckle1.z);
    glTexCoord2f(0.33f, 0.5f); glVertex3f(naturalMid3.x, naturalMid3.y, naturalMid3.z);
    
    glTexCoord2f(0.33f, 0.5f); glVertex3f(naturalMid3.x, naturalMid3.y, naturalMid3.z);
    glTexCoord2f(0.2f, 0.8f); glVertex3f(naturalKnuckle1.x, naturalKnuckle1.y, naturalKnuckle1.z);
    glTexCoord2f(0.4f, 0.8f); glVertex3f(naturalKnuckle2.x, naturalKnuckle2.y, naturalKnuckle2.z);
    
    glTexCoord2f(0.33f, 0.5f); glVertex3f(naturalMid3.x, naturalMid3.y, naturalMid3.z);
    glTexCoord2f(0.4f, 0.8f); glVertex3f(naturalKnuckle2.x, naturalKnuckle2.y, naturalKnuckle2.z);
    glTexCoord2f(0.5f, 0.5f); glVertex3f(naturalMid4.x, naturalMid4.y, naturalMid4.z);
    
    glTexCoord2f(0.5f, 0.5f); glVertex3f(naturalMid4.x, naturalMid4.y, naturalMid4.z);
    glTexCoord2f(0.4f, 0.8f); glVertex3f(naturalKnuckle2.x, naturalKnuckle2.y, naturalKnuckle2.z);
    glTexCoord2f(0.6f, 0.8f); glVertex3f(naturalKnuckle3.x, naturalKnuckle3.y, naturalKnuckle3.z);
    
    glTexCoord2f(0.5f, 0.5f); glVertex3f(naturalMid4.x, naturalMid4.y, naturalMid4.z);
    glTexCoord2f(0.6f, 0.8f); glVertex3f(naturalKnuckle3.x, naturalKnuckle3.y, naturalKnuckle3.z);
    glTexCoord2f(0.66f, 0.5f); glVertex3f(naturalMid5.x, naturalMid5.y, naturalMid5.z);
    
    glTexCoord2f(0.66f, 0.5f); glVertex3f(naturalMid5.x, naturalMid5.y, naturalMid5.z);
    glTexCoord2f(0.6f, 0.8f); glVertex3f(naturalKnuckle3.x, naturalKnuckle3.y, naturalKnuckle3.z);
    glTexCoord2f(0.8f, 0.8f); glVertex3f(naturalKnuckle4.x, naturalKnuckle4.y, naturalKnuckle4.z);
    
    glTexCoord2f(0.66f, 0.5f); glVertex3f(naturalMid5.x, naturalMid5.y, naturalMid5.z);
    glTexCoord2f(0.8f, 0.8f); glVertex3f(naturalKnuckle4.x, naturalKnuckle4.y, naturalKnuckle4.z);
    glTexCoord2f(1.0f, 0.5f); glVertex3f(naturalMid6.x, naturalMid6.y, naturalMid6.z);
    
    glTexCoord2f(1.0f, 0.5f); glVertex3f(naturalMid6.x, naturalMid6.y, naturalMid6.z);
    glTexCoord2f(0.8f, 0.8f); glVertex3f(naturalKnuckle4.x, naturalKnuckle4.y, naturalKnuckle4.z);
    glTexCoord2f(1.0f, 0.8f); glVertex3f(naturalKnuckle5.x, naturalKnuckle5.y, naturalKnuckle5.z);
    
    glTexCoord2f(1.0f, 0.5f); glVertex3f(naturalMid6.x, naturalMid6.y, naturalMid6.z);
    glTexCoord2f(1.0f, 0.8f); glVertex3f(naturalKnuckle5.x, naturalKnuckle5.y, naturalKnuckle5.z);
    glTexCoord2f(1.0f, 0.5f); glVertex3f(naturalMid2.x, naturalMid2.y, naturalMid2.z);
    
    // Additional smooth surface coverage for seamless integration
    glTexCoord2f(0.2f, 0.2f); glVertex3f(naturalBase5.x, naturalBase5.y, naturalBase5.z);
    glTexCoord2f(0.4f, 0.4f); glVertex3f(naturalMid3.x, naturalMid3.y, naturalMid3.z);
    glTexCoord2f(0.6f, 0.4f); glVertex3f(naturalMid4.x, naturalMid4.y, naturalMid4.z);
    
    glTexCoord2f(0.6f, 0.4f); glVertex3f(naturalMid4.x, naturalMid4.y, naturalMid4.z);
    glTexCoord2f(0.8f, 0.4f); glVertex3f(naturalMid5.x, naturalMid5.y, naturalMid5.z);
    glTexCoord2f(0.5f, 0.2f); glVertex3f(naturalBase5.x, naturalBase5.y, naturalBase5.z);
    
    glTexCoord2f(0.5f, 0.2f); glVertex3f(naturalBase5.x, naturalBase5.y, naturalBase5.z);
    glTexCoord2f(0.8f, 0.4f); glVertex3f(naturalMid5.x, naturalMid5.y, naturalMid5.z);
    glTexCoord2f(0.8f, 0.2f); glVertex3f(naturalBase4.x, naturalBase4.y, naturalBase4.z);
    
    glEnd();
}

static void drawHand() {
    if (g_HandJoints.empty()) return;
    
    // Enable texturing if enabled
    if (g_TextureEnabled) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, g_HandTexture);
        // Set color to white to show texture clearly
        glColor3f(1.0f, 1.0f, 1.0f);
    } else {
        glDisable(GL_TEXTURE_2D);
        // Set skin color for non-textured mode
        glColor3f(0.9f, 0.8f, 0.7f);
    }
    
    // Draw first hand and arm
    // Draw low-poly arm first (behind hand)
    drawLowPolyArm();
    
    // Re-enable texture for hand if needed
    if (g_TextureEnabled) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, g_HandTexture);
        glColor3f(1.0f, 1.0f, 1.0f);
    } else {
        glDisable(GL_TEXTURE_2D);
        glColor3f(0.9f, 0.8f, 0.7f);
    }
    
    // Draw connection piece between arm and palm for seamless integration
    Vec3 armConnection = {0.0f, 0.0f, 0.05f};  // Connection point
    Vec3 wristPos = g_HandJoints[0].position;  // Hand wrist position
    drawRobotArmSegment(armConnection, wristPos, 0.85f, 0.28f);  // Realistic wrist connection
    
    // Draw low-poly palm
    drawSkinnedPalm();
    
    // Draw low-poly thumb
    drawLowPolyThumb();
    
    // Draw low-poly fingers
    drawLowPolyFinger(5, 6, 7, 8);   // Index finger
    drawLowPolyFinger(9, 10, 11, 12); // Middle finger
    drawLowPolyFinger(13, 14, 15, 16); // Ring finger
    drawLowPolyFinger(17, 18, 19, 20); // Pinky finger

    // Disable texturing for joints
    glDisable(GL_TEXTURE_2D);

    // Optional: Draw joints as small cubes for low-poly style
    glColor3f(0.8f, 0.7f, 0.6f); // Darker for joint definition
    for (const auto& joint : g_HandJoints) {
        drawJoint(joint.position, 0.015f); // Smaller joints
    }
    
    // Draw second hand and arm if available
    if (!g_HandJoints2.empty()) {
        // Enable texturing if enabled
        if (g_TextureEnabled) {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, g_HandTexture);
            glColor3f(1.0f, 1.0f, 1.0f);
        } else {
            glDisable(GL_TEXTURE_2D);
            glColor3f(0.9f, 0.8f, 0.7f);
        }
        
        // Draw second arm
        drawLowPolyArm2();
        
        // Re-enable texture for second hand if needed
        if (g_TextureEnabled) {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, g_HandTexture);
            glColor3f(1.0f, 1.0f, 1.0f);
        } else {
            glDisable(GL_TEXTURE_2D);
            glColor3f(0.9f, 0.8f, 0.7f);
        }
        
        // Draw connection piece between arm and palm for second hand
        Vec3 armConnection2 = {4.0f, 0.0f, 0.05f};  // Connection point for second hand
        Vec3 wristPos2 = g_HandJoints2[0].position;  // Second hand wrist position
        drawRobotArmSegment(armConnection2, wristPos2, 0.85f, 0.28f);  // Realistic wrist connection
        
        // Draw second hand palm
        drawSkinnedPalm2();
        
        // Draw second hand thumb
        drawLowPolyThumb2();
        
        // Draw second hand fingers
        drawLowPolyFinger2(5, 6, 7, 8);   // Index finger
        drawLowPolyFinger2(9, 10, 11, 12); // Middle finger
        drawLowPolyFinger2(13, 14, 15, 16); // Ring finger
        drawLowPolyFinger2(17, 18, 19, 20); // Pinky finger

        // Disable texturing for joints
        glDisable(GL_TEXTURE_2D);

        // Optional: Draw joints as small cubes for low-poly style
        glColor3f(0.8f, 0.7f, 0.6f); // Darker for joint definition
        for (const auto& joint : g_HandJoints2) {
            drawJoint(joint.position, 0.015f); // Smaller joints
        }
    }
}

static void renderScene() {
    glClearColor(0.1f, 0.1f, 0.15f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Camera transforms
    glTranslatef(g_PanX, g_PanY, 0.0f);
    glTranslatef(0.0f, 0.0f, -g_CamDist);
    glRotatef(g_OrbitPitchDeg, 1, 0, 0);
    glRotatef(g_OrbitYawDeg, 0, 1, 0);

    // Draw hand
    drawHand();
}
// =============================================================

// ========================= Controls ===========================
static void ResetCamera() {
    g_OrbitYawDeg = 0.0f;
    g_OrbitPitchDeg = 15.0f;
    g_PanX = g_PanY = 0.0f;
    g_CamDist = 5.0f;
}

static void OnMouseDragOrbit(int x, int y) {
    const float sens = 0.3f;
    int dx = x - g_LastMouseX;
    int dy = y - g_LastMouseY;
    g_LastMouseX = x; g_LastMouseY = y;

    g_OrbitYawDeg += dx * sens;
    g_OrbitPitchDeg += dy * sens;
    if (g_OrbitPitchDeg > 89.0f) g_OrbitPitchDeg = 89.0f;
    if (g_OrbitPitchDeg < -89.0f) g_OrbitPitchDeg = -89.0f;
}

static void OnMouseWheel(short delta) {
    if (delta > 0) g_CamDist *= 0.9f; 
    else g_CamDist *= 1.1f;
    if (g_CamDist < 0.5f) g_CamDist = 0.5f;
    if (g_CamDist > 50.0f) g_CamDist = 50.0f;
}

static void OnPanKey(WORD key) {
    const float step = 0.1f;
    if (key == 'W') g_PanY += step;
    if (key == 'S') g_PanY -= step;
    if (key == 'A') g_PanX -= step;
    if (key == 'D') g_PanX += step;
}
// =============================================================

// ======================= Win32 plumbing =======================
static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        g_hDC = GetDC(hWnd);
        if (!initPixelFormat(g_hDC)) return -1;
        g_hRC = wglCreateContext(g_hDC);
        if (!g_hRC) return -1;
        if (!wglMakeCurrent(g_hDC, g_hRC)) return -1;
        initGL();
        
        // Initialize hand and arm
        InitializeHand();
        InitializeArm();
        
        // Initialize second hand and arm
        InitializeHand2();
        InitializeArm2();
        
        ResetCamera();
        return 0;

    case WM_SIZE:
        resizeGL(LOWORD(lParam), HIWORD(lParam));
        return 0;

    case WM_LBUTTONDOWN:
        g_LButtonDown = true;
        g_LastMouseX = GET_X_LPARAM(lParam);
        g_LastMouseY = GET_Y_LPARAM(lParam);
        SetCapture(hWnd);
        return 0;

    case WM_LBUTTONUP:
        g_LButtonDown = false;
        ReleaseCapture();
        return 0;

    case WM_MOUSEMOVE:
        if (g_LButtonDown) {
            OnMouseDragOrbit(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            InvalidateRect(hWnd, nullptr, FALSE);
        }
        return 0;

    case WM_MOUSEWHEEL:
        OnMouseWheel((short)GET_WHEEL_DELTA_WPARAM(wParam));
        InvalidateRect(hWnd, nullptr, FALSE);
        return 0;

    case WM_KEYDOWN:
        switch (wParam) {
        case 'W': case 'S': case 'A': case 'D':
            OnPanKey(WORD(wParam));
            InvalidateRect(hWnd, nullptr, FALSE);
            break;

        case VK_F1:
            g_Wireframe = !g_Wireframe;
            applyPolygonMode();
            InvalidateRect(hWnd, nullptr, FALSE);
            break;

        case VK_F2: {
            // Toggle texture on/off
            g_TextureEnabled = !g_TextureEnabled;
            InvalidateRect(hWnd, nullptr, FALSE);
            break;
        }

        case VK_F3: {
            // Toggle between solid skin and joint visualization
            static bool showJoints = false;
            showJoints = !showJoints;
            // This could be used to show/hide joint spheres
            InvalidateRect(hWnd, nullptr, FALSE);
            break;
        }

        case 'R':
            ResetCamera();
            InvalidateRect(hWnd, nullptr, FALSE);
            break;

        case VK_ESCAPE:
            PostQuitMessage(0);
            break;

        default: break;
        }
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps; 
        BeginPaint(hWnd, &ps);
        renderScene();
        SwapBuffers(g_hDC);
        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_DESTROY:
        if (g_hRC) {
            wglMakeCurrent(nullptr, nullptr);
            wglDeleteContext(g_hRC);
            g_hRC = nullptr;
        }
        if (g_hDC) {
            ReleaseDC(hWnd, g_hDC);
            g_hDC = nullptr;
        }
        PostQuitMessage(0);
        return 0;

    default: break;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASSA wc = {};
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = "HandDrawingGL";
    RegisterClassA(&wc);

    RECT r = { 0,0,1280,720 };
    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);

    HWND hWnd = CreateWindowA(
        wc.lpszClassName, "Hand Drawing with OpenGL",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        r.right - r.left, r.bottom - r.top,
        nullptr, nullptr, hInst, nullptr);

    g_hWnd = hWnd;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}
