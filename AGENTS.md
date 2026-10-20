# AGENTS.md — raikv

Rai Key Value: the base library of the fleet. Shared-memory hash table
(`HashTab`, sysv/posix/heap maps), the event loop (`EvPoll`, `EvSocket`,
`EvConnection`, timers, TCP/UDP/unix/dgram), the pub/sub route table
(`RouteDB`, `RouteVec`, `RoutePublish`), bloom filters, bit sets, delta
coders, a logger, and the `KeyCtx` API every data service builds on.
Namespace `rai::kv`. No sibling dependencies (pcre2, c-ares, systemd, zlib
from the distro).

## Workspace layout (injinj fleet convention)

- This repo is one of the `injinj` siblings. Every dependency is a sibling
  checkout (none for this repo); headers come from
  `-I../<dep>/include` and libraries from
  `../<dep>/FC43_x86_64/lib64/lib<dep>.a`. There is no install step between
  siblings — build the dependency, then build this repo.
- Dependencies are declared once, in `build_depends.mak`
  (`<dep>_dep := MAJOR.MINOR`, the rpm BuildRequires minimum). `./deps.sh`
  clones missing siblings from it, `./deps.sh -b` also builds them in
  dependency order, `./deps.sh -s` prints the commit of this repo and every
  sibling (the only record of what a release was built from).
- Whole-fleet build order lives in `~/injinj/GNUmakefile` (`make build`):
  openpgm → raikv → libdecnumber → raimd → linecook → lzf → rdbparser → h3 →
  HdrHistogram_c → sassrv → raids → natsmd → omm → raims → rvcache →
  raimdapi → ossl.

## Build

- `make` → `FC43_x86_64/bin`, `FC43_x86_64/lib64` (static `.a` + versioned
  `.so`). The build dir is `$(short_dist)$(lsb_dist_ver)_$(uname_m)$(port_extra)`:
  Fedora 43 → `FC43_x86_64`, Rocky 9.5 → `ROCKY9.5_x86_64`.
- `make port_extra=-g` → debug build in `FC43_x86_64-g`.
  `make port_extra=-mingw` → Windows cross build in `FC43_x86_64-mingw`
  (mingw64 toolchain; the mingw build links sibling *master* checkouts).
- `NO_STL=1` builds with `-fno-rtti -fno-exceptions` and links with the C
  compiler; the library code is written to survive that (error returns, no
  exceptions, no RTTI, no STL containers in public headers). Keep it that way.
- `make -j8` is safe; the makefile dependencies are complete (verified clean
  and incremental). One trap: the **first `-j` run right after a compile
  error** can fail test executables with bogus undefined references
  (symbols from this repo's own lib) because they link against a library
  left half-updated by the failed run. Run plain `make` once, then `-j`
  works again. It is not a missing dependency — don't go looking for one.
- Warnings are clean. A new warning is a defect.
- If a sibling's public headers changed, rebuild the sibling first. A link
  warning like `lib<dep>.so.1.N, needed by ../<x>/.../lib<x>.so, not found`
  means sibling `<x>` was built against an older `<dep>` — rebuild `<x>`;
  the warning becomes a hard failure at run time.
- Ad hoc test program without touching the GNUmakefile:
  `g++ -std=c++11 -O1 -g -Wall -Wextra -Iinclude -I../<dep>/include -o /tmp/x
  test/x.cpp -LFC43_x86_64/lib64 -lraikv -L../<dep>/FC43_x86_64/lib64 -l<dep> …`
  then run with `LD_LIBRARY_PATH` pointing at each `lib64` (the link picks
  the `.so`). For repos with many siblings, capture the makefile's own
  lines once: `touch src/<any>.cpp; make -n | grep -- '-c src/<any>.cpp'`
  and `make -n | grep -- '-o FC43_x86_64/bin/' | head -1`, then reuse the
  compile line (minus `-c/-o`) and the link line's `-Wl,-rpath` + lib tail.

## Versioning and release

- Version is `major_num.minor_num.patch_num-build_num` in `.copr/Makefile`
  (included by the GNUmakefile). `make spec_version` prints the
  `raikv_dep := MAJOR.MINOR` line that dependents pin in their
  `build_depends.mak`.
- `rpm/raikv.spec` carries placeholders (`Version: 999.999`, release
  `99999`, `BuildRequires: <dep> _<dep>_dep`); `make srpm` substitutes them
  from `build_depends.mak` and tars `git ls-tree HEAD` into
  `rpmbuild/SOURCES`. Keep the `_ensure_git` line in `.copr/Makefile`:
  COPR's `make_srpm` chroot has no git and every `$(shell git …)` would
  silently expand empty.
- COPR: `injinj/dev` builds the `dev` branch of github.com/injinj/raikv
  on push (webhook); `injinj/rel` builds github.com/raitechnology/raikv.
  Remotes: `origin` = raitechnology (release), `ii` = injinj (dev).
  Chroots: epel-7/8/9/10, fedora-43/44/45/rawhide, x86_64.
- ABI convention across the fleet: a dependent's rpm `Requires` is
  `MAJOR.MINOR` and its `BuildRequires` is the full version; the MINOR is
  bumped together with any change that alters the C++ ABI of a public
  header, so a rebuilt dependent never silently loads an old `.so`.
  Bump `minor_num` here when public headers change, then re-pin and
  rebuild the dependents.
- Link only against installable sibling libraries (`$(lnk_lib)`), never a
  sibling's object files; if a binary needs a sibling helper, move the
  source into the sibling's library and release the sibling first.
- Signed rpms are published to https://www.raitechnology.com/repo/
  (COPR rel → download → sign → createrepo → rsync; tooling in
  `~/injinj/rel`).

## Code conventions

- C++11, two-space indent, `/* … */` block comments, aligned member
  comments; snake_case functions/variables, CamelCase types.
- Namespaces: `rai::<repo-namespace>` (see below). System headers first,
  then project headers.
- Errors are return codes / status enums; no exceptions, no RTTI in library
  code. Raw pointers and placement-new over allocators are the norm; look
  at how the neighbouring code allocates before adding `new`/`delete`.
- Keep the generated `CMakeLists.txt` out of your edits — it is derived from
  the GNUmakefile (and slated for removal).

## Layout

- `include/raikv/` — public headers. Entry points: `shm_ht.h` (map
  open/create), `key_ctx.h` (`KeyCtx`: acquire/find/value/resize/release),
  `ev_net.h` (`EvPoll`, `EvSocket`, `EvConnection`), `ev_publish.h`
  (`EvPublish`, the message envelope every transport forwards),
  `route_db.h`/`route_ht.h` (subject routing), `bloom.h`, `work.h`
  (`WorkAllocT`), `key_hash.h`, `util.h`.
- `src/` — implementation, one `.cpp` per header family.
- `test/` — one executable per feature (`test_routes`, `test_bloom`,
  `test_timer`, `test_tcp`, …); `kv_test`, `hash_test`, `ctest`,
  `pq_test`, `zipf_test` are benchmarks/stress tools.
- `bin`: `kv_server`, `kv_cli`, `ping`, `pubsub`, `load`.

## Running the shm map

- `FC43_x86_64/bin/kv_server` — default map `sysv:raikv.shm` (2 GB sysv
  segment; persists after exit, `ipcs -m` to inspect).
- **Bare `kv_server` deletes the existing map and creates a fresh one.**
  Existing attachers keep the deleted (`dest`) segment; new attaches get
  the new one. Never run it bare against a map holding data you care about.
- `kv_server -a` attaches to the existing map (the non-destructive mode you
  normally want). Its active duty is small — it clears locks left by a
  process that died holding an entry — but keeping one attached beside
  the data services is cheap insurance.
- Services take `-m sysv:raikv.shm`; no `-m` (or `-m none`) means heap
  mode (`EvShm::open(NULL)` returns 0 with `map == NULL`).

## Contracts other repos depend on

- `KeyCtx` op sequence: `EvKeyCtx` placement-new with variable key buffer
  (`EvKeyCtx::size(klen)`), `ctx->set(kctx)` primes a `KeyCtx(map, dbx_id)`;
  then `acquire(&wrk)` (result `<= KEY_IS_NEW` is success, `KEY_OK` =
  already existed) → `value(&p,&sz)` / `resize(&p,sz)` / `tombstone()` →
  `release()`. Read-only: `find(&wrk)` then `value()` (copied). Seed with
  `map->hdr.get_hash_seed(kctx->db_num, hseed)`. Work memory is a
  `WorkAllocT<1024>`, `wrk.reset()` per op.
- The hash entry type byte (`kctx.set_type()/get_type()`) is shared
  across the fleet: raids stores its redis types there, rvcache/raimd store
  the message codec's `ftype` (RVMSG 0xbe, RWF field list 0xca, …). Don't
  repurpose it.
- `EvPublish.msg_enc` is a 32-bit codec id, not the type byte; a bare byte
  falls through every transport's switch to "opaque".
- `EvPublish.hdr_len` delimits a transport header in front of the payload
  (omm uses it for the RWF message header). Consumers must honour it.
- Route lookups are hash-table resolved at publish time; a static call
  graph of `EvPoll` tells you nothing about who receives a message —
  read `RouteDB` and the transport's `on_msg` instead.

## Tests

- No single test runner. Build, then run the `test_*` executables you
  touched; they are self-checking and print a pass/fail summary.
  <!-- TODO: list which test_* cover which header, and any that need a map -->

## Open items / history

<!-- TODO: fill from git log and the fleet daily notes -->
