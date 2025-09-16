// generated from rosidl_generator_cpp/resource/idl__struct.hpp.em
// with input from rviz_resource_interfaces:srv/GetResource.idl
// generated code does not contain a copyright notice

#ifndef RVIZ_RESOURCE_INTERFACES__SRV__DETAIL__GET_RESOURCE__STRUCT_HPP_
#define RVIZ_RESOURCE_INTERFACES__SRV__DETAIL__GET_RESOURCE__STRUCT_HPP_

#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <vector>

#include "rosidl_runtime_cpp/bounded_vector.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


#ifndef _WIN32
# define DEPRECATED__rviz_resource_interfaces__srv__GetResource_Request __attribute__((deprecated))
#else
# define DEPRECATED__rviz_resource_interfaces__srv__GetResource_Request __declspec(deprecated)
#endif

namespace rviz_resource_interfaces
{

namespace srv
{

// message struct
template<class ContainerAllocator>
struct GetResource_Request_
{
  using Type = GetResource_Request_<ContainerAllocator>;

  explicit GetResource_Request_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->path = "";
      this->etag = "";
    }
  }

  explicit GetResource_Request_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : path(_alloc),
    etag(_alloc)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->path = "";
      this->etag = "";
    }
  }

  // field types and members
  using _path_type =
    std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>;
  _path_type path;
  using _etag_type =
    std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>;
  _etag_type etag;

  // setters for named parameter idiom
  Type & set__path(
    const std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>> & _arg)
  {
    this->path = _arg;
    return *this;
  }
  Type & set__etag(
    const std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>> & _arg)
  {
    this->etag = _arg;
    return *this;
  }

  // constant declarations

  // pointer types
  using RawPtr =
    rviz_resource_interfaces::srv::GetResource_Request_<ContainerAllocator> *;
  using ConstRawPtr =
    const rviz_resource_interfaces::srv::GetResource_Request_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<rviz_resource_interfaces::srv::GetResource_Request_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<rviz_resource_interfaces::srv::GetResource_Request_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      rviz_resource_interfaces::srv::GetResource_Request_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<rviz_resource_interfaces::srv::GetResource_Request_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      rviz_resource_interfaces::srv::GetResource_Request_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<rviz_resource_interfaces::srv::GetResource_Request_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<rviz_resource_interfaces::srv::GetResource_Request_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<rviz_resource_interfaces::srv::GetResource_Request_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__rviz_resource_interfaces__srv__GetResource_Request
    std::shared_ptr<rviz_resource_interfaces::srv::GetResource_Request_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__rviz_resource_interfaces__srv__GetResource_Request
    std::shared_ptr<rviz_resource_interfaces::srv::GetResource_Request_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const GetResource_Request_ & other) const
  {
    if (this->path != other.path) {
      return false;
    }
    if (this->etag != other.etag) {
      return false;
    }
    return true;
  }
  bool operator!=(const GetResource_Request_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct GetResource_Request_

// alias to use template instance with default allocator
using GetResource_Request =
  rviz_resource_interfaces::srv::GetResource_Request_<std::allocator<void>>;

// constant definitions

}  // namespace srv

}  // namespace rviz_resource_interfaces


#ifndef _WIN32
# define DEPRECATED__rviz_resource_interfaces__srv__GetResource_Response __attribute__((deprecated))
#else
# define DEPRECATED__rviz_resource_interfaces__srv__GetResource_Response __declspec(deprecated)
#endif

namespace rviz_resource_interfaces
{

namespace srv
{

// message struct
template<class ContainerAllocator>
struct GetResource_Response_
{
  using Type = GetResource_Response_<ContainerAllocator>;

  explicit GetResource_Response_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->status_code = 0l;
      this->error_reason = "";
      this->expanded_path = "";
      this->etag = "";
    }
  }

  explicit GetResource_Response_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : error_reason(_alloc),
    expanded_path(_alloc),
    etag(_alloc)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->status_code = 0l;
      this->error_reason = "";
      this->expanded_path = "";
      this->etag = "";
    }
  }

  // field types and members
  using _status_code_type =
    int32_t;
  _status_code_type status_code;
  using _error_reason_type =
    std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>;
  _error_reason_type error_reason;
  using _expanded_path_type =
    std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>;
  _expanded_path_type expanded_path;
  using _etag_type =
    std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>;
  _etag_type etag;
  using _body_type =
    std::vector<uint8_t, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<uint8_t>>;
  _body_type body;

  // setters for named parameter idiom
  Type & set__status_code(
    const int32_t & _arg)
  {
    this->status_code = _arg;
    return *this;
  }
  Type & set__error_reason(
    const std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>> & _arg)
  {
    this->error_reason = _arg;
    return *this;
  }
  Type & set__expanded_path(
    const std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>> & _arg)
  {
    this->expanded_path = _arg;
    return *this;
  }
  Type & set__etag(
    const std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>> & _arg)
  {
    this->etag = _arg;
    return *this;
  }
  Type & set__body(
    const std::vector<uint8_t, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<uint8_t>> & _arg)
  {
    this->body = _arg;
    return *this;
  }

  // constant declarations
  // guard against 'ERROR' being predefined by MSVC by temporarily undefining it
#if defined(_WIN32)
#  if defined(ERROR)
#    pragma push_macro("ERROR")
#    undef ERROR
#  endif
#endif
  static constexpr int32_t ERROR =
    0;
#if defined(_WIN32)
#  pragma warning(suppress : 4602)
#  pragma pop_macro("ERROR")
#endif
  static constexpr int32_t OK =
    1;
  static constexpr int32_t NOT_MODIFIED =
    2;

  // pointer types
  using RawPtr =
    rviz_resource_interfaces::srv::GetResource_Response_<ContainerAllocator> *;
  using ConstRawPtr =
    const rviz_resource_interfaces::srv::GetResource_Response_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<rviz_resource_interfaces::srv::GetResource_Response_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<rviz_resource_interfaces::srv::GetResource_Response_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      rviz_resource_interfaces::srv::GetResource_Response_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<rviz_resource_interfaces::srv::GetResource_Response_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      rviz_resource_interfaces::srv::GetResource_Response_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<rviz_resource_interfaces::srv::GetResource_Response_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<rviz_resource_interfaces::srv::GetResource_Response_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<rviz_resource_interfaces::srv::GetResource_Response_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__rviz_resource_interfaces__srv__GetResource_Response
    std::shared_ptr<rviz_resource_interfaces::srv::GetResource_Response_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__rviz_resource_interfaces__srv__GetResource_Response
    std::shared_ptr<rviz_resource_interfaces::srv::GetResource_Response_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const GetResource_Response_ & other) const
  {
    if (this->status_code != other.status_code) {
      return false;
    }
    if (this->error_reason != other.error_reason) {
      return false;
    }
    if (this->expanded_path != other.expanded_path) {
      return false;
    }
    if (this->etag != other.etag) {
      return false;
    }
    if (this->body != other.body) {
      return false;
    }
    return true;
  }
  bool operator!=(const GetResource_Response_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct GetResource_Response_

// alias to use template instance with default allocator
using GetResource_Response =
  rviz_resource_interfaces::srv::GetResource_Response_<std::allocator<void>>;

// constant definitions
// guard against 'ERROR' being predefined by MSVC by temporarily undefining it
#if defined(_WIN32)
#  if defined(ERROR)
#    pragma push_macro("ERROR")
#    undef ERROR
#  endif
#endif
#if __cplusplus < 201703L
// static constexpr member variable definitions are only needed in C++14 and below, deprecated in C++17
template<typename ContainerAllocator>
constexpr int32_t GetResource_Response_<ContainerAllocator>::ERROR;
#endif  // __cplusplus < 201703L
#if defined(_WIN32)
#  pragma warning(suppress : 4602)
#  pragma pop_macro("ERROR")
#endif
#if __cplusplus < 201703L
// static constexpr member variable definitions are only needed in C++14 and below, deprecated in C++17
template<typename ContainerAllocator>
constexpr int32_t GetResource_Response_<ContainerAllocator>::OK;
#endif  // __cplusplus < 201703L
#if __cplusplus < 201703L
// static constexpr member variable definitions are only needed in C++14 and below, deprecated in C++17
template<typename ContainerAllocator>
constexpr int32_t GetResource_Response_<ContainerAllocator>::NOT_MODIFIED;
#endif  // __cplusplus < 201703L

}  // namespace srv

}  // namespace rviz_resource_interfaces

namespace rviz_resource_interfaces
{

namespace srv
{

struct GetResource
{
  using Request = rviz_resource_interfaces::srv::GetResource_Request;
  using Response = rviz_resource_interfaces::srv::GetResource_Response;
};

}  // namespace srv

}  // namespace rviz_resource_interfaces

#endif  // RVIZ_RESOURCE_INTERFACES__SRV__DETAIL__GET_RESOURCE__STRUCT_HPP_
