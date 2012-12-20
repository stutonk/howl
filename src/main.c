#include "main.h"
#include <gio/gio.h>

static void lua_run(int argc, char *argv[], const gchar *app_root, lua_State *L)
{
  gchar *start_script;
  int status, i;

  start_script = g_build_filename(app_root, "lib", "howl", "init.lua", NULL);
  status = luaL_loadfile(L, start_script);
  g_free(start_script);

  if (status) {
    fprintf(stderr, "Couldn't load file: %s\n", lua_tostring(L, -1));
    exit(1);
  }

  lua_pushstring(L, (char *)app_root);
  lua_newtable(L);
  for(i = 0; i < argc; ++i) {
    lua_pushnumber(L, i + 1);
    lua_pushstring(L, argv[i]);
    lua_settable(L, -3);
  }
  status = lua_pcall(L, 2, 0, 0);
  if (status) {
      g_critical("Failed to run script: %s\n", lua_tostring(L, -1));
      exit(1);
  }
}

static gchar *get_app_root(const gchar *invocation_path)
{
  gchar *path;
  GFile *root, *app, *parent;

  app = g_file_new_for_path(invocation_path);
  parent = g_file_get_parent(app);
  root = g_file_get_parent(parent);
  path = g_file_get_path(root);
  g_object_unref(app);
  g_object_unref(parent);
  g_object_unref(root);
  return path;
}

static lua_State *open_lua_state(const gchar *app_root)
{
  lua_State *l = luaL_newstate();
  luaL_openlibs(l);

  /* lpeg */
  luaopen_lpeg(l);
  lua_pop(l, 1);

  /* lgi */
  lua_getglobal(l, "package");
  lua_getfield(l, -1, "loaded");
  luaopen_lgi_corelgilua51(l);
  lua_setfield(l, -2, "lgi.corelgilua51");
  lua_pop(l, 2);

  /* core bindings */
  lua_newtable(l);
  sci_init(l, app_root);
  lua_setglobal(l, "_core");

  return l;
}

int main(int argc, char *argv[])
{
  g_type_init();
  gchar *app_root = get_app_root(argv[0]);
  lua_State *L = open_lua_state(app_root);
  gtk_init(&argc, &argv);
  lua_run(argc, argv, app_root, L);
  lua_close(L);
  g_free(app_root);
  sci_close();
  return 0;
}
