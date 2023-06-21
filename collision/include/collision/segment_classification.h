/**
 * @file segment_classification.h
 * @author khalilhenoud@gmail.com
 * @brief to avoid including segment.h when only the classification is required.
 * @version 0.1
 * @date 2023-06-20
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef SEGMENT_CLASSIFICATION
#define SEGMENT_CLASSIFICATION

#ifdef __cplusplus
extern "C" {
#endif


typedef
enum segments_classification_t {
  SEGMENTS_IDENTICAL,
  SEGMENTS_PARALLEL,
  SEGMENTS_COLINEAR_NO_OVERLAP,
  SEGMENTS_COLINEAR_FULL_OVERLAP,
  SEGMENTS_COLINEAR_OVERLAPPING,
  SEGMENTS_COLINEAR_OVERLAPPING_AT_POINT,
  SEGMENTS_COPLANAR_INTERSECT,
  SEGMENTS_COPLANAR_NO_INTERSECT,
  SEGMENTS_DISTINCT,
  SEGMENTS_COUNT
} segments_classification_t;

#ifdef __cplusplus
}
#endif

#endif