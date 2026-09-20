# Threat model (0.1)

## Protected assets

- user documents outside the explicitly selected input;
- user credentials, registry data, other processes and network access;
- launcher availability and integrity;
- local settings, recovery files and logs.

## Untrusted inputs

All opened documents, ZIP metadata, IPC bytes and command-line paths are untrusted. Document type
is determined by extension for routing only; the worker independently checks OOXML/ODF ZIP shape.

## Implemented controls

- one input copy per random directory;
- canonical path containment check;
- same-user local IPC and 256-bit session token;
- fixed 1 MiB IPC frame limit and typed JSON validation;
- 256 MiB input, 1 GiB expanded metadata, 10,000 entry, XML/image/entry size,
  path-depth and 200:1 ratio defaults;
- macro, embedded executable, traversal, OLE/embedding and external-link-part findings;
- one-shot worker, 15-second client timeout, 20-second worker lifetime;
- Windows Job Object: kill-on-close, one active process, 512 MiB process memory;
- no shell execution and no networking/update implementation.

## Not yet mitigated

- Windows token capabilities and network ACLs (AppContainer/restricted token);
- CPU quota and hard wall-clock termination while synchronous scanner code is executing;
- nested archives, actual XML token/depth limits and relationship `TargetMode=External` parsing;
- image pixel/dimension and engine allocation limits;
- PDF parsing/rendering in a dedicated worker;
- signed binaries, signed updates and installer hardening;
- continuous fuzzing of IPC, ZIP metadata and adapters.

These gaps prevent Vestra 0.1 from being advertised as a hardened sandbox.

