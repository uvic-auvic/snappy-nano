# A single object detection result
string object_class        # Name of detected object (from COCO classes)
float32 confidence         # Confidence score 0.0-1.0
float32 distance_m         # Distance from camera in meters (-1 if unavailable)
BoundingBox2D bounding_box # Bounding box in pixel coordinates
builtin_interfaces/Time timestamp  # Detection timestamp
