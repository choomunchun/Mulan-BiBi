

#include <Windows.h>
#include <gl/GL.h>
#include <gl/GLU.h>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>


struct Vec3 {
    float x, y, z;
};
float gRotation = 0;
std::vector<Vec3> gVertices;
std::vector<std::vector<int>> gFaces; // indices of vertices




#pragma comment (lib, "OpenGL32.lib")


#define WINDOW_TITLE "OpenGL Window"






LRESULT WINAPI WindowProcedure(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;


    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) PostQuitMessage(0);
        break;


    default:
        break;
    }


    return DefWindowProc(hWnd, msg, wParam, lParam);
}
//--------------------------------------------------------------------


bool initPixelFormat(HDC hdc)
{
    PIXELFORMATDESCRIPTOR pfd;
    ZeroMemory(&pfd, sizeof(PIXELFORMATDESCRIPTOR));


    pfd.cAlphaBits = 8;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 0;


    pfd.dwFlags = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;


    pfd.iLayerType = PFD_MAIN_PLANE;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;


    // choose pixel format returns the number most similar pixel format available
    int n = ChoosePixelFormat(hdc, &pfd);


    // set pixel format returns whether it sucessfully set the pixel format
    if (SetPixelFormat(hdc, n, &pfd))
    {
        return true;
    }
    else
    {
        return false;
    }
}
//--------------------------------------------------------------------
bool loadOBJ(const char* filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return false;


    std::string line;
    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string type;
        ss >> type;


        if (type == "v") {
            Vec3 v;
            ss >> v.x >> v.y >> v.z;
            gVertices.push_back(v);
        }
        else if (type == "f") {
            std::vector<int> face;
            std::string token;
            while (ss >> token) {
                // "f v/vt/vn" format → take only v
                int vIndex = std::stoi(token.substr(0, token.find('/')));
                face.push_back(vIndex - 1); // OBJ is 1-based
            }
            gFaces.push_back(face);
        }
    }
    return true;
}


void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);


    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, 800.0 / 600.0, 0.1, 100.0);
   
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0, 15, 40,   // camera position
        0, 15, 0,   // look at center
        0, 1, 0);  // up vector


    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // wireframe
   
    glColor3f(1, 1, 1); // white lines
    glRotatef(gRotation, 0.0f, 1.0f, 0.0f);
    // Draw faces
    for (auto& face : gFaces) {
        glBegin(GL_POLYGON);
        for (int idx : face) {
            Vec3 v = gVertices[idx];
            glVertex3f(v.x, v.y, v.z);
        }
        glEnd();
    }
    gRotation += 0.01f;
    if (gRotation >= 360.0f) gRotation -= 360.0f;
}


//--------------------------------------------------------------------


int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nCmdShow)
{
    WNDCLASSEX wc;
    ZeroMemory(&wc, sizeof(WNDCLASSEX));


    wc.cbSize = sizeof(WNDCLASSEX);
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpfnWndProc = WindowProcedure;
    wc.lpszClassName = WINDOW_TITLE;
    wc.style = CS_HREDRAW | CS_VREDRAW;


    if (!RegisterClassEx(&wc)) return false;


    HWND hWnd = CreateWindow(WINDOW_TITLE, WINDOW_TITLE, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL, NULL, wc.hInstance, NULL);


    //--------------------------------
    //  Initialize window for OpenGL
    //--------------------------------


    HDC hdc = GetDC(hWnd);


    //  initialize pixel format for the window
    initPixelFormat(hdc);


    //  get an openGL context
    HGLRC hglrc = wglCreateContext(hdc);


    //  make context current
    if (!wglMakeCurrent(hdc, hglrc)) return false;


    //--------------------------------
    //  End initialization
    //--------------------------------
    if (!loadOBJ("mulan5.obj")) {
        MessageBox(NULL, "Failed to load OBJ file!", "Error", MB_OK);
        return -1;
    }


    ShowWindow(hWnd, nCmdShow);


    MSG msg;
    ZeroMemory(&msg, sizeof(msg));


    while (true)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT) break;


            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }


        display();


        SwapBuffers(hdc);
    }


    UnregisterClass(WINDOW_TITLE, wc.hInstance);


    return true;
}
//--------------------------------------------------------------------

