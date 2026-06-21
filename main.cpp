
#include <GL/glut.h>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <cstring>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ================================================================
//  CONSTANTES
// ================================================================
static const float LANE_X[3] = { -3.0f, 0.0f, 3.0f };
static const float ROAD_HALF_W = 4.9f;
static const float GRASS_HALF_W = 35.0f;
static const float GUARD_X = 5.45f;
static const float TREE_X = 8.2f;

// ================================================================
//  STRUCTURES
// ================================================================
enum GameState { STATE_START, STATE_PLAYING, STATE_GAMEOVER };

struct PlayerCar { float x, y, z, targetX; int lane; };
struct PoliceCar { float x, y, z, targetX, flash; };

struct Coin { float x, y, z, spin; bool alive; };
struct Train { float x, z, halfW, halfL; bool alive; };
struct Tree { float x, z; };
struct Guard { float x, z; };     // barriere de securite
struct Building {
    float x, z;        // centre
    float w, d, h;     // largeur, profondeur, hauteur
    float r, g, b;     // couleur
};

// ================================================================
//  VARIABLES GLOBALES
// ================================================================
static GameState  gState = STATE_START;
static int        gScore = 0;
static int        gCoins = 0;
static float      gSpeed = 14.0f;
static float      gTimePlayed = 0.0f;
static float      gDT = 0.0f;
static bool       gFirstPersonView = false;
static bool       gViewRight = false;
static bool       gViewLeft = false;

static PlayerCar  gPlayer = { 0,0.3f,0, 0,1 };
static PoliceCar  gPolice = { 0,0.3f,-14, 0,0 };

static std::vector<Coin>     gCoinList;
static std::vector<Train>    gTrainList;
static std::vector<Tree>     gTreeList;
static std::vector<Guard>    gGuardList;
static std::vector<Building> gBuildingList;

static float gTreeGenZ = -10.0f;
static float gGuardGenZ = -10.0f;
static float gBuildingGenZ = -10.0f;
static float gCoinTimer = 0.0f;
static float gTrainTimer = 0.0f;

static int   gWinW = 1024, gWinH = 768;

// Flash d'impact quand on collecte une piece
static float gCoinFlash = 0.0f;

// ================================================================
//  REINITIALISATION
// ================================================================
static void resetGame()
{
    gState = STATE_PLAYING;
    gScore = 0;
    gCoins = 0;
    gSpeed = 14.0f;
    gTimePlayed = 0.0f;
    gCoinFlash = 0.0f;
    gFirstPersonView = false;
    gViewRight = false;
    gViewLeft = false;

    gPlayer = { LANE_X[1], 0.3f, 0.0f, LANE_X[1], 1 };
    gPolice = { LANE_X[1], 0.3f, -14.0f, LANE_X[1], 0.0f };

    gCoinList.clear();
    gTrainList.clear();
    gTreeList.clear();
    gGuardList.clear();
    gBuildingList.clear();

    gTreeGenZ = -10.0f;
    gGuardGenZ = -10.0f;
    gBuildingGenZ = -10.0f;
    gCoinTimer = 0.0f;
    gTrainTimer = 0.0f;

    srand((unsigned)time(nullptr));
}

// ================================================================
//  INITIALISATION OpenGL
// ================================================================
static void initGL()
{
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_NORMALIZE);

    GLfloat amb[] = { 0.22f, 0.22f, 0.22f, 1.f };
    GLfloat diff[] = { 0.92f, 0.88f, 0.76f, 1.f };
    GLfloat spec[] = { 1.0f,  1.0f,  1.0f,  1.f };
    glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diff);
    glLightfv(GL_LIGHT0, GL_SPECULAR, spec);

    /* Brouillard atmospherique */
    glEnable(GL_FOG);
    GLfloat fogC[] = { 0.55f, 0.78f, 0.97f, 1.f };
    glFogfv(GL_FOG_COLOR, fogC);
    glFogi(GL_FOG_MODE, GL_LINEAR);
    glFogf(GL_FOG_START, 55.0f);
    glFogf(GL_FOG_END, 105.0f);

    resetGame();
}

// ================================================================
//  UTILITAIRES
// ================================================================
static void solidBox(float hw, float hh, float hl)
{
    glPushMatrix();
    glScalef(hw * 2, hh * 2, hl * 2);
    glutSolidCube(1.0f);
    glPopMatrix();
}

static void glutPrint(float x, float y, const char* s,
    void* font = GLUT_BITMAP_HELVETICA_18)
{
    glRasterPos2f(x, y);
    for (; *s; ++s) glutBitmapCharacter(font, *s);
}

// ================================================================
//  DESSIN : CIEL (rendu 2D avant la scene 3D)
// ================================================================
static void drawSky()
{
    /* Sauvegarde des matrices et etats */
    glMatrixMode(GL_PROJECTION);
    glPushMatrix(); glLoadIdentity();
    gluOrtho2D(0, gWinW, 0, gWinH);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix(); glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);

    int H = gWinH, W = gWinW;
    int horizon = (int)(H * 0.40f);

    /* Degrade vertical : bleu fonce -> bleu ciel -> horizon brumeux */
    glBegin(GL_QUADS);
    glColor3f(0.06f, 0.08f, 0.48f);  glVertex2i(0, W); glVertex2i(W, W);
    glColor3f(0.36f, 0.70f, 0.97f);  glVertex2i(W, horizon); glVertex2i(0, horizon);
    glEnd();
    glBegin(GL_QUADS);
    glColor3f(0.36f, 0.70f, 0.97f);  glVertex2i(0, horizon); glVertex2i(W, horizon);
    glColor3f(0.70f, 0.87f, 0.97f);  glVertex2i(W, 0);       glVertex2i(0, 0);
    glEnd();

    /* Soleil */
    int sx = (int)(W * 0.72f), sy = (int)(H * 0.74f);
    int sr = 42;
    glBegin(GL_TRIANGLE_FAN);
    glColor3f(1.f, 0.97f, 0.62f);
    glVertex2i(sx, sy);
    for (int i = 0; i <= 40; i++) {
        float a = i * 2.f * (float)M_PI / 40;
        glVertex2f(sx + cosf(a) * sr, sy + sinf(a) * sr);
    }
    glEnd();

    /////* Halo du soleil */
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBegin(GL_TRIANGLE_FAN);
    glColor4f(1.f, 0.90f, 0.50f, 0.25f);
    glVertex2i(sx, sy);
    for (int i = 0; i <= 40; i++) {
        float a = i * 2.f * (float)M_PI / 40;
        glColor4f(1.f, 0.88f, 0.40f, 0.0f);
        glVertex2f(sx + cosf(a) * (sr * 3.0f), sy + sinf(a) * (sr * 3.0f));
    }
    glEnd();
    

    /* Rayons du soleil */
    for (int ri = 0; ri < 12; ri++) {
        float a = ri * (float)M_PI / 6.f;
        float r1 = (float)sr * 1.15f, r2 = (float)sr * 2.2f;
        glBegin(GL_QUADS);
        glColor4f(1.f, 0.95f, 0.5f, 0.18f);
        glVertex2f(sx + cosf(a - 0.04f) * r1, sy + sinf(a - 0.04f) * r1);
        glVertex2f(sx + cosf(a + 0.04f) * r1, sy + sinf(a + 0.04f) * r1);
        glColor4f(1.f, 0.95f, 0.5f, 0.0f);
        glVertex2f(sx + cosf(a + 0.10f) * r2, sy + sinf(a + 0.10f) * r2);
        glVertex2f(sx + cosf(a - 0.10f) * r2, sy + sinf(a - 0.10f) * r2);
        glEnd();
    }
    glDisable(GL_BLEND);

    /* Nuages */
    auto cloud = [&](int cx, int cy, float rw, float rh, float alpha) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        /* Plusieurs ellipses pour un nuage duveteux */
        float puff[5][4] = {
            { 0,      0,     rw,       rh       },
            {-rw * 0.4f, rh * 0.25f, rw * 0.65f, rh * 0.70f },
            { rw * 0.4f, rh * 0.25f, rw * 0.65f, rh * 0.70f },
            {-rw * 0.2f,-rh * 0.2f,  rw * 0.50f, rh * 0.55f },
            { rw * 0.2f,-rh * 0.2f,  rw * 0.50f, rh * 0.55f }
        };
        for (auto& p : puff) {
            glBegin(GL_TRIANGLE_FAN);
            glColor4f(0.97f, 0.97f, 1.0f, alpha);
            glVertex2f(cx + p[0], cy + p[1]);
            for (int i = 0; i <= 28; i++) {
                float a = i * 2.f * (float)M_PI / 28;
                glColor4f(0.96f, 0.96f, 1.0f, alpha * 0.6f);
                glVertex2f(cx + p[0] + cosf(a) * p[2], cy + p[1] + sinf(a) * p[3]);
            }
            glEnd();
        }
        glDisable(GL_BLEND);
        };
    cloud(W / 6, (int)(H * 0.78f), 100, 38, 0.90f);
    cloud(W / 2, (int)(H * 0.83f), 130, 46, 0.85f);
    cloud(W * 11 / 12, (int)(H * 0.86f), 70, 28, 0.80f);
    cloud(W / 10, (int)(H * 0.90f), 55, 22, 0.75f);

    /* Restauration des etats */
    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_FOG);
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);  glPopMatrix();
}

// ================================================================
//  DESSIN : ROUTE
// ================================================================
static void drawRoad()
{
    float pz = gPlayer.z;
    float near_ = pz - 28.f;
    float far_ = pz + 115.f;

    glDisable(GL_FOG);

    /* Herbe */
    glColor3f(0.18f, 0.52f, 0.10f);
    glNormal3f(0, 1, 0);
    glBegin(GL_QUADS);
    glVertex3f(-GRASS_HALF_W, -0.01f, near_); glVertex3f(GRASS_HALF_W, -0.01f, near_);
    glVertex3f(GRASS_HALF_W, -0.01f, far_);  glVertex3f(-GRASS_HALF_W, -0.01f, far_);
    glEnd();

    glEnable(GL_FOG);

    /* Asphalte */
    glColor3f(0.20f, 0.20f, 0.22f);
    glBegin(GL_QUADS);
    glVertex3f(-ROAD_HALF_W, 0.f, near_); glVertex3f(ROAD_HALF_W, 0.f, near_);
    glVertex3f(ROAD_HALF_W, 0.f, far_);  glVertex3f(-ROAD_HALF_W, 0.f, far_);
    glEnd();

    /* Bandes de bord jaunes */
    auto edgeLine = [&](float ex) {
        glColor3f(1.f, 0.88f, 0.f);
        glBegin(GL_QUADS);
        glVertex3f(ex - 0.10f, 0.006f, near_);
        glVertex3f(ex + 0.10f, 0.006f, near_);
        glVertex3f(ex + 0.10f, 0.006f, far_);
        glVertex3f(ex - 0.10f, 0.006f, far_);
        glEnd();
        };
    edgeLine(-ROAD_HALF_W + 0.10f);
    edgeLine(ROAD_HALF_W - 0.10f);

    /* Pointilles blancs entre voies */
    float dashL = 3.0f, gapL = 3.0f, dw = 0.07f;
    float period = dashL + gapL;
    float laneDiv[] = { -1.5f, 1.5f };
    for (float lx : laneDiv) {
        glColor3f(1.f, 1.f, 1.f);
        float zp = floorf(near_ / period) * period;
        while (zp < far_) {
            float z0 = fmaxf(zp, near_);
            float z1 = fminf(zp + dashL, far_);
            if (z1 > z0) {
                glBegin(GL_QUADS);
                glVertex3f(lx - dw, 0.007f, z0);
                glVertex3f(lx + dw, 0.007f, z0);
                glVertex3f(lx + dw, 0.007f, z1);
                glVertex3f(lx - dw, 0.007f, z1);
                glEnd();
            }
            zp += period;
        }
    }
}

// ================================================================
//  DESSIN : ROUE (améliorée : jante + pneu + enjoliveur)
// ================================================================
static void drawWheel(float r, float w)
{
    GLUquadric* q = gluNewQuadric();
    gluQuadricNormals(q, GLU_SMOOTH);

    /* Pneu */
    glColor3f(0.11f, 0.11f, 0.11f);
    gluCylinder(q, r, r, w, 20, 1);
    gluDisk(q, 0, r, 20, 1);
    glTranslatef(0, 0, w);
    gluDisk(q, 0, r, 20, 1);

    /* Jante argentée (anneau fin sur la face extérieure) */
    glColor3f(0.88f, 0.88f, 0.88f);
    gluDisk(q, r * 0.90f, r, 20, 1);

    /* Cache-moyeu central */
    glColor3f(0.75f, 0.75f, 0.78f);
    gluDisk(q, 0, r * 0.35f, 10, 1);

    gluDeleteQuadric(q);
}

// ================================================================
//  DESSIN : VOITURE (améliorée avec détails)
//  police = true  -> voiture de police (blanc/bleu + gyrophare)
//  police = false -> voiture rouge du joueur
// ================================================================
static void drawVehicle(float x, float y, float z, bool police, float flash)
{
    glPushMatrix();
    glTranslatef(x, y, z);

    // Police plus visible (agrandie)
    if (police) glScalef(1.25f, 1.25f, 1.25f);

    float bW = 0.72f, bH = 0.38f, bL = 1.60f;

    /* ---- CARROSSERIE ---- */
    GLfloat spec[] = { 0.88f,0.88f,0.88f,1.f };
    GLfloat shin[] = { 92.f };
    glMaterialfv(GL_FRONT, GL_SPECULAR, spec);
    glMaterialfv(GL_FRONT, GL_SHININESS, shin);

    if (police) glColor3f(0.90f, 0.90f, 0.94f);
    else       glColor3f(0.92f, 0.05f, 0.05f);

    glPushMatrix();
    glTranslatef(0, bH, 0);
    solidBox(bW, bH, bL);
    glPopMatrix();

    /* Toit / habitacle */
    float cW = bW * 0.80f, cH = 0.26f, cL = bL * 0.52f;
    if (police) glColor3f(0.86f, 0.86f, 0.90f);
    else       glColor3f(0.85f, 0.04f, 0.04f);
    glPushMatrix();
    glTranslatef(0, bH * 2 + cH, -bL * 0.06f);
    solidBox(cW, cH, cL);
    glPopMatrix();

    /* Pare-brise */
    GLfloat wSpec[] = { 1.f,1.f,1.f,1.f };
    GLfloat wShin[] = { 128.f };
    glMaterialfv(GL_FRONT, GL_SPECULAR, wSpec);
    glMaterialfv(GL_FRONT, GL_SHININESS, wShin);
    glColor3f(0.42f, 0.58f, 0.78f); // teinte plus foncée
    glPushMatrix();
    glTranslatef(0, bH * 2 + cH * 0.5f, bL * 0.47f + 0.01f);
    solidBox(cW * 0.95f, cH * 0.80f, 0.03f);
    glPopMatrix();
    /* Lunette arriere */
    glPushMatrix();
    glTranslatef(0, bH * 2 + cH * 0.5f, -bL * 0.53f - 0.01f);
    solidBox(cW * 0.90f, cH * 0.75f, 0.03f);
    glPopMatrix();
    glMaterialfv(GL_FRONT, GL_SPECULAR, spec);
    glMaterialfv(GL_FRONT, GL_SHININESS, shin);

    /* Vitres laterales */
    glColor3f(0.38f, 0.55f, 0.75f);
    for (float sx : {-bW - 0.01f, bW + 0.01f}) {
        glPushMatrix();
        glTranslatef(sx, bH * 2 + cH * 0.55f, -bL * 0.07f);
        solidBox(0.02f, cH * 0.72f, cL * 0.75f);
        glPopMatrix();
    }

    /* ---- RETROVISEURS ---- */
    glColor3f(0.15f, 0.15f, 0.15f);
    for (float sx : {-bW - 0.06f, bW + 0.06f}) {
        glPushMatrix();
        glTranslatef(sx, bH + cH * 1.2f, bL * 0.25f);
        solidBox(0.03f, 0.08f, 0.05f);
        glPopMatrix();
    }

    /* ---- CALANDRE AVANT ---- */
    glColor3f(0.05f, 0.05f, 0.05f);
    glPushMatrix();
    glTranslatef(0, bH * 0.65f, bL + 0.01f);
    solidBox(bW * 0.75f, 0.06f, 0.02f);
    // barrettes verticales
    for (int i = -3; i <= 3; i++) {
        float cx = i * (bW * 0.7f / 6);
        glPushMatrix();
        glTranslatef(cx, 0, 0);
        solidBox(0.01f, 0.06f, 0.01f);
        glPopMatrix();
    }
    glPopMatrix();

    /* Phares avant (plus modernes) */
    glColor3f(1.f, 0.97f, 0.80f);
    for (float sx : {-bW + 0.14f, bW - 0.14f}) {
        glPushMatrix();
        glTranslatef(sx, bH + 0.10f, bL + 0.02f);
        solidBox(0.13f, 0.09f, 0.04f);
        glPopMatrix();
    }

    /* Feux arriere */
    glColor3f(0.95f, 0.05f, 0.05f);
    for (float sx : {-bW + 0.14f, bW - 0.14f}) {
        glPushMatrix();
        glTranslatef(sx, bH + 0.10f, -bL - 0.01f);
        solidBox(0.13f, 0.09f, 0.04f);
        glPopMatrix();
    }

    /* ---- SPOILER ARRIERE ---- */
    glColor3f(0.10f, 0.10f, 0.10f);
    glPushMatrix();
    glTranslatef(0, bH + 0.45f, -bL - 0.05f);
    solidBox(bW * 0.88f, 0.06f, 0.08f);
    // montants
    for (float sx : {-bW * 0.75f, bW * 0.75f}) {
        glPushMatrix();
        glTranslatef(sx, -0.15f, 0);
        solidBox(0.03f, 0.12f, 0.05f);
        glPopMatrix();
    }
    glPopMatrix();

    /* ---- DOUBLE ECHAPPEMENT ---- */
    for (float sx : {-bW * 0.30f, bW * 0.30f}) {
        glPushMatrix();
        glTranslatef(sx, bH * 0.30f, -bL - 0.08f);
        glRotatef(90, 1, 0, 0);
        GLUquadric* eq = gluNewQuadric();
        gluCylinder(eq, 0.05f, 0.06f, 0.12f, 10, 1);
        gluDisk(eq, 0, 0.05f, 10, 1);
        glTranslatef(0, 0, 0.12f);
        gluDisk(eq, 0, 0.06f, 10, 1);
        gluDeleteQuadric(eq);
        glPopMatrix();
    }

    /* Pare-chocs */
    if (police) glColor3f(0.80f, 0.80f, 0.84f);
    else       glColor3f(0.25f, 0.25f, 0.25f);
    glPushMatrix();
    glTranslatef(0, bH * 0.30f, bL + 0.06f);
    solidBox(bW, 0.09f, 0.09f);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(0, bH * 0.30f, -bL - 0.06f);
    solidBox(bW, 0.09f, 0.09f);
    glPopMatrix();

    /* ---- PLAQUE D'IMMATRICULATION ---- */
    glColor3f(1.f, 1.f, 1.f);
    glPushMatrix();
    glTranslatef(0, bH * 0.55f, -bL - 0.07f);
    solidBox(bW * 0.25f, 0.05f, 0.01f);
    glColor3f(0.05f, 0.05f, 0.10f);
    glPushMatrix();
    glTranslatef(0, 0, 0.01f);
    solidBox(bW * 0.20f, 0.03f, 0.005f);
    glPopMatrix();
    glPopMatrix();

    /* Bandes laterales police */
    if (police) {
        glColor3f(0.10f, 0.10f, 0.10f);
        glPushMatrix();
        glTranslatef(0, bH + 0.05f, 0);
        solidBox(bW + 0.01f, 0.06f, bL + 0.01f);
        glPopMatrix();
    }

    /* ---- GYROPHARE (police) ---- */
    if (police) {
        glColor3f(0.12f, 0.12f, 0.12f);
        glPushMatrix();
        glTranslatef(0, bH * 2 + cH * 2 + 0.04f, -bL * 0.08f);
        solidBox(cW * 0.48f, 0.04f, cL * 0.52f);
        glPopMatrix();

        bool blueOn = fmodf(flash, 0.45f) < 0.225f;

        /* Feu bleu (plus gros) */
        glColor3f(blueOn ? 0.10f : 0.04f,
            blueOn ? 0.18f : 0.04f,
            blueOn ? 1.00f : 0.28f);
        glPushMatrix();
        glTranslatef(-cW * 0.22f, bH * 2 + cH * 2 + 0.18f, -bL * 0.08f);
        solidBox(0.18f, 0.10f, cL * 0.48f);
        glPopMatrix();

        /* Feu rouge */
        bool redOn = !blueOn;
        glColor3f(redOn ? 1.00f : 0.28f, 0.04f, 0.04f);
        glPushMatrix();
        glTranslatef(cW * 0.22f, bH * 2 + cH * 2 + 0.18f, -bL * 0.08f);
        solidBox(0.18f, 0.10f, cL * 0.48f);
        glPopMatrix();

        /* Halo lumineux autour du gyrophare */
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        float alpha = 0.35f + 0.15f * sinf(flash * 12.f);
        glColor4f(blueOn ? 0.0f : 1.0f, 0.0f, blueOn ? 1.0f : 0.0f, alpha);
        glPushMatrix();
        glTranslatef(0, bH * 2 + cH * 2 + 0.35f, -bL * 0.08f);
        solidBox(cW * 0.60f, 0.25f, cL * 0.60f);
        glPopMatrix();
        glDisable(GL_BLEND);
    }

    /* ---- ROUES ---- */
    float wr = 0.27f, ww = 0.17f;
    float wxArr[] = { -bW - ww * 0.5f, bW + ww * 0.5f };
    float wzArr[] = { bL * 0.60f,  -bL * 0.60f };
    for (float wx : wxArr) {
        for (float wz : wzArr) {
            glPushMatrix();
            glTranslatef(wx, wr, wz);
            glRotatef(90, 0, 1, 0);
            drawWheel(wr, ww);
            glPopMatrix();
        }
    }

    GLfloat dSpec[] = { 0.28f,0.28f,0.28f,1.f };
    GLfloat dShin[] = { 25.f };
    glMaterialfv(GL_FRONT, GL_SPECULAR, dSpec);
    glMaterialfv(GL_FRONT, GL_SHININESS, dShin);

    glPopMatrix();
}

// ================================================================
//  DESSIN : TRAIN (amélioré avec nez profilé, chasse-pierres, détails)
// ================================================================
static void drawTrain(const Train& t)
{
    glPushMatrix();
    glTranslatef(t.x, 0, t.z);

    float W = t.halfW, L = t.halfL, H = 1.35f;

    GLfloat ts[] = { 0.5f,0.5f,0.6f,1.f };
    GLfloat th[] = { 60.f };
    glMaterialfv(GL_FRONT, GL_SPECULAR, ts);
    glMaterialfv(GL_FRONT, GL_SHININESS, th);

    /* Corps principal */
    glColor3f(0.10f, 0.14f, 0.52f);
    glPushMatrix();
    glTranslatef(0, H, 0);
    solidBox(W, H, L);
    glPopMatrix();

    /* Toit */
    glColor3f(0.15f, 0.18f, 0.58f);
    glPushMatrix();
    glTranslatef(0, H * 2 + 0.14f, 0);
    solidBox(W * 0.78f, 0.14f, L * 0.93f);
    glPopMatrix();

    /* Bande décorative jaune/orange */
    glColor3f(0.90f, 0.60f, 0.05f);
    glPushMatrix();
    glTranslatef(0, H * 1.0f, 0);
    solidBox(W + 0.01f, 0.08f, L + 0.01f);
    glPopMatrix();

    /* Fenêtres (plus nombreuses) */
    glColor3f(0.62f, 0.82f, 0.95f);
    int nW = 5;
    float step = (L * 2 - 1.0f) / (nW - 1);
    for (int wi = 0; wi < nW; wi++) {
        float wz = -L + 0.5f + wi * step;
        for (float sx : {-W - 0.01f, W + 0.01f}) {
            glPushMatrix();
            glTranslatef(sx, H * 1.38f, wz);
            solidBox(0.03f, H * 0.28f, 0.32f);
            glPopMatrix();
        }
    }

    /* ---- NEZ PROFILE (avant du train) ---- */
    glColor3f(0.08f, 0.10f, 0.42f);
    glPushMatrix();
    glTranslatef(0, H, L + 0.5f);   // position du nez
    glRotatef(90, 0, 1, 0);        // orienter le cône le long de Z
    glutSolidCone(W * 0.75f, 1.5f, 12, 4);
    glPopMatrix();

    /* Chasse-pierres (triangle à l'avant en bas) */
    glColor3f(0.12f, 0.12f, 0.12f);
    glBegin(GL_TRIANGLES);
    float cx = 0, cz = L + 0.3f;
    glVertex3f(cx, 0.2f, cz + 0.8f);
    glVertex3f(cx - W * 0.9f, 0.2f, cz);
    glVertex3f(cx + W * 0.9f, 0.2f, cz);
    glEnd();

    /* Extrémité arrière */
    glColor3f(0.08f, 0.10f, 0.42f);
    glPushMatrix();
    glTranslatef(0, H, -L - 0.01f);
    solidBox(W * 0.85f, H * 0.60f, 0.08f);
    glPopMatrix();

    /* Bogies (chariots de roues) améliorés */
    float bogieZ[] = { -L * 0.55f, L * 0.55f };
    for (float bz : bogieZ) {
        glColor3f(0.18f, 0.18f, 0.18f);
        glPushMatrix();
        glTranslatef(0, 0.22f, bz);
        solidBox(W * 0.80f, 0.22f, L * 0.22f);
        float sxArr[] = { -W * 0.60f, W * 0.60f };
        for (float sx : sxArr) {
            glPushMatrix();
            glTranslatef(sx, -0.06f, 0);
            glRotatef(90, 0, 1, 0);
            GLUquadric* q = gluNewQuadric();
            gluQuadricNormals(q, GLU_SMOOTH);
            /* roue métallique */
            glColor3f(0.38f, 0.38f, 0.38f);
            gluCylinder(q, 0.28f, 0.28f, 0.12f, 12, 1);
            gluDisk(q, 0, 0.28f, 12, 1);
            glTranslatef(0, 0, 0.12f);
            gluDisk(q, 0, 0.28f, 12, 1);
            /* boudin central */
            glColor3f(0.22f, 0.22f, 0.22f);
            gluDisk(q, 0.05f, 0.28f, 12, 1);
            gluDeleteQuadric(q);
            glPopMatrix();
        }
        glPopMatrix();
    }

    GLfloat ds[] = { 0.28f,0.28f,0.28f,1.f };
    GLfloat dh[] = { 25.f };
    glMaterialfv(GL_FRONT, GL_SPECULAR, ds);
    glMaterialfv(GL_FRONT, GL_SHININESS, dh);

    glPopMatrix();
}

// ================================================================
//  DESSIN : PIECE D'OR
// ================================================================
static void drawCoin(const Coin& c)
{
    if (!c.alive) return;
    glPushMatrix();
    glTranslatef(c.x, c.y, c.z);
    glRotatef(c.spin, 0, 1, 0);

    GLfloat cs[] = { 1.f,0.88f,0.15f,1.f };
    GLfloat ch[] = { 110.f };
    glMaterialfv(GL_FRONT, GL_SPECULAR, cs);
    glMaterialfv(GL_FRONT, GL_SHININESS, ch);

    glColor3f(1.f, 0.80f, 0.05f);
    GLUquadric* q = gluNewQuadric();
    gluQuadricNormals(q, GLU_SMOOTH);
    /* Face avant */
    gluDisk(q, 0, 0.36f, 20, 1);
    /* Tranche */
    gluCylinder(q, 0.36f, 0.36f, 0.09f, 20, 1);
    /* Face arriere */
    glTranslatef(0, 0, 0.09f);
    gluDisk(q, 0, 0.36f, 20, 1);
    /* Etoile centrale */
    glColor3f(1.f, 0.92f, 0.42f);
    gluDisk(q, 0, 0.18f, 8, 1);
    gluDeleteQuadric(q);

    GLfloat ds[] = { 0.28f,0.28f,0.28f,1.f }; GLfloat dh[] = { 25.f };
    glMaterialfv(GL_FRONT, GL_SPECULAR, ds);
    glMaterialfv(GL_FRONT, GL_SHININESS, dh);

    glPopMatrix();
}

// ================================================================
//  DESSIN : ARBRE
// ================================================================
static void drawTree(const Tree& t)
{
    glPushMatrix();
    glTranslatef(t.x, 0, t.z);

    /* Tronc */
    glColor3f(0.43f, 0.26f, 0.09f);
    GLUquadric* q = gluNewQuadric();
    glRotatef(-90, 1, 0, 0);
    gluCylinder(q, 0.17f, 0.10f, 1.7f, 8, 1);
    glRotatef(90, 1, 0, 0);

    /* 3 couches de feuillage conique */
    float yr[] = { 1.7f, 2.45f, 3.10f };
    float rr[] = { 1.52f, 1.18f, 0.82f };
    float ht[] = { 1.55f, 1.30f, 1.05f };
    float gc[] = { 0.38f, 0.50f, 0.62f };
    for (int i = 0; i < 3; i++) {
        glColor3f(0.07f, gc[i], 0.07f);
        glPushMatrix();
        glTranslatef(0, yr[i], 0);
        glRotatef(-90, 1, 0, 0);
        glutSolidCone(rr[i], ht[i], 10, 4);
        glPopMatrix();
    }
    gluDeleteQuadric(q);
    glPopMatrix();
}

// ================================================================
//  DESSIN : BARRIERE DE SECURITE (glissiere)
// ================================================================
static void drawGuard(const Guard& g)
{
    glPushMatrix();
    glTranslatef(g.x, 0, g.z);

    /* Bloc en beton */
    glColor3f(0.70f, 0.70f, 0.70f);
    glPushMatrix();
    glTranslatef(0, 0.44f, 0);
    solidBox(0.14f, 0.44f, 1.0f);
    glPopMatrix();

    /* Bande orange retro-reflechissante */
    glColor3f(0.95f, 0.47f, 0.02f);
    glPushMatrix();
    glTranslatef(0, 0.74f, 0);
    solidBox(0.145f, 0.075f, 1.01f);
    glPopMatrix();

    glPopMatrix();
}

// ================================================================
//  DESSIN : BATIMENT
// ================================================================
static void drawBuilding(const Building& b)
{
    glPushMatrix();
    glTranslatef(b.x, 0, b.z);

    // Corps principal
    glColor3f(b.r, b.g, b.b);
    solidBox(b.w * 0.5f, b.h * 0.5f, b.d * 0.5f);

    // Toit plus sombre
    glColor3f(b.r * 0.55f, b.g * 0.55f, b.b * 0.55f);
    glPushMatrix();
    glTranslatef(0, b.h * 0.5f + 0.15f, 0);
    solidBox(b.w * 0.48f, 0.15f, b.d * 0.48f);
    glPopMatrix();

    // Fenêtres sur la façade côté route
    float signFront = (b.x < 0) ? 1.0f : -1.0f; // la façade qui donne sur la route
    float fx = signFront * (b.w * 0.5f + 0.005f);
    float winW = b.w * 0.09f, winH = b.h * 0.12f;
    float startY = b.h * 0.18f, startZ = -b.d * 0.38f;
    int cols = std::max(1, (int)(b.d * 0.7f / 1.2f));
    int rows = std::max(1, (int)(b.h * 0.55f / 1.5f));
    float colSpacing = (cols > 1) ? (b.d * 0.7f) / (cols - 1) : 0;
    float rowSpacing = (rows > 1) ? (b.h * 0.55f) / (rows - 1) : 0;

    glColor3f(0.88f, 0.92f, 1.0f);
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            float z = startZ + c * colSpacing;
            float y = startY + r * rowSpacing;
            glBegin(GL_QUADS);
            glVertex3f(fx, y, z);
            glVertex3f(fx, y, z + winW);
            glVertex3f(fx, y + winH, z + winW);
            glVertex3f(fx, y + winH, z);
            glEnd();
        }
    }

    glPopMatrix();
}

// ================================================================
//  HUD (affichage tete haute)
// ================================================================
static void drawHUD()
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix(); glLoadIdentity();
    gluOrtho2D(0, gWinW, 0, gWinH);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix(); glLoadIdentity();

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_FOG);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    /* Panneau semi-transparent */
    int px = 12, py = gWinH - 12, pw = 218, ph = 95;
    /* Bordure */
    glColor4f(0.90f, 0.70f, 0.00f, 0.70f);
    glBegin(GL_QUADS);
    glVertex2i(px - 2, py + 2); glVertex2i(px + pw + 2, py + 2);
    glVertex2i(px + pw + 2, py - ph - 2); glVertex2i(px - 2, py - ph - 2);
    glEnd();
    /* Fond */
    glColor4f(0.0f, 0.0f, 0.0f, 0.50f);
    glBegin(GL_QUADS);
    glVertex2i(px, py); glVertex2i(px + pw, py);
    glVertex2i(px + pw, py - ph); glVertex2i(px, py - ph);
    glEnd();
    glDisable(GL_BLEND);

    /* Score */
    glColor3f(1.f, 0.92f, 0.10f);
    {
        std::ostringstream ss; ss << "Score: " << gScore;
        glutPrint(px + 12, py - 32, ss.str().c_str());
    }

    /* Pieces */
    glColor3f(1.f, 0.78f, 0.05f);
    {
        std::ostringstream cs; cs << "Pieces: " << gCoins;
        glutPrint(px + 12, py - 58, cs.str().c_str());
    }

    /* Vitesse */
    glColor3f(0.52f, 1.f, 0.52f);
    {
        int kmh = (int)(gSpeed * 3.1f);
        std::ostringstream sp; sp << "Vitesse: " << kmh << " km/h";
        glutPrint(px + 12, py - 84, sp.str().c_str());
    }

    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_FOG);
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);  glPopMatrix();
}

// ================================================================
//  ECRANS START / GAME OVER
// ================================================================
static void drawOverlay()
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix(); glLoadIdentity();
    gluOrtho2D(0, gWinW, 0, gWinH);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix(); glLoadIdentity();

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_FOG);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    /* Voile sombre */
    glColor4f(0, 0, 0, (gState == STATE_GAMEOVER) ? 0.62f : 0.50f);
    glBegin(GL_QUADS);
    glVertex2i(0, 0); glVertex2i(gWinW, 0);
    glVertex2i(gWinW, gWinH); glVertex2i(0, gWinH);
    glEnd();
    glDisable(GL_BLEND);

    int cx = gWinW / 2, cy = gWinH / 2;

    if (gState == STATE_START) {
        /* Titre */
        glColor3f(1.f, 0.88f, 0.05f);
        glutPrint(cx - 92, cy + 62, "ROAD ESCAPE", GLUT_BITMAP_TIMES_ROMAN_24);

        /* Sous-titre */
        glColor3f(0.92f, 0.92f, 0.92f);
        glutPrint(cx - 105, cy + 15, "La police est derriere vous!");

        /* Touche demarrer */
        glColor3f(1.f, 1.f, 0.3f);
        glutPrint(cx - 112, cy - 22, "Appuyez sur ESPACE pour demarrer");

        /* Controles */
        glColor3f(0.72f, 0.85f, 0.72f);
        glutPrint(cx - 148, cy - 55, "FLECHES GAUCHE/DROITE -> changer de voie");
        glutPrint(cx - 120, cy - 78, "Maintenez R / E -> vues laterales");
        glutPrint(cx - 115, cy - 101, "D -> vue subjective (toggle)");
        glutPrint(cx - 138, cy - 124, "Evitez les trains, collectez les pieces!");
    }
    else if (gState == STATE_GAMEOVER) {
        /* Game Over */
        glColor3f(1.f, 0.18f, 0.18f);
        glutPrint(cx - 98, cy + 62, "GAME  OVER!", GLUT_BITMAP_TIMES_ROMAN_24);

        glColor3f(1.f, 1.f, 0.f);
        {
            std::ostringstream fs;
            fs << "Score final: " << gScore << "     Pieces: " << gCoins;
            glutPrint(cx - 120, cy + 10, fs.str().c_str());
        }

        glColor3f(1.f, 1.f, 1.f);
        glutPrint(cx - 118, cy - 32, "Appuyez sur ESPACE pour rejouer");
    }

    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_FOG);
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);  glPopMatrix();
}

// ================================================================
//  DISPLAY PRINCIPAL
// ================================================================
static void display()
{
    glClearColor(0.55f, 0.78f, 0.97f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* 1. Ciel 2D */
    drawSky();

    /* 2. Camera 3D */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(57.f, (float)gWinW / gWinH, 0.1f, 200.f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    float px = gPlayer.x, py = gPlayer.y, pz = gPlayer.z;
    bool sideRight = gViewRight;
    bool sideLeft = gViewLeft;

    if (sideRight) {
        // Vue latérale droite (touche R)
        gluLookAt(px + 14.f, py + 5.5f, pz,
            px, py + 1.0f, pz,
            0, 1, 0);
    }
    else if (sideLeft) {
        // Vue latérale gauche (touche E)
        gluLookAt(px - 14.f, py + 5.5f, pz,
            px, py + 1.0f, pz,
            0, 1, 0);
    }
    else if (gFirstPersonView) {
        // Vue subjective (touche D)
        gluLookAt(px, py + 0.8f, pz,          // œil du conducteur
            px, py + 0.8f, pz + 10.f,   // regarde devant
            0, 1, 0);
    }
    else {
        // Vue arrière par-dessus l'épaule
        gluLookAt(px * 0.55f, py + 5.8f, pz - 11.5f,
            px * 0.45f, py + 0.9f, pz + 24.f,
            0, 1, 0);
    }

    /* Position soleil */
    GLfloat lp[] = { px + 18, 45, pz - 28, 1 };
    glLightfv(GL_LIGHT0, GL_POSITION, lp);

    /* 3. Route */
    drawRoad();

    /* 4. Bâtiments */
    for (auto& b : gBuildingList) drawBuilding(b);

    /* 5. Arbres */
    for (auto& t : gTreeList)  drawTree(t);

    /* 6. Barrieres */
    for (auto& g : gGuardList) drawGuard(g);

    /* 7. Pieces */
    for (auto& c : gCoinList)  drawCoin(c);

    /* 8. Trains */
    for (auto& t : gTrainList) drawTrain(t);

    /* 9. Voiture de police */
    drawVehicle(gPolice.x, gPolice.y, gPolice.z, true, gPolice.flash);

    /* 10. Voiture du joueur (cachée en vue subjective sans vue latérale) */
    bool hidePlayer = (gFirstPersonView && !sideRight && !sideLeft);
    if (!hidePlayer) drawVehicle(px, py, pz, false, 0);

    /* 11. HUD */
    if (gState == STATE_PLAYING) drawHUD();

    /* 12. Ecrans de fin / debut */
    if (gState != STATE_PLAYING) drawOverlay();

    glutSwapBuffers();
}

// ================================================================
//  RESHAPE
// ================================================================
static void reshape(int w, int h)
{
    gWinW = w; gWinH = h;
    glViewport(0, 0, w, h);
}

// ================================================================
//  CLAVIER
// ================================================================
static void keyboard(unsigned char key, int, int)
{
    if (key == 'r' || key == 'R') { gViewRight = true; }
    if (key == 'e' || key == 'E') { gViewLeft = true; }
    if (key == 'd' || key == 'D') {
        if (gState == STATE_PLAYING) gFirstPersonView = !gFirstPersonView;
    }
    if (key == ' ') {
        if (gState == STATE_START || gState == STATE_GAMEOVER) resetGame();
    }
    if (key == 27) exit(0);
}
static void keyboardUp(unsigned char key, int, int)
{
    if (key == 'r' || key == 'R') gViewRight = false;
    if (key == 'e' || key == 'E') gViewLeft = false;
}
static void special(int key, int, int)
{
    if (gState != STATE_PLAYING) return;
    if (key == GLUT_KEY_RIGHT && gPlayer.lane > 0) {
        gPlayer.lane--;
        gPlayer.targetX = LANE_X[gPlayer.lane];
    }
    if (key == GLUT_KEY_LEFT && gPlayer.lane < 2) {
        gPlayer.lane++;
        gPlayer.targetX = LANE_X[gPlayer.lane];
    }
}

// ================================================================
//  BOUCLE DE JEU
// ================================================================
static void update(int)
{
    static float lastT = 0;
    float now = glutGet(GLUT_ELAPSED_TIME) * 0.001f;
    gDT = now - lastT; lastT = now;
    if (gDT > 0.07f) gDT = 0.07f;

    if (gState == STATE_PLAYING)
    {
        gTimePlayed += gDT;

        /* Vitesse progressive */
        gSpeed = 14.0f + gTimePlayed * 0.28f;
        if (gSpeed > 30.0f) gSpeed = 30.0f;

        /* Avancement joueur */
        gPlayer.z += gSpeed * gDT;

        /* Transition laterale fluide */
        gPlayer.x += (gPlayer.targetX - gPlayer.x) * 8.5f * gDT;

        // Police : plus proche pour être plus visible (7 unités au lieu de 10)
        float targetPZ = gPlayer.z - 7.0f;
        gPolice.z += (targetPZ - gPolice.z) * 2.2f * gDT;
        gPolice.x += (gPlayer.x - gPolice.x) * 1.8f * gDT;
        gPolice.flash += gDT;

        /* Rotation des pieces */
        for (auto& c : gCoinList) c.spin += 115.f * gDT;

        /* Flash de collecte qui decroit */
        if (gCoinFlash > 0) gCoinFlash -= gDT * 2.5f;

        /* ======== SPAWNING ======== */
        float spawnZ = gPlayer.z + 88.f;

        /* Pieces */
        gCoinTimer += gDT;
        float coinInterval = 1.1f - gTimePlayed * 0.008f;
        if (coinInterval < 0.55f) coinInterval = 0.55f;
        if (gCoinTimer > coinInterval) {
            gCoinTimer = 0;
            int lane = rand() % 3;
            int count = (rand() % 3 == 0) ? (3 + rand() % 3) : 1;
            for (int i = 0; i < count; i++) {
                Coin c{ LANE_X[lane], 0.98f, spawnZ + i * 2.5f, 0.f, true };
                gCoinList.push_back(c);
            }
        }

        /* Trains */
        gTrainTimer += gDT;
        float trainInterval = 4.2f - gTimePlayed * 0.025f;
        if (trainInterval < 1.8f) trainInterval = 1.8f;
        if (gTrainTimer > trainInterval) {
            gTrainTimer = 0;
            /* 1 ou 2 voies bloquees (jamais les 3) */
            int blocked = (rand() % 3 == 0) ? 2 : 1;
            int order[3] = { 0,1,2 };
            for (int i = 2; i > 0; i--) std::swap(order[i], order[rand() % (i + 1)]);
            for (int b = 0; b < blocked; b++) {
                Train t{ LANE_X[order[b]], spawnZ + 4.5f, 1.18f, 4.8f, true };
                gTrainList.push_back(t);
            }
        }

        /* ======== GENERATION TERRAIN ======== */
        while (gTreeGenZ < gPlayer.z + 125.f) {
            float ofs = (float)(rand() % 4);
            gTreeList.push_back({ -TREE_X, gTreeGenZ });
            gTreeList.push_back({ TREE_X, gTreeGenZ });
            gTreeGenZ += 8.5f + ofs;
        }
        while (gGuardGenZ < gPlayer.z + 125.f) {
            gGuardList.push_back({ -GUARD_X, gGuardGenZ });
            gGuardList.push_back({ GUARD_X, gGuardGenZ });
            gGuardGenZ += 4.5f;
        }
        while (gBuildingGenZ < gPlayer.z + 130.f) {
            float side = (rand() % 2 == 0) ? -1.0f : 1.0f;
            float bx = side * (15.0f + (rand() % 13));   // 15 .. 28
            float bw = 3.0f + (rand() % 5) * 0.5f;      // 3.0 .. 5.0
            float bd = 2.5f + (rand() % 4) * 0.5f;      // 2.5 .. 4.0
            float bh = 4.0f + (rand() % 8) * 0.5f;      // 4.0 .. 7.5
            float r = 0.55f + (rand() % 25) * 0.018f;
            float g = 0.48f + (rand() % 25) * 0.018f;
            float bl = 0.52f + (rand() % 25) * 0.018f;
            gBuildingList.push_back({ bx, gBuildingGenZ, bw, bd, bh, r, g, bl });
            gBuildingGenZ += 25.0f + (rand() % 15);
        }

        /* ======== NETTOYAGE ======== */
        float cutZ = gPlayer.z - 40.f;
        gCoinList.erase(std::remove_if(gCoinList.begin(), gCoinList.end(),
            [&](const Coin& c) { return !c.alive || c.z < cutZ; }), gCoinList.end());
        gTrainList.erase(std::remove_if(gTrainList.begin(), gTrainList.end(),
            [&](const Train& t) { return !t.alive || t.z < cutZ; }), gTrainList.end());
        gTreeList.erase(std::remove_if(gTreeList.begin(), gTreeList.end(),
            [&](const Tree& t) { return t.z < cutZ; }), gTreeList.end());
        gGuardList.erase(std::remove_if(gGuardList.begin(), gGuardList.end(),
            [&](const Guard& g) { return g.z < cutZ; }), gGuardList.end());
        gBuildingList.erase(std::remove_if(gBuildingList.begin(), gBuildingList.end(),
            [&](const Building& b) { return b.z < cutZ; }), gBuildingList.end());

        /* ======== COLLISIONS ======== */
        /* Pieces */
        for (auto& c : gCoinList) {
            if (!c.alive) continue;
            if (fabsf(gPlayer.x - c.x) < 0.95f && fabsf(gPlayer.z - c.z) < 1.40f) {
                c.alive = false;
                gCoins++;
                gScore += 50;
                gCoinFlash = 0.4f;
            }
        }

        /* Trains */
        for (auto& t : gTrainList) {
            if (!t.alive) continue;
            if (fabsf(gPlayer.x - t.x) < (0.72f + t.halfW) &&
                fabsf(gPlayer.z - t.z) < (1.60f + t.halfL)) {
                gState = STATE_GAMEOVER;
            }
        }

        /* Score de distance */
        gScore += (int)(gSpeed * gDT * 0.9f);
    }

    glutPostRedisplay();
    glutTimerFunc(16, update, 0);   /* ~60 FPS */
}

// ================================================================
//  MAIN
// ================================================================
int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1024, 768);
    glutInitWindowPosition(80, 40);
    glutCreateWindow("Road Escape  |  Evitez les trains, semez la police!");

    initGL();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutKeyboardUpFunc(keyboardUp);
    glutSpecialFunc(special);
    glutTimerFunc(16, update, 0);

    glutMainLoop();
    return 0;
}