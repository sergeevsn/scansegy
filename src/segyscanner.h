#ifndef SEGYSCANNER_H
#define SEGYSCANNER_H

#include <string>
#include <vector>
#include <set>
#include <map>
#include <memory>
#include "basetypes.h"
#include "segyread/SegyReader.hpp"

class SegyScanner {
public:
    SegyScanner();
    ~SegyScanner() = default;
    
    // Main processing function
    int process(const std::string& input_path, const std::set<std::string>& domains = {"sou", "rec", "cdp"});
    
private:
    // File discovery and validation
    std::vector<std::string> discoverFiles(const std::string& input_path);
    bool validateFile(const std::string& filepath);
    
    // Directory management
    void createOutputDirectories(const std::string& base_path);
    
    // SEG-Y file analysis
    struct FileInfo {
        std::string filename;
        int num_traces;
        int num_samples;
        int sample_interval_ms;  // в миллисекундах
        int max_time_ms;         // максимальное время в миллисекундах
    };
    
    FileInfo analyzeFile(const std::string& filepath);
    
    // Data extraction and processing
    struct TraceData {
        int32_t ffid;
        int32_t trace_number;
        int32_t cdp;
        int32_t source;
        int32_t sou_x, sou_y, sou_elev;
        int32_t rec_x, rec_y, rec_elev;
        int32_t cdp_x, cdp_y;
        int32_t iline, xline;
    };
    
    std::vector<TraceData> extractTraceData(const std::string& filepath, int num_traces);
    
    // Table generation
    void generateInfoTable(const std::string& output_dir, const std::vector<std::string>& processed_files);
    void generateRangesTable(const std::string& output_dir, const std::vector<std::string>& processed_files);
    void generateSourceTable(const std::string& output_dir, const std::string& filename, const std::vector<TraceData>& traces);
    void generateReceiverTable(const std::string& output_dir, const std::string& filename, const std::vector<TraceData>& traces);
    void generateCdpTable(const std::string& output_dir, const std::string& filename, const std::vector<TraceData>& traces);
    
    // Map generation
    void generateMaps(const std::string& output_dir, const std::vector<std::string>& processed_files, const std::set<std::string>& domains);
    
    // Utility functions
    std::string getFilenameWithoutPath(const std::string& filepath);
    std::string getFilenameWithoutExtension(const std::string& filepath);
    void writeTableHeader(std::ofstream& file, const std::vector<std::string>& headers, const std::vector<int>& column_widths);
    void writeTableRow(std::ofstream& file, const std::vector<std::string>& values, const std::vector<int>& column_widths);
    std::vector<int> calculateColumnWidths(const std::vector<std::string>& headers, const std::vector<std::vector<std::string>>& data);
    std::string formatCell(const std::string& value, int width);
    void calculateRanges(const std::string& filename, const std::vector<TraceData>& traces);
    
    // Data storage for map generation and ranges
    std::map<std::string, std::set<SourceInfo>> all_sources_;
    std::map<std::string, std::set<ReceiverInfo>> all_receivers_;
    std::map<std::string, std::set<CdpInfo>> all_cdps_;
    std::map<std::string, std::vector<TraceData>> all_traces_;
    std::map<std::string, FileInfo> all_file_info_;
    
    // Range calculation
    struct Range {
        int32_t min_val, max_val;
        Range() : min_val(0), max_val(0) {}
        Range(int32_t min, int32_t max) : min_val(min), max_val(max) {}
        std::string toString() const {
            if (min_val == max_val) {
                return std::to_string(min_val);
            }
            return std::to_string(min_val) + "-" + std::to_string(max_val);
        }
    };
    
    std::map<std::string, std::map<std::string, Range>> header_ranges_;
};

#endif // SEGYSCANNER_H
