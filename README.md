# ESP8266


## Buil
To build the project use `./build.sh`.
To build and flash use `./flash.sh`.


## Preparte tolchain
Instruction can be found at 
	- https://github.com/espressif/esp8266_at/wiki/Toolchain
	- https://github.com/esp8266/esp8266-wiki/wiki/Toolchain
	
## Connection
Esp is a wifi station.
Esp ip address is permantly set.
You can change accespoint using uart.

## Comunicaton
Esp is a TCP server, and receiv mesege from tcp client. Example of msg is `{id|request}` and in response espe send `{id|response}`, where id is a integer. 
If request is special msg then the esp serv this msg, and send response. When request is not special request, then the msg body will be sent to uart client, example of msg: `<client_id, size_of_data>{request}`. After reciwing request by uart colient, the uart client send response, example of response: `{response}`. 
Uart client can also send own msg widouth request, example `<id_of_tcp_client, size_of_data>{special_command_name|data}`.

Important: The characters `{}` shuld by count when counting size of data, example: `szieof({as}) = 4`.

Special commands ar listened in `user_special_commands.h`.
Uart have also special commands.