#include "shared.h"
#include "client.h"
#include "render.h"

RE_RegisterShaderNoMip_t RE_RegisterShaderNoMip = (RE_RegisterShaderNoMip_t)0x4FCAE0;
RE_RegisterShader_t RE_RegisterShader = (RE_RegisterShader_t)0x4FCA80;
R_FindShader_t R_FindShader = (R_FindShader_t)0x4FC5C0;
SCR_DrawPic_t SCR_DrawPic = (SCR_DrawPic_t)0x4168D0;
SCR_AdjustFrom640_t SCR_AdjustFrom640 = (SCR_AdjustFrom640_t)0x416810;
/*
SCR_DrawSmallStringExt_t SCR_DrawSmallStringExt = (SCR_DrawSmallStringExt_t)0x416A30;
SCR_DrawSmallChar_t SCR_DrawSmallChar = (SCR_DrawSmallChar_t)0x416980;
*/
SCR_DrawString_t SCR_DrawString = (SCR_DrawString_t)0x4DF570;
RE_SetColor_t RE_SetColor = (RE_SetColor_t)0x4DDCF0;
RB_SetGL2D_t RB_SetGL2D = (RB_SetGL2D_t)0x4D6CB0;

/* FONTS */

//HDC hDC = (HDC)0x19BFFE4;
HWND hWnd = (HWND)0x8E5268;
HGLRC hGLRC = (HGLRC)0x19BFFE8;


unsigned int fontMainMenuHeader = 0;
unsigned int fontIngameChatMessage = 0;
unsigned int font14px = 0;

//GLYPHMETRICSFLOAT gmf[256];

void DestroyFonts() {
	if (fontMainMenuHeader)
		glDeleteLists(fontMainMenuHeader, 96);
	if (fontIngameChatMessage)
		glDeleteLists(fontIngameChatMessage, 96);
	if (font14px)
		glDeleteLists(font14px, 96);
}

bool GenerateFont(int fontsize, const char *fontname, unsigned int *base) {
	bool ret = true;
	if (base != nullptr&&*base)
		return false;

	fontsize *= -1;

	//HDC hDC = GetDC(NULL);
	HDC hDC = *(HDC*)0x19BFFE4;

	HFONT	font;
	HFONT	oldfont;

	*base = glGenLists(96);

	font = CreateFont(fontsize,
		0, //font width
		0, //escapement angle
		0, //orientation angle
		FW_NORMAL, //font weight
		FALSE, //italic
		FALSE, //underline
		FALSE, //strikeout
		ANSI_CHARSET, //character set identifier
		OUT_TT_PRECIS, //output precision
		CLIP_DEFAULT_PRECIS, //clipping precision
		ANTIALIASED_QUALITY, //output quality
		FF_DONTCARE | DEFAULT_PITCH, //family and pitch
		fontname);

	oldfont = (HFONT)SelectObject(hDC, font);
	//if(!wglUseFontOutlinesA(hDC,32,96,base,0.0f,0.2f,WGL_FONT_POLYGONS,gmf)) {
	if (!wglUseFontBitmapsA(hDC, 32, 96, *base)) {
		//MsgBox(va("failed usefontbitmaps %s", GetLastErrorAsString().c_str()));
		ret = false;
	}
	SelectObject(hDC, oldfont);
	DeleteObject(font);
	return ret;
}

cvar_t *cg_chat_font;

void GenerateFonts() {
	cg_chat_font = Cvar_Get("cg_chat_font", "Tahoma", CVAR_ARCHIVE);
	GenerateFont(28, "Arial", &fontMainMenuHeader);
	GenerateFont(14, cg_chat_font->string, &fontIngameChatMessage);
	GenerateFont(14, "Verdana", &font14px);
}

void PrintFont(unsigned int fontID, const char *fmt, ...) {
	char txt[256] = { 0 };
	va_list va;
	if (!fmt || !*fmt)
		return;
	if (!fontID)
		return;

	va_start(va, fmt);
	vsprintf(txt, fmt, va);
	va_end(va);

	glPushAttrib(GL_LIST_BIT);
	glListBase(fontID - 32);
	glCallLists(strlen(txt), GL_UNSIGNED_BYTE, txt);
	glPopAttrib();
}

GLvoid glPrint(const char *fmt, ...)					// Custom GL "Print" Routine
{
	char		text[256];								// Holds Our String
	va_list		ap;										// Pointer To List Of Arguments

	if (fmt == NULL)									// If There's No Text
		return;											// Do Nothing

	va_start(ap, fmt);									// Parses The String For Variables
	vsprintf(text, fmt, ap);						// And Converts Symbols To Actual Numbers
	va_end(ap);		

	glPushAttrib(GL_LIST_BIT);							// Pushes The Display List Bits
	glListBase(fontMainMenuHeader - 32);								// Sets The Base Character to 32
	glCallLists(strlen(text), GL_UNSIGNED_BYTE, text);	// Draws The Display List Text
	glPopAttrib();										// Pops The Display List Bits
}

/* custom shader text adding - Richard */

char **s_shaderText = (char**)0x11EDD60;

#define GLOW_SHADER \
	"glowshader2d                                        \n" \
"{                                                   \n" \
"	nopicmip                                         \n" \
"	nomipmaps                                        \n" \
"	{                                                \n" \
"		rgbgen const ( 0 1 .5 )                      \n" \
"		blendFunc add                                \n" \
"	}                                                \n" \
"	{                                                \n" \
"		map $whiteImage                              \n" \
"		rgbgen constLighting ( 0.8 0.632 0.252 )     \n" \
"		alphaGen wave sin -0.1 1.1 0 .5              \n" \
"		blendFunc GL_SRC_ALPHA GL_ONE                \n" \
"	}                                                \n" \
"}                                                   \n"

void ScanAndLoadShaderFiles(void) {
	void(*o)(void) = (void(*)(void))0x4FD560;
	o();
}

#define EMPTY_SHADER_TEMPLATE "%s\n{\n\t{\n\t\tmap $whiteimage\n\t}\n}\n"

void AddCustomShader() {
	strcat(*s_shaderText, "\n" GLOW_SHADER);
}

void __declspec(naked) AddCustomShader_w() {
	__asm
	{
		call AddCustomShader
		mov eax, dword ptr [esp + 0x1c]
		push eax
		mov eax, 0x4FD73A
		jmp eax
	}
}

void* ri_Hunk_AllocAlign(int size) {
	void*(*Hunk_AllocAlign)(int) = (void*(*)(int))0x432160;

	int s = size + strlen(GLOW_SHADER) + 2;
	return Hunk_AllocAlign(s);
}

/* 2d render stuff */


void InitRender() {
	GenerateFonts();
}

void DestroyRender() {
	DestroyFonts();
}

void RGL_DrawQuad(float x, float y, float w, float h, vec4_t rgba) {
	SCR_AdjustFrom640(&x, &y, &w, &h);

	glFinish();

	if (!*(int*)0x16D8E70)
		RB_SetGL2D();
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glColor4fv(rgba);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0);
	glVertex2f(x, y);

	glTexCoord2f(1, 0);
	glVertex2f(x + w, y);


	glTexCoord2f(1, 1);
	glVertex2f(x + w, y + h);


	glTexCoord2f(0, 1);
	glVertex2f(x, y + h);
	glEnd();
	glFlush();
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
}

void RGL_DrawPic(float x, float y, float w, float h, unsigned int tex) {
	SCR_AdjustFrom640(&x, &y, &w, &h);

	glFinish();

	if (!*(int*)0x16D8E70)
		RB_SetGL2D();
	glColor3f(1, 1, 1);
	glBindTexture(GL_TEXTURE_2D, tex);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0);
	glVertex2f(x, y);

	glTexCoord2f(1, 0);
	glVertex2f(x + w, y);


	glTexCoord2f(1, 1);
	glVertex2f(x + w, y + h);


	glTexCoord2f(0, 1);
	glVertex2f(x, y + h);
	glEnd();
	glFlush();
}



/* Render stuff texture / geometry etc */
shader_t *lastShader;
bool hook_bind = false;

void GL_Bind()
{
	/*image_t* image;
	*/
	int image;
	__asm mov image, esi

	void(*call)();
	*(int*)&call = 0x4D5450;
	__asm mov esi, image
	call();
}

void APIENTRY qglBindTexture(GLenum target, GLuint texture) {
	glBindTexture(target, texture);
}


static GLfloat rot = 0.0;
static time_t rot_time = 0;

void drawCube(vec3_t org, float size) {
	glPushMatrix();
	glTranslatef(org[0], org[1], org[2]);

	glBegin(GL_QUADS);
	// front face
	glNormal3f(0, 0, 1);
	glColor3f(1.0, 0.0, 0.0);
	glTexCoord2d(0, 0);
	glVertex3f(size / 2, size / 2, size / 2);
	glVertex3f(-size / 2, size / 2, size / 2);
	glVertex3f(-size / 2, -size / 2, size / 2);
	glVertex3f(size / 2, -size / 2, size / 2);
	// left face
	glNormal3f(-1, 0, 0);
	glColor3f(0.0, 1.0, 0.0);
	glTexCoord2d(0, 1);
	glVertex3f(-size / 2, size / 2, size / 2);
	glVertex3f(-size / 2, -size / 2, size / 2);
	glVertex3f(-size / 2, -size / 2, -size / 2);
	glVertex3f(-size / 2, size / 2, -size / 2);
	// back face
	glNormal3f(0, 0, -1);
	glColor3f(0.0, 0.0, 1.0);
	glTexCoord2d(1, 0);
	glVertex3f(size / 2, size / 2, -size / 2);
	glVertex3f(-size / 2, size / 2, -size / 2);
	glVertex3f(-size / 2, -size / 2, -size / 2);
	glVertex3f(size / 2, -size / 2, -size / 2);
	// right face
	glNormal3f(1, 0, 0);
	glColor3f(1.0, 1.0, 0.0);
	glTexCoord2d(1, 1);
	glVertex3f(size / 2, size / 2, size / 2);
	glVertex3f(size / 2, -size / 2, size / 2);
	glVertex3f(size / 2, -size / 2, -size / 2);
	glVertex3f(size / 2, size / 2, -size / 2);
	// top face
	glNormal3f(0, 1, 0);
	glColor3f(1.0, 0.0, 1.0);
	glTexCoord2d(0, 0);
	glVertex3f(size / 2, size / 2, size / 2);
	glVertex3f(-size / 2, size / 2, size / 2);
	glVertex3f(-size / 2, size / 2, -size / 2);
	glVertex3f(size / 2, size / 2, -size / 2);
	// bottom face
	glNormal3f(0, -1, 0);
	glColor3f(0.0, 1.0, 1.0);
	glTexCoord2d(1, 0);
	glVertex3f(size / 2, -size / 2, size / 2);
	glVertex3f(-size / 2, -size / 2, size / 2);
	glVertex3f(-size / 2, -size / 2, -size / 2);
	glVertex3f(size / 2, -size / 2, -size / 2);
	glEnd();

	glPopMatrix();
}

void ExtraRender() {
	cvar_t *t = Cvar_Get("extrarender", "0", 0); //without if u use r_showimages it crashes
	if (!t->integer)
		return;
	//vec3_t org = {-8332,-8984,60};
	vec3_t org = { -9527, -7515, 100 };
	//vec3_t org = {-9528,-7626,200};

	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_NORMALIZE);

	drawCube(org, 60);

	glColor3f(1, 1, 1);

	glDisable(GL_COLOR_MATERIAL);
	glDisable(GL_NORMALIZE);
}


UINT32 pExecuteRenderCommands = 0x4D8AB0;

void __declspec(naked) RB_ExecuteRenderCommands() {
	__asm
	{
		call pExecuteRenderCommands
		//call ExtraRender
		ret
	}
}

void DrawTris(void *input) {}