# Overview
This project is an amalgamation of several experiments with hyper-dimensional computing and byte-pair encoding. The tool works by first encoding an input file, computing a hyper-dimensional profile vector based on each trigram from the encoded data, then, given two (user-input) symbols, predict the next 100. Depending on the input text, the output can be quite entertaining.

For example, the lyrics to the song "One Beer" by MF DOOM...
```sh
$ ./hdc-test-v3 one-beer.txt
[INFO] encoded data from 'one-beer.txt'
[INFO]   original size: 3466
[INFO]   reduced size: 1344
[INFO]   byte-pair table size: 619
[INFO] computed profile vector
[INFO] enter table indices:
415,346
[INFO] keeis es).em'em <ld thon'eminormeHe You're emumbIs and  chapMsome on myust but ought overtroy a's the ly diebod-maboies.Nowlike verOAnd asHowi and , herleadive er Ts aX!).'re Wly ("your tought  byourshnost .(Youjust  to ? @Ty.`ou li!
```

Did I mention it's giberish?

## Resources
- [hdc-test-v0](https://github.com/luke-holt/hdc-experiment)
- [Byte Pair Encoding](https://github.com/luke-holt/byte-pair-encoding)

