# C interface

# Datatypes

## struct Plugin, typedef'd as 'SPlugin'

### dll
dynamically loaded library object.

### path
(mutable) string of the dll path with file name.

### last_write
last modification time of the dll object.

### err
int value that stores specific error codes when a plugin issue arises.


## anonymous enum

### CPluginErrNone
error code indicating no error.

### CPluginErrNoPath
error code indicating the 'path' member (see struct Plugin) wasn't allocated nor initialized.

### CPluginErrLibLoadFail
error code indicating the dynamically loaded library failed to load.

### CPluginErrNoLibLoaded
error code indicating that there's no DLL loaded to continue plugin operations.

### CPluginErrCantReload
error code indicating the DLL failed to reload.


## PluginEvent

plugin callback signature for when a plugin has been loaded, unloaded, or reloaded.

```c
void PluginEvent(struct Plugin *pl_ctxt, void *userdata, bool reloading);
```


## PluginDirEvent

plugin callback signature for when iterating a plugin directory and a plugin with the appropriate extension has been found.

```c
void PluginDirEvent(const char pl_path[], const char pl_name[], void *userdata);
```


# Functions/Methods

## plugin_load
```c
bool plugin_load(struct Plugin *pl, const char path[], PluginEvent load_fn, void *userdata);
```

### Description
Loads a dynamically loaded library object (DLL) to a Plugin instance and invokes an optional plugin load callback.

### Parameters
* `pl` - pointer to a `Plugin` instance.
* `path` - file path to the DLL object.
* `load_fn` - pointer to a loading function, can be NULL.
* `userdata` - pointer to data that is given to `load_fn` when invoked.

### Return Value
true if loading was successful, false if an error occurred, check the `err` member and use `plugin_get_err` to get an error message string.

### Example
```c
void on_load_dll_example(struct Plugin *pl_ctxt, void *userdata, bool reloading) {
	puts(userdata);
}

int main(void)
{
	struct Plugin pl = {0};
	if( !plugin_load(&pl, "./libexample.so", &on_load_dll_example, "loaded libexample!") ) {
		puts(plugin_get_err(&pl));
		return 1;
	}
}
```


## plugin_clear
```c
void plugin_clear(struct Plugin *pl, PluginEvent unload_fn, void *userdata)
```

### Description
unloads and deallocates/cleans up internal data for a plugin instance.

### Parameters
* `pl` - pointer to plugin instance.
* `unload_fn` - function pointer to call before cleaning up, can be NULL.
* `userdata` - pointer to data to pass to `unload_fn`.

### Return Value
None

### Example
```c
void on_load_dll_example(struct Plugin *pl_ctxt, void *userdata, bool reloading) {
	puts(userdata);
}

void on_unload_dll_example(struct Plugin *pl_ctxt, void *userdata, bool reloading) {
	puts(userdata);
}

int main(void)
{
	struct Plugin pl = {0};
	if( !plugin_load(&pl, "./libexample.so", &on_load_dll_example, "loaded libexample!") ) {
		puts(plugin_get_err(&pl));
		return 1;
	}
	
	...
	
	plugin_clear(&pl, &on_unload_dll_example, "unloaded libexample!");
}
```


## plugin_changed
```c
bool plugin_changed(struct Plugin *pl);
```

### Description
checks if a plugin's DLL object file was changed.

### Parameters
* `pl` - pointer to raw script data.

### Return Value
true if the file did change, false otherwise.

### Example
```c
struct Plugin pl;

...

if( plugin_changed(&pl) ) {
	/// code.
}
```


## plugin_reload
```c
bool plugin_reload(struct Plugin *pl, PluginEvent load_fn, PluginEvent unload_fn, void *userdata);
```

### Description
Unloads and then reloads a plugin's DLL object.

### Parameters
* `pl` - pointer to Plugin instance.
* `load_fn` - function pointer invoked when plugin has been reloaded, can be NULL.
* `unload_fn` - function pointer invoked when plugin has been unloaded, can be NULL.
* `userdata` - pointer to data passed to `load_fn` and `unload_fn`.

### Return Value
true if plugin was reloaded, false otherwise. Errors return false. check `err` field and use `plugin_get_err` for error string message.

### Example
```c
struct Plugin pl;

...

if( plugin_changed(&pl) && !plugin_reload(&pl, on_plugin_load, on_plugin_unload, &data) ) {
	puts(plugin_get_err(&pl));
}
```


## plugin_get_err
```c
const char *plugin_get_err(const struct Plugin *pl);
```

### Description
gives an appropriate string message for a specific error code on a plugin instance.

### Parameters
* `pl` - pointer to a plugin instance.

### Return Value
pointer to a string.


## plugin_get_obj
```c
void *plugin_get_obj(struct Plugin *pl, const char name[]);
```

### Description
retrieves an object pointer from the Plugin DLL.

### Parameters
* `pl` - pointer to Plugin instance.
* `name` - string name of the object to get.

### Return Value
pointer to the object, NULL if error occurred.

### Example
```c
void on_plugin_load(struct Plugin *pl_ctxt, void *userdata, bool reloading) {
	struct Data *d = userdata;
	d->start_func  = plugin_get_obj(pl_ctxt, "start_func");
	d->stop_func   = plugin_get_obj(pl_ctxt, "stop_func");
	d->update_func = plugin_get_obj(pl_ctxt, "update_func");
	d->global_data = plugin_get_obj(pl_ctxt, "global_data");
}
```


## plugin_dir_open
```c
bool plugin_dir_open(const char dir[], PluginDirEvent dir_fn, void *userdata, const char custom_ext[]);
```

### Description
searches through a specific directory for plugins. This also searches subfolders as well!

### Parameters
* `dir` - folder path that will hold plugins.
* `dir_fn` - function pointer invoked when a plugin with the system or custom extension name was found.
* `userdata` - pointer to data passed to `dir_fn`.
* `custom_ext` - string of a custom file extension for application specific plugins.

### Return Value
true if directory was accessible, false otherwise.

### Example
```c
struct Data {
	struct Plugin pl;
	...;
};

void load_our_plugins(const char pl_path[], const char pl_name[], void *userdata) {
	struct Data *d = userdata;
	if( !plugin_load(&d->pl, pl_path, on_plugin_load, d) ) {
		puts(plugin_get_err(&d->pl));
	}
}

int main(void) {
	struct Data d = {0};
	if( !plugin_dir_open("plugins", load_our_plugins, &d, NULL) ) {
		puts(plugin_get_err(&d.pl));
		return 1;
	}
}
```