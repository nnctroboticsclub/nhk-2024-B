my_mount_build:
	[ -d robot1-main/BUILD ] || mkdir robot1-main/BUILD
	[ -d robot-bridge/BUILD ] || mkdir robot-bridge/BUILD
	[ -d robot-collect/BUILD ] || mkdir robot-collect/BUILD

	[ -d controller-stm32/BUILD ] || mkdir controller-stm32/BUILD
	[ -d connection-test/BUILD ] || mkdir connection-test/BUILD

	[ -d esp32/build ] || mkdir esp32/build
	[ -d controller-stm32/src/usb_rs/target ] || mkdir controller-stm32/src/usb_rs/target

	mount | grep "/workspaces/nhk-2024-b/.cache" || sudo mount /dev/lvg-share/cache .cache


	sudo mount --bind .cache/n24b_r1 robot1-main/BUILD
	sudo mount --bind .cache/n24b_r2 robot-bridge/BUILD
	sudo mount --bind .cache/n24b_r3 robot-collect/BUILD
	sudo mount --bind .cache/n24b_cs controller-stm32/BUILD
	sudo mount --bind .cache/n24b_ct connection-test/BUILD
	sudo mount --bind .cache/n24b_es esp32/build
	sudo mount --bind .cache/n24b_us controller-stm32/src/usb_rs/target
	sudo mount --bind .cache/libs common_libs/.projects

my_umount_build:
	sudo umount robot1-main/BUILD || :
	sudo umount robot-bridge/BUILD || :
	sudo umount robot-collect/BUILD || :
	sudo umount controller-stm32/BUILD || :
	sudo umount connection-test/BUILD || :
	sudo umount esp32/build || :
	sudo umount controller-stm32/src/usb_rs/target || :
	sudo umount .cache || :

my_prune_build:
	rm -rf robot1-main/BUILD/*
	rm -rf robot-bridge/BUILD/*
	rm -rf robot-collect/BUILD/*
	rm -rf controller-stm32/BUILD/*
	rm -rf connection-test/BUILD/*
	rm -rf esp32/build/*
	rm -rf controller-stm32/src/usb_rs/target/*