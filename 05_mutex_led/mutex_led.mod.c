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
	{ 0x4202ea9c, "class_destroy" },
	{ 0xfe990052, "gpio_free" },
	{ 0x507c67b4, "gpiod_direction_output_raw" },
	{ 0x47229b5c, "gpio_request" },
	{ 0x8693c29e, "of_get_named_gpio_flags" },
	{ 0xd0f859e2, "device_destroy" },
	{ 0x376c6eea, "of_find_node_opts_by_path" },
	{ 0xf1a8b4a9, "device_create" },
	{ 0x644b2a6e, "cdev_del" },
	{ 0xb47e05b2, "__class_create" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0x24ade49f, "cdev_add" },
	{ 0xf5787540, "cdev_init" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0x3fd78f3b, "register_chrdev_region" },
	{ 0xe346f67a, "__mutex_init" },
	{ 0xdecd0b29, "__stack_chk_fail" },
	{ 0xc5850110, "printk" },
	{ 0x5f754e5a, "memset" },
	{ 0x39dc96e9, "gpiod_set_raw_value" },
	{ 0x9a36bdf0, "gpio_to_desc" },
	{ 0x514cc273, "arm_copy_from_user" },
	{ 0x8f678b07, "__stack_chk_guard" },
	{ 0xc271c3be, "mutex_lock" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
	{ 0x67ea780, "mutex_unlock" },
};

MODULE_INFO(depends, "");

