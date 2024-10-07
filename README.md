Server side runs: 
./server -r <port_number> -m <buffer_capacity>

Client side runs: 
Transfer data points:
./client -n <# data items> -w <# workers> -b <BoundedBuffer_size> -p <# patients> -h <# histogram threads> -m <buffer_capacity> -a <IP_address> -r <port_number>

File Transfer:
./client -w <# workers> -b <BoundedBuffer_size> -m <buffer_capacity> -f <filename> -a <IP_address> -r <port_number>
