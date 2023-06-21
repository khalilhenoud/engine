/**
 * @file sphere.c
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-06-17
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <math.h>
#include <assert.h>
#include <collision/sphere.h>


spheres_classification_t
classify_spheres(
  const sphere_t *source, 
  const sphere_t *target,
  vector3f* penetration)
{
  vector3f from_to;
  vector3f_set_diff_v3f(&from_to, &target->center, &source->center);

  assert(penetration != NULL);

  {
    float distance = length_v3f(&from_to);
    if (nextafterf(distance, 0.f) == 0.f) {
      if (nextafterf(source->radius, target->radius) == target->radius)
        return SPHERES_IDENTICAL;
      else
        return SPHERES_SHARE_CENTER;
    } else {
      float combined_radius = source->radius + target->radius;
      if (distance < combined_radius) {
        float scale = combined_radius - distance;
        normalize_set_v3f(&from_to);
        mult_set_v3f(&from_to, scale);
        *penetration = from_to;
        return SPHERES_COLLIDE;
      } else
        return SPHERES_DISTINCT;
    }
  }
}