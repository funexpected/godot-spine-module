# modules/spine — Spine 2D skeletal animation

Godot 3.3 binding for [Esoteric Software's Spine Runtimes](http://esotericsoftware.com/). Loads Spine projects (`.skel`/`.json` + `.atlas` + textures) and plays them on a `Node2D`.

## Multi-version runtime

The module embeds multiple Spine C runtimes side-by-side under [runtime/](runtime/). Version is selected at build time:

```bash
scons spine_runtime_3_8=yes spine_runtime_4_1=yes
```

Supported: `3.6`, `3.7`, `3.8`, `4.0`, `4.1`. Defaults: `4.1` and `3.6` (see [spine_versions.py:1](spine_versions.py#L1)). Exact upstream commits are pinned per version in the same file.

## Source files

| File | Role |
|------|------|
| [spine.h](spine.h) / [spine.cpp](spine.cpp) | `Spine` Node2D — the main scene node. |
| `spine_runtime.*` (per-version in `runtime/`) | C wrappers around each Spine-runtime version. |
| [animation_node_spine.h](animation_node_spine.h) / [.cpp](animation_node_spine.cpp) | `SpineMachine` (AnimationTree) + `AnimationNodeSpineAnimation` (AnimationRootNode). |
| [spine_batcher.h](spine_batcher.h) / [.cpp](spine_batcher.cpp) | Draw-call batching for Spine meshes. |
| [register_types.cpp](register_types.cpp) | ClassDB registration, `ResourceFormatLoaderSpine` for `.skel`/`.json`/`.atlas`. |

## Registered classes

- `Spine` (Node2D) — the playback node.
- `SpineResource` (Resource) — loaded skeleton + atlas.
- `SpineMachine` (AnimationTree) — drives a `Spine` node from an animation tree.
- `AnimationNodeSpineAnimation` (AnimationRootNode) — picks an animation by name.

Resource loader ([register_types.cpp:130-134](register_types.cpp#L130-L134)) recognizes `.skel`, `.json`, `.atlas`.

## Usage

```gdscript
var spine := Spine.new()
spine.resource = preload("res://hero.json")     # Spine export
add_child(spine)
spine.play("idle", 0, true)                     # name, track, loop
```

## Origin & license

Forked from community modules (`sanikoyes/godot-spine`, `godot-spine-module`) and extended by Funexpected with multi-runtime versioning and editor integration.

Upstream Spine runtime code: [Spine Runtimes Software License v2.5](LICENSE) (Esoteric Software). Binding code: MIT.
