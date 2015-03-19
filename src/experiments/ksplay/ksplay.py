#!/usr/bin/python

class Node(object):
  def __init__(self):
    self.keys = []
    self.children = [None]

  def insert(self, inserted_key):
    before = len([key for key in self.keys if key < inserted_key])
    self.keys = self.keys[:before] + [inserted_key] + self.keys[before:]
    # XXX: assert none
    self.children.append(None)

  def remove(self, removed_key):
    before = len([key for key in self.keys if key < removed_key])
    self.keys[before:] = self.keys[before+1:]
    # XXX: assert none
    self.children.pop()

  def contains(self, key):
    return key in self.keys

  def __str__(self):
    return "[" + str(self.keys) + " => " + str(self.children) + "]"

class Tree(object):
  K = 3

  def __init__(self):
    self.root = Node()

  def walk_to(self, splayed_key):
    stack = []
    current = self.root

    while current != None:
      #print("at " + str(current))
      stack.append(current)
      for key, child in zip(current.keys, current.children):
        if splayed_key == key:
          go_to = None
          break
        if splayed_key < key:
          go_to = child
          break
      else:
        if current.children:
          go_to = current.children[-1]
        else:
          go_to = None

      current = go_to

    return stack

  @staticmethod
  def flatten(stack):
    """Returns the keys and external nodes of a stack in BFS order."""
    keys, children = [], []
    def explore(node):
      if node in stack:
        for child, key in zip(node.children, node.keys):
          explore(child)
          keys.append(key)
        if node.children:
          explore(node.children[-1])
        else:
          explore(None)
      else:
        children.append(node)
    explore(stack[0])
    return keys, children

  @classmethod
  def compose(cls, keys, children):
    """Puts keys and children under a new framework and returns its root."""
    assert len(keys) == len(children) - 1

    print("Compose(", keys, ",", children, ")")

    if len(children) <= cls.K:
      root = Node()
      root.keys = keys
      root.children = children
      return root
    else:
      # Allowed operations: exact K-splay, underfull K-splay, overfull K-splay
      # assert (len(children) == cls.K * cls.K or
      #         len(children) == cls.K * (cls.K - 1) or
      #         len(children) == cls.K * (cls.K + 1))
      root = Node()
      root.children = []
      while len(children) >= cls.K:
        key_slice = keys[:cls.K]
        children_slice = children[:cls.K]

        if len(keys) >= cls.K:
          lower_node = Node()
          lower_node.keys = key_slice[:-1]
          lower_node.children = children_slice

          root.keys.append(keys[cls.K-1])
        else:
          lower_node = Node()
          lower_node.keys = key_slice
          lower_node.children = children_slice

        root.children.append(lower_node)

        keys = keys[cls.K:]
        children = children[cls.K:]

      # Special case for terminal splays.
      #if len(children) % cls.K != 0:
      #  # Append all remaining children under the root.
      #  min_idx = len(children) - (len(children) % cls.K)
      #  for i in range(min_idx, len(children)):
      #    if i < len(keys):
      #      root.keys.append(keys[i])
      #    root.children.append(children[i])

      return root

  @classmethod
  def ksplay_step(cls, stack):
    """Does a single K-splaying step on the stack."""
    #assert len(stack[-1].children) in [cls.K-1, cls.K, cls.K+1]

    print("K-splaying step on the following nodes:")
    for node in stack:
      print("- ", node)

    suffix_length = cls.K
    if len(stack[-1].children) >= cls.K:
      suffix_length += 1
    if len(stack[-1].children) >= cls.K+1:
      suffix_length += 1

    # Handles both terminal and nonterminal cases.
    keys, children = cls.flatten(stack[-suffix_length:])
    stack[-suffix_length:] = [cls.compose(keys, children)]
    return stack

  @classmethod
  def ksplay(cls, stack):
    """K-splays together the stack and returns the new root."""
    if len(stack) == 1:
      stack = cls.ksplay_step(stack)
    else:
      while len(stack) > 1:
        stack = cls.ksplay_step(stack)
    print("New root:", stack[0])
    return stack[0]

  def insert(self, key):
    stack = self.walk_to(key)
    target_node = stack[-1]
    print("Insert", key, "into", target_node)
    target_node.insert(key)
    print("Done:", target_node)
    self.root = self.ksplay(stack)

  def delete(self, key):
    raise Exception('TODO: delete')

  def contains(self, key):
    stack = self.walk_to(key)
    target_node = stack[-1]
    result = target_node.contains(key)
    self.root = self.ksplay(stack)
    return result
