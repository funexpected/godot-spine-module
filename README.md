This module add [Spine](http://esotericsoftware.com/) 3.*.

## Installation

You need to compile your own version of engine with this module.

Clone engine if you didn't yet:

```bash
git clone https://github.com/godotengine/godot.git
```

Go to godot directory:
```bash
cd godot
```

Spine module works only with godot 3.* version, sou don't forget to switch the branch:
```
git fetch
git checkout 3.5
```

Add content of this this module to the `modules/spine` directory. You can add it as submodule:

```
git submodule add https://github.com/funexpected/godot-spine-module.git modules/spine
```

Now compile engine:
```
scons -j8
```

After building engine, you can found executable in the `bin` directory.

Module supports spine runtime versions **3.6**, **3.7**, **3.8**, **4.0** and **4.1**. 
Multiple versions of runtimes can be enabled at compile time:
```
scons spine_runtime_3_6=yes spine_runtime_4_1=yes
```

Multiple versions of runtime works at the same time, using the same `Spine` node.


## Usage

- Add `Spine` node to the scene.
- Make sure the animation and atlas filenames has the same name (`spineboy.skel` and `spineboy.atlas` for example).
- Set `.skel` or `.json` file to `resource` property
- Select the animation
- Enable the node using `active` property
- Discover properties and API using in-editor help
- Write the issues for explanations.

## About the lisence

This module is forked from [sanikoyes's godot branch](https://github.com/sanikoyes/godot/tree/develop/modules/spine) and some of code are forked from [godot-spine-module](https://github.com/jjay/godot-spine-module). Both of the code are declared as MIT lisence.
