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
#define NCYCLES 100

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

EGLDisplay	g_sEGLDisplay;
EGLSurface	g_sEGLSurface;
EGLContext	g_sEGLContext;
GLuint g_iVShader, g_iFShader;
GLuint g_iProgram;
GLint g_iLocPosition;
GLint g_iLocTexCoord;
GLuint g_iTex;

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

void UpdatePixmap_memcpy(XImage *pXImage, unsigned char *TexData)
{
	memcpy(pXImage->data, TexData, WIDTH * HEIGHT * 4);
}

void UpdatePixmap_UsingX(CAVWindow wind, Pixmap XPixMap)
{
	GC gc = DefaultGC(wind.GetXDisplayPtr(), 0);

	static unsigned long foreground;
	XSetForeground(wind.GetXDisplayPtr(), gc, foreground++);
	XFillRectangle(wind.GetXDisplayPtr(), XPixMap, gc, 0, 0, WIDTH, HEIGHT);

	const char message[] = "pixmap to texture";
	XSetForeground(wind.GetXDisplayPtr(), gc, 0x0000ff);
	XDrawString(wind.GetXDisplayPtr(), XPixMap, gc, 10, 15, message, strlen(message));

	XFlushGC(wind.GetXDisplayPtr(), gc);
	XSync(wind.GetXDisplayPtr(), True);
}

void SetupEGL(CAVWindow &wind)
{
	EGLint major, minor;

	g_sEGLDisplay = eglGetDisplay(wind.GetXDisplayPtr()); assert(g_sEGLDisplay != EGL_NO_DISPLAY);

	EGLBoolean success = eglInitialize(g_sEGLDisplay, &major, &minor); assert(success == EGL_TRUE);
	printf("major=%d, minor = %d\n", major, minor);

	g_configAttributes[15] = EGL_OPENGL_ES2_BIT;
	g_contextAttributes[1] = 2;
	g_contextAttributes[2] = EGL_NONE;

	g_config = findConfig(g_sEGLDisplay);
	g_sEGLSurface = eglCreateWindowSurface(g_sEGLDisplay, g_config, (EGLNativeWindowType)wind.GethWnd(), g_windowAttributes); assert(g_sEGLSurface != EGL_NO_SURFACE);
	eglBindAPI(EGL_OPENGL_ES_API);
	g_sEGLContext = eglCreateContext(g_sEGLDisplay, g_config, EGL_NO_CONTEXT, g_contextAttributes); assert(g_sEGLContext != EGL_NO_CONTEXT);
	eglMakeCurrent(g_sEGLDisplay, g_sEGLSurface, g_sEGLSurface, g_sEGLContext);

	printf("Vendor =%s\n", eglQueryString(g_sEGLDisplay, EGL_VENDOR));
	printf("API    =%s\n", eglQueryString(g_sEGLDisplay, EGL_CLIENT_APIS));
	printf("Version=%s\n", eglQueryString(g_sEGLDisplay, EGL_VERSION));
	printf("Extens =%s\n", eglQueryString(g_sEGLDisplay, EGL_EXTENSIONS));
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

	_GLCheck();
}

void SetupShaders()
{
	GLint iStatus;

	g_iVShader = glCreateShader(GL_VERTEX_SHADER);
	g_iFShader = glCreateShader(GL_FRAGMENT_SHADER);
	const char *pVShader = _VShaderSource_Blender;
	const char *pFShader = _FShaderSource_Blender;
	glShaderSource(g_iVShader, 1, &pVShader, NULL);
	glShaderSource(g_iFShader, 1, &pFShader, NULL);
	glCompileShader(g_iVShader);
	glGetShaderiv(g_iVShader, GL_COMPILE_STATUS, &iStatus); assert(iStatus != GL_FALSE);
	glCompileShader(g_iFShader);
	glGetShaderiv(g_iFShader, GL_COMPILE_STATUS, &iStatus); assert(iStatus != GL_FALSE);
	g_iProgram = glCreateProgram();
	glAttachShader(g_iProgram, g_iVShader);
	glAttachShader(g_iProgram, g_iFShader);
	glLinkProgram(g_iProgram);
	g_iLocPosition = glGetAttribLocation(g_iProgram, "a_position");
	g_iLocTexCoord = glGetAttribLocation(g_iProgram, "a_texCoord");
	_GLCheck();

	glGenTextures(1, &g_iTex);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glBindTexture(GL_TEXTURE_2D, g_iTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	_GLCheck();
}

char *Mode2String[3] = {
	"glTexSubImage2D",
	"EGLIMage and memcpy",
	"EGLIMage and XFillRectangle"
};

int main(int argc, char **argv)
{
try
{
	int mode = -1;
	if (argc != 2)
	{
		printf("Usage:\n%s <mode>\n", argv[0]);
		printf("  mode=0 update texture using glTexSubImage2D\n");
		printf("  mode=1 update texture using EGLIMage and memcpy\n");
		printf("  mode=2 update texture using EGLIMage and XFillRectangle\n");
		exit (1);
	}
	mode = atoi(argv[1]);
	printf("Running mode=%d (%s)\n", mode, Mode2String[mode]);

	CAVWindow wind;

	wind.CreaFinestra(WIDTH, HEIGHT, L"testEGLImage", 0, 0);
	SetupEGL(wind);
	SetupShaders();

	unsigned char *TexData = new unsigned char[WIDTH * HEIGHT * 4];

	const EGLint img_attribs[] = {
		EGL_IMAGE_PRESERVED_KHR, EGL_TRUE,
		EGL_NONE
	};

	Pixmap XPixMap = XCreatePixmap(wind.GetXDisplayPtr(), wind.GethWnd(), WIDTH, HEIGHT, 32);
	XImage *pXImage = XGetImage(wind.GetXDisplayPtr(), XPixMap, 0, 0, WIDTH, HEIGHT, 0xffffffff, XYPixmap);
	printf("XGetImage %d %d %d %p\n", pXImage->width, pXImage->height, pXImage->depth, pXImage->data);

	EGLImageKHR eglImage = eglCreateImageKHR(g_sEGLDisplay, EGL_NO_CONTEXT, EGL_NATIVE_PIXMAP_KHR, (EGLClientBuffer)XPixMap, img_attribs);
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

		if (mode==0)
			UploadTexture_UsingTextSubImage2D(TexData, g_iTex);
		if (mode == 1)
			UpdatePixmap_memcpy(pXImage, TexData);
		if (mode == 2)
			UpdatePixmap_UsingX(wind, XPixMap);

		//draw
		glUseProgram(g_iProgram);
		GLint BlenderSamplerLocInput = glGetUniformLocation(g_iProgram, "InputTex");
		glVertexAttribPointer(g_iLocTexCoord, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), &vVertices[3]);
		glEnableVertexAttribArray(g_iLocTexCoord);
		glActiveTexture(GL_TEXTURE0);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glUniform1i(BlenderSamplerLocInput, 0);
		glVertexAttribPointer(g_iLocPosition, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), vVertices);
		glEnableVertexAttribArray(g_iLocPosition);
		glBindTexture(GL_TEXTURE_2D, g_iTex);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);
		GLenum glError = glGetError();
		_GLCheck();
		if (!eglSwapBuffers(g_sEGLDisplay, g_sEGLSurface))
			printf("Failed to swap buffers. %x\n", eglGetError());

	}
	printf("typer enter\n");
	getchar();

	delete[] TexData;
	eglDestroyImageKHR(g_sEGLDisplay, eglImage);
	XFreePixmap(wind.GetXDisplayPtr(), XPixMap);
	wind.ChiudiFinestra();
}
catch (const char *e)
{
printf ("Exception: %s\n", e);
}

	return 0;
}

