#include <Windows.h>
#include <windowsx.h>
#include <gl/GL.h>
#include <gl/GLU.h>
#include <cmath>

#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "Glu32.lib")

// --- Global Variables ---
HDC   ghDC = NULL;
HGLRC ghRC = NULL;
HWND  ghWnd = NULL;

bool isDragging = false;
float rotationX = 20.0f;
float rotationY = 0.0f;
int lastMouseX, lastMouseY;
float zoom = 10.0f; // For zooming with the mouse wheel

const float PI = 3.14159265358979323846f;

// --- Struct for Material Properties ---
struct Material {
    GLfloat ambient[4];
    GLfloat diffuse[4];
    GLfloat specular[4];
    GLfloat shininess;
};

// Define materials for our sword parts
Material polishedSteel = {
    {0.23f, 0.23f, 0.23f, 1.0f}, {0.77f, 0.77f, 0.77f, 1.0f},
    {0.99f, 0.99f, 0.99f, 1.0f}, 100.0f
};
Material darkJade = {
    {0.0f, 0.1f, 0.05f, 1.0f}, {0.1f, 0.35f, 0.2f, 1.0f},
    {0.45f, 0.55f, 0.45f, 1.0f}, 32.0f
};
Material hiltWrap = {
    {0.08f, 0.05f, 0.05f, 1.0f}, {0.2f, 0.15f, 0.15f, 1.0f},
    {0.1f, 0.1f, 0.1f, 1.0f}, 10.0f
};

// Helper function to apply a material
void setMaterial(const Material& mat) {
    glMaterialfv(GL_FRONT, GL_AMBIENT, mat.ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, mat.diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat.specular);
    glMaterialf(GL_FRONT, GL_SHININESS, mat.shininess);
}

// --- Forward Declarations ---
LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
void initGL();
void renderScene();

// --- Helper and Drawing Functions (Unchanged from "Mulan" version) ---

void drawSphere(double r, int lats, int longs) { 
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
void drawCylinder(double r, double h, int slices) {
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
void drawInscription() {
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
void drawBlade() {
    setMaterial(polishedSteel);
    float bladeLength = 5.0f; float bladeWidth = 0.2f; float bladeThickness = 0.05f;
    glBegin(GL_QUADS);
    glNormal3f(0.5f, 0.125f, 0.0f);
    glVertex3f(0.0f, 0.0f, bladeThickness); glVertex3f(bladeWidth, 0.0f, 0.0f); glVertex3f(bladeWidth, bladeLength, 0.0f); glVertex3f(0.0f, bladeLength, bladeThickness);
    glNormal3f(0.5f, -0.125f, 0.0f);
    glVertex3f(bladeWidth, 0.0f, 0.0f); glVertex3f(0.0f, 0.0f, -bladeThickness); glVertex3f(0.0f, bladeLength, -bladeThickness); glVertex3f(bladeWidth, bladeLength, 0.0f);
    glNormal3f(-0.5f, 0.125f, 0.0f);
    glVertex3f(0.0f, 0.0f, bladeThickness); glVertex3f(-bladeWidth, 0.0f, 0.0f); glVertex3f(-bladeWidth, bladeLength, 0.0f); glVertex3f(0.0f, bladeLength, bladeThickness);
    glNormal3f(-0.5f, -0.125f, 0.0f);
    glVertex3f(-bladeWidth, 0.0f, 0.0f); glVertex3f(0.0f, 0.0f, -bladeThickness); glVertex3f(0.0f, bladeLength, -bladeThickness); glVertex3f(-bladeWidth, bladeLength, 0.0f);
    glEnd();
    glBegin(GL_TRIANGLES);
    float tipLength = 0.5f;
    glNormal3f(0.5f, 0.5f, 0.0f); glVertex3f(0.0f, bladeLength + tipLength, 0.0f); glVertex3f(bladeWidth, bladeLength, 0.0f); glVertex3f(0.0f, bladeLength, bladeThickness);
    glNormal3f(0.5f, -0.5f, 0.0f); glVertex3f(0.0f, bladeLength + tipLength, 0.0f); glVertex3f(0.0f, bladeLength, -bladeThickness); glVertex3f(bladeWidth, bladeLength, 0.0f);
    glNormal3f(-0.5f, 0.5f, 0.0f); glVertex3f(0.0f, bladeLength + tipLength, 0.0f); glVertex3f(-bladeWidth, bladeLength, 0.0f); glVertex3f(0.0f, bladeLength, bladeThickness);
    glNormal3f(-0.5f, -0.5f, 0.0f); glVertex3f(0.0f, bladeLength + tipLength, 0.0f); glVertex3f(0.0f, bladeLength, -bladeThickness); glVertex3f(-bladeWidth, bladeLength, 0.0f);
    glEnd();
    drawInscription();
}
void drawGuard() {
    setMaterial(darkJade);
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
void drawHilt() {
    setMaterial(hiltWrap);
    drawCylinder(0.1f, 1.2f, 20);
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
void drawPommel() {
    setMaterial(darkJade);
    glPushMatrix();
    glScalef(1.0f, 0.7f, 1.0f);
    drawSphere(0.18f, 30, 30);
    glPopMatrix();
}
void drawTassel() {
    glPushMatrix();
    glTranslatef(0.0f, -0.05f, 0.0f);
    setMaterial(darkJade);
    drawSphere(0.05f, 20, 20);
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

// --- Main Rendering and Initialization Functions ---

void renderScene() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // CHANGED: Camera is now centered on the model's approximate center
    gluLookAt(0.0, 1.5, zoom,   // Eye position
              0.0, 1.5, 0.0,    // Look at position (center of model)
              0.0, 1.0, 0.0);   // Up vector

    // Define light source position
    GLfloat light_pos[] = { 4.0f, 8.0f, 10.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
    
    // Create a matrix for the sword model
    glPushMatrix();

    // Apply user-controlled rotation
    glRotatef(rotationX, 1.0f, 0.0f, 0.0f);
    glRotatef(rotationY, 0.0f, 1.0f, 0.0f);
    
    // CHANGED: Translate the sword so its center is at the world origin (0,0,0)
    // before rotation is applied. This makes it rotate around its center.
    glTranslatef(0.0, -3.0, 0.0);

    // Render sword components
    drawBlade();
    drawGuard();
    glPushMatrix(); glTranslatef(0.0, -1.2, 0.0); drawHilt(); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0, -1.3, 0.0); drawPommel(); drawTassel(); glPopMatrix();
    
    glPopMatrix(); // End of sword matrix

    SwapBuffers(ghDC);
}

void initGL() {
    // CHANGED: Neutral dark gray background
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    // REMOVED: Blending for shadows is no longer needed
    // glEnable(GL_BLEND);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GLfloat light_ambient[] = { 0.3f, 0.3f, 0.3f, 1.0f };
    GLfloat light_diffuse[] = { 0.9f, 0.9f, 0.9f, 1.0f };
    GLfloat light_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);

    glShadeModel(GL_SMOOTH);
}

// --- Win32 Application Structure (Unchanged) ---

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASS wc; MSG msg; BOOL quit = FALSE;
    wc.style = CS_OWNDC; wc.lpfnWndProc = WindowProc; wc.cbClsExtra = 0; wc.cbWndExtra = 0;
    wc.hInstance = hInstance; wc.hIcon = LoadIcon(NULL, IDI_APPLICATION); wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH); wc.lpszMenuName = NULL; wc.lpszClassName = "GLWindowClass";
    RegisterClass(&wc);
    ghWnd = CreateWindow("GLWindowClass", "Sword Model Viewer", WS_CAPTION | WS_POPUPWINDOW | WS_VISIBLE, 0, 0, 800, 600, NULL, NULL, hInstance, NULL);
    while (!quit) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) { quit = TRUE; }
            else { TranslateMessage(&msg); DispatchMessage(&msg); }
        } else { renderScene(); }
    }
    return msg.wParam;
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            ghDC = GetDC(hWnd);
            PIXELFORMATDESCRIPTOR pfd; ZeroMemory(&pfd, sizeof(pfd));
            pfd.nSize = sizeof(pfd); pfd.nVersion = 1; pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
            pfd.iPixelType = PFD_TYPE_RGBA; pfd.cColorBits = 32; pfd.cDepthBits = 16; pfd.iLayerType = PFD_MAIN_PLANE;
            int format = ChoosePixelFormat(ghDC, &pfd);
            SetPixelFormat(ghDC, format, &pfd);
            ghRC = wglCreateContext(ghDC);
            wglMakeCurrent(ghDC, ghRC);
            initGL();
            return 0;
        }
        case WM_CLOSE: PostQuitMessage(0); return 0;
        case WM_DESTROY: wglMakeCurrent(NULL, NULL); wglDeleteContext(ghRC); ReleaseDC(hWnd, ghDC); return 0;
        case WM_SIZE: {
            int width = LOWORD(lParam); int height = HIWORD(lParam);
            glViewport(0, 0, width, height); glMatrixMode(GL_PROJECTION); glLoadIdentity();
            gluPerspective(45.0, (double)width / (double)height, 1.0, 100.0); glMatrixMode(GL_MODELVIEW);
            return 0;
        }
        case WM_LBUTTONDOWN:
            isDragging = true; lastMouseX = GET_X_LPARAM(lParam); lastMouseY = GET_Y_LPARAM(lParam);
            SetCapture(hWnd); return 0;
        case WM_LBUTTONUP:
            isDragging = false; ReleaseCapture(); return 0;
        case WM_MOUSEMOVE:
            if (isDragging) {
                int currentX = GET_X_LPARAM(lParam); int currentY = GET_Y_LPARAM(lParam);
                rotationY += (currentX - lastMouseX); rotationX += (currentY - lastMouseY);
                lastMouseX = currentX; lastMouseY = currentY;
            }
            return 0;
        case WM_MOUSEWHEEL:
            zoom -= (float)GET_WHEEL_DELTA_WPARAM(wParam) / 120.0f * 0.5f;
            if (zoom < 2.0f) zoom = 2.0f;
            if (zoom > 30.0f) zoom = 30.0f;
            return 0;
        default:
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
}