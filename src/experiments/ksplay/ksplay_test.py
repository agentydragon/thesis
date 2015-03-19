#!/usr/bin/python

import ksplay

def test_insert_into_node():
  node = ksplay.Node()
  node.insert(10)
  assert node.keys == [10]
  node.insert(15)
  node.insert(5)
  assert node.keys == [5, 10, 15]

def test_insert_into_node_right():
  node = ksplay.Node()
  node.insert(10)
  node.insert(5)
  assert node.keys == [5, 10]

def test_delete_form_node():
  node = ksplay.Node()
  node.keys = [10, 20, 30, 40]
  node.children = [None, None, None, None, None]
  node.remove(10)
  assert node.keys == [20, 30, 40]
  node.remove(40)
  assert node.keys == [20, 30]
  node.remove(20)
  assert node.keys == [30]
  node.remove(30)
  assert node.keys == []

def test_walk_to():
  tree = ksplay.Tree()

  a = ksplay.Node()
  a.keys = [48, 49]
  a.children = [None, None, None]

  b = ksplay.Node()
  b.keys = [30, 45]
  b.children = [None, None, a]

  c = ksplay.Node()
  c.keys = [25, 50, 75]
  c.children = [None, b, None, None]

  tree.root = ksplay.Node()
  tree.root.keys = [100, 200]
  tree.root.children = [c, None, None, None]

  assert tree.walk_to(47) == [tree.root, c, b, a]

def test_flatten():
  a = ksplay.Node()
  a.keys = [100]
  a.children = ['A', 'B']

  b = ksplay.Node()
  b.keys = [20, 50]
  b.children = ['C', 'D', a]

  c = ksplay.Node()
  c.keys = [10, 500]
  c.children = ['E', b, 'F']

  keys, children = ksplay.Tree.flatten([c, b, a])
  assert keys == [10, 20, 50, 100, 500]
  assert children == ['E', 'C', 'D', 'A', 'B', 'F']

def test_nonfull_compose():
  ksplay.Tree.K = 3
  keys = [10, 20]
  children = ['a', 'b', 'c']
  root = ksplay.Tree.compose(keys, children)
  assert root.keys == [10, 20]
  assert root.children == ['a', 'b', 'c']

def test_exact_compose():
  ksplay.Tree.K = 3

  # Exact compose
  keys = [10, 20, 30, 40, 50, 60, 70, 80]
  children = ['A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I']
  root = ksplay.Tree.compose(keys, children)
  assert root.keys == [30, 60]
  assert root.children[0].keys == [10, 20]
  assert root.children[0].children == ['A', 'B', 'C']
  assert root.children[1].keys == [40, 50]
  assert root.children[1].children == ['D', 'E', 'F']
  assert root.children[2].keys == [70, 80]
  assert root.children[2].children == ['G', 'H', 'I']

def test_underfull_compose():
  ksplay.Tree.K = 3

  # Underfull compose
  keys = [10, 20, 30, 40, 50]
  children = ['a', 'b', 'c', 'd', 'e', 'f']
  root = ksplay.Tree.compose(keys, children)
  assert root.keys == [30]
  assert root.children[0].keys == [10, 20]
  assert root.children[0].children == ['a', 'b', 'c']
  assert root.children[1].keys == [40, 50]
  assert root.children[1].children == ['d', 'e', 'f']

def test_overfull_compose():
  # Overfull compose
  ksplay.Tree.K = 2
  keys = [10, 20, 30, 40, 50]
  children = ['a', 'b', 'c', 'd', 'e', 'f']
  root = ksplay.Tree.compose(keys, children)
  assert root.keys == [20, 40]
  assert root.children[0].keys == [10]
  assert root.children[0].children == ['a', 'b']
  assert root.children[1].keys == [30]
  assert root.children[1].children == ['c', 'd']
  assert root.children[2].keys == [50]
  assert root.children[2].children == ['e', 'f']

def test_mixed_compose():
  # Terminal splay
  ksplay.Tree.K = 3
  keys = [10, 20, 30, 40]
  children = ['a', 'b', 'c', 'd', 'e']
  root = ksplay.Tree.compose(keys, children)
  assert root.keys == [30, 40]
  assert root.children[0].keys == [10, 20]
  assert root.children[0].children == ['a', 'b', 'c']
  assert root.children[1] == 'd'
  assert root.children[2] == 'e'

def test_ksplay_trivial():
  # K-splay the only node
  ksplay.Tree.K = 2
  root = ksplay.Node()
  root.keys = [10]
  root.children = ['a', 'b']
  splayed = ksplay.Tree.ksplay([root])
  assert splayed.keys == [10]
  assert splayed.children == ['a', 'b']

def test_insert():
  tree = ksplay.Tree()
  assert not tree.contains(10)

  tree.insert(10)
  assert tree.contains(10)

  tree.insert(20)
  assert all(tree.contains(key) for key in [10, 20])

  tree.insert(30)
  assert all(tree.contains(key) for key in [10, 20, 30])

  for key in [15, 25, 5, 50, 40, 35, 45]:
    tree.insert(key)
  assert all(tree.contains(key)
             for key in [5, 10, 15, 20, 25, 30, 35, 40, 45, 50])

def test_insert_and_delete():
  def assert_contains(keys):
    assert all(tree.contains(key) for key in keys)
  def assert_does_not_contain(keys):
    assert not any(tree.contains(key) for key in keys)

  tree = ksplay.Tree()
  for key in range(10):
    tree.insert(key * 2)

  for key in [6, 8, 10]:
    tree.delete(key)

  assert_contains([0, 2, 4, 12, 14, 16, 18])
  assert_does_not_contain([6, 8, 10])

  for key in [0, 12, 16]:
    tree.delete(key)

  assert_contains([2, 4, 14, 18])
  assert_does_not_contain([0, 6, 8, 10, 12, 16])

  for key in [4, 2, 18]:
    tree.delete(key)

  assert_contains([14])
  assert_does_not_contain([0, 2, 4, 6, 8, 10, 12, 16, 18])

  tree.delete(14)
  assert_does_not_contain([14])
