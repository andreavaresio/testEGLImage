#include "EGL/egl.h"
#define EGL_EGLEXT_PROTOTYPES
#include "EGL/eglext.h"
#include "GLES2/gl2.h"
#define GL_GLEXT_PROTOTYPES
#include "GLES2/gl2ext.h"
#include "assert.h"
#include "stdio.h"
#include "stdlib.h"
#include <unistd.h>
#include "AVWindow.h"
#include "stdio.h"
#include "string.h"

#define WIDTH 1280
#define HEIGHT 720
#define NCYCLES 1

EGLint g_configAttributes[] =
{
	/* DO NOT MODIFY. */
	/* These attributes are in a known order and may be re-written at initialization according to application requests. */
	EGL_SAMPLES, 4,

	EGL_ALPHA_SIZE, 0,
	EGL_RED_SIZE, 8,
	EGL_GREEN_SIZE, 8,
	EGL_BLUE_SIZE, 8,
	EGL_BUFFER_SIZE, 32,
	EGL_STENCIL_SIZE, 0,
	EGL_RENDERABLE_TYPE, 0,    /* This field will be completed according to application request. */

	EGL_SURFACE_TYPE, EGL_WINDOW_BIT,

	EGL_DEPTH_SIZE, 16,

	/* MODIFY BELOW HERE. */
	/* If you need to add or override EGL attributes from above, do it below here. */

	EGL_NONE
};

EGLint g_contextAttributes[] =
{
	EGL_CONTEXT_CLIENT_VERSION, 0, /* This field will be completed according to application request. */
	EGL_NONE, EGL_NONE,            /* If we use OpenGL ES 3.1, we will have to use EGL_CONTEXT_MINOR_VERSION,
								   and we overwrite these fields. */
								   EGL_NONE
};

EGLConfig g_config;

EGLint g_windowAttributes[] =
{
	EGL_NONE
	/*
	* Uncomment and remove EGL_NONE above to specify EGL_RENDER_BUFFER value to eglCreateWindowSurface.
	* EGL_RENDER_BUFFER,     EGL_BACK_BUFFER,
	* EGL_NONE
	*/
};

EGLConfig findConfig(EGLDisplay display)
{
	bool strictMatch;
#if defined(__arm__)
	strictMatch = true;
#else
	strictMatch = false;
#endif

	EGLConfig *configsArray = NULL;
	EGLint numberOfConfigs = 0;
	EGLBoolean success = EGL_FALSE;

	/* Enumerate available EGL configurations which match or exceed our required attribute list. */
	success = eglChooseConfig(display, g_configAttributes, NULL, 0, &numberOfConfigs);
	if (success != EGL_TRUE)
	{
		printf("Failed to enumerate EGL configs");
		exit(1);
	}

	printf("Number of configs found is %d\n", numberOfConfigs);

	if (numberOfConfigs == 0)
	{
		printf("Disabling AntiAliasing to try and find a config.\n");
		g_configAttributes[1] = EGL_DONT_CARE;
		success = eglChooseConfig(display, g_configAttributes, NULL, 0, &numberOfConfigs);
		if (success != EGL_TRUE)
		{
			printf("Failed to enumerate EGL configs 2");
			exit(1);
		}

		if (numberOfConfigs == 0)
		{
			printf("No configs found with the requested attributes");
			exit(1);
		}
		else
		{
			printf("Configs found when antialiasing disabled.\n ");
		}
	}

	/* Allocate space for all EGL configs available and get them. */
	configsArray = (EGLConfig *)calloc(numberOfConfigs, sizeof(EGLConfig));

	success = eglChooseConfig(display, g_configAttributes, configsArray, numberOfConfigs, &numberOfConfigs);
	if (success != EGL_TRUE)
	{
		printf("Failed to enumerate EGL configs 3");
		exit(1);
	}
	bool matchFound = false;
	int matchingConfig = -1;

	if (strictMatch)
	{
		/*
		* Loop through the EGL configs to find a color depth match.
		* Note: This is necessary, since EGL considers a higher color depth than requested to be 'better'
		* even though this may force the driver to use a slow color conversion blitting routine.
		*/
		EGLint redSize = g_configAttributes[5];
		EGLint greenSize = g_configAttributes[7];
		EGLint blueSize = g_configAttributes[9];

		for (int configsIndex = 0; (configsIndex < numberOfConfigs) && !matchFound; configsIndex++)
		{
			EGLint attributeValue = 0;

			success = eglGetConfigAttrib(display, configsArray[configsIndex], EGL_RED_SIZE, &attributeValue);
			if (success != EGL_TRUE)
			{
				printf("Failed to get EGL attribute");
				exit(1);
			}

			if (attributeValue == redSize)
			{
				success = eglGetConfigAttrib(display, configsArray[configsIndex], EGL_GREEN_SIZE, &attributeValue);
				if (success != EGL_TRUE)
				{
					printf("Failed to get EGL attribute 2");
					exit(1);
				}

				if (attributeValue == greenSize)
				{
					success = eglGetConfigAttrib(display, configsArray[configsIndex], EGL_BLUE_SIZE, &attributeValue);
					if (success != EGL_TRUE)
					{
						printf("Failed to get EGL attribute 3");
						exit(1);
					}

					if (attributeValue == blueSize)
					{
						matchFound = true;
						matchingConfig = configsIndex;
					}
				}
			}
		}
	}
	else
	{
		/* If not strict we can use any matching config. */
		matchingConfig = 0;
		matchFound = true;
	}

	if (!matchFound)
	{
		printf("Failed to find matching EGL config");
		exit(1);
	}

	/* Return the matching config. */
	EGLConfig configToReturn = configsArray[matchingConfig];

	free(configsArray);
	configsArray = NULL;

	return configToReturn;
}

#define _GLCheck()\
{\
	GLenum glError = glGetError(); \
	if(glError != GL_NO_ERROR) \
			{ \
		printf ("glGetError() = %i (0x%.8x) at line %i\n", glError, glError, __LINE__); \
			} \
}\

#define VERTEXPOS 1.0f
#define TEXTUREPOS 1.0f
GLfloat vVertices[] = {
	-VERTEXPOS, VERTEXPOS, 0.0f,	// Position 0
	0.0f, 0.0f,						// TexCoord 0 
	-VERTEXPOS, -VERTEXPOS, 0.0f,	// Position 1
	0.0f, TEXTUREPOS,				// TexCoord 1
	VERTEXPOS, -VERTEXPOS, 0.0f,	// Position 2
	TEXTUREPOS, TEXTUREPOS,			// TexCoord 2
	VERTEXPOS, VERTEXPOS, 0.0f,		// Position 3
	TEXTUREPOS, 0.0f,				// TexCoord 3
};

const GLushort indices[] = { 0, 1, 2, 0, 2, 3 };

const char _VShaderSource_Blender[] =
"attribute vec4 a_position;   \n"
"attribute vec2 a_texCoord;   \n"
"varying vec2 v_texCoord;     \n"
"void main()                  \n"
"{                            \n"
"   gl_Position = a_position; \n"
"   v_texCoord = a_texCoord;  \n"
"}                            \n";

const char _FShaderSource_Blender[] =
"precision mediump float;													\n"
"uniform sampler2D InputTex;												\n"
"varying vec2 v_texCoord;													\n"
"																			\n"
"void main(void)															\n"
"{																			\n"
"  //gl_FragColor=texture2D(InputTex,v_texCoord);							\n"
"  vec4 col=texture2D(InputTex,v_texCoord);									\n"
"  gl_FragColor=vec4(col.b, col.g, col.r, col.a);                           \n"
"}																			\n";

void UploadTexture_UsingTextSubImage2D(unsigned char *InPicture, GLuint _iTex)
{
	glActiveTexture(GL_TEXTURE0);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glBindTexture(GL_TEXTURE_2D, _iTex);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, WIDTH, HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, InPicture);
	_GLCheck();
}

void UpdatePixmap(CAVWindow wind, Pixmap XPixMap, XImage *pXImage, unsigned char *TexData)
{
#if 0
	memcpy(pXImage->data, TexData, WIDTH * HEIGHT * 4);
#else
	GC gc = DefaultGC(wind.GetXDisplayPtr(), 0);

	static unsigned long foreground;
	XSetForeground(wind.GetXDisplayPtr(), gc, foreground++);
	XFillRectangle(wind.GetXDisplayPtr(), XPixMap, gc, 0, 0, WIDTH, HEIGHT);

	const char message[] = "pixmap to texture";
	XSetForeground(wind.GetXDisplayPtr(), gc, 0x0000ff);
	XDrawString(wind.GetXDisplayPtr(), XPixMap, gc, 10, 15, message, strlen(message));

	XFlushGC(wind.GetXDisplayPtr(), gc);
	XSync(wind.GetXDisplayPtr(), True);
#endif
}

int main(int argc, char **argv)
{
	CAVWindow wind;

	EGLDisplay	m_sEGLDisplay;
	EGLSurface	m_sEGLSurface;
	EGLContext	m_sEGLContext;
	EGLint major, minor;

	GLuint m_iVShader, m_iFShader;
	GLuint m_iProgram;
	GLint m_iLocPosition;
	GLint m_iLocTexCoord;
	GLint iStatus;

	wind.CreaFinestra(WIDTH, HEIGHT, L"testEGLImage", 0, 0);
	m_sEGLDisplay = eglGetDisplay(wind.GetXDisplayPtr()); assert(m_sEGLDisplay != EGL_NO_DISPLAY);

	EGLBoolean success = eglInitialize(m_sEGLDisplay, &major, &minor); assert(success == EGL_TRUE);
	printf ("major=%d, minor = %d\n", major, minor );

	g_configAttributes[15] = EGL_OPENGL_ES2_BIT;
	g_contextAttributes[1] = 2;
	g_contextAttributes[2] = EGL_NONE;

	g_config = findConfig(m_sEGLDisplay);
	m_sEGLSurface = eglCreateWindowSurface(m_sEGLDisplay, g_config, (EGLNativeWindowType)wind.GethWnd(), g_windowAttributes); assert(m_sEGLSurface != EGL_NO_SURFACE);
	eglBindAPI(EGL_OPENGL_ES_API);
	m_sEGLContext = eglCreateContext(m_sEGLDisplay, g_config, EGL_NO_CONTEXT, g_contextAttributes); assert(m_sEGLContext != EGL_NO_CONTEXT);
	eglMakeCurrent(m_sEGLDisplay, m_sEGLSurface, m_sEGLSurface, m_sEGLContext);

	printf ("Vendor =%s\n", eglQueryString(m_sEGLDisplay, EGL_VENDOR));
	printf ("API    =%s\n", eglQueryString(m_sEGLDisplay, EGL_CLIENT_APIS));
	printf ("Version=%s\n", eglQueryString(m_sEGLDisplay, EGL_VERSION));
	printf ("Extens =%s\n", eglQueryString(m_sEGLDisplay, EGL_EXTENSIONS));
	const GLubyte *strExt = glGetString(GL_EXTENSIONS);
	printf("GL_EXTENSIONS: %s\n", strExt);

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_DITHER);
	glDisable(GL_POLYGON_OFFSET_FILL);
	glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
	glDisable(GL_SAMPLE_COVERAGE);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_STENCIL_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	m_iVShader = glCreateShader(GL_VERTEX_SHADER);
	m_iFShader = glCreateShader(GL_FRAGMENT_SHADER);
	const char *pVShader = _VShaderSource_Blender;
	const char *pFShader = _FShaderSource_Blender;
	glShaderSource(m_iVShader, 1, &pVShader, NULL);
	glShaderSource(m_iFShader, 1, &pFShader, NULL);
	glCompileShader(m_iVShader);
	glGetShaderiv(m_iVShader, GL_COMPILE_STATUS, &iStatus); assert(iStatus != GL_FALSE);
	glCompileShader(m_iFShader);
	glGetShaderiv(m_iFShader, GL_COMPILE_STATUS, &iStatus);assert(iStatus != GL_FALSE);
	m_iProgram = glCreateProgram();
	glAttachShader(m_iProgram, m_iVShader);
	glAttachShader(m_iProgram, m_iFShader);
	glLinkProgram(m_iProgram);
	m_iLocPosition = glGetAttribLocation(m_iProgram, "a_position");
	m_iLocTexCoord = glGetAttribLocation(m_iProgram, "a_texCoord");
	_GLCheck();

	GLuint iTex;
	glGenTextures(1, &iTex);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glBindTexture(GL_TEXTURE_2D, iTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	unsigned char *TexData = new unsigned char[WIDTH * HEIGHT * 4];

	const EGLint img_attribs[] = {
		EGL_IMAGE_PRESERVED_KHR, EGL_TRUE,
		EGL_NONE
	};

	Pixmap XPixMap = XCreatePixmap(wind.GetXDisplayPtr(), wind.GethWnd(), WIDTH, HEIGHT, 32);
	XImage *pXImage = XGetImage(wind.GetXDisplayPtr(), XPixMap, 0, 0, WIDTH, HEIGHT, 0xffffffff, XYPixmap);
	printf("XGetImage %d %d %d %p\n", pXImage->width, pXImage->height, pXImage->depth, pXImage->data);

	EGLImageKHR eglImage = eglCreateImageKHR(m_sEGLDisplay, EGL_NO_CONTEXT, EGL_NATIVE_PIXMAP_KHR, (EGLClientBuffer)XPixMap, img_attribs);
	EGLint eglError = eglGetError();
	if (eglError != EGL_SUCCESS)
		printf("eglGetError() = %i (0x%.8x) at line %i\n", eglError, eglError, __LINE__);

	glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, eglImage);
	_GLCheck();

	printf("Starting loop\n");
	for (int iCycles = 0; iCycles < NCYCLES; iCycles++)
	{
		memset(TexData, 0, WIDTH * HEIGHT * 4);
		for (int y = 0; y < HEIGHT; y++)
		{
			for (int x = iCycles; x < iCycles + 16; x++)
			{
				TexData[y * WIDTH * 4 + 4 * x + 0] = 255;
				TexData[y * WIDTH * 4 + 4 * x + 1] = 255;
				TexData[y * WIDTH * 4 + 4 * x + 2] = 255;
				TexData[y * WIDTH * 4 + 4 * x + 3] = 255;
			}
		}

		UploadTexture_UsingTextSubImage2D(TexData, iTex);
		//UpdatePixmap(wind, XPixMap, pXImage, TexData);

		//draw
		glUseProgram(m_iProgram);
		GLint BlenderSamplerLocInput = glGetUniformLocation(m_iProgram, "InputTex");
		glVertexAttribPointer(m_iLocTexCoord, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), &vVertices[3]);
		glEnableVertexAttribArray(m_iLocTexCoord);
		glActiveTexture(GL_TEXTURE0);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glUniform1i(BlenderSamplerLocInput, 0);
		glVertexAttribPointer(m_iLocPosition, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), vVertices);
		glEnableVertexAttribArray(m_iLocPosition);
		glBindTexture(GL_TEXTURE_2D, iTex);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);
		GLenum glError = glGetError();
		_GLCheck();
		if (!eglSwapBuffers(m_sEGLDisplay, m_sEGLSurface))
			printf("Failed to swap buffers. %x\n", eglGetError());

	}
	printf("typer enter\n");
	getchar();

	delete[] TexData;
	eglDestroyImageKHR(m_sEGLDisplay, eglImage);
	XFreePixmap(wind.GetXDisplayPtr(), XPixMap);
	wind.ChiudiFinestra();

	//glClearColor(1.0, 1.0, 0.0, 1.0);
	//glClear(GL_COLOR_BUFFER_BIT);
	//glFlush();
	//eglSwapBuffers(m_sEGLDisplay, m_sEGLSurface);

	return 0;
}

int main_minimal(int argc, char **argv)
{

	const EGLint attr[] =
	{
		EGL_IMAGE_PRESERVED_KHR, EGL_FALSE,
		EGL_NONE
	};

	Display* Xdisplay = XOpenDisplay(NULL);
	unsigned image_w = 1280;
	unsigned image_h = 720;
	Pixmap pixmap = XCreatePixmap(Xdisplay, DefaultRootWindow(Xdisplay), image_w, image_h, 32);

	EGLDisplay EGLdisplay = eglGetDisplay(Xdisplay); if (EGL_NO_DISPLAY == EGLdisplay) { printf("ERROR in eglGetCurrentDisplay %x\n", eglGetError()); exit(1); }
	if (EGL_FALSE == eglInitialize(EGLdisplay, NULL, NULL)){ printf("ERROR in eglInitialize %x\n", eglGetError()); exit(1); }

	EGLImageKHR eglImage = eglCreateImageKHR(EGLdisplay, EGL_NO_CONTEXT, EGL_NATIVE_PIXMAP_KHR, (EGLClientBuffer)pixmap, attr); if (EGL_NO_IMAGE_KHR == eglImage){printf("ERROR in eglCreateImageKHR %x\n", eglGetError());exit(1);}

	printf("Ended\n");
}
