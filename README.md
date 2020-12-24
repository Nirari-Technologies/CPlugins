## Introduction

**CPlugins** is a header-only, Windows, OSX, and Linux compatible DLL-Plugin helper library for quickly creating and managing a plugin architecture.

## Usage

```c
#include "c_plugins.h"

struct GameStuff {
	struct Plugin pl;
	void        (*start_func) (struct GameStuff *gs);
	int         (*stop_func)  (struct GameStuff *gs);
	bool        (*update_func)(struct GameStuff *gs);
};

void on_plugin_load(struct Plugin *pl_ctxt, void *userdata, bool reloading) {
	struct GameStuff *gs = userdata;
	gs->start_func = plugin_get_obj(pl_ctxt, "start_func");
	gs->stop_func = plugin_get_obj(pl_ctxt, "stop_func");
	gs->update_func = plugin_get_obj(pl_ctxt, "update_func");
}

void on_plugin_unload(struct Plugin *pl_ctxt, void *userdata, bool reloading) {
	struct GameStuff *gs = userdata;
	gs->start_func = NULL;
	gs->stop_func = NULL;
	gs->update_func = NULL;
}

static void load_our_plugins(const char pl_path[], const char pl_name[], void *userdata) {
	struct GameStuff *gs = userdata;
	if( !plugin_load(&gs->pl, pl_path, on_plugin_load, gs) ) {
		puts(plugin_get_err(&gs->pl));
	}
}


int main(void) {
	struct GameStuff gs = {0};
	if( !plugin_dir_open("plugins", load_our_plugins, &gs, NULL) ) {
		puts(plugin_get_err(&gs.pl));
		return 1;
	}
	
	int controller = 0;
	do {
		controller = getchar();
		if( plugin_changed(&gs.pl) && !plugin_reload(&gs.pl, on_plugin_load, on_plugin_unload, &gs) ) {
			puts(plugin_get_err(&gs.pl));
			break;
		}
		
		if( gs.start_func != NULL )  { (*gs.start_func)(&gs); }
		if( gs.update_func != NULL ) { (*gs.update_func)(&gs); }
		if( gs.stop_func != NULL )   { (*gs.stop_func)(&gs); }
	} while( controller != 'q' && controller != 'Q' );
	
	plugin_clear(&gs.pl, on_plugin_unload, &gs);
}
```

## Installation

### Requirements

C99 compliant compiler
It is preferable that you compile using GCC or Clang/LLVM.

### Installation
Just drag and drop into your project folder and include it into your code.

## Credits
* Khanno Hanna / Assyrianic.

## License
CPlugins is licensed under MIT License.
