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

Results are saved in `src/experiments/performance/results.tsv`. You can plot
some interesting graphs by running:
```bash
cd experiments/performance
gnuplot plot_results.gnuplot
```
