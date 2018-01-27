# AirMap Monitor Service {#monitord}

The AirMap monitor service monitors the state of vehicles and takes
action based on changes to this state:

 - when the state of a vehicle changes to active:
   - create a flight with the AirMap services
   - start flight communications
   - transmit telemetry updates to AirMap
   - receive updates for manned and unmanned aerial traffic relevant to a flight
 - when the state of a vehicle changes to inactive:
   - stop flight communications
   - end flight

The daemon exposes its functionality via a gRPC interface (see
`${AIRMAPD_ROOT}/interfaces/grpc/airmap/monitor/monitor.proto`). Client
applications can either rely on the C++-API avaiable in
`${AIRMAPD_ROOT}/include/airmap/monitor/client.h` or rely on the gRPC
ecosystem to easily connect to the daemon in their choice of language
and runtime. Please see `${AIRMAPD_ROOT/examples/monitor/client.cpp` for
an example of using the C++-API. The following diagram summarizes the overall setup:

![monitord](doc/images/monitord.png)

# Service Configuration

The service is executed with the following command:

```
$ airmap daemon 
```

Please note that the service needs exactly one MavLink endpoint to be configured on the command line.
The following endpoint types are supported:
 - TCP: Provide `--tcp-endpoint-ip=IP` and `--tcp-endpoint-port=PORT` to `airmap daemon`.
 - UDP: Provide `--udp-endpoint-port=PORT` to `airmap daemon`.
 - Serial: Provide `--serial-device=PATH/TO/DEVICE` to `airmap daemon`.

The gRPC endpoint exported by the service can be specified with `--grpc-endpoint=ENDPOINT`, defaulting to `0.0.0.0:9090`.
The overall daemon configuration for accessing the AirMap services can be specified with `--config-file=PATH/TO/CONFIG/FILE`.
By default, the config file is expected in `~/.config/airmap/production/config.json`.