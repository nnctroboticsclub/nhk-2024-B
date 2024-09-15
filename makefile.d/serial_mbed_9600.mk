st9:
	[ -z "$(DEV)" ] && { echo "\$$DEV Not specified"; exit 1; } || :

	stty -F $(DEV) \
		-brkint -icrnl -imaxbel -opost \
		-onlcr -isig -icanon -iexten \
		-echo -echoe -echok -echoctl \
		-echoke speed 9600
	socat stdio $(DEV)