// generated from rosidl_generator_c/resource/idl__functions.h.em
// with input from rviz_resource_interfaces:srv/GetResource.idl
// generated code does not contain a copyright notice

#ifndef RVIZ_RESOURCE_INTERFACES__SRV__DETAIL__GET_RESOURCE__FUNCTIONS_H_
#define RVIZ_RESOURCE_INTERFACES__SRV__DETAIL__GET_RESOURCE__FUNCTIONS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stdlib.h>

#include "rosidl_runtime_c/visibility_control.h"
#include "rviz_resource_interfaces/msg/rosidl_generator_c__visibility_control.h"

#include "rviz_resource_interfaces/srv/detail/get_resource__struct.h"

/// Initialize srv/GetResource message.
/**
 * If the init function is called twice for the same message without
 * calling fini inbetween previously allocated memory will be leaked.
 * \param[in,out] msg The previously allocated message pointer.
 * Fields without a default value will not be initialized by this function.
 * You might want to call memset(msg, 0, sizeof(
 * rviz_resource_interfaces__srv__GetResource_Request
 * )) before or use
 * rviz_resource_interfaces__srv__GetResource_Request__create()
 * to allocate and initialize the message.
 * \return true if initialization was successful, otherwise false
 */
ROSIDL_GENERATOR_C_PUBLIC_rviz_resource_interfaces
bool
rviz_resource_interfaces__srv__GetResource_Request__init(rviz_resource_interfaces__srv__GetResource_Request * msg);

/// Finalize srv/GetResource message.
/**
 * \param[in,out] msg The allocated message pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_rviz_resource_interfaces
void
rviz_resource_interfaces__srv__GetResource_Request__fini(rviz_resource_interfaces__srv__GetResource_Request * msg);

/// Create srv/GetResource message.
/**
 * It allocates the memory for the message, sets the memory to zero, and
 * calls
 * rviz_resource_interfaces__srv__GetResource_Request__init().
 * \return The pointer to the initialized message if successful,
 * otherwise NULL
 */
ROSIDL_GENERATOR_C_PUBLIC_rviz_resource_interfaces
rviz_resource_interfaces__srv__GetResource_Request *
rviz_resource_interfaces__srv__GetResource_Request__create();

/// Destroy srv/GetResource message.
/**
 * It calls
 * rviz_resource_interfaces__srv__GetResource_Request__fini()
 * and frees the memory of the message.
 * \param[in,out] msg The allocated message pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_rviz_resource_interfaces
void
rviz_resource_interfaces__srv__GetResource_Request__destroy(rviz_resource_interfaces__srv__GetResource_Request * msg);

/// Check for srv/GetResource message equality.
/**
 * \param[in] lhs The message on the left hand size of the equality operator.
 * \param[in] rhs The message on the right hand size of the equality operator.
 * \return true if messages are equal, otherwise false.
 */
ROSIDL_GENERATOR_C_PUBLIC_rviz_resource_interfaces
bool
rviz_resource_interfaces__srv__GetResource_Request__are_equal(const rviz_resource_interfaces__srv__GetResource_Request * lhs, const rviz_resource_interfaces__srv__GetResource_Request * rhs);

/// Copy a srv/GetResource message.
/**
 * This functions performs a deep copy, as opposed to the shallow copy that
 * plain assignment yields.
 *
 * \param[in] input The source message pointer.
 * \param[out] output The target message pointer, which must
 *   have been initialized before calling this function.
 * \return true if successful, or false if either pointer is null
 *   or memory allocation fails.
 */
ROSIDL_GENERATOR_C_PUBLIC_rviz_resource_interfaces
bool
rviz_resource_interfaces__srv__GetResource_Request__copy(
  const rviz_resource_interfaces__srv__GetResource_Request * input,
  rviz_resource_interfaces__srv__GetResource_Request * output);

/// Initialize array of srv/GetResource messages.
/**
 * It allocates the memory for the number of elements and calls
 * rviz_resource_interfaces__srv__GetResource_Request__init()
 * for each element of the array.
 * \param[in,out] array The allocated array pointer.
 * \param[in] size The size / capacity of the array.
 * \return true if initialization was successful, otherwise false
 * If the array pointer is valid and the size is zero it is guaranteed
 # to return true.
 */
ROSIDL_GENERATOR_C_PUBLIC_rviz_resource_interfaces
bool
rviz_resource_interfaces__srv__GetResource_Request__Sequence__init(rviz_resource_interfaces__srv__GetResource_Request__Sequence * array, size_t size);

/// Finalize array of srv/GetResource messages.
/**
 * It calls
 * rviz_resource_interfaces__srv__GetResource_Request__fini()
 * for each element of the array and frees the memory for the number of
 * elements.
 * \param[in,out] array The initialized array pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_rviz_resource_interfaces
void
rviz_resource_interfaces__srv__GetResource_Request__Sequence__fini(rviz_resource_interfaces__srv__GetResource_Request__Sequence * array);

/// Create array of srv/GetResource messages.
/**
 * It allocates the memory for the array and calls
 * rviz_resource_interfaces__srv__GetResource_Request__Sequence__init().
 * \param[in] size The size / capacity of the array.
 * \return The pointer to the initialized array if successful, otherwise NULL
 */
ROSIDL_GENERATOR_C_PUBLIC_rviz_resource_interfaces
rviz_resource_interfaces__srv__GetResource_Request__Sequence *
rviz_resource_interfaces__srv__GetResource_Request__Sequence__create(size_t size);

/// Destroy array of srv/GetResource messages.
/**
 * It calls
 * rviz_resource_interfaces__srv__GetResource_Request__Sequence__fini()
 * on the array,
 * and frees the memory of the array.
 * \param[in,out] array The initialized array pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_rviz_resource_interfaces
void
rviz_resource_interfaces__srv__GetResource_Request__Sequence__destroy(rviz_resource_interfaces__srv__GetResource_Request__Sequence * array);

/// Check for srv/GetResource message array equality.
/**
 * \param[in] lhs The message array on the left hand size of the equality operator.
 * \param[in] rhs The message array on the right hand size of the equality operator.
 * \return true if message arrays are equal in size and content, otherwise false.
 */
ROSIDL_GENERATOR_C_PUBLIC_rviz_resource_interfaces
bool
rviz_resource_interfaces__srv__GetResource_Request__Sequence__are_equal(const rviz_resource_interfaces__srv__GetResource_Request__Sequence * lhs, const rviz_resource_interfaces__srv__GetResource_Request__Sequence * rhs);

/// Copy an array of srv/GetResource messages.
/**
 * This functions performs a deep copy, as opposed to the shallow copy that
 * plain assignment yields.
 *
 * \param[in] input The source array pointer.
 * \param[out] output The target array pointer, which must
 *   have been initialized before calling this function.
 * \return true if successful, or false if either pointer
 *   is null or memory allocation fails.
 */
ROSIDL_GENERATOR_C_PUBLIC_rviz_resource_interfaces
bool
rviz_resource_interfaces__srv__GetResource_Request__Sequence__copy(
  const rviz_resource_interfaces__srv__GetResource_Request__Sequence * input,
  rviz_resource_interfaces__srv__GetResource_Request__Sequence * output);

/// Initialize srv/GetResource message.
/**
 * If the init function is called twice for the same message without
 * calling fini inbetween previously allocated memory will be leaked.
 * \param[in,out] msg The previously allocated message pointer.
 * Fields without a default value will not be initialized by this function.
 * You might want to call memset(msg, 0, sizeof(
 * rviz_resource_interfaces__srv__GetResource_Response
 * )) before or use
 * rviz_resource_interfaces__srv__GetResource_Response__create()
 * to allocate and initialize the message.
 * \return true if initialization was successful, otherwise false
 */
ROSIDL_GENERATOR_C_PUBLIC_rviz_resource_interfaces
bool
rviz_resource_interfaces__srv__GetResource_Response__init(rviz_resource_interfaces__srv__GetResource_Response * msg);

/// Finalize srv/GetResource message.
/**
 * \param[in,out] msg The allocated message pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_rviz_resource_interfaces
void
rviz_resource_interfaces__srv__GetResource_Response__fini(rviz_resource_interfaces__srv__GetResource_Response * msg);

/// Create srv/GetResource message.
/**
 * It allocates the memory for the message, sets the memory to zero, and
 * calls
 * rviz_resource_interfaces__srv__GetResource_Response__init().
 * \return The pointer to the initialized message if successful,
 * otherwise NULL
 */
ROSIDL_GENERATOR_C_PUBLIC_rviz_resource_interfaces
rviz_resource_interfaces__srv__GetResource_Response *
rviz_resource_interfaces__srv__GetResource_Response__create();

/// Destroy srv/GetResource message.
/**
 * It calls
 * rviz_resource_interfaces__srv__GetResource_Response__fini()
 * and frees the memory of the message.
 * \param[in,out] msg The allocated message pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_rviz_resource_interfaces
void
rviz_resource_interfaces__srv__GetResource_Response__destroy(rviz_resource_interfaces__srv__GetResource_Response * msg);

/// Check for srv/GetResource message equality.
/**
 * \param[in] lhs The message on the left hand size of the equality operator.
 * \param[in] rhs The message on the right hand size of the equality operator.
 * \return true if messages are equal, otherwise false.
 */
ROSIDL_GENERATOR_C_PUBLIC_rviz_resource_interfaces
bool
rviz_resource_interfaces__srv__GetResource_Response__are_equal(const rviz_resource_interfaces__srv__GetResource_Response * lhs, const rviz_resource_interfaces__srv__GetResource_Response * rhs);

/// Copy a srv/GetResource message.
/**
 * This functions performs a deep copy, as opposed to the shallow copy that
 * plain assignment yields.
 *
 * \param[in] input The source message pointer.
 * \param[out] output The target message pointer, which must
 *   have been initialized before calling this function.
 * \return true if successful, or false if either pointer is null
 *   or memory allocation fails.
 */
ROSIDL_GENERATOR_C_PUBLIC_rviz_resource_interfaces
bool
rviz_resource_interfaces__srv__GetResource_Response__copy(
  const rviz_resource_interfaces__srv__GetResource_Response * input,
  rviz_resource_interfaces__srv__GetResource_Response * output);

/// Initialize array of srv/GetResource messages.
/**
 * It allocates the memory for the number of elements and calls
 * rviz_resource_interfaces__srv__GetResource_Response__init()
 * for each element of the array.
 * \param[in,out] array The allocated array pointer.
 * \param[in] size The size / capacity of the array.
 * \return true if initialization was successful, otherwise false
 * If the array pointer is valid and the size is zero it is guaranteed
 # to return true.
 */
ROSIDL_GENERATOR_C_PUBLIC_rviz_resource_interfaces
bool
rviz_resource_interfaces__srv__GetResource_Response__Sequence__init(rviz_resource_interfaces__srv__GetResource_Response__Sequence * array, size_t size);

/// Finalize array of srv/GetResource messages.
/**
 * It calls
 * rviz_resource_interfaces__srv__GetResource_Response__fini()
 * for each element of the array and frees the memory for the number of
 * elements.
 * \param[in,out] array The initialized array pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_rviz_resource_interfaces
void
rviz_resource_interfaces__srv__GetResource_Response__Sequence__fini(rviz_resource_interfaces__srv__GetResource_Response__Sequence * array);

/// Create array of srv/GetResource messages.
/**
 * It allocates the memory for the array and calls
 * rviz_resource_interfaces__srv__GetResource_Response__Sequence__init().
 * \param[in] size The size / capacity of the array.
 * \return The pointer to the initialized array if successful, otherwise NULL
 */
ROSIDL_GENERATOR_C_PUBLIC_rviz_resource_interfaces
rviz_resource_interfaces__srv__GetResource_Response__Sequence *
rviz_resource_interfaces__srv__GetResource_Response__Sequence__create(size_t size);

/// Destroy array of srv/GetResource messages.
/**
 * It calls
 * rviz_resource_interfaces__srv__GetResource_Response__Sequence__fini()
 * on the array,
 * and frees the memory of the array.
 * \param[in,out] array The initialized array pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_rviz_resource_interfaces
void
rviz_resource_interfaces__srv__GetResource_Response__Sequence__destroy(rviz_resource_interfaces__srv__GetResource_Response__Sequence * array);

/// Check for srv/GetResource message array equality.
/**
 * \param[in] lhs The message array on the left hand size of the equality operator.
 * \param[in] rhs The message array on the right hand size of the equality operator.
 * \return true if message arrays are equal in size and content, otherwise false.
 */
ROSIDL_GENERATOR_C_PUBLIC_rviz_resource_interfaces
bool
rviz_resource_interfaces__srv__GetResource_Response__Sequence__are_equal(const rviz_resource_interfaces__srv__GetResource_Response__Sequence * lhs, const rviz_resource_interfaces__srv__GetResource_Response__Sequence * rhs);

/// Copy an array of srv/GetResource messages.
/**
 * This functions performs a deep copy, as opposed to the shallow copy that
 * plain assignment yields.
 *
 * \param[in] input The source array pointer.
 * \param[out] output The target array pointer, which must
 *   have been initialized before calling this function.
 * \return true if successful, or false if either pointer
 *   is null or memory allocation fails.
 */
ROSIDL_GENERATOR_C_PUBLIC_rviz_resource_interfaces
bool
rviz_resource_interfaces__srv__GetResource_Response__Sequence__copy(
  const rviz_resource_interfaces__srv__GetResource_Response__Sequence * input,
  rviz_resource_interfaces__srv__GetResource_Response__Sequence * output);

#ifdef __cplusplus
}
#endif

#endif  // RVIZ_RESOURCE_INTERFACES__SRV__DETAIL__GET_RESOURCE__FUNCTIONS_H_
