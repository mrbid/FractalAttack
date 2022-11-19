/*
    James William Fletcher (github.com/mrbid)
        November 2022
*/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define uint GLushort
#define sint GLshort
#define f32 GLfloat

#include "inc/gl.h"
#define GLFW_INCLUDE_NONE
#include "inc/glfw3.h"

#ifndef __x86_64__
    #define NOSSE
#endif

#define SEIR_RAND

#include "inc/esAux2.h"
#include "inc/res.h"
#include "ncube.h"
#include "exo.h"
#include "inner.h"

//*************************************
// globals
//*************************************
GLFWwindow* window;
uint winw = 1024;
uint winh = 768;
double t = 0;   // time
f32 dt = 0;     // delta time
double fc = 0;  // frame count
double lfct = 0;// last frame count time
f32 aspect;
double x,y,lx,ly;
double rww, ww, rwh, wh, ww2, wh2;
double uw, uh, uw2, uh2; // normalised pixel dpi

// render state id's
GLint projection_id;
GLint modelview_id;
GLint normalmat_id = -1;
GLint position_id;
GLint lightpos_id;
GLint solidcolor_id;
GLint color_id;
GLint opacity_id;
GLint normal_id; // 

// render state matrices
mat projection;
mat view;
mat model;
mat modelview;

// models
ESModel mdlMenger;
ESModel mdlExo;
ESModel mdlInner;

// camera vars
#define FAR_DISTANCE 10000.f
vec lightpos = {0.f, 0.f, 0.f};
uint focus_cursor = 0;
double sens = 0.001f;
f32 xrot = 0.f;
f32 yrot = 0.f;

// game vars
#define GFX_SCALE 0.01f
#define MOVE_SPEED 0.003f
uint keystate[6] = {0};
vec pp = {0.f, 0.f, 0.f};
vec ppr = {0.f, 0.f, -2.0f};
uint hits = 0;
uint brake = 0;

typedef struct
{
    vec dir, pos;
    f32 rot, r,g,b;
} comet;
#define NUM_COMETS 9
comet comets[NUM_COMETS];

//*************************************
// utility functions
//*************************************
void timestamp(char* ts)
{
    const time_t tt = time(0);
    strftime(ts, 16, "%H:%M:%S", localtime(&tt));
}
float clamp(float f, float min, float max)
{
    if(f > max){return max;}
    else if(f < min){return min;}
    return f;
}
void scaleBuffer(GLfloat* b, GLsizeiptr s)
{
    for(GLsizeiptr i = 0; i < s; i++)
        b[i] *= GFX_SCALE;
}
void doExoImpact(vec p, float f)
{
    GLsizeiptr s = exo_numvert*3;
    for(GLsizeiptr i = 0; i < s; i+=3)
    {
        vec v = {exo_vertices[i], exo_vertices[i+1], exo_vertices[i+2]};
        const f32 ds = vDist(v, p);
        if(ds < f)
        {
            vNorm(&v);
            vMulS(&v, v, f-ds);
            exo_vertices[i]   -= v.x;
            exo_vertices[i+1] -= v.y;
            exo_vertices[i+2] -= v.z;
            exo_colors[i]   -= 0.2f;
            exo_colors[i+1] -= 0.2f;
            exo_colors[i+2] -= 0.2f;
        }
    }
    esRebind(GL_ARRAY_BUFFER, &mdlExo.vid, exo_vertices, sizeof(exo_vertices), GL_STATIC_DRAW);
    esRebind(GL_ARRAY_BUFFER, &mdlExo.cid, exo_colors, sizeof(exo_colors), GL_STATIC_DRAW);
}
void randComet(uint i)
{
    vRuvBT(&comets[i].pos);
    vMulS(&comets[i].pos, comets[i].pos, 10.f);

    vec dp;
    vRuvTA(&dp);
    vSub(&comets[i].dir, comets[i].pos, dp);
    vNorm(&comets[i].dir);
    vInv(&comets[i].dir);

    vec of = comets[i].dir;
    vInv(&of);
    vMulS(&of, of, randf()*6.f);
    vAdd(&comets[i].pos, comets[i].pos, of);

    comets[i].rot = randfc();
    comets[i].r = randfc();
    comets[i].g = randfc();
    comets[i].b = randfc();
}
void randComets()
{
    for(uint i = 0; i < NUM_COMETS; i++)
        randComet(i);
}

//*************************************
// update & render
//*************************************
void main_loop()
{
//*************************************
// time delta for frame interpolation
//*************************************
    static double lt = 0;
    dt = t-lt;
    lt = t;

//*************************************
// keystates
//*************************************

    static vec vd;
    mGetViewDir(&vd, view);

    static uint inverted = 0;
    if(view.m[1][1] < 0.f) // could probably do some bit magic here to transfer the signed part of the view float to a 1 float to avoid the branching and turn it into a multiply op to invert the signs
        inverted = 1;
    else
        inverted = 0;

    static uint locked = 0;
    if(view.m[1][1] > -0.03f && view.m[1][1] < 0.03f)
        locked = 1;
    else
        locked = 0;

    if(keystate[2] == 1) // W
    {
        vec m;
        vMulS(&m, vd, MOVE_SPEED * dt);
        vSub(&pp, pp, m);
    }

    if(keystate[3] == 1) // S
    {
        vec m;
        vMulS(&m, vd, MOVE_SPEED * dt);
        vAdd(&pp, pp, m);
    }

    if(keystate[0] == 1) // A
    {
        vec vdc;
        vCross(&vdc, vd, (vec){0.f, 1.f, 0.f});
        vec m;
        vMulS(&m, vdc, MOVE_SPEED * dt);
        if(inverted == 0)
            vAdd(&pp, pp, m);
        else
            vSub(&pp, pp, m);
    }

    if(keystate[1] == 1) // D
    {
        vec vdc;
        vCross(&vdc, vd, (vec){0.f, 1.f, 0.f});
        vec m;
        vMulS(&m, vdc, MOVE_SPEED * dt);
        if(inverted == 0)
            vSub(&pp, pp, m);
        else
            vAdd(&pp, pp, m);
    }

    if(keystate[4] == 1) // SPACE
    {
        vec vdc;
        vCross(&vdc, vd, (vec){0.f, 1.f, 0.f});
        vCross(&vdc, vd, vdc);
        vec m;
        vMulS(&m, vdc, MOVE_SPEED * dt);
        if(inverted == 0)
            vAdd(&pp, pp, m);
        else
            vSub(&pp, pp, m);
    }

    if(keystate[5] == 1) // SHIFT
    {
        vec vdc;
        vCross(&vdc, vd, (vec){0.f, 1.f, 0.f});
        vCross(&vdc, vd, vdc);
        vec m;
        vMulS(&m, vdc, MOVE_SPEED * dt);
        if(inverted == 0)
            vSub(&pp, pp, m);
        else
            vAdd(&pp, pp, m);
    }

    if(brake == 1)
        vMulS(&pp, pp, 0.99f*(1.f-dt));

    vAdd(&ppr, ppr, pp);

    const f32 pmod = vMod(ppr);
    if(pmod < 1.14f)
    {
        const f32 hf = vMod(pp)*10.f;
        vec n = ppr;
        vNorm(&n);
         vReflect(&pp, pp, (vec){-n.x, -n.y, -n.z}); // better if I don't normalise pp
         vMulS(&pp, pp, 0.3f);
        vMulS(&n, n, 1.14f - pmod);
        vAdd(&ppr, ppr, n);

        doExoImpact((vec){-ppr.x, -ppr.y, -ppr.z}, hf);
        hits++;
    }

//*************************************
// camera
//*************************************

    if(focus_cursor == 1)
    {
        glfwGetCursorPos(window, &x, &y);
        
        if(locked == 0)
        {
            if(inverted == 0)
                xrot += (ww2-x)*sens;
            else
                xrot -= (ww2-x)*sens;
        }
        yrot += (wh2-y)*sens;

        // if(yrot > d2PI)
        //     yrot = d2PI;
        // if(yrot < -d2PI)
        //     yrot = -d2PI;

        glfwSetCursorPos(window, ww2, wh2);
    }

    mIdent(&view);
    mRotate(&view, yrot, 1.f, 0.f, 0.f);
    mRotate(&view, xrot, 0.f, 1.f, 0.f);
    // mRotY(&view, yrot);
    // mRotX(&view, xrot);
    mTranslate(&view, ppr.x, ppr.y, ppr.z);

    static f32 ft = 0.f;
    ft += dt*0.03f;
    lightpos.x = sinf(ft) * 6.3f;
    lightpos.y = cosf(ft) * 6.3f;
    lightpos.z = sinf(ft) * 6.3f;

//*************************************
// render
//*************************************
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ///
    
    shadeLambert2(&position_id, &projection_id, &modelview_id, &lightpos_id, &color_id, &opacity_id);
    glUniformMatrix4fv(projection_id, 1, GL_FALSE, (GLfloat*) &projection.m[0][0]);
    glUniform3f(lightpos_id, lightpos.x, lightpos.y, lightpos.z);
    glUniform1f(opacity_id, 1.f);
    normalmat_id = -1;
    
    ///

    glBindBuffer(GL_ARRAY_BUFFER, mdlExo.vid);
    glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(position_id);

    glBindBuffer(GL_ARRAY_BUFFER, mdlExo.cid);
    glVertexAttribPointer(color_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(color_id);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdlExo.iid);

    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (GLfloat*) &view.m[0][0]);
    glDrawElements(GL_TRIANGLES, exo_numind, GL_UNSIGNED_INT, 0);

    /// the inner wont draw now if occluded by the exo due to depth buffer
    
    if(hits > 0)
    {
        glBindBuffer(GL_ARRAY_BUFFER, mdlInner.vid);
        glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(position_id);

        glBindBuffer(GL_ARRAY_BUFFER, mdlInner.cid);
        glVertexAttribPointer(color_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(color_id);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdlExo.iid);

        glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (GLfloat*) &view.m[0][0]);
        glDrawElements(GL_TRIANGLES, exo_numind, GL_UNSIGNED_INT, 0);
    }

    ///

    // lambert
    shadeLambert(&position_id, &projection_id, &modelview_id, &lightpos_id, &color_id, &opacity_id);
    glUniformMatrix4fv(projection_id, 1, GL_FALSE, (GLfloat*) &projection.m[0][0]);
    glUniform3f(lightpos_id, lightpos.x, lightpos.y, lightpos.z);
    glUniform1f(opacity_id, 1.f);

    // bind menger
    glBindBuffer(GL_ARRAY_BUFFER, mdlMenger.vid);
    glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(position_id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdlMenger.iid);

    // "light source" dummy object
    mIdent(&model);
    mTranslate(&model, lightpos.x, lightpos.y, lightpos.z);
    mScale(&model, 3.4f, 3.4f, 3.4f);
    glUniform3f(color_id, 1.f, 1.f, 0.f);
    mMul(&modelview, &model, &view);
    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);
    if(normalmat_id != -1)
    {
        mat inverted, normalmat;
        mIdent(&inverted);
        mIdent(&normalmat);
        mInvert(&inverted.m[0][0], &modelview.m[0][0]);
        mTranspose(&normalmat, &inverted);
        glUniformMatrix4fv(normalmat_id, 1, GL_FALSE, (GLfloat*) &normalmat.m[0][0]);
    }
    glDrawElements(GL_TRIANGLES, ncube_numind, GL_UNSIGNED_INT, 0);

    // phong
    shadePhong(&position_id, &projection_id, &modelview_id, &normalmat_id, &lightpos_id, &color_id, &opacity_id);
    glUniformMatrix4fv(projection_id, 1, GL_FALSE, (GLfloat*) &projection.m[0][0]);
    glUniform3f(lightpos_id, lightpos.x, lightpos.y, lightpos.z);

    // comets
    glEnable(GL_BLEND);
    for(uint i = 0; i < NUM_COMETS; i++)
    {
        if(comets[i].r == -1337){continue;}
        
        const f32 pi = 0.16f*dt;
        vAdd(&comets[i].pos, comets[i].pos, (vec){comets[i].dir.x*pi,  comets[i].dir.y*pi, comets[i].dir.z*pi});
        comets[i].rot += 0.7f*dt;

        if(vMod(comets[i].pos) < 1.14f)
        {
            comets[i].r = -1337;
            doExoImpact((vec){comets[i].pos.x, comets[i].pos.y, comets[i].pos.z}, 0.1f);
            hits++;
            randComet(i);
            continue;
        }

        for(int j = 0; j < 8; j++)
        {
            const float fi = (float)j;
            const float sc = 1.f - (1.f*(fi*0.1f));

            mIdent(&model);
            vec tp = comets[i].dir;
            vMulS(&tp, tp, fi*0.03f);
            vSub(&tp, comets[i].pos, tp);
            mTranslate(&model, tp.x, tp.y, tp.z);
            mScale(&model, sc, sc, sc);
            mRotate(&model, comets[i].rot+(fi*0.1f), 1.f, 0.f, 0.f);
            mRotate(&model, comets[i].rot, 0.f, 1.f, 0.f);

            mMul(&modelview, &model, &view);
            glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);
            if(normalmat_id != -1)
            {
                mat inverted, normalmat;
                mIdent(&inverted);
                mIdent(&normalmat);
                mInvert(&inverted.m[0][0], &modelview.m[0][0]);
                mTranspose(&normalmat, &inverted);
                glUniformMatrix4fv(normalmat_id, 1, GL_FALSE, (GLfloat*) &normalmat.m[0][0]);
            }

            comets[i].r += randfc()*dt*0.1f;
            comets[i].g += randfc()*dt*0.1f;
            comets[i].b += randfc()*dt*0.1f;
            comets[i].r += randfc()*(fi*0.007f);
            comets[i].g += randfc()*(fi*0.007f);
            comets[i].b += randfc()*(fi*0.007f);
            comets[i].r = clamp(comets[i].r, -1.f, 1.f);
            comets[i].g = clamp(comets[i].g, -1.f, 1.f);
            comets[i].b = clamp(comets[i].b, -1.f, 1.f);
            glUniform3f(color_id, comets[i].r, comets[i].g, comets[i].b);

            glUniform1f(opacity_id, 0.5f-(fi*0.04f));
            
            glDrawElements(GL_TRIANGLES, ncube_numind, GL_UNSIGNED_INT, 0);
        }
    }
    glDisable(GL_BLEND);

    //
    
    glfwSwapBuffers(window);
}

//*************************************
// Input Handelling
//*************************************
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if(action == GLFW_PRESS)
    {
        if(key == GLFW_KEY_A){ keystate[0] = 1; }
        else if(key == GLFW_KEY_D){ keystate[1] = 1; }
        else if(key == GLFW_KEY_W){ keystate[2] = 1; }
        else if(key == GLFW_KEY_S){ keystate[3] = 1; }
        else if(key == GLFW_KEY_SPACE){ keystate[4] = 1; }
        else if(key == GLFW_KEY_LEFT_SHIFT){ keystate[5] = 1; }
        else if(key == GLFW_KEY_F)
        {
            if(t-lfct > 2.0)
            {
                char strts[16];
                timestamp(&strts[0]);
                printf("[%s] FPS: %g\n", strts, fc/(t-lfct));
                lfct = t;
                fc = 0;
            }
        }
        else if(key == GLFW_KEY_V)
        {
            // view inversion
            printf(":: %+.2f\n", view.m[1][1]);

            // dump view matrix
            printf("%+.2f %+.2f %+.2f %+.2f\n", view.m[0][0], view.m[0][1], view.m[0][2], view.m[0][3]);
            printf("%+.2f %+.2f %+.2f %+.2f\n", view.m[1][0], view.m[1][1], view.m[1][2], view.m[1][3]);
            printf("%+.2f %+.2f %+.2f %+.2f\n", view.m[2][0], view.m[2][1], view.m[2][2], view.m[2][3]);
            printf("%+.2f %+.2f %+.2f %+.2f\n", view.m[3][0], view.m[3][1], view.m[3][2], view.m[3][3]);
            printf("---\n");
        }
    }
    else if(action == GLFW_RELEASE)
    {
        if(key == GLFW_KEY_A){ keystate[0] = 0; }
        else if(key == GLFW_KEY_D){ keystate[1] = 0; }
        else if(key == GLFW_KEY_W){ keystate[2] = 0; }
        else if(key == GLFW_KEY_S){ keystate[3] = 0; }
        else if(key == GLFW_KEY_SPACE){ keystate[4] = 0; }
        else if(key == GLFW_KEY_LEFT_SHIFT){ keystate[5] = 0; }
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if(action == GLFW_PRESS)
    {
        if(button == GLFW_MOUSE_BUTTON_LEFT)
        {
            focus_cursor = 1 - focus_cursor;
            if(focus_cursor == 0)
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            else
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
            glfwSetCursorPos(window, ww2, wh2);
            glfwGetCursorPos(window, &ww2, &wh2);
        }
        else if(button == GLFW_MOUSE_BUTTON_RIGHT)
            brake = 1;
    }
    else if(action == GLFW_RELEASE)
        brake = 0;
}

void window_size_callback(GLFWwindow* window, int width, int height)
{
    winw = width;
    winh = height;

    glViewport(0, 0, winw, winh);
    aspect = (f32)winw / (f32)winh;
    ww = winw;
    wh = winh;
    rww = 1.0/ww;
    rwh = 1.0/wh;
    ww2 = ww/2.0;
    wh2 = wh/2.0;
    uw = (double)aspect/ww;
    uh = 1.0/wh;
    uw2 = (double)aspect/ww2;
    uh2 = 1.0/wh2;

    mIdent(&projection);
    mPerspective(&projection, 60.0f, aspect, 0.01f, FAR_DISTANCE);
    glUniformMatrix4fv(projection_id, 1, GL_FALSE, (GLfloat*) &projection.m[0][0]);
}

//*************************************
// Process Entry Point
//*************************************
int main(int argc, char** argv)
{
    // allow custom msaa level
    int msaa = 16;
    if(argc >= 2){msaa = atoi(argv[1]);}

    // help
    printf("----\n");
    printf("Fractal Attack\n");
    printf("----\n");
    printf("James William Fletcher (github.com/mrbid)\n");
    printf("----\n");
    printf("Argv(1): msaa\n");
    printf("F = FPS to console.\n");
    printf("W, A, S, D, SPACE, LEFT SHIFT\n");
    printf("Right Click to Break\n");
    printf("----\n");

    // init glfw
    if(!glfwInit()){printf("glfwInit() failed.\n"); exit(EXIT_FAILURE);}
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_SAMPLES, msaa);
    window = glfwCreateWindow(winw, winh, "Fractal Attack", NULL, NULL);
    if(!window)
    {
        printf("glfwCreateWindow() failed.\n");
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    const GLFWvidmode* desktop = glfwGetVideoMode(glfwGetPrimaryMonitor());
    glfwSetWindowPos(window, (desktop->width/2)-(winw/2), (desktop->height/2)-(winh/2)); // center window on desktop
    glfwSetWindowSizeCallback(window, window_size_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwMakeContextCurrent(window);
    gladLoadGL(glfwGetProcAddress);
    glfwSwapInterval(1); // 0 for immediate updates, 1 for updates synchronized with the vertical retrace, -1 for adaptive vsync

    // set icon
    glfwSetWindowIcon(window, 1, &(GLFWimage){16, 16, (unsigned char*)&icon_image.pixel_data});

    // set comets
    randComets();

//*************************************
// projection
//*************************************

    window_size_callback(window, winw, winh);

//*************************************
// bind vertex and index buffers
//*************************************

    // ***** BIND MENGER *****
    scaleBuffer(ncube_vertices, ncube_numvert*3);
    esBind(GL_ARRAY_BUFFER, &mdlMenger.vid, ncube_vertices, sizeof(ncube_vertices), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlMenger.iid, ncube_indices, sizeof(ncube_indices), GL_STATIC_DRAW);

    // ***** BIND INNER *****
    scaleBuffer(exo_vertices, exo_numvert*3);
    esBind(GL_ARRAY_BUFFER, &mdlInner.vid, exo_vertices, sizeof(exo_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlInner.cid, inner_colors, sizeof(inner_colors), GL_STATIC_DRAW);

    // ***** BIND EXO *****
    GLsizeiptr s = exo_numvert*3;
    for(GLsizeiptr i = 0; i < s; i+=3)
    {
        const f32 g = (exo_colors[i] + exo_colors[i+1] + exo_colors[i+2]) / 3;
        const f32 h = (1.f-g)*0.01f;
        vec v = {exo_vertices[i], exo_vertices[i+1], exo_vertices[i+2]};
        vNorm(&v);
        vMulS(&v, v, h);
        exo_vertices[i]   -= v.x;
        exo_vertices[i+1] -= v.y;
        exo_vertices[i+2] -= v.z;
        exo_vertices[i] *= 1.03f;
        exo_vertices[i+1] *= 1.03f;
        exo_vertices[i+2] *= 1.03f;
    }
    esBind(GL_ARRAY_BUFFER, &mdlExo.vid, exo_vertices, sizeof(exo_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlExo.cid, exo_colors, sizeof(exo_colors), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlExo.iid, exo_indices, sizeof(exo_indices), GL_STATIC_DRAW);

//*************************************
// compile & link shader programs
//*************************************

    makePhong();
    makeLambert();
    makeLambert2();

//*************************************
// configure render options
//*************************************

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.f, 0.f, 0.f, 0.f);

//*************************************
// execute update / render loop
//*************************************

    // init
    t = glfwGetTime();
    lfct = t;
    
    // event loop
    while(!glfwWindowShouldClose(window))
    {
        t = glfwGetTime();
        glfwPollEvents();
        main_loop();
        fc++;
    }

    // done
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
    return 0;
}
