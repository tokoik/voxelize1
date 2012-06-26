#include <stdio.h>
#if defined(WIN32)
//#  pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")
#  pragma comment(lib, "glew32.lib")
#  include "glew.h"
#  include "glut.h"
#  include "glext.h"
#elif defined(__APPLE__) || defined(MACOSX)
#  include <GLUT/glut.h>
#else
#  define GL_GLEXT_PROTOTYPES
#  include <GL/glut.h>
#endif

#ifdef DEBUG
static void CheckFramebufferStatus(void)
{
  GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);

  switch (status) {
  case GL_FRAMEBUFFER_COMPLETE_EXT:
    break;
  case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
    fprintf(stderr, "Unsupported framebuffer format\n");
    break;
  case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
    fprintf(stderr, "Framebuffer incomplete, missing attachment\n");
    break;
  case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
    fprintf(stderr, "Framebuffer incomplete, duplicate attachment\n");
    break;
  case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
    fprintf(stderr, "Framebuffer incomplete, attached images must have same dimensions\n");
    break;
  case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
    fprintf(stderr, "Framebuffer incomplete, attached images must have same format\n");
    break;
  case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
    fprintf(stderr, "Framebuffer incomplete, missing draw buffer\n");
    break;
  case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
    fprintf(stderr, "Framebuffer incomplete, missing read buffer\n");
    break;
  default:
    fprintf(stderr, "Programming error; will fail on all hardware: %04x\n", status);
    break;
  }
}

static void CheckOpenGLError(const char *str)
{
  GLenum error = glGetError();

  switch (error) {
  case GL_NO_ERROR:
    break;
  case GL_INVALID_ENUM:
    fprintf(stderr, "%s: An unacceptable value is specified for an enumerated argument\n", str);
    break;
  case GL_INVALID_VALUE:
    fprintf(stderr, "%s: A numeric argument is out of range\n", str);
    break;
  case GL_INVALID_OPERATION:
    fprintf(stderr, "%s: The specified operation is not allowed in the current state\n", str);
    break;
  case GL_STACK_OVERFLOW:
    fprintf(stderr, "%s: This command would cause a stack overflow\n", str);
    break;
  case GL_STACK_UNDERFLOW:
    fprintf(stderr, "%s: This command would cause a a stack underflow\n", str);
    break;
  case GL_OUT_OF_MEMORY:
    fprintf(stderr, "%s: There is not enough memory left to execute the command\n", str);
    break;
  case GL_TABLE_TOO_LARGE:
    fprintf(stderr, "%s: The specified table exceeds the implementation's maximum supported table size\n", str);
    break;
#  ifndef GL_INVALID_FRAMEBUFFER_OPERATION
#    define GL_INVALID_FRAMEBUFFER_OPERATION 0x0506
#  endif
  case GL_INVALID_FRAMEBUFFER_OPERATION:
    fprintf(stderr, "%s: The specified operation is not allowed current frame buffer\n", str);
    break;
  default:
    fprintf(stderr, "%s: An OpenGL error has occured: 0x%04x\n", str, error);
    break;
  }
}
#else
#  define CheckFramebufferStatus()
#  define CheckOpenGLError(str)
#endif

/*
** アニメーションのサイクル（ミリ秒）
*/
#define CYCLE 5000

/*
** 境界箱
*/
static GLdouble pmin[] = { -1.0, -1.0, -1.0 };
static GLdouble pmax[] = {  1.0,  1.0,  1.0 };

/*
** ボリュームの大きさ
*/
#define SLICEX 256
#define SLICEY 256
#define SLICEZ 256

/*
** ピクセルバッファオブジェクト
*/
static GLuint pb;

/*
** ボクセル化
*/
static void voxelize(const GLdouble pmin[], const GLdouble pmax[],
                     GLuint sx, GLuint sy, GLuint sz,
                     GLuint pb)
{
  GLuint rb, fb;

  /* レンダーバッファオブジェクトを生成して結合する */
  glGenRenderbuffersEXT(1, &rb);
  glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, rb);
  CheckOpenGLError("BindRenderbuffer");

  /* レンダーバッファオブジェクトのメモリを割り当てる */
  glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_RGB, sx, sy);
  CheckOpenGLError("RenderbufferStorage");

  /* レンダーバッファオブジェクトの結合を解除する */
  glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);

  /* フレームバッファオブジェクトを生成して結合する */
  glGenFramebuffersEXT(1, &fb);
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb);
  CheckOpenGLError("BindFramebuffer");

  /* フレームバッファオブジェクトにレンダーバッファオブジェクトを結合する */
  glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
    GL_RENDERBUFFER_EXT, rb);
  CheckOpenGLError("FramebufferRenderbuffer");
  CheckFramebufferStatus();

  /* ピクセルバッファオブジェクトを結合する */
  glBindBuffer(GL_PIXEL_PACK_BUFFER, pb);
  CheckOpenGLError("BindPixelBuffer");

  /* 背景色 */
  glClearColor(0.0, 0.0, 0.0, 1.0);

  /* 前景色 */
  glColor3d(1.0, 1.0, 1.0);

  /* フレームバッファに書きこむたびにフレームバッファの内容を反転する */
  glLogicOp(GL_INVERT);

  /* 隠面消去処理は行わない */
  glDisable(GL_DEPTH_TEST);

  /* ビューポートをボリュームの xy 平面の大きさに一致させる */
  glViewport(0, 0, sx, sy);

  /* 視点を境界箱の前方面の位置に移動する */
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslated(0.0, 0.0, -pmax[2]);

  /* 視野空間をずらしながら描画する */
  for (GLuint z = 0; z < sz; ++z) {
    GLdouble offset = (pmax[2] - pmin[2]) * ((GLdouble)z + 0.5) / (GLdouble)sz;

    /* 境界箱を視野空間に設定する */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(pmin[0], pmax[0], pmin[1], pmax[1], offset, pmax[2] - pmin[2]);
    glMatrixMode(GL_MODELVIEW);

    /* 画面消去 */
    glClear(GL_COLOR_BUFFER_BIT);
    CheckOpenGLError("Clear");

    /* 論理演算処理開始 */
    glEnable(GL_COLOR_LOGIC_OP);

    /* 図形表示 */
    glutSolidTeapot(0.5);
    //glutSolidSphere(0.9, 4000, 2000);
    CheckOpenGLError("draw");

    /* 論理演算処理終了 */
    glDisable(GL_COLOR_LOGIC_OP);

    glFlush();

    /* フレームバッファオブジェクトからデータを読み出す */
    glReadPixels(0, 0, sx, sy, GL_RED, GL_UNSIGNED_BYTE,
      (GLubyte *)0 + sx * sy * z);
    CheckOpenGLError("ReadPixels");
  }

  /* ピクセルバッファオブジェクトの結合を解除する */
  glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

  /* フレームバッファオブジェクトの結合を解除する */
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

  /* フレームバッファオブジェクトを削除する */
  glDeleteFramebuffersEXT(1, &fb);

  /* レンダーバッファオブジェクトを削除する */
  glDeleteRenderbuffersEXT(1, &rb);
}

/*
** 画面表示
*/
static void display(void)
{
  int z = (int)((double)SLICEZ
    * (double)(glutGet(GLUT_ELAPSED_TIME) % CYCLE) / (double)CYCLE);

  glClear(GL_COLOR_BUFFER_BIT);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pb);
  glDrawPixels(SLICEX, SLICEY, GL_LUMINANCE, GL_UNSIGNED_BYTE,
    (GLubyte *)0 + SLICEX * SLICEY * z);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
  glutSwapBuffers();
}

/*
** アニメーション
*/
static void idle(void)
{
  glutPostRedisplay();
}

/*
** 初期設定
*/
static void init(void)
{
#if defined(WIN32)
  /* GLEW の初期化 */
  GLenum err = glewInit();
  if (err != GLEW_OK) {
    fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
    exit(1);
  }
#endif

  /* ピクセルバッファオブジェクトのを作成して結合する */
  glGenBuffers(1, &pb);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pb);
  CheckOpenGLError("BindPixelBuffer");

  /* ピクセルバッファオブジェクトのメモリを確保する */
  glBufferData(GL_PIXEL_UNPACK_BUFFER, SLICEX * SLICEY * SLICEZ, 0, GL_DYNAMIC_COPY);
  CheckOpenGLError("BufferData");

  /* ピクセルバッファオブジェクトの結合を解除する */
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
  CheckOpenGLError("ReleasePixelBuffer");

  /* ボクセル化 */
  voxelize(pmin, pmax, SLICEX, SLICEY, SLICEZ, pb);
}

/*
** メインプログラム
*/
int main(int argc, char *argv[])
{
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
  glutCreateWindow(argv[0]);
  glutDisplayFunc(display);
  glutIdleFunc(idle);
  init();
  glutMainLoop();

  return 0;
}
