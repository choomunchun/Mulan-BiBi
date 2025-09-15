#define NOMINMAX
#include <Windows.h>
#include <windowsx.h>
#include <gl/GL.h>
#include <gl/GLU.h>
#define _USE_MATH_DEFINES
#include <cmath>
#include <vector>
#include <cstdio>
#include<algorithm>
#include <string>

#include "utils.h"
#include "spear.h"
#include "shield.h"
#include "armor.h"
#include "background.h"
#include "sword.h"
#include "texture.h"


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
// --- Rendering mode ---
enum RenderMode { RM_WIREFRAME, RM_SOLID, RM_TEXTURED };
RenderMode gRenderMode = RM_TEXTURED;   // default


bool  gLMBDown = false;
POINT gLastMouse = { 0, 0 };

// --- Camera State ---
Vec3  gTarget = { 0.0f, 3.5f, 0.0f };
float gDist = 15.0f;
float gYaw = 0.2f;
float gPitch = 0.1f;

// --- Projection State ---
enum ProjectionMode { PROJ_PERSPECTIVE = 0, PROJ_ORTHOGRAPHIC = 1 };
ProjectionMode gProjMode = PROJ_PERSPECTIVE;
bool gViewportMode = false; // false = full viewport, true = split viewport

// --- Input State ---
bool keyW = false, keyS = false, keyA = false, keyD = false; // For Camera
bool keyUp = false, keydown = false, keyLeft = false, keyRight = false, keyShift = false; // For Character
bool keyF = false; // For Fist Animation
bool keyG = false; // For K-pop Dance Animation
bool keyX = false; // For Sword Toggle
bool keySlash = false; // For Slow Arm Bend

// == Weapon controls
bool gSpearVisible = false;
bool gShieldVisible = false;
bool gWeaponInRightHand = true; // For both spear and sword
bool gArmorVisible = false;
float g_shoulderArmorScale = 1.0f; // NEW: Scale for shoulder armor
float g_helmetSideOffsetX = 0.0f;  // Offset for helmet side covers
float g_helmetSideOffsetY = 0.0f;
float g_helmetSideOffsetZ = 0.0f;
float g_helmetBackOffsetX = 0.0f;  // Offset for helmet back cover
float g_helmetBackOffsetY = 0.0f;
float g_helmetBackOffsetZ = 0.0f;
float g_helmetDomeRotationX = 0.0f; // Rotation for helmet dome
float g_helmetDomeRotationY = -55.0f;
float g_helmetDomeRotationZ = 0.0f;

// == Background controls
bool gBackgroundVisible = true; // Background starts visible
bool keyK = false; // For Background Toggle

LARGE_INTEGER gFreq = { 0 }, gPrev = { 0 };

// --- Leg Animation State ---
float gAnimTime = 0.0f;
float gWalkSpeed = 0.5f;
float gRunSpeed = 1.0f;
bool  gRunningAnim = false;
bool  gIsMoving = false;

// --- Proportions Control ---
#define BODY_SCALE 3.5f
#define ARM_SCALE 0.14f  // Increased for better coverage and gap elimination
#define HAND_SCALE 1.25f  // Increased from 0.15f for larger hands

// --- Jaw Configuration ---
float JAW_CURVATURE = 3.0f;      // Controls jaw curve smoothness (higher = smoother curve)
float JAW_WIDTH_MIN = 0.01f;      // Minimum jaw width at chin (0.4 = narrow, 0.8 = wide)
float JAW_TRANSITION = -0.1f;    // Y position where jaw narrowing begins (-0.2 = higher, -0.5 = lower)

// --- Mesh Quality Configuration ---
#define TORSO_SEGMENTS 24        // Number of segments around torso circumference (16 = default, 32 = high quality)
#define HEAD_SEGMENTS 20         // Number of horizontal segments around head circumference  
#define HEAD_LAYERS 24           // Number of vertical layers in head geometry

// --- Facial Features Scale Configuration ---
float EYE_SCALE = 1.2f;          // Controls eye size (0.5 = small, 1.0 = normal, 1.5 = large)
float NOSE_SCALE = 1.0f;         // Controls nose size (0.5 = small, 1.0 = normal, 1.5 = large)

// --- Arm Bend Configuration ---
float gLeftLowerArmBend = 0.0f;  // Bend angle for left lower arm (positive = bend up)
float gRightLowerArmBend = 0.0f; // Bend angle for right lower arm (positive = bend up)
const float MAX_ARM_BEND = 120.0f; // Maximum bend angle in degrees

// --- Slow Arm Bend Controls ---
float gSlowArmBendSpeed = 30.0f; // Degrees per second for slow arm bending
bool gSlowArmBendActive = false; // Whether slow arm bending is active

// --- Boxing Stance Animation ---
bool gInBoxingStance = false;    // Current stance state
bool gBoxingAnimActive = false;  // Animation in progress
float gBoxingAnimTime = 0.0f;    // Animation timer
const float BOXING_ANIM_DURATION = 1.0f; // Animation duration in seconds

// Relaxed pose (arms hanging down)
const float RELAXED_SHOULDER_PITCH = 0.0f;
const float RELAXED_SHOULDER_YAW = 0.0f;
const float RELAXED_SHOULDER_ROLL = 0.0f;
const float RELAXED_ELBOW_BEND = 0.0f;

// Boxing stance (guard position)
const float BOXING_SHOULDER_PITCH = 55.0f;   // Forward rotation 50-60°
const float BOXING_SHOULDER_YAW = -10.0f;    // Slight inward rotation
const float BOXING_SHOULDER_ROLL = 180.0f;   // 180-degree arm rotation
const float BOXING_ELBOW_BEND = 90.0f;       // Elbow bent 80-100°

// Current pose values (interpolated)
float gCurrentLeftShoulderPitch = 0.0f;
float gCurrentLeftShoulderYaw = 0.0f;
float gCurrentLeftShoulderRoll = 0.0f;
float gCurrentLeftElbowBend = 0.0f;
float gCurrentRightShoulderPitch = 0.0f;
float gCurrentRightShoulderYaw = 0.0f;
float gCurrentRightShoulderRoll = 0.0f;
float gCurrentRightElbowBend = 0.0f;

// Joint visualization
bool gShowJointVisuals = true;   // Show joint spheres/boxes
float MOUTH_SCALE = 1.2f;        // Controls mouth size (0.5 = small, 1.0 = normal, 1.5 = large)
float EYEBROW_SCALE = 1.0f;      // Controls eyebrow thickness (0.5 = thin, 1.0 = normal, 1.5 = thick)
float HAIR_SCALE = 1.0f;         // Controls hair volume (0.5 = flat, 1.0 = normal, 1.5 = voluminous)

// --- Animation & Character State ---
Vec3  gCharacterPos = { 0.0f, 0.0f, 0.0f };
float gCharacterYaw = 0.0f;
float gWalkPhase = 0.0f;
float gMoveSpeed = 0.0f;
const float WALK_SPEED = 1.8f;    // Enhanced: Slightly slower for more graceful walking
const float RUN_SPEED = 3.8f;     // Enhanced: Adjusted to maintain good speed ratio
const float TURN_SPEED = 120.0f;

// --- Fist Animation State ---
float gFistAnimationTime = 0.0f;
bool gFistAnimationActive = false;
bool gIsFist = false; // Current state: false = open hand, true = fist
const float FIST_ANIMATION_DURATION = 1.0f;

// --- K-pop Dance Animation State ---
bool gIsDancing = false;
bool gIsJumping = false; // Keep for compatibility with existing jump logic
float gJumpPhase = 0.0f;            // Jump phase for original jump function compatibility  
float gDancePhase = 0.0f;           // Current phase of dance (0.0 to 8.0 for 8-second dance)
float gDanceTime = 0.0f;            // Total dance time elapsed
float gJumpVerticalOffset = 0.0f;   // Current vertical position offset (used for dance jumps too)
const float DANCE_SPEED = 4.0f;     // Dance animation speed multiplier
const float DANCE_DURATION = 8.0f;  // Total dance sequence duration in seconds
const float JUMP_HEIGHT = 2.0f;     // Maximum jump height for dance moves
const float JUMP_DURATION = 0.8f;   // Keep original jump duration for compatibility

// --- Kung Fu Pattern State ---
int gKungFuPattern = 0; // 0 = normal, 1 = crane, 2 = dragon, 3 = tiger
bool gConcreteHands = false;

// --- Kung Fu Animation State ---
bool gKungFuAnimating = false;
float gKungFuAnimationTime = 0.0f;
int gKungFuAnimationPhase = 0;
const float KUNGFU_ANIMATION_SPEED = 2.0f;
const float PHASE_DURATION = 1.5f; // Duration of each animation phase

// --- Sword Attack Animation State ---
bool gSwordAttackAnimating = false;
float gSwordAttackAnimationTime = 0.0f;
int gSwordAttackPhase = 0; // 0 = ready, 1 = wind up, 2 = strike, 3 = follow through
const float SWORD_ATTACK_DURATION = 2.5f; // Total attack animation duration - much slower
const float SWORD_WINDUP_DURATION = 0.8f; // Wind up phase duration - slower dramatic overhead raise
const float SWORD_STRIKE_DURATION = 0.3f; // Strike phase duration - slower but still powerful
const float SWORD_FOLLOWTHROUGH_DURATION = 1.4f; // Follow through phase duration - longer recovery

// Sword attack pose offsets
float gSwordAttackTorsoRotation = 0.0f;
float gSwordAttackShoulderOffset = 0.0f;
float gSwordAttackArmExtension = 0.0f;
float gSwordAttackLegStance = 0.0f;

// --- Spear Attack Animation State ---
bool gSpearAttackAnimating = false;
float gSpearAttackAnimationTime = 0.0f;
int gSpearAttackPhase = 0; // 0 = ready, 1 = wind up, 2 = thrust, 3 = follow through
const float SPEAR_ATTACK_DURATION = 2.0f; // Total attack animation duration
const float SPEAR_WINDUP_DURATION = 0.6f; // Wind up phase duration
const float SPEAR_THRUST_DURATION = 0.4f; // Thrust phase duration
const float SPEAR_FOLLOWTHROUGH_DURATION = 1.0f; // Follow through phase duration

// Spear attack pose offsets
float gSpearAttackTorsoRotation = 0.0f;
float gSpearAttackShoulderOffset = 0.0f;
float gSpearAttackArmExtension = 0.0f;
float gSpearAttackLegStance = 0.0f;

// --- Shield Block Animation State ---
bool gShieldBlockAnimating = false;
float gShieldBlockAnimationTime = 0.0f;
int gShieldBlockPhase = 0; // 0 = ready, 1 = raise, 2 = hold, 3 = lower
const float SHIELD_BLOCK_DURATION = 1.5f; // Total block animation duration
const float SHIELD_RAISE_DURATION = 0.3f; // Raise phase duration
const float SHIELD_HOLD_DURATION = 0.6f; // Hold phase duration
const float SHIELD_LOWER_DURATION = 0.6f; // Lower phase duration

// Shield block pose offsets
float gShieldBlockArmRaise = 0.0f;
float gShieldBlockTorsoLean = 0.0f;
float gShieldBlockStance = 0.0f;

// --- Realistic Hand Form States ---
enum HandForm {
    HAND_OPEN = 0,
    HAND_FIST = 1,
    HAND_CLAW = 2,        // Tiger claw - fingers curved like claws
    HAND_SPEAR = 3,       // Snake hand - fingers straight and together
    HAND_CRANE_BEAK = 4,  // Crane beak - fingertips together
    HAND_SWORD_FINGER = 5, // Two fingers extended (sword hand)
    HAND_PALM_STRIKE = 6,   // Open palm ready for striking
    HAND_GRIP_SPEAR = 7,   // for spear
    HAND_GRIP_SWORD = 8    // for sword - proper sword grip
};

// --- Anime-style Animation Parameters ---
struct KungFuPose {
    float leftShoulderPitch, rightShoulderPitch;
    float leftShoulderYaw, rightShoulderYaw;
    float leftShoulderRoll, rightShoulderRoll;
    float leftArmAngle, rightArmAngle;
    float torsoYaw, torsoPitch, torsoRoll;
    HandForm leftHandForm, rightHandForm;  // Add hand forms
    float leftElbowBend, rightElbowBend;   // Individual elbow control
    float leftWristPitch, rightWristPitch; // Wrist positioning
};

// Animation keyframes for each style
KungFuPose gCraneSequence[4];
KungFuPose gDragonSequence[4];
KungFuPose gTigerSequence[4];
KungFuPose gCurrentPose, gTargetPose;

// =========================================================
// == 📍 ENHANCED: NATURAL WALKING ANIMATION CONTROLS ==
// =========================================================
const float WALK_ARM_SWING = 18.0f;      // Walking arm swing angle (natural)
const float RUN_ARM_SWING = 35.0f;       // Running arm swing angle (natural)
const float BODY_BOB_AMOUNT = 0.03f;     // How much the body moves up and down (natural)
const float HIP_SWAY_AMOUNT = 0.8f;      // Hip sway side-to-side (natural feminine walk)
const float SHOULDER_COUNTER_SWAY = 0.6f; // Shoulder counter-movement
const float HEAD_BOB_AMOUNT = 0.01f;     // Subtle head bobbing
const float STEP_TIMING_OFFSET = 0.1f;   // Offset between different body parts

// --- Data Structures ---

// Base mesh data (read-only aftfer creation)
std::vector<Vec3f> gAllVertices;
std::vector<std::vector<int>> gAllQuads;
std::vector<Tri>   gTris;
std::vector<Vec3f> gVertexNormals;
struct Vec2f { float u, v; };
std::vector<Vec2f> gTexCoords;
// These will be modified each frame to create the animation
std::vector<Vec3f> gAnimatedVertices;
std::vector<Vec3f> gAnimatedNormals;
struct JointPose { float torsoYaw, torsoPitch, torsoRoll; float headYaw, headPitch, headRoll; } g_pose = { 0,0,0, 0,0,0 };
struct HandJoint { Vec3 position; int parentIndex; };
struct ArmJoint { Vec3 position; int parentIndex; };

// --- Texture Support ---
GLuint g_HandTexture = 0; // texture name
BITMAP BMP; // bitmap structure
HBITMAP hBMP = NULL; // bitmap handle
bool g_TextureEnabled = true;

// --- Body/Head Data ---
GLUquadric* g_headQuadric = nullptr;
float* segCos = nullptr;
float* segSin = nullptr;

static void setRenderMode(RenderMode m) {
    gRenderMode = m;
    switch (gRenderMode) {
    case RM_WIREFRAME:
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_LIGHTING); // Wireframe is easier to see without lighting
        break;
    case RM_SOLID:
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDisable(GL_TEXTURE_2D);
        glEnable(GL_LIGHTING);
        break;
    case RM_TEXTURED:
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_LIGHTING);
        break;
    }

    // Enhanced texture loading from main2.cpp
    if (hBMP != NULL) {
        GetObject(hBMP, sizeof(BMP), &BMP);

        // Assign texture to polygon
        glEnable(GL_TEXTURE_2D);
        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, BMP.bmWidth, BMP.bmHeight, 0, GL_BGR, GL_UNSIGNED_BYTE, BMP.bmBits);

        DeleteObject(hBMP);
        hBMP = NULL;
    }
}

inline void bindConcreteTex() {
    if (g_TextureEnabled) {
        glEnable(GL_TEXTURE_2D);
        Tex::bind(Tex::id[Tex::Silver]); // or whatever slot your “concrete” uses
        glColor3f(1.0f, 1.0f, 1.0f);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    }
}

inline void unbindConcreteTex() {
    Tex::unbind();
}

inline void setSolidColorIfNeeded(float r, float g, float b) {
    if (gRenderMode != RM_TEXTURED) glColor3f(r, g, b); // TextureScope sets white for textured
}
// =============================================================

// --- Body/Head Data ---
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
// Dynamic segment arrays for configurable geometry

// Initialize segment arrays based on TORSO_SEGMENTS
void initializeSegmentArrays() {
    if (segCos) delete[] segCos;
    if (segSin) delete[] segSin;

    segCos = new float[TORSO_SEGMENTS];
    segSin = new float[TORSO_SEGMENTS];

    for (int i = 0; i < TORSO_SEGMENTS; i++) {
        float angle = 2.0f * PI * i / TORSO_SEGMENTS;
        segCos[i] = cos(angle);
        segSin[i] = sin(angle);
    }
}

#define HEAD_CENTER_Y 1.92f  // Perfectly centered on torso middle, sits naturally on neck
#define HEAD_RADIUS   0.3f
#define QUAD(r1,y1,r2,y2,cA,sA,cB,sB) glVertex3f((r1)*(cA),(y1),(r1)*(sA));glVertex3f((r2)*(cA),(y2),(r2)*(sA));glVertex3f((r2)*(cB),(y2),(r2)*(sB));glVertex3f((r1)*(cA),(y1),(r1)*(sA));glVertex3f((r2)*(cB),(y2),(r2)*(sB));glVertex3f((r1)*(cB),(y1),(r1)*(sB));
#define BAND(rA,yA,rB,yB) QUAD(rA,yA,rB,yB,C0,S0,C1,S1) QUAD(rA,yA,rB,yB,C1,S1,C2,S2) QUAD(rA,yA,rB,yB,C2,S2,C3,S3) QUAD(rA,yA,rB,yB,C3,S3,C4,S4) QUAD(rA,yA,rB,yB,C4,S4,C5,S5) QUAD(rA,yA,rB,yB,C5,S5,C6,S6) QUAD(rA,yA,rB,yB,C6,S6,C7,S7) QUAD(rA,yA,rB,yB,C7,S7,C8,S8) QUAD(rA,yA,rB,yB,C8,S8,C9,S9) QUAD(rA,yA,rB,yB,C9,S9,C10,S10) QUAD(rA,yA,rB,yB,C10,S10,C11,S11) QUAD(rA,yA,rB,yB,C11,S11,C12,S12) QUAD(rA,yA,rB,yB,C12,S12,C13,S13) QUAD(rA,yA,rB,yB,C13,S13,C14,S14) QUAD(rA,yA,rB,yB,C14,S14,C15,S15) QUAD(rA,yA,rB,yB,C15,S15,C0,S0)

// --- Arm/Hand Data ---
std::vector<HandJoint> g_HandJoints;
std::vector<HandJoint> g_HandJoints2;
std::vector<ArmJoint> g_ArmJoints;
std::vector<ArmJoint> g_ArmJoints2;

// ===================================================================
//
// SWORD MODEL INTEGRATION
//
// ===================================================================

// --- Sword State Variables ---
bool gSwordVisible = false;
bool gSwordInRightHand = true; // true = right hand, false = left hand
float gSwordScale = 0.3f; // Scale down the sword to fit in hand

// SWORD CONTROLS:
// Press 'X' to toggle sword visibility
// Press 'Z' to perform warrior sword attack animation
// When holding sword, the hand automatically forms a fist for proper grip

// Define materials for our sword parts (implementations for extern declarations in sword.h)
SwordMaterial polishedSteel = {
    {0.23f, 0.23f, 0.23f, 1.0f}, {0.77f, 0.77f, 0.77f, 1.0f},
    {0.99f, 0.99f, 0.99f, 1.0f}, 100.0f
};
SwordMaterial darkJade = {
    {0.0f, 0.1f, 0.05f, 1.0f}, {0.1f, 0.35f, 0.2f, 1.0f},
    {0.45f, 0.55f, 0.45f, 1.0f}, 32.0f
};
SwordMaterial hiltWrap = {
    {0.08f, 0.05f, 0.05f, 1.0f}, {0.2f, 0.15f, 0.15f, 1.0f},
    {0.1f, 0.1f, 0.1f, 1.0f}, 10.0f
};

// Helper function to apply a material
void setSwordMaterial(const SwordMaterial& mat) {
    glMaterialfv(GL_FRONT, GL_AMBIENT, mat.ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, mat.diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat.specular);
    glMaterialf(GL_FRONT, GL_SHININESS, mat.shininess);
}

// Put this at file-scope, above your draw* functions
struct TextureScope {
    bool didBind = false;
    bool useTexGen = false;
    TextureScope(GLuint tex, bool enableTexGen = false,
        float sScale = 0.35f, float tScale = 0.35f, float rScale = 0.35f)
        : useTexGen(enableTexGen)
    {
        if (gRenderMode == RM_TEXTURED) {
            glEnable(GL_TEXTURE_2D);
            Tex::bind(tex);
            glColor3f(1.f, 1.f, 1.f);
            didBind = true;
            if (useTexGen) Tex::enableObjectLinearST(sScale, tScale, rScale);
        }
    }
    ~TextureScope() {
        if (!didBind) return;
        if (useTexGen) Tex::disableObjectLinearST();
        Tex::unbind();
    }
};


// --- Sword Drawing Functions ---
void drawSwordSphere(double r, int lats, int longs) {
    for (int i = 0; i <= lats; i++) {
        double lat0 = PI * (-0.5 + (double)(i - 1) / lats);
        double z0 = r * sin(lat0); double zr0 = r * cos(lat0);
        double lat1 = PI * (-0.5 + (double)i / lats);
        double z1 = r * sin(lat1); double zr1 = r * cos(lat1);
        glBegin(GL_TRIANGLE_STRIP);
        for (int j = 0; j <= longs; j++) {
            double lng = 2 * PI * (double)(j - 1) / longs;
            double x = cos(lng); double y = sin(lng);
            glNormal3f(x * zr0, y * zr0, z0); glVertex3f(x * zr0, y * zr0, z0);
            glNormal3f(x * zr1, y * zr1, z1); glVertex3f(x * zr1, y * zr1, z1);
        }
        glEnd();
    }
}

void drawSwordCylinder(double r, double h, int slices) {
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= slices; i++) {
        float angle = 2.0f * PI * i / slices;
        float x = cos(angle); float z = sin(angle);
        glNormal3f(x, 0.0f, z);
        glVertex3f(r * x, h, r * z); glVertex3f(r * x, 0.0f, r * z);
    }
    glEnd();
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0.0f, 1.0f, 0.0f); glVertex3f(0.0f, h, 0.0f);
    for (int i = 0; i <= slices; i++) {
        float angle = 2.0f * PI * i / slices;
        glVertex3f(r * cos(angle), h, r * sin(angle));
    }
    glEnd();
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0.0f, -1.0f, 0.0f); glVertex3f(0.0f, 0.0f, 0.0f);
    for (int i = slices; i >= 0; i--) {
        float angle = 2.0f * PI * i / slices;
        glVertex3f(r * cos(angle), 0.0f, r * sin(angle));
    }
    glEnd();
}

void drawSwordInscription() {
    glDisable(GL_LIGHTING);
    glColor3f(0.1f, 0.1f, 0.1f); glLineWidth(2.0f);
    glPushMatrix();
    glTranslatef(0.0f, 0.5f, 0.051f);
    glScalef(0.05f, 0.05f, 0.05f);
    glBegin(GL_LINES); // 忠
    glVertex2f(-1, -1); glVertex2f(1, -1); glVertex2f(1, -1); glVertex2f(1, 1);
    glVertex2f(1, 1); glVertex2f(-1, 1); glVertex2f(-1, 1); glVertex2f(-1, -1);
    glVertex2f(0, -1); glVertex2f(0, 1);
    glEnd();
    glTranslatef(0.0f, -3.0f, 0.0f);
    glBegin(GL_LINES); // 勇
    glVertex2f(0, 1); glVertex2f(-1, 0.5); glVertex2f(0, 1); glVertex2f(1, 0.5);
    glVertex2f(-0.5, 0); glVertex2f(0.5, 0); glVertex2f(-1.2, -0.5); glVertex2f(1.2, -0.5);
    glVertex2f(0, -0.5); glVertex2f(-0.5, -1);
    glEnd();
    glTranslatef(0.0f, -3.0f, 0.0f);
    glBegin(GL_LINES); // 真
    glVertex2f(0, 1); glVertex2f(0, 0); glVertex2f(-1, 1); glVertex2f(1, 1);
    glVertex2f(-1, 0); glVertex2f(1, 0); glVertex2f(-0.8, -0.5); glVertex2f(-0.2, -1);
    glVertex2f(0.8, -0.5); glVertex2f(0.2, -1);
    glEnd();
    glPopMatrix();
    glEnable(GL_LIGHTING);
}

void drawSwordBlade() {
    setSwordMaterial(polishedSteel);
    float bladeLength = 5.0f; float bladeWidth = 0.2f; float bladeThickness = 0.05f;
    glBegin(GL_QUADS);
    // Right face with texture coordinates
    glNormal3f(0.5f, 0.125f, 0.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(0.0f, 0.0f, bladeThickness);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(bladeWidth, 0.0f, 0.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(bladeWidth, bladeLength, 0.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(0.0f, bladeLength, bladeThickness);
    
    glNormal3f(0.5f, -0.125f, 0.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(bladeWidth, 0.0f, 0.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(0.0f, 0.0f, -bladeThickness);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(0.0f, bladeLength, -bladeThickness);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(bladeWidth, bladeLength, 0.0f);
    
    // Left face with texture coordinates
    glNormal3f(-0.5f, 0.125f, 0.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(0.0f, 0.0f, bladeThickness);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-bladeWidth, 0.0f, 0.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-bladeWidth, bladeLength, 0.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(0.0f, bladeLength, bladeThickness);
    
    glNormal3f(-0.5f, -0.125f, 0.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-bladeWidth, 0.0f, 0.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(0.0f, 0.0f, -bladeThickness);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(0.0f, bladeLength, -bladeThickness);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-bladeWidth, bladeLength, 0.0f);
    glEnd();
    
    glBegin(GL_TRIANGLES);
    float tipLength = 0.5f;
    // Tip triangles with texture coordinates
    glNormal3f(0.5f, 0.5f, 0.0f);
    glTexCoord2f(0.5f, 1.0f); glVertex3f(0.0f, bladeLength + tipLength, 0.0f);
    glTexCoord2f(1.0f, 0.8f); glVertex3f(bladeWidth, bladeLength, 0.0f);
    glTexCoord2f(0.0f, 0.8f); glVertex3f(0.0f, bladeLength, bladeThickness);
    
    glNormal3f(0.5f, -0.5f, 0.0f);
    glTexCoord2f(0.5f, 1.0f); glVertex3f(0.0f, bladeLength + tipLength, 0.0f);
    glTexCoord2f(0.0f, 0.8f); glVertex3f(0.0f, bladeLength, -bladeThickness);
    glTexCoord2f(1.0f, 0.8f); glVertex3f(bladeWidth, bladeLength, 0.0f);
    
    glNormal3f(-0.5f, 0.5f, 0.0f);
    glTexCoord2f(0.5f, 1.0f); glVertex3f(0.0f, bladeLength + tipLength, 0.0f);
    glTexCoord2f(0.0f, 0.8f); glVertex3f(-bladeWidth, bladeLength, 0.0f);
    glTexCoord2f(1.0f, 0.8f); glVertex3f(0.0f, bladeLength, bladeThickness);
    
    glNormal3f(-0.5f, -0.5f, 0.0f);
    glTexCoord2f(0.5f, 1.0f); glVertex3f(0.0f, bladeLength + tipLength, 0.0f);
    glTexCoord2f(1.0f, 0.8f); glVertex3f(0.0f, bladeLength, -bladeThickness);
    glTexCoord2f(0.0f, 0.8f); glVertex3f(-bladeWidth, bladeLength, 0.0f);
    glEnd();
    drawSwordInscription();
}

void drawSwordGuard() {
    setSwordMaterial(darkJade);
    float guardWidth = 0.6f; float guardHeight = 0.2f; float guardDepth = 0.1f; int segments = 10;
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= segments; ++i) {
        float angle = (PI / segments) * i;
        float x = guardWidth * cos(angle); float y = guardHeight * sin(angle) - (guardHeight * 0.5f);
        glNormal3f(cos(angle), sin(angle), 0.0f);
        glVertex3f(x, y, guardDepth); glVertex3f(x, y, -guardDepth);
    }
    glEnd();
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= segments; ++i) {
        float angle = (PI / segments) * i;
        float x = -guardWidth * cos(angle); float y = guardHeight * sin(angle) - (guardHeight * 0.5f);
        glNormal3f(-cos(angle), sin(angle), 0.0f);
        glVertex3f(x, y, guardDepth); glVertex3f(x, y, -guardDepth);
    }
    glEnd();
}

void drawSwordHilt() {
    setSwordMaterial(hiltWrap);
    drawSwordCylinder(0.1f, 1.2f, 20);
    glDisable(GL_LIGHTING);
    glColor3f(0.2f, 0.4f, 0.25f);
    float wrapRadius = 0.1f * 1.05f; float wrapWidth = 0.02f;
    glBegin(GL_QUAD_STRIP);
    for (float i = 0; i < 1.2f; i += 0.05f) {
        float angle = i * 15.0f;
        glVertex3f(wrapRadius * cos(angle), i, wrapRadius * sin(angle));
        glVertex3f((wrapRadius - wrapWidth) * cos(angle), i, (wrapRadius - wrapWidth) * sin(angle));
    }
    glEnd();
    glEnable(GL_LIGHTING);
}

void drawSwordPommel() {
    setSwordMaterial(darkJade);
    glPushMatrix();
    glScalef(1.0f, 0.7f, 1.0f);
    drawSwordSphere(0.18f, 30, 30);
    glPopMatrix();
}

void drawSwordTassel() {
    glPushMatrix();
    glTranslatef(0.0f, -0.05f, 0.0f);
    setSwordMaterial(darkJade);
    drawSwordSphere(0.05f, 20, 20);
    glPopMatrix();
    glDisable(GL_LIGHTING);
    glColor3f(0.9f, 0.2f, 0.3f); glLineWidth(2.5f);
    glBegin(GL_LINES);
    for (int i = 0; i < 12; ++i) {
        float angle = 2 * PI * i / 12.0f;
        glVertex3f(0.0f, -0.1f, 0.0f);
        glVertex3f(0.2f * cos(angle), -0.8f, 0.2f * sin(angle));
    }
    glEnd();
    glEnable(GL_LIGHTING);
}

// Main sword drawing function
void drawSword() {
    if (!gSwordVisible) return;

    glPushMatrix();

    // Apply weapon texture to the entire sword
    bool usingTexture = (gRenderMode == RM_TEXTURED);
    if (usingTexture) {
        Tex::bind(Tex::id[Tex::Weapon]);
        glEnable(GL_TEXTURE_2D);
        Tex::enableObjectLinearST(0.3f, 0.8f, 0.3f); // Scale texture coordinates for sword
        glColor3f(1.0f, 1.0f, 1.0f); // White color to show texture properly
    }

    // Scale the sword to fit in hand
    glScalef(gSwordScale, gSwordScale, gSwordScale);

    // Position sword components relative to grip point (0,0,0)
    glTranslatef(0.0, -3.0, 0.0); // Move so grip is at origin

    // Render sword components
    drawSwordBlade();
    drawSwordGuard();
    glPushMatrix();
    glTranslatef(0.0, -1.2, 0.0);
    drawSwordHilt();
    glPopMatrix();
    glPushMatrix();
    glTranslatef(0.0, -1.3, 0.0);
    drawSwordPommel();
    drawSwordTassel();
    glPopMatrix();

    // Disable texture after rendering
    if (usingTexture) {
        Tex::disableObjectLinearST();
        Tex::unbind();
        glDisable(GL_TEXTURE_2D);
    }

    glPopMatrix();
}

// ===================================================================
//
// SECTION 2: HELPER & SETUP FUNCTIONS
//
// ===================================================================

// --- Forward Declarations ---
LRESULT WINAPI WindowProcedure(HWND, UINT, WPARAM, LPARAM);
void display();
void updateCharacter(float dt);
void drawBodyAndHead(float leftLegAngle, float rightLegAngle, float leftArmAngle, float rightArmAngle); // Updated signature
void drawArmsAndHands(float leftArmAngle, float rightArmAngle);
void drawInternalShoulderJoints();
void drawCustomFaceShape();
void drawMulanHead();
void drawMulanFace();
void drawMulanEyes();
void drawMulanEyebrows();
void drawMulanNose();
void drawMulanMouth();
void drawMulanHair();
void initializeFistPositions(); // Add fist animation initialization
void toggleFistAnimation(); // Add fist animation toggle
void updateFistAnimation(float deltaTime); // Add fist animation update
void updateBoxingStance(float deltaTime); // Add boxing stance animation update
void toggleBoxingStance(); // Add boxing stance toggle
void startJump(); // Start jump animation
void startDance(); // Start K-pop dance animation
void updateDance(); // Update dance animation
void updateJumpAnimation(float deltaTime); // Update jump animation
void startSwordAttack(); // Start sword attack animation
void updateSwordAttackAnimation(float deltaTime); // Update sword attack animation
void startSpearAttack(); // Start spear attack animation
void updateSpearAttackAnimation(float deltaTime); // Update spear attack animation
void startShieldBlock(); // Start shield block animation
void updateShieldBlockAnimation(float deltaTime); // Update shield block animation
Vec3 lerp(const Vec3& a, const Vec3& b, float t); // Linear interpolation
float smoothStep(float t); // Smooth easing function
void initializeKungFuSequences(); // Add kung fu animation initialization
void updateKungFuAnimation(float deltaTime);
void startKungFuAnimation(int style);

// --- Hand Drawing Function Forward Declarations ---
void drawConcretePalm(const std::vector<HandJoint>& joints);
void drawConcreteThumb(const std::vector<HandJoint>& joints);
void drawConcreteFinger(const std::vector<HandJoint>& joints, int mcpIdx, int pipIdx, int dipIdx, int tipIdx);
void drawSkinnedPalm(const std::vector<HandJoint>& joints);
void drawSkinnedPalm2(const std::vector<HandJoint>& joints);
void drawLowPolyThumb(const std::vector<HandJoint>& joints);
void drawLowPolyFinger(const std::vector<HandJoint>& joints, int mcpIdx, int pipIdx, int dipIdx, int tipIdx);

// ======== SKIN TEXTURE HELPERS ========
inline void useSkinTexture() {
    if (g_TextureEnabled) {
        glEnable(GL_TEXTURE_2D);
        Tex::bind(Tex::id[Tex::Skin]);   // requires Tex::loadAll() to have run
        glColor3f(1.0f, 1.0f, 1.0f);     // show texture colors
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    }
}

inline void stopTexture() {
    Tex::unbind();
}


// --- Math & Model Building ---
float dot(const Vec3f& a, const Vec3f& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

// --------------------- FOOT GEOMETRY (from leg.cpp) ---------------------
const Vec3f gFootVertices[] = {
    // Toes
    {0.25f, 0.0f, 1.4f}, {0.1f, 0.0f, 1.45f}, {0.1f, 0.25f, 1.45f}, {0.25f, 0.25f, 1.4f}, // 0-3
    {0.25f, 0.0f, 1.2f}, {0.1f, 0.0f, 1.2f}, {0.1f, 0.3f, 1.2f}, {0.25f, 0.3f, 1.2f},     // 4-7
    {0.05f, 0.0f, 1.4f}, {-0.1f, 0.0f, 1.35f}, {-0.1f, 0.2f, 1.35f}, {0.05f, 0.2f, 1.4f}, // 8-11
    {0.05f, 0.0f, 1.15f}, {-0.1f, 0.0f, 1.15f}, {-0.1f, 0.25f, 1.15f}, {0.05f, 0.25f, 1.15f}, // 12-15
    {-0.15f, 0.0f, 1.3f}, {-0.28f, 0.0f, 1.25f}, {-0.28f, 0.18f, 1.25f}, {-0.15f, 0.18f, 1.3f}, // 16-19
    {-0.15f, 0.0f, 1.1f}, {-0.28f, 0.0f, 1.1f}, {-0.28f, 0.22f, 1.1f}, {-0.15f, 0.22f, 1.1f}, // 20-23
    {-0.32f, 0.0f, 1.2f}, {-0.42f, 0.0f, 1.1f}, {-0.42f, 0.16f, 1.1f}, {-0.32f, 0.16f, 1.2f}, // 24-27
    {-0.32f, 0.0f, 1.0f}, {-0.42f, 0.0f, 1.0f}, {-0.42f, 0.2f, 1.0f}, {-0.32f, 0.2f, 1.0f}, // 28-31
    {-0.46f, 0.0f, 1.0f}, {-0.55f, 0.0f, 0.9f}, {-0.55f, 0.14f, 0.9f}, {-0.46f, 0.14f, 1.0f}, // 32-35
    {-0.46f, 0.0f, 0.8f}, {-0.55f, 0.0f, 0.8f}, {-0.55f, 0.18f, 0.8f}, {-0.46f, 0.18f, 0.8f}, // 36-39

    // Sides + ankle + heel + arch (rest of foot)
    {0.3f, 0.0f, 0.8f}, {0.3f, 0.4f, 0.8f}, {-0.6f, 0.0f, 0.6f}, {-0.6f, 0.3f, 0.6f},
    {0.0f, 0.9f, 0.2f}, {0.5f, 0.8f, 0.1f}, {0.55f, 0.9f, -0.5f}, {0.0f, 0.8f, -0.7f}, {-0.55f, 0.9f, -0.5f}, {-0.5f, 0.8f, 0.1f},
    {0.0f, 0.0f, 0.2f}, {0.4f, 0.0f, -0.5f}, {-0.4f, 0.0f, -0.5f},
    {0.35f, 0.6f, 0.5f}, {0.1f, 0.7f, 0.4f}, {-0.15f, 0.65f, 0.4f}, {-0.35f, 0.5f, 0.4f}, {-0.5f, 0.45f, 0.3f},
    {0.2f, 0.0f, 0.7f}, {0.0f, 0.0f, 0.65f}, {-0.2f, 0.0f, 0.6f}, {-0.4f, 0.0f, 0.5f},
    {0.0f, 1.2f, 0.2f}, {0.3f, 1.1f, 0.1f}, {0.35f, 1.2f, -0.4f}, {0.0f, 1.1f, -0.6f}, {-0.35f, 1.2f, -0.4f}, {-0.3f, 1.1f, 0.1f},
    {0.4f, 0.4f, 0.0f}, {-0.45f, 0.4f, 0.0f}, {0.2f, 0.0f, -0.1f}, {-0.2f, 0.0f, -0.1f},
    {0.3f, 0.4f, -0.7f}, {0.0f, 0.3f, -0.85f}, {-0.3f, 0.4f, -0.7f}
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
const int gNumFootQuads = sizeof(gFootQuads) / sizeof(gFootQuads[0]);

// --------------------- Math Helpers from leg.cpp ---------------------
void addRingQuads(int ring1StartIdx, int ring2StartIdx, int numSegments) {
    for (int i = 0; i < numSegments; ++i) {
        int i0 = ring1StartIdx + i;
        int i1 = ring1StartIdx + (i + 1) % numSegments;
        int i2 = ring2StartIdx + (i + 1) % numSegments;
        int i3 = ring2StartIdx + i;
        gAllQuads.push_back({ i0, i1, i2, i3 });
    }
}

inline Vec3f rotateY(const Vec3f& v, float ang) {
    float s = std::sin(ang), c = std::cos(ang);
    return { c * v.x + s * v.z, v.y, -s * v.x + c * v.z };
}

// --------------------- Build Mesh (Leg + Foot) from leg.cpp ---------------------
void buildLegMesh() {
    gAllVertices.clear(); gAllQuads.clear();
    const int legSegments = 40; const float legScale = 0.7f; int currentVertexIndex = 0;
    int ankleRingIdx = currentVertexIndex; auto ankleRing = generateRing(0.0f, 0.9f, -0.3f, 0.5f * legScale, 0.4f * legScale, legSegments); gAllVertices.insert(gAllVertices.end(), ankleRing.begin(), ankleRing.end()); currentVertexIndex += legSegments;
    int lowerShinRingIdx = currentVertexIndex; auto lowerShinRing = generateRing(0.0f, 1.45f, -0.2f, 0.55f * legScale, 0.45f * legScale, legSegments); gAllVertices.insert(gAllVertices.end(), lowerShinRing.begin(), lowerShinRing.end()); currentVertexIndex += legSegments; addRingQuads(ankleRingIdx, lowerShinRingIdx, legSegments);
    int lowerCalfRingIdx = currentVertexIndex; auto lowerCalfRing = generateRing(0.0f, 2.0f, -0.2f, 0.6f * legScale, 0.5f * legScale, legSegments); gAllVertices.insert(gAllVertices.end(), lowerCalfRing.begin(), lowerCalfRing.end()); currentVertexIndex += legSegments; addRingQuads(lowerShinRingIdx, lowerCalfRingIdx, legSegments);
    int midCalfRingIdx = currentVertexIndex; auto midCalfRing = generateRing(0.0f, 2.75f, 0.0f, 0.7f * legScale, 0.6f * legScale, legSegments); gAllVertices.insert(gAllVertices.end(), midCalfRing.begin(), midCalfRing.end()); currentVertexIndex += legSegments; addRingQuads(lowerCalfRingIdx, midCalfRingIdx, legSegments);
    int upperCalfRingIdx = currentVertexIndex; auto upperCalfRing = generateRing(0.0f, 3.5f, 0.1f, 0.65f * legScale, 0.55f * legScale, legSegments); gAllVertices.insert(gAllVertices.end(), upperCalfRing.begin(), upperCalfRing.end()); currentVertexIndex += legSegments; addRingQuads(midCalfRingIdx, upperCalfRingIdx, legSegments);
    int kneeJointBottomIdx = currentVertexIndex; auto kneeJointBottom = generateRing(0.0f, 4.0f, 0.15f, 0.7f * legScale, 0.6f * legScale, legSegments); gAllVertices.insert(gAllVertices.end(), kneeJointBottom.begin(), kneeJointBottom.end()); currentVertexIndex += legSegments; addRingQuads(upperCalfRingIdx, kneeJointBottomIdx, legSegments);
    int kneeJointMidLowerIdx = currentVertexIndex; auto kneeJointMidLower = generateRing(0.0f, 4.3f, 0.25f, 0.75f * legScale, 0.65f * legScale, legSegments); gAllVertices.insert(gAllVertices.end(), kneeJointMidLower.begin(), kneeJointMidLower.end()); currentVertexIndex += legSegments; addRingQuads(kneeJointBottomIdx, kneeJointMidLowerIdx, legSegments);
    int kneeJointMidUpperIdx = currentVertexIndex; auto kneeJointMidUpper = generateRing(0.0f, 4.7f, 0.3f, 0.8f * legScale, 0.7f * legScale, legSegments); gAllVertices.insert(gAllVertices.end(), kneeJointMidUpper.begin(), kneeJointMidUpper.end()); currentVertexIndex += legSegments; addRingQuads(kneeJointMidLowerIdx, kneeJointMidUpperIdx, legSegments);
    int kneeJointTopIdx = currentVertexIndex; auto kneeJointTop = generateRing(0.0f, 5.0f, 0.2f, 0.75f * legScale, 0.65f * legScale, legSegments); gAllVertices.insert(gAllVertices.end(), kneeJointTop.begin(), kneeJointTop.end()); currentVertexIndex += legSegments; addRingQuads(kneeJointMidUpperIdx, kneeJointTopIdx, legSegments);
    int lowerThighRingIdx = currentVertexIndex; auto lowerThighRing = generateRing(0.0f, 5.5f, 0.1f, 0.8f * legScale, 0.7f * legScale, legSegments); gAllVertices.insert(gAllVertices.end(), lowerThighRing.begin(), lowerThighRing.end()); currentVertexIndex += legSegments; addRingQuads(kneeJointTopIdx, lowerThighRingIdx, legSegments);
    int midThighRingIdx = currentVertexIndex; auto midThighRing = generateRing(0.0f, 6.5f, 0.0f, 0.9f * legScale, 0.8f * legScale, legSegments); gAllVertices.insert(gAllVertices.end(), midThighRing.begin(), midThighRing.end()); currentVertexIndex += legSegments; addRingQuads(lowerThighRingIdx, midThighRingIdx, legSegments);
    int upperThighRingIdx = currentVertexIndex; auto upperThighRing = generateRing(0.0f, 7.5f, -0.1f, 0.95f * legScale, 0.85f * legScale, legSegments); gAllVertices.insert(gAllVertices.end(), upperThighRing.begin(), upperThighRing.end()); currentVertexIndex += legSegments; addRingQuads(midThighRingIdx, upperThighRingIdx, legSegments);
    int hipRingIdx = currentVertexIndex; auto hipRing = generateRing(0.0f, 8.5f, -0.2f, 1.0f * legScale, 0.9f * legScale, legSegments); gAllVertices.insert(gAllVertices.end(), hipRing.begin(), hipRing.end()); currentVertexIndex += legSegments; addRingQuads(upperThighRingIdx, hipRingIdx, legSegments);
    int topCenterIdx = currentVertexIndex++; gAllVertices.push_back({ 0.0f, 8.7f, -0.2f }); for (int i = 0; i < legSegments; ++i) { gAllQuads.push_back({ hipRingIdx + i, hipRingIdx + (i + 1) % legSegments, topCenterIdx, -1 }); }
    const float footScaleX = 0.95f, footScaleZ = 0.95f, footLift = 0.0f, footShiftZ = -0.15f, footYaw = 0.0f; int footStartIdx = currentVertexIndex; for (int i = 0; i < gNumFootVertices; ++i) { Vec3f v = gFootVertices[i]; v.x *= footScaleX * legScale; v.z *= footScaleZ * legScale; v = rotateY(v, footYaw); v.y += footLift; v.z += footShiftZ; gAllVertices.push_back(v); ++currentVertexIndex; } for (int i = 0; i < gNumFootQuads; ++i) { std::vector<int> quad; for (int j = 0; j < 4; ++j) { int idx = gFootQuads[i][j]; if (idx == -1) break; quad.push_back(footStartIdx + idx); } gAllQuads.push_back(quad); }
    int footRingStart = footStartIdx + 44; int footRingCount = 6; int adapterStartIdx = currentVertexIndex; for (int i = 0; i < 12; ++i) { float s = (i / 12.0f) * footRingCount; int k = (int)std::floor(s); float t = s - k; int k0 = (k) % footRingCount; int k1 = (k + 1) % footRingCount; const Vec3f& a = gAllVertices[footRingStart + k0]; const Vec3f& b = gAllVertices[footRingStart + k1]; Vec3f sample = { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t }; gAllVertices.push_back(sample); ++currentVertexIndex; } addRingQuads(ankleRingIdx, adapterStartIdx, 12);
}

void computeLegFootUVs() {
    gTexCoords.resize(gAllVertices.size());
    if (gAllVertices.empty()) return;

    float yMin = 1e9f, yMax = -1e9f;
    for (const auto& v : gAllVertices) {
        yMin = std::min(yMin, v.y);
        yMax = std::max(yMax, v.y);
    }
    float ySpan = std::max(0.001f, yMax - yMin);
    float footCutoffY = yMin + 1.2f;

    for (size_t i = 0; i < gAllVertices.size(); ++i) {
        const auto& v = gAllVertices[i];
        if (v.y <= footCutoffY) { // Planar mapping for the foot
            float u = (v.x + 0.6f) / 1.2f;
            float vv = (v.z + 0.8f) / 2.3f;
            gTexCoords[i] = { u, vv };
        }
        else { // Cylindrical mapping for the leg
            float angle = atan2(v.x, v.z);
            float u = (angle + PI) / (2.0f * PI);
            float vv = (v.y - yMin) / ySpan;
            gTexCoords[i] = { u, vv };
        }
    }
}
void drawLegAndFootMesh() {
    // Make sure we use the explicit UVs we computed (no texgen!)
    if (gRenderMode == RM_TEXTURED) Tex::disableObjectLinearST();

    setSolidColorIfNeeded(0.9f, 0.7f, 0.6f);
    TextureScope ts(Tex::id[Tex::Skin], /*useTexGen*/false);  // binds + enables in RM_TEXTURED

    glBegin(GL_TRIANGLES);
    for (const auto& t : gTris) {
        auto emit = [&](int idx) {
            const Vec3f& v = gAllVertices[idx];
            const Vec3f& n = gVertexNormals[idx];
            const Vec2f& uv = gTexCoords[idx];
            glNormal3f(n.x, n.y, n.z);
            glTexCoord2f(uv.u, uv.v);   // these now take effect
            glVertex3f(v.x, v.y, v.z);
            };
        emit(t.a); emit(t.b); emit(t.c);
    }
    glEnd();
}



// --------------------- Triangles & Normals from leg.cpp ---------------------
Vec3f faceNormal(int i0, int i1, int i2) {
    const Vec3f& v0 = gAllVertices[i0]; const Vec3f& v1 = gAllVertices[i1]; const Vec3f& v2 = gAllVertices[i2];
    return normalize(cross(sub(v1, v0), sub(v2, v0)));
}

void buildTriangles()
{
    gTris.clear();
    for (const auto& q : gAllQuads) {
        if (q.size() == 3 || (q.size() == 4 && q[3] == -1)) {
            gTris.push_back({ q[0], q[1], q[2] });
        }
        else if (q.size() == 4) {
            gTris.push_back({ q[0], q[1], q[3] });
            gTris.push_back({ q[1], q[2], q[3] });
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

static void InitializeHand() {
    g_HandJoints.clear();
    g_HandJoints.resize(21);

    // WRIST (0) - moved backward to better connect with arm
    g_HandJoints[0] = { {0.0f, 0.0f, -0.2f}, -1 };

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

    g_HandJoints2[0] = { {0.0f * flipSign, 0.0f, -0.2f}, -1 };
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
    g_ArmJoints.resize(8); // Increased to 8 joints for better arm structure
    g_ArmJoints[0] = { {0.0f, 0.0f, 0.1f}, -1 };        // Shoulder base
    g_ArmJoints[1] = { {0.15f, 0.08f, 1.8f}, 0 };       // Upper arm start
    g_ArmJoints[2] = { {0.25f, 0.12f, 3.8f}, 1 };       // Upper arm mid
    g_ArmJoints[3] = { {0.30f, 0.15f, 5.8f}, 2 };       // Elbow joint (upper arm end) - extended upper arm
    // Lower arm starts here and will be transformed based on bend angle
    g_ArmJoints[4] = { {0.32f, 0.18f, 6.8f}, 3 };       // Lower arm start (from elbow) - shortened gap
    g_ArmJoints[5] = { {0.28f, 0.22f, 8.2f}, 4 };       // Lower arm mid - shortened
    g_ArmJoints[6] = { {0.25f, 0.28f, 9.2f}, 5 };       // Lower arm end - shortened  
    g_ArmJoints[7] = { {0.20f, 0.35f, 9.8f}, 6 };       // Wrist joint - much shorter forearm
}

static void InitializeArm2() {
    g_ArmJoints2.clear();
    g_ArmJoints2.resize(8); // Increased to 8 joints for better arm structure
    float flipSign = -1.0f;
    g_ArmJoints2[0] = { {0.0f * flipSign, 0.0f, 0.1f}, -1 };        // Shoulder base
    g_ArmJoints2[1] = { {0.15f * flipSign, 0.08f, 1.8f}, 0 };       // Upper arm start
    g_ArmJoints2[2] = { {0.25f * flipSign, 0.12f, 3.8f}, 1 };       // Upper arm mid
    g_ArmJoints2[3] = { {0.30f * flipSign, 0.15f, 5.8f}, 2 };       // Elbow joint (upper arm end) - extended upper arm
    // Lower arm starts here and will be transformed based on bend angle
    g_ArmJoints2[4] = { {0.32f * flipSign, 0.18f, 6.8f}, 3 };       // Lower arm start (from elbow) - shortened gap
    g_ArmJoints2[5] = { {0.28f * flipSign, 0.22f, 8.2f}, 4 };       // Lower arm mid - shortened
    g_ArmJoints2[6] = { {0.25f * flipSign, 0.28f, 9.2f}, 5 };       // Lower arm end - shortened
    g_ArmJoints2[7] = { {0.20f * flipSign, 0.35f, 9.8f}, 6 };       // Wrist joint - much shorter forearm
}

void initializeCharacterParts() {
    initializeSegmentArrays();
    buildLegMesh();
    buildTriangles();
    computeVertexNormals();
    computeLegFootUVs(); // <<< Call the new UV generation function

    InitializeHand();
    InitializeHand2();
    InitializeArm();
    InitializeArm2();
    initializeFistPositions();
    initializeKungFuSequences();

    Tex::loadAll(); // Load all textures from your texture manager

    if (!g_headQuadric) {
        g_headQuadric = gluNewQuadric();
        gluQuadricNormals(g_headQuadric, GLU_SMOOTH);
        gluQuadricTexture(g_headQuadric, GL_TRUE);
    }
    BackgroundRenderer::init();
    setRenderMode(gRenderMode); // Set initial render state
}


// ===================================================================
//
// FIST ANIMATION FUNCTIONS
//
// ===================================================================

// Store original hand positions for smooth transitions
std::vector<HandJoint> g_OriginalHandJoints;
std::vector<HandJoint> g_OriginalHandJoints2;
std::vector<HandJoint> g_FistHandJoints;
std::vector<HandJoint> g_FistHandJoints2;

void initializeFistPositions() {
    // Store original positions
    g_OriginalHandJoints = g_HandJoints;
    g_OriginalHandJoints2 = g_HandJoints2;

    // Create fist positions for left hand
    g_FistHandJoints.clear();
    g_FistHandJoints.resize(21);

    // WRIST (0) - stays the same
    g_FistHandJoints[0] = { {0.0f, 0.0f, 0.0f}, -1 };

    // THUMB - curl inward for fist (1-4)
    g_FistHandJoints[1] = { {0.4f, -0.2f, 0.2f}, 0 };
    g_FistHandJoints[2] = { {0.5f, -0.3f, 0.4f}, 1 };
    g_FistHandJoints[3] = { {0.6f, -0.4f, 0.5f}, 2 };
    g_FistHandJoints[4] = { {0.7f, -0.5f, 0.6f}, 3 };

    // INDEX FINGER - curl into fist (5-8)
    g_FistHandJoints[5] = { {0.3f, 0.1f, 1.0f}, 0 };
    g_FistHandJoints[6] = { {0.4f, -0.2f, 1.2f}, 5 };
    g_FistHandJoints[7] = { {0.5f, -0.4f, 1.0f}, 6 };
    g_FistHandJoints[8] = { {0.6f, -0.5f, 0.8f}, 7 };

    // MIDDLE FINGER - curl into fist (9-12)
    g_FistHandJoints[9] = { {0.0f, 0.1f, 1.1f}, 0 };
    g_FistHandJoints[10] = { {0.1f, -0.2f, 1.3f}, 9 };
    g_FistHandJoints[11] = { {0.2f, -0.4f, 1.1f}, 10 };
    g_FistHandJoints[12] = { {0.3f, -0.5f, 0.9f}, 11 };

    // RING FINGER - curl into fist (13-16)
    g_FistHandJoints[13] = { {-0.3f, 0.1f, 1.0f}, 0 };
    g_FistHandJoints[14] = { {-0.2f, -0.2f, 1.2f}, 13 };
    g_FistHandJoints[15] = { {-0.1f, -0.4f, 1.0f}, 14 };
    g_FistHandJoints[16] = { {0.0f, -0.5f, 0.8f}, 15 };

    // PINKY - curl into fist (17-20)
    g_FistHandJoints[17] = { {-0.5f, 0.05f, 0.8f}, 0 };
    g_FistHandJoints[18] = { {-0.4f, -0.2f, 0.9f}, 17 };
    g_FistHandJoints[19] = { {-0.3f, -0.4f, 0.7f}, 18 };
    g_FistHandJoints[20] = { {-0.2f, -0.5f, 0.5f}, 19 };

    // Create fist positions for right hand (mirrored)
    g_FistHandJoints2.clear();
    g_FistHandJoints2.resize(21);
    float flipSign = -1.0f;

    g_FistHandJoints2[0] = { {0.0f, 0.0f, 0.0f}, -1 };
    g_FistHandJoints2[1] = { {0.4f * flipSign, -0.2f, 0.2f}, 0 };
    g_FistHandJoints2[2] = { {0.5f * flipSign, -0.3f, 0.4f}, 1 };
    g_FistHandJoints2[3] = { {0.6f * flipSign, -0.4f, 0.5f}, 2 };
    g_FistHandJoints2[4] = { {0.7f * flipSign, -0.5f, 0.6f}, 3 };
    g_FistHandJoints2[5] = { {0.3f * flipSign, 0.1f, 1.0f}, 0 };
    g_FistHandJoints2[6] = { {0.4f * flipSign, -0.2f, 1.2f}, 5 };
    g_FistHandJoints2[7] = { {0.5f * flipSign, -0.4f, 1.0f}, 6 };
    g_FistHandJoints2[8] = { {0.6f * flipSign, -0.5f, 0.8f}, 7 };
    g_FistHandJoints2[9] = { {0.0f * flipSign, 0.1f, 1.1f}, 0 };
    g_FistHandJoints2[10] = { {0.1f * flipSign, -0.2f, 1.3f}, 9 };
    g_FistHandJoints2[11] = { {0.2f * flipSign, -0.4f, 1.1f}, 10 };
    g_FistHandJoints2[12] = { {0.3f * flipSign, -0.5f, 0.9f}, 11 };
    g_FistHandJoints2[13] = { {-0.3f * flipSign, 0.1f, 1.0f}, 0 };
    g_FistHandJoints2[14] = { {-0.2f * flipSign, -0.2f, 1.2f}, 13 };
    g_FistHandJoints2[15] = { {-0.1f * flipSign, -0.4f, 1.0f}, 14 };
    g_FistHandJoints2[16] = { {0.0f * flipSign, -0.5f, 0.8f}, 15 };
    g_FistHandJoints2[17] = { {-0.5f * flipSign, 0.05f, 0.8f}, 0 };
    g_FistHandJoints2[18] = { {-0.4f * flipSign, -0.2f, 0.9f}, 17 };
    g_FistHandJoints2[19] = { {-0.3f * flipSign, -0.4f, 0.7f}, 18 };
    g_FistHandJoints2[20] = { {-0.2f * flipSign, -0.5f, 0.5f}, 19 };
}

// Linear interpolation between two Vec3 points
Vec3 lerp(const Vec3& a, const Vec3& b, float t) {
    return { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t };
}

// Smooth step function for easing
float smoothStep(float t) {
    return t * t * (3.0f - 2.0f * t);
}

void updateFistAnimation(float deltaTime) {
    if (!gFistAnimationActive) return;

    gFistAnimationTime += deltaTime;

    // Calculate animation progress (0 to 1)
    float progress = gFistAnimationTime / FIST_ANIMATION_DURATION;

    if (progress >= 1.0f) {
        // Animation finished
        progress = 1.0f;
        gFistAnimationActive = false;
        gIsFist = !gIsFist; // Toggle to the target state
    }

    // Apply smooth easing
    float easedProgress = smoothStep(progress);

    // Choose interpolation direction based on target state (inverse of current)
    if (!gIsFist) {
        // Currently open, going to fist
        for (int i = 0; i < 21; ++i) {
            g_HandJoints[i].position = lerp(g_OriginalHandJoints[i].position, g_FistHandJoints[i].position, easedProgress);
            g_HandJoints2[i].position = lerp(g_OriginalHandJoints2[i].position, g_FistHandJoints2[i].position, easedProgress);
        }
    }
    else {
        // Currently fist, going to open
        for (int i = 0; i < 21; ++i) {
            g_HandJoints[i].position = lerp(g_FistHandJoints[i].position, g_OriginalHandJoints[i].position, easedProgress);
            g_HandJoints2[i].position = lerp(g_FistHandJoints2[i].position, g_OriginalHandJoints2[i].position, easedProgress);
        }
    }
}

void toggleFistAnimation() {
    if (gFistAnimationActive) return; // Don't interrupt ongoing animation

    gFistAnimationActive = true;
    gFistAnimationTime = 0.0f;
    // The animation direction is determined by the current gIsFist state in updateFistAnimation
}

// SWORD ATTACK ANIMATION FUNCTIONS
void startSwordAttack() {
    if (gSwordAttackAnimating) return; // Don't interrupt ongoing animation
    
    gSwordAttackAnimating = true;
    gSwordAttackAnimationTime = 0.0f;
    gSwordAttackPhase = 0;
    
    // Reset attack pose offsets
    gSwordAttackTorsoRotation = 0.0f;
    gSwordAttackShoulderOffset = 0.0f;
    gSwordAttackArmExtension = 0.0f;
    gSwordAttackLegStance = 0.0f;
}

void updateSwordAttackAnimation(float deltaTime) {
    if (!gSwordAttackAnimating) return;
    
    gSwordAttackAnimationTime += deltaTime;
    
    // Calculate progress through current phase
    float phaseTime = 0.0f;
    float phaseDuration = 0.0f;
    
    if (gSwordAttackAnimationTime <= SWORD_WINDUP_DURATION) {
        // Wind up phase
        gSwordAttackPhase = 1;
        phaseTime = gSwordAttackAnimationTime;
        phaseDuration = SWORD_WINDUP_DURATION;
        
        float progress = phaseTime / phaseDuration;
        progress = smoothStep(progress); // Smooth easing
        
        // Wind up: dramatic preparation - raise sword high above head like a real warrior
        gSwordAttackTorsoRotation = -30.0f * progress; // More pronounced torso twist
        gSwordAttackShoulderOffset = -25.0f * progress; // Greater shoulder movement for overhead position
        gSwordAttackArmExtension = -35.0f * progress; // Pull arm way back and up
        gSwordAttackLegStance = 15.0f * progress; // Wider stance for power
        
    } else if (gSwordAttackAnimationTime <= SWORD_WINDUP_DURATION + SWORD_STRIKE_DURATION) {
        // Strike phase
        gSwordAttackPhase = 2;
        phaseTime = gSwordAttackAnimationTime - SWORD_WINDUP_DURATION;
        phaseDuration = SWORD_STRIKE_DURATION;
        
        float progress = phaseTime / phaseDuration;
        progress = progress * progress * (3.0f - 2.0f * progress); // Explosive strike motion
        
        // Strike: explosive overhead to forward swing motion
        float windupTorso = -30.0f;
        float windupShoulder = -25.0f;
        float windupArm = -35.0f;
        float windupLeg = 15.0f;
        
        gSwordAttackTorsoRotation = windupTorso + (50.0f * progress); // Powerful torso rotation
        gSwordAttackShoulderOffset = windupShoulder + (40.0f * progress); // Shoulder drives forward
        gSwordAttackArmExtension = windupArm + (60.0f * progress); // Full arm extension like real sword strike
        gSwordAttackLegStance = windupLeg + (-20.0f * progress); // Weight shifts forward aggressively
        
    } else if (gSwordAttackAnimationTime <= SWORD_ATTACK_DURATION) {
        // Follow through phase
        gSwordAttackPhase = 3;
        phaseTime = gSwordAttackAnimationTime - SWORD_WINDUP_DURATION - SWORD_STRIKE_DURATION;
        phaseDuration = SWORD_FOLLOWTHROUGH_DURATION;
        
        float progress = phaseTime / phaseDuration;
        progress = smoothStep(progress); // Smooth recovery
        
        // Follow through: controlled recovery to ready position
        float strikeTorso = 20.0f;
        float strikeShoulder = 15.0f;
        float strikeArm = 25.0f;
        float strikeLeg = -5.0f;
        
        gSwordAttackTorsoRotation = strikeTorso * (1.0f - progress);
        gSwordAttackShoulderOffset = strikeShoulder * (1.0f - progress);
        gSwordAttackArmExtension = strikeArm * (1.0f - progress);
        gSwordAttackLegStance = strikeLeg * (1.0f - progress);
        
    } else {
        // Animation complete
        gSwordAttackAnimating = false;
        gSwordAttackPhase = 0;
        gSwordAttackTorsoRotation = 0.0f;
        gSwordAttackShoulderOffset = 0.0f;
        gSwordAttackArmExtension = 0.0f;
        gSwordAttackLegStance = 0.0f;
    }
}

// SPEAR ATTACK ANIMATION FUNCTIONS
void startSpearAttack() {
    if (gSpearAttackAnimating) return; // Don't interrupt ongoing animation
    
    gSpearAttackAnimating = true;
    gSpearAttackAnimationTime = 0.0f;
    gSpearAttackPhase = 0;
    
    // Reset attack pose offsets
    gSpearAttackTorsoRotation = 0.0f;
    gSpearAttackShoulderOffset = 0.0f;
    gSpearAttackArmExtension = 0.0f;
    gSpearAttackLegStance = 0.0f;
}

void updateSpearAttackAnimation(float deltaTime) {
    if (!gSpearAttackAnimating) return;
    
    gSpearAttackAnimationTime += deltaTime;
    
    // Calculate progress through current phase
    float phaseTime = 0.0f;
    float phaseDuration = 0.0f;
    
    if (gSpearAttackAnimationTime <= SPEAR_WINDUP_DURATION) {
        // Wind up phase - prepare for powerful thrust
        gSpearAttackPhase = 1;
        phaseTime = gSpearAttackAnimationTime;
        phaseDuration = SPEAR_WINDUP_DURATION;
        
        float progress = phaseTime / phaseDuration;
        progress = smoothStep(progress); // Smooth easing
        
        // Wind up: pull spear back and prepare for thrust like a warrior
        gSpearAttackTorsoRotation = -20.0f * progress; // Torso twist back
        gSpearAttackShoulderOffset = -30.0f * progress; // Pull shoulder back
        gSpearAttackArmExtension = -40.0f * progress; // Pull arm way back
        gSpearAttackLegStance = 20.0f * progress; // Wider stance for power
        
    } else if (gSpearAttackAnimationTime <= SPEAR_WINDUP_DURATION + SPEAR_THRUST_DURATION) {
        // Thrust phase - explosive forward lunge
        gSpearAttackPhase = 2;
        phaseTime = gSpearAttackAnimationTime - SPEAR_WINDUP_DURATION;
        phaseDuration = SPEAR_THRUST_DURATION;
        
        float progress = phaseTime / phaseDuration;
        progress = progress * progress * (3.0f - 2.0f * progress); // Explosive thrust motion
        
        // Thrust: explosive forward lunge motion
        float windupTorso = -20.0f;
        float windupShoulder = -30.0f;
        float windupArm = -40.0f;
        float windupLeg = 20.0f;
        
        gSpearAttackTorsoRotation = windupTorso + (35.0f * progress); // Powerful torso rotation
        gSpearAttackShoulderOffset = windupShoulder + (50.0f * progress); // Shoulder drives forward
        gSpearAttackArmExtension = windupArm + (70.0f * progress); // Full arm extension for thrust
        gSpearAttackLegStance = windupLeg + (-25.0f * progress); // Weight shifts forward
        
    } else if (gSpearAttackAnimationTime <= SPEAR_ATTACK_DURATION) {
        // Follow through phase - controlled recovery
        gSpearAttackPhase = 3;
        phaseTime = gSpearAttackAnimationTime - SPEAR_WINDUP_DURATION - SPEAR_THRUST_DURATION;
        phaseDuration = SPEAR_FOLLOWTHROUGH_DURATION;
        
        float progress = phaseTime / phaseDuration;
        progress = smoothStep(progress); // Smooth recovery
        
        // Follow through: controlled recovery to ready position
        float thrustTorso = 15.0f;
        float thrustShoulder = 20.0f;
        float thrustArm = 30.0f;
        float thrustLeg = -5.0f;
        
        gSpearAttackTorsoRotation = thrustTorso * (1.0f - progress);
        gSpearAttackShoulderOffset = thrustShoulder * (1.0f - progress);
        gSpearAttackArmExtension = thrustArm * (1.0f - progress);
        gSpearAttackLegStance = thrustLeg * (1.0f - progress);
        
    } else {
        // Animation complete
        gSpearAttackAnimating = false;
        gSpearAttackPhase = 0;
        gSpearAttackTorsoRotation = 0.0f;
        gSpearAttackShoulderOffset = 0.0f;
        gSpearAttackArmExtension = 0.0f;
        gSpearAttackLegStance = 0.0f;
    }
}

// SHIELD BLOCK ANIMATION FUNCTIONS
void startShieldBlock() {
    if (gShieldBlockAnimating) return; // Don't interrupt ongoing animation
    
    gShieldBlockAnimating = true;
    gShieldBlockAnimationTime = 0.0f;
    gShieldBlockPhase = 0;
    
    // Reset block pose offsets
    gShieldBlockArmRaise = 0.0f;
    gShieldBlockTorsoLean = 0.0f;
    gShieldBlockStance = 0.0f;
}

void updateShieldBlockAnimation(float deltaTime) {
    if (!gShieldBlockAnimating) return;
    
    gShieldBlockAnimationTime += deltaTime;
    
    // Calculate progress through current phase
    float phaseTime = 0.0f;
    float phaseDuration = 0.0f;
    
    if (gShieldBlockAnimationTime <= SHIELD_RAISE_DURATION) {
        // Raise phase - quickly raise shield to block
        gShieldBlockPhase = 1;
        phaseTime = gShieldBlockAnimationTime;
        phaseDuration = SHIELD_RAISE_DURATION;
        
        float progress = phaseTime / phaseDuration;
        progress = progress * progress; // Fast start
        
        // Raise: quickly raise shield and brace for impact with bent arm
        gShieldBlockArmRaise = 80.0f * progress; // Higher raise for better protection
        gShieldBlockTorsoLean = -8.0f * progress; // Less lean, more upright defensive stance
        gShieldBlockStance = 35.0f * progress; // Wider stance for stability
        
    } else if (gShieldBlockAnimationTime <= SHIELD_RAISE_DURATION + SHIELD_HOLD_DURATION) {
        // Hold phase - maintain defensive position
        gShieldBlockPhase = 2;
        phaseTime = gShieldBlockAnimationTime - SHIELD_RAISE_DURATION;
        phaseDuration = SHIELD_HOLD_DURATION;
        
        // Hold: maintain defensive position with slight tension
        float tension = sinf(phaseTime * 8.0f) * 1.0f; // Reduced tension for more stable hold
        gShieldBlockArmRaise = 80.0f + tension; // Maintain high shield position
        gShieldBlockTorsoLean = -8.0f;
        gShieldBlockStance = 35.0f;
        
    } else if (gShieldBlockAnimationTime <= SHIELD_BLOCK_DURATION) {
        // Lower phase - return to ready position
        gShieldBlockPhase = 3;
        phaseTime = gShieldBlockAnimationTime - SHIELD_RAISE_DURATION - SHIELD_HOLD_DURATION;
        phaseDuration = SHIELD_LOWER_DURATION;
        
        float progress = phaseTime / phaseDuration;
        progress = smoothStep(progress); // Smooth recovery
        
        // Lower: controlled return to ready position
        gShieldBlockArmRaise = 80.0f * (1.0f - progress);
        gShieldBlockTorsoLean = -8.0f * (1.0f - progress);
        gShieldBlockStance = 35.0f * (1.0f - progress);
        
    } else {
        // Animation complete
        gShieldBlockAnimating = false;
        gShieldBlockPhase = 0;
        gShieldBlockArmRaise = 0.0f;
        gShieldBlockTorsoLean = 0.0f;
        gShieldBlockStance = 0.0f;
        // Reset lower arm bend when shield animation ends
        gLeftLowerArmBend = 0.0f;
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

// Draw arm connector segment at the base of the hand to bridge to wrist joint
static void drawHandArmConnector(const std::vector<HandJoint>& joints) {
    Vec3 wrist = joints[0].position;

    // Create arm-like connector extending from hand base toward wrist
    // Make it extend backward (negative Z direction) from the palm to meet the arm
    Vec3 connectorStart = { wrist.x, wrist.y, wrist.z - 0.8f };  // Extended back from palm base
    Vec3 connectorEnd = wrist;

    // Enable texturing to match the arm
    if (g_TextureEnabled) {
        Tex::bind(Tex::id[gConcreteHands ? Tex::Silver : Tex::Skin]);
        glColor3f(1.0f, 1.0f, 1.0f);
    }
    else {
        Tex::unbind();
        glDisable(GL_TEXTURE_2D);
        glColor3f(0.85f, 0.64f, 0.52f);
    }


    // Draw the connecting segment using anatomical arm style
    drawAnatomicalArmSegment(connectorStart, connectorEnd, 1.2f, 0.9f, 0.9f, 0.7f);

    // Draw a joint at the connection point
    drawRobotJoint(connectorEnd, 0.12f);
}

// Draw joint visualization sphere
static void drawJointSphere(const Vec3& pos, float radius, float r, float g, float b) {
    if (!gShowJointVisuals) return;
    glPushAttrib(GL_ENABLE_BIT);
    glPushMatrix();
    glTranslatef(pos.x, pos.y, pos.z);
    glDisable(GL_TEXTURE_2D);
    glColor3f(r, g, b);

    // Draw sphere using triangle strips
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
            float lng = 2.0f * PI * (float)j / slices;
            float x0 = r0 * cosf(lng);
            float z0 = r0 * sinf(lng);
            float x1 = r1 * cosf(lng);
            float z1 = r1 * sinf(lng);

            glVertex3f(x0, y0, z0);
            glVertex3f(x1, y1, z1);
        }
        glEnd();
    }

    glPopMatrix();
    glPopAttrib();
}

// Draw joint visualization box
static void drawJointBox(const Vec3& pos, float size, float r, float g, float b) {
    if (!gShowJointVisuals) return;
    glPushAttrib(GL_ENABLE_BIT);
    glPushMatrix();
    glTranslatef(pos.x, pos.y, pos.z);
    glDisable(GL_TEXTURE_2D);
    glColor3f(r, g, b);

    float half = size * 0.5f;

    glBegin(GL_TRIANGLES);

    // Front face
    glVertex3f(-half, -half, half);
    glVertex3f(half, -half, half);
    glVertex3f(half, half, half);
    glVertex3f(-half, -half, half);
    glVertex3f(half, half, half);
    glVertex3f(-half, half, half);

    // Back face
    glVertex3f(half, -half, -half);
    glVertex3f(-half, -half, -half);
    glVertex3f(-half, half, -half);
    glVertex3f(half, -half, -half);
    glVertex3f(-half, half, -half);
    glVertex3f(half, half, -half);

    // Right face
    glVertex3f(half, -half, half);
    glVertex3f(half, -half, -half);
    glVertex3f(half, half, -half);
    glVertex3f(half, -half, half);
    glVertex3f(half, half, -half);
    glVertex3f(half, half, half);

    // Left face
    glVertex3f(-half, -half, -half);
    glVertex3f(-half, -half, half);
    glVertex3f(-half, half, half);
    glVertex3f(-half, -half, -half);
    glVertex3f(-half, half, half);
    glVertex3f(-half, half, -half);

    // Top face
    glVertex3f(-half, half, half);
    glVertex3f(half, half, half);
    glVertex3f(half, half, -half);
    glVertex3f(-half, half, half);
    glVertex3f(half, half, -half);
    glVertex3f(-half, half, -half);

    // Bottom face
    glVertex3f(-half, -half, -half);
    glVertex3f(half, -half, -half);
    glVertex3f(half, -half, half);
    glVertex3f(-half, -half, -half);
    glVertex3f(half, -half, half);
    glVertex3f(-half, -half, half);

    glEnd();
    glPopMatrix();
    glPopAttrib();
}

// Smooth interpolation function
float lerpf(float a, float b, float t) {
    return a + (b - a) * t;
}

// Update boxing stance animation
void updateBoxingStance(float deltaTime) {
    if (!gBoxingAnimActive) return;

    gBoxingAnimTime += deltaTime;
    float progress = gBoxingAnimTime / BOXING_ANIM_DURATION;

    if (progress >= 1.0f) {
        progress = 1.0f;
        gBoxingAnimActive = false;
    }

    // Apply smooth easing
    float easedProgress = smoothStep(progress);

    // Determine target values based on stance
    float targetLeftShoulderPitch, targetLeftShoulderYaw, targetLeftShoulderRoll, targetLeftElbowBend;
    float targetRightShoulderPitch, targetRightShoulderYaw, targetRightShoulderRoll, targetRightElbowBend;

    if (gInBoxingStance) {
        // Transitioning to boxing stance
        targetLeftShoulderPitch = BOXING_SHOULDER_PITCH;
        targetLeftShoulderYaw = BOXING_SHOULDER_YAW;
        targetLeftShoulderRoll = BOXING_SHOULDER_ROLL;   // 180-degree rotation
        targetLeftElbowBend = BOXING_ELBOW_BEND;
        targetRightShoulderPitch = BOXING_SHOULDER_PITCH;
        targetRightShoulderYaw = -BOXING_SHOULDER_YAW; // Mirror for right arm
        targetRightShoulderRoll = BOXING_SHOULDER_ROLL;  // 180-degree rotation
        targetRightElbowBend = BOXING_ELBOW_BEND;
    }
    else {
        // Transitioning to relaxed pose
        targetLeftShoulderPitch = RELAXED_SHOULDER_PITCH;
        targetLeftShoulderYaw = RELAXED_SHOULDER_YAW;
        targetLeftShoulderRoll = RELAXED_SHOULDER_ROLL;
        targetLeftElbowBend = RELAXED_ELBOW_BEND;
        targetRightShoulderPitch = RELAXED_SHOULDER_PITCH;
        targetRightShoulderYaw = RELAXED_SHOULDER_YAW;
        targetRightShoulderRoll = RELAXED_SHOULDER_ROLL;
        targetRightElbowBend = RELAXED_ELBOW_BEND;
    }

    // Interpolate current values
    gCurrentLeftShoulderPitch = lerpf(RELAXED_SHOULDER_PITCH, targetLeftShoulderPitch, easedProgress);
    gCurrentLeftShoulderYaw = lerpf(RELAXED_SHOULDER_YAW, targetLeftShoulderYaw, easedProgress);
    gCurrentLeftShoulderRoll = lerpf(RELAXED_SHOULDER_ROLL, targetLeftShoulderRoll, easedProgress);
    gCurrentLeftElbowBend = lerpf(RELAXED_ELBOW_BEND, targetLeftElbowBend, easedProgress);
    gCurrentRightShoulderPitch = lerpf(RELAXED_SHOULDER_PITCH, targetRightShoulderPitch, easedProgress);
    gCurrentRightShoulderYaw = lerpf(RELAXED_SHOULDER_YAW, targetRightShoulderYaw, easedProgress);
    gCurrentRightShoulderRoll = lerpf(RELAXED_SHOULDER_ROLL, targetRightShoulderRoll, easedProgress);
    gCurrentRightElbowBend = lerpf(RELAXED_ELBOW_BEND, targetRightElbowBend, easedProgress);
}

// Toggle boxing stance
void toggleBoxingStance() {
    if (gBoxingAnimActive) return; // Don't interrupt ongoing animation

    gInBoxingStance = !gInBoxingStance;
    gBoxingAnimActive = true;
    gBoxingAnimTime = 0.0f;
}

// Start jump animation
void startJump() {
    if (!gIsJumping) {  // Only start if not already jumping
        gIsJumping = true;
        gJumpPhase = 0.0f;
        gJumpVerticalOffset = 0.0f;
    }
}

// Update jump animation
void updateJumpAnimation(float deltaTime) {
    if (!gIsJumping) return;

    // Advance jump phase
    gJumpPhase += deltaTime / JUMP_DURATION;

    if (gJumpPhase >= 2.0f) {
        // Jump complete
        gIsJumping = false;
        gJumpPhase = 0.0f;
        gJumpVerticalOffset = 0.0f;
    }
    else {
        // Calculate vertical offset using a parabolic curve
        // Phase 0.0 to 1.0 = going up, 1.0 to 2.0 = coming down
        float normalizedPhase = gJumpPhase;
        if (normalizedPhase > 1.0f) normalizedPhase = 2.0f - normalizedPhase; // Mirror for descent

        // Use parabolic curve for natural jump motion
        gJumpVerticalOffset = JUMP_HEIGHT * (4.0f * normalizedPhase * (1.0f - normalizedPhase));
    }
}

// Start K-pop dance animation
void startDance() {
    if (!gIsDancing) {  // Only start if not already dancing
        gIsDancing = true;
        gDanceTime = 0.0f;
        gDancePhase = 0.0f;
        gJumpVerticalOffset = 0.0f;

        // Reset all pose values to neutral before starting dance
        g_pose.torsoYaw = g_pose.torsoPitch = g_pose.torsoRoll = 0.0f;
        g_pose.headYaw = g_pose.headPitch = g_pose.headRoll = 0.0f;
        gCurrentPose.leftArmAngle = gCurrentPose.rightArmAngle = 0.0f;
        gCurrentPose.torsoYaw = gCurrentPose.torsoPitch = gCurrentPose.torsoRoll = 0.0f;
    }
}

// Update K-pop dance animation with full-body choreography
void updateDance() {
    if (!gIsDancing) return;

    const float deltaTime = 0.016f; // Assuming 60 FPS
    gDanceTime += deltaTime;
    gDancePhase = gDanceTime * DANCE_SPEED;

    if (gDanceTime >= DANCE_DURATION) {
        // Dance complete - return to neutral pose
        gIsDancing = false;
        gDanceTime = 0.0f;
        gDancePhase = 0.0f;
        gJumpVerticalOffset = 0.0f;

        // Reset pose to neutral
        g_pose.torsoYaw = g_pose.torsoPitch = g_pose.torsoRoll = 0.0f;
        g_pose.headYaw = g_pose.headPitch = g_pose.headRoll = 0.0f;
        gCurrentPose.leftArmAngle = gCurrentPose.rightArmAngle = 0.0f;
        gCurrentPose.torsoYaw = gCurrentPose.torsoPitch = gCurrentPose.torsoRoll = 0.0f;
        return;
    }

    // K-pop dance choreography with 8-second sequence
    float t = gDancePhase;
    float beat = fmod(t, 1.0f); // Individual beat within each second
    int measure = (int)(gDanceTime * 2.0f) % 16; // 16 different measures for variety

    // === ARM MOVEMENTS (K-pop style) ===
    float leftArmBase = 0.0f, rightArmBase = 0.0f;

    if (measure < 2) {
        // Opening: Arms sweep out and up
        leftArmBase = -60.0f + sinf(t * 2.0f) * 40.0f;
        rightArmBase = -60.0f - sinf(t * 2.0f) * 40.0f;
    }
    else if (measure < 4) {
        // Wave motion: Arms create flowing waves
        leftArmBase = -30.0f + sinf(t * 3.0f) * 50.0f;
        rightArmBase = -30.0f + sinf(t * 3.0f + PI * 0.5f) * 50.0f;
    }
    else if (measure < 6) {
        // Point and pose: Sharp pointing moves
        leftArmBase = (beat < 0.5f) ? -80.0f : 20.0f;
        rightArmBase = (beat < 0.5f) ? 20.0f : -80.0f;
    }
    else if (measure < 8) {
        // Cross and uncross: Arms cross over chest
        float crossPhase = sinf(t * 4.0f);
        leftArmBase = crossPhase * -45.0f;
        rightArmBase = -crossPhase * -45.0f;
    }
    else if (measure < 10) {
        // High energy: Arms pump up and down
        leftArmBase = -90.0f + cosf(t * 6.0f) * 60.0f;
        rightArmBase = -90.0f + cosf(t * 6.0f + PI) * 60.0f;
    }
    else if (measure < 12) {
        // Side to side: Arms swing side to side
        leftArmBase = sinf(t * 2.0f) * -70.0f;
        rightArmBase = sinf(t * 2.0f) * 70.0f;
    }
    else if (measure < 14) {
        // Hip hop style: One arm up, one down alternating
        leftArmBase = (sinf(t * 4.0f) > 0) ? -90.0f : 30.0f;
        rightArmBase = (sinf(t * 4.0f) > 0) ? 30.0f : -90.0f;
    }
    else {
        // Finale: Both arms up in victory pose
        leftArmBase = -80.0f + sinf(t * 8.0f) * 10.0f;
        rightArmBase = -80.0f + sinf(t * 8.0f) * 10.0f;
    }

    // Apply arm movements
    gCurrentPose.leftArmAngle = leftArmBase;
    gCurrentPose.rightArmAngle = rightArmBase;

    // === TORSO MOVEMENTS ===
    // Torso sway and rotation
    g_pose.torsoYaw = sinf(t * 2.5f) * 15.0f;
    g_pose.torsoPitch = sinf(t * 1.8f) * 8.0f;
    g_pose.torsoRoll = cosf(t * 3.2f) * 12.0f;

    // === HEAD MOVEMENTS ===
    // Head bobs and turns with the beat
    g_pose.headYaw = sinf(t * 3.0f) * 20.0f;
    g_pose.headPitch = cosf(t * 4.0f) * 10.0f;
    g_pose.headRoll = sinf(t * 2.8f) * 8.0f;

    // === JUMPING AND BODY MOVEMENTS ===
    // Add periodic jumps during energetic sections
    if (measure >= 8 && measure < 12) {
        // High energy section with jumps
        float jumpCycle = fmod(t * 2.0f, 2.0f);
        if (jumpCycle < 0.3f) {
            float jumpPhase = jumpCycle / 0.3f;
            gJumpVerticalOffset = JUMP_HEIGHT * sinf(jumpPhase * PI) * 0.7f; // Smaller jumps for dance
        }
        else {
            gJumpVerticalOffset = 0.0f;
        }
    }
    else if (measure >= 4 && measure < 6) {
        // Sharp movements section with small hops
        gJumpVerticalOffset = (sinf(t * 8.0f) > 0.8f) ? JUMP_HEIGHT * 0.3f : 0.0f;
    }
    else {
        // Ground-based movements
        gJumpVerticalOffset = sinf(t * 1.5f) * 0.2f; // Subtle body bobbing
    }

    // Add extra body dynamics to make it more lively
    gCurrentPose.torsoYaw += g_pose.torsoYaw;
    gCurrentPose.torsoPitch += g_pose.torsoPitch;
    gCurrentPose.torsoRoll += g_pose.torsoRoll;
}

// Helper function to get transformed wrist position accounting for lower arm bending
Vec3 getTransformedWristPosition(const std::vector<ArmJoint>& armJoints, float lowerArmBend) {
    if (armJoints.size() < 8) return { 0.0f, 0.0f, 0.0f };

    Vec3 elbowPos = armJoints[3].position;
    Vec3 originalWristPos = armJoints[7].position; // Last joint is wrist

    // Transform wrist position based on elbow bend
    Vec3 relativePos = {
        originalWristPos.x - elbowPos.x,
        originalWristPos.y - elbowPos.y,
        originalWristPos.z - elbowPos.z
    };

    // Apply rotation around X-axis (pitch rotation for bending up/down)
    float bendRad = lowerArmBend * PI / 180.0f;
    float cosB = cosf(bendRad);
    float sinB = sinf(bendRad);

    Vec3 rotatedPos = {
        relativePos.x,
        relativePos.y * cosB - relativePos.z * sinB,
        relativePos.y * sinB + relativePos.z * cosB
    };

    // Translate back from elbow origin
    return {
        rotatedPos.x + elbowPos.x,
        rotatedPos.y + elbowPos.y,
        rotatedPos.z + elbowPos.z
    };
}

// Draw complete anatomically complex arm with texture for hand 1
static void drawLowPolyArm(const std::vector<ArmJoint>& armJoints) {
    if (armJoints.empty()) return;

    // Determine which arm this is and get the appropriate bend angle from boxing stance + slow arm bend
    float lowerArmBend = 0.0f;
    if (&armJoints == &g_ArmJoints) {
        // Combine boxing stance + slow bend (negate only the slow bend for forward direction)
        lowerArmBend = gCurrentLeftElbowBend + (-gLeftLowerArmBend);
    }
    else if (&armJoints == &g_ArmJoints2) {
        // Combine boxing stance + slow bend (negate only the slow bend for forward direction)
        lowerArmBend = gCurrentRightElbowBend + (-gRightLowerArmBend);
    }

    // Calculate transformed lower arm joint positions when bending
    std::vector<Vec3> transformedJoints;
    transformedJoints.resize(armJoints.size());

    // Copy upper arm joints (0-3) without transformation
    for (size_t i = 0; i <= 3; ++i) {
        transformedJoints[i] = armJoints[i].position;
    }

    // Transform lower arm joints (4-7) based on elbow bend
    Vec3 elbowPos = armJoints[3].position;
    for (size_t i = 4; i < armJoints.size(); ++i) {
        Vec3 originalPos = armJoints[i].position;
        // Translate to elbow origin
        Vec3 relativePos = { originalPos.x - elbowPos.x, originalPos.y - elbowPos.y, originalPos.z - elbowPos.z };

        // Apply rotation around X-axis (pitch rotation for bending up/down)
        float bendRad = lowerArmBend * PI / 180.0f;
        float cosB = cosf(bendRad);
        float sinB = sinf(bendRad);

        Vec3 rotatedPos = {
            relativePos.x,
            relativePos.y * cosB - relativePos.z * sinB,
            relativePos.y * sinB + relativePos.z * cosB
        };

        // Translate back from elbow origin
        transformedJoints[i] = {
            rotatedPos.x + elbowPos.x,
            rotatedPos.y + elbowPos.y,
            rotatedPos.z + elbowPos.z
        };
    }

    // Draw arm segments - upper arm (shoulder to elbow)
    Vec3 shoulderSocket = { 0.0f, 0.0f, 0.0f };  // Start from shoulder socket
    Vec3 wristConnect = { 0.0f, 0.0f, 0.12f };  // Extended connection to hand

    // Add shoulder connector piece
    drawAnatomicalArmSegment(shoulderSocket, { 0.0f, -0.1f, 0.0f }, 1.2f, 1.0f, 1.0f, 0.8f);

    // Upper arm segments (joints 0-3) - use original positions
    drawAnatomicalArmSegment(wristConnect, transformedJoints[1], 0.9f, 0.7f, 0.7f, 0.5f);
    drawAnatomicalArmSegment(transformedJoints[1], transformedJoints[2], 1.0f, 0.8f, 0.8f, 0.6f);
    drawAnatomicalArmSegment(transformedJoints[2], transformedJoints[3], 1.1f, 0.9f, 0.9f, 0.65f);

    // Lower arm segments (joints 3-7) - use transformed positions
    drawAnatomicalArmSegment(transformedJoints[3], transformedJoints[4], 1.05f, 0.85f, 0.85f, 0.7f);
    drawAnatomicalArmSegment(transformedJoints[4], transformedJoints[5], 1.0f, 0.8f, 0.8f, 0.65f);
    drawAnatomicalArmSegment(transformedJoints[5], transformedJoints[6], 0.95f, 0.75f, 0.75f, 0.6f);
    drawAnatomicalArmSegment(transformedJoints[6], transformedJoints[7], 0.9f, 0.7f, 0.7f, 0.55f);

    // Add wrist extension that bridges directly to hand positioning
    // This extension should match exactly where the hand will be positioned
    Vec3 wristToHandBridge = {
        transformedJoints[7].x,
        transformedJoints[7].y + 0.02f,  // Small Y offset matching hand positioning
        transformedJoints[7].z + 0.15f   // Bridge forward to where hand base will be
    };
    drawAnatomicalArmSegment(transformedJoints[7], wristToHandBridge, 0.85f, 0.65f, 0.65f, 0.5f);

    // Draw joints using transformed positions
    for (size_t i = 1; i < armJoints.size(); ++i) {
        Vec3 pos = transformedJoints[i];
        float jointSize = 0.15f + i * 0.02f;
        drawRobotJoint(pos, jointSize);
    }

    // ========== INTEGRATED HAND DRAWING ==========
    // Draw the hand directly attached to the final wrist joint
    glPushMatrix();

    // Move to the exact transformed wrist position
    Vec3 finalWrist = transformedJoints[7];
    glTranslatef(finalWrist.x, finalWrist.y, finalWrist.z);

    // Add small offset to position hand properly relative to wrist
    glTranslatef(0.0f, 0.02f, 0.15f);

    // Determine which hand this is and apply appropriate rotations
    if (&armJoints == &g_ArmJoints) {
        // Left hand - rotate based on boxing stance
        if (gInBoxingStance) {
            // Boxing stance: palm facing down
            glRotatef(90.0f, 1.0f, 0.0f, 0.0f);  // Rotate around X-axis to make palm face down
            glRotatef(180.0f, 0.0f, 0.0f, 1.0f); // Additional Z rotation for proper orientation
        }
        else {
            // Normal stance: palm facing forward
            glRotatef(180.0f, 0.0f, 0.0f, 1.0f);
        }
    }
    else if (&armJoints == &g_ArmJoints2) {
        // Right hand - rotate based on boxing stance
        if (gInBoxingStance) {
            // Boxing stance: palm facing down
            glRotatef(90.0f, 1.0f, 0.0f, 0.0f);  // Rotate around X-axis to make palm face down
            glRotatef(180.0f, 0.0f, 0.0f, 1.0f); // Additional Z rotation for proper orientation
        }
        else {
            // Normal stance: palm facing forward
            glRotatef(180.0f, 0.0f, 0.0f, 1.0f);
        }
    }

    // Scale the hand appropriately
    glScalef(HAND_SCALE * 1.0f, HAND_SCALE * 1.0f, HAND_SCALE * 1.1f);

    // Draw the appropriate hand
    if (&armJoints == &g_ArmJoints) {
        // Left hand
        if (gConcreteHands) {
            // Draw concrete hands
            glColor3f(0.6f, 0.6f, 0.65f); // Concrete color
            drawConcretePalm(g_HandJoints);
            drawConcreteThumb(g_HandJoints);
            drawConcreteFinger(g_HandJoints, 5, 6, 7, 8);
            drawConcreteFinger(g_HandJoints, 9, 10, 11, 12);
            drawConcreteFinger(g_HandJoints, 13, 14, 15, 16);
            drawConcreteFinger(g_HandJoints, 17, 18, 19, 20);
        }
        else {
            // Draw skin hands
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
        }
    }
    else if (&armJoints == &g_ArmJoints2) {
        // Right hand
        if (gConcreteHands) {
            // Draw concrete hands
            glColor3f(0.6f, 0.6f, 0.65f); // Concrete color
            drawConcretePalm(g_HandJoints2);
            drawConcreteThumb(g_HandJoints2);
            drawConcreteFinger(g_HandJoints2, 5, 6, 7, 8);
            drawConcreteFinger(g_HandJoints2, 9, 10, 11, 12);
            drawConcreteFinger(g_HandJoints2, 13, 14, 15, 16);
            drawConcreteFinger(g_HandJoints2, 17, 18, 19, 20);
        }
        else {
            // Draw skin hands
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
        }
    }

    glPopMatrix();
    // ========== END INTEGRATED HAND DRAWING ==========

    // ========== JOINT VISUALIZATIONS ==========
    // Draw elbow joint sphere (pivot point) - bright red
    drawJointSphere(transformedJoints[3], 0.08f, 1.0f, 0.0f, 0.0f);

    // Draw wrist joint box (end of lower arm) - bright green
    drawJointBox(transformedJoints[7], 0.1f, 0.0f, 1.0f, 0.0f);

    // Draw additional joint markers for better visualization
    for (size_t i = 1; i < transformedJoints.size(); ++i) {
        Vec3 pos = transformedJoints[i];
        if (i == 3) {
            // Elbow - larger red sphere
            drawJointSphere(pos, 0.1f, 1.0f, 0.0f, 0.0f);
        }
        else if (i == 7) {
            // Wrist - green box
            drawJointBox(pos, 0.08f, 0.0f, 1.0f, 0.0f);
        }
        else {
            // Other joints - small yellow spheres
            drawJointSphere(pos, 0.05f, 1.0f, 1.0f, 0.0f);
        }
    }
    // ========== END JOINT VISUALIZATIONS ==========
}

// Draw a connecting segment from wrist joint to palm base to ensure seamless connection
static void drawWristToPalmConnector(const Vec3& wristJointPos, const Vec3& palmBasePos) {
    if (g_TextureEnabled) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, g_HandTexture);
        glColor3f(1.0f, 1.0f, 1.0f);
    }
    else {
        glDisable(GL_TEXTURE_2D);
        glColor3f(0.85f, 0.64f, 0.52f); // Skin color
    }

    // Create a simple cylindrical connector
    const int sides = 8;
    float angleStep = 2.0f * PI / sides;
    float radius = 0.12f; // Connector radius

    glBegin(GL_TRIANGLES);

    // Cylindrical sides connecting wrist to palm
    for (int i = 0; i < sides; i++) {
        int next = (i + 1) % sides;

        float angle1 = i * angleStep;
        float angle2 = next * angleStep;

        float x1 = cosf(angle1) * radius;
        float y1 = sinf(angle1) * radius;
        float x2 = cosf(angle2) * radius;
        float y2 = sinf(angle2) * radius;

        // First triangle
        glNormal3f((x1 + x2) * 0.5f, (y1 + y2) * 0.5f, 0.0f);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(wristJointPos.x + x1, wristJointPos.y + y1, wristJointPos.z);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(wristJointPos.x + x2, wristJointPos.y + y2, wristJointPos.z);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(palmBasePos.x + x2, palmBasePos.y + y2, palmBasePos.z);

        // Second triangle
        glTexCoord2f(0.0f, 0.0f); glVertex3f(wristJointPos.x + x1, wristJointPos.y + y1, wristJointPos.z);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(palmBasePos.x + x2, palmBasePos.y + y2, palmBasePos.z);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(palmBasePos.x + x1, palmBasePos.y + y1, palmBasePos.z);
    }

    glEnd();
}

// Draw a connecting segment from hand base toward the wrist to ensure seamless connection
static void drawHandEdgeConnector() {
    if (g_TextureEnabled) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, g_HandTexture);
        glColor3f(1.0f, 1.0f, 1.0f);
    }
    else {
        glDisable(GL_TEXTURE_2D);
        glColor3f(0.85f, 0.64f, 0.52f); // Skin color
    }

    // Draw connector segment from hand base backward to bridge any gap to wrist
    // This creates a seamless connection between the wrist extension and hand
    Vec3 handBase = { 0.0f, 0.0f, -0.1f };      // Hand base position (relative to hand coordinate system)
    Vec3 wristBridge = { 0.0f, -0.02f, -0.25f }; // Connect back toward wrist extension point

    // Draw the bridging segment using the anatomical arm segment function for consistency
    drawAnatomicalArmSegment(wristBridge, handBase, 0.8f, 0.6f, 0.6f, 0.45f);

    // Add a joint at the connection point for visual consistency
    drawRobotJoint(wristBridge, 0.12f);
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

    // Layer 0: Extended wrist base (reaching back toward arm) - MAXIMALLY EXTENDED for guaranteed connection
    palmGrid[0][0] = { wrist.x - 0.8f, wrist.y - 0.05f, wrist.z - 0.6f };   // Much further back
    palmGrid[0][1] = { wrist.x - 0.6f, wrist.y - 0.03f, wrist.z - 0.55f };  // Much further back
    palmGrid[0][2] = { wrist.x - 0.4f, wrist.y - 0.01f, wrist.z - 0.5f };   // Much further back
    palmGrid[0][3] = { wrist.x - 0.2f, wrist.y + 0.0f, wrist.z - 0.47f };   // Much further back
    palmGrid[0][4] = { wrist.x + 0.0f, wrist.y + 0.01f, wrist.z - 0.45f };  // Much further back
    palmGrid[0][5] = { wrist.x + 0.2f, wrist.y + 0.0f, wrist.z - 0.47f };   // Much further back
    palmGrid[0][6] = { wrist.x + 0.4f, wrist.y - 0.01f, wrist.z - 0.5f };   // Much further back
    palmGrid[0][7] = { wrist.x + 0.6f, wrist.y - 0.03f, wrist.z - 0.55f };  // Much further back
    palmGrid[0][8] = { wrist.x + 0.8f, wrist.y - 0.05f, wrist.z - 0.6f };   // Much further back

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

    // Layer 0: Wrist base (MAXIMALLY EXTENDED for guaranteed connection)
    palmGrid[0][0] = { wrist.x - 0.8f, wrist.y - 0.05f, wrist.z - 0.6f };   // Much further back
    palmGrid[0][1] = { wrist.x - 0.6f, wrist.y - 0.03f, wrist.z - 0.55f };  // Much further back
    palmGrid[0][2] = { wrist.x - 0.4f, wrist.y - 0.01f, wrist.z - 0.5f };   // Much further back
    palmGrid[0][3] = { wrist.x - 0.2f, wrist.y + 0.0f, wrist.z - 0.47f };   // Much further back
    palmGrid[0][4] = { wrist.x + 0.0f, wrist.y + 0.01f, wrist.z - 0.45f };  // Much further back
    palmGrid[0][5] = { wrist.x + 0.2f, wrist.y + 0.0f, wrist.z - 0.47f };   // Much further back
    palmGrid[0][6] = { wrist.x + 0.4f, wrist.y - 0.01f, wrist.z - 0.5f };   // Much further back
    palmGrid[0][7] = { wrist.x + 0.6f, wrist.y - 0.03f, wrist.z - 0.55f };  // Much further back
    palmGrid[0][8] = { wrist.x + 0.8f, wrist.y - 0.05f, wrist.z - 0.6f };   // Much further back

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

// =============== CONCRETE HAND & ARM FUNCTIONS ===============

static void drawConcreteFingerSegment(const Vec3& start, const Vec3& end, float radius) {
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

    // Concrete texture appearance - more angular/blocky
    glColor3f(0.6f, 0.6f, 0.65f); // Gray concrete color

    // Draw as angular cylinder with concrete-like surface
    const int sides = 6; // Hexagonal for more concrete-like appearance
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < sides; i++) {
        float angle1 = (float)i * 2.0f * PI / sides;
        float angle2 = (float)(i + 1) * 2.0f * PI / sides;

        float x1 = radius * cosf(angle1);
        float y1 = radius * sinf(angle1);
        float x2 = radius * cosf(angle2);
        float y2 = radius * sinf(angle2);

        // Side faces with rough normals
        Vec3f n = normalize({ (x1 + x2) * 0.5f, (y1 + y2) * 0.5f, 0.0f });
        glNormal3f(n.x, n.y, n.z);
        glVertex3f(x1, y1, 0.0f);
        glVertex3f(x2, y2, 0.0f);
        glVertex3f(x2, y2, length);

        glVertex3f(x1, y1, 0.0f);
        glVertex3f(x2, y2, length);
        glVertex3f(x1, y1, length);
    }
    glEnd();

    // Add concrete-like texture details
    glColor3f(0.5f, 0.5f, 0.55f); // Slightly darker for texture
    glBegin(GL_LINES);
    for (int i = 0; i < sides; i++) {
        float angle = (float)i * 2.0f * PI / sides;
        float x = radius * cosf(angle);
        float y = radius * sinf(angle);
        glVertex3f(x, y, 0.0f);
        glVertex3f(x, y, length);
    }
    glEnd();

    glPopMatrix();
}

static void drawConcretePalm(const std::vector<HandJoint>& joints) {
    glColor3f(0.6f, 0.6f, 0.65f); // Concrete gray

    Vec3 wrist = joints[0].position;
    Vec3 indexBase = joints[5].position;
    Vec3 middleBase = joints[9].position;
    Vec3 ringBase = joints[13].position;
    Vec3 pinkyBase = joints[17].position;
    Vec3 thumbBase = joints[1].position;

    // Draw palm as concrete block
    glBegin(GL_TRIANGLES);

    // Palm surface - more angular/geometric
    Vec3 palmCenter = { (indexBase.x + pinkyBase.x) * 0.5f,
                       (indexBase.y + pinkyBase.y) * 0.5f + 0.1f,
                       (indexBase.z + pinkyBase.z) * 0.5f };

    // Create blocky palm geometry
    Vec3 corners[8] = {
        { wrist.x - 0.6f, wrist.y - 0.1f, wrist.z - 0.2f },
        { wrist.x + 0.6f, wrist.y - 0.1f, wrist.z - 0.2f },
        { palmCenter.x + 0.6f, palmCenter.y, palmCenter.z },
        { palmCenter.x - 0.6f, palmCenter.y, palmCenter.z },
        { wrist.x - 0.6f, wrist.y + 0.3f, wrist.z - 0.2f },
        { wrist.x + 0.6f, wrist.y + 0.3f, wrist.z - 0.2f },
        { palmCenter.x + 0.6f, palmCenter.y + 0.3f, palmCenter.z },
        { palmCenter.x - 0.6f, palmCenter.y + 0.3f, palmCenter.z }
    };

    // Draw cube faces
    int faces[12][3] = {
        {0,1,2}, {0,2,3}, // bottom
        {4,6,5}, {4,7,6}, // top  
        {0,4,1}, {1,4,5}, // front
        {2,6,3}, {3,6,7}, // back
        {0,3,4}, {3,7,4}, // left
        {1,5,2}, {2,5,6}  // right
    };

    for (int i = 0; i < 12; i++) {
        Vec3 v1 = corners[faces[i][0]];
        Vec3 v2 = corners[faces[i][1]];
        Vec3 v3 = corners[faces[i][2]];

        // Calculate normal using Vec3f functions
        Vec3f v1f = { v1.x, v1.y, v1.z };
        Vec3f v2f = { v2.x, v2.y, v2.z };
        Vec3f v3f = { v3.x, v3.y, v3.z };
        Vec3f normal = normalize(cross(sub(v2f, v1f), sub(v3f, v1f)));
        glNormal3f(normal.x, normal.y, normal.z);
        glVertex3f(v1.x, v1.y, v1.z);
        glVertex3f(v2.x, v2.y, v2.z);
        glVertex3f(v3.x, v3.y, v3.z);
    }
    glEnd();
}

static void drawConcreteThumb(const std::vector<HandJoint>& joints) {
    drawConcreteFingerSegment(joints[1].position, joints[2].position, 0.22f);
    drawConcreteFingerSegment(joints[2].position, joints[3].position, 0.19f);
    drawConcreteFingerSegment(joints[3].position, joints[4].position, 0.16f);
}

static void drawConcreteFinger(const std::vector<HandJoint>& joints, int mcpIdx, int pipIdx, int dipIdx, int tipIdx) {
    if (mcpIdx < joints.size() && pipIdx < joints.size() && dipIdx < joints.size() && tipIdx < joints.size()) {
        drawConcreteFingerSegment(joints[mcpIdx].position, joints[pipIdx].position, 0.20f);
        drawConcreteFingerSegment(joints[pipIdx].position, joints[dipIdx].position, 0.17f);
        drawConcreteFingerSegment(joints[dipIdx].position, joints[tipIdx].position, 0.14f);
    }
}

// =============== REALISTIC KUNG FU HAND FORMS ===============

void setHandForm(std::vector<HandJoint>& joints, HandForm form) {
    switch (form) {
    case HAND_FIST: { // Closed fist - all fingers curled inward
        // Thumb - bent inward across palm
        joints[1].position = { 0.3f, -0.2f, 0.3f };
        joints[2].position = { 0.4f, -0.4f, 0.5f };
        joints[3].position = { 0.3f, -0.5f, 0.7f };
        joints[4].position = { 0.2f, -0.6f, 0.8f };

        // Index finger - curled into fist
        joints[5].position = { 0.2f, 0.1f, 1.0f };
        joints[6].position = { 0.1f, -0.2f, 1.3f };
        joints[7].position = { 0.0f, -0.5f, 1.4f };
        joints[8].position = { -0.1f, -0.7f, 1.3f };

        // Middle finger - curled into fist
        joints[9].position = { 0.0f, 0.1f, 1.1f };
        joints[10].position = { -0.1f, -0.2f, 1.4f };
        joints[11].position = { -0.2f, -0.5f, 1.5f };
        joints[12].position = { -0.3f, -0.7f, 1.4f };

        // Ring finger - curled into fist
        joints[13].position = { -0.2f, 0.1f, 1.0f };
        joints[14].position = { -0.3f, -0.2f, 1.3f };
        joints[15].position = { -0.4f, -0.5f, 1.4f };
        joints[16].position = { -0.5f, -0.7f, 1.3f };

        // Pinky - curled into fist
        joints[17].position = { -0.4f, 0.05f, 0.8f };
        joints[18].position = { -0.5f, -0.2f, 1.1f };
        joints[19].position = { -0.6f, -0.4f, 1.2f };
        joints[20].position = { -0.7f, -0.6f, 1.1f };
        break;
    }
    case HAND_GRIP_SPEAR: {
        joints[1].position = { 0.4f, -0.1f, 0.3f };
        joints[2].position = { 0.5f, -0.2f, 0.5f };
        joints[3].position = { 0.4f, -0.3f, 0.7f };
        joints[4].position = { 0.3f, -0.4f, 0.9f };
        joints[5].position = { 0.3f, 0.1f, 1.0f };
        joints[6].position = { 0.3f, 0.0f, 1.5f };
        joints[7].position = { 0.2f, -0.3f, 1.6f };
        joints[8].position = { 0.1f, -0.5f, 1.5f };
        joints[9].position = { 0.0f, 0.1f, 1.1f };
        joints[10].position = { 0.0f, 0.0f, 1.7f };
        joints[11].position = { -0.1f, -0.3f, 1.8f };
        joints[12].position = { -0.2f, -0.5f, 1.7f };
        joints[13].position = { -0.3f, 0.1f, 1.0f };
        joints[14].position = { -0.3f, 0.0f, 1.6f };
        joints[15].position = { -0.4f, -0.3f, 1.7f };
        joints[16].position = { -0.5f, -0.5f, 1.6f };
        joints[17].position = { -0.5f, 0.05f, 0.8f };
        joints[18].position = { -0.5f, -0.1f, 1.2f };
        joints[19].position = { -0.6f, -0.3f, 1.3f };
        joints[20].position = { -0.7f, -0.5f, 1.2f };
        break;
    }
    case HAND_CLAW: { // Tiger Claw - fingers curved like attacking claws
        // Thumb - curved inward
        joints[1].position = { 0.4f, -0.1f, 0.3f };
        joints[2].position = { 0.7f, -0.2f, 0.5f };
        joints[3].position = { 0.9f, -0.3f, 0.6f };
        joints[4].position = { 1.0f, -0.4f, 0.7f };

        // Index finger - strongly curved claw
        joints[5].position = { 0.3f, 0.1f, 1.0f };
        joints[6].position = { 0.4f, -0.1f, 1.5f };
        joints[7].position = { 0.5f, -0.4f, 1.8f };
        joints[8].position = { 0.6f, -0.6f, 1.9f };

        // Middle finger - curved claw
        joints[9].position = { 0.0f, 0.1f, 1.1f };
        joints[10].position = { 0.1f, -0.1f, 1.6f };
        joints[11].position = { 0.2f, -0.4f, 1.9f };
        joints[12].position = { 0.3f, -0.6f, 2.0f };

        // Ring finger - curved claw
        joints[13].position = { -0.3f, 0.1f, 1.0f };
        joints[14].position = { -0.2f, -0.1f, 1.5f };
        joints[15].position = { -0.1f, -0.4f, 1.8f };
        joints[16].position = { 0.0f, -0.6f, 1.9f };

        // Pinky - small claw
        joints[17].position = { -0.5f, 0.05f, 0.8f };
        joints[18].position = { -0.4f, -0.1f, 1.2f };
        joints[19].position = { -0.3f, -0.3f, 1.4f };
        joints[20].position = { -0.2f, -0.5f, 1.5f };
        break;
    }

    case HAND_SPEAR: { // Snake Hand - fingers straight and together
        // Thumb - tucked along palm
        joints[1].position = { 0.3f, -0.2f, 0.2f };
        joints[2].position = { 0.4f, -0.3f, 0.4f };
        joints[3].position = { 0.5f, -0.4f, 0.5f };
        joints[4].position = { 0.6f, -0.5f, 0.6f };

        // All fingers straight and close together
        joints[5].position = { 0.1f, 0.1f, 1.0f };
        joints[6].position = { 0.1f, 0.0f, 1.7f };
        joints[7].position = { 0.1f, 0.0f, 2.2f };
        joints[8].position = { 0.1f, 0.0f, 2.6f };

        joints[9].position = { 0.0f, 0.1f, 1.1f };
        joints[10].position = { 0.0f, 0.0f, 1.8f };
        joints[11].position = { 0.0f, 0.0f, 2.3f };
        joints[12].position = { 0.0f, 0.0f, 2.7f };

        joints[13].position = { -0.1f, 0.1f, 1.0f };
        joints[14].position = { -0.1f, 0.0f, 1.7f };
        joints[15].position = { -0.1f, 0.0f, 2.2f };
        joints[16].position = { -0.1f, 0.0f, 2.6f };

        joints[17].position = { -0.2f, 0.05f, 0.9f };
        joints[18].position = { -0.2f, 0.0f, 1.5f };
        joints[19].position = { -0.2f, 0.0f, 1.9f };
        joints[20].position = { -0.2f, 0.0f, 2.2f };
        break;
    }

    case HAND_CRANE_BEAK: { // Crane Beak - fingertips together
        // All fingertips meet at a point
        Vec3 beakTip = { 0.0f, 0.1f, 2.3f };

        joints[1].position = { 0.3f, -0.1f, 0.4f };
        joints[2].position = { 0.4f, 0.0f, 1.2f };
        joints[3].position = { 0.2f, 0.05f, 1.8f };
        joints[4].position = beakTip;

        joints[5].position = { 0.2f, 0.1f, 1.0f };
        joints[6].position = { 0.2f, 0.05f, 1.6f };
        joints[7].position = { 0.1f, 0.08f, 2.0f };
        joints[8].position = beakTip;

        joints[9].position = { 0.0f, 0.1f, 1.1f };
        joints[10].position = { 0.0f, 0.05f, 1.7f };
        joints[11].position = { 0.0f, 0.1f, 2.1f };
        joints[12].position = beakTip;

        joints[13].position = { -0.2f, 0.1f, 1.0f };
        joints[14].position = { -0.2f, 0.05f, 1.6f };
        joints[15].position = { -0.1f, 0.08f, 2.0f };
        joints[16].position = beakTip;

        joints[17].position = { -0.3f, 0.05f, 0.9f };
        joints[18].position = { -0.3f, 0.0f, 1.4f };
        joints[19].position = { -0.2f, 0.05f, 1.8f };
        joints[20].position = beakTip;
        break;
    }

    case HAND_SWORD_FINGER: { // Two fingers extended (sword hand)
        // Thumb - folded down
        joints[1].position = { 0.4f, -0.2f, 0.2f };
        joints[2].position = { 0.5f, -0.4f, 0.3f };
        joints[3].position = { 0.6f, -0.5f, 0.4f };
        joints[4].position = { 0.7f, -0.6f, 0.5f };

        // Index and middle fingers extended (sword fingers)
        joints[5].position = { 0.2f, 0.1f, 1.0f };
        joints[6].position = { 0.2f, 0.0f, 1.7f };
        joints[7].position = { 0.2f, 0.0f, 2.2f };
        joints[8].position = { 0.2f, 0.0f, 2.6f };

        joints[9].position = { -0.2f, 0.1f, 1.1f };
        joints[10].position = { -0.2f, 0.0f, 1.8f };
        joints[11].position = { -0.2f, 0.0f, 2.3f };
        joints[12].position = { -0.2f, 0.0f, 2.7f };

        // Ring and pinky folded
        joints[13].position = { -0.4f, 0.1f, 1.0f };
        joints[14].position = { -0.4f, -0.3f, 1.2f };
        joints[15].position = { -0.3f, -0.5f, 1.0f };
        joints[16].position = { -0.2f, -0.6f, 0.8f };

        joints[17].position = { -0.6f, 0.05f, 0.8f };
        joints[18].position = { -0.5f, -0.3f, 0.9f };
        joints[19].position = { -0.4f, -0.5f, 0.7f };
        joints[20].position = { -0.3f, -0.6f, 0.5f };
        break;
    }

    case HAND_PALM_STRIKE: { // Open palm ready for striking
        // Thumb extended and strong
        joints[1].position = { 0.6f, -0.1f, 0.4f };
        joints[2].position = { 0.9f, 0.0f, 0.7f };
        joints[3].position = { 1.1f, 0.1f, 0.9f };
        joints[4].position = { 1.2f, 0.2f, 1.1f };

        // All fingers extended and spread for maximum surface area
        joints[5].position = { 0.4f, 0.1f, 1.0f };
        joints[6].position = { 0.4f, 0.0f, 1.8f };
        joints[7].position = { 0.4f, 0.0f, 2.3f };
        joints[8].position = { 0.4f, 0.0f, 2.7f };

        joints[9].position = { 0.0f, 0.1f, 1.1f };
        joints[10].position = { 0.0f, 0.0f, 1.9f };
        joints[11].position = { 0.0f, 0.0f, 2.4f };
        joints[12].position = { 0.0f, 0.0f, 2.8f };

        joints[13].position = { -0.4f, 0.1f, 1.0f };
        joints[14].position = { -0.4f, 0.0f, 1.8f };
        joints[15].position = { -0.4f, 0.0f, 2.3f };
        joints[16].position = { -0.4f, 0.0f, 2.7f };

        joints[17].position = { -0.6f, 0.05f, 0.8f };
        joints[18].position = { -0.6f, 0.0f, 1.5f };
        joints[19].position = { -0.6f, 0.0f, 1.9f };
        joints[20].position = { -0.6f, 0.0f, 2.2f };
        break;
    }

    case HAND_GRIP_SWORD: { // Proper sword grip - fingers wrapped around hilt
        // Thumb - wrapped around grip
        joints[1].position = { 0.5f, -0.1f, 0.4f };
        joints[2].position = { 0.6f, -0.2f, 0.6f };
        joints[3].position = { 0.5f, -0.3f, 0.8f };
        joints[4].position = { 0.4f, -0.4f, 1.0f };

        // Index finger - wrapped around hilt
        joints[5].position = { 0.2f, 0.1f, 1.0f };
        joints[6].position = { 0.1f, -0.1f, 1.4f };
        joints[7].position = { 0.0f, -0.3f, 1.6f };
        joints[8].position = { -0.1f, -0.5f, 1.5f };

        // Middle finger - firmly gripping
        joints[9].position = { 0.0f, 0.1f, 1.1f };
        joints[10].position = { -0.1f, -0.1f, 1.5f };
        joints[11].position = { -0.2f, -0.3f, 1.7f };
        joints[12].position = { -0.3f, -0.5f, 1.6f };

        // Ring finger - supporting grip
        joints[13].position = { -0.2f, 0.1f, 1.0f };
        joints[14].position = { -0.3f, -0.1f, 1.4f };
        joints[15].position = { -0.4f, -0.3f, 1.6f };
        joints[16].position = { -0.5f, -0.5f, 1.5f };

        // Pinky - completing the grip
        joints[17].position = { -0.4f, 0.05f, 0.8f };
        joints[18].position = { -0.5f, -0.1f, 1.1f };
        joints[19].position = { -0.6f, -0.3f, 1.3f };
        joints[20].position = { -0.7f, -0.5f, 1.2f };
        break;
    }

    default:
        // Keep original hand positions for HAND_OPEN and HAND_FIST
        break;
    }
}


void initializeKungFuSequences() {
    // Crane Style Sequence - Graceful, flowing like a crane with precise hand forms
    gCraneSequence[0] = { 45.0f, 90.0f, -30.0f, 0.0f, -20.0f, 20.0f, -30.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                         HAND_CRANE_BEAK, HAND_PALM_STRIKE, 15.0f, 20.0f, -10.0f, 5.0f }; // Preparatory stance
    gCraneSequence[1] = { 120.0f, 45.0f, -60.0f, 30.0f, -40.0f, -10.0f, -60.0f, -30.0f, -15.0f, 10.0f, 5.0f,
                         HAND_CRANE_BEAK, HAND_CRANE_BEAK, 5.0f, 10.0f, -20.0f, -15.0f }; // Wing spread high
    gCraneSequence[2] = { 60.0f, 135.0f, -45.0f, 60.0f, -60.0f, 40.0f, -45.0f, -90.0f, 15.0f, -5.0f, -10.0f,
                         HAND_SPEAR, HAND_CRANE_BEAK, 30.0f, 5.0f, 10.0f, -25.0f }; // Swooping strike
    gCraneSequence[3] = { 90.0f, 75.0f, -20.0f, 15.0f, -30.0f, 25.0f, 0.0f, -15.0f, 0.0f, 0.0f, 0.0f,
                         HAND_PALM_STRIKE, HAND_CRANE_BEAK, 20.0f, 15.0f, 0.0f, 0.0f }; // Return to center

    // Dragon Style Sequence - Powerful, serpentine movements with flowing hands
    gDragonSequence[0] = { 75.0f, 75.0f, -20.0f, 20.0f, -30.0f, 30.0f, -20.0f, -20.0f, 0.0f, 0.0f, 0.0f,
                          HAND_CLAW, HAND_CLAW, 25.0f, 25.0f, 5.0f, 5.0f }; // Coiling stance
    gDragonSequence[1] = { 45.0f, 150.0f, -60.0f, 80.0f, -80.0f, 60.0f, -45.0f, -90.0f, -30.0f, 15.0f, 20.0f,
                          HAND_CLAW, HAND_SPEAR, 10.0f, 5.0f, 15.0f, -20.0f }; // Rising dragon
    gDragonSequence[2] = { 135.0f, 30.0f, 45.0f, -30.0f, 70.0f, -70.0f, -75.0f, -15.0f, 30.0f, -10.0f, -15.0f,
                          HAND_SPEAR, HAND_CLAW, 40.0f, 35.0f, 20.0f, 10.0f }; // Sweeping claw
    gDragonSequence[3] = { 90.0f, 120.0f, 0.0f, 45.0f, -45.0f, 60.0f, -45.0f, -60.0f, 0.0f, 5.0f, 0.0f,
                          HAND_PALM_STRIKE, HAND_CLAW, 20.0f, 15.0f, 0.0f, 5.0f }; // Flowing transition

    // Tiger Style Sequence - Aggressive, explosive movements with powerful claws
    gTigerSequence[0] = { 90.0f, 90.0f, -40.0f, 40.0f, -50.0f, 50.0f, -30.0f, -30.0f, 0.0f, 0.0f, 0.0f,
                         HAND_CLAW, HAND_CLAW, 30.0f, 30.0f, 10.0f, 10.0f }; // Stalking stance
    gTigerSequence[1] = { 135.0f, 60.0f, -80.0f, -20.0f, -90.0f, 20.0f, -75.0f, -15.0f, -20.0f, 20.0f, 15.0f,
                         HAND_CLAW, HAND_FIST, 5.0f, 40.0f, 25.0f, -10.0f }; // Pounce preparation  
    gTigerSequence[2] = { 60.0f, 150.0f, -30.0f, 70.0f, -40.0f, 80.0f, -15.0f, -90.0f, 25.0f, -15.0f, -20.0f,
                         HAND_CLAW, HAND_CLAW, 45.0f, 10.0f, 30.0f, 15.0f }; // Striking claw
    gTigerSequence[3] = { 120.0f, 45.0f, -60.0f, 0.0f, -75.0f, 30.0f, -60.0f, 0.0f, -10.0f, 10.0f, 5.0f,
                         HAND_PALM_STRIKE, HAND_CLAW, 25.0f, 35.0f, 15.0f, 5.0f }; // Recovery stance
}

// Smooth interpolation between poses
KungFuPose lerpPose(const KungFuPose& from, const KungFuPose& to, float t) {
    // Apply easing function for smoother animation
    float easedT = t * t * (3.0f - 2.0f * t); // Smooth step

    KungFuPose result;
    result.leftShoulderPitch = from.leftShoulderPitch + (to.leftShoulderPitch - from.leftShoulderPitch) * easedT;
    result.rightShoulderPitch = from.rightShoulderPitch + (to.rightShoulderPitch - from.rightShoulderPitch) * easedT;
    result.leftShoulderYaw = from.leftShoulderYaw + (to.leftShoulderYaw - from.leftShoulderYaw) * easedT;
    result.rightShoulderYaw = from.rightShoulderYaw + (to.rightShoulderYaw - from.rightShoulderYaw) * easedT;
    result.leftShoulderRoll = from.leftShoulderRoll + (to.leftShoulderRoll - from.leftShoulderRoll) * easedT;
    result.rightShoulderRoll = from.rightShoulderRoll + (to.rightShoulderRoll - from.rightShoulderRoll) * easedT;
    result.leftArmAngle = from.leftArmAngle + (to.leftArmAngle - from.leftArmAngle) * easedT;
    result.rightArmAngle = from.rightArmAngle + (to.rightArmAngle - from.rightArmAngle) * easedT;
    result.torsoYaw = from.torsoYaw + (to.torsoYaw - from.torsoYaw) * easedT;
    result.torsoPitch = from.torsoPitch + (to.torsoPitch - from.torsoPitch) * easedT;
    result.torsoRoll = from.torsoRoll + (to.torsoRoll - from.torsoRoll) * easedT;
    result.leftElbowBend = from.leftElbowBend + (to.leftElbowBend - from.leftElbowBend) * easedT;
    result.rightElbowBend = from.rightElbowBend + (to.rightElbowBend - from.rightElbowBend) * easedT;
    result.leftWristPitch = from.leftWristPitch + (to.leftWristPitch - from.leftWristPitch) * easedT;
    result.rightWristPitch = from.rightWristPitch + (to.rightWristPitch - from.rightWristPitch) * easedT;

    // For hand forms, switch at halfway point for crisp transitions
    result.leftHandForm = (easedT < 0.5f) ? from.leftHandForm : to.leftHandForm;
    result.rightHandForm = (easedT < 0.5f) ? from.rightHandForm : to.rightHandForm;

    return result;
}

void updateKungFuAnimation(float deltaTime) {
    if (!gKungFuAnimating) return;

    gKungFuAnimationTime += deltaTime * KUNGFU_ANIMATION_SPEED;

    // Calculate which phase we're in and progress within that phase
    float totalPhaseTime = gKungFuAnimationTime;
    int currentPhase = (int)(totalPhaseTime / PHASE_DURATION) % 4;
    float phaseProgress = fmodf(totalPhaseTime, PHASE_DURATION) / PHASE_DURATION;

    // Get the current sequence based on style
    KungFuPose* sequence;
    switch (gKungFuPattern) {
    case 1: sequence = gCraneSequence; break;
    case 2: sequence = gDragonSequence; break;
    case 3: sequence = gTigerSequence; break;
    default:
        gKungFuAnimating = false;
        return;
    }

    // Interpolate between current and next keyframe
    KungFuPose fromPose = sequence[currentPhase];
    KungFuPose toPose = sequence[(currentPhase + 1) % 4];

    gCurrentPose = lerpPose(fromPose, toPose, phaseProgress);

    // Update global pose for torso
    g_pose.torsoYaw = gCurrentPose.torsoYaw;
    g_pose.torsoPitch = gCurrentPose.torsoPitch;
    g_pose.torsoRoll = gCurrentPose.torsoRoll;
}

void startKungFuAnimation(int style) {
    if (style == 0) {
        gKungFuAnimating = false;
        gKungFuPattern = 0;
        // Reset to neutral pose
        g_pose.torsoYaw = 0.0f;
        g_pose.torsoPitch = 0.0f;
        g_pose.torsoRoll = 0.0f;
        return;
    }

    gKungFuPattern = style;
    gKungFuAnimating = true;
    gKungFuAnimationTime = 0.0f;
    gKungFuAnimationPhase = 0;
}

// --------------------- Animation and Drawing from leg.cpp ---------------------
void animateLegVertices(float hipAngle, float kneeAngle, bool mirror) {
    if (gAnimatedVertices.size() != gAllVertices.size()) {
        gAnimatedVertices.resize(gAllVertices.size());
        gAnimatedNormals.resize(gAllVertices.size());
    }

    float hipRad = hipAngle * (float)M_PI / 180.0f;
    float kneeRad = kneeAngle * (float)M_PI / 180.0f;

    float cosHip = cosf(hipRad), sinHip = sinf(hipRad); // Use sinf/cosf
    float cosKnee = cosf(kneeRad), sinKnee = sinf(kneeRad); // Use sinf/cosf

    const float kneeY = 4.5f, kneeZ = 0.2f;
    const float hipY = 8.5f, hipZ = -0.2f;

    for (size_t i = 0; i < gAllVertices.size(); ++i) {
        Vec3f v = gAllVertices[i];
        Vec3f n = gVertexNormals[i];

        // 1. Knee bend (only affects lower leg vertices)
        if (v.y <= kneeY) {
            float translatedY = v.y - kneeY;
            float translatedZ = v.z - kneeZ;
            v.y = translatedY * cosKnee - translatedZ * sinKnee + kneeY;
            v.z = translatedY * sinKnee + translatedZ * cosKnee + kneeZ;

            // Also rotate the normal
            float normalY = n.y;
            float normalZ = n.z;
            n.y = normalY * cosKnee - normalZ * sinKnee;
            n.z = normalY * sinKnee + normalZ * cosKnee;
        }

        // 2. Hip bend (affects all vertices)
        float translatedY = v.y - hipY;
        float translatedZ = v.z - hipZ;
        v.y = translatedY * cosHip - translatedZ * sinHip + hipY;
        v.z = translatedY * sinHip + translatedZ * cosHip + hipZ;

        // Also rotate the normal
        float normalY = n.y;
        float normalZ = n.z;
        n.y = normalY * cosHip - normalZ * sinHip;
        n.z = normalY * sinHip + normalZ * cosHip;

        // 3. Mirror if necessary
        if (mirror) {
            v.x *= -1.0f;
            n.x *= -1.0f;
        }

        gAnimatedVertices[i] = v;
        gAnimatedNormals[i] = n;
    }
}

void drawLeg() {
    // Set material/texture based on the current render mode
    if (gRenderMode == RM_TEXTURED) {
        glEnable(GL_TEXTURE_2D);
        Tex::bind(Tex::id[Tex::Skin]);
        glColor3f(1.0f, 1.0f, 1.0f);
    }
    else {
        glDisable(GL_TEXTURE_2D);
        glColor3f(0.9f, 0.7f, 0.6f); // Solid/wireframe skin color
    }

    glBegin(GL_TRIANGLES);
    for (const auto& t : gTris) {
        auto emit = [&](int vertexIndex) {
            const Vec2f& uv = gTexCoords[vertexIndex];
            const Vec3f& n = gAnimatedNormals[vertexIndex];
            const Vec3f& v = gAnimatedVertices[vertexIndex];

            glNormal3f(n.x, n.y, n.z);
            if (gRenderMode == RM_TEXTURED) {
                // Flip V-coordinate for BMPs which are often stored upside down
                glTexCoord2f(uv.u, 1.0f - uv.v);
            }
            glVertex3f(v.x, v.y, v.z);
            };
        emit(t.a);
        emit(t.b);
        emit(t.c);
    }
    glEnd();

    if (gRenderMode == RM_TEXTURED) {
        Tex::unbind();
    }
}

// --- Body and Head Drawing ---
// Draw Mulan's detailed facial features
void drawMulanEyes() {
    const float headR = HEAD_RADIUS;
    const float eyeY = headR * 0.2f; // Position relative to head center
    const float eyeZ = headR * 0.95f; // Moved forward to face surface
    const float eyeSpacing = 0.1f; // Slightly wider spacing

    // Apply eye scaling
    glPushMatrix();
    glTranslatef(0.0f, eyeY, eyeZ); // Move to eye level
    glScalef(EYE_SCALE, EYE_SCALE, 1.0f); // Scale eyes
    glTranslatef(0.0f, -eyeY, -eyeZ); // Move back

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

    glPopMatrix(); // End eye scaling
}

void drawMulanEyebrows() {
    const float headR = HEAD_RADIUS;
    const float browY = headR * 0.32f; // Position relative to head center
    const float browZ = headR * 0.92f; // Moved forward to face surface
    const float eyeSpacing = 0.1f; // Match eye spacing

    // Dark brown eyebrows
    glColor3f(0.15f, 0.1f, 0.05f);
    glLineWidth(3.0f * EYEBROW_SCALE); // Scale eyebrow thickness

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

    // Apply nose scaling
    glPushMatrix();
    glTranslatef(0.0f, noseY, noseZ); // Move to nose center
    glScalef(NOSE_SCALE, NOSE_SCALE, NOSE_SCALE); // Scale nose
    glTranslatef(0.0f, -noseY, -noseZ); // Move back

    // Nose bridge (subtle for East Asian features)
    glColor3f(0.85f, 0.65f, 0.55f); // Slightly darker than skin
    glLineWidth(2.0f * NOSE_SCALE); // Scale line width

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
    glPopMatrix(); // End nose scaling
}

void drawMulanMouth() {
    const float headR = HEAD_RADIUS;
    const float mouthY = -headR * 0.3f; // Position relative to head center
    const float mouthZ = headR * 0.95f; // Moved forward to face surface

    // Apply mouth scaling
    glPushMatrix();
    glTranslatef(0.0f, mouthY, mouthZ); // Move to mouth center
    glScalef(MOUTH_SCALE, MOUTH_SCALE, 1.0f); // Scale mouth
    glTranslatef(0.0f, -mouthY, -mouthZ); // Move back

    // Natural lip color
    glColor3f(0.8f, 0.4f, 0.4f);
    glLineWidth(2.5f * MOUTH_SCALE); // Scale line width

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
    glPopMatrix(); // End mouth scaling
}

// [MODIFY] Make texturing conditional & self-contained.
//          Use hair.bmp with object-linear texgen; provide solid fallback when textures are off.
//          Keep your geometry (cap + bob + bangs).

void drawMulanHair() {
    const float headR = HEAD_RADIUS;
    const int   segments = 32;

    glPushMatrix();
    // [KEEP] local volume tweak around the head origin (head is already positioned by caller)
    glTranslatef(0.0f, headR * 0.5f, 0.0f);
    glScalef(HAIR_SCALE, HAIR_SCALE, HAIR_SCALE);
    glTranslatef(0.0f, -headR * 0.5f, 0.0f);

    // [ADD] Texture state (hair.bmp) with object-linear mapping
    bool usingTexture = (gRenderMode == RM_TEXTURED) || g_TextureEnabled;
    if (usingTexture) {
        glEnable(GL_TEXTURE_2D);
        Tex::bind(Tex::id[Tex::Hair]);     // hair.bmp
        Tex::enableObjectLinearST(0.45f, 0.45f, 0.45f);
        glColor3f(1, 1, 1);
    }
    else {
        glDisable(GL_TEXTURE_2D);
        glColor3f(0.07f, 0.07f, 0.07f);    // dark hair fallback
    }

    // --- Part 1: Voluminous Hair Cap ---
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < segments; i++) {
        float angle1 = (float)i / segments * 2.0f * PI;
        float angle2 = (float)(i + 1) / segments * 2.0f * PI;

        float capRadius = headR * 1.15f;
        float x1 = capRadius * cosf(angle1), z1 = capRadius * sinf(angle1);
        float x2 = capRadius * cosf(angle2), z2 = capRadius * sinf(angle2);

        Vec3f top_center = { 0.0f, headR * 1.25f, 0.0f };
        Vec3f edge_v1 = { x1,   headR * 0.75f, z1 };
        Vec3f edge_v2 = { x2,   headR * 0.75f, z2 };

        Vec3f n = normalize(cross(sub(edge_v1, top_center), sub(edge_v2, top_center)));
        glNormal3f(n.x, n.y, n.z);

        glVertex3f(top_center.x, top_center.y, top_center.z);
        glVertex3f(edge_v1.x, edge_v1.y, edge_v1.z);
        glVertex3f(edge_v2.x, edge_v2.y, edge_v2.z);
    }
    glEnd();

    // --- Part 2: Main Bob Shape (Sides & Back) ---
    glBegin(GL_QUADS);
    for (int i = 0; i < segments; i++) {
        float angle1 = (float)i / segments * 2.0f * PI;
        float angle2 = (float)(i + 1) / segments * 2.0f * PI;

        // [KEEP] Leave space for bangs
        if (sinf(angle1) > 0.3f && fabsf(cosf(angle1)) < 0.8f) continue;

        float topRadius = headR * 1.15f;
        float topY = headR * 0.75f;
        float x1_top = topRadius * cosf(angle1), z1_top = topRadius * sinf(angle1);
        float x2_top = topRadius * cosf(angle2), z2_top = topRadius * sinf(angle2);

        float bottomRadius = headR * 0.95f;
        float bottomY = -headR * 0.5f;

        // [KEEP] short sides, long back
        if (fabsf(cosf(angle1)) > 0.5f) {
            bottomY = headR * 0.1f;
            bottomRadius = headR * 1.25f;
        }
        else if (sinf(angle1) < -0.3f) {
            bottomY = -headR * 1.0f;
            bottomRadius = headR * 1.25f;
        }

        float x1_bot = bottomRadius * cosf(angle1), z1_bot = bottomRadius * sinf(angle1);
        float x2_bot = bottomRadius * cosf(angle2), z2_bot = bottomRadius * sinf(angle2);

        Vec3f v_top1 = { x1_top, topY, z1_top };
        Vec3f v_top2 = { x2_top, topY, z2_top };
        Vec3f v_bot1 = { x1_bot, bottomY, z1_bot };
        Vec3f v_bot2 = { x2_bot, bottomY, z2_bot };

        Vec3f n = normalize(cross(sub(v_top2, v_top1), sub(v_bot1, v_top1)));
        glNormal3f(n.x, n.y, n.z);

        glVertex3f(v_top1.x, v_top1.y, v_top1.z);
        glVertex3f(v_top2.x, v_top2.y, v_top2.z);
        glVertex3f(v_bot2.x, v_bot2.y, v_bot2.z);
        glVertex3f(v_bot1.x, v_bot1.y, v_bot1.z);
    }
    glEnd();

    // --- Part 3: French Fringe (Bangs) ---
    glBegin(GL_QUADS);
    for (int i = 0; i < segments; i++) {
        float angle1 = (float)i / segments * 2.0f * PI;
        float angle2 = (float)(i + 1) / segments * 2.0f * PI;

        // [KEEP] Straight front bangs
        if (sinf(angle1) < 0.4f || fabsf(cosf(angle1)) > 0.6f) continue;

        float topRadius = headR * 1.15f, topY = headR * 0.8f;
        float x1_top = topRadius * cosf(angle1), z1_top = topRadius * sinf(angle1);
        float x2_top = topRadius * cosf(angle2), z2_top = topRadius * sinf(angle2);

        float bottomRadius = headR * 1.08f, bottomY = headR * 0.3f; // ~eyebrow
        float x1_bot = bottomRadius * cosf(angle1), z1_bot = bottomRadius * sinf(angle1);
        float x2_bot = bottomRadius * cosf(angle2), z2_bot = bottomRadius * sinf(angle2);

        Vec3f v_top1 = { x1_top, topY, z1_top };
        Vec3f v_top2 = { x2_top, topY, z2_top };
        Vec3f v_bot1 = { x1_bot, bottomY, z1_bot };
        Vec3f v_bot2 = { x2_bot, bottomY, z2_bot };

        Vec3f n = normalize(cross(sub(v_top2, v_top1), sub(v_bot1, v_top1)));
        glNormal3f(n.x, n.y, n.z);

        glVertex3f(v_top1.x, v_top1.y, v_top1.z);
        glVertex3f(v_top2.x, v_top2.y, v_top2.z);
        glVertex3f(v_bot2.x, v_bot2.y, v_bot2.z);
        glVertex3f(v_bot1.x, v_bot1.y, v_bot1.z);
    }
    glEnd();

    // [ADD] Cleanup only what we enabled here
    if (usingTexture) {
        Tex::disableObjectLinearST();
        Tex::unbind();
    }

    glPopMatrix();
}


void drawMulanFace() {
    drawMulanHair();
    drawMulanEyebrows();
    drawMulanEyes();
    drawMulanNose();
    drawMulanMouth();
}
// [MODIFY] Geometry kept; relies on object-linear texgen when textured.
//          No explicit glTexCoord* calls are needed; normals kept for lighting.

void drawCustomFaceShape() {
    // Define face parameters - much larger to match original sphere size
    const float faceWidth = HEAD_RADIUS * 1.1f;
    const float faceHeight = HEAD_RADIUS * 1.4f;
    const float faceDepth = HEAD_RADIUS * 0.9f;
    const int segments = HEAD_SEGMENTS; // Horizontal segments
    const int layers = HEAD_LAYERS;     // Vertical layers

    glBegin(GL_TRIANGLES);

    for (int layer = 0; layer < layers - 1; layer++) {
        float y1 = ((float)layer / (layers - 1)) * 2.0f - 1.0f; // -1 to 1
        float y2 = ((float)(layer + 1) / (layers - 1)) * 2.0f - 1.0f;

        // Convert to actual Y coordinates
        float actualY1 = y1 * faceHeight * 0.5f;
        float actualY2 = y2 * faceHeight * 0.5f;

        // Calculate face width scaling based on height for smooth curved jaw
        float widthScale1, widthScale2;

        if (y1 < JAW_TRANSITION) { // Lower jaw area - create smooth curved shape
            float jawFactor = (y1 + 1.0f) / (1.0f + JAW_TRANSITION); // 0 at bottom, 1 at jaw transition line
            // Use configurable curvature for smooth jaw shape
            float curveValue = powf(jawFactor, 1.0f / JAW_CURVATURE);
            widthScale1 = JAW_WIDTH_MIN + (1.0f - JAW_WIDTH_MIN) * curveValue;
        }
        else if (y1 < 0.4f) { // Mid face - full width
            widthScale1 = 1.0f;
        }
        else { // Upper face/forehead
            float foreheadFactor = (y1 - 0.4f) / 0.6f;
            widthScale1 = 1.0f - 0.1f * foreheadFactor; // Slightly narrower forehead
        }

        if (y2 < JAW_TRANSITION) { // Lower jaw area - create smooth curved shape
            float jawFactor = (y2 + 1.0f) / (1.0f + JAW_TRANSITION); // 0 at bottom, 1 at jaw transition line
            // Use configurable curvature for smooth jaw shape
            float curveValue = powf(jawFactor, 1.0f / JAW_CURVATURE);
            widthScale2 = JAW_WIDTH_MIN + (1.0f - JAW_WIDTH_MIN) * curveValue;
        }
        else if (y2 < 0.4f) { // Mid face - full width
            widthScale2 = 1.0f;
        }
        else { // Upper face/forehead
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
            Vec3f normal1 = normalize({ x1_1, actualY1 * 0.3f, z1_1 });
            Vec3f normal2 = normalize({ x1_2, actualY1 * 0.3f, z1_2 });
            Vec3f normal3 = normalize({ x2_1, actualY2 * 0.3f, z2_1 });
            Vec3f normal4 = normalize({ x2_2, actualY2 * 0.3f, z2_2 });

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

        float bottomWidthScale = JAW_WIDTH_MIN; // Use configurable jaw width at chin
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


void drawMulanHead()
{
    if (!g_headQuadric) {
        g_headQuadric = gluNewQuadric();
        gluQuadricNormals(g_headQuadric, GLU_SMOOTH);
        gluQuadricTexture(g_headQuadric, GL_TRUE);
    }

    glPushMatrix();

    if (gRenderMode == RM_TEXTURED) {
        glEnable(GL_TEXTURE_2D);
        Tex::bind(Tex::id[Tex::Skin]);                 // skin.bmp
        glColor3f(1.0f, 1.0f, 1.0f);                   // show the texture’s color
        // Use object-linear S/T/R so we don’t need to emit texcoords for the custom mesh
        Tex::enableObjectLinearST(0.35f, 0.35f, 0.35f);
    }
    else {
        glDisable(GL_TEXTURE_2D);
        glColor3f(0.92f, 0.76f, 0.65f);
    }

    drawCustomFaceShape();

    {
        const float faceWidth = HEAD_RADIUS * 1.1f;
        const float faceHeight = HEAD_RADIUS * 1.4f;
        const float faceDepth = HEAD_RADIUS * 0.9f;
        const int   segments = HEAD_SEGMENTS;

        const float bottomY = -faceHeight * 0.5f;
        const float jawScale = JAW_WIDTH_MIN; // narrow chin
        const float radX = faceWidth * jawScale;
        const float radZ = faceDepth * 0.6f;

        glBegin(GL_TRIANGLES);
        for (int i = 0; i < segments; ++i) {
            float a0 = (float)i / segments * 2.0f * PI;
            float a1 = (float)(i + 1) / segments * 2.0f * PI;

            Vec3f c{ 0.0f, bottomY, 0.0f };
            Vec3f v0{ radX * cosf(a0), bottomY, radZ * sinf(a0) };
            Vec3f v1{ radX * cosf(a1), bottomY, radZ * sinf(a1) };

            glNormal3f(0, -1, 0);
            // Winding: from below we want CW, so emit center -> v1 -> v0
            glVertex3f(c.x, c.y, c.z);
            glVertex3f(v1.x, v1.y, v1.z);
            glVertex3f(v0.x, v0.y, v0.z);
        }
        glEnd();
    }

    drawMulanFace();

    // [ADD] Clean up the texture/gen if we used them
    if (gRenderMode == RM_TEXTURED) {
        Tex::disableObjectLinearST();
        Tex::unbind();
    }

    glPopMatrix();
}

void drawTorso() {
    glShadeModel(GL_SMOOTH);

    // ===== Main torso (skirt/cloth) =====
    {
        setSolidColorIfNeeded(0.2f, 0.4f, 0.8f);            // SOLID / WIREFRAME color
        TextureScope ts(Tex::id[Tex::Skirt], /*useTexGen*/true, 0.5f, 0.5f, 1.0f);

        glBegin(GL_TRIANGLES);
        drawCurvedBand(R0, Y0, R1, Y1, segCos, segSin);
        drawCurvedBand(R1, Y1, R2, Y2, segCos, segSin);
        drawCurvedBand(R2, Y2, R3, Y3, segCos, segSin);
        drawCurvedBand(R3, Y3, R4, Y4, segCos, segSin);
        drawCurvedBand(R4, Y4, R5, Y5, segCos, segSin);
        drawCurvedBand(R5, Y5, R6, Y6, segCos, segSin);
        drawCurvedBand(R6, Y6, R7, Y7, segCos, segSin);
        drawCurvedBand(R7, Y7, R8, Y8, segCos, segSin);
        drawCurvedBand(R8, Y8, R9, Y9, segCos, segSin);
        drawCurvedBand(R9, Y9, R10, Y10, segCos, segSin);
        drawCurvedBand(R10, Y10, R11, Y11, segCos, segSin);
        drawCurvedBand(R11, Y11, R12, Y12, segCos, segSin);
        drawCurvedBand(R12, Y12, R13, Y13, segCos, segSin);
        drawCurvedBand(R13, Y13, R14, Y14, segCos, segSin);
        drawCurvedBand(R14, Y14, R15, Y15, segCos, segSin);
        drawCurvedBand(R15, Y15, R16, Y16, segCos, segSin);
        glEnd();
    }

    // Bottom cap (usually hidden) – keep simple, untextured is fine
    {
        setSolidColorIfNeeded(0.2f, 0.4f, 0.8f);
        glBegin(GL_TRIANGLES);
        const int N = TORSO_SEGMENTS;     // use your ring resolution (not hardcoded 16)
        for (int i = 0; i < N; ++i) {
            float cA = segCos[i], sA = segSin[i];
            float cB = segCos[(i + 1) % N], sB = segSin[(i + 1) % N];

            Vec3f center = { 0.0f, Y0, 0.0f };
            Vec3f v1 = { R0 * cA, Y0, R0 * sA };
            Vec3f v2 = { R0 * cB, Y0, R0 * sB };

            if (sA < 0) v1.z *= 0.5f; // your back curvature tweak
            if (sB < 0) v2.z *= 0.5f;

            glNormal3f(0, -1, 0);
            glVertex3f(center.x, center.y, center.z);
            glVertex3f(v2.x, v2.y, v2.z);
            glVertex3f(v1.x, v1.y, v1.z);
        }
        glEnd();
    }

    // ===== Shoulder + neck bands (skin) =====
    {
        setSolidColorIfNeeded(0.9f, 0.7f, 0.6f);           // SOLID / WIREFRAME skin color
        TextureScope ts(Tex::id[Tex::Skin], /*useTexGen*/true, 0.5f, 0.5f, 1.0f);

        glBegin(GL_TRIANGLES);
        drawCurvedBand(R16, Y16, R17, Y17, segCos, segSin);
        drawCurvedBand(R17, Y17, R18, Y18, segCos, segSin);
        drawCurvedBand(R18, Y18, R19, Y19, segCos, segSin);
        glEnd();
    }

    // Neck top cap
    {
        setSolidColorIfNeeded(0.9f, 0.7f, 0.6f);
        glBegin(GL_TRIANGLES);
        const int N = TORSO_SEGMENTS;
        for (int i = 0; i < N; ++i) {
            float cA = segCos[i], sA = segSin[i];
            float cB = segCos[(i + 1) % N], sB = segSin[(i + 1) % N];
            Vec3f center = { 0.0f, Y19, 0.0f };
            Vec3f v1 = { R19 * cA, Y19, R19 * sA };
            Vec3f v2 = { R19 * cB, Y19, R19 * sB };
            glNormal3f(0, 1, 0);
            glVertex3f(center.x, center.y, center.z);
            glVertex3f(v1.x, v1.y, v1.z);
            glVertex3f(v2.x, v2.y, v2.z);
        }
        glEnd();
    }

    glShadeModel(GL_FLAT); // back to your default
    drawInternalShoulderJoints();
}


// Draw internal ball-and-socket joints (hidden inside torso and arms)
// [MODIFY] Use skirt.bmp so the internal balls match torso material.
// [FIX] Right-side bug: use a new local quadric instead of reusing an out-of-scope pointer.
// [KEEP] Your clipping so only the inside hemisphere shows.

void drawInternalShoulderJoints() {
    const float shoulderOffsetTorso = R14 * 0.90f;
    const float jointHeight = Y14;
    const float ballRadius = 0.08f;

    const bool useTex = (gRenderMode == RM_TEXTURED) || g_TextureEnabled;
    if (useTex) {
        glEnable(GL_TEXTURE_2D);
        Tex::bind(Tex::id[Tex::Skirt]);   // skirt.bmp (same as torso)
        Tex::enableObjectLinearST(0.75f, 0.75f, 0.75f);
        glColor3f(1, 1, 1);
    }
    else {
        Tex::unbind();
        glDisable(GL_TEXTURE_2D);
        glColor3f(0.2f, 0.4f, 0.8f);      // fallback solid
    }

    // --- Left Shoulder Ball (inside torso) ---
    glPushMatrix();
    glTranslatef(-shoulderOffsetTorso, jointHeight, 0.1f);

    GLdouble clipPlaneLeft[] = { -1.0, 0.0, 0.0, -shoulderOffsetTorso + 0.1f };
    glClipPlane(GL_CLIP_PLANE0, clipPlaneLeft);
    glEnable(GL_CLIP_PLANE0);

    if (GLUquadric* q = gluNewQuadric()) {
        gluQuadricTexture(q, useTex ? GL_TRUE : GL_FALSE);
        gluSphere(q, ballRadius, 16, 16);
        gluDeleteQuadric(q);
    }
    glDisable(GL_CLIP_PLANE0);
    glPopMatrix();

    // --- Right Shoulder Ball (inside torso) ---
    glPushMatrix();
    glTranslatef(shoulderOffsetTorso, jointHeight, 0.1f);

    GLdouble clipPlaneRight[] = { 1.0, 0.0, 0.0, -shoulderOffsetTorso + 0.1f };
    glClipPlane(GL_CLIP_PLANE0, clipPlaneRight);
    glEnable(GL_CLIP_PLANE0);

    if (GLUquadric* q = gluNewQuadric()) {
        gluQuadricTexture(q, useTex ? GL_TRUE : GL_FALSE);
        gluSphere(q, ballRadius, 16, 16);
        gluDeleteQuadric(q);
    }
    glDisable(GL_CLIP_PLANE0);
    glPopMatrix();

    if (useTex) {
        Tex::disableObjectLinearST();
        Tex::unbind();
    }
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
// [MODIFY] Texture handling:
//   - When gConcreteHands is true, use silver.bmp (Tex::Silver) instead of untextured gray.
//   - When using textures, enable object-linear texgen around just the arm meshes
//     so sword/spear/shield mapping isn’t affected.
// [KEEP] Your pose logic and hierarchy.

void drawArmsAndHands(float leftArmAngle, float rightArmAngle) {
    // Use animated kung fu poses if animation is active
    float leftShoulderPitch, rightShoulderPitch;
    float leftShoulderYaw, rightShoulderYaw;
    float leftShoulderRoll, rightShoulderRoll;
    float leftElbowBend = 20.0f, rightElbowBend = 20.0f;
    float leftWristPitch = 0.0f, rightWristPitch = 0.0f;
    float leftWristYaw = 0.0f, rightWristYaw = 0.0f;
    float leftWristRoll = 0.0f, rightWristRoll = 0.0f;

    if (gKungFuAnimating) {
        // Use animated pose values
        leftShoulderPitch = gCurrentPose.leftShoulderPitch;
        rightShoulderPitch = gCurrentPose.rightShoulderPitch;
        leftShoulderYaw = gCurrentPose.leftShoulderYaw;
        rightShoulderYaw = gCurrentPose.rightShoulderYaw;
        leftShoulderRoll = gCurrentPose.leftShoulderRoll;
        rightShoulderRoll = gCurrentPose.rightShoulderRoll;
        leftArmAngle += gCurrentPose.leftArmAngle;
        rightArmAngle += gCurrentPose.rightArmAngle;
        leftElbowBend = gCurrentPose.leftElbowBend;
        rightElbowBend = gCurrentPose.rightElbowBend;
        leftWristPitch = gCurrentPose.leftWristPitch;
        rightWristPitch = gCurrentPose.rightWristPitch;

        // Apply realistic hand forms during animation while preserving wrist connection
        Vec3 leftWristBackup = g_HandJoints[0].position;
        Vec3 rightWristBackup = g_HandJoints2[0].position;

        setHandForm(g_HandJoints, gCurrentPose.leftHandForm);
        setHandForm(g_HandJoints2, gCurrentPose.rightHandForm);

        // Restore wrist positions to maintain arm connection
        g_HandJoints[0].position = leftWristBackup;
        g_HandJoints2[0].position = rightWristBackup;
    }
    else if (gSwordAttackAnimating) {
        // Use sword attack pose values - warrior-like sword attack stance
        // Create realistic warrior sword attack with proper arm bending patterns
        
        float swordArmPitch = 0.0f;
        float swordArmExtension = 0.0f;
        float swordElbowBend = 0.0f;
        float balanceArmPitch = 0.0f;
        
        // Determine which arm holds the sword and calculate realistic warrior movements
        if (gWeaponInRightHand) {
            // Right arm sword attack - realistic overhead to forward swing
            switch (gSwordAttackPhase) {
                case 1: // Wind up phase - raise arm high above head
                    swordArmPitch = 120.0f + gSwordAttackShoulderOffset; // High overhead position
                    swordElbowBend = 70.0f + abs(gSwordAttackShoulderOffset) * 0.8f; // Bent elbow for power
                    balanceArmPitch = 60.0f; // Left arm raised for balance
                    break;
                case 2: // Strike phase - powerful downward swing
                    swordArmPitch = 45.0f + gSwordAttackArmExtension; // Swinging down and forward
                    swordElbowBend = 25.0f + gSwordAttackArmExtension * 0.4f; // Extending for reach
                    balanceArmPitch = 90.0f; // Left arm extended for balance
                    break;
                case 3: // Follow through - arm extends fully forward
                    swordArmPitch = 30.0f + gSwordAttackArmExtension * 0.7f; // Forward completion
                    swordElbowBend = 10.0f + gSwordAttackArmExtension * 0.2f; // Nearly straight
                    balanceArmPitch = 45.0f; // Left arm settling
                    break;
                default: // Default/ready position
                    swordArmPitch = 90.0f;
                    swordElbowBend = 20.0f;
                    balanceArmPitch = 90.0f;
                    break;
            }
            
            // Apply to right arm (sword arm) and left arm (balance arm)
            rightShoulderPitch = swordArmPitch + rightArmAngle;
            rightShoulderYaw = 20.0f + gSwordAttackTorsoRotation * 0.8f;
            rightShoulderRoll = 40.0f + gSwordAttackArmExtension;
            rightElbowBend = swordElbowBend;
            
            leftShoulderPitch = balanceArmPitch + leftArmAngle;
            leftShoulderYaw = -20.0f - gSwordAttackTorsoRotation * 0.3f;
            leftShoulderRoll = -40.0f;
            leftElbowBend = 20.0f + gSwordAttackShoulderOffset * 0.3f;
            
        } else {
            // Left arm sword attack - mirror the movements
            switch (gSwordAttackPhase) {
                case 1: // Wind up phase - raise arm high above head
                    swordArmPitch = 120.0f + gSwordAttackShoulderOffset;
                    swordElbowBend = 70.0f + abs(gSwordAttackShoulderOffset) * 0.8f;
                    balanceArmPitch = 60.0f;
                    break;
                case 2: // Strike phase - powerful downward swing
                    swordArmPitch = 45.0f + gSwordAttackArmExtension;
                    swordElbowBend = 25.0f + gSwordAttackArmExtension * 0.4f;
                    balanceArmPitch = 90.0f;
                    break;
                case 3: // Follow through - arm extends fully forward
                    swordArmPitch = 30.0f + gSwordAttackArmExtension * 0.7f;
                    swordElbowBend = 10.0f + gSwordAttackArmExtension * 0.2f;
                    balanceArmPitch = 45.0f;
                    break;
                default:
                    swordArmPitch = 90.0f;
                    swordElbowBend = 20.0f;
                    balanceArmPitch = 90.0f;
                    break;
            }
            
            // Apply to left arm (sword arm) and right arm (balance arm)
            leftShoulderPitch = swordArmPitch + leftArmAngle;
            leftShoulderYaw = -20.0f - gSwordAttackTorsoRotation * 0.8f;
            leftShoulderRoll = -40.0f - gSwordAttackArmExtension;
            leftElbowBend = swordElbowBend;
            
            rightShoulderPitch = balanceArmPitch + rightArmAngle;
            rightShoulderYaw = 20.0f + gSwordAttackTorsoRotation * 0.3f;
            rightShoulderRoll = 40.0f;
            rightElbowBend = 20.0f + gSwordAttackShoulderOffset * 0.3f;
        }
        
        // Make sure sword-holding hand forms a proper grip
        if (gSwordVisible) {
            if (gWeaponInRightHand) {
                setHandForm(g_HandJoints2, HAND_GRIP_SWORD);
                setHandForm(g_HandJoints, HAND_OPEN); // Left hand open for balance
            } else {
                setHandForm(g_HandJoints, HAND_GRIP_SWORD);
                setHandForm(g_HandJoints2, HAND_OPEN); // Right hand open for balance
            }
        }
    }
    else if (gSpearAttackAnimating) {
        // Use spear attack pose values - spear thrust stance
        // Create realistic spear thrust with proper arm extension patterns
        
        float spearArmPitch = 0.0f;
        float spearArmExtension = 0.0f;
        float spearElbowBend = 0.0f;
        float balanceArmPitch = 0.0f;
        
        // Determine which arm holds the spear and calculate realistic thrust movements
        if (gWeaponInRightHand) {
            // Right arm spear attack - powerful forward thrust
            switch (gSpearAttackPhase) {
                case 1: // Wind up phase - pull spear back for thrust
                    spearArmPitch = 70.0f + gSpearAttackShoulderOffset * 0.5f;
                    spearElbowBend = 40.0f + abs(gSpearAttackShoulderOffset) * 0.6f;
                    balanceArmPitch = 45.0f;
                    break;
                case 2: // Thrust phase - explosive forward extension
                    spearArmPitch = 30.0f + gSpearAttackArmExtension * 0.4f;
                    spearElbowBend = 10.0f + gSpearAttackArmExtension * 0.2f;
                    balanceArmPitch = 80.0f;
                    break;
                case 3: // Follow through - controlled recovery
                    spearArmPitch = 50.0f + gSpearAttackArmExtension * 0.3f;
                    spearElbowBend = 25.0f + gSpearAttackArmExtension * 0.3f;
                    balanceArmPitch = 60.0f;
                    break;
            }
            
            leftShoulderPitch = 90.0f + balanceArmPitch + leftArmAngle;
            rightShoulderPitch = spearArmPitch + rightArmAngle;
            leftShoulderYaw = -20.0f;
            rightShoulderYaw = 20.0f + gSpearAttackShoulderOffset * 0.3f;
            leftShoulderRoll = -40.0f;
            rightShoulderRoll = 40.0f + gSpearAttackShoulderOffset * 0.5f;
            leftElbowBend = 20.0f;
            rightElbowBend = spearElbowBend;
        } else {
            // Left arm spear attack - mirror the movements
            switch (gSpearAttackPhase) {
                case 1: // Wind up phase - pull spear back for thrust
                    spearArmPitch = 70.0f + gSpearAttackShoulderOffset * 0.5f;
                    spearElbowBend = 40.0f + abs(gSpearAttackShoulderOffset) * 0.6f;
                    balanceArmPitch = 45.0f;
                    break;
                case 2: // Thrust phase - explosive forward extension
                    spearArmPitch = 30.0f + gSpearAttackArmExtension * 0.4f;
                    spearElbowBend = 10.0f + gSpearAttackArmExtension * 0.2f;
                    balanceArmPitch = 80.0f;
                    break;
                case 3: // Follow through - controlled recovery
                    spearArmPitch = 50.0f + gSpearAttackArmExtension * 0.3f;
                    spearElbowBend = 25.0f + gSpearAttackArmExtension * 0.3f;
                    balanceArmPitch = 60.0f;
                    break;
            }
            
            leftShoulderPitch = spearArmPitch + leftArmAngle;
            rightShoulderPitch = 90.0f + balanceArmPitch + rightArmAngle;
            leftShoulderYaw = -20.0f + gSpearAttackShoulderOffset * 0.3f;
            rightShoulderYaw = 20.0f;
            leftShoulderRoll = -40.0f + gSpearAttackShoulderOffset * 0.5f;
            rightShoulderRoll = 40.0f;
            leftElbowBend = spearElbowBend;
            rightElbowBend = 20.0f;
        }
        
        // Make sure spear-holding hand forms a proper grip
        if (gSpearVisible) {
            if (gWeaponInRightHand) {
                setHandForm(g_HandJoints2, HAND_GRIP_SPEAR);
                setHandForm(g_HandJoints, HAND_OPEN); // Left hand open for balance
            } else {
                setHandForm(g_HandJoints, HAND_GRIP_SPEAR);
                setHandForm(g_HandJoints2, HAND_OPEN); // Right hand open for balance
            }
        }
    }
    else if (gShieldBlockAnimating) {
        // Use shield block pose values - tactical defensive stance
        // Create realistic shield blocking with arm bent up to protect body
        
        float shieldArmPitch = 0.0f;
        float shieldElbowBend = 0.0f;
        float balanceArmPitch = 0.0f;
        
        // Shield is held in left arm with bent elbow for body protection
        switch (gShieldBlockPhase) {
            case 1: // Raise phase - quickly raise shield with bent arm
                shieldArmPitch = 45.0f + gShieldBlockArmRaise * 0.5f; // Less shoulder raise, more elbow bend
                shieldElbowBend = 120.0f + gShieldBlockArmRaise * 0.8f; // High elbow bend for protection
                balanceArmPitch = 30.0f + gShieldBlockArmRaise * 0.2f;
                break;
            case 2: // Hold phase - maintain defensive position with shield protecting torso
                shieldArmPitch = 45.0f + gShieldBlockArmRaise * 0.5f;
                shieldElbowBend = 120.0f + gShieldBlockArmRaise * 0.8f; // Keep high elbow bend
                balanceArmPitch = 30.0f + gShieldBlockArmRaise * 0.2f;
                break;
            case 3: // Lower phase - controlled return
                shieldArmPitch = 45.0f + gShieldBlockArmRaise * 0.5f;
                shieldElbowBend = 120.0f + gShieldBlockArmRaise * 0.8f;
                balanceArmPitch = 30.0f + gShieldBlockArmRaise * 0.2f;
                break;
        }
        
        // Shield held in left arm (tactical defensive position with minimal upper arm movement)
        leftShoulderPitch = 30.0f + leftArmAngle; // Reduced shoulder movement - keep upper arm stable
        rightShoulderPitch = 90.0f + balanceArmPitch + rightArmAngle;
        leftShoulderYaw = 30.0f; // Positive value to rotate toward right side
        rightShoulderYaw = 10.0f; // Ready position
        leftShoulderRoll = -75.0f; // Reduced roll - keep upper arm more stable
        rightShoulderRoll = 25.0f;
        leftElbowBend = shieldElbowBend; // High elbow bend for tactical position
        rightElbowBend = 10.0f;
        
        // Apply enhanced lower arm bending for better body defense positioning
        // Bend the left lower arm (forearm) up significantly more while keeping upper arm stable
        if (gShieldBlockPhase >= 1) {
            gLeftLowerArmBend = -80.0f * (gShieldBlockArmRaise / 80.0f); // Increased to -80° for more pronounced forearm bend
        }
        
        // Shield blocking hand form
        if (gShieldVisible) {
            setHandForm(g_HandJoints, HAND_FIST); // Left hand makes fist when holding shield
            setHandForm(g_HandJoints2, HAND_OPEN); // Right hand ready for action
        }
    }
    else if (gBoxingAnimActive || gInBoxingStance) {
        // Use boxing stance pose values
        leftShoulderPitch = gCurrentLeftShoulderPitch;
        rightShoulderPitch = gCurrentRightShoulderPitch;
        leftShoulderYaw = gCurrentLeftShoulderYaw;
        rightShoulderYaw = gCurrentRightShoulderYaw;
        leftShoulderRoll = gCurrentLeftShoulderRoll;   // Use the 180-degree rotation
        rightShoulderRoll = gCurrentRightShoulderRoll; // Use the 180-degree rotation

        // Boxing stance doesn't affect leg animation angles
        // leftArmAngle and rightArmAngle remain as they are

        // Note: elbow bending is now handled in drawLowPolyArm via gCurrentLeftElbowBend/gCurrentRightElbowBend
        leftElbowBend = 0.0f; // Not used anymore, handled in arm drawing
        rightElbowBend = 0.0f; // Not used anymore, handled in arm drawing

        // Make fists in boxing stance for realistic look
        if (g_FistHandJoints.size() == g_HandJoints.size()) {
            g_HandJoints = g_FistHandJoints;
        }
        if (g_FistHandJoints2.size() == g_HandJoints2.size()) {
            g_HandJoints2 = g_FistHandJoints2;
        }
    }
    else {
        // Use default poses and restore original hand positions
        leftShoulderPitch = 90.0f + leftArmAngle;
        rightShoulderPitch = 90.0f + rightArmAngle;
        leftShoulderYaw = -20.0f;
        rightShoulderYaw = 20.0f;
        leftShoulderRoll = -40.0f;
        rightShoulderRoll = 40.0f;

        // Lift arm higher when holding sword
        if ((gSwordVisible || gSpearVisible) && !gWeaponInRightHand) {
            leftShoulderPitch += 30.0f;
        }
        if ((gSwordVisible || gSpearVisible) && gWeaponInRightHand) {
            rightShoulderPitch += 30.0f;
        }

        // Restore original hand joint positions when not animating (but preserve fist animation)
        // Also check if we need to make a fist for holding the sword
        bool needLeftSwordGrip = gSwordVisible && !gWeaponInRightHand;
        bool needRightSwordGrip = gSwordVisible && gWeaponInRightHand;
        bool needLeftSpearGrip = gSpearVisible && !gWeaponInRightHand;
        bool needRightSpearGrip = gSpearVisible && gWeaponInRightHand;
        bool needLeftShieldGrip = gShieldVisible; // Left hand holds shield

        if (!gFistAnimationActive && !gIsFist && !needLeftSwordGrip && !needRightSwordGrip && !needLeftSpearGrip && !needRightSpearGrip && !needLeftShieldGrip) {
            if (g_OriginalHandJoints.size() == g_HandJoints.size()) g_HandJoints = g_OriginalHandJoints;
            if (g_OriginalHandJoints2.size() == g_HandJoints2.size()) g_HandJoints2 = g_OriginalHandJoints2;
        }

        if (needLeftSwordGrip) setHandForm(g_HandJoints, HAND_GRIP_SWORD);
        if (needRightSwordGrip) setHandForm(g_HandJoints2, HAND_GRIP_SWORD);
        if (needLeftSpearGrip) setHandForm(g_HandJoints, HAND_GRIP_SPEAR);
        if (needRightSpearGrip) setHandForm(g_HandJoints2, HAND_GRIP_SPEAR);
        if (needLeftShieldGrip) setHandForm(g_HandJoints, HAND_FIST); // Shield requires fist grip
    }

    // Set material properties for concrete or skin
    if (gRenderMode == RM_TEXTURED) {
        glEnable(GL_TEXTURE_2D);
        Tex::bind(gConcreteHands ? Tex::id[Tex::Silver] : Tex::id[Tex::Skin]);
        glColor3f(1.0f, 1.0f, 1.0f); // Use white to show full texture color
    }
    else {
        glDisable(GL_TEXTURE_2D);
        if (gConcreteHands) {
            // Concrete/Silver mode
            if (gRenderMode == RM_TEXTURED) Tex::bind(Tex::id[Tex::Silver]);
            if (gRenderMode != RM_TEXTURED) glColor3f(0.7f, 0.7f, 0.75f);
        }
        else {
            // Normal Skin mode
            if (gRenderMode == RM_TEXTURED) Tex::bind(Tex::id[Tex::Skin]);
            if (gRenderMode != RM_TEXTURED) glColor3f(0.85f, 0.64f, 0.52f);  // <<< Correct skin color
        }
    }

    // --- Draw Left Arm (connected to shoulder peak R15) ---
    glPushMatrix();
    {
        float shoulderOffset = R14 * 0.98f; // Closer to shoulder for better connection
        glTranslatef(-shoulderOffset, Y14, 0.05f); // Reduced Z offset for tighter connection
        glRotatef(leftShoulderPitch, 1.0f, 0.0f, 0.0f);
        glRotatef(leftShoulderYaw, 0.0f, 1.0f, 0.0f);
        glRotatef(leftShoulderRoll, 0.0f, 0.0f, 1.0f);

        // Draw internal shoulder socket (connected to shoulder band R15)
        glPushMatrix();
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

        if (gShieldVisible) {
            glPushMatrix();
            // Position relative to the elbow to attach to forearm
            Vec3 elbowPos = g_ArmJoints[3].position;
            glTranslatef(elbowPos.x * ARM_SCALE, elbowPos.y * ARM_SCALE, elbowPos.z * ARM_SCALE);
            
            // Adjust shield position based on animation state
            if (gShieldBlockAnimating && gShieldBlockPhase >= 1) {
                // During shield defense animation - completely cover hand
                glTranslatef(0.1f, -0.5f, 1.0f); // Move shield forward and down to completely cover hand
                glRotatef(-60.0f, 0, 1, 0); // Reduced Y rotation for frontal coverage
                glRotatef(45.0f, 1, 0, 0); // Strong upward tilt to block view of hand
                glRotatef(-25.0f, 0, 0, 1); // More roll to wrap around hand area
                glScalef(0.5f, 0.5f, 0.5f); // Slightly larger shield for better coverage
            } else {
                // Normal shield position when just visible
                glTranslatef(-0.1f, -0.2f, 0.5f); // Standard position
                glRotatef(-80.0f, 0, 1, 0);
                glRotatef(25.0f, 1, 0, 0);
                glRotatef(-15.0f, 0, 0, 1);
                glScalef(0.4f, 0.4f, 0.4f);
            }
            
            drawShield();
            glPopMatrix();
        }

        glPushMatrix();
        if (gConcreteHands) {
            if (gRenderMode == RM_TEXTURED) {
                Tex::bind(Tex::id[Tex::Armor]); // <<< USE ARMOR TEXTURE
                glColor3f(1.0f, 1.0f, 1.0f);
            }
            if (gRenderMode != RM_TEXTURED) glColor3f(0.75f, 0.75f, 0.8f); // Armor color
            glScalef(ARM_SCALE * 1.15f, ARM_SCALE * 1.15f, ARM_SCALE * 1.15f);
            drawLowPolyArm(g_ArmJoints);
        }
        else {
            if (gRenderMode == RM_TEXTURED) {
                Tex::bind(Tex::id[Tex::Skin]);
                glColor3f(1.0f, 1.0f, 1.0f);
            }
            if (gRenderMode != RM_TEXTURED) glColor3f(0.9f, 0.7f, 0.6f);
            glScalef(ARM_SCALE, ARM_SCALE, ARM_SCALE);
            drawLowPolyArm(g_ArmJoints);
        }
        glPopMatrix();

        if (gRenderMode == RM_TEXTURED) Tex::unbind();

        // Draw left arm armor if armor is visible
        if (gArmorVisible) {
            glPushMatrix();
            // Upper arm armor only
            drawUpperArmArmor();
            glPopMatrix();
        }

        Vec3 elbowPos = g_ArmJoints[3].position;
        glTranslatef(elbowPos.x * ARM_SCALE, elbowPos.y * ARM_SCALE, elbowPos.z * ARM_SCALE);
        glRotatef(gLeftLowerArmBend, 1.0f, 0.0f, 0.0f);  // Use new lower arm bend system
        glTranslatef(-elbowPos.x * ARM_SCALE, -elbowPos.y * ARM_SCALE, -elbowPos.z * ARM_SCALE);

        // Get transformed wrist position that accounts for lower arm bending
        Vec3 leftWristPos = getTransformedWristPosition(g_ArmJoints, gCurrentLeftElbowBend + (-gLeftLowerArmBend));
        glTranslatef(leftWristPos.x * ARM_SCALE, leftWristPos.y * ARM_SCALE, leftWristPos.z * ARM_SCALE);
        glRotatef(leftWristPitch, 1.0f, 0.0f, 0.0f);  // Use animated wrist pitch
        glRotatef(leftWristYaw, 0.0f, 1.0f, 0.0f);    // Use animated wrist yaw
        glRotatef(leftWristRoll, 0.0f, 0.0f, 1.0f);   // Use animated wrist roll

        // Add sword grip rotation to the wrist joint so arm stays connected
        if (gSwordVisible && !gWeaponInRightHand) {
            glRotatef(-45.0f, 0.0f, 1.0f, 0.0f); // Rotate wrist 45 degrees for natural sword grip
        }

        // NOTE: Hand is now drawn integrated within drawLowPolyArm() function
        // No separate hand drawing needed here - it's part of the arm hierarchy

        // Draw sword in left hand if enabled
        if (gSwordVisible && !gWeaponInRightHand) {
            glPushMatrix();
            // Position sword in front of hand for proper grip
            glTranslatef(0.03f, 0.05f, 0.3f); // Move sword forward in front of hand (moved left from mirrored position)
            glRotatef(90.0f, 1.0f, 0.0f, 0.0f); // Orient sword downward (same as right hand)
            glRotatef(-15.0f, 0.0f, 0.0f, 1.0f); // Slight tilt for natural grip (opposite direction)
            // Move sword up so hand grips hilt (hilt is about 1.0 units below guard in sword model)
            glTranslatef(0.0f, 1.0f, 0.0f); // Move sword up so hilt is in hand
            drawSword();
            glPopMatrix();
        }
    }
    glPopMatrix();

    // --- Draw Right Arm (connected to shoulder peak R15) ---
    glPushMatrix();
    {
        float shoulderOffset = R14 * 0.98f; // Closer to shoulder for better connection
        glTranslatef(shoulderOffset, Y14, 0.05f); // Reduced Z offset for tighter connection
        glRotatef(rightShoulderPitch, 1.0f, 0.0f, 0.0f);
        glRotatef(rightShoulderYaw, 0.0f, 1.0f, 0.0f);
        glRotatef(rightShoulderRoll, 0.0f, 0.0f, 1.0f);
        glColor3f(0.85f, 0.64f, 0.52f); // Match arm color
        // Draw internal shoulder socket (connected to shoulder band R15)
        glPushMatrix();
        
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
        if (gConcreteHands) {
            if (gRenderMode == RM_TEXTURED) {
                Tex::bind(Tex::id[Tex::Armor]); // <<< USE ARMOR TEXTURE
                glColor3f(1.0f, 1.0f, 1.0f);
            }
            if (gRenderMode != RM_TEXTURED) glColor3f(0.75f, 0.75f, 0.8f); // Armor color
            glScalef(ARM_SCALE * 1.15f, ARM_SCALE * 1.15f, ARM_SCALE * 1.15f);
            drawLowPolyArm(g_ArmJoints2);
        }
        else {
            if (gRenderMode == RM_TEXTURED) {
                Tex::bind(Tex::id[Tex::Skin]);
                glColor3f(1.0f, 1.0f, 1.0f);
            }
            if (gRenderMode != RM_TEXTURED) glColor3f(0.9f, 0.7f, 0.6f);
            glScalef(ARM_SCALE, ARM_SCALE, ARM_SCALE);
            drawLowPolyArm(g_ArmJoints2);
        }
        glPopMatrix();

        if (gRenderMode == RM_TEXTURED) Tex::unbind();

        // Draw right arm armor if armor is visible
        if (gArmorVisible) {
            glPushMatrix();
            // Mirror the armor for right arm
            glScalef(-1.0f, 1.0f, 1.0f);
            glFrontFace(GL_CW);
            // Upper arm armor only
            drawUpperArmArmor();
            glFrontFace(GL_CCW);
            glPopMatrix();
        }

        Vec3 rightElbowPos = g_ArmJoints2[3].position;
        glTranslatef(rightElbowPos.x * ARM_SCALE, rightElbowPos.y * ARM_SCALE, rightElbowPos.z * ARM_SCALE);
        glRotatef(gRightLowerArmBend, 1.0f, 0.0f, 0.0f);  // Use new lower arm bend system
        glTranslatef(-rightElbowPos.x * ARM_SCALE, -rightElbowPos.y * ARM_SCALE, -rightElbowPos.z * ARM_SCALE);

        // Get transformed wrist position that accounts for lower arm bending
        Vec3 rightWristPos = getTransformedWristPosition(g_ArmJoints2, gCurrentRightElbowBend + (-gRightLowerArmBend));
        glTranslatef(rightWristPos.x * ARM_SCALE, rightWristPos.y * ARM_SCALE, rightWristPos.z * ARM_SCALE);
        glRotatef(rightWristPitch, 1.0f, 0.0f, 0.0f);  // Use animated wrist pitch
        glRotatef(rightWristYaw, 0.0f, 1.0f, 0.0f);    // Use animated wrist yaw  
        glRotatef(rightWristRoll, 0.0f, 0.0f, 1.0f);   // Use animated wrist roll

        // Add sword grip rotation to the wrist joint so arm stays connected
        if (gSpearVisible && gWeaponInRightHand) {
            glPushMatrix();
            glTranslatef(0.0f, 0.05f, 0.1f); // Adjust grip position
            glRotatef(80.0f, 1.0f, 0.0f, 0.0f); // Angle
            glRotatef(-25.0f, 0.0f, 0.0f, 1.0f); // Tilt
            glScalef(0.3f, 0.3f, 0.3f);
            drawSpear();
            glPopMatrix();
        }
        if (gSwordVisible && gWeaponInRightHand) {
            glRotatef(45.0f, 0.0f, 1.0f, 0.0f); // Rotate wrist 45 degrees for natural sword grip
        }

        // NOTE: Hand is now drawn integrated within drawLowPolyArm() function
        // No separate hand drawing needed here - it's part of the arm hierarchy

        // Draw sword in right hand if enabled
        if (gSwordVisible && gWeaponInRightHand) {
            glPushMatrix();
            // Position sword in front of hand for proper grip
            glTranslatef(-0.18f, 0.05f, 0.3f); // Move sword forward in front of hand (moved further left)
            glRotatef(90.0f, 1.0f, 0.0f, 0.0f); // Orient sword downward
            glRotatef(15.0f, 0.0f, 0.0f, 1.0f); // Slight tilt for natural grip
            // Move sword up so hand grips hilt (hilt is about 1.0 units below guard in sword model)
            glTranslatef(0.0f, 1.0f, 0.0f); // Move sword up so hilt is in hand
            drawSword();
            glPopMatrix();
        }
    }
    glPopMatrix();

    // Disable texture after drawing hands
    glDisable(GL_TEXTURE_2D);
    if (gRenderMode == RM_TEXTURED) {
        Tex::unbind(); // Clean up texture state
    }
}

// [MODIFY] Make neck local to the head pivot and texture with skin.bmp using object-linear texgen.
// [REMOVE] World-space translate based on HEAD_CENTER_Y; caller now positions us (see drawBodyAndHead).
// [ADD] Slight overlap into head to hide seam.

void drawTexturedNeck() {
    if (!g_headQuadric) {
        g_headQuadric = gluNewQuadric();
        gluQuadricNormals(g_headQuadric, GLU_SMOOTH);
        gluQuadricTexture(g_headQuadric, GL_TRUE);
    }

    // Texture on/off
    bool usingTexture = (gRenderMode == RM_TEXTURED) || g_TextureEnabled;
    if (usingTexture) {
        glEnable(GL_TEXTURE_2D);
        Tex::bind(Tex::id[Tex::Skin]);           // skin.bmp
        Tex::enableObjectLinearST(0.5f, 0.5f, 0.5f);
        glColor3f(1, 1, 1);
    }
    else {
        Tex::unbind();
        glDisable(GL_TEXTURE_2D);
        glColor3f(0.92f, 0.76f, 0.65f);
    }

    // [MODIFY] Neck is drawn just below the head’s local origin.
    // Slightly intrude into the head to avoid any gap.
    const float neckTopY = -HEAD_RADIUS * 0.68f; // just inside the head shell
    const float neckHeight = 0.42f;                // extends down toward torso
    const float neckTopR = 0.20f;
    const float neckBottomR = 0.22f;

    glPushMatrix();
    glTranslatef(0.0f, neckTopY, 0.0f);
    glRotatef(-90.0f, 1, 0, 0);         // GLU cylinders go along +Z → rotate so length goes along +Y
    gluCylinder(g_headQuadric, neckTopR, neckBottomR, neckHeight, 24, 1);
    glPopMatrix();

    if (usingTexture) {
        Tex::disableObjectLinearST();
        Tex::unbind();
    }
}


// --- Main Character Drawing ---
// [MODIFY] Rewire head + neck so they’re positioned and textured consistently.
//          Torso uses its own texture inside drawTorso(); head/neck use skin here.
//          Nothing else in your animation flow changes.

void drawBodyAndHead(float leftLegAngle, float rightLegAngle, float leftArmAngle, float rightArmAngle)
{
    glPushMatrix();

    // [KEEP] Body placement to meet the leg tops (your comment about 6.0f is fine)
    glTranslatef(0.0f, 6.0f - 0.05f, 0.0f);
    glScalef(BODY_SCALE, BODY_SCALE, BODY_SCALE);

    // Enhanced: Apply shoulder sway for natural walking
    float shoulderSway = 0.0f;
    if (fabsf(gMoveSpeed) > 0.1f) {
        float phase = gWalkPhase;
        shoulderSway = -sinf(phase * 0.5f + STEP_TIMING_OFFSET) * SHOULDER_COUNTER_SWAY;
    }
    glRotatef(shoulderSway, 0, 0, 1); // Apply shoulder sway rotation

    // [KEEP] Torso orientation (animated)
    glRotatef(g_pose.torsoYaw, 0, 1, 0);
    glRotatef(g_pose.torsoPitch, 1, 0, 0);
    glRotatef(g_pose.torsoRoll, 0, 0, 1);
    
    // Apply sword attack animation torso rotation
    if (gSwordAttackAnimating) {
        glRotatef(gSwordAttackTorsoRotation, 0, 1, 0); // Rotate around Y-axis for sword swing
    }
    // Apply spear attack animation torso rotation  
    if (gSpearAttackAnimating) {
        glRotatef(gSpearAttackTorsoRotation, 0, 1, 0); // Rotate around Y-axis for spear thrust
    }
    // Apply shield block animation torso lean
    if (gShieldBlockAnimating) {
        glRotatef(gShieldBlockTorsoLean, 1, 0, 0); // Lean back slightly for shield defense
    }

    // [KEEP] Arms
    drawArmsAndHands(leftArmAngle, rightArmAngle);

    // [KEEP] Torso (note: your drawTorso() should bind Tex::Skirt internally)
    drawTorso();

    // ------------------------------
    // [ADD] Head/Neck block
    // ------------------------------
    glPushMatrix();
    {
        // Enhanced: Apply head bobbing for natural walking
        float headBob = 0.0f;
        if (fabsf(gMoveSpeed) > 0.1f) {
            float bobPhase = gWalkPhase * 2.0f;
            headBob = sinf(bobPhase) * HEAD_BOB_AMOUNT;
        }
        
        // Move to the neck/head pivot (center of head). This matches your HEAD_CENTER_Y.
        glTranslatef(0.0f, HEAD_CENTER_Y + headBob, 0.0f);

        // Apply head orientation (relative to torso)
        glRotatef(g_pose.headYaw, 0, 1, 0);
        glRotatef(g_pose.headPitch, 1, 0, 0);
        glRotatef(g_pose.headRoll, 0, 0, 1);

        // Draw neck first so head sits on top seamlessly.
        // IMPORTANT: drawTexturedNeck() should use SKIN texture inside itself.
        // If your neck code expects torso-space Y values (≈1.6–2.0), you can
        // leave it as-is; it's running under the same body matrix here.
        drawTexturedNeck();     // [MOVE HERE] (was outside the body matrix before)

        // Now draw the head (it’s modeled around local origin, centered vertically).
        drawMulanHead();        // [MOVE HERE] (was called after popping matrices)
    }
    glPopMatrix();

    glPopMatrix();
}


// ===================================================================
//
// SECTION: PROJECTION & VIEWPORT FUNCTIONS
//
// ===================================================================

void setupProjection() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    if (gViewportMode) {
        // Split viewport mode - show both orthographic and perspective
        int halfWidth = gWidth / 2;

        // Left half - Orthographic
        glViewport(0, 0, halfWidth, gHeight);
        float orthoSize = gDist * 0.5f; // Scale orthographic view based on distance
        float aspect = (float)halfWidth / gHeight;
        glOrtho(-orthoSize * aspect, orthoSize * aspect, -orthoSize, orthoSize, 0.1f, 100.0f);
    }
    else {
        // Full viewport mode
        glViewport(0, 0, gWidth, gHeight);

        if (gProjMode == PROJ_ORTHOGRAPHIC) {
            // Orthographic projection
            float orthoSize = gDist * 0.5f; // Scale based on camera distance
            float aspect = (float)gWidth / gHeight;
            glOrtho(-orthoSize * aspect, orthoSize * aspect, -orthoSize, orthoSize, 0.1f, 100.0f);
        }
        else {
            // Perspective projection
            gluPerspective(45.0, (double)gWidth / gHeight, 0.1, 100.0);
        }
    }
}

void setupSplitViewportPerspective() {
    // Right half - Perspective (for split viewport mode)
    int halfWidth = gWidth / 2;
    glViewport(halfWidth, 0, halfWidth, gHeight);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)halfWidth / gHeight, 0.1, 100.0);
}

void renderScene() {
    // Render background first (if enabled)
    if (gBackgroundVisible) {
        glPushMatrix();
        glTranslatef(0.0f, -2.5f, 0.0f); // Lower the entire background by 2.5 units
        BackgroundRenderer::render();
        glPopMatrix();
    }

    // Common scene rendering logic for both orthographic and perspective modes

    setRenderMode(gRenderMode);

    glEnable(GL_LIGHTING); glEnable(GL_LIGHT0); glEnable(GL_COLOR_MATERIAL);
    float lightPos[] = { 20.f,20.f,30.f,1.f };
    float ambientLight[] = { 0.4f,0.4f,0.4f,1.f };
    float diffuseLight[] = { 0.7f,0.7f,0.7f,1.f };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);

    float bodyBob = 0.0f;
    float leftArmSwing = 0.0f, rightArmSwing = 0.0f;
    float hipSway = 0.0f;
    float shoulderSway = 0.0f;
    float headBob = 0.0f;
    float torsoRotation = 0.0f;

    if (fabsf(gMoveSpeed) > 0.1f) {
        float animSpeed = keyShift ? 1.8f : 1.0f;
        float phase = gWalkPhase * animSpeed;
        float sinPhase = sinf(phase);
        float cosPhase = cosf(phase);
        
        // Enhanced body bobbing with more natural curve
        float bobPhase = phase * 2.0f;
        bodyBob = (cosf(bobPhase) * -0.5f + 0.5f) * -BODY_BOB_AMOUNT;
        
        // Natural arm swing with slight delay and easing
        float armPhase = phase + STEP_TIMING_OFFSET;
        float armSwingAmplitude = keyShift ? RUN_ARM_SWING : WALK_ARM_SWING;
        rightArmSwing = sinf(armPhase) * armSwingAmplitude * 0.8f;
        leftArmSwing = -sinf(armPhase) * armSwingAmplitude * 0.8f;
        
        // Hip sway (side-to-side movement)
        hipSway = sinf(phase * 0.5f) * HIP_SWAY_AMOUNT;
        
        // Shoulder counter-sway (opposite to hips for natural walking)
        shoulderSway = -sinf(phase * 0.5f + STEP_TIMING_OFFSET) * SHOULDER_COUNTER_SWAY;
        
        // Subtle head bobbing
        headBob = sinf(bobPhase) * HEAD_BOB_AMOUNT;
        
        // Subtle torso rotation (realistic walking twist)
        torsoRotation = sinf(phase * 0.7f) * 2.0f;
    }

    glPushMatrix();
    // Enhanced: Apply hip sway and body rotation for natural walking
    glTranslatef(gCharacterPos.x + hipSway, gCharacterPos.y + bodyBob + gJumpVerticalOffset, gCharacterPos.z);
    glRotatef(gCharacterYaw + torsoRotation, 0.0f, 1.0f, 0.0f);

    float current_speed = gRunningAnim ? gRunSpeed : gWalkSpeed;
    float max_hip_angle = gRunningAnim ? 28.0f : 18.0f;    // Enhanced: More natural range
    float max_knee_angle = gRunningAnim ? 65.0f : 42.0f;   // Enhanced: Better knee bend

    // Enhanced leg animation with more natural timing and easing
    float left_leg_phase = gAnimTime * current_speed;
    float right_leg_phase = gAnimTime * current_speed + PI;
    
    // Natural hip movement with easing
    float leftHipSin = sinf(left_leg_phase);
    float rightHipSin = sinf(right_leg_phase);
    float hip_angle_L = gIsMoving ? (max_hip_angle * leftHipSin * 0.9f) : 0.0f;
    float hip_angle_R = gIsMoving ? (max_hip_angle * rightHipSin * 0.9f) : 0.0f;
    
    // Natural knee movement with better follow-through
    float leftKneePhase = left_leg_phase + 0.3f; // Slight delay for natural movement
    float rightKneePhase = right_leg_phase + 0.3f;
    float knee_angle_L = gIsMoving ? (max_knee_angle * std::max(0.0f, sinf(leftKneePhase) * 0.8f)) : 0.0f;
    float knee_angle_R = gIsMoving ? (max_knee_angle * std::max(0.0f, sinf(rightKneePhase) * 0.8f)) : 0.0f;
    
    // Apply sword attack stance modifications
    if (gSwordAttackAnimating) {
        // Warrior stance: wider leg position, weight shifting
        hip_angle_L += gSwordAttackLegStance * 0.5f; // Slight back leg position
        hip_angle_R -= gSwordAttackLegStance * 0.3f; // Front leg forward
        knee_angle_L += abs(gSwordAttackLegStance) * 0.2f; // Slight knee bend for stability
        knee_angle_R += abs(gSwordAttackLegStance) * 0.3f; // More bend in front leg
    }
    
    // Apply spear attack stance modifications
    if (gSpearAttackAnimating) {
        // Spear thrust stance: forward lunge position
        hip_angle_L += gSpearAttackLegStance * 0.4f; // Back leg for stability
        hip_angle_R -= gSpearAttackLegStance * 0.6f; // Front leg extends for thrust
        knee_angle_L += abs(gSpearAttackLegStance) * 0.3f; // Bend for power
        knee_angle_R += abs(gSpearAttackLegStance) * 0.4f; // Forward leg bent for lunge
    }
    
    // Apply shield block stance modifications
    if (gShieldBlockAnimating) {
        // Shield tactical defensive stance: stable, braced position
        hip_angle_L += gShieldBlockStance * 0.2f; // Slightly back for balance
        hip_angle_R += gShieldBlockStance * 0.3f; // Forward for aggressive defense
        knee_angle_L += abs(gShieldBlockStance) * 0.3f; // Bent for stability
        knee_angle_R += abs(gShieldBlockStance) * 0.5f; // More bend in forward leg
        
        // Apply torso lean for shield defense to show hand better
        if (gShieldBlockPhase >= 1) {
            // Lean torso slightly away from shield to reveal hand
            glRotatef(gShieldBlockTorsoLean * 0.5f, 0, 0, 1); // Slight side lean
        }
    }

    const float kneeY = 4.5f, kneeZ = 0.2f;
    const float hipY = 8.5f, hipZ = -0.2f;

    // --- Draw Left Leg & Armor ---
    glPushMatrix();
    glTranslatef(-0.75f, -2.0f, 0.55f);
    animateLegVertices(hip_angle_L, knee_angle_L, false);
    //glColor3f(0.9f, 0.7f, 0.6f); // Set skin color
    drawLeg();
    if (gArmorVisible) {
        glPushMatrix();
        glTranslatef(0, hipY, hipZ);
        glRotatef(hip_angle_L, 1.0f, 0.0f, 0.0f);
        glTranslatef(0, -hipY, -hipZ);
        drawUpperLegArmor();
        glTranslatef(0, kneeY, kneeZ);
        glRotatef(knee_angle_L, 1.0f, 0.0f, 0.0f);
        glTranslatef(0, -kneeY, -kneeZ);
        drawLowerLegArmor();
        drawSabatons();
        glPopMatrix();
    }
    glPopMatrix();

    // --- Draw Right Leg & Armor ---
    glPushMatrix();
    glTranslatef(0.75f, -2.0f, 0.55f);
    animateLegVertices(hip_angle_R, knee_angle_R, true);
    //glColor3f(0.9f, 0.7f, 0.6f); // Set skin color
    drawLeg();
    if (gArmorVisible) {
        glPushMatrix();
        // CORRECTED: Isolate the mirroring so it only affects the armor
        glPushMatrix();
        glScalef(-1, 1, 1);
        glFrontFace(GL_CW);

        glTranslatef(0, hipY, hipZ);
        glRotatef(hip_angle_R, 1.0f, 0.0f, 0.0f);
        glTranslatef(0, -hipY, -hipZ);
        drawUpperLegArmor();
        glTranslatef(0, kneeY, kneeZ);
        glRotatef(knee_angle_R, 1.0f, 0.0f, 0.0f);
        glTranslatef(0, -kneeY, -kneeZ);
        drawLowerLegArmor();
        drawSabatons();
        glFrontFace(GL_CCW);
        glPopMatrix(); // End mirroring
        glPopMatrix();
    }
    glPopMatrix();

    // --- Draw Body, Head, and Armor ---
    if (gArmorVisible) {
        glPushMatrix();
        glTranslatef(0.0f, 6.0f - 0.05f, 0.0f);
        glScalef(BODY_SCALE, BODY_SCALE, BODY_SCALE);
        glRotatef(g_pose.torsoYaw, 0, 1, 0);
        glRotatef(g_pose.torsoPitch, 1, 0, 0);
        glRotatef(g_pose.torsoRoll, 0, 0, 1);
        drawArmor();
        drawShoulderArmor();
        glPushMatrix();
        glTranslatef(0, HEAD_CENTER_Y, 0);
        glRotatef(g_pose.headYaw, 0, 1, 0);
        glRotatef(g_pose.headPitch, 1, 0, 0);
        glRotatef(g_pose.headRoll, 0, 0, 1);
        glTranslatef(0, -0.2f, 0);
        drawHelmet();
        glPopMatrix();
        glPopMatrix();
    }

    drawBodyAndHead(0.0f, 0.0f, leftArmSwing, rightArmSwing);

    glPopMatrix();
}

void display() {
    glClearColor(0.6f, 0.3f, 0.7f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    setRenderMode(gRenderMode);
    // Handle viewport and projection setup
    if (gViewportMode) {
        // Split viewport mode - render model twice with different projections

        // Clear the entire screen first
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Left viewport - Orthographic
        int halfWidth = gWidth / 2;
        glViewport(0, 0, halfWidth, gHeight);
        glLoadIdentity();
        float orthoSize = gDist * 0.5f;
        float aspect = (float)halfWidth / gHeight;
        glOrtho(-orthoSize * aspect, orthoSize * aspect, -orthoSize, orthoSize, 0.1f, 100.0f);

        // Render scene in orthographic mode (left half)
        glMatrixMode(GL_MODELVIEW); glLoadIdentity();
        Vec3 eye; { eye.x = gTarget.x + gDist * cosf(gPitch) * sinf(gYaw); eye.y = gTarget.y + gDist * sinf(gPitch); eye.z = gTarget.z + gDist * cosf(gPitch) * cosf(gYaw); }
        gluLookAt(eye.x, eye.y, eye.z, gTarget.x, gTarget.y, gTarget.z, 0.0, 1.0, 0.0);

        // Render the scene for orthographic view
        renderScene();

        // Right viewport - Perspective
        glViewport(halfWidth, 0, halfWidth, gHeight);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(45.0, (double)halfWidth / gHeight, 0.1, 100.0);

        // Render scene in perspective mode (right half)
        glMatrixMode(GL_MODELVIEW); glLoadIdentity();
        gluLookAt(eye.x, eye.y, eye.z, gTarget.x, gTarget.y, gTarget.z, 0.0, 1.0, 0.0);

        // Render the scene for perspective view
        renderScene();

        return; // Skip the single viewport rendering below

    }
    else {
        // Single viewport mode
        glViewport(0, 0, gWidth, gHeight);

        if (gProjMode == PROJ_ORTHOGRAPHIC) {
            // Orthographic projection
            float orthoSize = gDist * 0.5f;
            float aspect = (float)gWidth / gHeight;
            glOrtho(-orthoSize * aspect, orthoSize * aspect, -orthoSize, orthoSize, 0.1f, 100.0f);
        }
        else {
            // Perspective projection (default)
            gluPerspective(45.0, (double)gWidth / gHeight, 0.1, 100.0);
        }
    }

    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    Vec3 eye; { eye.x = gTarget.x + gDist * cosf(gPitch) * sinf(gYaw); eye.y = gTarget.y + gDist * sinf(gPitch); eye.z = gTarget.z + gDist * cosf(gPitch) * cosf(gYaw); }
    gluLookAt(eye.x, eye.y, eye.z, gTarget.x, gTarget.y, gTarget.z, 0.0, 1.0, 0.0);

    // Update animation and movement state
    gIsMoving = keyUp || keydown;
    gRunningAnim = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    if (gIsMoving) {
        gAnimTime += 0.016f;
    }

    // Render the scene
    renderScene();
}
// ===================================================================
//
// SECTION 4: WIN32 APPLICATION FRAMEWORK
//
// ===================================================================

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
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

    // Display configuration controls
    printf("=== CONFIGURATION CONTROLS ===\n");
    printf("JAW CONFIGURATION:\n");
    printf("J/K - Adjust jaw curvature (%.1f) - smoother/sharper curve\n", JAW_CURVATURE);
    printf("H/L - Adjust jaw width (%.2f) - narrower/wider chin\n", JAW_WIDTH_MIN);
    printf("Y/U - Adjust jaw transition (%.2f) - lower/higher jaw narrowing\n", JAW_TRANSITION);
    printf("\nFACIAL FEATURES:\n");
    printf("4/5 - Adjust eye size (%.1f) - smaller/larger eyes\n", EYE_SCALE);
    printf("6/7 - Adjust nose size (%.1f) - smaller/larger nose\n", NOSE_SCALE);
    printf("8/9 - Adjust mouth size (%.1f) - smaller/larger mouth\n", MOUTH_SCALE);
    printf("T/G - Adjust eyebrow thickness (%.1f) - thinner/thicker\n", EYEBROW_SCALE);
    printf("B/V - Adjust hair volume (%.1f) - less/more volume\n", HAIR_SCALE);
    printf("\nMESH QUALITY:\n");
    printf("Torso segments: %d, Head segments: %d, Head layers: %d\n", TORSO_SEGMENTS, HEAD_SEGMENTS, HEAD_LAYERS);
    printf("(Edit #defines in code to change mesh quality)\n");
    printf("\nAUDIO CONTROLS:\n");
    printf("T - Toggle war sound on/off\n");
    printf("+/- - Increase/decrease war sound volume\n");
    printf("K - Toggle background visibility\n");
    printf("\nCOLLISION SYSTEM:\n");
    printf("R - Toggle collision debug visualization\n");
    printf("Character now has realistic collision with castles and objects\n");
    printf("===============================\n");

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

    // Cleanup background system
    BackgroundRenderer::cleanup();

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
        
        // Calculate potential new position
        float newX = gCharacterPos.x - sin(angleRad) * gMoveSpeed * dt;
        float newZ = gCharacterPos.z - cos(angleRad) * gMoveSpeed * dt;
        
        // Character collision radius (adjust based on character size)
        float characterRadius = 1.2f;
        
        // Check collision before moving
        if (!BackgroundRenderer::checkCollision(newX, newZ, characterRadius)) {
            // Safe to move - update position
            gCharacterPos.x = newX;
            gCharacterPos.z = newZ;
        } else {
            // Collision detected - try sliding along walls
            // Try moving only in X direction
            if (!BackgroundRenderer::checkCollision(newX, gCharacterPos.z, characterRadius)) {
                gCharacterPos.x = newX;
            }
            // Try moving only in Z direction
            else if (!BackgroundRenderer::checkCollision(gCharacterPos.x, newZ, characterRadius)) {
                gCharacterPos.z = newZ;
            }
            // If both fail, character is blocked completely
        }
        
        float phaseSpeed = keyShift ? 7.0f : 4.5f;     // Enhanced: More natural walking rhythm
        gWalkPhase += gMoveSpeed * dt * phaseSpeed / 2.0f;  // Enhanced: Smoother phase progression
    }

    // Update animations
    updateFistAnimation(dt);
    updateKungFuAnimation(dt);  // Add kung fu animation updates
    updateBoxingStance(dt);     // Add boxing stance animation updates
    updateJumpAnimation(dt);    // Add jump animation updates
    updateDance();              // Add K-pop dance animation updates
    updateSwordAttackAnimation(dt); // Add sword attack animation updates
    updateSpearAttackAnimation(dt); // Add spear attack animation updates
    updateShieldBlockAnimation(dt); // Add shield block animation updates

    // Update slow arm bending
    if (keySlash) {
        gSlowArmBendActive = true;
        // Slowly bend both arms up bit by bit
        gLeftLowerArmBend += gSlowArmBendSpeed * dt;
        gRightLowerArmBend += gSlowArmBendSpeed * dt;
        
        // Clamp to maximum arm bend angle
        if (gLeftLowerArmBend > MAX_ARM_BEND) {
            gLeftLowerArmBend = MAX_ARM_BEND;
        }
        if (gRightLowerArmBend > MAX_ARM_BEND) {
            gRightLowerArmBend = MAX_ARM_BEND;
        }
    } else {
        gSlowArmBendActive = false;
        // Slowly return arms to normal position when key is released
        if (gLeftLowerArmBend > 0.0f) {
            gLeftLowerArmBend -= gSlowArmBendSpeed * dt * 1.5f; // Return slightly faster
            if (gLeftLowerArmBend < 0.0f) {
                gLeftLowerArmBend = 0.0f;
            }
        }
        if (gRightLowerArmBend > 0.0f) {
            gRightLowerArmBend -= gSlowArmBendSpeed * dt * 1.5f; // Return slightly faster
            if (gRightLowerArmBend < 0.0f) {
                gRightLowerArmBend = 0.0f;
            }
        }
    }

    // Update background animation
    BackgroundRenderer::update(dt);
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
        else if (wParam == '1' || wParam == VK_NUMPAD1) {   // Wireframe
            setRenderMode(RM_WIREFRAME);
            InvalidateRect(hWnd, NULL, FALSE);
        }
        else if (wParam == '2' || wParam == VK_NUMPAD2) {   // Solid (glColor)
            setRenderMode(RM_SOLID);
            InvalidateRect(hWnd, NULL, FALSE);
        }
        else if (wParam == '3' || wParam == VK_NUMPAD3) {   // Textured
            setRenderMode(RM_TEXTURED);
            InvalidateRect(hWnd, NULL, FALSE);
        }

        else if (wParam == 'F') { toggleFistAnimation(); } // Toggle fist animation
        else if (wParam == 'G') { startDance(); } // K-pop dance animation
        else if (wParam == 'B') { toggleBoxingStance(); } // Toggle boxing stance (guard position)
        else if (wParam == 'H') { gShowJointVisuals = !gShowJointVisuals; } // Toggle joint visualization
        else if (wParam == 'X') { 
            if (gSwordVisible) {
                gSwordVisible = false; // Hide sword
            } else {
                gSwordVisible = true; // Show sword
                if (gSpearVisible) gSpearVisible = false; // Hide spear if it's visible
            }
        } // Toggle sword visibility (mutual exclusive with spear)
        else if (wParam == 'Z') { startSwordAttack(); } // Warrior sword attack animation
        else if (wParam == 'V') { 
            if (gSpearVisible) {
                gSpearVisible = false; // Hide spear
            } else {
                gSpearVisible = true; // Show spear
                if (gSwordVisible) gSwordVisible = false; // Hide sword if it's visible
            }
        } // Toggle spear visibility (mutual exclusive with sword)
        else if (wParam == 'J') { 
            if (gSpearVisible) startSpearAttack(); // Spear thrust attack animation
        } // Spear thrust attack (only when spear is visible)
        else if (wParam == 'C') { 
            if (gShieldVisible) {
                gShieldVisible = false; // Hide shield
            } else {
                gShieldVisible = true; // Show shield
            }
        } // Toggle shield visibility
        else if (wParam == 'I') { 
            if (gShieldVisible) startShieldBlock(); // Shield block animation
        } // Shield block animation (only when shield is visible)
        else if (wParam == 'M') { gArmorVisible = !gArmorVisible; gConcreteHands = gArmorVisible;}
        // Helmet dome rotation controls
        //else if (wParam == '1') { g_helmetDomeRotationX += 5.0f; }
        //else if (wParam == '2') { g_helmetDomeRotationX -= 5.0f; }
        else if (wParam == '3') { g_helmetDomeRotationY += 5.0f; }
        else if (wParam == '4') { g_helmetDomeRotationY -= 5.0f; }
        //else if (wParam == '5') { g_helmetDomeRotationZ += 5.0f; }
        //else if (wParam == '6') { g_helmetDomeRotationZ -= 5.0f; }
        else if (wParam == 'K') { gBackgroundVisible = !gBackgroundVisible; } // Toggle background visibility
        else if (wParam == 'T') { // Toggle war sound
            if (BackgroundRenderer::warSoundPlaying) {
                BackgroundRenderer::stopWarSound();
            } else {
                BackgroundRenderer::playWarSound();
            }
        }
        else if (wParam == VK_OEM_PLUS) { // + key to increase volume
            BackgroundRenderer::setWarSoundVolume(BackgroundRenderer::warSoundVolume + 0.1f);
        }
        else if (wParam == VK_OEM_MINUS) { // - key to decrease volume
            BackgroundRenderer::setWarSoundVolume(BackgroundRenderer::warSoundVolume - 0.1f);
        }
        else if (wParam == 'R') { // R key to reset everything
            // Reset camera
            gYaw = 0.2f; gPitch = 0.1f; gDist = 15.0f; 
            gTarget = { 0.0f, 3.5f, 0.0f }; 
            
            // Reset character position and orientation
            gCharacterPos = { 0, 0, 0 }; 
            gCharacterYaw = 0;
            gMoveSpeed = 0.0f;
            gWalkPhase = 0.0f;
            
            // Reset animations
            gAnimTime = 0.0f;
            gIsMoving = false;
            gRunningAnim = false;
            gJumpVerticalOffset = 0.0f;
            
            // Reset all special animations
            gKungFuAnimating = false;
            gSwordAttackAnimating = false;
            gSpearAttackAnimating = false;
            gShieldBlockAnimating = false;
            gBoxingAnimActive = false;
            gInBoxingStance = false;
            gIsDancing = false;
            gIsJumping = false;
            gFistAnimationActive = false;
            
            // Reset weapon/armor visibility
            gSwordVisible = false;
            gSpearVisible = false;
            gShieldVisible = false;
            gArmorVisible = false;
            
            // Reset arm bending (including shield defense arm bend)
            gLeftLowerArmBend = 0.0f;
            gRightLowerArmBend = 0.0f;
            gSlowArmBendActive = false;
            
            // Stop war sound if playing
            if (BackgroundRenderer::warSoundPlaying) {
                BackgroundRenderer::stopWarSound();
            }
            
            // Reset background settings
            gBackgroundVisible = true;
            BackgroundRenderer::collisionDebugMode = false;
            
            printf("=== EVERYTHING RESET ===");
        }
        // Projection and Viewport Controls
        else if (wParam == 'O') { gProjMode = PROJ_ORTHOGRAPHIC; } // Orthographic projection
        else if (wParam == 'P') { gProjMode = PROJ_PERSPECTIVE; } // Perspective projection
        else if (wParam == 'L') { gViewportMode = !gViewportMode; } // Toggle split viewport
        // Kung Fu Animation Styles - Now with flowing anime-like animations!
        else if (wParam == 'Q') { startKungFuAnimation(1); } // Crane style animation
        else if (wParam == 'N') { startKungFuAnimation(0); } // Stop animation / Normal pose
        // Higher jaw transition
         // Facial Features Scale Controls
        else if (wParam == 'W') keyW = true; else if (wParam == 'S') keyS = true;
        else if (wParam == 'A') keyA = true; else if (wParam == 'D') keyD = true;
        else if (wParam == 'G') keyG = true;
        else if (wParam == VK_OEM_2) keySlash = true; // '/' key for slow arm bend
        else if (wParam == VK_UP) keyUp = true; else if (wParam == VK_DOWN) keydown = true;
        else if (wParam == VK_LEFT) keyLeft = true; else if (wParam == VK_RIGHT) keyRight = true;
        else if (wParam == VK_SHIFT) keyShift = true;
        return 0;
    case WM_KEYUP:
        if (wParam == 'W') keyW = false; else if (wParam == 'S') keyS = false;
        else if (wParam == 'A') keyA = false; else if (wParam == 'D') keyD = false;
        else if (wParam == 'G') keyG = false;
        else if (wParam == VK_OEM_2) keySlash = false; // '/' key
        else if (wParam == VK_UP) keyUp = false; else if (wParam == VK_DOWN) keydown = false;
        else if (wParam == VK_LEFT) keyLeft = false; else if (wParam == VK_RIGHT) keyRight = false;
        else if (wParam == VK_SHIFT) keyShift = false;
        return 0;

    default: return DefWindowProc(hWnd, msg, wParam, lParam);
    }
}
