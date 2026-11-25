Plan:
 first being able to generate unique 64bit ints as short guids

vqs_t
	vector3f v;
	quatf q;
	vector3f s;

transform_component_t				// we can specialize it in the future
	vqs_t transform;

mesh_component_t
	vqs_t offset;
	asset_ref mesh;
	asset_ref material;

font_component_t
	asset_ref font;

sublevel_component_t
	vqs_t transform;
	asset_ref sublevel;

level_component_t
	vqs_t transform;
	asset_ref level;

skinned_mesh_component_t
	vqs_t offset;
	asset_ref mesh;
	asset_ref skeleton;
	asset_ref anim_graph;

logic_component_t
	unique uint64_t identifer // that can be mapped to code

light_component_t
	vqs transform;
	light_t light;

bulk_light_component_t
	cvector_t transforms;
	cvector_t lights;

camera_component_t
	camera_t camera;

node_t/entity_t/scene_t these are all the same thing

identifier {
	union {
		cstring_t name;
		uint64_t id;
	};
};

node_t
	cvector<node_t> children;
	identifiter_t id;
	payload_t payload;		// pointer or not?

struct payload_t {
	cvector_t components;	// requires a { type_id, void * } pair
};