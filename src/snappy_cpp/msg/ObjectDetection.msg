string object_class
float32 confidence
float32 distance_m
uint8[] quadrants              # 5x5 grid cells, numbered 1-25 row-major from top-left
BoundingBox2D bounding_box
Polygon2D[] mask_polygons       # usually length 1, but supports multi-contour masks
builtin_interfaces/Time timestamp
