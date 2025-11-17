Assets vs Data definitions and next steps
=========================================
Types can be asset referencing or not. A type that has no reference to an asset
is called a data type while types that references an asset can have its data 
loaded irrespective of wether the asset portion is loaded or not.

An example of a type that is 'currently' data only is 'light_t'. Currently the 
type can be either fully loaded as part of a scene or not at all. Loading is 
part of type serialize/deserialize functions. The reason why 'currently' is 
highlighted at the beginning of the paragraph is that a type that starts as a 
data type can ultimately become an asset type, if the type requires a reference
to an asset, for example, we might want to animate the type, but not load the 
animation track until the track becomes relevant. That part would be stored 
separately and loaded separately depending on some runtime requirements.

An example of an asset type is 'texture_t'. A texture is composed of 2 parts, a
data section which currently consists of only the path to the texture data on 
disc. The type's serialize/deserialize only loads reads the path from the 
package on disc, the data itself is not loaded. In this scenario, a runtime 
component is responsible for loading the asset in question. A texture may need 
not be loaded unless we are at a certain distance from it, or the texture might
already be loaded because another object is already using it in the scene.
In short, the runtime component is responsible for deserializing the data, which
usually happen through specialized loaders (png loader, etc...).

A asset can depend on several loaders types, for example the runtime font asset
depend on the texture and the csv file. Even though that is the case, the font
asset may not reference the image it depend on, it can only access the type 
associated with the loader defined in its package. And indeed the font data is
not related to the image in question.

Restructuring
=============
Game
----
The entity module currently is acting like a centralized place where all
dependencies are currently located. This is not ideal, instead, we are going to
create a game module that include the dependencies on the packages that the game
requires. For example, the game package will include the dependency on the mesh,
collision, math and renderer modules if it requires it. These dependencies are
on a game per game module basis.

Game will also hold composite types, for example if our Actor type is specific
to our game and if in our game the actor holds 2 meshes and 1 texture, then we
can define this type in our Game package and have the scene hold references to 
this type, or any custom type used in our game package.

Note: The editor package necessarily depends on our game package.

Entity
------
The entity module should be trimmed, with the mesh, font and texture material 
split into their own individual modules (other types can also have their own
modules depending on the granularity we require). The base structure of the 
scene should be modified to be type agnostic; basically it should be a list of
node of nodes, where each node holds a type agnostic pointer (either a 'void *' 
or a new pointer type to be introduced in the clib).
Additionally the scene will have a repo of types, this is similar to our current
setup where identical types are kept in arrays consecutively. This would be
useful to index into, as opposed to hold a hard references.

Note: might be smart to rename this to scene? Entity is more general though...

Steps
=====
1- Define a pointer type that holds the type guid it refers to. The pointer type
also caches the vtable of the type in question.
2- Move bvh, font, mesh, material and texture to their own corresponding
packages.
3- Define the appropriate loaders in their respective packages.
4- Define a Game package that references everything else. This will replace the
application package that does that right now.
5- Modify the converter to hold a reference to the game package.
6- rename converter to tool.