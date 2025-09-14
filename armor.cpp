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
    const int segments = 32;
    const float helmetRadius = HEAD_RADIUS * 1.75f;
    const float thickness = 0.1f;
    
    glPushMatrix();
    // Position the helmet on the head
    glTranslatef(0.0f, 0.55f, 0.1f);
	glRotatef(0.0f, 0.0f, 1.0f, 0.0f); // Slight forward tilt
    // Calculate the actual eye level relative to helmet position
    const float headCenterY = 0.0f;
    const float eyeY = HEAD_RADIUS * 0.2f; // Eye Y relative to head center = 0.06f
    const float eyeLevelInHelmet = (headCenterY + eyeY) - 0.25f; // = -0.19f
    
    // Define helmet sections heights
    const float topDomeHeight = helmetRadius * 0.8f;     // Upper dome section
    const float eyeLevelHeight = eyeLevelInHelmet;       // Eye level height
    const float slitHeight = 0.15f;                      // Height of the eye slit
    const float slitTop = eyeLevelHeight + slitHeight;   // Top of eye slit
    const float slitBottom = eyeLevelHeight - slitHeight; // Bottom of eye slit
    const float neckGuardTop = -helmetRadius * 0.6f;     // Where neck guard starts
    const float neckGuardBottom = -helmetRadius * 1.3f;  // Where neck guard ends
    
    // Define angular sections (in radians)
    const float frontStartAngle = -PI * 0.3f;  // Where front section starts (-54 degrees)
    const float frontEndAngle = PI * 0.3f;     // Where front section ends (54 degrees)
    
    // 1. Draw the top dome section (complete hemisphere)
    glPushMatrix();
    // Apply dome rotation
    glRotatef(g_helmetDomeRotationX, 1.0f, 0.0f, 0.0f);
    glRotatef(g_helmetDomeRotationY, 0.0f, 1.0f, 0.0f);
    glRotatef(g_helmetDomeRotationZ, 0.0f, 0.0f, 1.0f);
    
    glBegin(GL_TRIANGLES);
    // Top center point
    Vec3f topCenter = {0.0f, topDomeHeight, 0.0f};
    
    // Create dome cap using triangles from center to rim
    for (int i = 0; i < segments; i++) {
        float angle1 = 2.0f * PI * (float)i / segments;
        float angle2 = 2.0f * PI * (float)(i + 1) / segments;
        
        float x1 = helmetRadius * cosf(angle1);
        float z1 = helmetRadius * sinf(angle1);
        float x2 = helmetRadius * cosf(angle2);
        float z2 = helmetRadius * sinf(angle2);
        
        // Create triangle from center to rim
        Vec3f v1 = topCenter;
        Vec3f v2 = {x1, slitTop, z1};
        Vec3f v3 = {x2, slitTop, z2};
        
        // Calculate normal
        Vec3f edge1 = {v2.x - v1.x, v2.y - v1.y, v2.z - v1.z};
        Vec3f edge2 = {v3.x - v1.x, v3.y - v1.y, v3.z - v1.z};
        Vec3f normal = normalize(cross(edge1, edge2));
        
        glNormal3f(normal.x, normal.y, normal.z);
        glVertex3f(v1.x, v1.y, v1.z);
        glVertex3f(v2.x, v2.y, v2.z);
        glVertex3f(v3.x, v3.y, v3.z);
    }
    glEnd();
    glPopMatrix();
    
    // 2. Draw the side and back pieces (excluding front face)
    glPushMatrix();
    // Apply the same rotation as the dome first
    glRotatef(g_helmetDomeRotationX, 1.0f, 0.0f, 0.0f);
    glRotatef(g_helmetDomeRotationY, 0.0f, 1.0f, 0.0f);
    glRotatef(g_helmetDomeRotationZ, 0.0f, 0.0f, 1.0f);
    // Then apply side cover translation
    glTranslatef(g_helmetSideOffsetX, g_helmetSideOffsetY, g_helmetSideOffsetZ);
    
    glBegin(GL_QUADS);
    for (int i = 0; i < segments; i++) {
        float angle1 = 2.0f * PI * (float)i / segments;
        float angle2 = 2.0f * PI * (float)(i + 1) / segments;
        
        // Skip the front face section
        if ((angle1 >= frontStartAngle && angle1 <= frontEndAngle) ||
            (angle2 >= frontStartAngle && angle2 <= frontEndAngle)) {
            continue;
        }
        
        float x1 = helmetRadius * cosf(angle1);
        float z1 = helmetRadius * sinf(angle1);
        float x2 = helmetRadius * cosf(angle2);
        float z2 = helmetRadius * sinf(angle2);
        
        // Calculate normal pointing outward
        Vec3f normal = normalize({(x1 + x2) * 0.5f, 0.0f, (z1 + z2) * 0.5f});
        
        // Draw quad connecting top to bottom
        glNormal3f(normal.x, normal.y, normal.z);
        glVertex3f(x1, slitTop, z1);       // Top-left
        glVertex3f(x2, slitTop, z2);       // Top-right
        glVertex3f(x2, slitBottom, z2);    // Bottom-right
        glVertex3f(x1, slitBottom, z1);    // Bottom-left
    }
    glEnd();
    glPopMatrix();
    
    // 3. Draw the back piece separately (can be translated independently)
    glPushMatrix();
    // Apply the same rotation as the dome first
    glRotatef(g_helmetDomeRotationX, 1.0f, 0.0f, 0.0f);
    glRotatef(g_helmetDomeRotationY, 0.0f, 1.0f, 0.0f);
    glRotatef(g_helmetDomeRotationZ, 0.0f, 0.0f, 1.0f);
    // Then apply back cover translation
    glTranslatef(g_helmetBackOffsetX, g_helmetBackOffsetY, g_helmetBackOffsetZ);
    
    glBegin(GL_QUADS);
    for (int i = 0; i < segments; i++) {
        float angle1 = 2.0f * PI * (float)i / segments;
        float angle2 = 2.0f * PI * (float)(i + 1) / segments;
        
        // Only draw the back section
        if (sinf(angle1) > -0.5f || sinf(angle2) > -0.5f) {
            continue;
        }
        
        float x1 = helmetRadius * cosf(angle1);
        float z1 = helmetRadius * sinf(angle1);
        float x2 = helmetRadius * cosf(angle2);
        float z2 = helmetRadius * sinf(angle2);
        
        // Calculate normal pointing outward
        Vec3f normal = normalize({(x1 + x2) * 0.5f, 0.0f, (z1 + z2) * 0.5f});
        
        // Draw quad connecting top to bottom
        glNormal3f(normal.x, normal.y, normal.z);
        glVertex3f(x1, slitTop, z1);       // Top-left
        glVertex3f(x2, slitTop, z2);       // Top-right
        glVertex3f(x2, slitBottom, z2);    // Bottom-right
        glVertex3f(x1, slitBottom, z1);    // Bottom-left
    }
    glEnd();
    glPopMatrix();
    
    // 4. Draw the lower part of helmet (below eye level to neck)
    glPushMatrix();
    // Apply the same rotation as the dome
    glRotatef(g_helmetDomeRotationX, 1.0f, 0.0f, 0.0f);
    glRotatef(g_helmetDomeRotationY, 0.0f, 1.0f, 0.0f);
    glRotatef(g_helmetDomeRotationZ, 0.0f, 0.0f, 1.0f);
    
    glBegin(GL_QUADS);
    for (int i = 0; i < segments; i++) {
        float angle1 = 2.0f * PI * (float)i / segments;
        float angle2 = 2.0f * PI * (float)(i + 1) / segments;
        
        // Skip the front face section
        if ((angle1 >= frontStartAngle && angle1 <= frontEndAngle) ||
            (angle2 >= frontStartAngle && angle2 <= frontEndAngle)) {
            continue;
        }
        
        float x1 = helmetRadius * cosf(angle1);
        float z1 = helmetRadius * sinf(angle1);
        float x2 = helmetRadius * cosf(angle2);
        float z2 = helmetRadius * sinf(angle2);
        
        // Calculate normal pointing outward
        Vec3f normal = normalize({(x1 + x2) * 0.5f, 0.0f, (z1 + z2) * 0.5f});
        
        // Draw quad connecting eye level to neck
        glNormal3f(normal.x, normal.y, normal.z);
        glVertex3f(x1, slitBottom, z1);    // Top-left
        glVertex3f(x2, slitBottom, z2);    // Top-right
        glVertex3f(x2, neckGuardTop, z2);  // Bottom-right
        glVertex3f(x1, neckGuardTop, z1);  // Bottom-left
    }
    glEnd();
    glPopMatrix();
    
    // 5. Draw the neck guard
    glPushMatrix();
    // Apply the same rotation as the dome to keep them aligned
    glRotatef(g_helmetDomeRotationX, 1.0f, 0.0f, 0.0f);
    glRotatef(g_helmetDomeRotationY, 0.0f, 1.0f, 0.0f);
    glRotatef(g_helmetDomeRotationZ, 0.0f, 0.0f, 1.0f);
    
    // Create rings for neck guard
    std::vector<Vec3f> neckTop = generateRing(0.0f, neckGuardTop, 0.0f, 
                                             helmetRadius * 1.1f, helmetRadius * 1.1f, segments);
    std::vector<Vec3f> neckBottom = generateRing(0.0f, neckGuardBottom, 0.0f, 
                                                helmetRadius * 1.4f, helmetRadius * 1.4f, segments);
    
    // Draw neck guard from top to bottom
    glBegin(GL_QUADS);
    for (int i = 0; i < segments; i++) {
        float angle1 = 2.0f * PI * (float)i / segments;
        float angle2 = 2.0f * PI * (float)(i + 1) / segments;
        
        // Draw complete circle - no front face restriction for neck guard
        int k1 = i;
        int k2 = (i + 1) % segments;
        
        // Calculate normal pointing outward
        Vec3f normal = normalize({(neckTop[k1].x + neckTop[k2].x) * 0.5f, 0.0f, (neckTop[k1].z + neckTop[k2].z) * 0.5f});
        
        // Draw quad connecting neck top to neck bottom
        glNormal3f(normal.x, normal.y, normal.z);
        glVertex3f(neckTop[k1].x, neckTop[k1].y, neckTop[k1].z);      // Top-left
        glVertex3f(neckTop[k2].x, neckTop[k2].y, neckTop[k2].z);      // Top-right
        glVertex3f(neckBottom[k2].x, neckBottom[k2].y, neckBottom[k2].z); // Bottom-right
        glVertex3f(neckBottom[k1].x, neckBottom[k1].y, neckBottom[k1].z); // Bottom-left
    }
    glEnd();
    glPopMatrix();
    
    // 6. Draw the helmet crest/ridge on top
    glPushMatrix();
    // Apply the same rotation as the dome and neck guard
    glRotatef(g_helmetDomeRotationX, 1.0f, 0.0f, 0.0f);
    glRotatef(g_helmetDomeRotationY, 0.0f, 1.0f, 0.0f);
    glRotatef(g_helmetDomeRotationZ, 0.0f, 0.0f, 1.0f);
    
    glTranslatef(0.0f, helmetRadius * 1.3f, 0.0f); // Raised crest position
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
    
    glBegin(GL_QUAD_STRIP);
    float crestLength = helmetRadius * 1.8f; // Longer crest
    float crestHeight = 0.15f;              // Taller crest
    float crestWidth = 0.05f;
    
    for (int i = 0; i <= segments; i++) {
        float t = (float)i / segments;
        float z = -crestLength * 0.5f + crestLength * t;
        
        glNormal3f(0.0f, 1.0f, 0.0f);
        glVertex3f(-crestWidth, crestHeight, z);
        glVertex3f(crestWidth, crestHeight, z);
        
        glNormal3f(0.0f, -1.0f, 0.0f);
        glVertex3f(-crestWidth, 0.0f, z);
        glVertex3f(crestWidth, 0.0f, z);
    }
    glEnd();
    glPopMatrix();
    
    glPopMatrix();
}

void drawArmor() {
    setArmorMaterial();
    glPushMatrix();
    // Increased scale to make armor significantly bigger than torso
    glScalef(1.2f, 1.15f, 1.2f);

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

void drawShoulderArmor() {
    // Shoulder armor removed - replaced with head armor
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

// ===================================================================
// ARM ARMOR IMPLEMENTATION
// ===================================================================

void drawUpperArmArmor() {
    setArmorMaterial();
    const float armScale = 0.14f; // Same as ARM_SCALE
    const float armorRadius = (0.8f) * armScale; // Base arm radius + armor thickness
    
    // Upper arm coordinates (scaled from g_ArmJoints)
    // Joint 0: Shoulder base (0.0, 0.0, 0.1)
    // Joint 3: Elbow (0.30, 0.15, 5.8)
    
    Vec3f shoulderPos = { 0.0f * armScale, 0.0f * armScale, 0.1f * armScale };
    Vec3f elbowPos = { 0.30f * armScale, 0.15f * armScale, 5.8f * armScale };
    
    // Calculate cylinder length and orientation
    Vec3f armVector = { elbowPos.x - shoulderPos.x, elbowPos.y - shoulderPos.y, elbowPos.z - shoulderPos.z };
    float armLength = sqrtf(armVector.x * armVector.x + armVector.y * armVector.y + armVector.z * armVector.z);
    
    glPushMatrix();
    
    // Position at shoulder
    glTranslatef(shoulderPos.x, shoulderPos.y, shoulderPos.z);
    
    // Orient cylinder along arm direction
    // Calculate rotation angles to align cylinder with arm vector
    float yaw = atan2f(armVector.x, armVector.z) * 180.0f / PI;
    float pitch = -asinf(armVector.y / armLength) * 180.0f / PI;
    
    glRotatef(yaw, 0.0f, 1.0f, 0.0f);
    glRotatef(pitch, 1.0f, 0.0f, 0.0f);
    
    // Draw cylinder using GLU quadric
    GLUquadric* quadric = gluNewQuadric();
    if (quadric) {
        gluQuadricNormals(quadric, GLU_SMOOTH);
        gluQuadricOrientation(quadric, GLU_OUTSIDE);
        gluCylinder(quadric, armorRadius, armorRadius, armLength, 32, 1);
        gluDeleteQuadric(quadric);
    }
    
    glPopMatrix();
}


