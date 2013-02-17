#include <stdio.h>
#include <signal.h>
#include <syslog.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <webkit/webkit.h>
#include <JavaScriptCore/JSStringRef.h>
#include <JavaScriptCore/JSContextRef.h>

gboolean on_key_press(GtkWidget*, GdkEventKey*, gpointer);

void reload_browser(int);
void toggle_fullscreen(int);
void jsmessage(int);
void maximize();
void unmaximize();

static WebKitWebView* web_view;
static GtkWidget *window;
static WebKitWebFrame *webFrame;
static JSObjectRef globalObject;
static JSContextRef context;
 
gchar* default_url = "file:///home/pi/kiosk/test-reload.html";

int main(int argc, char** argv) {

  gtk_init(&argc, &argv);

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  g_signal_connect(window, "key-press-event", G_CALLBACK(on_key_press), NULL);
  g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

  web_view = WEBKIT_WEB_VIEW(webkit_web_view_new());

  webFrame = webkit_web_view_get_main_frame (web_view);
  context = webkit_web_frame_get_global_context(webFrame);
  globalObject = JSContextGetGlobalObject(context);

  setlogmask (LOG_UPTO (LOG_NOTICE));
  openlog ("kioskbrowser", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);

  signal(SIGHUP, reload_browser);
  signal(SIGUSR1, jsmessage);
  signal(SIGUSR2, jsmessage);

  gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(web_view));

  if(argc > 1) {
    webkit_web_view_load_uri(web_view, argv[1]);
  }
  else {
    webkit_web_view_load_uri(web_view, default_url);
  }

  maximize();
  gtk_widget_show_all(window);
  gtk_main();

  return 0;
}

gboolean on_key_press(GtkWidget* window, GdkEventKey* key, gpointer userdata) {
  if(key->type == GDK_KEY_PRESS && key->keyval == GDK_F5) {
    reload_browser(0);
  }
  else if(key->type == GDK_KEY_PRESS && key->keyval == GDK_F11) {
    toggle_fullscreen(0);
  }

  return FALSE;
}

void reload_browser(int signum) {
  webkit_web_view_reload_bypass_cache(web_view);
}

void toggle_fullscreen(int signum) {
  if(gtk_window_get_decorated(GTK_WINDOW(window))) {
    maximize();
  }
  else {
    unmaximize();
  }
}

void jsmessage(int signum) {
  
  JSValueRef  arguments[2];
  JSValueRef result;
  int num_arguments = 2;

  JSStringRef myFunctionName = JSStringCreateWithUTF8CString("my_function");
  JSObjectRef functionObject = (JSObjectRef)JSObjectGetProperty(context, globalObject, myFunctionName, NULL);
  arguments[0] = JSValueMakeNumber(context, signum);
  arguments[1] = JSValueMakeNumber(context, 3.14);
  result = JSObjectCallAsFunction(context, functionObject, globalObject, num_arguments, arguments, NULL);
  syslog (LOG_NOTICE, "After SIG %d", signum);
}


void maximize() {
  gtk_window_maximize(GTK_WINDOW(window));
  gtk_window_fullscreen(GTK_WINDOW(window));
  gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
}

void unmaximize() {
  gtk_window_unmaximize(GTK_WINDOW(window));
  gtk_window_unfullscreen(GTK_WINDOW(window));
  gtk_window_set_decorated(GTK_WINDOW(window), TRUE);
  gtk_window_resize(GTK_WINDOW(window), 1280, 768);
}
