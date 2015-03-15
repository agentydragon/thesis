Shared dictionary API
=====================

This library implements several data structures that support dictionary or
ordered dictionary operations. All implementations are usable through
an unified API.

To use the shared API, you need to ``#include "dict/dict.h``.

A dictionary instance is represented by a pointer to :c:type:`dict`,
which can be initialized by :c:func:`dict_init` and destroyed
by :c:func:`dict_destroy`.

Dictionaries store key-value pairs. Both keys and values are 64-bit
(``uint64_t``) and there may be at most one value per key.
Use :c:func:`dict_insert` to insert a new pair and :c:func:`dict_delete`
to remove an existing pair. To query for the presence of a key or for its
associated value, use :c:func:`dict_find`.

.. c:function:: int8_t dict_init(dict** _this, const dict_api* api, void* args)
Creates a new empty dictionary instance. ``*_this`` will be changed to hold
the new instance. ``api`` specifies the implementation to use; example
values include ``&dict_btree`` or ``&dict_cobt``.

.. TODO: void* args

Returns 0 if the dictionary was succesfully created, nonzero otherwise.
If :c:func:`dict_init` was successfull, the new dictionary needs to be
destroyed by a later call to :c:func:`dict_destroy`. If there were any
errors, no cleanup is required.

.. TODO: err_t?

.. c:function:: void dist_destroy(dict** _this)
Destroys the dictionary pointer to by ``_this`` and frees memory used by it.

.. c:type:: dict
A dictionary instance is represented by a pointer to ``dict*``.

.. c:type: dict_api

.. c:function:: int8_t dict_find(dict* this, uint64_t key, uint64_t *value, bool *found)
Looks up a key-value pair with the given ``key`` in ``this``.
If such a pair exists, ``*found`` is set to ``true``, and if ``value`` is not
``NULL``, ``*value`` is set to the value in the pair. If the dictionary contains
no value for ``key``, ``*found`` is set to ``false``.
Returns 0 if there were no errors, nonzero otherwise.

.. NOTE:
   ``found`` is a "required return value", because you should always check
   ``*found`` before using ``*value``. ``value`` is, on the other hand,
   optional. This enables querying for a key's presence without retrieving
   its value.

.. c:function:: int8_t dict_insert(dict* this, uint64_t key, uint64_t value)
Inserts a new key-value pair into ``this``. ``key`` must not be present
in ``this`` yet.
Returns 0 if successful, non-zero if an error occurs or if ``this`` already
contains a value for ``key``.

.. c:function:: int8_t dict_delete(dict* this, uint64_t key)
Removes the key-value pair associated with ``key`` from ``this``.
Returns 0 if successful, non-zero if an error occurs or if ``this`` does
not contain the key ``key``.

Sorted dictionary functions
---------------------------
A dictionary implementation may optionally provide support for
predecessor and successor queries using :c:func:`dict_prev` and
:c:func:`dict_next`. If the implementation doesn't support order queries,
:c:func:`dict_prev` and :c:func:`dict_next` do nothing and indicate
this by returning non-zero.

.. c:function:: int8_t dict_prev(dict* this, uint64_t key, uint64_t *prev_key, bool *found)
Looks up the largest key strictly smaller than ``key`` in ``this``.
If such a key exists, ``*found`` is set to ``true`` and ``*prev_key`` is
set to the previous key (if ``prev_key != NULL``).
Otherwise, ``key`` is non-strictly smaller than all keys in ``this``, so
``*found`` is set to ``false`` to indicate that no smaller key exists.
Returns 0 on success, non-zero on errors.

.. c:function:: int8_t dict_next(dict* this, uint64_t key, uint64_t *next_key, bool *found)
Looks up the smallest key strictly larger than ``key`` in ``this``.
``next_key``, ``found`` and the return value are analogous
to :c:func:`dict_prev`.

.. TODO: dict_dump, dict_check

Implementing a dictionary
-------------------------
To build a new dictionary implementation, you need to create an instance
of the :c:type:`dict_api` structure, probably via a global variable
named `dict_something` and declared in a header file. The :c:type:`dict_api`
structure contains pointers to various functions called by the ``dict_*``
family of functions to manipulate the data structure.
The functions represent the structure as an opaque ``void*`` pointer.

.. c:type:: dict_api
.. c:member:: const char* name
Zero-terminated string identifying the implementation or ``NULL``.

.. c:member:: int (*init)(void** _this, void* args)
Initializes a new instance of the data structure. Sets ``*_this`` to
an implementation-specific value representing the new empty structure.
Returns 0 if no errors occurred.
Must not be ``NULL``.

.. c:member:: void (*destroy)(void** _this)
Frees all memory used by the structure in ``*_this``.
Must not be ``NULL``.

All of the following required members perform functions analogous to
the ``dict_*`` family of functions:
.. c:member:: void (*find)(void* this, uint64_t key, uint64_t *value, bool *found)
.. c:member:: void (*insert)(void* this, uint64_t key, uint64_t value)
.. c:member:: void (*delete)(void* this, uint64_t key)

Optionally, the data structure may implement the following extensions:
.. c:member:: void (*next)(void* this, uint64_t key, uint64_t *next_key, bool *found)
.. c:member:: void (*prev)(void* this, uint64_t key, uint64_t *prev_key, bool *found)
.. TODO: dump, check
