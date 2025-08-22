// Build: Visual Studio (Win32/x64) — link opengl32.lib
// Put mulan5.obj beside the EXE, or change kModelPath.

#define NOMINMAX
#include <Windows.h>
#include <windowsx.h>  // GET_X_LPARAM, GET_Y_LPARAM, GET_WHEEL_DELTA_WPARAM
#include <gl/GL.h>
#pragma comment(lib, "opengl32.lib")

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
#include <string>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <cctype>
#include <cmath>

// =========================== Config ===========================
static const char* kWindowTitle = "Mulan OBJ – Camera, Face, Shoes & Walking Legs";
static const char* kModelPath = "mulan5.obj";           // change if needed
static const char* kHeadName = "Bip001_Head_011";      // target head group
// =============================================================

// =========================== Types ============================
struct Vec3 { float x = 0, y = 0, z = 0; };
struct Tri { int i0 = 0, i1 = 0, i2 = 0; int groupId = -1; };

struct Mesh {
    std::vector<Vec3> vertices;
    std::vector<Tri>  tris;
    std::vector<std::string> groupNames; // groupId -> name
    std::vector<Vec3> normals;           // per-vertex
    Vec3 bboxMin{ 0,0,0 }, bboxMax{ 0,0,0 }, centroid{ 0,0,0 };
};

struct LegSegment {
    std::vector<int> groups; // all group ids that belong to this segment
    Vec3 pivotTop{ 0,0,0 };    // pivot at segment's top (center xz, max y)
};
struct LegChain {
    LegSegment thigh, calf, foot, toe;
    bool valid() const {
        return !thigh.groups.empty() || !calf.groups.empty() || !foot.groups.empty() || !toe.groups.empty();
    }
};
// =============================================================

// ====================== Global Win32/GL =======================
HWND  g_hWnd = nullptr;
HDC   g_hDC = nullptr;
HGLRC g_hRC = nullptr;
int   g_Width = 1280, g_Height = 720;

Mesh  g_Model;
bool  g_ModelLoaded = false;

// Camera/orbit/pan/zoom
float g_Scale = 1.0f;
float g_CamDist = 6.0f;
float g_CamDistDefault = 6.0f;
float g_OrbitYawDeg = 0.0f;
float g_OrbitPitchDeg = 15.0f; // slight tilt
float g_PanX = 0.0f, g_PanY = 0.0f;

bool  g_Wireframe = false;

// Mouse
bool  g_LButtonDown = false;
int   g_LastMouseX = 0, g_LastMouseY = 0;

// Character world position & facing
float g_CharPosX = 0.0f, g_CharPosZ = 0.0f;
float g_HeadingDeg = 0.0f; // 面朝前进方向（绕Y）

// Face (head) rotation state
static float g_FaceYawDeg = 0.0f;  // J/K
static float g_FacePitchDeg = 0.0f;  // I/M
static float g_FaceRollDeg = 0.0f;  // U/O

// Groups
int   g_HeadGroupId = -1;
Vec3  g_HeadPivot{ 0,0,0 };

// Shoes control
std::vector<int> g_ShoeGroupIds;
bool g_ShoesOn = true;                   // visible or not
bool g_UsingFeetAsShoesFallback = false; // true if using Foot/Toe fallback

// Legs (detected by keywords or exact names)
LegChain g_LeftLeg;
LegChain g_RightLeg;

// Walking animation state
bool g_KeyLeft = false, g_KeyRight = false, g_KeyUp = false, g_KeyDown = false;
float g_WalkPhase = 0.0f;   // radians
float g_WalkBlend = 0.0f;   // 0..1 smooth in/out
// =============================================================

// ========================= Math utils =========================
static const float kPI = 3.14159265358979323846f;
static const float kTAU = 6.2831853071795864769f;

static inline Vec3 operator+(const Vec3& a, const Vec3& b) { return { a.x + b.x,a.y + b.y,a.z + b.z }; }
static inline Vec3 operator-(const Vec3& a, const Vec3& b) { return { a.x - b.x,a.y - b.y,a.z - b.z }; }
static inline Vec3 operator*(const Vec3& a, float s) { return { a.x * s,a.y * s,a.z * s }; }
static inline void addTo(Vec3& a, const Vec3& b) { a.x += b.x; a.y += b.y; a.z += b.z; }
static inline void normalize(Vec3& v) {
    float len = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len > 1e-6f) { v.x /= len; v.y /= len; v.z /= len; }
}
static void PerspectiveGL(double fovY, double aspect, double zNear, double zFar) {
    double fH = std::tan(fovY * 0.5 * 3.14159265358979323846 / 180.0) * zNear;
    double fW = fH * aspect;
    glFrustum(-fW, fW, -fH, fH, zNear, zFar);
}
static inline std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    size_t b = s.find_last_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    return s.substr(a, b - a + 1);
}
static inline std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c) { return char(std::tolower(c)); });
    return s;
}
// =============================================================

// ========================= OBJ loader =========================
struct FaceIdx { int v = 0, vt = 0, vn = 0; };

static bool parseFaceToken(const std::string& tok, int vCount, FaceIdx& out) {
    int vi = 0, vti = 0, vni = 0; bool have_vt = false, have_vn = false;
    size_t p1 = tok.find('/');
    if (p1 == std::string::npos) {
        vi = std::stoi(tok);
    }
    else {
        size_t p2 = tok.find('/', p1 + 1);
        vi = std::stoi(tok.substr(0, p1));
        if (p2 == std::string::npos) {
            std::string s_vt = tok.substr(p1 + 1);
            if (!s_vt.empty()) { vti = std::stoi(s_vt); have_vt = true; }
        }
        else {
            std::string s_vt = tok.substr(p1 + 1, p2 - p1 - 1);
            std::string s_vn = tok.substr(p2 + 1);
            if (!s_vt.empty()) { vti = std::stoi(s_vt); have_vt = true; }
            if (!s_vn.empty()) { vni = std::stoi(s_vn); have_vn = true; }
        }
    }
    auto fixIndex = [vCount](int idx)->int {
        return (idx < 0) ? (vCount + idx) : (idx - 1);
    };
    out.v = fixIndex(vi);
    out.vt = have_vt ? fixIndex(vti) : -1;
    out.vn = have_vn ? fixIndex(vni) : -1;
    return (out.v >= 0 && out.v < vCount);
}

static void computeBounds(Mesh& m) {
    if (m.vertices.empty()) { m.bboxMin = { 0,0,0 }; m.bboxMax = { 0,0,0 }; m.centroid = { 0,0,0 }; return; }
    m.bboxMin = m.bboxMax = m.vertices[0];
    Vec3 sum{ 0,0,0 };
    for (const auto& p : m.vertices) {
        m.bboxMin.x = std::min(m.bboxMin.x, p.x);
        m.bboxMin.y = std::min(m.bboxMin.y, p.y);
        m.bboxMin.z = std::min(m.bboxMin.z, p.z);
        m.bboxMax.x = std::max(m.bboxMax.x, p.x);
        m.bboxMax.y = std::max(m.bboxMax.y, p.y);
        m.bboxMax.z = std::max(m.bboxMax.z, p.z);
        addTo(sum, p);
    }
    float inv = 1.0f / float(m.vertices.size());
    m.centroid = sum * inv;
}

static void computeVertexNormals(Mesh& m) {
    m.normals.assign(m.vertices.size(), { 0,0,0 });
    for (const auto& t : m.tris) {
        const Vec3& A = m.vertices[t.i0];
        const Vec3& B = m.vertices[t.i1];
        const Vec3& C = m.vertices[t.i2];
        Vec3 U = B - A;
        Vec3 V = C - A;
        Vec3 N = { U.y * V.z - U.z * V.y, U.z * V.x - U.x * V.z, U.x * V.y - U.y * V.x };
        addTo(m.normals[t.i0], N);
        addTo(m.normals[t.i1], N);
        addTo(m.normals[t.i2], N);
    }
    for (auto& n : m.normals) normalize(n);
}

static int getOrCreateGroup(std::unordered_map<std::string, int>& map, Mesh& m, const std::string& name) {
    auto it = map.find(name);
    if (it != map.end()) return it->second;
    int id = (int)m.groupNames.size();
    m.groupNames.push_back(name);
    map[name] = id;
    return id;
}

static bool loadOBJ(const char* path, Mesh& out) {
    std::ifstream in(path);
    if (!in.is_open()) return false;

    out = Mesh{};
    std::vector<Vec3> tempV;
    std::unordered_map<std::string, int> groupMap;
    int currentGroup = getOrCreateGroup(groupMap, out, "default");

    std::string line;
    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        if (line.rfind("v ", 0) == 0) {
            std::istringstream ss(line.substr(2));
            Vec3 p; ss >> p.x >> p.y >> p.z; tempV.push_back(p);
        }
        else if (line.rfind("g ", 0) == 0) {
            std::string gname = trim(line.substr(2));
            std::istringstream gs(gname);
            std::string first; gs >> first;
            if (first.empty()) first = "default";
            currentGroup = getOrCreateGroup(groupMap, out, first);
        }
        else if (line.rfind("f ", 0) == 0) {
            std::istringstream fs(line.substr(2));
            std::vector<FaceIdx> idx;
            std::string tok;
            while (fs >> tok) {
                FaceIdx fi;
                if (parseFaceToken(tok, (int)tempV.size(), fi))
                    idx.push_back(fi);
            }
            if (idx.size() < 3) continue;
            for (size_t i = 1; i + 1 < idx.size(); ++i) {
                Tri t;
                t.i0 = idx[0].v;
                t.i1 = idx[i].v;
                t.i2 = idx[i + 1].v;
                t.groupId = currentGroup;
                out.tris.push_back(t);
            }
        }
        else {
            // ignore vt, vn, usemtl, mtllib, s, o...
        }
    }
    out.vertices = tempV;
    computeBounds(out);
    computeVertexNormals(out);
    return true;
}
// =============================================================

// ============ Head & Shoes detection + Leg detection ==========
static int findGroupIdByExactName(const Mesh& m, const char* name) {
    for (size_t i = 0; i < m.groupNames.size(); ++i)
        if (m.groupNames[i] == name) return (int)i;
    return -1;
}
static bool containsWord(const std::string& s, const char* word) {
    std::string t = toLower(s);
    std::string w = toLower(std::string(word));
    return t.find(w) != std::string::npos;
}
static bool isLeftName(const std::string& s) {
    std::string t = toLower(s);
    return t.find("_l") != std::string::npos || t.find(" left") != std::string::npos || t.find("left_") != std::string::npos;
}
static bool isRightName(const std::string& s) {
    std::string t = toLower(s);
    return t.find("_r") != std::string::npos || t.find(" right") != std::string::npos || t.find("right_") != std::string::npos;
}

static Vec3 computeGroupsBBoxPivotTop(const Mesh& m, const std::vector<int>& groupIds) {
    if (groupIds.empty()) return m.centroid;
    Vec3 mn{ +1e9f,+1e9f,+1e9f }, mx{ -1e9f,-1e9f,-1e9f };
    bool any = false;
    for (int gid : groupIds) {
        for (const auto& t : m.tris) {
            if (t.groupId != gid) continue;
            const Vec3& a = m.vertices[t.i0], & b = m.vertices[t.i1], & c = m.vertices[t.i2];
            if (!any) { mn = a; mx = a; any = true; }
            mn.x = std::min(mn.x, a.x); mn.y = std::min(mn.y, a.y); mn.z = std::min(mn.z, a.z);
            mn.x = std::min(mn.x, b.x); mn.y = std::min(mn.y, b.y); mn.z = std::min(mn.z, b.z);
            mn.x = std::min(mn.x, c.x); mn.y = std::min(mn.y, c.y); mn.z = std::min(mn.z, c.z);
            mx.x = std::max(mx.x, a.x); mx.y = std::max(mx.y, a.y); mx.z = std::max(mx.z, a.z);
            mx.x = std::max(mx.x, b.x); mx.y = std::max(mx.y, b.y); mx.z = std::max(mx.z, b.z);
            mx.x = std::max(mx.x, c.x); mx.y = std::max(mx.y, c.y); mx.z = std::max(mx.z, c.z);
        }
    }
    if (!any) return m.centroid;
    return { (mn.x + mx.x) * 0.5f, mx.y, (mn.z + mx.z) * 0.5f };
}

static void chooseHeadGroupAndPivot() {
    g_HeadGroupId = findGroupIdByExactName(g_Model, kHeadName);
    if (g_HeadGroupId < 0) {
        int best = -1;
        for (int gid = 0; gid < (int)g_Model.groupNames.size(); ++gid)
            if (containsWord(g_Model.groupNames[gid], "head")) { best = gid; break; }
        if (best >= 0) g_HeadGroupId = best;
    }
    if (g_HeadGroupId < 0) {
        double bestY = -1e9; int bestId = -1;
        for (int gid = 0; gid < (int)g_Model.groupNames.size(); ++gid) {
            double sy = 0; size_t cnt = 0;
            for (const auto& t : g_Model.tris) {
                if (t.groupId != gid) continue;
                sy += g_Model.vertices[t.i0].y + g_Model.vertices[t.i1].y + g_Model.vertices[t.i2].y;
                cnt += 3;
            }
            double cy = (cnt ? sy / double(cnt) : 0.0);
            if (cy > bestY) { bestY = cy; bestId = gid; }
        }
        g_HeadGroupId = bestId;
    }
    if (g_HeadGroupId >= 0)
        g_HeadPivot = computeGroupsBBoxPivotTop(g_Model, std::vector<int>{g_HeadGroupId});
    else
        g_HeadPivot = g_Model.centroid;
}

static void chooseShoeGroups() {
    g_ShoeGroupIds.clear();
    g_UsingFeetAsShoesFallback = false;

    // 1) Prefer real shoes: names contain "shoe" or "boot"
    for (int gid = 0; gid < (int)g_Model.groupNames.size(); ++gid) {
        std::string n = toLower(g_Model.groupNames[gid]);
        if (n.find("shoe") != std::string::npos || n.find("boot") != std::string::npos)
            g_ShoeGroupIds.push_back(gid);
    }
    if (!g_ShoeGroupIds.empty()) {
        g_ShoesOn = false; // start OFF so first Q puts shoes on
        OutputDebugStringA("[INFO] Found shoe groups. Start with shoes OFF.\n");
        return;
    }

    // 2) Fallback: use Foot/Toe as "shoes"
    for (int gid = 0; gid < (int)g_Model.groupNames.size(); ++gid) {
        std::string n = toLower(g_Model.groupNames[gid]);
        if (n.find("foot") != std::string::npos || n.find("toe") != std::string::npos)
            g_ShoeGroupIds.push_back(gid);
    }
    if (!g_ShoeGroupIds.empty()) {
        g_UsingFeetAsShoesFallback = true;
        g_ShoesOn = true;
        OutputDebugStringA("[WARN] No shoe groups; using Foot/Toe as fallback. Start with shoes ON.\n");
    }
    else {
        g_ShoesOn = true; // nothing to toggle
        OutputDebugStringA("[WARN] No shoe/foot/toe groups detected; Q has no effect.\n");
    }
}

// ---------- Legs: prioritize your exact group names; fallback to keywords ----------
static void detectLegChainOneSide(bool left, LegChain& out) {
    out = LegChain{};

    auto pushIfFound = [&](const char* name, std::vector<int>& dst) {
        int id = findGroupIdByExactName(g_Model, name);
        if (id >= 0) dst.push_back(id);
        return id >= 0;
    };

    bool usedExact = false;
    if (left) {
        bool any = false;
        any |= pushIfFound("Bip001_L_Calf_02", out.calf.groups);
        any |= pushIfFound("Bip001_L_Foot_03", out.foot.groups);
        any |= pushIfFound("Bip001_L_Toe0_04", out.toe.groups);
        usedExact = any;
    }
    else {
        bool any = false;
        any |= pushIfFound("Bip001_R_Calf_05", out.calf.groups);
        any |= pushIfFound("Bip001_R_Foot_06", out.foot.groups);
        any |= pushIfFound("Bip001_R_Toe0_07", out.toe.groups);
        usedExact = any;
    }

    if (!usedExact) {
        for (int gid = 0; gid < (int)g_Model.groupNames.size(); ++gid) {
            const std::string& name = g_Model.groupNames[gid];
            bool side = left ? isLeftName(name) : isRightName(name);
            if (!side) continue;
            std::string t = toLower(name);
            if (t.find("thigh") != std::string::npos || t.find("upleg") != std::string::npos || t.find("upperleg") != std::string::npos) {
                out.thigh.groups.push_back(gid);
            }
            else if (t.find("calf") != std::string::npos || t.find("shin") != std::string::npos || t.find("leg") != std::string::npos) {
                out.calf.groups.push_back(gid);
            }
            else if (t.find("foot") != std::string::npos) {
                out.foot.groups.push_back(gid);
            }
            else if (t.find("toe") != std::string::npos) {
                out.toe.groups.push_back(gid);
            }
        }
    }

    if (!out.thigh.groups.empty()) out.thigh.pivotTop = computeGroupsBBoxPivotTop(g_Model, out.thigh.groups);
    if (!out.calf.groups.empty())  out.calf.pivotTop = computeGroupsBBoxPivotTop(g_Model, out.calf.groups);
    if (!out.foot.groups.empty())  out.foot.pivotTop = computeGroupsBBoxPivotTop(g_Model, out.foot.groups);
    if (!out.toe.groups.empty())  out.toe.pivotTop = computeGroupsBBoxPivotTop(g_Model, out.toe.groups);
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
    glDisable(GL_CULL_FACE);       // safer if winding is mixed
    glShadeModel(GL_SMOOTH);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    GLfloat lightPos[4] = { 0.2f, 1.0f, 1.0f, 0.0f };
    GLfloat lightCol[4] = { 1, 1, 1, 1 };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightCol);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightCol);

    GLfloat matDiff[4] = { 0.8f, 0.8f, 0.85f, 1 };
    GLfloat matSpec[4] = { 0.2f, 0.2f, 0.2f,  1 };
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, matDiff);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, matSpec);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 16.0f);

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

// Draw helpers
static void drawGroups(const Mesh& m, const std::vector<int>& groups, const std::unordered_set<int>& skip) {
    if (groups.empty()) return;
    glBegin(GL_TRIANGLES);
    for (const auto& t : m.tris) {
        if (skip.find(t.groupId) != skip.end()) continue;
        if (std::find(groups.begin(), groups.end(), t.groupId) == groups.end()) continue;
        const Vec3& n0 = m.normals[t.i0];
        const Vec3& n1 = m.normals[t.i1];
        const Vec3& n2 = m.normals[t.i2];
        const Vec3& v0 = m.vertices[t.i0];
        const Vec3& v1 = m.vertices[t.i1];
        const Vec3& v2 = m.vertices[t.i2];
        glNormal3f(n0.x, n0.y, n0.z); glVertex3f(v0.x, v0.y, v0.z);
        glNormal3f(n1.x, n1.y, n1.z); glVertex3f(v1.x, v1.y, v1.z);
        glNormal3f(n2.x, n2.y, n2.z); glVertex3f(v2.x, v2.y, v2.z);
    }
    glEnd();
}

static void drawMeshExcludingGroups(const Mesh& m, const std::unordered_set<int>& excluded) {
    glBegin(GL_TRIANGLES);
    for (const auto& t : m.tris) {
        if (excluded.find(t.groupId) != excluded.end()) continue;
        const Vec3& n0 = m.normals[t.i0];
        const Vec3& n1 = m.normals[t.i1];
        const Vec3& n2 = m.normals[t.i2];
        const Vec3& v0 = m.vertices[t.i0];
        const Vec3& v1 = m.vertices[t.i1];
        const Vec3& v2 = m.vertices[t.i2];
        glNormal3f(n0.x, n0.y, n0.z); glVertex3f(v0.x, v0.y, v0.z);
        glNormal3f(n1.x, n1.y, n1.z); glVertex3f(v1.x, v1.y, v1.z);
        glNormal3f(n2.x, n2.y, n2.z); glVertex3f(v2.x, v2.y, v2.z);
    }
    glEnd();
}

static void calcAutoScaleAndCam() {
    Vec3 size = g_Model.bboxMax - g_Model.bboxMin;
    float maxDim = std::max(size.x, std::max(size.y, size.z));
    if (maxDim < 1e-6f) maxDim = 1.0f;
    g_Scale = 2.5f / maxDim;   // Fit largest dimension to ~2.5 units
    g_CamDist = 6.0f;
    g_CamDistDefault = g_CamDist;
}

// ★ 带 footLift 的腿链绘制（支持缺失 Thigh）
static void drawLegChain(const LegChain& leg,
    float hipDeg, float kneeDeg, float ankleDeg, float toeDeg,
    float footLift,
    const std::unordered_set<int>& skipSet)
{
    if (!leg.valid()) return;

    auto hasThigh = !leg.thigh.groups.empty();
    auto hasCalf = !leg.calf.groups.empty();
    auto hasFoot = !leg.foot.groups.empty();
    auto hasToe = !leg.toe.groups.empty();

    glPushMatrix();
    if (hasThigh) {
        // Root = Thigh
        glPushMatrix();
        glTranslatef(leg.thigh.pivotTop.x, leg.thigh.pivotTop.y, leg.thigh.pivotTop.z);
        glRotatef(hipDeg, 1, 0, 0); // 髋
        glTranslatef(-leg.thigh.pivotTop.x, -leg.thigh.pivotTop.y, -leg.thigh.pivotTop.z);
        drawGroups(g_Model, leg.thigh.groups, skipSet);

        if (hasCalf) {
            glPushMatrix();
            glTranslatef(leg.calf.pivotTop.x, leg.calf.pivotTop.y, leg.calf.pivotTop.z);
            glRotatef(kneeDeg, 1, 0, 0); // 膝
            glTranslatef(-leg.calf.pivotTop.x, -leg.calf.pivotTop.y, -leg.calf.pivotTop.z);
            drawGroups(g_Model, leg.calf.groups, skipSet);

            if (hasFoot) {
                glPushMatrix();
                glTranslatef(leg.foot.pivotTop.x, leg.foot.pivotTop.y, leg.foot.pivotTop.z);
                glRotatef(ankleDeg, 1, 0, 0); // 踝
                if (footLift > 0.0f) glTranslatef(0.0f, footLift, 0.0f); // 抬脚
                glTranslatef(-leg.foot.pivotTop.x, -leg.foot.pivotTop.y, -leg.foot.pivotTop.z);
                drawGroups(g_Model, leg.foot.groups, skipSet);

                if (hasToe) {
                    glPushMatrix();
                    glTranslatef(leg.toe.pivotTop.x, leg.toe.pivotTop.y, leg.toe.pivotTop.z);
                    glRotatef(toeDeg, 1, 0, 0); // 趾
                    glTranslatef(-leg.toe.pivotTop.x, -leg.toe.pivotTop.y, -leg.toe.pivotTop.z);
                    drawGroups(g_Model, leg.toe.groups, skipSet);
                    glPopMatrix();
                }
                glPopMatrix();
            }
            glPopMatrix();
        }
        glPopMatrix();
    }
    else if (hasCalf) {
        // Root = Calf（无大腿时，把髋摆动的一部分合入小腿）
        glPushMatrix();
        glTranslatef(leg.calf.pivotTop.x, leg.calf.pivotTop.y, leg.calf.pivotTop.z);
        glRotatef(0.5f * hipDeg + kneeDeg, 1, 0, 0); // 0.5*hip + knee 近似
        glTranslatef(-leg.calf.pivotTop.x, -leg.calf.pivotTop.y, -leg.calf.pivotTop.z);
        drawGroups(g_Model, leg.calf.groups, skipSet);

        if (hasFoot) {
            glPushMatrix();
            glTranslatef(leg.foot.pivotTop.x, leg.foot.pivotTop.y, leg.foot.pivotTop.z);
            glRotatef(ankleDeg, 1, 0, 0);
            if (footLift > 0.0f) glTranslatef(0.0f, footLift, 0.0f);
            glTranslatef(-leg.foot.pivotTop.x, -leg.foot.pivotTop.y, -leg.foot.pivotTop.z);
            drawGroups(g_Model, leg.foot.groups, skipSet);

            if (hasToe) {
                glPushMatrix();
                glTranslatef(leg.toe.pivotTop.x, leg.toe.pivotTop.y, leg.toe.pivotTop.z);
                glRotatef(toeDeg, 1, 0, 0);
                glTranslatef(-leg.toe.pivotTop.x, -leg.toe.pivotTop.y, -leg.toe.pivotTop.z);
                drawGroups(g_Model, leg.toe.groups, skipSet);
                glPopMatrix();
            }
            glPopMatrix();
        }
        glPopMatrix();
    }
    else if (hasFoot) {
        // 只有脚/趾时
        glPushMatrix();
        glTranslatef(leg.foot.pivotTop.x, leg.foot.pivotTop.y, leg.foot.pivotTop.z);
        glRotatef(ankleDeg + 0.5f * hipDeg + 0.5f * kneeDeg, 1, 0, 0);
        if (footLift > 0.0f) glTranslatef(0.0f, footLift, 0.0f);
        glTranslatef(-leg.foot.pivotTop.x, -leg.foot.pivotTop.y, -leg.foot.pivotTop.z);
        drawGroups(g_Model, leg.foot.groups, skipSet);

        if (hasToe) {
            glPushMatrix();
            glTranslatef(leg.toe.pivotTop.x, leg.toe.pivotTop.y, leg.toe.pivotTop.z);
            glRotatef(toeDeg, 1, 0, 0);
            glTranslatef(-leg.toe.pivotTop.x, -leg.toe.pivotTop.y, -leg.toe.pivotTop.z);
            drawGroups(g_Model, leg.toe.groups, skipSet);
            glPopMatrix();
        }
        glPopMatrix();
    }
    else if (hasToe) {
        glPushMatrix();
        glTranslatef(leg.toe.pivotTop.x, leg.toe.pivotTop.y, leg.toe.pivotTop.z);
        glRotatef(toeDeg + ankleDeg + kneeDeg + hipDeg, 1, 0, 0);
        glTranslatef(-leg.toe.pivotTop.x, -leg.toe.pivotTop.y, -leg.toe.pivotTop.z);
        drawGroups(g_Model, leg.toe.groups, skipSet);
        glPopMatrix();
    }
    glPopMatrix();
}

static void renderScene() {
    glClearColor(0.08f, 0.08f, 0.09f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // ---- Camera transforms (orbit viewer) ----
    glTranslatef(g_PanX, g_PanY, 0.0f);         // W/S/A/D pan
    glTranslatef(0.0f, 0.0f, -g_CamDist);       // wheel zoom
    glRotatef(g_OrbitPitchDeg, 1, 0, 0);          // mouse drag
    glRotatef(g_OrbitYawDeg, 0, 1, 0);

    // ---- Character (world-space) transform ----
    glPushMatrix();
    glTranslatef(g_CharPosX, 0.0f, g_CharPosZ);  // ←↑→↓ walk in X/Z
    glRotatef(g_HeadingDeg, 0, 1, 0);             // 面朝前进方向

    // ---- Model local transforms ----
    glScalef(g_Scale, g_Scale, g_Scale);
    glTranslatef(-g_Model.centroid.x, -g_Model.centroid.y, -g_Model.centroid.z);

    // Build exclusion set: head (draw later), shoes (if OFF), legs (重绘)
    std::unordered_set<int> excluded;
    if (g_HeadGroupId >= 0) excluded.insert(g_HeadGroupId);
    if (!g_ShoesOn) {
        for (int gid : g_ShoeGroupIds) excluded.insert(gid);
    }
    auto addAll = [&](const std::vector<int>& v) { for (int id : v) excluded.insert(id); };
    addAll(g_LeftLeg.thigh.groups); addAll(g_LeftLeg.calf.groups);
    addAll(g_LeftLeg.foot.groups); addAll(g_LeftLeg.toe.groups);
    addAll(g_RightLeg.thigh.groups); addAll(g_RightLeg.calf.groups);
    addAll(g_RightLeg.foot.groups); addAll(g_RightLeg.toe.groups);

    // Draw body (without head, without legs, and without shoes if toggled off)
    drawMeshExcludingGroups(g_Model, excluded);

    // ---- 计算步态角度（更明显） ----
    float blend = g_WalkBlend; // 0..1
    const float hipAmp = 30.0f * blend;   // 髋
    const float kneeAmp = 45.0f * blend;   // 膝
    const float ankleAmp = 20.0f * blend;   // 踝
    const float toeAmp = 15.0f * blend;   // 趾
    const float liftAmp = 0.06f * blend;   // 抬脚高度（按模型尺寸可调）

    float phaseL = g_WalkPhase;
    float phaseR = g_WalkPhase + kPI;

    float hipL = hipAmp * std::sin(phaseL);
    float hipR = hipAmp * std::sin(phaseR);

    auto kneeFromPhase = [&](float ph) {
        float s = std::sin(ph);
        return (s > 0.0f) ? (kneeAmp * s) : 0.0f;
    };
    float kneeL = kneeFromPhase(phaseL);
    float kneeR = kneeFromPhase(phaseR);

    auto ankleFromPhase = [&](float ph, float kneeDeg) {
        return -0.5f * kneeDeg + ankleAmp * std::sin(ph + 0.35f);
    };
    float ankleL = ankleFromPhase(phaseL, kneeL);
    float ankleR = ankleFromPhase(phaseR, kneeR);

    auto toeFromPhase = [&](float ph) {
        float s = std::sin(ph);
        return (s > 0.0f) ? (toeAmp * s * 0.6f) : 0.0f;
    };
    float toeL = toeFromPhase(phaseL);
    float toeR = toeFromPhase(phaseR);

    auto liftFromPhase = [&](float ph) {
        float s = std::sin(ph);
        if (s > 0.0f) return liftAmp * (s * s); // 平滑上抬
        return 0.0f;
    };
    float liftL = liftFromPhase(phaseL);
    float liftR = liftFromPhase(phaseR);

    // ---- Draw legs with joint rotations + 抬脚 ----
    std::unordered_set<int> shoeSkip;
    if (!g_ShoesOn) for (int gid : g_ShoeGroupIds) shoeSkip.insert(gid);

    drawLegChain(g_LeftLeg, hipL, kneeL, ankleL, toeL, liftL, shoeSkip);
    drawLegChain(g_RightLeg, hipR, kneeR, ankleR, toeR, liftR, shoeSkip);

    // ---- Draw rotating head last ----
    if (g_HeadGroupId >= 0) {
        glPushMatrix();
        glTranslatef(g_HeadPivot.x, g_HeadPivot.y, g_HeadPivot.z);
        glRotatef(g_FaceYawDeg, 0, 1, 0);
        glRotatef(g_FacePitchDeg, 1, 0, 0);
        glRotatef(g_FaceRollDeg, 0, 0, 1);
        glTranslatef(-g_HeadPivot.x, -g_HeadPivot.y, -g_HeadPivot.z);

        std::vector<int> headOnly{g_HeadGroupId};
        std::unordered_set<int> noSkip;
        drawGroups(g_Model, headOnly, noSkip);
        glPopMatrix();
    }
    glPopMatrix();
}
// =============================================================

// ========================= Controls ===========================
static void ResetAll() {
    g_OrbitYawDeg = 0.0f;
    g_OrbitPitchDeg = 15.0f;
    g_PanX = g_PanY = 0.0f;
    g_CamDist = g_CamDistDefault;

    g_FaceYawDeg = g_FacePitchDeg = g_FaceRollDeg = 0.0f;

    g_CharPosX = g_CharPosZ = 0.0f;
    g_HeadingDeg = 0.0f;

    g_WalkPhase = 0.0f;
    g_WalkBlend = 0.0f;
    g_KeyLeft = g_KeyRight = g_KeyUp = g_KeyDown = false;
}

static void OnMouseDragOrbit(int x, int y) {
    const float sens = 0.3f; // degrees per pixel
    int dx = x - g_LastMouseX;
    int dy = y - g_LastMouseY;
    g_LastMouseX = x; g_LastMouseY = y;

    g_OrbitYawDeg += dx * sens;
    g_OrbitPitchDeg += dy * sens;
    if (g_OrbitPitchDeg > 89.0f) g_OrbitPitchDeg = 89.0f;
    if (g_OrbitPitchDeg < -89.0f) g_OrbitPitchDeg = -89.0f;
}

static void OnMouseWheel(short delta) {
    if (delta > 0) g_CamDist *= 0.9f; else g_CamDist *= 1.1f;
    if (g_CamDist < 0.5f) g_CamDist = 0.5f;
    if (g_CamDist > 200.0f) g_CamDist = 200.0f;
}

static void OnPanKey(WORD key) {
    const float step = 0.1f; // screen-space pan
    if (key == 'W') g_PanY += step;
    if (key == 'S') g_PanY -= step;
    if (key == 'A') g_PanX -= step;
    if (key == 'D') g_PanX += step;
}

static void UpdateWalkingAndAnimation() {
    const float dt = 1.0f / 60.0f;

    // 移动方向（基于按键）
    float dx = (g_KeyRight ? 1.0f : 0.0f) + (g_KeyLeft ? -1.0f : 0.0f);
    float dz = (g_KeyUp ? 1.0f : 0.0f) + (g_KeyDown ? -1.0f : 0.0f);
    float len = std::sqrt(dx * dx + dz * dz);
    if (len > 0.0f) { dx /= len; dz /= len; }

    // 世界移动
    const float moveSpeed = 1.2f; // units/sec
    g_CharPosX += dx * moveSpeed * dt;
    g_CharPosZ += dz * moveSpeed * dt;

    // 面朝移动方向：heading = atan2(dx, dz)（+Z 为正前）
    if (len > 0.0f) {
        float targetHeadingDeg = std::atan2(dx, dz) * (180.0f / kPI);
        float s = 12.0f * dt; // 转身速度
        float diff = targetHeadingDeg - g_HeadingDeg;
        while (diff > 180.0f) diff -= 360.0f;
        while (diff < -180.0f) diff += 360.0f;
        g_HeadingDeg += diff * s;
    }

    // 步频随速度变化
    float speedScalar = len;                    // 0..1
    float cadenceHz = 1.9f + 0.7f * speedScalar; // ~1.9–2.6 步/秒
    float phaseRate = kTAU * cadenceHz;        // rad/sec

    if (len > 0.0f) {
        g_WalkPhase = std::fmod(g_WalkPhase + phaseRate * dt, kTAU);
        g_WalkBlend += 3.0f * dt; if (g_WalkBlend > 1.0f) g_WalkBlend = 1.0f;
    }
    else {
        g_WalkBlend -= 3.0f * dt; if (g_WalkBlend < 0.0f) g_WalkBlend = 0.0f;
    }

    InvalidateRect(g_hWnd, nullptr, FALSE);
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

        // Load model
        g_ModelLoaded = loadOBJ(kModelPath, g_Model);
        if (g_ModelLoaded) {
            chooseHeadGroupAndPivot();
            chooseShoeGroups();
            detectLegChainOneSide(true, g_LeftLeg);
            detectLegChainOneSide(false, g_RightLeg);
            calcAutoScaleAndCam();
            ResetAll();
        }
        // 60Hz timer for animation & movement
        SetTimer(hWnd, 1, 16, nullptr);
        return 0;

    case WM_SIZE:
        resizeGL(LOWORD(lParam), HIWORD(lParam));
        return 0;

    case WM_TIMER:
        if (wParam == 1) {
            UpdateWalkingAndAnimation();
        }
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
            // Camera pan
        case 'W': case 'S': case 'A': case 'D':
            OnPanKey(WORD(wParam));
            InvalidateRect(hWnd, nullptr, FALSE);
            break;

            // Walk (world movement) — key states
        case VK_LEFT:  g_KeyLeft = true;  break;
        case VK_RIGHT: g_KeyRight = true;  break;
        case VK_UP:    g_KeyUp = true;  break;
        case VK_DOWN:  g_KeyDown = true;  break;

            // Face rotation (optional)
        case 'J': g_FaceYawDeg -= 5.0f; InvalidateRect(hWnd, nullptr, FALSE); break;
        case 'K': g_FaceYawDeg += 5.0f; InvalidateRect(hWnd, nullptr, FALSE); break;
        case 'I': g_FacePitchDeg += 5.0f; InvalidateRect(hWnd, nullptr, FALSE); break;
        case 'M': g_FacePitchDeg -= 5.0f; InvalidateRect(hWnd, nullptr, FALSE); break;
        case 'U': g_FaceRollDeg -= 5.0f; InvalidateRect(hWnd, nullptr, FALSE); break;
        case 'O': g_FaceRollDeg += 5.0f; InvalidateRect(hWnd, nullptr, FALSE); break;

            // Wireframe/Fill
        case VK_F1:
            g_Wireframe = !g_Wireframe;
            applyPolygonMode();
            InvalidateRect(hWnd, nullptr, FALSE);
            break;

            // Shoes toggle (Q)
        case 'Q':
            if (!g_ShoeGroupIds.empty()) {
                g_ShoesOn = !g_ShoesOn;
                InvalidateRect(hWnd, nullptr, FALSE);
            }
            break;

            // Reset
        case 'R':
            ResetAll();
            InvalidateRect(hWnd, nullptr, FALSE);
            break;

            // Exit
        case VK_ESCAPE:
            PostQuitMessage(0);
            break;

        default: break;
        }
        return 0;

    case WM_KEYUP:
        switch (wParam) {
        case VK_LEFT:  g_KeyLeft = false; break;
        case VK_RIGHT: g_KeyRight = false; break;
        case VK_UP:    g_KeyUp = false; break;
        case VK_DOWN:  g_KeyDown = false; break;
        default: break;
        }
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps; BeginPaint(hWnd, &ps);
        if (g_ModelLoaded) renderScene();
        SwapBuffers(g_hDC);
        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_DESTROY:
        KillTimer(hWnd, 1);
        if (g_hRC) { wglMakeCurrent(nullptr, nullptr); wglDeleteContext(g_hRC); g_hRC = nullptr; }
        if (g_hDC) { ReleaseDC(hWnd, g_hDC); g_hDC = nullptr; }
        PostQuitMessage(0);
        return 0;

    default: break;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nCmdShow) {
    WNDCLASS wc = {};
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = "MulanWin32GL_WalkingLegs";
    RegisterClass(&wc);

    RECT r = { 0,0,1280,720 };
    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);

    HWND hWnd = CreateWindow(
        wc.lpszClassName, kWindowTitle,
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
