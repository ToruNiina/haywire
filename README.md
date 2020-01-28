# Haywire

An implementation of [Wireworld](https://en.wikipedia.org/wiki/Wireworld).

## Usage

```console
$ ./haywire [saved_data.toml (optional)]
```

- `Space`: toggle execution
- `Enter`: step-by-step execution
- `click`: turn cell stete empty -> conductor -> head -> tail
- `drag`: move cells relative to the window
- `Ctrl-S`: save status into a file

## Build

It depends on [SDL2](https://www.libsdl.org/).

```console
$ cmake ..
$ make
```

## Licensing terms

This product is licensed under the terms of the MIT License.

- Copyright (c) 2020 Toru Niina

All rights reserved.
