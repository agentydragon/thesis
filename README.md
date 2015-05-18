# Bachelor thesis
## Practical data structures

<a href="https://travis-ci.org/MichalPokorny/thesis">
![Build status](https://travis-ci.org/MichalPokorny/thesis.svg)
</a> (Travis CI: [https://travis-ci.org/MichalPokorny/thesis](https://travis-ci.org/MichalPokorny/thesis))

This is my Charles University CS bachelor thesis.
Feel free to grab what you like, but don't sell it and keep attribution
(legalese: http://creativecommons.org/licenses/by-nc-sa/2.0/legalcode,
 humanese: http://creativecommons.org/licenses/by-nc-sa/2.0/).

`text/` contains the text of the thesis. `src/` contains associated source
code in C.

I am developing this project on Linux with GCC. It may work somewhere else,
but only accidentally.

## Dependencies
The code requires [LibUCW 6.4](http://www.ucw.cz/libucw/) to build (I use its
red-black trees). My code assumes that it's installed under the `/usr` prefix.

For benchmarking, you additionally need:
* [jansson](http://www.digip.org/jansson/) for JSON output
* [matplotlib](http://matplotlib.org/) for plotting graphs

`experiments/btree-dot` needs:
* [graphviz](http://www.graphviz.org/) for rendering graphs

`experiments/cloud` needs:
* [urllib3](https://urllib3.readthedocs.org/) for fetching the dataset

## Running tests
```bash
make test
# Or:
make
bin/test
```

## Measuring test coverage
You need GCOV and LCOV.
```bash
make test_coverage
# browse src/coverage-out/index.html
```

## Benchmarking
`src/experiments/performance` contains a "script" that runs several operation
sequences against dictionary implementations. Use it like this:
```bash
make bin/experiments/performance

# To run all benchmarks:
bin/experiments/performance
```

Results are saved in `src/experiments/performance/results.json`. You can plot
some interesting graphs by running:
```bash
cd experiments/performance
./plot_results.py
```
