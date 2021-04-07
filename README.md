# Legio - Web-compatible Trackerless P2P Network

Legio is a trackerless peer-to-peer network for distributed applications with nodes running both on native platforms and in web browsers. Technically, Legio is a WebRTC-based peer-to-peer network with distributed signaling. It is licensed under LGPLv2.

> "**Legio** nomen mihi est, quia multi sumus."
>
> "My name is Legion, for we are many."
>
> -- Gospel according to Mark, 5:9

## Dependencies

Only [GnuTLS](https://www.gnutls.org/) or [OpenSSL](https://www.openssl.org/) is necessary.

## Building

### Clone repository and submodules

```bash
$ git clone https://github.com/paullouisageneau/legio.git
$ cd legio
$ git submodule update --init --recursive
```

### Build for Native

```bash
$ cmake -B build
$ cd build
$ make -j2
```

### Build for WebAssembly with Emscripten

```bash
$ cmake -B build-emscripten -DCMAKE_TOOLCHAIN_FILE=$EMSDK/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake
$ cd build-emscripten
$ make -j2
```

