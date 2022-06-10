# Geoflow
Flowchart tool for geo-spatial data processing. Highly experimental with many rough edges!

![Build](https://github.com/geoflow3d/geoflow/workflows/Build/badge.svg)

# Installation
## Requirements
+ [nlohmann JSON](https://github.com/nlohmann/json/releases) at least version 3.10.5

## Windows
Simply download and run the Geoflow-bundle installer from the [latest release](https://github.com/geoflow3d/geoflow-bundle/releases/latest). This will also install the most important plugins.

### Installing and using plugins
Geoflow plugins provide the nodes for Geoflow. Nodes are the functional parts of the flowchart that implement a specific function. You can find the plugins on the [geoflow3d organisation page](https://github.com/geoflow3d). The plugin repositories have the `gfp-` prefix, that is an acronym for geoflow plugin.

To use plugins you need to do this:

1. Set an environment variable called `GF_PLUGIN_FOLDER` that points to a directory where you will put the plugins, e.g. `C:\Users\<USERNAME>\.geoflow\plugins`.
2. Download the plugin from the plugin repository, eg. the [LAS plugin](https://github.com/geoflow3d/gfp-las/releases/latest). Currently a plugin on Windows is simply a `.dll` file.
3. Copy the plugin file, eg. `gfp_las.dll` to the `GF_PLUGIN_FOLDER` folder you set up earlier.

Geoflow should automatically load all plugins that it finds in the `GF_PLUGIN_FOLDER` folder. Notice that a new version of geoflow may also require a new version of a plugin.

## Building from source
Requires compiler with c++17 support  (see https://en.cppreference.com/w/cpp/compiler_support).

```
mkdir build
cd build
cmake .. -DGF_BUILD_GUI=[ON|OFF]
cmake --build . --parallel 4 --config Release
```

### Building with GUI
Requires additional dependencies `glm` and `glfw` that need to be installed by the user.

### Platform specific instructions
Have a look at the [workflow files](https://github.com/tudelft3d/geoflow/tree/master/.github/workflows).

# Usage
## Command line interface (`geof`)
```
Geoflow
Usage: geof [OPTIONS] [SUBCOMMAND]

Options:
  -h,--help                   Print this help message and exit
  --verbose                   Print verbose messages
[Option Group: Info]
  Debug information
  Options:
    -v,--version                Print version information
    -p,--plugins                List available plugins
    -n,--nodes                  List available nodes from plugins that are loaded

Subcommands:
  run                         Load and run flowchart
  set                         Set flowchart globals (comes after run)

```
### examples
Print version information:
`geof --version`

Get help on subcommand:
`geof run --help`

Running a flowchart with default globals:
`geof run <flowchart file>`

List available globals:
`geof run <flowchart file> --globals`

Running a flowchart with user-specified global values:
`geof run <flowchart file> set [--config <TOML config file with globals>] [--GLOBAL1 <value> --GLOBAL2 <value> ...]`


## GUI (`geoflow`)
Takes the same parameters as `geof` on the command line.

- Right click to open the menu to create new nodes
- Drag from input/output terminals to make connections
- Right click on a node to access its context menu
- Translate in the 3D viewer by left-mouse dragging while holding `ctrl`, faster zooming by holding `ctrl` while scrolling.