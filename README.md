# Rep

Repeat input files.

## Installation

```sh
$ make
$ sudo make install
```

## Usage

```sh
# repeat standard input (by default 10 times)
$ echo hello world | rep

# repeat input file 5 times
$ rep -n 5 file

# repeat concatenation of input files
$ echo hello world | rep file_1 - file_2
```
