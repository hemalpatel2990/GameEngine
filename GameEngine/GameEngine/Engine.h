
#ifndef ENGINE_H
#define ENGINE_H

// Bang
#include "LibrarySetup.h"

#include "GL/gl3w.h"

#define GLFW_NO_GLU 1
#define GLFW_INCLUDE_GLCOREARB 1
#include "GL/glfw.h"

#include "sb6ext.h"

#include <stdio.h>
#include <string.h>


class Engine
{
public:
	Engine( const char* windowName, const int Width,const int Height);
    virtual ~Engine();

	// required overloading
    virtual void Initialize() = 0;
    virtual void LoadContent() = 0;
	virtual void Update(float currentTime) = 0;
    virtual void Draw() = 0;
    virtual void UnLoadContent() = 0;

	// optional overloading
	//virtual bool UpdateTriggerFunc();
	virtual void ClearBufferFunc();

	 void run();


	// private: --------------------------------------
private:
	// force to use the appropriate constructor
    Engine();

	// private functions
	void privPreInitialize();
	void privPreLoadContent();

public:
    virtual void onResize(int w, int h);
    virtual void onKey(int key, int action);
    virtual void onMouseButton(int button, int action);
    virtual void onMouseMove(int x, int y);
    virtual void onMouseWheel(int pos);
    virtual void onDebugMessage(GLenum source,
                                GLenum type,
                                GLuint id,
                                GLenum severity,
                                GLsizei length,
                                const GLchar* message);
    static void getMousePosition(int& x, int& y);

public:
    struct APPINFO
    {
        char title[128];
        int windowWidth;
        int windowHeight;
        int majorVersion;
        int minorVersion;
        int samples;
		#pragma warning( disable : 4201)
        union
        {
            struct
            {
                unsigned int    fullscreen  : 1;
                unsigned int    vsync       : 1;
                unsigned int    cursor      : 1;
                unsigned int    stereo      : 1;
                unsigned int    debug       : 1;
            };
            unsigned int        all;
        } flags;
    };

protected:
    APPINFO     info;

	static      Engine * app;


    static void GLFWCALL glfw_onResize(int w, int h);
    static void GLFWCALL glfw_onKey(int key, int action);
    static void GLFWCALL glfw_onMouseButton(int button, int action);
    static void GLFWCALL glfw_onMouseMove(int x, int y);
    static void GLFWCALL glfw_onMouseWheel(int pos);
    void setVsync(bool enable);
    static void APIENTRY debug_callback(GLenum source,GLenum type, GLuint id, GLenum severity,GLsizei length,const GLchar* message,GLvoid* userParam);
};




#endif /* ENGINE_H */

