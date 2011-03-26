"""An LRU bounded set.

This module implements a Least Recently Used bounded set.
"""


class Node(object):
    __slots__ = ('value', 'previous', 'next')
    # For use as a doubly linked list.
    def __init__(self, value, previous=None, next=None):
        self.value = value
        self.previous = previous
        self.next = next


class LRUSet(object):
    """Implements a bounded lru set with O(1) behavior.

    """
    def __init__(self, max_size):
        self.max_size = max_size
        self.current_size = 0
        self.lookup_table = {}
        self.head = None
        self.tail = None

    def __contains__(self, item):
        if item in self.lookup_table:
            # Remove it from its current position and
            # add it to the end of the list.
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
        node = Node(item, previous=self.tail)
        self.lookup_table[item] = node
        self.current_size += 1
        # If this is the first time an item is being
        # added, the head reference needs to be set.
        if self.head is None:
            self.head = node
            node.previous = None
        # If this is the second time an item is being
        # added, the tail reference needs to be set.
        elif self.tail is None:
            self.head.next = node
            node.previous = self.head
            self.tail = node
        # From the third item and onwards, simply
        # add the item to the end of the list.
        else:
            self.tail.next = node
            self.tail = node
        if self.current_size > self.max_size:
            # The head reference is always the least
            # recently used.
            self.remove(self.head.value)

    def remove(self, item):
        node = self.lookup_table[item]
        # Again, there is the need to special case if its
        # either the head or tail reference that is going to
        # be removed.
        if node is self.head:
            self.head = node.next
        elif node is self.tail:
            self.tail = node.previous
        else:
            node.previous.next = node.next
            node.next.previous = node.previous
        del self.lookup_table[item]
        self.current_size -= 1

    def __len__(self):
        return self.current_size

    def __iter__(self):
        current = self.head
        while current is not None:
            yield current.value
            current = current.next

