# Specter

## `specter_emu`
Unfinished software emulator, RISC-V, see [rv64-emu](https://github.com/TypeA2/rv64-emu-doom) for a more complete version (RV64IMAFDC support).

## `specter_rec`
Dynamic recompilation of target architecture to native. System calls are intercepted.

```
 -----------         -------------------------------------------
| Input ELF |  -->  | Array of target architecture instructions |
 -----------         -------------------------------------------
                                          ||
                                          \/
                      -----------------------------------------
                     | Array of internal abstract instructions |
                      -----------------------------------------
                                          ||
                                          \/
              ----------------------------------------------------------
             | Array native architecture code + injected framework code |
              ----------------------------------------------------------
```
