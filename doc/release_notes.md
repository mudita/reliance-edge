# Reliance Edge Release Notes

This file contains a list of updates made to Reliance Edge over the course of
recent releases and a list of known issues.

## Release History and Changes

### Reliance Edge v2.6, October 2021

#### Common Code Changes

- Introduce a new on-disk layout where directory data blocks have a signature,
  CRC (checksum), and sequence number, like all other metadata blocks.
    - Support for the original on-disk layout is retained.  Old disks do _not_
      need to be reformatted.
    - Formatter options have been introduced to allow formatting with the
      original on-disk layout if desired.
    - The FSE API continues to use the original on-disk layout because it does
      not have directory data blocks.
- Add new "rollback" APIs: red\_rollback() and RedFseRollback().  Thanks to
  Jean-Christophe Dubois for suggesting and implementing these APIs.
- Updated the block buffer cache to support configurable buffer alignment to
  allow the buffers to be used in DMA transfers.  This adds a new macro to
  redconf.h: `REDCONF_BUFFER_ALIGNMENT`.
- Implement an alternative block buffer cache (commercial kit only) to improve
  file system performance.  Features of the new cache:
    - No longer limited to a maximum of 255 block buffers.  The new cache can
      support many thousands of buffers, which can improve performance on
      systems which have sufficient RAM for many block buffers.
    - Support for RAM-constrained systems is retained.  The new cache and the
      old cache use about the same amount of RAM given the same buffer count.
    - Optional write-gather buffer which can improve write throughput
      serveralfold in some use cases.  This adds a new macro to redconf.h:
      `REDCONF_BUFFER_WRITE_GATHER_SIZE_KB`.  For the open source release, this
      new macro must have a value of `0U`.
- Optimized block-aligned writes to make one pass over the metadata rather than
  two passes, which slightly improves performance of such writes.
- Critical error messages now print the file name and line number where the
  error message originated, to assist with debugging.
- Fixed an endian-swapping bug that prevented little-endian host tools from
  operating an a volume from a big-endian target, and vice versa.
- Fix a bug where, after deleting all the files in a directory, the directory
  could still have a nonzero size, preventing it from being deleted.  This bug
  could only manifest when the directory entry size was _not_ an integral
  divisor of the block size.  In other words, when the following was true:
  `REDCONF_BLOCK_SIZE % (REDCONF_NAME_MAX + 4) != 0`

#### INTEGRITY Port Changes

Fixed several bugs which affected the client-server configuration (libredfs.a).
The unified configuration (libredfs\_unified.a) was unaffected by these issues.
The bugs were:

- A client was unable to mount a volume that was already mounted by another
  client.
- If a client unmounted a volume with the `MNT_FORCE` flag, it would fail to
  close its open handles if another client had the volume mounted, which could
  lead to spurious `EMFILE` or `EBUSY` errors later on.  Note that this bug
  could not manifest, due to the prior bug preventing multiple client mounts.
- When the Reliance Edge checker (fsck\_redfs) was invoked, the client library
  was sending the wrong opcode to the server, causing the volume to be
  reformatted rather than checked.

### SafeRTOS Port Changes

Added support for SafeRTOS in the commercial kit.  SafeRTOS is a safety-critical
real-time operating system from WITTENSTEIN high integrity systems.  See the
"SafeRTOS Integration" chapter of the _Developer's Guide_ for further details on
SafeRTOS support.

### Reliance Edge v2.5.1, May 2020

#### Common Code Changes

- Rebrand from Datalight to Tuxera.  For details, please read the following:
  <https://www.tuxera.com/blog/tuxera-and-datalight-continue-forward-as-one-unified-brand-tuxera/>
- Fix a v2.5 regression where the checker (available in the
  commercially-licensed SDK) did not work when run as a standalone application.

#### Linux Port Changes

- Fix a v2.5 regression where the RAM disk was erroneously allocated as a
  zero-sized buffer.

#### Win32 Port Changes

- Fix a v2.5 regression where the powerint and errinject test projects (provided
  with the commercially-licensed SDK) were not functional.

### Reliance Edge v2.5, April 2020

#### Common Code Changes

- By popular demand, implement support for automatically detecting the sector
  count and sector size at run-time, as an alternative to specifying those
  values at compile-time.
    - This change is backward compatible with old redconf.c/redconf.h
      configuration files, which will continue to work as before.  The
      configuration files only need to be updated if enabling this new feature
      is desired.  To enable the feature, download the v2.5 version of the
      Reliance Edge Configuration Utility and select the "Auto" checkbox for
      the sector count or sector size.
    - Related to this change, a new file has been added to Reliance Edge:
      bdev/bdev.c.  If you have your own build system or project which lists all
      the Reliance Edge source files, please update it to include this new file.
    - The block device OS service (osbdev.c) has a new API to query the sector
      count and sector size: RedOsBDevGetGeometry().  If you implemented your
      own version of osbdev.c for a previous version of Reliance Edge, to update
      to v2.5 you must add an implemention of this new API.  See
      os/stub/services/osbdev.c for a template.  If you do not need the new
      feature, the API can be stubbed to return `-RED_ENOTSUPP`.
- Implement a block allocation optimization that improves write performance in
  some circumstances.
- Add red_umount2() to the POSIX-like API.  It is similar to red_umount() except
  that unmount flags can be specified.  A flag is provided to force an unmount
  even if there are open handles.

#### FreeRTOS Port Changes

- Remove the F_DRIVER example implementation of the block device service, which
  was deemed obsolete because support for FAT SL was removed in FreeRTOS v10.

#### INTEGRITY Port Changes

- Implement support for the `MNT_RDONLY` and `MNT_NOTRIM` flags, used with the
  `mount()` system call.
- Implement support for the `MNT_FORCE` flag in the `unmount()` system call,
  which has the effect of forcing an unmount even if there are open handles.
- Calling `mount()` when Reliance Edge is already mounted now fails and sets
  errno to `EBUSY`; whereas previously it did nothing and returned success.  The
  new behavior is consistent with the behavior of the native file systems.
- If `mount()` or `unmount()` is passed a mount point with extraneous trailing
  path separators, they now fail and set errno to `EINVAL`.  This fixes a bug
  where using trailing path separators would seem to succeed, but the VFS would
  fail to update the mount point array correctly, which could cause problems for
  subsequent operations.

#### FUSE Port Changes

- Fix build errors in non-default configurations.

### Reliance Edge v2.4, November 2019

#### Common Code Changes

- Fixed a bug which caused the maximum inode size to be miscalculated in some
  configurations with larger block sizes (8 KB and up).
- Add support for a sector size and block size of 128 bytes (the previous
  minimum was 256 bytes).  This is primarily useful when small-capacity
  nonvolatile RAM is used as the storage device.
- red_sync() and red_close() have been fixed to work with read-only volumes.
- Add red_mount2() to the POSIX-like API.  It is similar to red_mount() except
  that mount flags can be specified.  Flags are provided for mounting read-only
  and (in the commercial kit) to control whether discards are issued.
- Add red_fstrim() to the POSIX-like API (commercial kit only).  The API is used
  to discard free space, similar to the fstrim utility on Linux systems.
- The `REDCONF_API_POSIX_FSTRIM` macro has been added to redconf.h to specify
  whether red_fstrim() should be included.  Existing redconf.h files must be
  updated by opening and saving them with the updated Configuration Utility.
- RedOsBDevDiscard() (which only exists in the commercial kit) has been updated
  to return a `REDSTATUS` value, instead of returning `void`.
- MVStressTest, a new stress test that can exercise multiple file system
  volumes, has been added to the commercial kit.

### Reliance Edge v2.3, January 2019

#### Common Code Changes

- Add support for red_sync() to the POSIX-like API, including the new
  `RED_TRANSACT_SYNC` automatic transaction flag and requisite configuration
  option in the Reliance Edge Configuration Utility.
- Add discard testing to BDevTest (available in the commercial kit).
- Fix bugs affecting multivolume use cases caused by incorrect use of
  "current volume" global variables.  These bugs could cause spurious I/O
  errors and metadata corruption.
- Fix a bug in checker (available in the commercial kit): an array indexing
  error would cause the wrong link count to be printed when link count
  corruption was detected.

#### INTEGRITY Port Changes

- Add support for the storage driver API introduced in INTEGRITY v11.7.
- Add INTEGRITY v11.7.x-compatible example projects for the INTEGRITY ARM
  Simulator and the Renesas R-Car H3 Starter Kit.
- Instead of unconditionally transacting all volumes, the INTEGRITY sync()
  system call now calls red_sync().  Transactions will only be performed on
  volumes which set the `RED_TRANSACT_SYNC` automatic transaction flag.
- Fixed a bug which resulted in errors when mounting more than one volume.
- An `EINVAL` error is now returned by `mount()` if the Reliance Edge mount
  point does not start with "//".  Because of how INTEGRITY parses paths, the
  "//" has always been required; and while this was documented, it was not
  enforced in the code, which could lead to subtle errors.

### Reliance Edge v2.2.1, June 2018

#### Common Code Changes

- Fix bugs in the implementation of the sector offset feature added in Reliance
  Edge v2.2.  Anyone who set the sector offset to a value other than zero is
  strongly encouraged to upgrade.
- Fix a minor bug in the POSIX-like API where operations which are not allowed
  on the root directory -- such as deleting, renaming, or recreating it -- would
  set `red_errno` to `RED_EINVAL` rather than the appropriate errno value.
- Fix a bug in the POSIX-like API Test Suite which caused a link error if
  relative paths were enabled but rename was disabled.
- Fix minor documentation issues.
- Fix a test bug which was causing the simulated power interruption test in
  projects/powerint to fail.  This test is only provided with the commercial
  kit.

#### INTEGRITY Port Changes

- Fix a bug in the block device code which would fail to report an error and
  leave the sector size uninitialized, later causing a memory violation.
- Update to support binary and text modes with fopen(), such as "w+b" or "rt".
  Regardless of open mode, Reliance Edge, like the native file systems, does not
  perform newline conversions.
- Minor fixes and enhancements to the INTEGRITY port documentation.
- Fix a path problem in the `host/Makefile` for the bbb-app-unified and
  bbb-app-clientserv example projects.

#### FreeRTOS Port Changes

- Fix a bug in the F_DRIVER example implementation of the block device service.
  According to the FreeRTOS documentation, the `release` function pointer in
  the `F_DRIVER` structure is allowed to be `NULL`, but the old code was
  calling that function unconditionally, without first ensuring it was
  non-`NULL`.  The updated code will only call `release` when it is non-`NULL`.

#### Linux Port Changes

- Add support for discards on compatible block devices (commercial kit only).
- Add locking to the FUSE port to fix multithreading issues.

### Reliance Edge v2.2, December 2017

#### Common Code Changes
- The Reliance Edge Configuration Utility has been updated to support specifying
  a sector offset for each volume.  This allows Reliance Edge to be used on
  partitioned storage devices without osbdev.c customization.  To upgrade an old
  redconf.c to be compatible with this change, the sector offset must be
  manually added to the entry for each volume in redconf.c, for example:
    - Before: `{ 512U, 65536U, false, 1024U, 0U, "VOL0:" }`
    - After: `{ 512U, 65536U, 0U, false, 1024U, 0U, "VOL0:" }`

#### INTEGRITY Port Changes
- Reliance Edge can now be used from more than one AddressSpace, via the new
  client-server implementation.  For cases where only one AddressSpace is
  using Reliance Edge and the client-server overhead is undesirable, a
  unified library is also offered; it runs in the same AddressSpace as the
  application, similar to the v2.1 release.
- The block device implementation has been updated to support IDE/ATA/SATA
  devices.

#### U-Boot Port Changes
- Added support for the Universal Boot Loader (U-Boot) in the open-source kit.
  This allows a system using U-Boot to boot from an operating system image or
  kernel image stored on a Reliance Edge file system volume.
- See the "U-Boot Integration" chapter of the _Developer's Guide_ for further
  details on U-Boot support.

### Reliance Edge v2.1, October 2017

#### Common Code Changes
- Added optional support for current working directories, relative paths, and
  dot and dot-dot handling.  This includes the new red_chdir() and red_getcwd()
  APIs.
- A volume path prefix, by itself, is now a valid reference to the root
  directory on that volume.  Assuming the path prefix is "VOL0:" and the path
  separator character is '/', previously red_opendir("VOL0:/") would succeed and
  red_opendir("VOL0:") would fail.  Now both will succeed.  This results in more
  intuitive behavior when the volume path prefix is changed to look like a
  directory, such as "/sdcard"; red_opendir("/sdcard") and red_chdir("/sdcard")
  intuitively look like they should work, and now they do.
- Updated the imgcopy utility to open files in binary mode.  When used on
  Windows, this fixes a problem where Windows attempted to perform newline
  conversion in binary files, corrupting the file data.
- Fixed a bug in the POSIX-like API Test Suite which caused it to fail when run
  on a volume other than volume zero.  Fixed another bug which caused it to
  fail when the volume name was unusually long.
- Added an option to FSIOTest to control flush frequency for the sequential
  write and sequential rewrite tests.  Default behavior is unchanged.

#### INTEGRITY Port Changes
- Added support for the INTEGRITY RTOS in the commercial kit.  Reliance Edge
  integrates into the INTEGRITY file system layer so that system calls like
  open() or read() work with Reliance Edge; there is no need to update an
  application to use Reliance Edge's POSIX-like API.
- Example projects for the BeagleBone Black are provided.
- For the time being, the osbdev.c implementation is only known to work with
  the SD card driver supplied with the BeagleBone Black BSP.  It is known _not_
  to work with the IDE driver used on x86 PCs.  Other storage drivers have not
  been tested.
- See the "INTEGRITY Integration" chapter of the _Developer's Guide_ for
  further details on INTEGRITY support.  Please read this chapter before using
  Reliance Edge on INTEGRITY.

#### FreeRTOS Port Changes
- Moved the several example implementations in osbdev.c into header files.
- Added a "stub" example, which is now the default implementation for FreeRTOS,
  and which deliberately does not compile.  This is to make it obvious that
  FreeRTOS users need to make modifications to osbdev.c for their environment.
- Added support for using Datalight FlashFX Tera as a block device.  This
  support only exists in the commercial kit.
- The Atmel Studio projects for the SAM4E-EK and SAM4S Xplained Pro boards are
  no longer included in the open source kit; they are still available in the
  commercial kit.

### Reliance Edge v2.0, January 2017

- Added support for Linux as a host environment
  - All "host" projects may now be built in either Windows or Linux using the
    `make` command.  The formatter and image builder are built, and the checker
    and image copier are also built in the commercial kit.
  - An additional host tool has been added for Linux only: `redfuse`.  It is a
    File System in User Space (FUSE) implementation, allowing a Reliance Edge
    volume to be mounted directly on Linux for easy access.  It is built from
    the host project folder using the command `make redfuse`.
  - The OS-specific API test (commercial kit only) is now ported to run on Linux
    for the purpose of verifying the FUSE implementation.
- Fixed a bug that could leave a directory in an invalid state after removing
  files.  For example, an affected directory might report a non-zero length even
  after all files had been deleted.
- Fixed a bug that would leave the driver in a bad state if a mount operation
  failed due to missing or corrupt metaroot blocks.

### Reliance Edge v1.1 (Beta), November 2016

- Added support for a discard (trim) interface in the commercial kit.  While
  discards are not integral to the behavior of the filesystem, they allow
  certain types of Flash drivers and media to perform at optimal speed and
  efficiency.  The commercial version of Reliance Edge now allows the user to
  implement this interface for compatible storage media.
  - This change added new fields to the configuration files redconf.h and
    redconf.c. The configuration utility has been updated to version 1.1 and
    existing configuration files must be updated using the updated utility.
- The configuration utility now has keyboard shortcuts for opening and saving
  the configuration.
- The configuration utility now adds version macros to easily identify when an
  outdated configuration file is used with Reliance Edge or vice versa.

### Reliance Edge v1.0.4, July 2016

- Added ARM mbed and ARM mbed OS support in the commercial kit, with an example
  projects for ARM mbed OS on the NXP FRDM-K64F board.
- Some minor deficiencies in the POSIX-like API test suite have been addressed.

### Reliance Edge v1.0.3, June 2016

- Added support for static memory allocation configuration in FreeRTOS
  version 9.  No common code changes.

### Reliance Edge v1.0.2, February 2016

#### Common Code Changes
- A new per-volume configuration option has been added: users can specify a
  number of times to retry a block device read, write or flush operation before
  returning a failure.  The configuration tool has been updated to version 1.0.2
  with this change.
  - This added a new field to the volume configuration in redconf.c: existing
    redconf.c files from v1.0.1 and earlier must be updated to work with v1.0.2.
    Open redconf.h and redconf.c with the configuration tool, enable
    "Retry block device I/O on failure" for any volumes if desired, and save the
    redconf files.

#### FreeRTOS Port Changes
- Added support for the STM32 HAL SD card driver in the FreeRTOS block device
  interface.  Two boards are supported out-of-the-box: the STM324xG-EVAL and the
  STM32F746NG-Discovery.  A sample project is included for the STM324xG-EVAL.

#### MQX Port Changes
- Fixed a bug which prevented Reliance Edge from compiling if the File System
  Essentials API was selected in the configuration.
- Fixed a bug which would have returned an uninitialized value from
  `RedOsBDevFlush()` for block devices that support flushing.

### Reliance Edge v1.0.1, October 2015

- Added MQX RTOS support in the commercial kit, with example projects for
  the Kinetis Design Studio.
- Bug fix in the F_DRIVER implementation of the FreeRTOS block device service.

### Reliance Edge v1.0, July 2015

#### Common Code Changes
- First release of commercial kit and MISRA C:2012 Design Assurance Package.
  The commercial kit includes many new tools and tests which were not previously
  available.
- Overhauled parsing of command-line parameters to be consistent for all tools
  and tests.  Command-line tools now use Unix-style short and long options (such
  as `-H` and `--help`) instead of DOS-style switches (such as `/?`).
- Renamed all os/\*/include/ostypes.h headers to os/\*/include/redostypes.h, so
  that all headers use the product prefix.  If you created a port using v0.9,
  this header needs to be renamed and its header guard (#ifndef OSTYPES_H etc.)
  should also be updated.
- Add a new header for OS-specific MISRA C:2012 deviation macros, located at
  os/\*/include/redosdeviations.h.  If you created a port using v0.9, copy the
  template from os/stub/include/redosdeviations.h into the include directory.
- Eliminated support for sector sizes less than 256.  If using a smaller sector
  size (say for a RAM disk), this must now be emulated in the implementation of
  the block device OS service.
- Added RedFseFormat() as an optional FSE API, allowing FSE applications to
  format the volume at run-time.
  - This added a new macro to redconf.h: existing redconf.h files from v0.9 must
    be updated to work with v1.0.  Open redconf.h with the configuration tool,
    ignore the warning about the missing macro, and save it.
- Internal restructuring has renamed the macros for the string and memory
  functions used in redconf.h.  An existing redconf.h file from v0.9 will need
  to be updated; for a file containing the old names, the new config tool will
  default to using the (slow) Reliance Edge string/memory functions; to use the
  C library or custom versions, this will need to be selected in the
  configuration utility.
- Fix a bug which would result in an error when attempting to create a name with
  one or more trailing path separators (such as `red_mkdir("/foo/bar/")`).
- Fix a bug where an open handle for an inode on one volume would prevent the
  same inode number from being deleted on a different volume.

#### FreeRTOS Port Changes

- The implementation of the timestamp OS service no longer requires that
  `configUSE_TIMERS` be set to `1`.

### Reliance Edge v0.9 (Beta), April 2015

First public release.

