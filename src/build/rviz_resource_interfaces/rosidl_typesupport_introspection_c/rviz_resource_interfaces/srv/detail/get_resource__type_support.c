// generated from rosidl_typesupport_introspection_c/resource/idl__type_support.c.em
// with input from rviz_resource_interfaces:srv/GetResource.idl
// generated code does not contain a copyright notice

#include <stddef.h>
#include "rviz_resource_interfaces/srv/detail/get_resource__rosidl_typesupport_introspection_c.h"
#include "rviz_resource_interfaces/msg/rosidl_typesupport_introspection_c__visibility_control.h"
#include "rosidl_typesupport_introspection_c/field_types.h"
#include "rosidl_typesupport_introspection_c/identifier.h"
#include "rosidl_typesupport_introspection_c/message_introspection.h"
#include "rviz_resource_interfaces/srv/detail/get_resource__functions.h"
#include "rviz_resource_interfaces/srv/detail/get_resource__struct.h"


// Include directives for member types
// Member `path`
// Member `etag`
#include "rosidl_runtime_c/string_functions.h"

#ifdef __cplusplus
extern "C"
{
#endif

void rviz_resource_interfaces__srv__GetResource_Request__rosidl_typesupport_introspection_c__GetResource_Request_init_function(
  void * message_memory, enum rosidl_runtime_c__message_initialization _init)
{
  // TODO(karsten1987): initializers are not yet implemented for typesupport c
  // see https://github.com/ros2/ros2/issues/397
  (void) _init;
  rviz_resource_interfaces__srv__GetResource_Request__init(message_memory);
}

void rviz_resource_interfaces__srv__GetResource_Request__rosidl_typesupport_introspection_c__GetResource_Request_fini_function(void * message_memory)
{
  rviz_resource_interfaces__srv__GetResource_Request__fini(message_memory);
}

static rosidl_typesupport_introspection_c__MessageMember rviz_resource_interfaces__srv__GetResource_Request__rosidl_typesupport_introspection_c__GetResource_Request_message_member_array[2] = {
  {
    "path",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_STRING,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(rviz_resource_interfaces__srv__GetResource_Request, path),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "etag",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_STRING,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(rviz_resource_interfaces__srv__GetResource_Request, etag),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  }
};

static const rosidl_typesupport_introspection_c__MessageMembers rviz_resource_interfaces__srv__GetResource_Request__rosidl_typesupport_introspection_c__GetResource_Request_message_members = {
  "rviz_resource_interfaces__srv",  // message namespace
  "GetResource_Request",  // message name
  2,  // number of fields
  sizeof(rviz_resource_interfaces__srv__GetResource_Request),
  rviz_resource_interfaces__srv__GetResource_Request__rosidl_typesupport_introspection_c__GetResource_Request_message_member_array,  // message members
  rviz_resource_interfaces__srv__GetResource_Request__rosidl_typesupport_introspection_c__GetResource_Request_init_function,  // function to initialize message memory (memory has to be allocated)
  rviz_resource_interfaces__srv__GetResource_Request__rosidl_typesupport_introspection_c__GetResource_Request_fini_function  // function to terminate message instance (will not free memory)
};

// this is not const since it must be initialized on first access
// since C does not allow non-integral compile-time constants
static rosidl_message_type_support_t rviz_resource_interfaces__srv__GetResource_Request__rosidl_typesupport_introspection_c__GetResource_Request_message_type_support_handle = {
  0,
  &rviz_resource_interfaces__srv__GetResource_Request__rosidl_typesupport_introspection_c__GetResource_Request_message_members,
  get_message_typesupport_handle_function,
};

ROSIDL_TYPESUPPORT_INTROSPECTION_C_EXPORT_rviz_resource_interfaces
const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_c, rviz_resource_interfaces, srv, GetResource_Request)() {
  if (!rviz_resource_interfaces__srv__GetResource_Request__rosidl_typesupport_introspection_c__GetResource_Request_message_type_support_handle.typesupport_identifier) {
    rviz_resource_interfaces__srv__GetResource_Request__rosidl_typesupport_introspection_c__GetResource_Request_message_type_support_handle.typesupport_identifier =
      rosidl_typesupport_introspection_c__identifier;
  }
  return &rviz_resource_interfaces__srv__GetResource_Request__rosidl_typesupport_introspection_c__GetResource_Request_message_type_support_handle;
}
#ifdef __cplusplus
}
#endif

// already included above
// #include <stddef.h>
// already included above
// #include "rviz_resource_interfaces/srv/detail/get_resource__rosidl_typesupport_introspection_c.h"
// already included above
// #include "rviz_resource_interfaces/msg/rosidl_typesupport_introspection_c__visibility_control.h"
// already included above
// #include "rosidl_typesupport_introspection_c/field_types.h"
// already included above
// #include "rosidl_typesupport_introspection_c/identifier.h"
// already included above
// #include "rosidl_typesupport_introspection_c/message_introspection.h"
// already included above
// #include "rviz_resource_interfaces/srv/detail/get_resource__functions.h"
// already included above
// #include "rviz_resource_interfaces/srv/detail/get_resource__struct.h"


// Include directives for member types
// Member `error_reason`
// Member `expanded_path`
// Member `etag`
// already included above
// #include "rosidl_runtime_c/string_functions.h"
// Member `body`
#include "rosidl_runtime_c/primitives_sequence_functions.h"

#ifdef __cplusplus
extern "C"
{
#endif

void rviz_resource_interfaces__srv__GetResource_Response__rosidl_typesupport_introspection_c__GetResource_Response_init_function(
  void * message_memory, enum rosidl_runtime_c__message_initialization _init)
{
  // TODO(karsten1987): initializers are not yet implemented for typesupport c
  // see https://github.com/ros2/ros2/issues/397
  (void) _init;
  rviz_resource_interfaces__srv__GetResource_Response__init(message_memory);
}

void rviz_resource_interfaces__srv__GetResource_Response__rosidl_typesupport_introspection_c__GetResource_Response_fini_function(void * message_memory)
{
  rviz_resource_interfaces__srv__GetResource_Response__fini(message_memory);
}

size_t rviz_resource_interfaces__srv__GetResource_Response__rosidl_typesupport_introspection_c__size_function__GetResource_Response__body(
  const void * untyped_member)
{
  const rosidl_runtime_c__uint8__Sequence * member =
    (const rosidl_runtime_c__uint8__Sequence *)(untyped_member);
  return member->size;
}

const void * rviz_resource_interfaces__srv__GetResource_Response__rosidl_typesupport_introspection_c__get_const_function__GetResource_Response__body(
  const void * untyped_member, size_t index)
{
  const rosidl_runtime_c__uint8__Sequence * member =
    (const rosidl_runtime_c__uint8__Sequence *)(untyped_member);
  return &member->data[index];
}

void * rviz_resource_interfaces__srv__GetResource_Response__rosidl_typesupport_introspection_c__get_function__GetResource_Response__body(
  void * untyped_member, size_t index)
{
  rosidl_runtime_c__uint8__Sequence * member =
    (rosidl_runtime_c__uint8__Sequence *)(untyped_member);
  return &member->data[index];
}

void rviz_resource_interfaces__srv__GetResource_Response__rosidl_typesupport_introspection_c__fetch_function__GetResource_Response__body(
  const void * untyped_member, size_t index, void * untyped_value)
{
  const uint8_t * item =
    ((const uint8_t *)
    rviz_resource_interfaces__srv__GetResource_Response__rosidl_typesupport_introspection_c__get_const_function__GetResource_Response__body(untyped_member, index));
  uint8_t * value =
    (uint8_t *)(untyped_value);
  *value = *item;
}

void rviz_resource_interfaces__srv__GetResource_Response__rosidl_typesupport_introspection_c__assign_function__GetResource_Response__body(
  void * untyped_member, size_t index, const void * untyped_value)
{
  uint8_t * item =
    ((uint8_t *)
    rviz_resource_interfaces__srv__GetResource_Response__rosidl_typesupport_introspection_c__get_function__GetResource_Response__body(untyped_member, index));
  const uint8_t * value =
    (const uint8_t *)(untyped_value);
  *item = *value;
}

bool rviz_resource_interfaces__srv__GetResource_Response__rosidl_typesupport_introspection_c__resize_function__GetResource_Response__body(
  void * untyped_member, size_t size)
{
  rosidl_runtime_c__uint8__Sequence * member =
    (rosidl_runtime_c__uint8__Sequence *)(untyped_member);
  rosidl_runtime_c__uint8__Sequence__fini(member);
  return rosidl_runtime_c__uint8__Sequence__init(member, size);
}

static rosidl_typesupport_introspection_c__MessageMember rviz_resource_interfaces__srv__GetResource_Response__rosidl_typesupport_introspection_c__GetResource_Response_message_member_array[5] = {
  {
    "status_code",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_INT32,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(rviz_resource_interfaces__srv__GetResource_Response, status_code),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "error_reason",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_STRING,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(rviz_resource_interfaces__srv__GetResource_Response, error_reason),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "expanded_path",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_STRING,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(rviz_resource_interfaces__srv__GetResource_Response, expanded_path),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "etag",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_STRING,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(rviz_resource_interfaces__srv__GetResource_Response, etag),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "body",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_UINT8,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    true,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(rviz_resource_interfaces__srv__GetResource_Response, body),  // bytes offset in struct
    NULL,  // default value
    rviz_resource_interfaces__srv__GetResource_Response__rosidl_typesupport_introspection_c__size_function__GetResource_Response__body,  // size() function pointer
    rviz_resource_interfaces__srv__GetResource_Response__rosidl_typesupport_introspection_c__get_const_function__GetResource_Response__body,  // get_const(index) function pointer
    rviz_resource_interfaces__srv__GetResource_Response__rosidl_typesupport_introspection_c__get_function__GetResource_Response__body,  // get(index) function pointer
    rviz_resource_interfaces__srv__GetResource_Response__rosidl_typesupport_introspection_c__fetch_function__GetResource_Response__body,  // fetch(index, &value) function pointer
    rviz_resource_interfaces__srv__GetResource_Response__rosidl_typesupport_introspection_c__assign_function__GetResource_Response__body,  // assign(index, value) function pointer
    rviz_resource_interfaces__srv__GetResource_Response__rosidl_typesupport_introspection_c__resize_function__GetResource_Response__body  // resize(index) function pointer
  }
};

static const rosidl_typesupport_introspection_c__MessageMembers rviz_resource_interfaces__srv__GetResource_Response__rosidl_typesupport_introspection_c__GetResource_Response_message_members = {
  "rviz_resource_interfaces__srv",  // message namespace
  "GetResource_Response",  // message name
  5,  // number of fields
  sizeof(rviz_resource_interfaces__srv__GetResource_Response),
  rviz_resource_interfaces__srv__GetResource_Response__rosidl_typesupport_introspection_c__GetResource_Response_message_member_array,  // message members
  rviz_resource_interfaces__srv__GetResource_Response__rosidl_typesupport_introspection_c__GetResource_Response_init_function,  // function to initialize message memory (memory has to be allocated)
  rviz_resource_interfaces__srv__GetResource_Response__rosidl_typesupport_introspection_c__GetResource_Response_fini_function  // function to terminate message instance (will not free memory)
};

// this is not const since it must be initialized on first access
// since C does not allow non-integral compile-time constants
static rosidl_message_type_support_t rviz_resource_interfaces__srv__GetResource_Response__rosidl_typesupport_introspection_c__GetResource_Response_message_type_support_handle = {
  0,
  &rviz_resource_interfaces__srv__GetResource_Response__rosidl_typesupport_introspection_c__GetResource_Response_message_members,
  get_message_typesupport_handle_function,
};

ROSIDL_TYPESUPPORT_INTROSPECTION_C_EXPORT_rviz_resource_interfaces
const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_c, rviz_resource_interfaces, srv, GetResource_Response)() {
  if (!rviz_resource_interfaces__srv__GetResource_Response__rosidl_typesupport_introspection_c__GetResource_Response_message_type_support_handle.typesupport_identifier) {
    rviz_resource_interfaces__srv__GetResource_Response__rosidl_typesupport_introspection_c__GetResource_Response_message_type_support_handle.typesupport_identifier =
      rosidl_typesupport_introspection_c__identifier;
  }
  return &rviz_resource_interfaces__srv__GetResource_Response__rosidl_typesupport_introspection_c__GetResource_Response_message_type_support_handle;
}
#ifdef __cplusplus
}
#endif

#include "rosidl_runtime_c/service_type_support_struct.h"
// already included above
// #include "rviz_resource_interfaces/msg/rosidl_typesupport_introspection_c__visibility_control.h"
// already included above
// #include "rviz_resource_interfaces/srv/detail/get_resource__rosidl_typesupport_introspection_c.h"
// already included above
// #include "rosidl_typesupport_introspection_c/identifier.h"
#include "rosidl_typesupport_introspection_c/service_introspection.h"

// this is intentionally not const to allow initialization later to prevent an initialization race
static rosidl_typesupport_introspection_c__ServiceMembers rviz_resource_interfaces__srv__detail__get_resource__rosidl_typesupport_introspection_c__GetResource_service_members = {
  "rviz_resource_interfaces__srv",  // service namespace
  "GetResource",  // service name
  // these two fields are initialized below on the first access
  NULL,  // request message
  // rviz_resource_interfaces__srv__detail__get_resource__rosidl_typesupport_introspection_c__GetResource_Request_message_type_support_handle,
  NULL  // response message
  // rviz_resource_interfaces__srv__detail__get_resource__rosidl_typesupport_introspection_c__GetResource_Response_message_type_support_handle
};

static rosidl_service_type_support_t rviz_resource_interfaces__srv__detail__get_resource__rosidl_typesupport_introspection_c__GetResource_service_type_support_handle = {
  0,
  &rviz_resource_interfaces__srv__detail__get_resource__rosidl_typesupport_introspection_c__GetResource_service_members,
  get_service_typesupport_handle_function,
};

// Forward declaration of request/response type support functions
const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_c, rviz_resource_interfaces, srv, GetResource_Request)();

const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_c, rviz_resource_interfaces, srv, GetResource_Response)();

ROSIDL_TYPESUPPORT_INTROSPECTION_C_EXPORT_rviz_resource_interfaces
const rosidl_service_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__SERVICE_SYMBOL_NAME(rosidl_typesupport_introspection_c, rviz_resource_interfaces, srv, GetResource)() {
  if (!rviz_resource_interfaces__srv__detail__get_resource__rosidl_typesupport_introspection_c__GetResource_service_type_support_handle.typesupport_identifier) {
    rviz_resource_interfaces__srv__detail__get_resource__rosidl_typesupport_introspection_c__GetResource_service_type_support_handle.typesupport_identifier =
      rosidl_typesupport_introspection_c__identifier;
  }
  rosidl_typesupport_introspection_c__ServiceMembers * service_members =
    (rosidl_typesupport_introspection_c__ServiceMembers *)rviz_resource_interfaces__srv__detail__get_resource__rosidl_typesupport_introspection_c__GetResource_service_type_support_handle.data;

  if (!service_members->request_members_) {
    service_members->request_members_ =
      (const rosidl_typesupport_introspection_c__MessageMembers *)
      ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_c, rviz_resource_interfaces, srv, GetResource_Request)()->data;
  }
  if (!service_members->response_members_) {
    service_members->response_members_ =
      (const rosidl_typesupport_introspection_c__MessageMembers *)
      ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_c, rviz_resource_interfaces, srv, GetResource_Response)()->data;
  }

  return &rviz_resource_interfaces__srv__detail__get_resource__rosidl_typesupport_introspection_c__GetResource_service_type_support_handle;
}
