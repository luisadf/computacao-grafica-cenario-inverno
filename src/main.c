#include <GL/freeglut.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define PI 3.14159265358979323846
#define MAX_SNOW 700
#define SNOW_STEP 50
#define MIN_SNOW 50

typedef struct
{
    float x, y, z;
    float speed;
    float phase;
} Snowflake;

static Snowflake snow[MAX_SNOW];
static int snow_count = 250;
static int snow_enabled = 1;
static int night = 0;
static int house_lights = 1;
static int ornaments = 1;
static float camera_angle = 28.0f;
static float camera_height = 22.0f;
static float camera_distance = 31.0f;
static int window_width = 1100;
static int window_height = 720;
static int previous_time = 0;
static float animation_time = 0.0f;

static float random_between(float min, float max)
{
    return min + (max - min) * (float)rand() / (float)RAND_MAX;
}

static void reset_snowflake(Snowflake *flake, int random_height)
{
    flake->x = random_between(-17.0f, 17.0f);
    flake->y = random_height ? random_between(0.1f, 16.0f) : 16.0f;
    flake->z = random_between(-16.0f, 16.0f);
    flake->speed = random_between(1.0f, 2.8f);
    flake->phase = random_between(0.0f, (float)(2.0 * PI));
}

static void color(float r, float g, float b)
{
    glColor3f(r, g, b);
}

static void box(float x, float y, float z, float w, float h, float d)
{
    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(w, h, d);
    glutSolidCube(1.0);
    glPopMatrix();
}

static void sphere(float x, float y, float z, float radius, int detail)
{
    glPushMatrix();
    glTranslatef(x, y, z);
    glutSolidSphere(radius, detail, detail);
    glPopMatrix();
}

/* FreeGLUT cones point along Z; rotate them to point upwards. */
static void cone(float x, float y, float z, float radius, float height, int sides)
{
    glPushMatrix();
    glTranslatef(x, y, z);
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    glutSolidCone(radius, height, sides, 4);
    glPopMatrix();
}

static void cylinder(float x, float y, float z, float radius, float height)
{
    GLUquadric *quadric = gluNewQuadric();
    if (quadric == NULL)
    {
        fprintf(stderr, "Erro: nao foi possivel criar uma quadrica OpenGL.\n");
        exit(EXIT_FAILURE);
    }
    glPushMatrix();
    glTranslatef(x, y, z);
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    gluCylinder(quadric, radius, radius, height, 16, 1);
    glPopMatrix();
    gluDeleteQuadric(quadric);
}

static void set_emission(float r, float g, float b)
{
    GLfloat emission[] = {r, g, b, 1.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emission);
}

static void draw_path(float x1, float z1, float x2, float z2, float bend)
{
    int i, band;
    float dx = x2 - x1;
    float dz = z2 - z1;
    float length = sqrtf(dx * dx + dz * dz);
    float nx = -dz / length;
    float nz = dx / length;
    const float edge_position[] = {-1.0f, -0.55f, 0.55f, 1.0f};
    const float edge_alpha[] = {0.0f, 0.30f, 0.30f, 0.0f};

    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    for (band = 0; band < 3; ++band)
    {
        glBegin(GL_TRIANGLE_STRIP);
        for (i = 0; i <= 40; ++i)
        {
            int edge;
            float t = i / 40.0f;
            float curve = bend * sinf((float)PI * t) +
                          0.12f * sinf(3.0f * (float)PI * t);
            float width = 0.95f + 0.10f * sinf(t * 14.0f + x1);
            float x = x1 + dx * t + nx * curve;
            float z = z1 + dz * t + nz * curve;
            for (edge = band; edge <= band + 1; ++edge)
            {
                if (night)
                    glColor4f(0.31f, 0.39f, 0.52f, edge_alpha[edge]);
                else
                    glColor4f(0.57f, 0.67f, 0.72f, edge_alpha[edge]);
                glVertex3f(x + nx * width * edge_position[edge], 0.025f,
                           z + nz * width * edge_position[edge]);
            }
        }
        glEnd();
    }
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

static void draw_ground(void)
{
    int x, z;
    color(0.91f, 0.95f, 0.96f);
    glNormal3f(0.0f, 1.0f, 0.0f);
    glBegin(GL_QUADS);
    for (x = -200; x < 200; x += 10)
    {
        for (z = -200; z < 200; z += 10)
        {
            glVertex3f((float)x, 0.0f, (float)z);
            glVertex3f((float)x, 0.0f, (float)(z + 10));
            glVertex3f((float)(x + 10), 0.0f, (float)(z + 10));
            glVertex3f((float)(x + 10), 0.0f, (float)z);
        }
    }
    glEnd();

    draw_path(-3.2f, 0.0f, -2.0f, 2.6f, 0.15f);
    draw_path(-9.8f, -5.2f, -2.0f, 2.6f, -0.8f);
    draw_path(8.5f, -4.1f, 3.0f, 2.6f, 0.9f);
    draw_path(-2.0f, 2.6f, 3.0f, 2.6f, 0.25f);
    draw_path(-2.0f, 2.6f, -1.0f, 11.0f, 0.7f);
}

static void draw_mountain(float x, float z, float radius, float height)
{
    int ring, side, edge;
    const int rings = 32;
    const int sides = 48;
    /* Rounded rings with irregular outlines form a single continuous surface. */
    for (ring = 0; ring < rings; ++ring)
    {
        glBegin(GL_QUAD_STRIP);
        for (side = 0; side <= sides; ++side)
        {
            float a = (float)(2.0 * PI * side / sides);
            float uneven = 1.0f + 0.13f * sinf(3.0f * a + x)
                           + 0.06f * cosf(5.0f * a + z);
            for (edge = 0; edge <= 1; ++edge)
            {
                float t = (ring + edge) / (float)rings;
                float r = radius * t * uneven;
                float y = height * (1.0f - t * t) *
                          (1.0f - 0.14f * t * (1.0f - t) * (1.0f + sinf(3.0f * a + x)));
                float snowline = 0.70f + 0.035f * sinf(5.0f * a + x);
                if (y > height * snowline)
                    color(0.90f, 0.94f, 0.95f);
                else
                    color(0.43f + 0.04f * t, 0.54f + 0.04f * t, 0.58f + 0.04f * t);
                glNormal3f(2.0f * height * t * cosf(a) / radius, 1.0f,
                           2.0f * height * t * sinf(a) / radius);
                glVertex3f(x + r * cosf(a) + 1.2f * (1.0f - t),
                           y, z + r * sinf(a));
            }
        }
        glEnd();
    }
}

static void draw_pine(float x, float z, float scale, int decorated)
{
    int i, j;
    glPushMatrix();
    glTranslatef(x, 0.0f, z);
    glScalef(scale, scale, scale);
    color(0.32f, 0.19f, 0.11f);
    cylinder(0.0f, 0.0f, 0.0f, 0.22f, 1.0f);
    for (i = 0; i < 4; ++i)
    {
        float y = 0.65f + i * 0.68f;
        float radius = 1.15f - i * 0.18f;
        color(0.07f + 0.015f * i, 0.27f + 0.025f * i, 0.19f);
        cone(0.0f, y, 0.0f, radius, 1.55f, 16);
        color(0.87f, 0.94f, 0.93f);
        cone(0.0f, y + 0.98f, 0.0f, radius * 0.37f, 0.57f, 16);
        if (decorated && ornaments)
        {
            for (j = 0; j < 8; ++j)
            {
                float a = (float)(2.0 * PI * j / 8.0 + i * 0.45);
                float h = 0.27f + 0.15f * (j % 2);
                float r = radius * (1.0f - h / 1.55f) + 0.03f;
                static const float lights[4][3] = {
                    {1.0f, 0.12f, 0.08f}, {1.0f, 0.75f, 0.12f},
                    {0.15f, 0.45f, 1.0f}, {0.18f, 1.0f, 0.28f}};
                const float *bulb = lights[(j + i) % 4];
                int lit = ((int)(animation_time * 2.0f) + i + j) % 2 == 0;
                float strength = lit ? 0.75f : 0.08f;
                float bx = r * cosf(a);
                float bz = r * sinf(a);
                color(bulb[0] * (lit ? 1.0f : 0.4f),
                      bulb[1] * (lit ? 1.0f : 0.4f),
                      bulb[2] * (lit ? 1.0f : 0.4f));
                set_emission(bulb[0] * strength, bulb[1] * strength, bulb[2] * strength);
                sphere(bx, y + h, bz, 0.095f, 10);
                set_emission(0.0f, 0.0f, 0.0f);
                if (lit && night)
                {
                    glDisable(GL_LIGHTING);
                    glEnable(GL_BLEND);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                    glDepthMask(GL_FALSE);
                    glColor4f(bulb[0], bulb[1], bulb[2], 0.18f);
                    sphere(bx, y + h, bz, 0.16f, 10);
                    glDepthMask(GL_TRUE);
                    glDisable(GL_BLEND);
                    glEnable(GL_LIGHTING);
                }
            }
        }
    }
    if (decorated && ornaments)
    {
        color(1.0f, 0.77f, 0.19f);
        set_emission(night ? 0.35f : 0.08f, night ? 0.22f : 0.05f, 0.0f);
        glPushMatrix();
        glTranslatef(0.0f, 4.35f, 0.0f);
        glBegin(GL_TRIANGLE_FAN);
        glNormal3f(0.0f, 0.0f, 1.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        for (i = 0; i <= 10; ++i)
        {
            float a = (float)(PI / 2.0 + 2.0 * PI * i / 10.0);
            float r = (i % 2 == 0) ? 0.34f : 0.14f;
            glVertex3f(r * cosf(a), r * sinf(a), 0.0f);
        }
        glEnd();
        glPopMatrix();
        set_emission(0.0f, 0.0f, 0.0f);
    }
    glPopMatrix();
}

static void draw_roof(void)
{
    /* The two roof slopes and gables share the cabin's local coordinates. */
    color(0.39f, 0.18f, 0.12f);
    glBegin(GL_QUADS);
    glNormal3f(-0.65f, 0.76f, 0.0f);
    glVertex3f(-2.2f, 2.54f, -1.85f);
    glVertex3f(-2.2f, 2.54f, 1.85f);
    glVertex3f(0.0f, 4.44f, 1.85f);
    glVertex3f(0.0f, 4.44f, -1.85f);
    glNormal3f(0.65f, 0.76f, 0.0f);
    glVertex3f(0.0f, 4.44f, -1.85f);
    glVertex3f(0.0f, 4.44f, 1.85f);
    glVertex3f(2.2f, 2.54f, 1.85f);
    glVertex3f(2.2f, 2.54f, -1.85f);
    glEnd();
    color(0.56f, 0.32f, 0.20f);
    glBegin(GL_TRIANGLES);
    glNormal3f(0.0f, 0.0f, 1.0f);
    glVertex3f(-1.9f, 2.8f, 1.61f);
    glVertex3f(1.9f, 2.8f, 1.61f);
    glVertex3f(0.0f, 4.44f, 1.61f);
    glNormal3f(0.0f, 0.0f, -1.0f);
    glVertex3f(1.9f, 2.8f, -1.61f);
    glVertex3f(-1.9f, 2.8f, -1.61f);
    glVertex3f(0.0f, 4.44f, -1.61f);
    glEnd();
    color(0.93f, 0.97f, 0.98f);
    glBegin(GL_QUADS);
    glNormal3f(-0.65f, 0.76f, 0.0f);
    glVertex3f(-2.22f, 2.62f, -1.90f);
    glVertex3f(-2.22f, 2.62f, 1.90f);
    glVertex3f(0.0f, 4.52f, 1.90f);
    glVertex3f(0.0f, 4.52f, -1.90f);
    glNormal3f(0.65f, 0.76f, 0.0f);
    glVertex3f(0.0f, 4.52f, -1.90f);
    glVertex3f(0.0f, 4.52f, 1.90f);
    glVertex3f(2.22f, 2.62f, 1.90f);
    glVertex3f(2.22f, 2.62f, -1.90f);
    glEnd();
}

static void draw_window(float x, float y, float z)
{
    color(0.26f, 0.14f, 0.09f);
    box(x, y, z, 0.91f, 1.05f, 0.08f);
    if (house_lights)
    {
        color(1.0f, 0.75f, 0.30f);
        set_emission(0.52f, 0.30f, 0.07f);
    }
    else
    {
        color(0.22f, 0.39f, 0.46f);
    }
    box(x, y, z + 0.05f, 0.75f, 0.88f, 0.05f);
    set_emission(0.0f, 0.0f, 0.0f);
    color(0.40f, 0.22f, 0.13f);
    box(x, y, z + 0.09f, 0.07f, 0.95f, 0.05f);
    box(x, y, z + 0.09f, 0.81f, 0.07f, 0.05f);
    color(0.92f, 0.96f, 0.97f);
    box(x, y - 0.54f, z + 0.10f, 1.0f, 0.10f, 0.20f);
}

static void draw_house(float x, float z, float scale)
{
    int i;
    glPushMatrix();
    glTranslatef(x, 0.0f, z);
    glScalef(scale, scale, scale);
    color(0.59f, 0.34f, 0.20f);
    box(0.0f, 1.4f, 0.0f, 3.8f, 2.8f, 3.2f);
    color(0.42f, 0.23f, 0.14f);
    for (i = 0; i < 8; ++i)
    {
        box(0.0f, 0.15f + i * 0.35f, 1.61f, 3.8f, 0.035f, 0.035f);
        box(1.91f, 0.15f + i * 0.35f, 0.0f, 0.035f, 0.035f, 3.2f);
    }
    color(0.35f, 0.19f, 0.11f);
    box(-1.83f, 1.4f, 1.63f, 0.15f, 2.8f, 0.15f);
    box(1.83f, 1.4f, 1.63f, 0.15f, 2.8f, 0.15f);
    draw_roof();
    color(0.48f, 0.28f, 0.23f);
    box(1.08f, 4.05f, -0.55f, 0.55f, 1.45f, 0.55f);
    color(0.93f, 0.96f, 0.97f);
    box(1.08f, 4.80f, -0.55f, 0.68f, 0.13f, 0.68f);
    color(0.25f, 0.13f, 0.08f);
    box(0.0f, 1.0f, 1.64f, 0.90f, 2.0f, 0.12f);
    color(0.49f, 0.29f, 0.17f);
    box(0.0f, 1.0f, 1.72f, 0.74f, 1.84f, 0.06f);
    color(0.94f, 0.72f, 0.26f);
    sphere(0.26f, 0.93f, 1.77f, 0.065f, 10);
    draw_window(-1.2f, 1.55f, 1.65f);
    draw_window(1.2f, 1.55f, 1.65f);
    color(0.79f, 0.85f, 0.84f);
    box(0.0f, 0.12f, 1.96f, 1.35f, 0.24f, 0.55f);
    color(0.94f, 0.97f, 0.98f);
    box(0.0f, 0.25f, 1.96f, 1.35f, 0.05f, 0.55f);
    glPopMatrix();
}

static void draw_snowman(void)
{
    int i;
    glPushMatrix();
    glTranslatef(1.0f, 0.0f, 4.0f);
    color(0.94f, 0.97f, 0.98f);
    sphere(0.0f, 0.70f, 0.0f, 0.70f, 20);
    sphere(0.0f, 1.65f, 0.0f, 0.52f, 20);
    sphere(0.0f, 2.40f, 0.0f, 0.40f, 20);
    color(0.13f, 0.17f, 0.19f);
    for (i = 0; i < 3; ++i)
        sphere(0.0f, 1.38f + i * 0.24f, 0.49f, 0.06f, 10);
    sphere(-0.13f, 2.49f, 0.35f, 0.055f, 10);
    sphere(0.13f, 2.49f, 0.35f, 0.055f, 10);
    color(0.94f, 0.40f, 0.08f);
    glPushMatrix();
    glTranslatef(0.0f, 2.37f, 0.37f);
    glutSolidCone(0.10, 0.42, 12, 2);
    glPopMatrix();
    color(0.70f, 0.10f, 0.12f);
    cylinder(0.0f, 2.02f, 0.0f, 0.43f, 0.15f);
    box(0.23f, 1.79f, 0.43f, 0.20f, 0.53f, 0.09f);
    color(0.12f, 0.16f, 0.18f);
    cylinder(0.0f, 2.73f, 0.0f, 0.46f, 0.10f);
    cylinder(0.0f, 2.82f, 0.0f, 0.29f, 0.40f);
    color(0.72f, 0.16f, 0.12f);
    cylinder(0.0f, 2.86f, 0.0f, 0.30f, 0.09f);
    color(0.37f, 0.23f, 0.13f);
    glPushMatrix();
    glTranslatef(-0.48f, 1.80f, 0.0f);
    glRotatef(55.0f, 0.0f, 0.0f, 1.0f);
    cylinder(0.0f, 0.0f, 0.0f, 0.055f, 0.95f);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(0.48f, 1.80f, 0.0f);
    glRotatef(-55.0f, 0.0f, 0.0f, 1.0f);
    cylinder(0.0f, 0.0f, 0.0f, 0.055f, 0.95f);
    glPopMatrix();
    glPopMatrix();
}

static void draw_fence(void)
{
    int i;
    color(0.49f, 0.30f, 0.18f);
    for (i = 0; i < 7; ++i)
    {
        float x = -10.0f + i * 1.15f;
        box(x, 0.6f, 4.7f, 0.16f, 1.2f, 0.16f);
        color(0.94f, 0.97f, 0.98f);
        box(x, 1.22f, 4.7f, 0.24f, 0.08f, 0.24f);
        color(0.49f, 0.30f, 0.18f);
    }
    box(-6.55f, 0.45f, 4.7f, 6.9f, 0.12f, 0.12f);
    box(-6.55f, 0.88f, 4.7f, 6.9f, 0.12f, 0.12f);
}

static void draw_bench(float x, float z, float angle)
{
    int i;
    glPushMatrix();
    glTranslatef(x, 0.0f, z);
    glRotatef(angle, 0.0f, 1.0f, 0.0f);
    color(0.35f, 0.21f, 0.14f);
    for (i = -1; i <= 1; i += 2)
    {
        box(i * 0.67f, 0.36f, 0.0f, 0.14f, 0.72f, 0.48f);
        box(i * 0.67f, 0.95f, -0.22f, 0.12f, 1.15f, 0.12f);
    }
    color(0.55f, 0.34f, 0.20f);
    box(0.0f, 0.76f, 0.0f, 1.75f, 0.13f, 0.58f);
    box(0.0f, 1.15f, -0.23f, 1.75f, 0.36f, 0.12f);
    color(0.90f, 0.95f, 0.96f);
    box(0.0f, 1.35f, -0.23f, 1.8f, 0.06f, 0.16f);
    glPopMatrix();
}

static void draw_lamp(float x, float z)
{
    color(0.17f, 0.19f, 0.20f);
    cylinder(x, 0.0f, z, 0.10f, 2.8f);
    box(x, 0.10f, z, 0.38f, 0.20f, 0.38f);
    box(x, 2.70f, z, 0.52f, 0.12f, 0.52f);
    if (night)
    {
        color(1.0f, 0.76f, 0.35f);
        set_emission(0.65f, 0.38f, 0.10f);
    }
    else
        color(0.83f, 0.82f, 0.65f);
    box(x, 3.02f, z, 0.36f, 0.55f, 0.36f);
    set_emission(0.0f, 0.0f, 0.0f);
    color(0.17f, 0.19f, 0.20f);
    box(x, 3.33f, z, 0.52f, 0.10f, 0.52f);
    color(0.92f, 0.96f, 0.97f);
    box(x, 3.40f, z, 0.55f, 0.07f, 0.55f);
}

static void draw_present(float x, float z, float size, int red)
{
    if (red)
        color(0.73f, 0.12f, 0.14f);
    else
        color(0.16f, 0.40f, 0.58f);
    box(x, size * 0.5f, z, size, size, size);
    color(0.95f, 0.73f, 0.28f);
    box(x, size * 0.5f, z, size * 0.16f, size * 1.02f, size * 1.02f);
    box(x, size * 0.5f, z, size * 1.02f, size * 1.02f, size * 0.16f);
    sphere(x - size * 0.12f, size * 1.07f, z, size * 0.13f, 10);
    sphere(x + size * 0.12f, size * 1.07f, z, size * 0.13f, 10);
}

static void draw_chimney_smoke(float x, float z, float scale)
{
    int puff, vertex;
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    for (puff = 0; puff < 6; ++puff)
    {
        float progress = fmodf(animation_time * 0.12f + puff / 6.0f +
                               fabsf(x) * 0.03f, 1.0f);
        float radius = scale * (0.18f + 0.38f * progress);
        float alpha = 0.30f * sinf((float)PI * progress);
        glPushMatrix();
        glTranslatef(x + scale * (1.08f + 0.3f * progress),
                     scale * (4.88f + 2.7f * progress),
                     z - scale * 0.55f);
        glRotatef(camera_angle, 0.0f, 1.0f, 0.0f);
        glRotatef(-camera_height, 1.0f, 0.0f, 0.0f);
        glBegin(GL_TRIANGLE_FAN);
        glColor4f(0.83f, 0.88f, 0.91f, alpha);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glColor4f(0.83f, 0.88f, 0.91f, 0.0f);
        for (vertex = 0; vertex <= 24; ++vertex)
        {
            float a = (float)(2.0 * PI * vertex / 24.0);
            glVertex3f(radius * cosf(a), radius * sinf(a), 0.0f);
        }
        glEnd();
        glPopMatrix();
    }
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

static void draw_sky_details(void)
{
    int i;
    glDisable(GL_LIGHTING);
    if (night)
    {
        color(0.95f, 0.96f, 0.80f);
        sphere(-27.0f, 30.0f, -65.0f, 2.8f, 20);
        glPointSize(2.5f);
        glBegin(GL_POINTS);
        for (i = 0; i < 65; ++i)
        {
            float x = (float)((i * 73) % 120 - 60);
            float y = 12.0f + (float)((i * 37) % 420) / 10.0f;
            float z = -70.0f + (float)((i * 11) % 8);
            glVertex3f(x, y, z);
        }
        glEnd();
    }
    else
    {
        color(1.0f, 0.90f, 0.58f);
        sphere(-27.0f, 30.0f, -65.0f, 3.2f, 20);
    }
    glEnable(GL_LIGHTING);
}

static void draw_snow(void)
{
    int i;
    if (!snow_enabled)
        return;
    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 1.0f, 1.0f);
    glPointSize(3.0f);
    glBegin(GL_POINTS);
    for (i = 0; i < snow_count; ++i)
        glVertex3f(snow[i].x, snow[i].y, snow[i].z);
    glEnd();
    glEnable(GL_LIGHTING);
}

static void text(int x, int y, const char *message)
{
    const unsigned char *p = (const unsigned char *)message;
    glRasterPos2i(x, y);
    while (*p)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *p++);
}

static void draw_hud(void)
{
    char status[160];
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0.0, window_width, 0.0, window_height);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.04f, 0.09f, 0.14f, 0.72f);
    glBegin(GL_QUADS);
    glVertex2i(12, window_height - 12);
    glVertex2i(500, window_height - 12);
    glVertex2i(500, window_height - 83);
    glVertex2i(12, window_height - 83);
    glEnd();
    color(1.0f, 1.0f, 1.0f);
    text(23, window_height - 31, "CENARIO DE INVERNO 3D");
    text(23, window_height - 48, "D: dia/noite   N: neve   +/-: intensidade   L: luzes   T: enfeites");
    snprintf(status, sizeof(status),
             "Setas: girar camera   Page Up/Down: zoom   Esc: sair   |   %s  Neve: %s (%d)",
             night ? "Noite" : "Dia", snow_enabled ? "ligada" : "desligada", snow_count);
    text(23, window_height - 67, status);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

static void display(void)
{
    int i;
    float angle = camera_angle * (float)PI / 180.0f;
    float elevation = camera_height * (float)PI / 180.0f;
    GLfloat daylight[] = {0.75f, 0.75f, 0.72f, 1.0f};
    GLfloat moonlight[] = {0.28f, 0.36f, 0.55f, 1.0f};
    GLfloat ambient_day[] = {0.32f, 0.32f, 0.32f, 1.0f};
    GLfloat ambient_night[] = {0.13f, 0.16f, 0.22f, 1.0f};
    GLfloat light_position[] = {-8.0f, 15.0f, 5.0f, 0.0f};
    GLfloat warm_light[] = {0.75f, 0.40f, 0.14f, 1.0f};
    GLfloat fog_day[] = {0.53f, 0.76f, 0.87f, 1.0f};
    GLfloat fog_night[] = {0.035f, 0.065f, 0.15f, 1.0f};
    GLfloat village_lights[5][4] = {
        {-3.2f, 2.0f, 0.0f, 1.0f}, {-9.8f, 1.6f, -5.4f, 1.0f},
        {8.5f, 1.7f, -4.3f, 1.0f}, {-5.6f, 3.0f, 2.1f, 1.0f},
        {5.8f, 3.0f, 3.0f, 1.0f}};

    if (night)
        glClearColor(0.035f, 0.065f, 0.15f, 1.0f);
    else
        glClearColor(0.53f, 0.76f, 0.87f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    gluLookAt(camera_distance * cosf(elevation) * sinf(angle),
              2.7f + camera_distance * sinf(elevation),
              -1.5f + camera_distance * cosf(elevation) * cosf(angle),
              0.0f, 2.7f, -1.5f, 0.0f, 1.0f, 0.0f);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, night ? moonlight : daylight);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, night ? ambient_night : ambient_day);
    for (i = 0; i < 5; ++i)
    {
        GLenum light = (GLenum)(GL_LIGHT1 + i);
        glLightfv(light, GL_POSITION, village_lights[i]);
        glLightfv(light, GL_DIFFUSE, warm_light);
        glLightf(light, GL_CONSTANT_ATTENUATION, 1.0f);
        glLightf(light, GL_LINEAR_ATTENUATION, 0.18f);
        glLightf(light, GL_QUADRATIC_ATTENUATION, 0.12f);
        if (night && (i >= 3 || house_lights))
            glEnable(light);
        else
            glDisable(light);
    }

    draw_sky_details();
    glEnable(GL_FOG);
    glFogi(GL_FOG_MODE, GL_LINEAR);
    glFogfv(GL_FOG_COLOR, night ? fog_night : fog_day);
    glFogf(GL_FOG_START, 65.0f);
    glFogf(GL_FOG_END, 150.0f);
    draw_ground();
    draw_mountain(-25.0f, -38.0f, 17.0f, 11.5f);
    draw_mountain(-10.0f, -42.0f, 14.5f, 15.0f);
    draw_mountain(8.0f, -41.0f, 19.0f, 12.5f);
    draw_mountain(26.0f, -37.0f, 15.5f, 13.5f);
    draw_pine(-15.0f, -10.0f, 1.15f, 0);
    draw_pine(-13.5f, -14.0f, 0.85f, 0);
    draw_pine(-5.0f, -13.0f, 1.05f, 0);
    draw_pine(3.0f, -13.5f, 1.1f, 0);
    draw_pine(14.0f, -10.0f, 1.3f, 0);
    draw_pine(14.0f, 1.0f, 0.9f, 0);
    draw_pine(-13.0f, 2.0f, 0.8f, 0);
    draw_house(-3.2f, -2.0f, 1.0f);
    draw_house(-9.8f, -7.0f, 0.8f);
    draw_house(8.5f, -6.0f, 0.85f);
    draw_pine(3.5f, -0.5f, 1.25f, 1);
    draw_present(2.35f, 0.45f, 0.55f, 1);
    draw_present(4.0f, 0.75f, 0.45f, 0);
    draw_snowman();
    draw_fence();
    draw_bench(1.0f, 7.0f, -15.0f);
    draw_bench(10.0f, 1.0f, 15.0f);
    draw_lamp(-5.6f, 2.1f);
    draw_lamp(5.8f, 3.0f);
    draw_chimney_smoke(-3.2f, -2.0f, 1.0f);
    draw_chimney_smoke(-9.8f, -7.0f, 0.8f);
    draw_chimney_smoke(8.5f, -6.0f, 0.85f);
    draw_snow();
    glDisable(GL_FOG);
    draw_hud();
    glutSwapBuffers();
}

static void reshape(int width, int height)
{
    if (height < 1)
        height = 1;
    if (width < 1)
        width = 1;
    window_width = width;
    window_height = height;
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(48.0, (double)width / (double)height, 0.1, 250.0);
    glMatrixMode(GL_MODELVIEW);
}

static void keyboard(unsigned char key, int x, int y)
{
    (void)x;
    (void)y;
    switch (key)
    {
    case 'd':
    case 'D':
        night = !night;
        break;
    case 'n':
    case 'N':
        snow_enabled = !snow_enabled;
        break;
    case 'l':
    case 'L':
        house_lights = !house_lights;
        break;
    case 't':
    case 'T':
        ornaments = !ornaments;
        break;
    case '+':
    case '=':
        if (snow_count < MAX_SNOW)
            snow_count += SNOW_STEP;
        break;
    case '-':
    case '_':
        if (snow_count > MIN_SNOW)
            snow_count -= SNOW_STEP;
        break;
    case 27:
        glutLeaveMainLoop();
        return;
    default:
        return;
    }
    glutPostRedisplay();
}

static void special_keys(int key, int x, int y)
{
    (void)x;
    (void)y;
    switch (key)
    {
    case GLUT_KEY_LEFT:
        camera_angle -= 5.0f;
        break;
    case GLUT_KEY_RIGHT:
        camera_angle += 5.0f;
        break;
    case GLUT_KEY_UP:
        if (camera_height < 75.0f)
            camera_height += 3.0f;
        break;
    case GLUT_KEY_DOWN:
        if (camera_height > 7.0f)
            camera_height -= 3.0f;
        break;
    case GLUT_KEY_PAGE_UP:
        if (camera_distance > 10.0f)
            camera_distance -= 1.0f;
        break;
    case GLUT_KEY_PAGE_DOWN:
        if (camera_distance < 35.0f)
            camera_distance += 1.0f;
        break;
    default:
        return;
    }
    if (camera_height > 75.0f)
        camera_height = 75.0f;
    if (camera_height < 7.0f)
        camera_height = 7.0f;
    glutPostRedisplay();
}

static void timer(int value)
{
    int i;
    int now = glutGet(GLUT_ELAPSED_TIME);
    float dt = (now - previous_time) / 1000.0f;
    (void)value;
    previous_time = now;
    if (dt < 0.0f)
        dt = 0.0f;
    if (dt > 0.05f)
        dt = 0.05f;
    animation_time += dt;
    if (snow_enabled)
    {
        for (i = 0; i < MAX_SNOW; ++i)
        {
            snow[i].y -= snow[i].speed * dt;
            snow[i].x += 0.18f * sinf(animation_time + snow[i].phase) * dt;
            if (snow[i].y < 0.0f)
                reset_snowflake(&snow[i], 0);
        }
    }
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

int main(int argc, char **argv)
{
    int i;
    srand(42);
    for (i = 0; i < MAX_SNOW; ++i)
        reset_snowflake(&snow[i], 1);
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(window_width, window_height);
    glutCreateWindow("Cenario de Inverno 3D");
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glShadeModel(GL_SMOOTH);
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special_keys);
    previous_time = glutGet(GLUT_ELAPSED_TIME);
    glutTimerFunc(16, timer, 0);
    glutMainLoop();
    return 0;
}
