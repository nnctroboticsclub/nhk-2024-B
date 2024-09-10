si:
	[ -z "$(DEV)" ] && { echo "\$$DEV Not specified"; exit 1; } || :

	stty -F $(DEV) speed 19200 -inlcr igncr onlcr -echo
	socat stdio $(DEV)