#define GL_GLEXT_PROTOTYPES

#include<stdio.h>
#include<stdbool.h>
#include<stdlib.h>
#include<stdint.h>

#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>
#include <GL/glext.h>

#include <libspectre/spectre.h>
#include "postscript.h"

#include "shader.h"
const char* vshader = "#version 450\nvec2 y=vec2(1.,-1);\nvec4 x[4]={y.yyxx,y.xyxx,y.yxxx,y.xxxx};void main(){gl_Position=x[gl_VertexID];}";

#define CANVAS_WIDTH 1920
#define CANVAS_HEIGHT 1080
#define SCANLINE_SIZE 10

#define CHAR_BUFF_SIZE 256

#define DEBUG
#define TIME_RENDER

inline void quit_asm() {
	asm volatile(".intel_syntax noprefix");
	asm volatile("xor edi, edi");
	asm volatile("push 231"); //exit_group
	asm volatile("pop rax");
	asm volatile("syscall");
	asm volatile(".att_syntax prefix");
	__builtin_unreachable();
}

GLuint vao;
GLuint p;
GLuint renderedTex;

bool rendered = false;
bool flipped = false;

#ifdef TIME_RENDER
GTimer* gtimer;
#endif

static gboolean check_escape(GtkWidget *widget, GdkEventKey *event)
{
	(void)widget;
	if (event->keyval == GDK_KEY_Escape) {
		quit_asm();
	}

	return FALSE;
}

// void render_postscript(unsigned char* postscript, unsigned char** data, int* row_length) {
// 	// 
// }

static gboolean
on_render (GtkGLArea *glarea, GdkGLContext *context)
{
	(void)context;
	if (rendered || gtk_widget_get_allocated_width((GtkWidget*)glarea) < CANVAS_WIDTH) return TRUE;
	if (!flipped) { gtk_gl_area_queue_render(glarea); flipped = true; return TRUE; }

	rendered = true;
	glUseProgram(p);
	glBindVertexArray(vao);
	glVertexAttrib1f(0, 0);
	glUniform1i(0, 0);
	glActiveTexture(GL_TEXTURE0 + 0);
	glBindTexture(GL_TEXTURE_2D, renderedTex);

	SpectreDocument* doc = spectre_document_new();
	unsigned char* rendered_data;
	int row_length;
	spectre_document_load(doc, "./postscript.ps");
	spectre_document_render(doc, &rendered_data, &row_length);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, row_length/4, row_length/4, 0, GL_BGRA, GL_UNSIGNED_BYTE, rendered_data);

  glEnable(GL_SCISSOR_TEST);
  for (int i = 0; i < CANVAS_HEIGHT; i += SCANLINE_SIZE) {
	  glScissor(0,i,1920,SCANLINE_SIZE);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glFinish();
		while (gtk_events_pending()) gtk_main_iteration();
  }

#ifdef TIME_RENDER
  printf("render time: %f\n", g_timer_elapsed(gtimer, NULL));
#endif
  return TRUE;
}

static void on_realize(GtkGLArea *glarea)
{
	gtk_gl_area_make_current(glarea);

	// compile shader
	GLuint f = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(f, 1, &shader_frag_min, NULL);
	glCompileShader(f);

#ifdef DEBUG
	GLint isCompiled = 0;
	glGetShaderiv(f, GL_COMPILE_STATUS, &isCompiled);
	if(isCompiled == GL_FALSE) {
		GLint maxLength = 0;
		glGetShaderiv(f, GL_INFO_LOG_LENGTH, &maxLength);

		char* error = malloc(maxLength);
		glGetShaderInfoLog(f, maxLength, &maxLength, error);
		printf("%s\n", error);

		quit_asm();
	}
#endif

	GLuint v = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(v, 1, &vshader, NULL);
	glCompileShader(v);

#ifdef DEBUG
	GLint isCompiled2 = 0;
	glGetShaderiv(v, GL_COMPILE_STATUS, &isCompiled2);
	if(isCompiled2 == GL_FALSE) {
		GLint maxLength = 0;
		glGetShaderiv(v, GL_INFO_LOG_LENGTH, &maxLength);

		char* error = malloc(maxLength);
		glGetShaderInfoLog(v, maxLength, &maxLength, error);
		printf("%s\n", error);

		quit_asm();
	}
#endif

	// link shader
	p = glCreateProgram();
	glAttachShader(p,v);
	glAttachShader(p,f);
	glLinkProgram(p);

#ifdef DEBUG
	GLint isLinked = 0;
	glGetProgramiv(p, GL_LINK_STATUS, (int *)&isLinked);
	if (isLinked == GL_FALSE) {
		GLint maxLength = 0;
		glGetProgramiv(p, GL_INFO_LOG_LENGTH, &maxLength);

		char* error = malloc(maxLength);
		glGetProgramInfoLog(p, maxLength, &maxLength,error);
		printf("%s\n", error);

		quit_asm();
	}
#endif

	glGenVertexArrays(1, &vao);

  glEnable(GL_TEXTURE_2D);
  glGenTextures(1, &renderedTex);
  glBindTexture(GL_TEXTURE_2D, renderedTex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void _start() {
	asm volatile("sub $8, %rsp\n");
#ifdef TIME_RENDER
	gtimer = g_timer_new();
#endif

	typedef void (*voidWithOneParam)(int*);
	voidWithOneParam gtk_init_one_param = (voidWithOneParam)gtk_init;
	(*gtk_init_one_param)(NULL);

	GtkWidget *win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	GtkWidget *glarea = gtk_gl_area_new();
	gtk_container_add(GTK_CONTAINER(win), glarea);

	g_signal_connect(win, "destroy", gtk_main_quit, NULL);
	g_signal_connect(win, "key_press_event", G_CALLBACK(check_escape), NULL);
	g_signal_connect(glarea, "realize", G_CALLBACK(on_realize), NULL);
	g_signal_connect(glarea, "render", G_CALLBACK(on_render), NULL);

	gtk_widget_show_all (win);

	gtk_window_fullscreen((GtkWindow*)win);
	GdkWindow* window = gtk_widget_get_window(win);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	GdkCursor* Cursor = gdk_cursor_new(GDK_BLANK_CURSOR);
#pragma GCC diagnostic pop
	gdk_window_set_cursor(window, Cursor);

	gtk_main();

	quit_asm();
}