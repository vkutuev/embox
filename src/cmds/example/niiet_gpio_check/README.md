# niiet_gpio_check

Reproduces the `niiet_gpio` driver IRQ-attach bug on the K1921VG1T and
verifies the fix. Runs on the NIIET-DEV-K1921VG1T board (GPIOB pin 3 =
USER button, idle high).

## Check 1 — IRQ-attach bookkeeping (the bug)

Two `gpio_setup_mode()` calls on the same port should leave the
interrupt armed. The unpatched driver never records the attached IRQ
handler, so the second call re-runs `irq_attach()`, gets `-EBUSY`, and
returns before enabling the interrupt.

Expected output:

- **Unpatched driver** (`niiet-gpio-bug` branch):

  ```
  check 1: INTENSET = 0x0  [FAIL] interrupt silently disabled (irq_attach -EBUSY)
  ```

- **Patched driver** (`niiet-gpio-fix` branch):

  ```
  check 1: INTENSET = 0x8  [PASS] interrupt armed
  ```

## Check 2 — re-config latch (observation)

Re-running `gpio_setup_mode()` rewrites the edge-configuration
registers; on this silicon that can latch a spurious `INTSTATUS` bit.
Reported for information only; it is a separate issue not fixed by the
Check 1 patch.

## Build

```console
$ make confload-platform/niiet/k1921vg1t_dev
$ echo "include embox.cmd.example.niiet_gpio_check.niiet_gpio_check" >> conf/mods.conf
$ make -j16
```

Then flash `build/base/bin/embox` and run `niiet_gpio_check` from the
shell.
