# MAVLink Log Format

*QGroundControl* allows you to generate plain MAVLink packet logs that can be 
replayed (with QGroundControl) to watch a mission again for analysis.

The format is binary:

* Byte 1-8: Timestamp in microseconds since Unix epoch as unsigned 64 bit integer
* Byte 9-271: MAVLink packet (263 bytes maximum packet length, not all bytes have to be actual data, the packet might be shorter. Includes packet start sign)

## Debugging

To check your data, open your written file in a hex editor. You should see after 8 bytes **0x55**. The first 8 bytes should also convert to a valid timestamp, so something either close to zero or around the number **1294571828792000** (which is the current Unix epoch timestamp in microseconds).

## C++ Sketch for logging MAVLink

The code fragment below shows how to implement logging using [C++ streams](http://www.cplusplus.com/reference/iostream/istream/) from the C++ standard library. 

```cpp
//write into mavlink logfile
const int len = MAVLINK_MAX_PACKET_LEN+sizeof(uint64_t);
uint8_t buf[len];
uint64_t time = getSystemTimeUsecs();
memcpy(buf, (void*)&time, sizeof(uint64_t));
mavlink_msg_to_send_buffer(buf+sizeof(uint64_t), msg);
mavlinkFile << buf << flush;
```