# libgdb

Utility library for implementing the [GDB protocol][gdb protocol].

## Design

Unfortunately, the GDB protocol doesn't use delimiters for protocol
messages. For this reason, a state machine is required to read packets
from a stream. This library implements such a state machine using a
parser combinator library. This state machine only returns a generic
package structure and doesn't perform any further canonicalization.

Since proper canonicalization of GDB packets is desirable this utility
library provides two parser stages. The first stage is the
aforementioned state machine, the second parser stages performs packet
specific validations and uses the most restrictive input definition on a
per packet basis.

This library attempts to follow some of the [langsec][langsec website]
principles outlined in [\[1\]][curing the vulnerable parsers].
Unfortunately, the second stage parser is unfinished at the moment and
has a fallback for accepting arbitrary inputs currently, thereby
circumventing the "most restrictive input definition".

Additionally, some utility functions for creating responses to received
GDB packets are provided by `libgdb/response.h`. However, this part of
the library is in very early stages of development.

[gdb protocol]: https://sourceware.org/gdb/onlinedocs/gdb/Remote-Protocol.html
[langsec website]: http://langsec.org/
[curing the vulnerable parsers]: https://www.usenix.org/publications/login/spring2017/bratus
