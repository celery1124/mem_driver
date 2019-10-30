#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0xc59e9fa0, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0x9b4da327, __VMLINUX_SYMBOL_STR(kmalloc_caches) },
	{ 0xfbc74f64, __VMLINUX_SYMBOL_STR(__copy_from_user) },
	{ 0x67c2fa54, __VMLINUX_SYMBOL_STR(__copy_to_user) },
	{ 0xd59397fb, __VMLINUX_SYMBOL_STR(mutex_unlock) },
	{ 0x597dda50, __VMLINUX_SYMBOL_STR(gen_pool_alloc) },
	{ 0xc9fec25c, __VMLINUX_SYMBOL_STR(misc_register) },
	{ 0xecdf0683, __VMLINUX_SYMBOL_STR(gen_pool_add_virt) },
	{ 0x563afdb, __VMLINUX_SYMBOL_STR(__mutex_init) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0x4872e635, __VMLINUX_SYMBOL_STR(mutex_lock) },
	{ 0xd3ee3db2, __VMLINUX_SYMBOL_STR(kmem_cache_alloc) },
	{ 0x52440088, __VMLINUX_SYMBOL_STR(gen_pool_destroy) },
	{ 0x37a0cba, __VMLINUX_SYMBOL_STR(kfree) },
	{ 0x9e1ebc08, __VMLINUX_SYMBOL_STR(remap_pfn_range) },
	{ 0x9e18d93e, __VMLINUX_SYMBOL_STR(gen_pool_create) },
	{ 0xcdbfd194, __VMLINUX_SYMBOL_STR(gen_pool_free) },
	{ 0xf3dd00cd, __VMLINUX_SYMBOL_STR(misc_deregister) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

