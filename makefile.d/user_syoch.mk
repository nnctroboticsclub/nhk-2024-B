my_mount_build:
	[ -d robot1-main/BUILD ] || mkdir robot1-main/BUILD
	[ -d controller-stm32/src/usb_rs/target ] || mkdir controller-stm32/src/usb_rs/target
	[ -d esp32/build ] || mkdir esp32/build

	mount | grep "/workspaces/nhk-2024-b/.cache" || sudo mount /dev/lvg-share/cache .cache


	sudo mount --bind .cache/n24b build
	sudo mount --bind .cache/n24b_es esp32/build
	sudo mount --bind .cache/n24b_us controller-stm32/src/usb_rs/target

my_umount_build:
	sudo umount build || :
	sudo umount esp32/build || :
	sudo umount controller-stm32/src/usb_rs/target || :
	sudo umount .cache || :

my_prune_build:
	rm -rf build*
	rm -rf esp32/build*
	rm -rf controller-stm32/src/usb_rs/target*