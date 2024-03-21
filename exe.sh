nohup sudo ./build/THUGEN > THUGEN.log 2>&1 &
sleep 0.1
sudo ./build/dumpcap -P -i 0000:01:00.1 -w /home/pg/THUGEN/test.pcap

