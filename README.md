# scansegy

Application for analyzing SEG-Y format files with generation of statistical tables and maps of source, receiver, and CDP locations.

## Features

- **SEG-Y File Analysis**: Reads binary headers and trace headers from SEG-Y files
- **Statistical Tables**: Generates comprehensive tables with file information and header ranges
- **Location Maps**: Creates scatter plots for source, receiver, and CDP positions
- **Selective Analysis**: Choose specific domains (sources, receivers, CDPs) for analysis
- **Batch Processing**: Process single files or entire directories
- **Cross-Platform**: Works on Linux, macOS, and Windows

## Requirements

- **C++11** or later
- **CMake** 3.16 or later
- **matplotlibplusplus** (included in the project)
- **OpenMP** (for parallel processing)

## Installation

### Building from Source

```bash
# Clone the repository
git clone <repository-url>
cd scansegy

# Create build directory
mkdir build
cd build

# Configure and build
cmake ..
make -j4

# The executable will be in build/scansegy
```

### Dependencies

Put `matplotlibplusplus` repository to project's folder before building

## Usage

### Basic Usage

```bash
# Process a single file (all domains)
./build/scansegy data/survey.sgy

# Process a directory (all domains)
./build/scansegy data/

# Show help
./build/scansegy --help
```

### Selective Domain Analysis

```bash
# Sources only
./build/scansegy -sou data/survey.sgy

# Receivers only
./build/scansegy -rec data/survey.sgy

# CDP only
./build/scansegy -cdp data/survey.sgy

# Domain combinations
./build/scansegy -sou -rec data/survey.sgy
./build/scansegy -sou -cdp data/survey.sgy
./build/scansegy -rec -cdp data/survey.sgy
```

### Command Line Parameters

| Parameter | Description |
|-----------|-------------|
| `-sou` | Generate source tables and maps |
| `-rec` | Generate receiver tables and maps |
| `-cdp` | Generate CDP tables and maps |
| `-h, --help` | Show help message |

**Note**: If no domain options are specified, all domains are generated. Options can be combined.

## Output Files

The program creates a `segyscan` directory with the following structure:

```
segyscan/
├── tables/
│   ├── info.txt          # Global file information table
│   ├── ranges.txt        # Global header ranges table
│   ├── sou.txt           # Source statistics table
│   ├── rec.txt           # Receiver statistics table
│   └── cdp.txt           # CDP statistics table
└── maps/
    ├── sou_map.png       # Source location map
    ├── rec_map.png       # Receiver location map
    └── cdp_map.png       # CDP location map
```

### File Descriptions

#### `info.txt`
Global table with information about all processed files:
- `file_name`: Name of the SEG-Y file
- `num_traces`: Number of traces in the file
- `num_samples`: Number of samples per trace
- `sample_interval_ms`: Sample interval in milliseconds
- `max_time_ms`: Maximum time in milliseconds

#### `ranges.txt`
Global table with min-max ranges of header fields across all files:
- First column: Header field names
- Subsequent columns: File names
- Each cell contains min-max range (e.g., "1-100")
- Tables are split into chunks of 5 files for readability

#### `sou.txt`, `rec.txt`, `cdp.txt`
Statistical tables for each domain:
- **sou.txt**: Source statistics (FFID, coordinates, elevation)
- **rec.txt**: Receiver statistics (channel, coordinates, elevation)
- **cdp.txt**: CDP statistics (CDP number, coordinates, inline/crossline)

#### Maps (`*.png`)
Scatter plots showing spatial distribution:
- **sou_map.png**: Source locations (X, Y coordinates)
- **rec_map.png**: Receiver locations (X, Y coordinates)
- **cdp_map.png**: CDP locations (X, Y coordinates)

## Examples

### Single File Analysis

```bash
# Full analysis
./build/scansegy data/survey_2023.sgy

# Sources and receivers only
./build/scansegy -sou -rec data/survey_2023.sgy
```

### Directory Analysis

```bash
# All files in directory
./build/scansegy data/surveys/

# CDP only for all files
./build/scansegy -cdp data/surveys/
```

### Batch Processing

```bash
# Process all SEG-Y files in current directory
./build/scansegy .

# Process with domain selection
./build/scansegy -sou -rec data/raw/
```

## Technical Details

### Supported SEG-Y Formats

- **Standard SEG-Y**: IBM floating-point format
- **IEEE SEG-Y**: IEEE floating-point format
- **Byte Order**: Both big-endian and little-endian
- **Trace Headers**: Standard 240-byte trace headers

### Header Fields Analyzed

- **FFID**: Field Record Number
- **Chan**: Channel Number
- **CDP**: Common Depth Point
- **Source**: Source Point Number
- **SourceX/Y**: Source coordinates
- **SourceElevation**: Source elevation
- **ReceiverX/Y**: Receiver coordinates
- **ReceiverElevation**: Receiver elevation
- **CDP_X/Y**: CDP coordinates
- **ILINE_3D**: 3D Inline number
- **CROSSLINE_3D**: 3D Crossline number

### Performance

- **Parallel Processing**: Uses OpenMP for multi-threaded analysis
- **Memory Efficient**: Processes files sequentially to minimize memory usage
- **Fast I/O**: Optimized file reading with minimal overhead

## Troubleshooting

### Common Issues

1. **"Cannot open file" error**
   - Check file path and permissions
   - Ensure SEG-Y file is not corrupted

2. **"No SEG-Y files found"**
   - Verify file extensions (.sgy, .segy)
   - Check directory path

3. **Build errors**
   - Ensure C++11 compiler is available
   - Check CMake version (3.16+)
   - Verify all dependencies are installed

4. **Empty output tables**
   - Check if SEG-Y file contains valid trace headers
   - Verify coordinate fields are populated

### Getting Help

- Check the help message: `./build/scansegy --help`
- Verify SEG-Y file format compatibility
- Check file permissions and paths

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests if applicable
5. Submit a pull request

## Changelog

### Version 1.0.0
- Initial release
- SEG-Y file analysis
- Statistical table generation
- Location map creation
- Selective domain analysis
- Batch processing support