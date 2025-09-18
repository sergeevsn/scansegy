#include "SegyReader.hpp"
#include <iostream>
#include <cstring>
#include <algorithm>
#include <stdexcept>
#include <unordered_map>
#include <iomanip>

// Константы для IBM to IEEE conversion (from sample_segy_io.cpp)
#define SEGYIO_IEMAXIB 0x7fffffff 
#define SEGYIO_IEEEMAX 0x7f7fffff 
#define SEGYIO_IEMINIB 0x00ffffff 

// Вспомогательные функции теперь принимают файловый поток в качестве аргумента
void SegyReader::readBinaryHeader(std::ifstream& file) {
    // Чтение бинарного заголовка (400 байт, начиная со смещения 3200)
    binary_header_.resize(400);
    file.seekg(3200);
    file.read(binary_header_.data(), 400);
    
    if (file.gcount() != 400) {
        throw std::runtime_error("Failed to read binary header");
    }
    
    // Извлечение интервала дискретизации (dt) из бинарного заголовка (смещение 3216, 2 байта)
    uint16_t dt_us;
    std::memcpy(&dt_us, binary_header_.data() + 16, sizeof(dt_us));
    dt_us = swapBytes16(dt_us);
    
    if (dt_us == 0) {
        throw std::runtime_error("Sample interval (dt) is zero in binary header");
    }
    
    // Преобразование из микросекунд в секунды
    dt_ = dt_us * 1e-6;
    
    // Извлечение количества сэмплов на трейс (смещение 3220, 2 байта)
    uint16_t n_samples_per_trace;
    std::memcpy(&n_samples_per_trace, binary_header_.data() + 20, sizeof(n_samples_per_trace));
    n_samples_per_trace = swapBytes16(n_samples_per_trace);
    
    if (n_samples_per_trace == 0) {
        throw std::runtime_error("Number of samples per trace is zero in binary header");
    }
    
    num_samples_ = n_samples_per_trace;
}

// Вспомогательные функции теперь принимают файловый поток в качестве аргумента
void SegyReader::readTraces(std::ifstream& file) {
    const size_t trace_header_size = 240;
    const size_t trace_data_size = num_samples_ * sizeof(uint32_t);
    const size_t full_trace_size = trace_header_size + trace_data_size;
    
    // Начало чтения с 3600 (после текстового и бинарного заголовков)
    file.seekg(3600);
    
    // Подсчет трейсов путем чтения файла
    std::streampos current_pos = 3600;
    file.seekg(0, std::ios::end);
    std::streampos file_size = file.tellg();
    file.seekg(current_pos);
    
    num_traces_ = (file_size - current_pos) / full_trace_size;
    
    if (num_traces_ == 0) {
        throw std::runtime_error("No traces found in SEGY file");
    }
    
    // Изменение размера векторов для хранения только заголовков трейсов
    trace_headers_.resize(num_traces_);
    // Не выделяем память для traces_ - они не нужны для сканирования
    
    // Чтение только заголовков трейсов
    for (size_t i = 0; i < num_traces_; ++i) {
        // Чтение заголовка трейса
        trace_headers_[i].resize(trace_header_size);
        file.read(trace_headers_[i].data(), trace_header_size);
        
        if (file.gcount() != trace_header_size) {
            throw std::runtime_error("Failed to read trace header " + std::to_string(i));
        }
        
        // Пропускаем данные трейса - они не нужны для сканирования
        file.seekg(trace_data_size, std::ios::cur);
        
        // Show progress every 500 traces or at the end
        if ((i + 1) % 500 == 0 || i == num_traces_ - 1) {
            print_progress_bar("Reading headers from disk", i + 1, num_traces_);
        }
    }
}


SegyReader::SegyReader(const std::string& file_path) 
    : file_path_(file_path), num_traces_(0), num_samples_(0), dt_(0.0) {
    // Эта функция теперь управляет единым потоком файла
    std::ifstream file(file_path_, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open SEGY file: " + file_path_);
    }
    
    // Чтение бинарного заголовка для получения метаданных
    readBinaryHeader(file);
    
    // Чтение только заголовков трейсов (данные трасс не нужны для сканирования)
    readTraces(file);
    
    // Файл закроется автоматически при выходе из области видимости (RAII)
}

const std::vector<float>& SegyReader::getTrace(size_t trace_index) const {
    if (trace_index >= num_traces_) {
        throw std::out_of_range("Trace index " + std::to_string(trace_index) + 
                               " is out of range (max: " + std::to_string(num_traces_ - 1) + ")");
    }
    // Данные трасс не загружены для сканирования - возвращаем пустой вектор
    static const std::vector<float> empty_trace;
    return empty_trace;
}

const std::vector<char>& SegyReader::getTraceHeader(size_t trace_index) const {
    if (trace_index >= num_traces_) {
        throw std::out_of_range("Trace index " + std::to_string(trace_index) + 
                               " is out of range (max: " + std::to_string(num_traces_ - 1) + ")");
    }
    return trace_headers_[trace_index];
}

uint16_t SegyReader::swapBytes16(uint16_t val) const {
    return (val << 8) | (val >> 8);
}

uint32_t SegyReader::swapBytes32(uint32_t val) const {
    val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
    return (val << 16) | (val >> 16);
}

// Функция оставлена без изменений по вашему запросу
float SegyReader::ibmToIeee(uint32_t ibm) const {
    if (ibm == 0) return 0.0f;
    
    static const int it[8] = { 0x21800000, 0x21400000, 0x21000000, 0x21000000,
                               0x20c00000, 0x20c00000, 0x20c00000, 0x20c00000 };
    static const int mt[8] = { 8, 4, 2, 2, 1, 1, 1, 1 };
    
    uint32_t manthi = ibm & 0x00ffffff;
    int ix = manthi >> 21;
    uint32_t iexp = ((ibm & 0x7f000000) - it[ix]) << 1;
    manthi = manthi * mt[ix] + iexp;
    
    uint32_t inabs = ibm & 0x7fffffff;
    if (inabs > SEGYIO_IEMAXIB) manthi = SEGYIO_IEEEMAX;
    
    manthi = manthi | (ibm & 0x80000000);
    uint32_t result_bits = (inabs < SEGYIO_IEMINIB) ? 0 : manthi;
    
    float result_float;
    std::memcpy(&result_float, &result_bits, sizeof(float));
    return result_float;
}

int32_t SegyReader::get_header_value_i32(size_t trace_index, const std::string& key) const {
    if (trace_index >= num_traces_) {
        throw std::out_of_range("Trace index out of range");
    }
    
    const auto& header = trace_headers_[trace_index];
    
    // Mapping of header field names to byte offsets (1-based)
    static const std::unordered_map<std::string, int> field_offsets = {
        {"FieldRecord", 1}, {"TraceNumber", 5}, {"CDP", 21}, {"EnergySourcePoint", 25},
        {"SourceX", 73}, {"SourceY", 77}, {"SourceElevation", 45}, {"ReceiverX", 81},
        {"ReceiverY", 85}, {"ReceiverElevation", 41}, {"CDP_X", 181}, {"CDP_Y", 185},
        {"ILINE_3D", 189}, {"CROSSLINE_3D", 193}
    };
    
    auto it = field_offsets.find(key);
    if (it == field_offsets.end()) {
        return 0; // Return 0 for unknown fields
    }
    
    int offset = it->second - 1; // Convert to 0-based offset
    if (offset + 4 <= static_cast<int>(header.size())) {
        uint32_t value;
        std::memcpy(&value, header.data() + offset, sizeof(value));
        return static_cast<int32_t>(swapBytes32(value));
    }
    
    return 0;
}

int16_t SegyReader::get_header_value_i16(size_t trace_index, const std::string& key) const {
    if (trace_index >= num_traces_) {
        throw std::out_of_range("Trace index out of range");
    }
    
    const auto& header = trace_headers_[trace_index];
    
    // Mapping of header field names to byte offsets (1-based)
    static const std::unordered_map<std::string, int> field_offsets = {
        {"TraceNumber", 5}
    };
    
    auto it = field_offsets.find(key);
    if (it == field_offsets.end()) {
        return 0;
    }
    
    int offset = it->second - 1;
    if (offset + 2 <= static_cast<int>(header.size())) {
        uint16_t value;
        std::memcpy(&value, header.data() + offset, sizeof(value));
        return static_cast<int16_t>(swapBytes16(value));
    }
    
    return 0;
}

void SegyReader::print_progress_bar(const std::string& label, int current, int total, int width) {
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