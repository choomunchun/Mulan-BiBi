
#include <Windows.h>
#include <gl/GL.h>
#include <gl/GLU.h>

#pragma comment (lib, "OpenGL32.lib")
#pragma comment (lib, "Glu32.lib")

#define WINDOW_TITLE "OpenGL Window"

// --------------------------------------------------------------
// Slim torso + neck + GLU sphere head. Torso (16 seg * 16 torso/neck bands) + sphere.
// Head rendered with gluSphere for perfect roundness and easy LOD changes.
// No for-loops: vertices are emitted via macro-expanded glVertex3f calls.
// --------------------------------------------------------------

// Joint pose (animation friendly)
struct JointPose {
	float torsoYaw;  // Y-axis
	float torsoPitch; // X-axis
	float torsoRoll; // Z-axis
	float headYaw;
	float headPitch;
	float headRoll;
} g_pose = {0,0,0, 0,0,0};

// Cos/Sin pairs for 16 segments (22.5 degrees step)
#define C0  1.0000000f
#define S0  0.0000000f
#define C1  0.9238795f
#define S1  0.3826834f
#define C2  0.7071068f
#define S2  0.7071068f
#define C3  0.3826834f
#define S3  0.9238795f
#define C4  0.0000000f
#define S4  1.0000000f
#define C5 -0.3826834f
#define S5  0.9238795f
#define C6 -0.7071068f
#define S6  0.7071068f
#define C7 -0.9238795f
#define S7  0.3826834f
#define C8 -1.0000000f
#define S8  0.0000000f
#define C9 -0.9238795f
#define S9 -0.3826834f
#define C10 -0.7071068f
#define S10 -0.7071068f
#define C11 -0.3826834f
#define S11 -0.9238795f
#define C12  0.0000000f
#define S12 -1.0000000f
#define C13  0.3826834f
#define S13 -0.9238795f
#define C14  0.7071068f
#define S14 -0.7071068f
#define C15  0.9238795f
#define S15 -0.3826834f

// Radii & heights: slimmer torso (R0-R15), neck (R15-R18). Head is separate sphere now.
#define R0  0.38f
#define R1  0.37f
#define R2  0.36f
#define R3  0.35f
#define R4  0.34f
#define R5  0.32f
#define R6  0.30f
#define R7  0.28f
#define R8  0.27f
#define R9  0.28f
#define R10 0.29f
#define R11 0.30f
#define R12 0.29f
#define R13 0.27f
#define R14 0.24f
#define R15 0.21f
#define R16 0.19f // shoulder base
#define R17 0.16f // slimmer lower neck
#define R18 0.133f // upper neck (matches sphere slice radius for seamless join)
// Head handled by GLU sphere (radius defined below)

#define Y0  0.00f
#define Y1  0.05f
#define Y2  0.10f
#define Y3  0.15f
#define Y4  0.20f
#define Y5  0.28f
#define Y6  0.36f
#define Y7  0.44f
#define Y8  0.52f
#define Y9  0.60f
#define Y10 0.68f
#define Y11 0.76f
#define Y12 0.82f
#define Y13 0.88f
#define Y14 0.92f
#define Y15 0.96f
#define Y16 0.98f
#define Y17 1.04f
#define Y18 1.10f
// Head sphere center & radius (kept separate for GLU) -> spans roughly 1.10f to 1.58f
#define HEAD_CENTER_Y 1.30f // lowered for seamless neck join after clipping
#define HEAD_RADIUS   0.24f

// Macro to output two triangles (quad) between two rings for one segment pair
#define QUAD(r1,y1,r2,y2, cA,sA, cB,sB) \
	glVertex3f((r1)*(cA), (y1), (r1)*(sA)); \
	glVertex3f((r2)*(cA), (y2), (r2)*(sA)); \
	glVertex3f((r2)*(cB), (y2), (r2)*(sB)); \
	glVertex3f((r1)*(cA), (y1), (r1)*(sA)); \
	glVertex3f((r2)*(cB), (y2), (r2)*(sB)); \
	glVertex3f((r1)*(cB), (y1), (r1)*(sB));

// Emit all quads for one vertical band between ring A and B (16 segments)
#define BAND(rA,yA,rB,yB) \
	QUAD(rA,yA,rB,yB, C0,S0,  C1,S1) \
	QUAD(rA,yA,rB,yB, C1,S1,  C2,S2) \
	QUAD(rA,yA,rB,yB, C2,S2,  C3,S3) \
	QUAD(rA,yA,rB,yB, C3,S3,  C4,S4) \
	QUAD(rA,yA,rB,yB, C4,S4,  C5,S5) \
	QUAD(rA,yA,rB,yB, C5,S5,  C6,S6) \
	QUAD(rA,yA,rB,yB, C6,S6,  C7,S7) \
	QUAD(rA,yA,rB,yB, C7,S7,  C8,S8) \
	QUAD(rA,yA,rB,yB, C8,S8,  C9,S9) \
	QUAD(rA,yA,rB,yB, C9,S9,  C10,S10) \
	QUAD(rA,yA,rB,yB, C10,S10, C11,S11) \
	QUAD(rA,yA,rB,yB, C11,S11, C12,S12) \
	QUAD(rA,yA,rB,yB, C12,S12, C13,S13) \
	QUAD(rA,yA,rB,yB, C13,S13, C14,S14) \
	QUAD(rA,yA,rB,yB, C14,S14, C15,S15) \
	QUAD(rA,yA,rB,yB, C15,S15, C0,S0)

// Eye circle 12-step cos/sin (30 deg steps)
#define EC0  1.0000000f
#define ES0  0.0000000f
#define EC1  0.8660254f
#define ES1  0.5000000f
#define EC2  0.5000000f
#define ES2  0.8660254f
#define EC3  0.0000000f
#define ES3  1.0000000f
#define EC4 -0.5000000f
#define ES4  0.8660254f
#define EC5 -0.8660254f
#define ES5  0.5000000f
#define EC6 -1.0000000f
#define ES6  0.0000000f
#define EC7 -0.8660254f
#define ES7 -0.5000000f
#define EC8 -0.5000000f
#define ES8 -0.8660254f
#define EC9  0.0000000f
#define ES9 -1.0000000f
#define EC10 0.5000000f
#define ES10 -0.8660254f
#define EC11 0.8660254f
#define ES11 -0.5000000f

// Arrays for animation-friendly access (torso + neck rings only)
static const float torsoRadii[] = {R0,R1,R2,R3,R4,R5,R6,R7,R8,R9,R10,R11,R12,R13,R14,R15,R16,R17,R18};
static const float torsoHeights[] = {Y0,Y1,Y2,Y3,Y4,Y5,Y6,Y7,Y8,Y9,Y10,Y11,Y12,Y13,Y14,Y15,Y16,Y17,Y18};
static const int   torsoRingCount = sizeof(torsoRadii)/sizeof(torsoRadii[0]);

static const float segCos[16] = {C0,C1,C2,C3,C4,C5,C6,C7,C8,C9,C10,C11,C12,C13,C14,C15};
static const float segSin[16] = {S0,S1,S2,S3,S4,S5,S6,S7,S8,S9,S10,S11,S12,S13,S14,S15};

// Sample a torso vertex (ring index, segment index) into out[3]
static void getTorsoVertex(int ring, int seg, float out[3])
{
	if(ring < 0 || ring >= torsoRingCount) { out[0]=out[1]=out[2]=0; return; }
	seg &= 15; // wrap
	float r = torsoRadii[ring];
	out[0] = r * segCos[seg];
	out[1] = torsoHeights[ring];
	out[2] = r * segSin[seg];
}

static void drawFaceFeatures()
{
	// Eyes
	const float eyeY = HEAD_CENTER_Y + HEAD_RADIUS*0.05f; // recomputed after head center change
	const float eyeZ = HEAD_RADIUS * 0.86f; // front of sphere
	const float eyeR = 0.026f;
	glColor3f(1.0f,1.0f,1.0f);
	glBegin(GL_LINE_LOOP); // left eye
		glVertex3f(-0.09f + eyeR*EC0, eyeY, eyeZ + eyeR*ES0);
		glVertex3f(-0.09f + eyeR*EC1, eyeY, eyeZ + eyeR*ES1);
		glVertex3f(-0.09f + eyeR*EC2, eyeY, eyeZ + eyeR*ES2);
		glVertex3f(-0.09f + eyeR*EC3, eyeY, eyeZ + eyeR*ES3);
		glVertex3f(-0.09f + eyeR*EC4, eyeY, eyeZ + eyeR*ES4);
		glVertex3f(-0.09f + eyeR*EC5, eyeY, eyeZ + eyeR*ES5);
		glVertex3f(-0.09f + eyeR*EC6, eyeY, eyeZ + eyeR*ES6);
		glVertex3f(-0.09f + eyeR*EC7, eyeY, eyeZ + eyeR*ES7);
		glVertex3f(-0.09f + eyeR*EC8, eyeY, eyeZ + eyeR*ES8);
		glVertex3f(-0.09f + eyeR*EC9, eyeY, eyeZ + eyeR*ES9);
		glVertex3f(-0.09f + eyeR*EC10, eyeY, eyeZ + eyeR*ES10);
		glVertex3f(-0.09f + eyeR*EC11, eyeY, eyeZ + eyeR*ES11);
	glEnd();
	glBegin(GL_LINE_LOOP); // right eye
		glVertex3f(0.09f + eyeR*EC0, eyeY, eyeZ + eyeR*ES0);
		glVertex3f(0.09f + eyeR*EC1, eyeY, eyeZ + eyeR*ES1);
		glVertex3f(0.09f + eyeR*EC2, eyeY, eyeZ + eyeR*ES2);
		glVertex3f(0.09f + eyeR*EC3, eyeY, eyeZ + eyeR*ES3);
		glVertex3f(0.09f + eyeR*EC4, eyeY, eyeZ + eyeR*ES4);
		glVertex3f(0.09f + eyeR*EC5, eyeY, eyeZ + eyeR*ES5);
		glVertex3f(0.09f + eyeR*EC6, eyeY, eyeZ + eyeR*ES6);
		glVertex3f(0.09f + eyeR*EC7, eyeY, eyeZ + eyeR*ES7);
		glVertex3f(0.09f + eyeR*EC8, eyeY, eyeZ + eyeR*ES8);
		glVertex3f(0.09f + eyeR*EC9, eyeY, eyeZ + eyeR*ES9);
		glVertex3f(0.09f + eyeR*EC10, eyeY, eyeZ + eyeR*ES10);
		glVertex3f(0.09f + eyeR*EC11, eyeY, eyeZ + eyeR*ES11);
	glEnd();
	// Nose (simple triangle)
	glColor3f(1.0f,0.8f,0.6f);
	glBegin(GL_LINE_LOOP); // nose (simple triangle)
		glVertex3f(0.0f,       eyeY+0.02f, eyeZ+0.00f);
		glVertex3f(-0.015f,    eyeY-0.04f, eyeZ+0.00f);
		glVertex3f(0.015f,     eyeY-0.04f, eyeZ+0.00f);
	glEnd();
	// Mouth (curve)
	glColor3f(1.0f,0.2f,0.2f);
	glBegin(GL_LINE_STRIP); // mouth
		glVertex3f(-0.08f, eyeY-0.10f, eyeZ+0.00f);
		glVertex3f(-0.05f, eyeY-0.12f, eyeZ+0.00f);
		glVertex3f(-0.02f, eyeY-0.13f, eyeZ+0.00f);
		glVertex3f( 0.02f, eyeY-0.13f, eyeZ+0.00f);
		glVertex3f( 0.05f, eyeY-0.12f, eyeZ+0.00f);
		glVertex3f( 0.08f, eyeY-0.10f, eyeZ+0.00f);
	glEnd();
}

static GLUquadric* g_headQuadric = nullptr;

static void drawHeadSphere()
{
	if(!g_headQuadric) return; // safety
	glPushMatrix();
	// Clip plane: keep y >= Y18 to slice sphere flush with neck top
	GLdouble eq[4] = {0.0, 1.0, 0.0, -Y18};
	glEnable(GL_CLIP_PLANE0);
	glClipPlane(GL_CLIP_PLANE0, eq);
	glTranslatef(0.0f, HEAD_CENTER_Y, 0.0f);
	gluSphere(g_headQuadric, HEAD_RADIUS, 32, 24); // smoother sphere
	glDisable(GL_CLIP_PLANE0);
	glPopMatrix();
}

static void drawTorso()
{
	glColor3f(0.2f, 0.8f, 0.3f);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // wireframe
	glLineWidth(1.0f);
	glBegin(GL_TRIANGLES); // torso+neck only now (16 segments * 18 bands *2 = 576 tris)
		BAND(R0, Y0, R1, Y1);
		BAND(R1, Y1, R2, Y2);
		BAND(R2, Y2, R3, Y3);
		BAND(R3, Y3, R4, Y4);
		BAND(R4, Y4, R5, Y5);
		BAND(R5, Y5, R6, Y6);
		BAND(R6, Y6, R7, Y7);
		BAND(R7, Y7, R8, Y8);
		BAND(R8, Y8, R9, Y9);
		BAND(R9, Y9, R10, Y10);
		BAND(R10, Y10, R11, Y11);
		BAND(R11, Y11, R12, Y12);
		BAND(R12, Y12, R13, Y13);
		BAND(R13, Y13, R14, Y14);
		BAND(R14, Y14, R15, Y15);
		BAND(R15, Y15, R16, Y16);
		BAND(R16, Y16, R17, Y17); // neck lower
		BAND(R17, Y17, R18, Y18); // neck upper
	glEnd();
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // restore (optional)
    drawHeadSphere(); // sphere (not rotated with head to preserve seamless neck clip)
}


LRESULT WINAPI WindowProcedure(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
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

void display()
{
//--------------------------------
//	OpenGL drawing
//--------------------------------
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.0f, -0.9f, -3.2f); // adjust for new head sphere center
	// Apply torso orientation (YXZ order)
	glRotatef(g_pose.torsoYaw,   0,1,0);
	glRotatef(g_pose.torsoPitch, 1,0,0);
	glRotatef(g_pose.torsoRoll,  0,0,1);
	drawTorso();
	// Rotate facial features around head center (independent head joint)
	glPushMatrix();
	glTranslatef(0.0f, HEAD_CENTER_Y, 0.0f);
	glRotatef(g_pose.headYaw,   0,1,0);
	glRotatef(g_pose.headPitch, 1,0,0);
	glRotatef(g_pose.headRoll,  0,0,1);
	glTranslatef(0.0f, -HEAD_CENTER_Y, 0.0f);
	drawFaceFeatures();
	glPopMatrix();
//--------------------------------
//	End of OpenGL drawing
//--------------------------------
}
//--------------------------------------------------------------------

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nCmdShow)
{
	WNDCLASSEX wc;
	ZeroMemory(&wc, sizeof(WNDCLASSEX));

	wc.cbSize			= sizeof(WNDCLASSEX);
	wc.hInstance		= GetModuleHandle(NULL);
	wc.lpfnWndProc		= WindowProcedure;
	wc.lpszClassName	= WINDOW_TITLE;
    wc.style			= CS_HREDRAW | CS_VREDRAW;

	if (!RegisterClassEx(&wc)) return false;

	HWND hWnd = CreateWindow(	WINDOW_TITLE, WINDOW_TITLE, WS_OVERLAPPEDWINDOW,
								CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
								NULL, NULL, wc.hInstance, NULL );
	
	//--------------------------------
	//	Initialize window for OpenGL
	//--------------------------------

	HDC hdc = GetDC(hWnd);

	//	initialize pixel format for the window
	initPixelFormat(hdc);

	//	get an openGL context
	HGLRC hglrc = wglCreateContext(hdc);

	//	make context current
	if (!wglMakeCurrent(hdc, hglrc)) return false;

	// Basic GL state
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	// Simple perspective frustum
	glFrustum(-0.6, 0.6, -0.45, 0.45, 1.0, 10.0);
	glMatrixMode(GL_MODELVIEW);

	// Create quadric for head sphere
	g_headQuadric = gluNewQuadric();
	gluQuadricDrawStyle(g_headQuadric, GLU_LINE); // wireframe sphere to match torso

	//--------------------------------
	//	End initialization
	//--------------------------------

	ShowWindow(hWnd, nCmdShow);
	
	MSG msg;
	ZeroMemory(&msg, sizeof(msg));

	while(true)
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

	if(g_headQuadric) { gluDeleteQuadric(g_headQuadric); g_headQuadric = nullptr; }
	UnregisterClass(WINDOW_TITLE, wc.hInstance);
	
	return true;
}
//--------------------------------------------------------------------
