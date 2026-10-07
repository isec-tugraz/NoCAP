# NoCAP: Practically Defending Page Cache Attacks

This repository contains the source code, kernel patches, and evaluation scripts
for the paper introducing NoCAP, a kernel-level defense against page cache
attacks. Our paper was accepted at NDSS 2027, Seoul, Republic of Korea.

Our artifacts have been evaluated to be Available and Functional.

### Table Of Contents
- [ Introduction ](#introduction)
    - [ Foreword ](#foreword)
- [ Requirements ](#requirements)
- [ Downloading the VM ](#downloading-the-vm)
- [ Structure of the Artifact ](#structure-of-the-artifact)
    - [ Patches ](#patches)
    - [ Primitives ](#primitives)
    - [ Flush Calibration ](#flush-calibration)
    - [ Root and User Scripts ](#root-and-user-scripts)
- [ Patches Structure ](#patches-structure)
- [ Evaluation Workflow ](#evaluation-workflow)
- [ Notes ](#notes)

## Introduction
Recently demonstrated page cache attacks (Neela *et al.*, NDSS 2026) rely on
four primitives: (i) flush, which removes a page from the page cache, (ii)
reload, which measures the time it takes to bring a page back into the page
cache, (iii) monitor, a mechanism that reports the page cache state without
modifying the state, and (iv) evict, a general technique of applying memory
pressure to remove pages from cache. All are unprivileged, and flush & reload
leak whether a page was resident in the page cache before the operation, through
timing alone.

NoCAP mitigates both flush & reload primitives at the kernel level without
removing their functionality, following two principles: preserve page cache
semantics, and remove observability rather than functionality. Flush is
mitigated with an adaptive delay, so the time taken to flush a page no longer
depends on whether it was in cache. Reload is mitigated with a first time miss
principle, where every process encountering a page for the first time
experiences it as if it came from disk, regardless of whether the page was
already resident from another process's access.

This artifact demonstrates that NoCAP removes the timing leakage from both
primitives while leaving their functional behavior unchanged: flush still
removes the page, and a re-read still succeeds. The comparison is done between
two kernel builds of the same Linux 6.18.2 base: an unpatched (stock) baseline,
and the same kernel with the NoCAP patchset applied.

Some of our testing code is borrowed from the artifact by Neela *et al.*:
https://zenodo.org/records/17915256

### Foreword
Our experiments and benchmarks in the paper were run natively on the host
kernel, with the mitigated kernel active directly on hardware, not inside a VM.
The experimental machines (Table I in the paper) were daily-used systems with
multiple physical cores, running normal background processes and systemd
services. All results in the paper were collected on Ubuntu systems, with one
exception on Fedora.

The VM supplied with this artifact was not used to produce the paper's results
and is not intended to reproduce their exact magnitude. It demonstrates only
that our prototype functions correctly, *i.e.*, that a timing gap between flush
and reload on cached versus uncached pages is present on the baseline kernel and
absent on the nocap kernel. We chose Void Linux for the VM because it is more
minimal than a full Ubuntu qcow2 disk image. The VM also has fewer vCPUs than
our experimental machines, and Void Linux uses runit instead of systemd, both of
which could affect scheduling & background load and can shift absolute timing
values. None of this bears on the claim under test here, which is a difference
in kind (leak present versus absent), not a difference in the specific numbers
reported in the paper. Reviewers should expect the qualitative result to
function. The exact timings may differ due to the different distribution,
underlying hardware, and the virtualized environment. 

## Requirements
The host machine should run a Linux distribution with QEMU (version 7.0.0 or
later, verifiable with `qemu-system-x86_64 --version`) and KVM acceleration
available. At least 4 GiB of free memory is recommended for the VM. The command:
`echo 3 | sudo tee /proc/sys/vm/drop_caches` can drop the host's entire page
cache, "freeing" up memory.

The kernel patches themselves target Linux v6.18.2 and were built and tested on
x86_64. Rebuilding the kernel is only necessary if you want to verify the
patchset independently of the prebuilt VM images, and requires the usual kernel
build dependencies.

The flush calibration script (`flush-calibration/parameter-determination/`) is a
Python program that fits a line to timing measurements and depends on `cvxopt`,
`numpy`, and `pandas` (listed in that directory's `requirements.txt`). The VM
ships with a prebuilt virtualenv for this.

## Downloading the VM
Download the VM and supporting files from the `NoCAP-VM.tar` tarball. Extract it
with `tar -xf NoCAP-VM.tar`. The tarball unpacks into a `NoCAP-VM/` subfolder
containing:
- QEMU `nocap-vm.qcow2` disk image,
- the kernels (`nocap-kernels/`),
- and the `run-vm.sh` script.

All three of them (image, kernel folder, and run script) are already together in
`NoCAP-VM/`; `cd NoCAP-VM/` before running the commands below.

## Structure of the Artifact
Apart from the `NoCAP-VM.tar`, all the source code and supplementary material
are in `sources.tar`. Extract it with `tar -xf sources.tar`; it unpacks into a
`sources/` subfolder that holds the paths described below.


### Patches
`patches/` contains the NoCAP kernel patchset as a series of numbered patch
files, plus the two kernel defconfigs used to build the baseline and nocap
kernels (`defconfig_baseline`, `defconfig_nocap`). See [Patches
Structure](#patches-structure) below for a description of what each patch does
and where NoCAP's implementation lives in the kernel tree.

### Primitives
`primitives/` contains the three userspace programs used to run and observe the
interaction of userspace syscalls with the page cache during evaluation:

- `flush/flush.c`: removes pages from the page cache with
  `posix_fadvise(POSIX_FADV_DONTNEED)` and reports the time each flush took.
- `reload/read-bypass-backward-reading.c`: reads a file back to front, one byte
  per page, and reports the time each read took.
- `monitor/cachestat.c`: reports whether each page of a file is currently
  resident in the page cache, using the `cachestat` syscall. Requires
  same-user access to the file.

Run `make` in `primitives/` to build all three into `primitives/build/`. All
three take a file path as their first argument and an optional list of page
numbers.

### Flush Calibration
`flush-calibration/parameter-determination/` contains `flush-repeat.c`, a
program that repeatedly flushes a growing prefix of a file while timing each
flush with `rdtsc`, and `analyse.py`, which fits a line (slope and intercept) to
flush time as a function of page count using a constrained least squares fit
(via `cvxopt`). The fitted line is later used to configure NoCAP's adaptive
flush delay so that it tracks the baseline kernel's own flush timing.

`flush-calibration/utils/` holds the small helper library (`rdtsc` wrapper, MSR
access, physical address lookups) that `flush-repeat.c` links against.

### Root and User Scripts
Two scripts run as `root` on both kernels before evaluation:

- `disable-readahead.sh`: sets `read_ahead_kb` to 0 on the root block device.
  Read-ahead would otherwise prefetch neighboring pages and contaminate the
  per-page timings taken by the primitives.
- `apply-sysctl-params.sh`: applies the slope and intercept fitted by
  `flush-calibration.sh` (run once on the baseline kernel) to NoCAP's adaptive
  flush delay, via `/proc/sys/vm/dontneed_wait_factor_centicycles` and
  `/proc/sys/vm/dontneed_wait_constant_centicycles`. Only meaningful on the
  nocap kernel.

The primitives in `primitives/` are compiled and run as `user`, matching the
cross privilege boundary the paper's threat model assumes for the flush and
reload primitives (both are unprivileged).

## Patches Structure
We describe the patch layout and where each part of the implementation lives in
the kernel tree.

### Kernel configurations
Although the configuration we provide are based on the stock Ubuntu 24.04 LTS
defconfig, the VM itself runs void linux and was configured with `make
x86_64_defconfig`. The config we provide in this repository was used for our
experiments in the paper.

These changes were applied to the base defconfig for our experimental setup:
 - Transparent hugepages are disabled
 - The local version is set to "-baseline" or "-nocap" to distinguish between
   both kernel builds

The nocap kernel defconfig has the following changes:
- Page cache isolation is enabled: `CONFIG_PAGE_CACHE_ISOLATION=y`
- The dynamic flush delay is enabled:
  `CONFIG_PAGE_CACHE_ISOLATION_DELAYED_FADV_DONTNEED=y`
- Readahead mimick readaheads are enabled:
  `CONFIG_PAGE_CACHE_ISOLATION_MIMIC_IO=y`
- Extending the localversion with VCS information: `CONFIG_LOCALVERSION_AUTO=y`

### Patchset
To easily view and inspect the patches in an interactive GUI manner, we
recommend KDE's Kompare. On systems without KDE, installing Kompare might
require installing numerous additional KDE libraries. Otherwise, command line
tools or even VS Code with its syntax highlighting is sufficient to inspect the
kernel patches.

The patchset is based upon the `v6.18.2` tag from linux-stable:
https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git/tag/?h=v6.18.2

The patches are roughly thematically split up. The included commit messages
roughly justify changes or describe problems encountered during the
implementation. Furthermore, in the files listed below, some comments are marked
with `NOCAP!: ` as comments of special interest. They highlight locations in the
patchset or general kernel code that are of special interest for NoCAP.

NoCAP mostly resides in `mm/filemap.c` as the implementation of the page cache.
For handling the resource cleanup, changes were also necessary at the inode,
fork, and readahead code.

- linux_nocap
  - include/linux
    - fs.h: adds the prototype for `fadvice_init`.
    - mm.h: extends the address_space with the access bit maple tree container.
    - mm_types.h: adds the filemap token to the mm_struct.
    - pagemap.h: contains the prototypes for the functions provided by NoCAP.
  - fs
    - inode.c: adds code for initializing and destroying the access bit maple
      tree.
  - init
    - main.c: adds a call to `fadvice_init` in `start_kernel`, to populate the
      flush delay timing sysctl files.
  - kernel
    - fork.c: assigns a filemap token to distinguish between mm_struct instances
      that were recycled through the kmem_cache, so that the existing bitfield
      can be cleared out and the new owner of the mm_struct cannot inherit the
      bitfield state from the old owner.
  - lib
    - iov_iter.c: sets the seen bit for folios written into the page cache.
  - mm
    - Kconfig: adds the `CONFIG_PAGE_CACHE_ISOLATION_*` options to the kernel
      config.
    - fadvise.c: implementation of the dynamic flush delay, to mitigate
      flush+flush. The sysctl files for the flush delay timing are instantiated
      here.
    - filemap.c: implementation of the per-mm_struct page cache isolation.
      Contains functions for obtaining the first-time-seen bitmask, resetting
      the bitmask when the mm_struct was recycled, setting bits or querying bits
      for whether folios were seen, and performing the mimicking synchronous
      readahead operations. When folios are discarded from the page cache (e.g.
      by `mm/truncate.c`), the bit is also cleared here for all mm_struct. For
      memory mapped files, mimicking synchronous readaheads is also implemented
      here. For completeness, the async readahead code that is currently
      crashing is also contained, including memory mapped file readaround.
    - readahead.c: adds a function for performing the mimicking readaheads that
      get called when pages are in the page cache but have not yet been seen by
      this mm_struct. The results of the mimicking readahead are discarded.

## Evaluation Workflow
This mirrors the high level workflow in the paper's artifact appendix with more
detail. Two passes are required: one on the baseline kernel to establish ground
truth and calibrate the nocap kernel's flush delay, and one on the nocap kernel
to confirm the timing leakage is gone.

These following steps assume that the VM, kernels, and run script has been
downloaded and set up (see [above](#downloading-the-vm)).

### Pass 1: baseline
1. Launch the VM with the baseline kernel and log in as `root` (password
   `root`). Either the QEMU GUI window or the terminal console can be used.
   ```sh
   # As `host`
   ./run-vm.sh baseline
   ```
2. After logging in, verify the kernel with `uname -r`. It should report
   `6.18.2-baseline-g711e5cbac8cc`.
3. Run the flush calibration script. Depending on hardware, this can take 10 to
   30 minutes. This builds `flush-repeat`, times how long it takes to flush
   varying pages of a testing file, and regresses a line to the result:
   ```sh
   # As `root`
   cd flush-calibration/parameter-determination/
   ./flush-calibration.sh
   ```
   This writes `data/<hostname>.csv` (raw samples) and
   `data/<hostname>-sysctl-params.csv` (the fitted slope and intercept), which
   are needed in pass 2.
4. Disable the asyncrhonous read-ahead mechanism
  ```sh
  # As `root`
  cd ~
  ./disable-readahead.sh
  ```
5. Log out as `root` with `exit`, log in as `user` (password `user`).
6. Create a testing file using the script, and then build the primitives:
   ```bash
   # As `user`
   cd ~
   ./create-testing-file.sh # creates /tmp/nocap-testing-file.bin
   
   cd ~/primitives
   make
   ```
7. Now, test that a boiled-down version of page cache attacks works (see
  [notes](#notes) below).
  - First, we test that flushing a page in cache versus a page **not** in cache
    has a timing difference. We expect the page in cache to take **longer** to
    flush.
    ```sh
    # As `user`
    cd ~/primitives

    # Reload the file -> pages are definitely in cache
    ./build/read-bypass-backward-reading /tmp/nocap-testing-file.bin

    # Use cachestat to determine whether the pages are really in cache
    ./build/monitor-cachestat /tmp/nocap-testing-file.bin

    # Flush the cached file -- Note the timings! (the first page can be a bit off)
    ./build/flush /tmp/nocap-testing-file.bin

    # Use cachestat to determine whether the pages are no longer in cache
    ./build/monitor-cachestat /tmp/nocap-testing-file.bin

    # Flush the non-cached file -- Note the timings! (the first page can be a bit off)
    ./build/flush /tmp/nocap-testing-file.bin
    ```
    Flushing pages that was in cache should take noticeably longer than flushing
    pages that were not. Repeat this step (reload, monitor, flush, monitor,
    flush) multiple times to see the general trend.

  - Second, we test that reloading a page in cache versus a page **not** in
    cache has a timing difference. We expect the page in cache to take **less
    time** to reload.
    ```sh
    # As `user`
    cd ~/primitives

    # Flush the file -> pages are definitely not in cache
    ./build/flush /tmp/nocap-testing-file.bin

    # Use cachestat to determine whether the pages are really not in cache
    ./build/monitor-cachestat /tmp/nocap-testing-file.bin

    # Reload the non-cached file -- Note the timings! (the first page can be a bit off)
    ./build/read-bypass-backward-reading /tmp/nocap-testing-file.bin

    # Use cachestat to determine whether the pages are cached
    ./build/monitor-cachestat /tmp/nocap-testing-file.bin

    # Reload the cached file -- Note the timings! (the first page can be a bit off)
    ./build/read-bypass-backward-reading /tmp/nocap-testing-file.bin
    ```
    Reloading pages that were in cache should be much faster than reloading
    pages that were not. Repeat this step (flush, monitor, reload, monitor,
    reload) multiple times to see the general trend.

8. Once asserted, log out as `user`, log in as `root`, then `poweroff`.


### Pass 2: NoCAP
1. Launch the VM with the nocap kernel and log in as `root`.  Either the QEMU GUI window or the terminal console can be used.
   ```sh
   # As `host`
   ./run-vm.sh nocap
   ```
2. After logging in, verify the kernel with `uname -r`. It should report
   `6.18.2-nocap-g711e5cbac8cc`.
3. Apply the sysctl parameters calibrated in pass 1 to the mitigation's adaptive
   flush delay:
   ```sh
   # as `root`
   cd ~
   ./apply-sysctl-params.sh
   ```
4. Disable the asyncrhonous read-ahead mechanism:
  ```sh
  # As `root`
  cd ~
  ./disable-readahead.sh
  ```
5. Log out as `root` with `exit`, log in as `user` (password `user`).

6. Ensure the testing file still exists: `ls -l /tmp/`.

7. Now, test that a boiled-down version of page cache attacks no longer works.
  - First, we test that flushing a page in cache versus a page **not** in cache
    **no longer** has a timing difference. We expect the page in cache to take
    similar time to flush.
    ```sh
    # As `user`
    cd ~/primitives

    # Reload the file -> pages are definitely in cache
    ./build/read-bypass-backward-reading /tmp/nocap-testing-file.bin

    # Use cachestat to determine whether the pages are really in cache
    ./build/monitor-cachestat /tmp/nocap-testing-file.bin

    # Flush the cached file -- Note the timings! (the first page can be a bit off)
    ./build/flush /tmp/nocap-testing-file.bin

    # Use cachestat to determine whether the pages are no longer in cache
    ./build/monitor-cachestat /tmp/nocap-testing-file.bin

    # Flush the non-cached file -- Note the timings! (the first page can be a bit off)
    ./build/flush /tmp/nocap-testing-file.bin
    ```
    The timings should be roughly similar. Repeat this step (reload, monitor,
    flush, monitor, flush) multiple times to see the general trend.

  - Second, we test that reloading a page in cache versus a page **not** in
    cache **no longer** has a timing difference. We expect the page in cache to
    take similar time to reload.
    ```sh
    # As `user`
    cd ~/primitives

    # Flush the file -> pages are definitely not in cache
    ./build/flush /tmp/nocap-testing-file.bin

    # Use cachestat to determine whether the pages are really not in cache
    ./build/monitor-cachestat /tmp/nocap-testing-file.bin

    # Reload the non-cached file -- Note the timings! (the first page can be a bit off)
    ./build/read-bypass-backward-reading /tmp/nocap-testing-file.bin

    # Use cachestat to determine whether the pages are cached
    ./build/monitor-cachestat /tmp/nocap-testing-file.bin

    # Reload the cached file -- Note the timings! (the first page can be a bit off)
    ./build/read-bypass-backward-reading /tmp/nocap-testing-file.bin
    ```
    The timings should be roughly similar. Repeat this step (flush, monitor,
    reload, monitor, reload) multiple times to see the general trend.

8. Once asserted, log out as `user`, log in as `root`, then `poweroff`.


## Notes
Our artifact utilizes a "boiled-down" version of page cache attacks to speed up
review time. To fully verify the cross-user threat model, add a new user
(username: `newuser`, password: `newuser`):
```sh
# as root
useradd -m -s /bin/zsh newuser
usermod --password "$(openssl passwd -6 newuser)" newuser

cp -a /home/user/primitives /home/newuser/primitives
cp -a /home/user/.oh-my-zsh /home/newuser/.oh-my-zsh
cp -a /home/user/.zshrc /home/newuser/.zshrc

chown -R newuser:newuser /home/newuser

chmod 777 /tmp/nocap-testing-file.bin
```

Afterwards, the time to flush / reload the page in the nocap kernel *between
users* should be similar to the multiple invocations mentioned before. First
ensure that `user` brings the file into cache, switch to `newuser` and reload
the file again. Timings should be similar. This also extends to flush.

Our artifact also uses cachestat, which has been [mitigated in early
2025](https://lore.kernel.org/linux-cve-announce/2025021055-CVE-2025-21691-2bd2@gregkh/).
While cachestat no longer works on files between users, cachestat does report
page cache residency when the file is owned by that user.

### Reviewer-noted improvements (not applied)
During artifact evaluation, some reviewers pointed out optimizations. We have not
included these in the patches, since they would no longer correspond to the
paper, but we note them here:

1. **Bitfield index and mask math (`filemap.c`).** Since we derive the bitfield
   index and mask from `sizeof(unsigned long)` where `BITS_PER_LONG` is intended,
   the metadata grows about 8x larger than needed, and the `1 <<` shift is an
   `int` shift rather than a `long` shift.
2. **`readahead_init_fake_mapping`.** Under `CONFIG_READ_ONLY_THP_FOR_FS` we call
   `atomic_set(&fake_mapping.nr_thps, ...)`, which uses `.` on a pointer and
   therefore does not compile when that option is enabled.
3. **Stack-allocated fake mapping.** As the `struct address_space fake_mapping`
   in `readahead_mimic_sync` lives on the kernel stack, it is risky on 16 KiB
   stacks, and a `kmalloc` allocation would be safer.
4. **Unchecked allocation error.** Since `filemap_get_access_bitmask` and the
   sync mimic mapping path ignore the error returned by
   `filemap_get_access_bits_struct`, they can dereference a NULL pointer on
   `-ENOMEM`.
5. **`rdtsc` busy-wait flush delay.** As the `FADV_DONTNEED` delay busy-waits on
   `rdtsc`, it pins a CPU and depends on TSC stability, while a sleeping wait
   would avoid pinning the core, with a bare-metal TSC caveat. Since
   `FADV_DONTNEED` is unprivileged, the added busy-wait is a local
   denial-of-service and energy exposure rather than a leak.