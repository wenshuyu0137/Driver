#include <linux/build-salt.h>
#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(.gnu.linkonce.this_module) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used __section(__versions) = {
	{ 0x3e549f1d, "module_layout" },
	{ 0xb013481f, "platform_driver_unregister" },
	{ 0xd7362d1, "__platform_driver_register" },
	{ 0x4e2531ba, "input_register_device" },
	{ 0x6dcee9a0, "input_allocate_device" },
	{ 0x9545af6d, "tasklet_init" },
	{ 0x2072ee9b, "request_threaded_irq" },
	{ 0xe239b92f, "irq_get_irq_data" },
	{ 0x5154e274, "gpiod_to_irq" },
	{ 0xc6f46339, "init_timer_key" },
	{ 0x526c3a6c, "jiffies" },
	{ 0x419fdde, "gpiod_direction_input" },
	{ 0x47229b5c, "gpio_request" },
	{ 0xc5850110, "printk" },
	{ 0x8693c29e, "of_get_named_gpio_flags" },
	{ 0x76454a84, "input_event" },
	{ 0xa480411c, "gpiod_get_raw_value" },
	{ 0x9a36bdf0, "gpio_to_desc" },
	{ 0xfaef0ed, "__tasklet_schedule" },
	{ 0xca54fee, "_test_and_set_bit" },
	{ 0xc38c83b8, "mod_timer" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
	{ 0x2a57243, "input_unregister_device" },
	{ 0x97934ecf, "del_timer_sync" },
	{ 0xfe990052, "gpio_free" },
	{ 0xc1514a3b, "free_irq" },
};

MODULE_INFO(depends, "");

