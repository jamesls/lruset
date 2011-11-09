"""An LRU bounded set.

This module implements a Least Recently Used bounded set.
"""


class _Node(object):
    __slots__ = ('value', 'previous', 'next')
    # For use as a doubly linked list.
    def __init__(self, value, previous_node=None, next_node=None):
        self.value = value
        self.previous = previous_node
        self.next = next_node


class LRUSet(object):
    """Implements a bounded lru set with O(1) behavior.

    """
    def __init__(self, max_size):
        self.max_size = max_size
        self.current_size = 0
        self._lookup_table = {}
        self._head = None
        self._tail = None

    def __contains__(self, item):
        if item in self._lookup_table:
            # Remove it from its current position and
            # add it to the end of the list.
            node = self._lookup_table.get(item)
            if node != self._head and node != self._tail:
                # We can shortcut the add/remove step
                # by just moving the node to the end of the
                # linked list.
                node.previous.next = node.next
                node.next.previous = node.previous
                self._tail.next = node
                self._tail = node
            else:
                self.remove(item)
                self.add(item)
            return True
        else:
            return False

    def add(self, item):
        # Interestingly, this next line is faster than
        # 'item in self'
        if self.__contains__(item):
            return
        node = _Node(item, previous_node=self._tail)
        self._lookup_table[item] = node
        self.current_size += 1
        # If this is the first time an item is being
        # added, the head reference needs to be set.
        if self._head is None:
            self._head = node
            node.previous = None
        # If this is the second time an item is being
        # added, the tail reference needs to be set.
        elif self._tail is None:
            self._head.next = node
            node.previous = self._head
            self._tail = node
        # From the third item and onwards, simply
        # add the item to the end of the list.
        else:
            self._tail.next = node
            self._tail = node
        if self.current_size > self.max_size:
            # The head reference is always the least
            # recently used.  This operation is inlined
            # instead of just calling remove() for faster
            # performance (~8-9% improvement).
            del self._lookup_table[self._head.value]
            self._head = self._head.next
            self.current_size -= 1

    def remove(self, item):
        node = self._lookup_table[item]
        # Again, there is the need to special case if its
        # either the head or tail reference that is going to
        # be removed.
        if node is self._head:
            self._head = node.next
        elif node is self._tail:
            self._tail = node.previous
        else:
            node.previous.next = node.next
            node.next.previous = node.previous
        del self._lookup_table[item]
        self.current_size -= 1

    def __len__(self):
        return self.current_size

    def __iter__(self):
        current = self._head
        while current is not None:
            yield current.value
            current = current.next
