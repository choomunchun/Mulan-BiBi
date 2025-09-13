#include "armor.h"
#include <Windows.h>
#include <gl/GL.h>
#include "utils.h"

extern float* segCos;
extern float* segSin;

void setArmorMaterial() {
    glColor3f(0.75f, 0.75f, 0.8f);
    glShadeModel(GL_SMOOTH);
}

void drawHelmet() {
    setArmorMaterial();
    const int segments = 20;
    const int layers = 16;
    const float radius = HEAD_RADIUS * 1.2f;

    glPushMatrix();
    for (int i = 0; i < layers / 2; i++) {
        float lat0 = PI * (-0.5f + (float)(i) / layers);
        float lat1 = PI * (-0.5f + (float)(i + 1) / layers);
        if (sinf(lat0) < 0) continue;

        float r0 = radius * cosf(lat0);
        float r1 = radius * cosf(lat1);
        float y0 = radius * sinf(lat0);
        float y1 = radius * sinf(lat1);

        glBegin(GL_QUAD_STRIP);
        for (int j = 0; j <= segments; j++) {
            float lng = 2.0f * PI * (float)j / segments;
            float x = cosf(lng);
            float z = sinf(lng);

            glNormal3f(x * r1, y1, z * r1);
            glVertex3f(x * r1, y1, z * r1);
            glNormal3f(x * r0, y0, z * r0);
            glVertex3f(x * r0, y0, z * r0);
        }
        glEnd();
    }
    glPopMatrix();
}

void drawArmor() {
    setArmorMaterial();
    glPushMatrix();
    glScalef(1.05f, 1.1f, 1.05f);

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
    glEnd();

    glPopMatrix();
}

void drawUpperLegArmor() { // Draws Thigh Guard
    setArmorMaterial();
    const int legSegments = 40;
    const float legScale = 0.7f * 1.15f;

    // Knee piece now starts lower to overlap the shin guard
    std::vector<Vec3f> kneeTop = generateRing(0.0f, 4.2f, 0.2f, 0.8f * legScale, 0.7f * legScale, legSegments);
    std::vector<Vec3f> upperThigh = generateRing(0.0f, 8.5f, -0.1f, 1.0f * legScale, 0.9f * legScale, legSegments);

    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= legSegments; i++) {
        int k = i % legSegments;
        Vec3f n = normalize({ upperThigh[k].x, 0, upperThigh[k].z });
        glNormal3f(n.x, n.y, n.z);
        glVertex3f(kneeTop[k].x, kneeTop[k].y, kneeTop[k].z);
        glVertex3f(upperThigh[k].x, upperThigh[k].y, upperThigh[k].z);
    }
    glEnd();

    // Knee Cop (the main knee bulge)
    // MODIFIED: Made the knee piece larger (from 4.5 to 5.0) to ensure overlap
    std::vector<Vec3f> kneeRing2 = generateRing(0.0f, 5.0f, 0.3f, 0.85f * legScale, 0.75f * legScale, legSegments);

    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= legSegments; i++) {
        int k = i % legSegments;
        Vec3f n = normalize({ kneeRing2[k].x, 0, kneeRing2[k].z });
        glNormal3f(n.x, n.y, n.z);
        glVertex3f(kneeRing2[k].x, kneeRing2[k].y, kneeRing2[k].z);
        glVertex3f(kneeTop[k].x, kneeTop[k].y, kneeTop[k].z);
    }
    glEnd();
}

void drawLowerLegArmor() { // Draws Shin Guard
    setArmorMaterial();
    const int legSegments = 40;
    const float legScale = 0.8f * 1.10f;

    std::vector<Vec3f> ankle = generateRing(0.0f, 0.5f, -0.3f, 0.5f * legScale, 0.4f * legScale, legSegments);
    // MODIFIED: Increased Y-value from 4.0f to 4.2f to overlap with the knee piece
    std::vector<Vec3f> kneeBottom = generateRing(0.0f, 4.2f, 0.15f, 0.75f * legScale, 0.65f * legScale, legSegments);

    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= legSegments; i++) {
        int k = i % legSegments;
        Vec3f n = normalize({ ankle[k].x, 0, ankle[k].z });
        glNormal3f(n.x, n.y, n.z);
        glVertex3f(ankle[k].x, ankle[k].y, ankle[k].z);
        glVertex3f(kneeBottom[k].x, kneeBottom[k].y, kneeBottom[k].z);
    }
    glEnd();
}

void drawSabatons() {
    setArmorMaterial();
    const int segments = 40;
    const float scale = 1.1f;

    glPushMatrix();
    glScalef(scale, scale, scale);
    glTranslatef(0, -0.05f, 0.2f);

    // Define the 3 key cross-sections of the sabaton
    std::vector<Vec3f> heel_top = generateRing(0.0f, 0.9f, -0.5f, 0.5f, 0.45f, segments);
    std::vector<Vec3f> heel_bottom = generateRing(0.0f, 0.0f, -0.5f, 0.5f, 0.4f, segments);

    std::vector<Vec3f> instep_top = generateRing(0.0f, 0.5f, 0.2f, 0.5f, 0.45f, segments);
    std::vector<Vec3f> instep_bottom = generateRing(0.0f, 0.0f, 0.2f, 0.5f, 0.45f, segments);

    std::vector<Vec3f> toe_top = generateRing(0.0f, 0.4f, 0.9f, 0.45f, 0.35f, segments);
    std::vector<Vec3f> toe_bottom = generateRing(0.0f, 0.0f, 0.9f, 0.45f, 0.35f, segments);

    glBegin(GL_QUADS);
    for (int i = 0; i < segments; ++i) {
        int next_i = (i + 1) % segments;

        // ---== SECTION 1: HEEL TO INSTEP ==---
        // 1a. Top Surface (Heel -> Instep)
        Vec3f n_ht = normalize(cross(sub(instep_top[i], heel_top[i]), sub(heel_top[next_i], heel_top[i])));
        glNormal3fv(&n_ht.x);
        glVertex3fv(&heel_top[i].x);
        glVertex3fv(&heel_top[next_i].x);
        glVertex3fv(&instep_top[next_i].x);
        glVertex3fv(&instep_top[i].x);

        // 1b. Bottom Surface (Heel -> Instep)
        Vec3f n_hb = normalize(cross(sub(heel_bottom[next_i], heel_bottom[i]), sub(instep_bottom[i], heel_bottom[i])));
        glNormal3fv(&n_hb.x);
        glVertex3fv(&heel_bottom[i].x);
        glVertex3fv(&instep_bottom[i].x);
        glVertex3fv(&instep_bottom[next_i].x);
        glVertex3fv(&heel_bottom[next_i].x);

        // 1c. Side Wall (Connecting Top and Bottom at the Heel-Instep seam)
        Vec3f n_hs = normalize(cross(sub(instep_top[i], heel_top[i]), sub(heel_bottom[i], heel_top[i])));
        glNormal3fv(&n_hs.x);
        glVertex3fv(&heel_top[i].x);
        glVertex3fv(&heel_bottom[i].x);
        glVertex3fv(&instep_bottom[i].x);
        glVertex3fv(&instep_top[i].x);

        // ---== SECTION 2: INSTEP TO TOE ==---
        // 2a. Top Surface (Instep -> Toe)
        Vec3f n_it = normalize(cross(sub(toe_top[i], instep_top[i]), sub(instep_top[next_i], instep_top[i])));
        glNormal3fv(&n_it.x);
        glVertex3fv(&instep_top[i].x);
        glVertex3fv(&instep_top[next_i].x);
        glVertex3fv(&toe_top[next_i].x);
        glVertex3fv(&toe_top[i].x);

        // 2b. Bottom Surface (Instep -> Toe)
        Vec3f n_ib = normalize(cross(sub(instep_bottom[next_i], instep_bottom[i]), sub(toe_bottom[i], instep_bottom[i])));
        glNormal3fv(&n_ib.x);
        glVertex3fv(&instep_bottom[i].x);
        glVertex3fv(&toe_bottom[i].x);
        glVertex3fv(&toe_bottom[next_i].x);
        glVertex3fv(&instep_bottom[next_i].x);

        // 2c. Side Wall (Connecting Top and Bottom at the Instep-Toe seam)
        Vec3f n_is = normalize(cross(sub(toe_top[i], instep_top[i]), sub(instep_bottom[i], instep_top[i])));
        glNormal3fv(&n_is.x);
        glVertex3fv(&instep_top[i].x);
        glVertex3fv(&instep_bottom[i].x);
        glVertex3fv(&toe_bottom[i].x);
        glVertex3fv(&toe_top[i].x);
    }
    glEnd();

    // ---== SECTION 3: CAP THE BACK AND FRONT ==---

    // **FIXED CODE:** This now builds a solid wall across the back of the heel,
    // connecting the top and bottom rings correctly.
    glBegin(GL_QUADS);
    for (int i = 0; i < segments; ++i) {
        int next_i = (i + 1) % segments;
        Vec3f n_back = normalize(cross(sub(heel_bottom[i], heel_top[i]), sub(heel_top[next_i], heel_top[i])));
        glNormal3fv(&n_back.x);
        glVertex3fv(&heel_top[i].x);
        glVertex3fv(&heel_top[next_i].x);
        glVertex3fv(&heel_bottom[next_i].x);
        glVertex3fv(&heel_bottom[i].x);
    }
    glEnd();

    // Cap the front of the toe
    glBegin(GL_QUADS);
    for (int i = 0; i < segments; ++i) {
        int next_i = (i + 1) % segments;
        Vec3f n_front = normalize(cross(sub(toe_top[next_i], toe_top[i]), sub(toe_bottom[i], toe_top[i])));
        glNormal3fv(&n_front.x);
        glVertex3fv(&toe_top[i].x);
        glVertex3fv(&toe_bottom[i].x);
        glVertex3fv(&toe_bottom[next_i].x);
        glVertex3fv(&toe_top[next_i].x);
    }
    glEnd();

    glPopMatrix();
}
