# generated from rosidl_generator_py/resource/_idl.py.em
# with input from rviz_resource_interfaces:srv/GetResource.idl
# generated code does not contain a copyright notice


# Import statements for member types

import builtins  # noqa: E402, I100

import rosidl_parser.definition  # noqa: E402, I100


class Metaclass_GetResource_Request(type):
    """Metaclass of message 'GetResource_Request'."""

    _CREATE_ROS_MESSAGE = None
    _CONVERT_FROM_PY = None
    _CONVERT_TO_PY = None
    _DESTROY_ROS_MESSAGE = None
    _TYPE_SUPPORT = None

    __constants = {
    }

    @classmethod
    def __import_type_support__(cls):
        try:
            from rosidl_generator_py import import_type_support
            module = import_type_support('rviz_resource_interfaces')
        except ImportError:
            import logging
            import traceback
            logger = logging.getLogger(
                'rviz_resource_interfaces.srv.GetResource_Request')
            logger.debug(
                'Failed to import needed modules for type support:\n' +
                traceback.format_exc())
        else:
            cls._CREATE_ROS_MESSAGE = module.create_ros_message_msg__srv__get_resource__request
            cls._CONVERT_FROM_PY = module.convert_from_py_msg__srv__get_resource__request
            cls._CONVERT_TO_PY = module.convert_to_py_msg__srv__get_resource__request
            cls._TYPE_SUPPORT = module.type_support_msg__srv__get_resource__request
            cls._DESTROY_ROS_MESSAGE = module.destroy_ros_message_msg__srv__get_resource__request

    @classmethod
    def __prepare__(cls, name, bases, **kwargs):
        # list constant names here so that they appear in the help text of
        # the message class under "Data and other attributes defined here:"
        # as well as populate each message instance
        return {
        }


class GetResource_Request(metaclass=Metaclass_GetResource_Request):
    """Message class 'GetResource_Request'."""

    __slots__ = [
        '_path',
        '_etag',
    ]

    _fields_and_field_types = {
        'path': 'string',
        'etag': 'string',
    }

    SLOT_TYPES = (
        rosidl_parser.definition.UnboundedString(),  # noqa: E501
        rosidl_parser.definition.UnboundedString(),  # noqa: E501
    )

    def __init__(self, **kwargs):
        assert all('_' + key in self.__slots__ for key in kwargs.keys()), \
            'Invalid arguments passed to constructor: %s' % \
            ', '.join(sorted(k for k in kwargs.keys() if '_' + k not in self.__slots__))
        self.path = kwargs.get('path', str())
        self.etag = kwargs.get('etag', str())

    def __repr__(self):
        typename = self.__class__.__module__.split('.')
        typename.pop()
        typename.append(self.__class__.__name__)
        args = []
        for s, t in zip(self.__slots__, self.SLOT_TYPES):
            field = getattr(self, s)
            fieldstr = repr(field)
            # We use Python array type for fields that can be directly stored
            # in them, and "normal" sequences for everything else.  If it is
            # a type that we store in an array, strip off the 'array' portion.
            if (
                isinstance(t, rosidl_parser.definition.AbstractSequence) and
                isinstance(t.value_type, rosidl_parser.definition.BasicType) and
                t.value_type.typename in ['float', 'double', 'int8', 'uint8', 'int16', 'uint16', 'int32', 'uint32', 'int64', 'uint64']
            ):
                if len(field) == 0:
                    fieldstr = '[]'
                else:
                    assert fieldstr.startswith('array(')
                    prefix = "array('X', "
                    suffix = ')'
                    fieldstr = fieldstr[len(prefix):-len(suffix)]
            args.append(s[1:] + '=' + fieldstr)
        return '%s(%s)' % ('.'.join(typename), ', '.join(args))

    def __eq__(self, other):
        if not isinstance(other, self.__class__):
            return False
        if self.path != other.path:
            return False
        if self.etag != other.etag:
            return False
        return True

    @classmethod
    def get_fields_and_field_types(cls):
        from copy import copy
        return copy(cls._fields_and_field_types)

    @builtins.property
    def path(self):
        """Message field 'path'."""
        return self._path

    @path.setter
    def path(self, value):
        if __debug__:
            assert \
                isinstance(value, str), \
                "The 'path' field must be of type 'str'"
        self._path = value

    @builtins.property
    def etag(self):
        """Message field 'etag'."""
        return self._etag

    @etag.setter
    def etag(self, value):
        if __debug__:
            assert \
                isinstance(value, str), \
                "The 'etag' field must be of type 'str'"
        self._etag = value


# Import statements for member types

# Member 'body'
import array  # noqa: E402, I100

# already imported above
# import builtins

# already imported above
# import rosidl_parser.definition


class Metaclass_GetResource_Response(type):
    """Metaclass of message 'GetResource_Response'."""

    _CREATE_ROS_MESSAGE = None
    _CONVERT_FROM_PY = None
    _CONVERT_TO_PY = None
    _DESTROY_ROS_MESSAGE = None
    _TYPE_SUPPORT = None

    __constants = {
        'ERROR': 0,
        'OK': 1,
        'NOT_MODIFIED': 2,
    }

    @classmethod
    def __import_type_support__(cls):
        try:
            from rosidl_generator_py import import_type_support
            module = import_type_support('rviz_resource_interfaces')
        except ImportError:
            import logging
            import traceback
            logger = logging.getLogger(
                'rviz_resource_interfaces.srv.GetResource_Response')
            logger.debug(
                'Failed to import needed modules for type support:\n' +
                traceback.format_exc())
        else:
            cls._CREATE_ROS_MESSAGE = module.create_ros_message_msg__srv__get_resource__response
            cls._CONVERT_FROM_PY = module.convert_from_py_msg__srv__get_resource__response
            cls._CONVERT_TO_PY = module.convert_to_py_msg__srv__get_resource__response
            cls._TYPE_SUPPORT = module.type_support_msg__srv__get_resource__response
            cls._DESTROY_ROS_MESSAGE = module.destroy_ros_message_msg__srv__get_resource__response

    @classmethod
    def __prepare__(cls, name, bases, **kwargs):
        # list constant names here so that they appear in the help text of
        # the message class under "Data and other attributes defined here:"
        # as well as populate each message instance
        return {
            'ERROR': cls.__constants['ERROR'],
            'OK': cls.__constants['OK'],
            'NOT_MODIFIED': cls.__constants['NOT_MODIFIED'],
        }

    @property
    def ERROR(self):
        """Message constant 'ERROR'."""
        return Metaclass_GetResource_Response.__constants['ERROR']

    @property
    def OK(self):
        """Message constant 'OK'."""
        return Metaclass_GetResource_Response.__constants['OK']

    @property
    def NOT_MODIFIED(self):
        """Message constant 'NOT_MODIFIED'."""
        return Metaclass_GetResource_Response.__constants['NOT_MODIFIED']


class GetResource_Response(metaclass=Metaclass_GetResource_Response):
    """
    Message class 'GetResource_Response'.

    Constants:
      ERROR
      OK
      NOT_MODIFIED
    """

    __slots__ = [
        '_status_code',
        '_error_reason',
        '_expanded_path',
        '_etag',
        '_body',
    ]

    _fields_and_field_types = {
        'status_code': 'int32',
        'error_reason': 'string',
        'expanded_path': 'string',
        'etag': 'string',
        'body': 'sequence<uint8>',
    }

    SLOT_TYPES = (
        rosidl_parser.definition.BasicType('int32'),  # noqa: E501
        rosidl_parser.definition.UnboundedString(),  # noqa: E501
        rosidl_parser.definition.UnboundedString(),  # noqa: E501
        rosidl_parser.definition.UnboundedString(),  # noqa: E501
        rosidl_parser.definition.UnboundedSequence(rosidl_parser.definition.BasicType('uint8')),  # noqa: E501
    )

    def __init__(self, **kwargs):
        assert all('_' + key in self.__slots__ for key in kwargs.keys()), \
            'Invalid arguments passed to constructor: %s' % \
            ', '.join(sorted(k for k in kwargs.keys() if '_' + k not in self.__slots__))
        self.status_code = kwargs.get('status_code', int())
        self.error_reason = kwargs.get('error_reason', str())
        self.expanded_path = kwargs.get('expanded_path', str())
        self.etag = kwargs.get('etag', str())
        self.body = array.array('B', kwargs.get('body', []))

    def __repr__(self):
        typename = self.__class__.__module__.split('.')
        typename.pop()
        typename.append(self.__class__.__name__)
        args = []
        for s, t in zip(self.__slots__, self.SLOT_TYPES):
            field = getattr(self, s)
            fieldstr = repr(field)
            # We use Python array type for fields that can be directly stored
            # in them, and "normal" sequences for everything else.  If it is
            # a type that we store in an array, strip off the 'array' portion.
            if (
                isinstance(t, rosidl_parser.definition.AbstractSequence) and
                isinstance(t.value_type, rosidl_parser.definition.BasicType) and
                t.value_type.typename in ['float', 'double', 'int8', 'uint8', 'int16', 'uint16', 'int32', 'uint32', 'int64', 'uint64']
            ):
                if len(field) == 0:
                    fieldstr = '[]'
                else:
                    assert fieldstr.startswith('array(')
                    prefix = "array('X', "
                    suffix = ')'
                    fieldstr = fieldstr[len(prefix):-len(suffix)]
            args.append(s[1:] + '=' + fieldstr)
        return '%s(%s)' % ('.'.join(typename), ', '.join(args))

    def __eq__(self, other):
        if not isinstance(other, self.__class__):
            return False
        if self.status_code != other.status_code:
            return False
        if self.error_reason != other.error_reason:
            return False
        if self.expanded_path != other.expanded_path:
            return False
        if self.etag != other.etag:
            return False
        if self.body != other.body:
            return False
        return True

    @classmethod
    def get_fields_and_field_types(cls):
        from copy import copy
        return copy(cls._fields_and_field_types)

    @builtins.property
    def status_code(self):
        """Message field 'status_code'."""
        return self._status_code

    @status_code.setter
    def status_code(self, value):
        if __debug__:
            assert \
                isinstance(value, int), \
                "The 'status_code' field must be of type 'int'"
            assert value >= -2147483648 and value < 2147483648, \
                "The 'status_code' field must be an integer in [-2147483648, 2147483647]"
        self._status_code = value

    @builtins.property
    def error_reason(self):
        """Message field 'error_reason'."""
        return self._error_reason

    @error_reason.setter
    def error_reason(self, value):
        if __debug__:
            assert \
                isinstance(value, str), \
                "The 'error_reason' field must be of type 'str'"
        self._error_reason = value

    @builtins.property
    def expanded_path(self):
        """Message field 'expanded_path'."""
        return self._expanded_path

    @expanded_path.setter
    def expanded_path(self, value):
        if __debug__:
            assert \
                isinstance(value, str), \
                "The 'expanded_path' field must be of type 'str'"
        self._expanded_path = value

    @builtins.property
    def etag(self):
        """Message field 'etag'."""
        return self._etag

    @etag.setter
    def etag(self, value):
        if __debug__:
            assert \
                isinstance(value, str), \
                "The 'etag' field must be of type 'str'"
        self._etag = value

    @builtins.property
    def body(self):
        """Message field 'body'."""
        return self._body

    @body.setter
    def body(self, value):
        if isinstance(value, array.array):
            assert value.typecode == 'B', \
                "The 'body' array.array() must have the type code of 'B'"
            self._body = value
            return
        if __debug__:
            from collections.abc import Sequence
            from collections.abc import Set
            from collections import UserList
            from collections import UserString
            assert \
                ((isinstance(value, Sequence) or
                  isinstance(value, Set) or
                  isinstance(value, UserList)) and
                 not isinstance(value, str) and
                 not isinstance(value, UserString) and
                 all(isinstance(v, int) for v in value) and
                 all(val >= 0 and val < 256 for val in value)), \
                "The 'body' field must be a set or sequence and each value of type 'int' and each unsigned integer in [0, 255]"
        self._body = array.array('B', value)


class Metaclass_GetResource(type):
    """Metaclass of service 'GetResource'."""

    _TYPE_SUPPORT = None

    @classmethod
    def __import_type_support__(cls):
        try:
            from rosidl_generator_py import import_type_support
            module = import_type_support('rviz_resource_interfaces')
        except ImportError:
            import logging
            import traceback
            logger = logging.getLogger(
                'rviz_resource_interfaces.srv.GetResource')
            logger.debug(
                'Failed to import needed modules for type support:\n' +
                traceback.format_exc())
        else:
            cls._TYPE_SUPPORT = module.type_support_srv__srv__get_resource

            from rviz_resource_interfaces.srv import _get_resource
            if _get_resource.Metaclass_GetResource_Request._TYPE_SUPPORT is None:
                _get_resource.Metaclass_GetResource_Request.__import_type_support__()
            if _get_resource.Metaclass_GetResource_Response._TYPE_SUPPORT is None:
                _get_resource.Metaclass_GetResource_Response.__import_type_support__()


class GetResource(metaclass=Metaclass_GetResource):
    from rviz_resource_interfaces.srv._get_resource import GetResource_Request as Request
    from rviz_resource_interfaces.srv._get_resource import GetResource_Response as Response

    def __init__(self):
        raise NotImplementedError('Service classes can not be instantiated')
