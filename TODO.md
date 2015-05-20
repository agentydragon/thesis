## TODO
This file contains ideas for future improvements of this work.

* Report space usage
* More real data
* knihovni cast, neobecna (explicitne) VS experimenty nad tim napsane
* `Update` operation ("`Find` that returns a pointer")
* What happens on other key/value sizes? What happens on other pointer sizes?
  (e.g. 32b)
* Compare with libdbm?

# Implement:
- Van Emde-Boas / x/y-fast trie
- B-star trees
- Judy array?
- vetsi hodnoty? parametricke na velikosti hodnot?
- FKS hashing
- Play around with hash table load factor/query time tradeoff
- multithreading
- on-disk structures (much more fun)
- scrapegoat tree, AVL tree
- cache-oblivious priority queue
- cache-line-aligned COBT? (neco jako 8-cestny implicitni strom)
- vector ops
- custom allocator (faster + more aligned than malloc)
- SIMD searching in B-trees
- Binary search in B-trees
- Find/FindNext/FindPrev with finger (should be much faster)
- PMA should reallocate, not allocate + free
- cache-sensitive b+ trees (increase tree fanout by eliminating pointers)
- pearson correlation coefficient (for counter-time correlation)

# Ideas:
- Cache-oblivious B-trees are about 4-competitive, something log(e)-optimal
  exists. Find out about that.

# Integration ideas:
* Memcache?
* Python/Ruby built-in hashes?
* Linux kernel trees (unfortunately very intrusive)

# Read:
* Jeffrey Scott Vitter - External Memory Algorithms and Data
* Rasmus Pagh - Optimality in External Memory Hashing
* http://www.cs.utexas.edu/~pingali/CS395T/2013fa/lectures/MemoryOptimizations_2013.pdf
* self-organizing heuristics for implicit data structures
* implicit data structures for weighted elements
* CSB+ trees (http://www.cs.columbia.edu/~kar/software/csb+/readme)
* http://pizzachili.dcc.uchile.cl/texts/nlang/

# Datasets:
* Enron e-mails dataset
* U.S. census
* aws.amazon.com/public-data-sets
* wikipedia snapshot
* Indiana university Click dataset (HTTP requests)
  (given a website, find next click on that website; key = `timestamp;website hash`)
* DIMACS implementation challenge 5 (dictionaries):
  http://www.cs.amherst.edu/~ccm/challenge5/
