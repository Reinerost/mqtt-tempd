# mqtt-tempd

MQTT temperature logger for OpenWrt with RRD storage and
browser-based temperature graphs.

The software receives temperature values via MQTT, stores them
in RRD databases and provides a simple browser-based graphical
display using uhttpd, a CGI script and HTML5 Canvas.

## Tested platform

mqtt-tempd was developed and tested on:

- LeMaker Banana Pi M1
- OpenWrt 22.03.5 (r20134-5f15225c1e)
- Target: `sunxi/cortexa7`
- Architecture: `arm_cortex-a7_neon-vfpv4`

Other OpenWrt devices may work as well, but have not been tested.

## Packages

Precompiled OpenWrt packages for the tested platform are included
in the `packages/` directory.

The following packages must be installed:

- `librrd`
- `rrdtool`
- `mqtt-tempd`

The supplied `librrd` and `rrdtool` packages contain RRDtool 1.10.3.

The precompiled packages were built specifically for:

- OpenWrt 22.03.5
- Target `sunxi/cortexa7`
- Architecture `arm_cortex-a7_neon-vfpv4`

They should not be installed on devices using an incompatible
OpenWrt release or target architecture.

Install the packages in this order:

```sh
opkg install librrd_*.ipk
opkg install rrdtool_*.ipk
opkg install mqtt-tempd_*.ipk
```

Additional dependencies required by these packages are expected
to be available from the configured OpenWrt package repositories.

Individual SHA256 checksum files for the supplied packages are
included in the `packages/` directory.

The checksums can be verified, for example, with:

```sh
sha256sum -c librrd.sha256
sha256sum -c rrdtool.sha256
sha256sum -c mqtt-tempd.sha256
```

## Configuration

The configuration file is:

```text
/etc/config/mqtt-tempd
```

Example sensor configuration:

```uci
config sensor
    option topic '/temperaturen/sens/ww'
    option name  'sens_ww'
    option label 'Warmwasser'
    option graph 'warmwasser'
```

The options have the following purposes:

- `topic` - MQTT topic from which the temperature value is received
- `name` - internal sensor name and RRD database name
- `label` - human-readable name used in the graphs
- `graph` - assigns the sensor to a graph

Up to two sensors can be assigned to the same graph by using the
same `graph` value.

The RRD storage directory is configured with:

```uci
config rrd
    option path '/var/lib/mqtt-tempd'
```

## Web interface

The package installs the web interface as:

```text
/www/temperatur.html
/www/cgi-bin/tempread
```

The web interface uses the OpenWrt uhttpd web server.

If uhttpd is not already installed, it must be installed separately
to use the browser-based graphs.

The graphs support the following time ranges:

- 1 hour
- 24 hours
- 7 days
- 30 days
- 1 year

The CGI script reads the RRD path, sensor names, labels and graph
assignments directly from the UCI configuration.

This means that sensor configuration does not need to be duplicated
in the CGI script.

## RRD storage

Temperature values are stored in RRD databases using RRDtool.

Each configured sensor has its own RRD database. The database name
is derived from the sensor `name` option.

For example:

```uci
option name 'sens_ww'
```

creates/uses:

```text
sens_ww.rrd
```

inside the configured RRD directory.

The databases contain several consolidation levels so that
temperature history can be retained efficiently over longer periods.

## Notes

The included binary packages were built specifically for the tested
LeMaker Banana Pi M1 running OpenWrt 22.03.5.

For other OpenWrt versions or target architectures, mqtt-tempd and
the supplied RRDtool packages should be rebuilt from source. 

## TODO

- Handle creation of the configured RRD directory automatically.
  Currently, the directory specified by `rrd.path` must exist before
  `mqtt-tempd` is started.

- Handle unavailable or not yet mounted storage gracefully.
  This is especially relevant when the RRD database is stored on an
  external disk that may be mounted after the daemon is started.
