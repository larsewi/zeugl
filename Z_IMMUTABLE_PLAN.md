# Z_IMMUTABLE Flag Implementation Plan

## Overview
The Z_IMMUTABLE flag will enable zeugl to handle files with immutable attributes by temporarily removing the immutable bit before atomic file replacement, then restoring it after the operation completes.

## Background
Different operating systems implement file immutability differently:
- **Linux**: Uses `ioctl()` with `FS_IOC_GETFLAGS` and `FS_IOC_SETFLAGS` to manipulate the immutable attribute (FS_IMMUTABLE_FL)
- **BSD (FreeBSD, OpenBSD, NetBSD)**: Uses `chflags()` system call with SF_IMMUTABLE (system) or UF_IMMUTABLE (user) flags
- **macOS**: Similar to BSD, uses `chflags()` with UF_IMMUTABLE flag
- **Solaris/illumos**: Uses extended attributes or ACLs (less common, will not implement)

## Design Goals
1. **Best-effort approach**: Silently proceed if immutable operations are not supported
2. **Cross-platform compatibility**: Support Linux, BSD variants, and macOS
3. **Atomic safety**: Ensure immutable bit handling doesn't compromise atomicity
4. **Backward compatibility**: Don't affect existing functionality when flag is not used

## Implementation Steps

### 1. Update Header File (`lib/zeugl.h`)
```c
#define Z_IMMUTABLE 1 << 4  // New flag to handle immutable files
```

### 2. Create Platform Detection (`lib/immutable.h`)
New header file for immutable operations:
```c
#ifndef __ZEUGL_IMMUTABLE_H__
#define __ZEUGL_IMMUTABLE_H__

#include <stdbool.h>

// Check if file has immutable attribute
bool is_immutable(const char *path);

// Remove immutable attribute (returns previous state)
bool clear_immutable(const char *path);

// Set immutable attribute
bool set_immutable(const char *path);

#endif
```

### 3. Platform-Specific Implementation (`lib/immutable.c`)

#### Structure:
```c
#include "config.h"
#include "immutable.h"

#ifdef __linux__
  // Linux implementation using ioctl
  #include <linux/fs.h>
  #include <sys/ioctl.h>
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__APPLE__)
  // BSD and macOS implementation using chflags
  #include <sys/stat.h>
  #include <unistd.h>
#endif
```

#### Linux Implementation:
- Open file with `open(path, O_RDONLY)`
- Use `ioctl(fd, FS_IOC_GETFLAGS, &flags)` to get current flags
- Check if `FS_IMMUTABLE_FL` is set
- Use `ioctl(fd, FS_IOC_SETFLAGS, &flags)` to clear/set immutable bit

#### BSD/macOS Implementation:
- Use `stat(path, &st)` to get current flags
- Check `st.st_flags` for `UF_IMMUTABLE` or `SF_IMMUTABLE`
- Use `chflags(path, flags)` to modify immutable bit

#### Fallback Implementation:
- Return false/success for all operations (best-effort)

### 4. Integrate with zeugl.c

#### Modifications to `zopen()`:
```c
// In zopen() function, after allocating struct zfile
file->flags = flags;  // Store flags for later use in zclose()
```

#### Modifications to `zclose()`:
```c
// Before rename operation in zclose()
bool was_immutable = false;
if (file->flags & Z_IMMUTABLE) {
    was_immutable = is_immutable(file->orig);
    if (was_immutable) {
        clear_immutable(file->orig);
        LOG_DEBUG("Temporarily cleared immutable attribute from '%s'", file->orig);
    }
}

// Perform the rename operation (existing code)
if (rename(file->temp, file->orig) != 0) {
    // ... existing error handling ...
}

// After successful rename
if ((file->flags & Z_IMMUTABLE) && was_immutable) {
    set_immutable(file->orig);
    LOG_DEBUG("Restored immutable attribute on '%s'", file->orig);
}
```

#### Update zfile Structure:
```c
struct zfile {
    char *orig;
    char *temp;
    char *mole;
    int fd;
    mode_t mode;
    int flags;           // Store the flags passed to zopen()
    struct zfile *next;
};
```

### 5. Update whackamole.c
Modify `whack_a_mole()` to accept a boolean parameter for handling immutable files:

```c
bool whack_a_mole(const char *orig, const char *temp, bool handle_immutable) {
    // ... existing code ...

    // Before final rename operation
    bool was_immutable = false;
    if (handle_immutable) {
        was_immutable = is_immutable(orig);
        if (was_immutable) {
            clear_immutable(orig);
            LOG_DEBUG("Temporarily cleared immutable attribute from '%s'", orig);
        }
    }

    // Existing rename operation
    if (rename(survivor, orig) == 0) {
        LOG_DEBUG("Replaced the last survivor (mole '%s') with the original file '%s'",
                  survivor, orig);

        // Restore immutable bit if needed
        if (handle_immutable && was_immutable) {
            set_immutable(orig);
            LOG_DEBUG("Restored immutable attribute on '%s'", orig);
        }
    } else {
        // ... existing error handling ...
    }

    // ... rest of function ...
}
```

Update function signature in `whackamole.h`:
```c
bool whack_a_mole(const char *orig, const char *temp, bool handle_immutable);
```

Update calls to `whack_a_mole()` in zeugl.c:
```c
// When calling whack_a_mole, pass whether to handle immutable files
whack_a_mole(file->orig, file->mole, file->flags & Z_IMMUTABLE);
```

### 6. Configure Script Updates (`configure.ac`)
Add checks for:
- `AC_CHECK_HEADERS([linux/fs.h sys/ioctl.h])`
- `AC_CHECK_FUNCS([chflags])`
- Define appropriate macros for platform detection

### 7. Testing Strategy

#### Unit Tests (`tests/`)
1. **Test basic immutable handling**:
   - Create file, set immutable, use Z_IMMUTABLE flag
   - Verify file can be modified with flag
   - Verify immutable bit is restored

2. **Test without Z_IMMUTABLE flag**:
   - Verify operations fail on immutable files without flag

3. **Test concurrent access**:
   - Multiple processes with Z_IMMUTABLE on same file

4. **Platform-specific tests**:
   - Skip tests on unsupported platforms
   - Test both user and system immutable bits (where applicable)

#### Test Implementation:
```bash
# Create test file
echo "content" > test.txt

# Linux: Set immutable
sudo chattr +i test.txt

# BSD/macOS: Set immutable
chflags uchg test.txt

# Test with zeugl
echo "new content" | zeugl -i test.txt

# Verify immutable is restored
lsattr test.txt  # Linux
ls -lo test.txt  # BSD/macOS
```

#### GitHub Action Workflow for Immutable Testing

Since immutable operations require root privileges, create a separate GitHub Action workflow that tests across multiple platforms:

##### `.github/workflows/immutable.yml`:
```yaml
name: Test immutable bit

on:
  push:
    branches: [ master ]
  pull_request:
    branches: [ master ]

jobs:
  test-immutable:
    strategy:
      fail-fast: false
      matrix:
        os: [ubuntu-latest, macos-latest]
        include:
          - os: ubuntu-latest
            container: ubuntu:latest
            container_options: --privileged
            deps_install: |
              apt-get update
              apt-get install -y build-essential autoconf automake libtool e2fsprogs
            set_immutable: chattr +i
            unset_immutable: chattr -i
            check_immutable: lsattr test.txt | grep -q '^----i'

          - os: macos-latest
            deps_install: brew install autoconf automake libtool
            set_immutable: chflags uchg
            unset_immutable: chflags nouchg
            check_immutable: ls -lO test.txt | grep -q 'uchg'

    runs-on: ${{ matrix.os }}
    container:
      image: ${{ matrix.container }}
      options: ${{ matrix.container_options }}

    steps:
    - uses: actions/checkout@v4

    - name: Install dependencies
      run: ${{ matrix.deps_install }}

    - name: Build zeugl
      run: |
        ./bootstrap.sh
        ./configure --enable-debug
        make

    - name: Test immutable operations
      run: |
        # Create test script
        cat > test_immutable.sh << 'EOF'
        #!/bin/bash
        set -e

        # Test 1: Basic immutable handling
        echo "Test 1: Basic immutable handling"
        echo "original" > test.txt
        ${{ matrix.set_immutable }} test.txt
        echo "modified" | ./cli/zeugl -i test.txt
        content=$(cat test.txt)
        [ "$content" = "modified" ] || exit 1
        ${{ matrix.check_immutable }} || exit 1
        ${{ matrix.unset_immutable }} test.txt
        rm test.txt

        # Test 2: Without Z_IMMUTABLE flag (should fail)
        echo "Test 2: Without Z_IMMUTABLE flag"
        echo "original" > test.txt
        ${{ matrix.set_immutable }} test.txt
        if echo "modified" | ./cli/zeugl test.txt 2>/dev/null; then
          echo "ERROR: Should have failed without -i flag"
          exit 1
        fi
        ${{ matrix.unset_immutable }} test.txt
        rm test.txt

        # Test 3: Concurrent access with immutable
        echo "Test 3: Concurrent access"
        echo "original" > test.txt
        ${{ matrix.set_immutable }} test.txt

        # Launch multiple concurrent writes
        for i in {1..5}; do
          echo "process$i" | ./cli/zeugl -i test.txt &
        done
        wait

        # Verify file is still immutable and has content
        ${{ matrix.check_immutable }} || exit 1
        [ -s test.txt ] || exit 1
        ${{ matrix.unset_immutable }} test.txt
        rm test.txt

        echo "All tests passed!"
        EOF

        chmod +x test_immutable.sh
        ./test_immutable.sh
```

This workflow will:
- Use a matrix strategy for Linux and macOS to reduce code duplication
- Test immutable functionality on both platforms
- Run on push/PR to master branch
- Include platform-specific commands via matrix variables
- Not interfere with regular `make check` tests that don't require root

### 8. CLI Tool Updates (`cli/main.c`)
Add new command-line option:
```c
case 'i':
    flags |= Z_IMMUTABLE;
    break;
```

Update usage message:
```
-i    Handle immutable files (temporarily remove immutable bit)
```

### 9. Documentation Updates

#### Man Page (`man/zeugl.1`):
- Document Z_IMMUTABLE flag behavior
- Explain platform differences
- Mention that it is "best effort" (no guarantees)
- Provide examples

#### README.md:
- Add section on immutable file handling
- Platform support matrix

## Error Handling

1. **Permission errors**: Operations may require elevated privileges
   - Log warning but continue (best-effort)

2. **Unsupported platforms**:
   - Silently ignore immutable operations

3. **Race conditions**:
   - File might become immutable between check and operation
   - Handle EACCES/EPERM errors gracefully

## Security Considerations

1. **Privilege escalation**: Never attempt to escalate privileges
2. **Atomicity**: Ensure immutable bit changes don't create race conditions
3. **Cleanup**: Always attempt to restore immutable bit, even on error paths

## Performance Impact

- Minimal: Only two additional system calls when flag is used
- No impact when flag is not used
- Cache immutable state to avoid repeated checks

## Future Enhancements

1. Support for additional attributes (append-only, no-dump, etc.)
2. Windows support (using FILE_ATTRIBUTE_READONLY)

## Implementation Timeline

1. **Phase 1**: Core implementation for Linux (2 days)
2. **Phase 2**: BSD and macOS support (2 days)
3. **Phase 3**: Testing and edge cases (3 days)
4. **Phase 4**: Documentation and examples (1 day)

## Risks and Mitigation

| Risk | Impact | Mitigation |
|------|--------|------------|
| Platform differences | Medium | Comprehensive testing on each platform |
| Permission issues | Low | Best-effort approach, clear documentation |
| Race conditions | Low | Proper locking, atomic operations |
| Breaking changes | Low | New flag is opt-in only |

## Success Criteria

1. Z_IMMUTABLE flag successfully handles immutable files on Linux
2. Z_IMMUTABLE flag successfully handles immutable files on BSD/macOS
3. Operations gracefully degrade on unsupported platforms
4. No regression in existing functionality
5. All tests pass on supported platforms
