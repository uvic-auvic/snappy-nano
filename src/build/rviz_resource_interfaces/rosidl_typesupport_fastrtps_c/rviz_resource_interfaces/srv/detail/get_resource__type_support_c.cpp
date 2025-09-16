// generated from rosidl_typesupport_fastrtps_c/resource/idl__type_support_c.cpp.em
// with input from rviz_resource_interfaces:srv/GetResource.idl
// generated code does not contain a copyright notice
#include "rviz_resource_interfaces/srv/detail/get_resource__rosidl_typesupport_fastrtps_c.h"


#include <cassert>
#include <limits>
#include <string>
#include "rosidl_typesupport_fastrtps_c/identifier.h"
#include "rosidl_typesupport_fastrtps_c/wstring_conversion.hpp"
#include "rosidl_typesupport_fastrtps_cpp/message_type_support.h"
#include "rviz_resource_interfaces/msg/rosidl_typesupport_fastrtps_c__visibility_control.h"
#include "rviz_resource_interfaces/srv/detail/get_resource__struct.h"
#include "rviz_resource_interfaces/srv/detail/get_resource__functions.h"
#include "fastcdr/Cdr.h"

#ifndef _WIN32
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wunused-parameter"
# ifdef __clang__
#  pragma clang diagnostic ignored "-Wdeprecated-register"
#  pragma clang diagnostic ignored "-Wreturn-type-c-linkage"
# endif
#endif
#ifndef _WIN32
# pragma GCC diagnostic pop
#endif

// includes and forward declarations of message dependencies and their conversion functions

#if defined(__cplusplus)
extern "C"
{
#endif

#include "rosidl_runtime_c/string.h"  // etag, path
#include "rosidl_runtime_c/string_functions.h"  // etag, path

// forward declare type support functions


using _GetResource_Request__ros_msg_type = rviz_resource_interfaces__srv__GetResource_Request;

static bool _GetResource_Request__cdr_serialize(
  const void * untyped_ros_message,
  eprosima::fastcdr::Cdr & cdr)
{
  if (!untyped_ros_message) {
    fprintf(stderr, "ros message handle is null\n");
    return false;
  }
  const _GetResource_Request__ros_msg_type * ros_message = static_cast<const _GetResource_Request__ros_msg_type *>(untyped_ros_message);
  // Field name: path
  {
    const rosidl_runtime_c__String * str = &ros_message->path;
    if (str->capacity == 0 || str->capacity <= str->size) {
      fprintf(stderr, "string capacity not greater than size\n");
      return false;
    }
    if (str->data[str->size] != '\0') {
      fprintf(stderr, "string not null-terminated\n");
      return false;
    }
    cdr << str->data;
  }

  // Field name: etag
  {
    const rosidl_runtime_c__String * str = &ros_message->etag;
    if (str->capacity == 0 || str->capacity <= str->size) {
      fprintf(stderr, "string capacity not greater than size\n");
      return false;
    }
    if (str->data[str->size] != '\0') {
      fprintf(stderr, "string not null-terminated\n");
      return false;
    }
    cdr << str->data;
  }

  return true;
}

static bool _GetResource_Request__cdr_deserialize(
  eprosima::fastcdr::Cdr & cdr,
  void * untyped_ros_message)
{
  if (!untyped_ros_message) {
    fprintf(stderr, "ros message handle is null\n");
    return false;
  }
  _GetResource_Request__ros_msg_type * ros_message = static_cast<_GetResource_Request__ros_msg_type *>(untyped_ros_message);
  // Field name: path
  {
    std::string tmp;
    cdr >> tmp;
    if (!ros_message->path.data) {
      rosidl_runtime_c__String__init(&ros_message->path);
    }
    bool succeeded = rosidl_runtime_c__String__assign(
      &ros_message->path,
      tmp.c_str());
    if (!succeeded) {
      fprintf(stderr, "failed to assign string into field 'path'\n");
      return false;
    }
  }

  // Field name: etag
  {
    std::string tmp;
    cdr >> tmp;
    if (!ros_message->etag.data) {
      rosidl_runtime_c__String__init(&ros_message->etag);
    }
    bool succeeded = rosidl_runtime_c__String__assign(
      &ros_message->etag,
      tmp.c_str());
    if (!succeeded) {
      fprintf(stderr, "failed to assign string into field 'etag'\n");
      return false;
    }
  }

  return true;
}  // NOLINT(readability/fn_size)

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_rviz_resource_interfaces
size_t get_serialized_size_rviz_resource_interfaces__srv__GetResource_Request(
  const void * untyped_ros_message,
  size_t current_alignment)
{
  const _GetResource_Request__ros_msg_type * ros_message = static_cast<const _GetResource_Request__ros_msg_type *>(untyped_ros_message);
  (void)ros_message;
  size_t initial_alignment = current_alignment;

  const size_t padding = 4;
  const size_t wchar_size = 4;
  (void)padding;
  (void)wchar_size;

  // field.name path
  current_alignment += padding +
    eprosima::fastcdr::Cdr::alignment(current_alignment, padding) +
    (ros_message->path.size + 1);
  // field.name etag
  current_alignment += padding +
    eprosima::fastcdr::Cdr::alignment(current_alignment, padding) +
    (ros_message->etag.size + 1);

  return current_alignment - initial_alignment;
}

static uint32_t _GetResource_Request__get_serialized_size(const void * untyped_ros_message)
{
  return static_cast<uint32_t>(
    get_serialized_size_rviz_resource_interfaces__srv__GetResource_Request(
      untyped_ros_message, 0));
}

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_rviz_resource_interfaces
size_t max_serialized_size_rviz_resource_interfaces__srv__GetResource_Request(
  bool & full_bounded,
  bool & is_plain,
  size_t current_alignment)
{
  size_t initial_alignment = current_alignment;

  const size_t padding = 4;
  const size_t wchar_size = 4;
  size_t last_member_size = 0;
  (void)last_member_size;
  (void)padding;
  (void)wchar_size;

  full_bounded = true;
  is_plain = true;

  // member: path
  {
    size_t array_size = 1;

    full_bounded = false;
    is_plain = false;
    for (size_t index = 0; index < array_size; ++index) {
      current_alignment += padding +
        eprosima::fastcdr::Cdr::alignment(current_alignment, padding) +
        1;
    }
  }
  // member: etag
  {
    size_t array_size = 1;

    full_bounded = false;
    is_plain = false;
    for (size_t index = 0; index < array_size; ++index) {
      current_alignment += padding +
        eprosima::fastcdr::Cdr::alignment(current_alignment, padding) +
        1;
    }
  }

  size_t ret_val = current_alignment - initial_alignment;
  if (is_plain) {
    // All members are plain, and type is not empty.
    // We still need to check that the in-memory alignment
    // is the same as the CDR mandated alignment.
    using DataType = rviz_resource_interfaces__srv__GetResource_Request;
    is_plain =
      (
      offsetof(DataType, etag) +
      last_member_size
      ) == ret_val;
  }

  return ret_val;
}

static size_t _GetResource_Request__max_serialized_size(char & bounds_info)
{
  bool full_bounded;
  bool is_plain;
  size_t ret_val;

  ret_val = max_serialized_size_rviz_resource_interfaces__srv__GetResource_Request(
    full_bounded, is_plain, 0);

  bounds_info =
    is_plain ? ROSIDL_TYPESUPPORT_FASTRTPS_PLAIN_TYPE :
    full_bounded ? ROSIDL_TYPESUPPORT_FASTRTPS_BOUNDED_TYPE : ROSIDL_TYPESUPPORT_FASTRTPS_UNBOUNDED_TYPE;
  return ret_val;
}


static message_type_support_callbacks_t __callbacks_GetResource_Request = {
  "rviz_resource_interfaces::srv",
  "GetResource_Request",
  _GetResource_Request__cdr_serialize,
  _GetResource_Request__cdr_deserialize,
  _GetResource_Request__get_serialized_size,
  _GetResource_Request__max_serialized_size
};

static rosidl_message_type_support_t _GetResource_Request__type_support = {
  rosidl_typesupport_fastrtps_c__identifier,
  &__callbacks_GetResource_Request,
  get_message_typesupport_handle_function,
};

const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_fastrtps_c, rviz_resource_interfaces, srv, GetResource_Request)() {
  return &_GetResource_Request__type_support;
}

#if defined(__cplusplus)
}
#endif

// already included above
// #include <cassert>
// already included above
// #include <limits>
// already included above
// #include <string>
// already included above
// #include "rosidl_typesupport_fastrtps_c/identifier.h"
// already included above
// #include "rosidl_typesupport_fastrtps_c/wstring_conversion.hpp"
// already included above
// #include "rosidl_typesupport_fastrtps_cpp/message_type_support.h"
// already included above
// #include "rviz_resource_interfaces/msg/rosidl_typesupport_fastrtps_c__visibility_control.h"
// already included above
// #include "rviz_resource_interfaces/srv/detail/get_resource__struct.h"
// already included above
// #include "rviz_resource_interfaces/srv/detail/get_resource__functions.h"
// already included above
// #include "fastcdr/Cdr.h"

#ifndef _WIN32
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wunused-parameter"
# ifdef __clang__
#  pragma clang diagnostic ignored "-Wdeprecated-register"
#  pragma clang diagnostic ignored "-Wreturn-type-c-linkage"
# endif
#endif
#ifndef _WIN32
# pragma GCC diagnostic pop
#endif

// includes and forward declarations of message dependencies and their conversion functions

#if defined(__cplusplus)
extern "C"
{
#endif

#include "rosidl_runtime_c/primitives_sequence.h"  // body
#include "rosidl_runtime_c/primitives_sequence_functions.h"  // body
// already included above
// #include "rosidl_runtime_c/string.h"  // error_reason, etag, expanded_path
// already included above
// #include "rosidl_runtime_c/string_functions.h"  // error_reason, etag, expanded_path

// forward declare type support functions


using _GetResource_Response__ros_msg_type = rviz_resource_interfaces__srv__GetResource_Response;

static bool _GetResource_Response__cdr_serialize(
  const void * untyped_ros_message,
  eprosima::fastcdr::Cdr & cdr)
{
  if (!untyped_ros_message) {
    fprintf(stderr, "ros message handle is null\n");
    return false;
  }
  const _GetResource_Response__ros_msg_type * ros_message = static_cast<const _GetResource_Response__ros_msg_type *>(untyped_ros_message);
  // Field name: status_code
  {
    cdr << ros_message->status_code;
  }

  // Field name: error_reason
  {
    const rosidl_runtime_c__String * str = &ros_message->error_reason;
    if (str->capacity == 0 || str->capacity <= str->size) {
      fprintf(stderr, "string capacity not greater than size\n");
      return false;
    }
    if (str->data[str->size] != '\0') {
      fprintf(stderr, "string not null-terminated\n");
      return false;
    }
    cdr << str->data;
  }

  // Field name: expanded_path
  {
    const rosidl_runtime_c__String * str = &ros_message->expanded_path;
    if (str->capacity == 0 || str->capacity <= str->size) {
      fprintf(stderr, "string capacity not greater than size\n");
      return false;
    }
    if (str->data[str->size] != '\0') {
      fprintf(stderr, "string not null-terminated\n");
      return false;
    }
    cdr << str->data;
  }

  // Field name: etag
  {
    const rosidl_runtime_c__String * str = &ros_message->etag;
    if (str->capacity == 0 || str->capacity <= str->size) {
      fprintf(stderr, "string capacity not greater than size\n");
      return false;
    }
    if (str->data[str->size] != '\0') {
      fprintf(stderr, "string not null-terminated\n");
      return false;
    }
    cdr << str->data;
  }

  // Field name: body
  {
    size_t size = ros_message->body.size;
    auto array_ptr = ros_message->body.data;
    cdr << static_cast<uint32_t>(size);
    cdr.serializeArray(array_ptr, size);
  }

  return true;
}

static bool _GetResource_Response__cdr_deserialize(
  eprosima::fastcdr::Cdr & cdr,
  void * untyped_ros_message)
{
  if (!untyped_ros_message) {
    fprintf(stderr, "ros message handle is null\n");
    return false;
  }
  _GetResource_Response__ros_msg_type * ros_message = static_cast<_GetResource_Response__ros_msg_type *>(untyped_ros_message);
  // Field name: status_code
  {
    cdr >> ros_message->status_code;
  }

  // Field name: error_reason
  {
    std::string tmp;
    cdr >> tmp;
    if (!ros_message->error_reason.data) {
      rosidl_runtime_c__String__init(&ros_message->error_reason);
    }
    bool succeeded = rosidl_runtime_c__String__assign(
      &ros_message->error_reason,
      tmp.c_str());
    if (!succeeded) {
      fprintf(stderr, "failed to assign string into field 'error_reason'\n");
      return false;
    }
  }

  // Field name: expanded_path
  {
    std::string tmp;
    cdr >> tmp;
    if (!ros_message->expanded_path.data) {
      rosidl_runtime_c__String__init(&ros_message->expanded_path);
    }
    bool succeeded = rosidl_runtime_c__String__assign(
      &ros_message->expanded_path,
      tmp.c_str());
    if (!succeeded) {
      fprintf(stderr, "failed to assign string into field 'expanded_path'\n");
      return false;
    }
  }

  // Field name: etag
  {
    std::string tmp;
    cdr >> tmp;
    if (!ros_message->etag.data) {
      rosidl_runtime_c__String__init(&ros_message->etag);
    }
    bool succeeded = rosidl_runtime_c__String__assign(
      &ros_message->etag,
      tmp.c_str());
    if (!succeeded) {
      fprintf(stderr, "failed to assign string into field 'etag'\n");
      return false;
    }
  }

  // Field name: body
  {
    uint32_t cdrSize;
    cdr >> cdrSize;
    size_t size = static_cast<size_t>(cdrSize);
    if (ros_message->body.data) {
      rosidl_runtime_c__uint8__Sequence__fini(&ros_message->body);
    }
    if (!rosidl_runtime_c__uint8__Sequence__init(&ros_message->body, size)) {
      fprintf(stderr, "failed to create array for field 'body'");
      return false;
    }
    auto array_ptr = ros_message->body.data;
    cdr.deserializeArray(array_ptr, size);
  }

  return true;
}  // NOLINT(readability/fn_size)

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_rviz_resource_interfaces
size_t get_serialized_size_rviz_resource_interfaces__srv__GetResource_Response(
  const void * untyped_ros_message,
  size_t current_alignment)
{
  const _GetResource_Response__ros_msg_type * ros_message = static_cast<const _GetResource_Response__ros_msg_type *>(untyped_ros_message);
  (void)ros_message;
  size_t initial_alignment = current_alignment;

  const size_t padding = 4;
  const size_t wchar_size = 4;
  (void)padding;
  (void)wchar_size;

  // field.name status_code
  {
    size_t item_size = sizeof(ros_message->status_code);
    current_alignment += item_size +
      eprosima::fastcdr::Cdr::alignment(current_alignment, item_size);
  }
  // field.name error_reason
  current_alignment += padding +
    eprosima::fastcdr::Cdr::alignment(current_alignment, padding) +
    (ros_message->error_reason.size + 1);
  // field.name expanded_path
  current_alignment += padding +
    eprosima::fastcdr::Cdr::alignment(current_alignment, padding) +
    (ros_message->expanded_path.size + 1);
  // field.name etag
  current_alignment += padding +
    eprosima::fastcdr::Cdr::alignment(current_alignment, padding) +
    (ros_message->etag.size + 1);
  // field.name body
  {
    size_t array_size = ros_message->body.size;
    auto array_ptr = ros_message->body.data;
    current_alignment += padding +
      eprosima::fastcdr::Cdr::alignment(current_alignment, padding);
    (void)array_ptr;
    size_t item_size = sizeof(array_ptr[0]);
    current_alignment += array_size * item_size +
      eprosima::fastcdr::Cdr::alignment(current_alignment, item_size);
  }

  return current_alignment - initial_alignment;
}

static uint32_t _GetResource_Response__get_serialized_size(const void * untyped_ros_message)
{
  return static_cast<uint32_t>(
    get_serialized_size_rviz_resource_interfaces__srv__GetResource_Response(
      untyped_ros_message, 0));
}

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_rviz_resource_interfaces
size_t max_serialized_size_rviz_resource_interfaces__srv__GetResource_Response(
  bool & full_bounded,
  bool & is_plain,
  size_t current_alignment)
{
  size_t initial_alignment = current_alignment;

  const size_t padding = 4;
  const size_t wchar_size = 4;
  size_t last_member_size = 0;
  (void)last_member_size;
  (void)padding;
  (void)wchar_size;

  full_bounded = true;
  is_plain = true;

  // member: status_code
  {
    size_t array_size = 1;

    last_member_size = array_size * sizeof(uint32_t);
    current_alignment += array_size * sizeof(uint32_t) +
      eprosima::fastcdr::Cdr::alignment(current_alignment, sizeof(uint32_t));
  }
  // member: error_reason
  {
    size_t array_size = 1;

    full_bounded = false;
    is_plain = false;
    for (size_t index = 0; index < array_size; ++index) {
      current_alignment += padding +
        eprosima::fastcdr::Cdr::alignment(current_alignment, padding) +
        1;
    }
  }
  // member: expanded_path
  {
    size_t array_size = 1;

    full_bounded = false;
    is_plain = false;
    for (size_t index = 0; index < array_size; ++index) {
      current_alignment += padding +
        eprosima::fastcdr::Cdr::alignment(current_alignment, padding) +
        1;
    }
  }
  // member: etag
  {
    size_t array_size = 1;

    full_bounded = false;
    is_plain = false;
    for (size_t index = 0; index < array_size; ++index) {
      current_alignment += padding +
        eprosima::fastcdr::Cdr::alignment(current_alignment, padding) +
        1;
    }
  }
  // member: body
  {
    size_t array_size = 0;
    full_bounded = false;
    is_plain = false;
    current_alignment += padding +
      eprosima::fastcdr::Cdr::alignment(current_alignment, padding);

    last_member_size = array_size * sizeof(uint8_t);
    current_alignment += array_size * sizeof(uint8_t);
  }

  size_t ret_val = current_alignment - initial_alignment;
  if (is_plain) {
    // All members are plain, and type is not empty.
    // We still need to check that the in-memory alignment
    // is the same as the CDR mandated alignment.
    using DataType = rviz_resource_interfaces__srv__GetResource_Response;
    is_plain =
      (
      offsetof(DataType, body) +
      last_member_size
      ) == ret_val;
  }

  return ret_val;
}

static size_t _GetResource_Response__max_serialized_size(char & bounds_info)
{
  bool full_bounded;
  bool is_plain;
  size_t ret_val;

  ret_val = max_serialized_size_rviz_resource_interfaces__srv__GetResource_Response(
    full_bounded, is_plain, 0);

  bounds_info =
    is_plain ? ROSIDL_TYPESUPPORT_FASTRTPS_PLAIN_TYPE :
    full_bounded ? ROSIDL_TYPESUPPORT_FASTRTPS_BOUNDED_TYPE : ROSIDL_TYPESUPPORT_FASTRTPS_UNBOUNDED_TYPE;
  return ret_val;
}


static message_type_support_callbacks_t __callbacks_GetResource_Response = {
  "rviz_resource_interfaces::srv",
  "GetResource_Response",
  _GetResource_Response__cdr_serialize,
  _GetResource_Response__cdr_deserialize,
  _GetResource_Response__get_serialized_size,
  _GetResource_Response__max_serialized_size
};

static rosidl_message_type_support_t _GetResource_Response__type_support = {
  rosidl_typesupport_fastrtps_c__identifier,
  &__callbacks_GetResource_Response,
  get_message_typesupport_handle_function,
};

const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_fastrtps_c, rviz_resource_interfaces, srv, GetResource_Response)() {
  return &_GetResource_Response__type_support;
}

#if defined(__cplusplus)
}
#endif

#include "rosidl_typesupport_fastrtps_cpp/service_type_support.h"
#include "rosidl_typesupport_cpp/service_type_support.hpp"
// already included above
// #include "rosidl_typesupport_fastrtps_c/identifier.h"
// already included above
// #include "rviz_resource_interfaces/msg/rosidl_typesupport_fastrtps_c__visibility_control.h"
#include "rviz_resource_interfaces/srv/get_resource.h"

#if defined(__cplusplus)
extern "C"
{
#endif

static service_type_support_callbacks_t GetResource__callbacks = {
  "rviz_resource_interfaces::srv",
  "GetResource",
  ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_fastrtps_c, rviz_resource_interfaces, srv, GetResource_Request)(),
  ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_fastrtps_c, rviz_resource_interfaces, srv, GetResource_Response)(),
};

static rosidl_service_type_support_t GetResource__handle = {
  rosidl_typesupport_fastrtps_c__identifier,
  &GetResource__callbacks,
  get_service_typesupport_handle_function,
};

const rosidl_service_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__SERVICE_SYMBOL_NAME(rosidl_typesupport_fastrtps_c, rviz_resource_interfaces, srv, GetResource)() {
  return &GetResource__handle;
}

#if defined(__cplusplus)
}
#endif
