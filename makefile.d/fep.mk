f_s:
	bash scripts/fep.sh

f_t:
	bash scripts/fep-test.sh

f_w:
	cd syoch-robotics; wireshark -X lua_script:wireshark/robo.lua -k -i <(cat output.pcap)
