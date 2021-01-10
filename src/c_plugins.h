#ifndef CPLUGINS_INCLUDED
#	define CPLUGINS_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "harbol_common_defines.h"
#include "harbol_common_includes.h"

#include "tinydir.h"

#ifndef CPLUGINS_API
#	define CPLUGINS_API    static inline
#endif


#ifdef OS_WINDOWS
#	include <windows.h>
#	include <direct.h>
#	include <synchapi.h>
	static inline void dosleep(int ms) {
		Sleep(ms);
	}
#	define stat _stat
#else
#	include <dlfcn.h>
#	include <unistd.h>
	static inline void dosleep(int ms) {
		usleep(ms * 1000);
	}
#endif


#ifdef OS_WINDOWS
#	ifndef MODULE_LOAD
#		define MODULE_LOAD(str)   LoadLibrary(str)
#	endif
#	ifndef MODULE_GET_OBJ
#		define MODULE_GET_OBJ     GetProcAddress
#	endif
#	ifndef MODULE_CLOSE
#		define MODULE_CLOSE       FreeLibrary
#	endif
#else
#	ifndef MODULE_LOAD
#		define MODULE_LOAD(str)   dlopen(str, RTLD_LAZY | RTLD_GLOBAL)
#	endif
#	ifndef MODULE_GET_OBJ
#		define MODULE_GET_OBJ     dlsym
#	endif
#	ifndef MODULE_CLOSE
#		define MODULE_CLOSE       dlclose
#	endif
#endif

#ifdef OS_WINDOWS
#	ifndef DIR_SEPARATOR
#		define DIR_SEPARATOR      "\\"
#	endif
#	ifndef LIB_EXT
#		define LIB_EXT            "dll"
#	endif
#elif defined OS_LINUX_UNIX
#	ifndef DIR_SEPARATOR
#		define DIR_SEPARATOR      "/"
#	endif
#	ifndef LIB_EXT
#		define LIB_EXT            "so"
#	endif
#elif defined OS_MAC
#	ifndef DIR_SEPARATOR
#		define DIR_SEPARATOR      "/"
#	endif
#	ifndef LIB_EXT
#		define LIB_EXT            "dylib"
#	endif
#endif


#ifdef OS_WINDOWS
typedef HMODULE PluginDLL;
#else
typedef void   *PluginDLL;
#endif


typedef void   *PlObj;



CPLUGINS_API char *alloc_copy_str(const char *restrict s) {
	const size_t len = strlen(s);
	char *cpy = calloc(len + 1, sizeof *cpy);
	strcpy(cpy, s);
	return cpy;
}


CPLUGINS_API char *sprintf_alloc(const char *restrict fmt, ...) {
	va_list ap, st;
	va_start(ap, fmt);
	va_copy(st, ap);
	
	char c = 0;
	const int32_t size = vsnprintf(&c, 1, fmt, ap);
	va_end(ap);
	
	char *restrict text = calloc(size + 2, sizeof *text);
	if( text != NULL ) {
		vsnprintf(text, size + 1, fmt, st);
	}
	va_end(st);
	return text;
}


enum {
	CPluginErrNone,
	CPluginErrNoPath,      /// 'path' member didn't properly initialize or allocate.
	CPluginErrLibLoadFail, /// DLL failed to load for some reason, check OS specific error.
	CPluginErrNoLibLoaded, /// No DLL was ever loaded.
	CPluginErrCantReload,  /// DLL failed to reload.
};


typedef struct Plugin {
	PluginDLL   dll;
	char       *path;
	time_t      last_write;
	int         err;
} SPlugin;


typedef void PluginEvent(struct Plugin *pl_ctxt, void *userdata, bool reloading);

CPLUGINS_API NEVER_NULL(1,2) bool plugin_load(struct Plugin *pl, const char path[], PluginEvent load_fn, void *userdata) {
	pl->dll = MODULE_LOAD(path);
	if( pl->dll==NULL ) {
		pl->err = CPluginErrLibLoadFail;
		return false;
	}
	
	pl->path = alloc_copy_str(path);
	struct stat result;
	if( !stat(path, &result) ) {
		pl->last_write = result.st_mtime;
	}
	if( load_fn != NULL ) {
		(*load_fn)(pl, userdata, false);
	}
	return true;
}

CPLUGINS_API NEVER_NULL(1) void plugin_clear(struct Plugin *pl, PluginEvent unload_fn, void *userdata) {
	if( unload_fn != NULL ) {
		(*unload_fn)(pl, userdata, false);
	}
	free(pl->path), pl->path=NULL;
	if( pl->dll != NULL ) {
		MODULE_CLOSE(pl->dll), pl->dll=NULL;
	}
	*pl = ( struct Plugin ){0};
}

CPLUGINS_API NO_NULL bool plugin_changed(struct Plugin *pl) {
	if( pl->path==NULL ) {
		pl->err = CPluginErrNoPath;
		return false;
	}
	
	struct stat result;
	if( !stat(pl->path, &result) ) {
		const bool change = pl->last_write != result.st_mtime;
		if( change ) {
			pl->last_write = result.st_mtime;
		}
		return change;
	}
	return false;
}

CPLUGINS_API NEVER_NULL(1) bool plugin_reload(struct Plugin *pl, PluginEvent load_fn, PluginEvent unload_fn, void *userdata) {
	if( pl->path==NULL ) {
		pl->err = CPluginErrNoPath;
		return false;
	}
	
	if( pl->dll != NULL ) {
		if( unload_fn != NULL ) {
			(*unload_fn)(pl, userdata, true);
		}
		MODULE_CLOSE(pl->dll);
	}
	
	pl->dll = MODULE_LOAD(pl->path);
	if( pl->dll==NULL ) {
		pl->err = CPluginErrCantReload;
		return false;
	}
	if( load_fn != NULL ) {
		(*load_fn)(pl, userdata, true);
	}
	return true;
}

CPLUGINS_API NO_NULL NONNULL_RET const char *plugin_get_err(const struct Plugin *pl) {
	switch( pl->err ) {
		case CPluginErrNone:        return "CPlugins :: No Error";
		case CPluginErrNoPath:      return "CPlugins :: Err **** No file path recorded for the plugin. ****";
		case CPluginErrLibLoadFail: return "CPlugins :: Err **** DLL failed to load. ****";
		case CPluginErrNoLibLoaded: return "CPlugins :: Err **** No DLL is loaded. ****";
		case CPluginErrCantReload:  return "CPlugins :: Err **** DLL failed to reload. ****";
		default:                    return "CPlugins :: Err **** Unknown Error. ****";
	}
}

/*
CPLUGINS_API NO_NULL void plugin_info(const struct Plugin *pl) {
	printf("Plugin:: DLL - '%p'\n", pl->dll);
	if( pl->path != NULL ) {
		printf("Plugin:: Path - '%s'\n", pl->path);
	}
	printf("Plugin:: Last Write Time - '%li'\n", pl->last_write);
}
*/

CPLUGINS_API NO_NULL void *plugin_get_obj(struct Plugin *pl, const char name[]) {
	if( pl->dll==NULL ) {
		pl->err = CPluginErrNoLibLoaded;
		return NULL;
	}
	pl->err = CPluginErrNone;
	return( void* )( uintptr_t )MODULE_GET_OBJ(pl->dll, name);
}



typedef void PluginDirEvent(const char pl_path[], const char pl_name[], void *userdata);

CPLUGINS_API void _dir_open(tinydir_dir *dir, PluginDirEvent dir_fn, void *userdata, const char *ext) {
	while( dir->has_next ) {
		tinydir_file *file = &( tinydir_file ){0};
		if( tinydir_readfile(dir, file) < 0 ) {
			continue;
		} else if( file->is_dir ) {
			if( file->name[0]=='.' ) {
				/// advance if parent dir dot.
				goto dir_iter_loop;
			}
			tinydir_dir *sub_dir = &( tinydir_dir ){0};
			if( tinydir_open(sub_dir, file->path) < 0 ) {
				/// jumping to tinydir_next at end of loop so we can advance the dir iterator.
				goto dir_iter_loop;
			} else {
				_dir_open(sub_dir, dir_fn, userdata, ext);
			}
		} else if( !strcmp(file->extension, ext) ) {
			(*dir_fn)(file->path, file->name, userdata);
		}
	dir_iter_loop:;
		if( tinydir_next(dir) < 0 ) {
			break;
		}
	}
	tinydir_close(dir);
}

CPLUGINS_API NEVER_NULL(1) bool plugin_dir_open(const char dir[], PluginDirEvent dir_fn, void *userdata, const char custom_ext[]) {
	/// 'FILENAME_MAX' defined in <stdio.h>
	char currdir[FILENAME_MAX] = {0};
#ifdef OS_WINDOWS
	if( GetCurrentDirectory(sizeof currdir, currdir) != 0 )
#else
	if( getcwd(currdir, sizeof currdir) != NULL )
#endif
	{
		char *pl_dir = sprintf_alloc("%s%s%s", currdir, DIR_SEPARATOR, dir);
		tinydir_dir *dir_handle = &( tinydir_dir ){0};
		if( tinydir_open(dir_handle, pl_dir) < 0 ) {
			fprintf(stderr, "Plugin Directory Error: **** unable to open dir: '%s' ****\n", pl_dir);
			tinydir_close(dir_handle);
		} else {
			_dir_open(dir_handle, dir_fn, userdata, custom_ext==NULL ? LIB_EXT : custom_ext);
		}
		free(pl_dir);
		return true;
	}
	return false;
}


#ifdef __cplusplus
} /// extern "C"
#endif

#endif /** CPLUGINS_INCLUDED */
