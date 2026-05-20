skinned_mesh_asset_t has a lot of issues, in terms of how it is architected,
and the question on cleaning it up is front of mind.

bvh_asset_t does not make sense to me, this seems to be a weird thing to make
into an asset. Like it could be a type, but not really an asset. It seems insane
to me to make it into an asset. It does not mesh well conceptually with that.

skinned_mesh_asset_t can be simplified and will be, but bvh_asset_t is a
structural issue. It isn't necessarily a collision algorithm, more like spatial
organizational algorithm, so I am not sure where it fits. Also it needs to be
accessible in a number of places, some assets needs it, other packages might
need it as well. So where and how to put it at? Part of the base library?

What are some other spatial partioning algorithms: bvh, octree, bsp, kdtree,
etc...

What if there is spatial package: How would that work, in terms of access to
this from an asset perspective. It would be built on top of math, like collision
is...