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

### Web interface configuration

The title shown in the browser and at the top of the web page is
configured with:

```uci
config web
    option sitename 'Temperaturen Bestensee'
```

The `sitename` value is used dynamically by the web interface.

### Sensor configuration

Example sensor configuration:

```uci
config sensor
    option topic '/temperaturen/sens/sol_v'
    option name 'sens_sol_v'
    option label 'Solar Vorlauf'
    option graphname 'Solar'

config sensor
    option topic '/temperaturen/sens/sol_r'
    option name 'sens_sol_r'
    option label 'Solar Rücklauf'
    option graphname 'Solar'
```

The options have the following purposes:

- `topic` - MQTT topic from which the temperature value is received
- `name` - internal sensor name and RRD database name
- `label` - human-readable sensor name used in the graph legend
- `graphname` - graph title and assignment of the sensor to a graph

Sensors using the same `graphname` are displayed together in the
same graph.

For example:

```uci
option graphname 'Solar'
```

on two sensor sections displays both sensors in one graph named
`Solar`.

Up to two sensors are currently displayed in one graph.

The graph list is generated dynamically from the configured
`graphname` values. Therefore no sensor or graph names need to be
hard-coded in the HTML page or CGI script.

Adding or changing graph groups normally requires only a change to
the UCI configuration.

## RRD storage

The directory containing the RRD databases is configured separately:

```uci
config rrd
    option path '/mnt/data/mqtt-tempd'
```

The path may point to internal storage or to a mounted external
filesystem.

The configured directory must exist before `mqtt-tempd` is started.

mqtt-tempd intentionally uses the configured path and does not
currently create the directory automatically.

Each configured sensor has its own RRD database.

The database filename is derived from the sensor `name` option.

For example:

```uci
option name 'sens_ww'
```

creates or uses:

```text
sens_ww.rrd
```

inside the configured RRD directory.

The databases contain several consolidation levels so that
temperature history can be retained efficiently over longer periods.

## Web interface

The package installs the web interface as:

```text
/www/temperatur.html
/www/cgi-bin/tempread
```

The web interface uses the OpenWrt uhttpd web server.

If uhttpd is not already installed, it must be installed separately
to use the browser-based graphs.

The web page retrieves its configuration dynamically from
`/etc/config/mqtt-tempd`.

The following information is therefore not hard-coded in the HTML
page:

- site title
- graph names
- graph count
- sensor assignments
- sensor labels
- RRD storage path

The CGI script reads the configured graph names and sensor
assignments directly from UCI.

The browser creates the required graph areas dynamically.

The graphs support the following time ranges:

- 1 hour
- 24 hours
- 7 days
- 30 days
- 1 year

The graphical rendering is performed in the browser using
HTML5 Canvas.

## CGI interface

The CGI script can also be called directly.

To retrieve the configured site name and graph list:

```text
/cgi-bin/tempread?list=1
```

Example response:

```json
{
    "sitename": "Temperaturen Bestensee",
    "graphs": [
        "Warmwasser",
        "Solar",
        "Speicher",
        "Außentemperatur"
    ]
}
```

To retrieve data for a graph:

```text
/cgi-bin/tempread?graph=Solar&range=24h
```

Supported ranges are:

```text
1h
24h
7d
30d
1y
```

## Notes

The included binary packages were built specifically for the tested
LeMaker Banana Pi M1 running OpenWrt 22.03.5.

For other OpenWrt versions or target architectures, mqtt-tempd and
the supplied RRDtool packages should be rebuilt from source.

## TODO

- Consider improved handling of unavailable or not yet mounted RRD
  storage.

- Consider automatic handling of the configured RRD directory.
  Currently the directory specified by `rrd.path` must already exist.
