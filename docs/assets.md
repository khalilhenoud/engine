Requirements:
-------------
- An asset is referred to in engine with a special type, e.g: asset_ref_t
- An asset cannot be used before it is loaded. Functions such as is_loaded(),
is_loading(), get_as_$type$(), unload() etc... are required to work with assets.
- Every asset type corresponds to an in-engine type, assets can themselves refer
to other assets (mesh references materials).
- Every asset type has its own loader (wether simple or complex is irrelevant).
- Assets are immutable in-engine (the editor is a different story).
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

Questions:
----------
- Are we going to support nested folders ? No.
- Are we going to support human readable names ? At the beginning we will, but
ultimately that might not be the case.
- Duplicates ? At the beginning we will allow them. That will change once we
move away from human readable names.

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