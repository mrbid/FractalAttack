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

//*************************************
// update & render
//*************************************
void main_loop()
{
//*************************************
// time delta for interpolation
//*************************************
    static double lt = 0;
    dt = t-lt;
    lt = t;

//*************************************
// keystates
//*************************************

    static vec vd;
    mGetViewDir(&vd, view);

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
        vAdd(&pp, pp, m);
    }

    if(keystate[1] == 1) // D
    {
        vec vdc;
        vCross(&vdc, vd, (vec){0.f, 1.f, 0.f});
        vec m;
        vMulS(&m, vdc, MOVE_SPEED * dt);
        vSub(&pp, pp, m);
    }

    if(keystate[4] == 1) // SPACE
    {
        pp.y -= MOVE_SPEED * dt;
    }

    if(keystate[5] == 1) // SHIFT
    {
        pp.y += MOVE_SPEED * dt;
    }

    if(brake == 1)
        vMulS(&pp, pp, 0.99f*(1.f-dt));

    vAdd(&ppr, ppr, pp);

    const f32 pmag = vMod(ppr);
    if(pmag < 1.14f)
    {
        const f32 hf = vMod(pp)*10.f;
        vec n = ppr;
        vNorm(&n);
         vReflect(&pp, pp, (vec){-n.x, -n.y, -n.z}); // better if I don't normalise pp
         vMulS(&pp, pp, 0.3f);
        vMulS(&n, n, 1.14f - pmag);
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

        xrot += (ww2-x)*sens;
        yrot += (wh2-y)*sens;

        if(yrot > d2PI)
            yrot = d2PI;
        if(yrot < -d2PI)
            yrot = -d2PI;

        glfwSetCursorPos(window, ww2, wh2);
    }

    mIdent(&view);
    mRotate(&view, yrot, 1.f, 0.f, 0.f);
    mRotate(&view, xrot, 0.f, 1.f, 0.f);
    mTranslate(&view, ppr.x, ppr.y, ppr.z);

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

    glBindBuffer(GL_ARRAY_BUFFER, mdlExo.vid);
    glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(position_id);

    glBindBuffer(GL_ARRAY_BUFFER, mdlExo.cid);
    glVertexAttribPointer(color_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(color_id);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdlExo.iid);

    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (GLfloat*) &view.m[0][0]);
    glDrawElements(GL_TRIANGLES, exo_numind, GL_UNSIGNED_INT, 0);

    ///
    
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
    rww = 1/ww;
    rwh = 1/wh;
    ww2 = ww/2;
    wh2 = wh/2;
    uw = (double)aspect / ww;
    uh = 1 / wh;
    uw2 = (double)aspect / ww2;
    uh2 = 1 / wh2;

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

//*************************************
// projection
//*************************************

    window_size_callback(window, winw, winh);

//*************************************
// bind vertex and index buffers
//*************************************

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

    //makePhong();
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
