# Controller feedback to the planner, published at the control rate on
# /controller/status. Errors are target - current; the planner decides what
# counts as "close enough" (tolerances live in the mission YAML).
int32 seq          # seq of the last Task applied (-1 until the first task)
float64 err_xy     # metres, horizontal |target - current| in world frame
float64 err_z      # metres, signed target_z - current_z (world z, +down)
float64 err_yaw    # radians, wrapped signed target_yaw - current_yaw
