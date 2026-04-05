Requirements:
-------------
- An asset is referred to in engine with a special type, e.g: asset_ref_t
- An asset cannot be used before it is loaded. Functions such as is_loaded(),
is_loading(), get_as_$type$(), unload() etc... are required to work with assets.
- Every asset type corresponds to an in-engine type, assets can themselves refer
to other assets (mesh_asset_t references material_asset_t).
- Every asset type has its own loader (wether simple or complex is irrelevant).
- Assets are immutable in-engine (only the editor can modify them).
- No duplicate assets can exist on disk. Some form of hashing (MD5) is required
to make sure no dupliates are saved to disk.
- Sometimes an asset type is designed to work with a runtime type, for example a
runtime skeletal mesh, requires a skinned mesh asset and an animation asset to
work, as it requires them to update the bone transform at runtime.
- A single uin64_t should be enough to locate any asset on disk.
- We do not support generic nested folders. To elaborate, free form folder
structures are not supported. Each asset type has a predefined folder name, that
has a flat hierarchy, all of those folders reside in a root folder. Currently
only a single root folder exists, that however might change, at which point we
might have a nested structure. But the root folder structure never changes.
- What is the relationship between assets and loaders? A loader is used by the
editor to import third party formats into our own format, and then save it to
disk in our own custom format. An asset can load the custom format directly.
- What is the difference between assets types and data types? Simply speaking,
an asset type is a leaf data type, basically it is stored on disk by itself as
opposed to part of another type, i.e: a light is part of a scene, with no
individual file to itself, a texture is stored as an individual file and
referenced as an asset_ref_t.
- Asset packages are hierarchical, with a base package 'assets'. This is a
neccessity in this particular case. As assets would naturally refer to other
assets, for example almost every visual asset would depend on the texture asset,
which itself is stored in a separate package.

Relaxed rules for initial implementation:
-----------------------------------------
- Use paths to refer to the file in question.
- We allow duplictes for now, this makes initial implementation simpler.
- No nested folders for now, only a single root folder.

SCRATCHPAD:
-----------
What is an asset type vs a data type?
Assets
  Texture, mesh, animations, materials, skeletons, skinned meshes, sounds...
Data types
  Lights, camera...
Is this really the difference?
Can the difference be define in any meaningfull way?

Is it having specializing loaders ? No, since meshes are assets but do not
require that.
Is it having the loading/unloading controlled by a specilized system ? Like a
load ondemand system ?
Is it stateless data types? The type of data that is identical between one game
and another, i.e: all games require meshes and textures. These are reusable,
stateless, crucial to building a game without being specific to it.

What about the runtime aspect of an asset, a skinned mesh requires a runtime
component to update the bones. While the asset is immutable, the runtime part is
not.

An asset has an id and a loaded/unloaded state.
An asset has to have a loaded/unloaded state. And should be identifiable with an
id

LOADER vs ASSETS:
Should there be a distinction? I am wondering about that.