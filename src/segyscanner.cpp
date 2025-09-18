#include "segyscanner.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <matplot/matplot.h>

using namespace matplot;

SegyScanner::SegyScanner() = default;

int SegyScanner::process(const std::string& input_path, const std::set<std::string>& domains) {
    try {
        // Step 1: Discover files
        std::cout << "Discovering SEG-Y files..." << std::endl;
        auto files = discoverFiles(input_path);
        if (files.empty()) {
            std::cerr << "No valid SEG-Y files found in: " << input_path << std::endl;
            return 1;
        }
        
        std::cout << "Found " << files.size() << " SEG-Y files" << std::endl;
        
        // Step 2: Create output directories
        std::string output_base = std::filesystem::is_directory(input_path) ? 
            input_path + "/segyscan" : 
            std::filesystem::path(input_path).parent_path().string() + "/segyscan";
        
        createOutputDirectories(output_base);
        
        // Step 3: Process each file
        std::vector<std::string> processed_files;
        for (const auto& filepath : files) {
            std::cout << "Processing: " << filepath << std::endl;
            
            try {
                // Analyze file
                auto file_info = analyzeFile(filepath);
                std::string filename = getFilenameWithoutExtension(filepath);
                
                // Store file info for global table
                all_file_info_[filename] = file_info;
                
                // Extract trace data
                auto traces = extractTraceData(filepath, file_info.num_traces);
                
                // Store traces for ranges calculation
                all_traces_[filename] = traces;
                
                // Calculate ranges for this file
                calculateRanges(filename, traces);
                
                // Generate domain-specific tables based on selection
                if (domains.find("sou") != domains.end()) {
                    generateSourceTable(output_base + "/tables", filename, traces);
                }
                if (domains.find("rec") != domains.end()) {
                    generateReceiverTable(output_base + "/tables", filename, traces);
                }
                if (domains.find("cdp") != domains.end()) {
                    generateCdpTable(output_base + "/tables", filename, traces);
                }
                
                processed_files.push_back(filename);
                
            } catch (const std::exception& e) {
                std::cerr << "Error processing " << filepath << ": " << e.what() << std::endl;
                continue;
            }
        }
        
        // Step 4: Generate info and ranges tables
        if (!processed_files.empty()) {
            std::cout << "Generating info table..." << std::endl;
            generateInfoTable(output_base + "/tables", processed_files);
            
            std::cout << "Generating ranges table..." << std::endl;
            generateRangesTable(output_base + "/tables", processed_files);
        }
        
        // Step 5: Generate maps
        if (!processed_files.empty()) {
            std::cout << "Generating maps..." << std::endl;
            generateMaps(output_base + "/maps", processed_files, domains);
        }
        
        std::cout << "Processing completed successfully!" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}

std::vector<std::string> SegyScanner::discoverFiles(const std::string& input_path) {
    std::vector<std::string> files;
    
    if (std::filesystem::is_regular_file(input_path)) {
        if (validateFile(input_path)) {
            files.push_back(input_path);
        }
    } else if (std::filesystem::is_directory(input_path)) {
        for (const auto& entry : std::filesystem::directory_iterator(input_path)) {
            if (entry.is_regular_file()) {
                std::string ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                
                if ((ext == ".sgy" || ext == ".segy") && validateFile(entry.path().string())) {
                    files.push_back(entry.path().string());
                }
            }
        }
    } else {
        throw std::runtime_error("Input path does not exist: " + input_path);
    }
    
    return files;
}

bool SegyScanner::validateFile(const std::string& filepath) {
    try {
        auto file_size = std::filesystem::file_size(filepath);
        return file_size > 3600;
    } catch (const std::exception&) {
        return false;
    }
}

void SegyScanner::createOutputDirectories(const std::string& base_path) {
    std::filesystem::create_directories(base_path);
    std::filesystem::create_directories(base_path + "/tables");
    std::filesystem::create_directories(base_path + "/maps");
}

SegyScanner::FileInfo SegyScanner::analyzeFile(const std::string& filepath) {
    SegyReader reader(filepath);
    
    FileInfo info;
    info.filename = getFilenameWithoutPath(filepath);
    info.num_samples = static_cast<int>(reader.num_samples());
    info.sample_interval_ms = static_cast<int>(reader.sample_interval() * 1000); // конвертируем в миллисекунды
    info.num_traces = static_cast<int>(reader.num_traces());
    info.max_time_ms = (info.num_samples - 1) * info.sample_interval_ms; // максимальное время
    
    return info;
}

std::vector<SegyScanner::TraceData> SegyScanner::extractTraceData(const std::string& filepath, int num_traces) {
    SegyReader reader(filepath);
    std::vector<TraceData> traces;
    traces.reserve(num_traces);
    
    std::string filename = getFilenameWithoutPath(filepath);
    
    for (int i = 0; i < num_traces; ++i) {
        TraceData trace;
        
        // Extract header values using the field map
        trace.ffid = reader.get_header_value_i32(i, "FieldRecord");
        trace.trace_number = reader.get_header_value_i32(i, "TraceNumber");
        trace.cdp = reader.get_header_value_i32(i, "CDP");
        trace.source = reader.get_header_value_i32(i, "EnergySourcePoint");
        trace.sou_x = reader.get_header_value_i32(i, "SourceX");
        trace.sou_y = reader.get_header_value_i32(i, "SourceY");
        trace.sou_elev = reader.get_header_value_i32(i, "SourceElevation");
        trace.rec_x = reader.get_header_value_i32(i, "ReceiverX");
        trace.rec_y = reader.get_header_value_i32(i, "ReceiverY");
        trace.rec_elev = reader.get_header_value_i32(i, "ReceiverElevation");
        trace.cdp_x = reader.get_header_value_i32(i, "CDP_X");
        trace.cdp_y = reader.get_header_value_i32(i, "CDP_Y");
        trace.iline = reader.get_header_value_i32(i, "ILINE_3D");
        trace.xline = reader.get_header_value_i32(i, "CROSSLINE_3D");
        
        traces.push_back(trace);
        
        // Show progress every 100 traces or at the end
        if ((i + 1) % 100 == 0 || i == num_traces - 1) {
            print_progress_bar("Reading traces from " + filename, i + 1, num_traces);
        }
    }
    
    return traces;
}

void SegyScanner::calculateRanges(const std::string& filename, const std::vector<TraceData>& traces) {
    if (traces.empty()) return;
    
    // Initialize ranges with first trace
    const auto& first = traces[0];
    header_ranges_[filename]["FFID"] = Range(first.ffid, first.ffid);
    header_ranges_[filename]["Chan"] = Range(first.trace_number, first.trace_number);
    header_ranges_[filename]["CDP"] = Range(first.cdp, first.cdp);
    header_ranges_[filename]["Source"] = Range(first.source, first.source);
    header_ranges_[filename]["Sou_X"] = Range(first.sou_x, first.sou_x);
    header_ranges_[filename]["Sou_Y"] = Range(first.sou_y, first.sou_y);
    header_ranges_[filename]["Sou_Elev"] = Range(first.sou_elev, first.sou_elev);
    header_ranges_[filename]["Rec_X"] = Range(first.rec_x, first.rec_x);
    header_ranges_[filename]["Rec_Y"] = Range(first.rec_y, first.rec_y);
    header_ranges_[filename]["Rec_Elev"] = Range(first.rec_elev, first.rec_elev);
    header_ranges_[filename]["CDP_X"] = Range(first.cdp_x, first.cdp_x);
    header_ranges_[filename]["CDP_Y"] = Range(first.cdp_y, first.cdp_y);
    header_ranges_[filename]["ILINE"] = Range(first.iline, first.iline);
    header_ranges_[filename]["XLINE"] = Range(first.xline, first.xline);
    
    // Update ranges with remaining traces
    for (size_t i = 1; i < traces.size(); ++i) {
        const auto& trace = traces[i];
        
        auto& ranges = header_ranges_[filename];
        ranges["FFID"].min_val = std::min(ranges["FFID"].min_val, trace.ffid);
        ranges["FFID"].max_val = std::max(ranges["FFID"].max_val, trace.ffid);
        
        ranges["Chan"].min_val = std::min(ranges["Chan"].min_val, trace.trace_number);
        ranges["Chan"].max_val = std::max(ranges["Chan"].max_val, trace.trace_number);
        
        ranges["CDP"].min_val = std::min(ranges["CDP"].min_val, trace.cdp);
        ranges["CDP"].max_val = std::max(ranges["CDP"].max_val, trace.cdp);
        
        ranges["Source"].min_val = std::min(ranges["Source"].min_val, trace.source);
        ranges["Source"].max_val = std::max(ranges["Source"].max_val, trace.source);
        
        ranges["Sou_X"].min_val = std::min(ranges["Sou_X"].min_val, trace.sou_x);
        ranges["Sou_X"].max_val = std::max(ranges["Sou_X"].max_val, trace.sou_x);
        
        ranges["Sou_Y"].min_val = std::min(ranges["Sou_Y"].min_val, trace.sou_y);
        ranges["Sou_Y"].max_val = std::max(ranges["Sou_Y"].max_val, trace.sou_y);
        
        ranges["Sou_Elev"].min_val = std::min(ranges["Sou_Elev"].min_val, trace.sou_elev);
        ranges["Sou_Elev"].max_val = std::max(ranges["Sou_Elev"].max_val, trace.sou_elev);
        
        ranges["Rec_X"].min_val = std::min(ranges["Rec_X"].min_val, trace.rec_x);
        ranges["Rec_X"].max_val = std::max(ranges["Rec_X"].max_val, trace.rec_x);
        
        ranges["Rec_Y"].min_val = std::min(ranges["Rec_Y"].min_val, trace.rec_y);
        ranges["Rec_Y"].max_val = std::max(ranges["Rec_Y"].max_val, trace.rec_y);
        
        ranges["Rec_Elev"].min_val = std::min(ranges["Rec_Elev"].min_val, trace.rec_elev);
        ranges["Rec_Elev"].max_val = std::max(ranges["Rec_Elev"].max_val, trace.rec_elev);
        
        ranges["CDP_X"].min_val = std::min(ranges["CDP_X"].min_val, trace.cdp_x);
        ranges["CDP_X"].max_val = std::max(ranges["CDP_X"].max_val, trace.cdp_x);
        
        ranges["CDP_Y"].min_val = std::min(ranges["CDP_Y"].min_val, trace.cdp_y);
        ranges["CDP_Y"].max_val = std::max(ranges["CDP_Y"].max_val, trace.cdp_y);
        
        ranges["ILINE"].min_val = std::min(ranges["ILINE"].min_val, trace.iline);
        ranges["ILINE"].max_val = std::max(ranges["ILINE"].max_val, trace.iline);
        
        ranges["XLINE"].min_val = std::min(ranges["XLINE"].min_val, trace.xline);
        ranges["XLINE"].max_val = std::max(ranges["XLINE"].max_val, trace.xline);
    }
}

void SegyScanner::generateInfoTable(const std::string& output_dir, const std::vector<std::string>& processed_files) {
    std::string filepath = output_dir + "/info.txt";
    std::ofstream file(filepath);
    
    if (!file.is_open()) {
        throw std::runtime_error("Cannot create file: " + filepath);
    }
    
    if (processed_files.empty()) return;
    
    std::vector<std::string> headers = {"file_name", "num_traces", "num_samples", "sample_interval_ms", "max_time_ms"};
    
    // Prepare all data rows
    std::vector<std::vector<std::string>> data;
    for (const auto& filename : processed_files) {
        if (all_file_info_.find(filename) != all_file_info_.end()) {
            const auto& info = all_file_info_[filename];
            data.push_back({info.filename, std::to_string(info.num_traces), 
                           std::to_string(info.num_samples), std::to_string(info.sample_interval_ms),
                           std::to_string(info.max_time_ms)});
        }
    }
    
    auto column_widths = calculateColumnWidths(headers, data);
    
    writeTableHeader(file, headers, column_widths);
    for (const auto& row : data) {
        writeTableRow(file, row, column_widths);
    }
}

void SegyScanner::generateRangesTable(const std::string& output_dir, const std::vector<std::string>& processed_files) {
    if (processed_files.empty()) return;
    
    std::string filepath = output_dir + "/ranges.txt";
    std::ofstream file(filepath);
    
    if (!file.is_open()) {
        throw std::runtime_error("Cannot create file: " + filepath);
    }
    
    const int MAX_FILES_PER_TABLE = 5;
    
    // Process files in chunks of MAX_FILES_PER_TABLE
    for (size_t start = 0; start < processed_files.size(); start += MAX_FILES_PER_TABLE) {
        size_t end = std::min(start + MAX_FILES_PER_TABLE, processed_files.size());
        std::vector<std::string> current_files(processed_files.begin() + start, processed_files.begin() + end);
        
        // Add empty line separator before each table (except the first one)
        if (start > 0) {
            file << std::endl;
        }
        
        // Define header names in order
        std::vector<std::string> header_names = {"FFID", "Chan", "CDP", "Source", "Sou_X", "Sou_Y", 
                                                "Sou_Elev", "Rec_X", "Rec_Y", "Rec_Elev", "CDP_X", "CDP_Y", "ILINE", "XLINE"};
        
        // Prepare headers: "Header" + file names
        std::vector<std::string> headers = {"Header"};
        for (const auto& filename : current_files) {
            headers.push_back(filename);
        }
        
        // Prepare data rows
        std::vector<std::vector<std::string>> data;
        for (const auto& header_name : header_names) {
            std::vector<std::string> row = {header_name};
            for (const auto& filename : current_files) {
                if (header_ranges_.find(filename) != header_ranges_.end() && 
                    header_ranges_[filename].find(header_name) != header_ranges_[filename].end()) {
                    row.push_back(header_ranges_[filename][header_name].toString());
                } else {
                    row.push_back("N/A");
                }
            }
            data.push_back(row);
        }
        
        auto column_widths = calculateColumnWidths(headers, data);
        
        writeTableHeader(file, headers, column_widths);
        for (const auto& row : data) {
            writeTableRow(file, row, column_widths);
        }
    }
}

void SegyScanner::generateSourceTable(const std::string& output_dir, const std::string& filename, const std::vector<TraceData>& traces) {
    std::set<SourceInfo> unique_sources;
    
    for (const auto& trace : traces) {
        SourceInfo source;
        source.ffid = trace.ffid;
        source.source = trace.source;
        source.sou_x = trace.sou_x;
        source.sou_y = trace.sou_y;
        source.sou_elev = trace.sou_elev;
        unique_sources.insert(source);
    }
    
    // Store for map generation
    all_sources_[filename] = unique_sources;
    
    // Write table
    std::string filepath = output_dir + "/" + filename + "_sou.txt";
    std::ofstream file(filepath);
    
    if (!file.is_open()) {
        throw std::runtime_error("Cannot create file: " + filepath);
    }
    
    std::vector<std::string> headers = {"Number", "FFID", "Source", "Sou_X", "Sou_Y", "Sou_Elev"};
    
    // Prepare all data rows
    std::vector<std::vector<std::string>> data;
    int number = 1;
    for (const auto& source : unique_sources) {
        data.push_back({std::to_string(number), std::to_string(source.ffid), 
                       std::to_string(source.source), std::to_string(source.sou_x),
                       std::to_string(source.sou_y), std::to_string(source.sou_elev)});
        number++;
    }
    
    auto column_widths = calculateColumnWidths(headers, data);
    
    writeTableHeader(file, headers, column_widths);
    for (const auto& row : data) {
        writeTableRow(file, row, column_widths);
    }
}

void SegyScanner::generateReceiverTable(const std::string& output_dir, const std::string& filename, const std::vector<TraceData>& traces) {
    std::set<ReceiverInfo> unique_receivers;
    
    for (const auto& trace : traces) {
        ReceiverInfo receiver;
        receiver.rec_x = trace.rec_x;
        receiver.rec_y = trace.rec_y;
        receiver.rec_elev = trace.rec_elev;
        unique_receivers.insert(receiver);
    }
    
    // Store for map generation
    all_receivers_[filename] = unique_receivers;
    
    // Write table
    std::string filepath = output_dir + "/" + filename + "_rec.txt";
    std::ofstream file(filepath);
    
    if (!file.is_open()) {
        throw std::runtime_error("Cannot create file: " + filepath);
    }
    
    std::vector<std::string> headers = {"Number", "Rec_X", "Rec_Y", "Rec_Elev"};
    
    // Prepare all data rows
    std::vector<std::vector<std::string>> data;
    int number = 1;
    for (const auto& receiver : unique_receivers) {
        data.push_back({std::to_string(number), std::to_string(receiver.rec_x),
                       std::to_string(receiver.rec_y), std::to_string(receiver.rec_elev)});
        number++;
    }
    
    auto column_widths = calculateColumnWidths(headers, data);
    
    writeTableHeader(file, headers, column_widths);
    for (const auto& row : data) {
        writeTableRow(file, row, column_widths);
    }
}

void SegyScanner::generateCdpTable(const std::string& output_dir, const std::string& filename, const std::vector<TraceData>& traces) {
    std::set<CdpInfo> unique_cdps;
    
    for (const auto& trace : traces) {
        CdpInfo cdp;
        cdp.cdp = trace.cdp;
        cdp.cdp_x = trace.cdp_x;
        cdp.cdp_y = trace.cdp_y;
        unique_cdps.insert(cdp);
    }
    
    // Store for map generation
    all_cdps_[filename] = unique_cdps;
    
    // Write table
    std::string filepath = output_dir + "/" + filename + "_cdp.txt";
    std::ofstream file(filepath);
    
    if (!file.is_open()) {
        throw std::runtime_error("Cannot create file: " + filepath);
    }
    
    std::vector<std::string> headers = {"Number", "CDP", "CDP_X", "CDP_Y"};
    
    // Prepare all data rows
    std::vector<std::vector<std::string>> data;
    int number = 1;
    for (const auto& cdp : unique_cdps) {
        data.push_back({std::to_string(number), std::to_string(cdp.cdp),
                       std::to_string(cdp.cdp_x), std::to_string(cdp.cdp_y)});
        number++;
    }
    
    auto column_widths = calculateColumnWidths(headers, data);
    
    writeTableHeader(file, headers, column_widths);
    for (const auto& row : data) {
        writeTableRow(file, row, column_widths);
    }
}

void SegyScanner::generateMaps(const std::string& output_dir, const std::vector<std::string>& processed_files, const std::set<std::string>& domains) {
    std::vector<std::string> colors = {"b", "r", "g", "m", "c", "y", "k"};
    
    // Generate sources map
    if (domains.find("sou") != domains.end()) {
        auto f1 = figure(true);
        f1->position(0, 0, 1200, 800);
        hold(on);
        
        std::vector<std::string> legend_labels;
        
        for (size_t i = 0; i < processed_files.size(); ++i) {
            const auto& filename = processed_files[i];
            const auto& sources = all_sources_[filename];
            
            if (sources.empty()) continue;
            
            std::vector<double> x, y;
            for (const auto& source : sources) {
                x.push_back(static_cast<double>(source.sou_x));
                y.push_back(static_cast<double>(source.sou_y));
            }
            
            auto h = scatter(x, y);
            h->marker_size(2);
            h->color(colors[i % colors.size()]);
            h->marker_face(true);
            h->display_name(filename);
            
            legend_labels.push_back(filename);
        }
        
        title("Source Locations");
        xlabel("X Coordinate");
        ylabel("Y Coordinate");
        gca()->x_axis().tick_label_format("%.0f");
        gca()->y_axis().tick_label_format("%.0f");
        
        if (!legend_labels.empty()) {
            auto leg = legend(legend_labels);
            leg->font_size(10);
            leg->location(legend::general_alignment::topright);
        }
        
        grid(on);
        save(output_dir + "/sources.png");
    }
    
    // Generate receivers map
    if (domains.find("rec") != domains.end()) {
        auto f2 = figure(true);
        f2->position(0, 0, 1200, 800);
        hold(on);
        
        std::vector<std::string> legend_labels;
        
        for (size_t i = 0; i < processed_files.size(); ++i) {
            const auto& filename = processed_files[i];
            const auto& receivers = all_receivers_[filename];
            
            if (receivers.empty()) continue;
            
            std::vector<double> x, y;
            for (const auto& receiver : receivers) {
                x.push_back(static_cast<double>(receiver.rec_x));
                y.push_back(static_cast<double>(receiver.rec_y));
            }
            
            auto h = scatter(x, y);
            h->marker_size(2);
            h->color(colors[i % colors.size()]);
            h->marker_face(true);
            h->display_name(filename);
            
            legend_labels.push_back(filename);
        }
        
        title("Receiver Locations");
        xlabel("X Coordinate");
        ylabel("Y Coordinate");
        gca()->x_axis().tick_label_format("%.0f");
        gca()->y_axis().tick_label_format("%.0f");
        
        if (!legend_labels.empty()) {
            auto leg = legend(legend_labels);
            leg->font_size(10);
            leg->location(legend::general_alignment::topright);
        }
        
        grid(on);
        save(output_dir + "/receivers.png");
    }
    
    // Generate CDPs map
    if (domains.find("cdp") != domains.end()) {
        auto f3 = figure(true);
        f3->position(0, 0, 1200, 800);
        hold(on);
        
        std::vector<std::string> legend_labels;
        
        for (size_t i = 0; i < processed_files.size(); ++i) {
            const auto& filename = processed_files[i];
            const auto& cdps = all_cdps_[filename];
            
            if (cdps.empty()) continue;
            
            std::vector<double> x, y;
            for (const auto& cdp : cdps) {
                x.push_back(static_cast<double>(cdp.cdp_x));
                y.push_back(static_cast<double>(cdp.cdp_y));
            }
            
            auto h = scatter(x, y);
            h->marker_size(2);
            h->color(colors[i % colors.size()]);
            h->marker_face(true);
            h->display_name(filename);
            
            legend_labels.push_back(filename);
        }
        
        title("CDP Locations");
        xlabel("X Coordinate");
        ylabel("Y Coordinate");
        gca()->x_axis().tick_label_format("%.0f");
        gca()->y_axis().tick_label_format("%.0f");
        
        if (!legend_labels.empty()) {
            auto leg = legend(legend_labels);
            leg->font_size(10);
            leg->location(legend::general_alignment::topright);
        }
        
        grid(on);
        save(output_dir + "/cdps.png");
    }
}

std::string SegyScanner::getFilenameWithoutPath(const std::string& filepath) {
    return std::filesystem::path(filepath).filename().string();
}

std::string SegyScanner::getFilenameWithoutExtension(const std::string& filepath) {
    return std::filesystem::path(filepath).stem().string();
}

void SegyScanner::writeTableHeader(std::ofstream& file, const std::vector<std::string>& headers, const std::vector<int>& column_widths) {
    for (size_t i = 0; i < headers.size(); ++i) {
        if (i > 0) file << " ";
        file << formatCell(headers[i], column_widths[i]);
    }
    file << std::endl;
}

void SegyScanner::writeTableRow(std::ofstream& file, const std::vector<std::string>& values, const std::vector<int>& column_widths) {
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0) file << " ";
        file << formatCell(values[i], column_widths[i]);
    }
    file << std::endl;
}

std::vector<int> SegyScanner::calculateColumnWidths(const std::vector<std::string>& headers, const std::vector<std::vector<std::string>>& data) {
    std::vector<int> widths(headers.size(), 0);
    
    // Calculate width for headers
    for (size_t i = 0; i < headers.size(); ++i) {
        widths[i] = std::max(widths[i], static_cast<int>(headers[i].length()));
    }
    
    // Calculate width for data
    for (const auto& row : data) {
        for (size_t i = 0; i < row.size() && i < widths.size(); ++i) {
            widths[i] = std::max(widths[i], static_cast<int>(row[i].length()));
        }
    }
    
    return widths;
}

std::string SegyScanner::formatCell(const std::string& value, int width) {
    std::string result = value;
    if (result.length() < static_cast<size_t>(width)) {
        // Right-justify by adding spaces to the left
        result = std::string(width - result.length(), ' ') + result;
    }
    return result;
}

void SegyScanner::print_progress_bar(const std::string& label, int current, int total, int width) {
    if (total == 0) return;

    // Используем double для большей точности и добавляем 0.5 для правильного математического округления
    double progress = static_cast<double>(current) / total;
    int bar_width = static_cast<int>(progress * width + 0.5);

    std::cout << "\r" // Возврат каретки
              << std::left << std::setw(30) << label // Устанавливаем фиксированную ширину метки и выравниваем по левому краю
              << ": [";

    for (int i = 0; i < width; ++i) {
        std::cout << (i < bar_width ? '#' : '.');
    }
    std::cout << "] " 
              << std::right << std::setw(3) << static_cast<int>(progress * 100.0) << "%" // Фиксированная ширина для процентов
              << " (" << current << "/" << total << ")"
              << std::flush; // Принудительный сброс буфера в консоль

    // --- ИСПРАВЛЕНИЕ 3: Перевод строки только ПОСЛЕ финального вывода ---
    if (current >= total) {
        std::cout << std::endl;
    }
}
