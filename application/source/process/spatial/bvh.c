/**
 * @file bvh.c
 * @author khalilhenoud@gmail.com
 * @brief non-general bvh for simplistic collision detection.
 * @version 0.1
 * @date 2023-10-07
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <assert.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <application/process/spatial/bvh.h>
#include <library/allocator/allocator.h>

#define FLOOR_ANGLE_DEGREES 60
#define PRIMITIVES_PER_LEAF 8


uint32_t
get_bvh_primitives_per_leaf(void)
{
  return PRIMITIVES_PER_LEAF;
}

static
void
merge_aabb(bvh_aabb_t* dst, const bvh_aabb_t* a, const bvh_aabb_t* b)
{
  dst->min_max[0].data[0] = fmin(a->min_max[0].data[0], b->min_max[0].data[0]);
  dst->min_max[0].data[1] = fmin(a->min_max[0].data[1], b->min_max[0].data[1]);
  dst->min_max[0].data[2] = fmin(a->min_max[0].data[2], b->min_max[0].data[2]);

  dst->min_max[1].data[0] = fmax(a->min_max[1].data[0], b->min_max[1].data[0]);
  dst->min_max[1].data[1] = fmax(a->min_max[1].data[1], b->min_max[1].data[1]);
  dst->min_max[1].data[2] = fmax(a->min_max[1].data[2], b->min_max[1].data[2]);
}

static
void
merge_aabb_inplace(bvh_aabb_t* dst, bvh_aabb_t* b)
{
  bvh_aabb_t a = *dst;
  dst->min_max[0].data[0] = fmin(a.min_max[0].data[0], b->min_max[0].data[0]);
  dst->min_max[0].data[1] = fmin(a.min_max[0].data[1], b->min_max[0].data[1]);
  dst->min_max[0].data[2] = fmin(a.min_max[0].data[2], b->min_max[0].data[2]);

  dst->min_max[1].data[0] = fmax(a.min_max[1].data[0], b->min_max[1].data[0]);
  dst->min_max[1].data[1] = fmax(a.min_max[1].data[1], b->min_max[1].data[1]);
  dst->min_max[1].data[2] = fmax(a.min_max[1].data[2], b->min_max[1].data[2]);
}

static
void
get_aabb(point3f points[3], bvh_aabb_t* aabb)
{
  point3f* min = aabb->min_max + 0;
  point3f* max = aabb->min_max + 1;
  *min = points[0];
  *max = points[0];

  for (uint32_t i = 0; i < 3; ++i) {
    min->data[0] = fmin(min->data[0], points[i].data[0]);
    min->data[1] = fmin(min->data[1], points[i].data[1]);
    min->data[2] = fmin(min->data[2], points[i].data[2]);

    max->data[0] = fmax(max->data[0], points[i].data[0]);
    max->data[1] = fmax(max->data[1], points[i].data[1]);
    max->data[2] = fmax(max->data[2], points[i].data[2]);
  }
}

static
void
get_centroid(bvh_aabb_t* aabb, point3f* centroid)
{
  vector3f diff = diff_v3f(aabb->min_max + 1, aabb->min_max + 0);
  diff = mult_v3f(&diff, 0.5f);
  *centroid = add_v3f(aabb->min_max, &diff);
}

static
void
swap_faces(bvh_t* bvh, uint32_t left, uint32_t right)
{
  assert(bvh);
  assert(left < bvh->count && right < bvh->count);

  {
    bvh_face_t copy = bvh->faces[left];
    bvh->faces[left] = bvh->faces[right];
    bvh->faces[right] = copy;
  }
}

static
void
get_subdivision_plane_naive(
  bvh_t* bvh,
  const bvh_node_t* node, 
  point3f* center, 
  uint32_t* axis)
{
  int32_t left = 0, right = 0;
  int32_t quota = INT32_MAX;
  uint32_t candidate = 0;
  vector3f extent = diff_v3f(node->bounds.min_max, node->bounds.min_max + 1);
  mult_set_v3f(&extent, 0.5f);
  *center = add_v3f(node->bounds.min_max, &extent);

  for (uint32_t i = 0; i < 3; ++i) {
    candidate = i;
    left = right = 0;

    for (uint32_t j = node->first_prim; j < node->last_prim; ++j) {
      if (bvh->faces[j].centroid.data[candidate] < center->data[candidate])
        ++left;
      else
        ++right;
    }

    if (abs(right - left) < quota) {
      quota = abs(right - left);
      *axis = candidate;
    }
  }

  assert(quota != INT32_MAX);
}

static 
void
subdivide_naive(bvh_t* bvh, uint32_t index)
{
  assert(bvh);
  assert(bvh->nodes_used < bvh->nodes_count);
  assert(index < bvh->nodes_count);

  {
    bvh_node_t* node = bvh->nodes + index;
    point3f center;
    uint32_t axis;
    uint32_t i_prims[3] = {0, 0, 0};

    // calculate the node aabb.
    node->bounds = bvh->faces[node->first_prim].aabb;
    for (uint32_t i = node->first_prim; i < node->last_prim; ++i)
      merge_aabb_inplace(&node->bounds, &bvh->faces[i].aabb);

    // stop the recursion if we have reached our goal.
    if ((node->last_prim - node->first_prim) <= get_bvh_primitives_per_leaf())
      return;
      
    get_subdivision_plane_naive(bvh, node, &center, &axis);

    {
      // split the list of primitives.
      uint32_t i = node->first_prim;
      uint32_t j = node->last_prim;

      while (i < j) {
        if (bvh->faces[i].centroid.data[axis] < center.data[axis])
          ++i;
        else
          swap_faces(bvh, i, --j);
      }

      // another reason to stop the recursion would be that no face has been
      // split
      if (i == node->first_prim || j == node->last_prim)
        return;
        
      node->left_index = bvh->nodes_used++;
      node->right_index = bvh->nodes_used++;

      bvh->nodes[node->left_index].first_prim = node->first_prim;
      bvh->nodes[node->left_index].last_prim = i;
      bvh->nodes[node->left_index].left_index = 0;
      bvh->nodes[node->left_index].right_index = 0;
      subdivide_naive(bvh, node->left_index);

      bvh->nodes[node->right_index].first_prim = i;
      bvh->nodes[node->right_index].last_prim = node->last_prim;
      bvh->nodes[node->right_index].left_index = 0;
      bvh->nodes[node->right_index].right_index = 0;
      subdivide_naive(bvh, node->right_index);
    }
  }
}

static
void
construct_bvh_naive(bvh_t* bvh, const allocator_t* allocator)
{
  // NOTE: the bvh is limited to a max of 2 * N - 1 nodes. where N is the number
  // of primitives per scene.
  bvh->nodes_count = 2 * bvh->count - 1;
  bvh->nodes = allocator->mem_cont_alloc(bvh->nodes_count, sizeof(bvh_node_t));
  memset(bvh->nodes, 0, sizeof(bvh_node_t) * bvh->nodes_count);

  // NOTE: last prim is exclusive, not inclusive.
  bvh_node_t* root = bvh->nodes;
  root->left_index = root->right_index = 0;
  root->first_prim = 0;
  root->last_prim = bvh->count;
  
  // subdivide.
  bvh->nodes_used++;
  subdivide_naive(bvh, 0);
}

bvh_t*
create_bvh(
  float** vertices, 
  uint32_t** indices, 
  uint32_t* indices_count, 
  uint32_t meshes_count,
  const allocator_t* allocator,
  bvh_construction_method_t method)
{
  assert(vertices && indices && indices_count && meshes_count && allocator);
  assert(method != BVH_CONSTRUCT_COUNT);

  // ensure the data is valid.
  for (uint32_t i = 0; i < meshes_count; ++i)
    assert(vertices[i] && indices[i] && indices_count[i]);

  {
    bvh_t* bvh = allocator->mem_alloc(sizeof(bvh_t));
    assert(bvh && "Allocation failed!");
    memset(bvh, 0, sizeof(bvh_t));

    // find the total number of faces to be added.
    for (uint32_t i = 0; i < meshes_count; ++i)
      bvh->count += indices_count[i]/3;

    bvh->faces = allocator->mem_cont_alloc(bvh->count, sizeof(bvh_face_t));

    {
      uint32_t iarray[3], count;
      bvh_face_t* face;
      uint32_t face_index = 0;
      for (uint32_t mesh_index = 0; mesh_index < meshes_count; ++mesh_index) {
        count = indices_count[mesh_index];
        for (uint32_t i = 0; i < count; i+=3, ++face_index) {
          iarray[0] = indices[mesh_index][i + 0];
          iarray[1] = indices[mesh_index][i + 1];
          iarray[2] = indices[mesh_index][i + 2];

          face = bvh->faces + face_index;

          // set the face vertices.
          for (uint32_t k = 0; k < 3; ++k) {
            face->points[k].data[0] = vertices[mesh_index][iarray[k] * 3 + 0];
            face->points[k].data[1] = vertices[mesh_index][iarray[k] * 3 + 1];
            face->points[k].data[2] = vertices[mesh_index][iarray[k] * 3 + 2];
          }

          {
            // min-max calculation.
            get_aabb(face->points, &face->aabb);
            get_centroid(&face->aabb, &face->centroid);
          }

          {
            // calculate normal
            vector3f v1, v2;
            vector3f_set_diff_v3f(&v1, face->points, face->points + 1);
            vector3f_set_diff_v3f(&v2, face->points, face->points + 2);
            face->normal = cross_product_v3f(&v1, &v2);
            normalize_set_v3f(&face->normal);
          }

          {
            // normal_dot, is_floor, is_valid calculation.
            float cosine_target = cosf(TO_RADIANS(FLOOR_ANGLE_DEGREES));
            face->normal_dot = face->normal.data[1];
            face->is_floor = face->normal_dot > cosine_target;
            face->is_valid = !(
              equal_to_v3f(face->points + 0, face->points + 1) ||
              equal_to_v3f(face->points + 0, face->points + 2) ||
              equal_to_v3f(face->points + 1, face->points + 2));
          }
        }
      }
    }

    if (method == BVH_CONSTRUCT_NAIVE)
      construct_bvh_naive(bvh, allocator);
    else
      assert(0);

    return bvh;
  }
}

void
free_bvh(bvh_t* bvh, const allocator_t* allocator)
{
  assert(bvh && bvh->faces && allocator);

  allocator->mem_free(bvh->faces);
  allocator->mem_free(bvh->nodes);
  allocator->mem_free(bvh);
}

int32_t
bounds_intersect(const bvh_aabb_t* left, const bvh_aabb_t* right)
{
  int32_t no_intersect_x = 
    left->min_max[0].data[0] > right->min_max[1].data[0] ||
    left->min_max[1].data[0] < right->min_max[0].data[0];
  int32_t no_intersect_y = 
    left->min_max[0].data[1] > right->min_max[1].data[1] ||
    left->min_max[1].data[1] < right->min_max[0].data[1];
  int32_t no_intersect_z = 
    left->min_max[0].data[2] > right->min_max[1].data[2] ||
    left->min_max[1].data[2] < right->min_max[0].data[2];
  return !(no_intersect_x || no_intersect_y || no_intersect_z);
}

static
int32_t
is_leaf(const bvh_node_t* node)
{
  return node->left_index == 0 && node->right_index == 0;
}

static
void
query_intersection_internal(
  bvh_t* bvh, 
  uint32_t node_index,
  bvh_aabb_t* bounds, 
  uint32_t array[256], 
  uint32_t* used)
{
  bvh_node_t* node = bvh->nodes + node_index;
  if (is_leaf(node)) {
    if (bounds_intersect(&node->bounds, bounds))
      array[(*used)++] = node_index;
    return;
  }

  if (!bounds_intersect(&node->bounds, bounds))
    return;
  
  query_intersection_internal(bvh, node->left_index, bounds, array, used);
  query_intersection_internal(bvh, node->right_index, bounds, array, used);
}

void
query_intersection(
  bvh_t* bvh, 
  bvh_aabb_t* bounds, 
  uint32_t array[256], 
  uint32_t* used)
{
  assert(bvh && bounds && array && used);
  *used = 0;

  query_intersection_internal(bvh, 0, bounds, array, used);
}