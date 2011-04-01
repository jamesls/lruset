A fixed size set in python.

This is a simple data structure that supports adding/removing elements
as well as checking the set for membership.


Currently all that's supported is ``add``, ``remove``, ``__contains__``,
and ``__iter__``.

There is currently no plans to support creating new sets from existing
LRUSets (union, intersection, difference, etc.)

Example usage::

  >>> import clruset
  >>> s = clruset.LRUSet(max_size=5)
  >>> for i in range(5): s.add(i)
  ... 
  >>> len(s)
  5
  >>> s.add(100)
  >>> len(s)
  5
  >>> 0 in s
  False

Once the ``max_size`` is reached, the LRU element is evicted.  In the
example above, this is ``0``, and ``0`` is shown to no longer be in the
set.  This behavior also applies to checking for membership::

  >>> import clruset
  >>> s = clruset.LRUSet(max_size=5)
  >>> for i in range(5): s.add(i)
  ... 
  >>> 0 in s
  True
  >>> 1 in s
  True
  >>> # 2 is now the LRU so adding a new element will evict it.
  ... 
  >>> s.add(100)
  >>> 2 in s
  False

Iteration is in LRU order where the first element is the LRU element::

  >>> import clruset
  >>> s = clruset.LRUSet(max_size=5)
  >>> for i in range(5): s.add(i)
  ... 
  >>> s.add(100)
  >>> s.add(200)
  >>> print list(iter(s))
  [2, 3, 4, 100, 200]
  >>> s.add('new')
  >>> print list(iter(s))
  [3, 4, 100, 200, 'new']


There's two implementations of ``LRUSet``, a pure python verion
(``lruset.LRUSet``), and a C version (``clruset.LRUSet``).
