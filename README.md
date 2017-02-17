# GREISTools

The tools for manipulating Javad GNSS receiver data

## Getting Started

There are currently 4 tools in the toolset:
* jps2db - a MySQL loader for raw data, unmaintained
* db2jps - an export tool for MySQL GNSS data, unmaintained
* jps2jps - a manipulation tool
* jpsdump - an examination tool

## Manipulation tool --- jps2jps

To primary goal of this program is to process a receiver raw data and perform time-windowing. The tools also places volume metadata and required version info in output files for compatibility.

```
jps2jps [OPTIONS] input-file
```
Available options are:
* ```-X --debug``` --- enable diagnostics output
* ```-A --accurate``` --- perform time-windowing more precisely by copying messages that are required to interpret observation data but belong to epochs that do not fit the time window
* ```-t --thorough``` --- perform time-windowing through the whole file
* ```--crlf``` --- add CR and LF after each message, some software(e.g. for RTKLIB) aligns to linefeeds
* ```-o=output-file --output=output-file``` --- specify the output file
* ```--start=YYYY-MM-DDThh:mm:ssZ``` --- specify the start of time-window of interest
* ```--stop=YYYY-MM-DDThh:mm:ssZ``` --- specify the end of time-window of interest


```input-file``` is readable file with at least one epoch formatted according to GREIS 

## Examination tool --- jpsdump

This tool reads the GREIS-formatted file and outputs specific information on it's contents.
```
jpsdump [OPTIONS] input-file
```
Available options are:
* ```-X --debug``` --- enable diagnostics output
* ```-V --verbose``` --- shows more details
* ```-t --thorough``` --- perform time-windowing through the whole file
* ```--start=YYYY-MM-DDThh:mm:ssZ``` --- specify the start of time-window of interest
* ```--stop=YYYY-MM-DDThh:mm:ssZ``` --- specify the end of time-window of interest

```input-file``` is readable file with at least one epoch formatted according to GREIS 

## License

This project is licensed under the LGPLv3 License - see the [LICENSE](LICENSE) file for details

