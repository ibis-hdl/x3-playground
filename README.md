# x3-playground

## Spirit X3 playground for compiler grammar

Here is the test code from 2022, with std::from_chars() - where libc++ until today 2025
has no floating points support, see [Godbolt](https://godbolt.org/z/ceqGondnE).
A libc++ port was written for this.

The first experiments with Boost::LEAF are also included here.

In addition, the literal parsers are implemented as parser functions.

All too much at once to integrate everything in the Code-Review-2024 branch!

Many good approaches that should be pursued further!

## also

- https://stackoverflow.com/questions/49722452/combining-rules-at-runtime-and-returning-rules/49722855#49722855
- https://stackoverflow.com/questions/71281614/how-do-i-get-which-to-work-correctly-in-boost-spirit-x3-expectation-failure
