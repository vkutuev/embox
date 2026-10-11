/**
 * @file
 * @brief Reproducer for the niiet_gpio driver IRQ-attach bug on K1921VG1T.
 *
 * Check 1 (the bug fixed by the attached patch): after two
 * gpio_setup_mode() calls on the same port, the interrupt should still
 * be armed (INTENSET bit set). On the unpatched driver the second call
 * re-runs irq_attach(), gets -EBUSY, and returns before enabling the
 * interrupt, silently leaving INTENSET clear.
 *
 * Check 2 (observation): re-running gpio_setup_mode() rewrites the
 * edge-configuration registers, which can latch a spurious INTSTATUS
 * bit on this silicon (a separate issue, not fixed here).
 */

#include <stdio.h>

#include <drivers/gpio.h>

#define GPIOB_BASE      0x40001000UL
#define GPIOB_INTSTATUS (GPIOB_BASE + 0x80)
#define GPIOB_INTENSET  (GPIOB_BASE + 0x60)
#define GPIOB_INTENCLR  (GPIOB_BASE + 0x64)

#define TEST_PORT   GPIO_PORT_B
#define TEST_PIN    (1U << 3)

static inline unsigned int gpio_reg_read(unsigned long a) {
	return *(volatile unsigned int *)a;
}

static inline void gpio_reg_write(unsigned long a, unsigned int v) {
	*(volatile unsigned int *)a = v;
}

int main(int argc, char **argv) {
	unsigned int v;
	int ok = 1;

	printf("niiet_gpio_check: GPIOB pin 3\n");

	/* Check 1: IRQ-attach bookkeeping (-EBUSY silent disable). */
	gpio_setup_mode(TEST_PORT, TEST_PIN,
			GPIO_MODE_IN | GPIO_MODE_IN_PULL_UP | GPIO_MODE_INT_FALLING);
	gpio_setup_mode(TEST_PORT, TEST_PIN,
			GPIO_MODE_IN | GPIO_MODE_IN_PULL_UP |
			GPIO_MODE_INT_FALLING | GPIO_MODE_INT_EN);

	v = gpio_reg_read(GPIOB_INTENSET);
	if (v & TEST_PIN) {
		printf("check 1: INTENSET = 0x%x  [PASS] interrupt armed\n", v);
	} else {
		printf("check 1: INTENSET = 0x%x  [FAIL] interrupt silently disabled (irq_attach -EBUSY)\n", v);
		ok = 0;
	}

	gpio_reg_write(GPIOB_INTENCLR, TEST_PIN);
	gpio_reg_write(GPIOB_INTSTATUS, TEST_PIN);

	/* Check 2: re-config latch (observation). */
	gpio_setup_mode(TEST_PORT, TEST_PIN,
			GPIO_MODE_IN | GPIO_MODE_IN_PULL_UP | GPIO_MODE_INT_FALLING);
	gpio_reg_write(GPIOB_INTSTATUS, TEST_PIN);

	gpio_setup_mode(TEST_PORT, TEST_PIN,
			GPIO_MODE_IN | GPIO_MODE_IN_PULL_UP | GPIO_MODE_INT_FALLING);

	v = gpio_reg_read(GPIOB_INTSTATUS);
	if (v & TEST_PIN) {
		printf("check 2: INTSTATUS = 0x%x  [observation] spurious latch on re-config\n", v);
	} else {
		printf("check 2: INTSTATUS = 0x%x  [observation] no latch\n", v);
	}

	gpio_reg_write(GPIOB_INTSTATUS, TEST_PIN);

	printf("niiet_gpio_check: %s\n", ok ? "PASS" : "FAIL");
	return ok ? 0 : 1;
}
