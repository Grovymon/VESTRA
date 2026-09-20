# Security policy

## Reporting a vulnerability

Do not publish exploitable document samples in a public issue. Open a private GitHub security
advisory for the repository owner and include the affected revision, Windows version, reproduction
steps and a minimally necessary sample. Do not include unrelated personal documents or logs.

Until a private reporting address is configured, repository owners should enable GitHub Private
Vulnerability Reporting before accepting external users.

## 0.1 security posture

- Macros are never executed. Macro-capable formats and `vbaProject.bin` force Protected View.
- The worker checks file size, OOXML ZIP structure, central-directory bounds, entry count,
  decompressed sizes, compression ratio, traversal names, embedded executables, macro parts,
  embedded/OLE objects and external-link parts before accepting a document.
- The launcher survives worker rejection, timeout, failure or crash.
- IPC is local, same-user, one-client, length-prefixed, size-limited, JSON-validated and protected
  with a per-session random token.
- Workers receive a temporary copy instead of the original user path and are terminated after one
  request. No `system()`, PowerShell, CMD or child-program launch exists in worker code.
- Logs remain local and are never submitted automatically. Do not log document contents or tokens.
- No networking code is present in 0.1. Update consent defaults to off.

## Known boundary gaps

The Windows Job Object limits lifetime, process count and memory, but does not remove the worker's
user token privileges or create network/file-system ACLs. Treat the current worker as crash and
resource isolation, not a complete malware sandbox. AppContainer/restricted-token launch and a
dedicated PDF render worker are release blockers for 1.0.

The ZIP preflight reads metadata only and does not inflate XML. A future worker stage must add
streaming XML limits, relationship-target parsing, nested container depth, image dimension limits,
CPU deadlines and engine-specific fuzz/regression corpora before enabling general office editing.

## Supported versions

Only the newest tagged release receives security fixes. There are no stable releases yet.


