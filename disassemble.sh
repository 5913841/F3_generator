sudo gdb -q -batch -ex "disassemble thread_main" ./F3_generator > disassemble_F3
sudo gdb -q -batch -ex "disassemble thread_main" ./build/trail/client_cps_core > disassemble_trail
sudo gdb -q -batch -ex "disassemble get_packet_l4_protocol" ./F3_generator > disassemble_F3_getpktl4protocol
sudo gdb -q -batch -ex "disassemble get_packet_l4_protocol" ./build/trail/client_cps_core > disassemble_trail_getpktl4protocol
sudo gdb -q -batch -ex "disassemble TCP::process" \
                   -ex "disassemble tcp_client_process" \
                   -ex "disassemble tcp_server_process" \
                   -ex "disassemble tcp_client_process_data" \
                   -ex "disassemble tcp_server_process_data" \
                   -ex "disassemble tcp_process_fin" \
                   -ex "disassemble tcp_client_process_syn_ack" \
                   -ex "disassemble tcp_server_process_syn" \
                   -ex "disassemble tcp_check_sequence" \
                   -ex "disassemble tcp_reply_more" \
                    ./F3_generator > disassemble_F3_TCP
sudo gdb -q -batch -ex "disassemble TCP::process" \
                   -ex "disassemble tcp_client_process" \
                   -ex "disassemble tcp_server_process" \
                   -ex "disassemble tcp_client_process_data" \
                   -ex "disassemble tcp_server_process_data" \
                   -ex "disassemble tcp_process_fin" \
                   -ex "disassemble tcp_client_process_syn_ack" \
                   -ex "disassemble tcp_server_process_syn" \
                   -ex "disassemble tcp_check_sequence" \
                   -ex "disassemble tcp_reply_more" \
                    ./build/trail/client_cps_core > disassemble_trail_TCP
sudo gdb -q -batch -ex "disassemble SocketPointerTable::find_socket(Socket*)" ./F3_generator > disassemble_F3_findsocket
sudo gdb -q -batch -ex "disassemble SocketPointerTable::find_socket(Socket*)" ./build/trail/client_cps_core > disassemble_trail_findsocket
sudo gdb -q -batch -ex "info types" ./F3_generator > gdb_types