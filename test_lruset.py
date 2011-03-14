#!/usr/bin/env python

import unittest
import time

import lruset
import clruset


class TestLRUSet(unittest.TestCase):
    def create_set(self, size):
        return clruset.LRUSet(size)

    def testNoElement(self):
        """Check boundary condition for empty set.
        """
        s = self.create_set(0)
        self.assertEqual(len(s), 0)

    def testSingleElement(self):
        """Check boundary condition for a single element.
        """
        s = self.create_set(1)
        s.add(1)
        self.assert_(1 in s)
        s.remove(1)
        self.assert_(1 not in s)

    def testLRUNoMax(self):
        """The set has no special behavior when len < max_size."""
        s = self.create_set(10)
        for i in xrange(10):
            s.add(i)
        for i in xrange(10):
            self.assert_(i in s)

    def testBasicAddRemoveCycling(self):
        """Add and remove are symmetric.

        In other words, if I add three elements and
        then remove three elements, there should
        be 0 elements in the set (asssuming I started
        with an empty set).
        """
        s = self.create_set(3)
        for i in xrange(3):
            s.add(i)
        for i in xrange(3):
            s.remove(i)
        self.assertEqual(len(s), 0)

    def testLRUBounded(self):
        """Ensure set never grows beyond its max_size.

        Also, the LRU element will be removed.
        """
        s = self.create_set(10)
        for i in xrange(11):
            s.add(i)
        # 0 is the LRU so it should have been removed.
        self.assertEqual(len(s), 10)
        self.assert_(0 not in s)


    def testLRUAccessOrder(self):
        """Verify LRU characteristics.
        """
        s = self.create_set(10)
        for i in xrange(10):
            s.add(i)
        for i in reversed(xrange(9)):
            self.assert_(i in s)
        s.add(100)
        self.assert_(9 not in s)
        s.add(101)
        self.assert_(8 not in s)

    def testLRUOrder(self):
        s = self.create_set(2)
        s.add(1)
        s.add(2)

        # Since 1 is accessed, 2 is the LRU and
        # will be removed when a new item is added.
        1 in s
        s.add(3)
        self.assert_(2 not in s)

        # Now make 1 the LRU by accessing 3.
        3 in s
        s.add(4)
        self.assert_(1 not in s)

    def testLRUWithMiddleElement(self):
        s = self.create_set(3)
        map(s.add, [0, 1, 2])
        self.assertTrue(1 in s)
        s.add(54)
        s.add(55)
        # Because 1 most accessed more recently than 0 and 2, it still in.
        self.assertTrue(1 in s)

    def testRemoveLastElementInSet(self):
        s = self.create_set(3)
        map(s.add, [0, 1, 2])
        s.remove(2)
        s.add(3)
        self.assertTrue(3 in s)

    def testLRUWhenAddingExistingElement(self):
        s = self.create_set(3)
        map(s.add, [0, 1, 2])
        s.add(0)
        s.add(3)
        s.add(4)
        # The set should be 0, 3, 4
        self.assertTrue(1 not in s)
        self.assertTrue(2 not in s)

    def testLRURemoveRange(self):
        s = self.create_set(10)
        for i in xrange(10):
            s.add(i)
        for i in xrange(10):
            s.remove(i)
        for i in xrange(10):
            self.assertTrue(i not in s, "%d in %s" % (i, s))
        for i in xrange(10):
            s.add(i)
        for i in xrange(10):
            self.assertTrue(i in s, "%d in %s" % (i, s))

    def testCurrentSize(self):
        s = self.create_set(5)
        for i in xrange(5):
            s.add(i)
        self.assertEqual(s.current_size, 5)
        for i in xrange(5):
            s.remove(i)
        self.assertEqual(s.current_size, 0)

    def testDuplicateAdds(self):
        s = self.create_set(5)
        for i in xrange(5):
            s.add(1)
        self.assertEqual(s.current_size, 1)

    def testRemoveRaisesKeyError(self):
        s = self.create_set(0)
        self.assertRaises(KeyError, s.remove, 54)


class TestLRUSetPython(TestLRUSet):
    def create_set(self, size):
        return lruset.LRUSet(size)


if __name__ == '__main__':
    unittest.main()
