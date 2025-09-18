#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <fstream>

class SegyReader {
public:
    /**
     * @brief Основной конструктор. Открывает SEG-Y файл для чтения.
     * @param file_path Путь к SEG-Y файлу.
     */
    explicit SegyReader(const std::string& file_path);
    
    // Запрещаем копирование и присваивание
    SegyReader(const SegyReader&) = delete;
    SegyReader& operator=(const SegyReader&) = delete;

    // --- ОСНОВНЫЕ МЕТОДЫ ДОСТУПА К ДАННЫМ ---
    
    const std::vector<float>& getTrace(size_t trace_index) const;
    const std::vector<char>& getTraceHeader(size_t trace_index) const;

    // --- ГЕТТЕРЫ ---
    
    size_t num_traces() const { return num_traces_; }
    size_t num_samples() const { return num_samples_; }
    double sample_interval() const { return dt_; }
    
    // --- МЕТОДЫ ДЛЯ ЧТЕНИЯ ЗАГОЛОВКОВ ТРАСС ---
    
    int32_t get_header_value_i32(size_t trace_index, const std::string& key) const;
    int16_t get_header_value_i16(size_t trace_index, const std::string& key) const;
    
    // --- УТИЛИТЫ ---
    
    void print_progress_bar(const std::string& label, int current, int total, int width = 50);

private:
    std::string file_path_;
    size_t num_traces_;
    size_t num_samples_;
    double dt_;
    
    std::vector<std::vector<float>> traces_;
    std::vector<std::vector<char>> trace_headers_;
    std::vector<char> binary_header_;
    
    // Вспомогательные методы
    void readBinaryHeader(std::ifstream& file);
    void readTraces(std::ifstream& file);
    
    uint16_t swapBytes16(uint16_t val) const;
    uint32_t swapBytes32(uint32_t val) const;
    float ibmToIeee(uint32_t ibm) const;
};
