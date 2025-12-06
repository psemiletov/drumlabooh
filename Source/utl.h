/*
written at 2023-25 by Peter Semiletov
this code is the public domain
*/

#ifndef UTL_H
#define UTL_H

#include <string>
#include <vector>


//for FastDeterministicRNG
#include <cstdint>
#include <algorithm>
#include <limits>

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif


// МИНИМАЛЬНЫЕ значения:
//minVal = LLONG_MIN  // -9,223,372,036,854,775,808
//maxVal = LLONG_MAX  // +9,223,372,036,854,775,807

#define RND_MAX_TWEAK 16777216

//based on AI-generated code

class FastDeterministicRNG 
{
public:
    FastDeterministicRNG() : state(0xDEADBEEFCAFEBABE) {}  // Дефолтное seed
    
    void setSeed(uint64_t seed) {
        state = seed ? seed : 0xDEADBEEFCAFEBABE;  // Защита от 0
    }
    
    uint64_t nextRaw() {
        // Улучшенный алгоритм (хотя для криптографии нужно что-то серьезнее)
        state ^= state >> 12;
        state ^= state << 25;
        state ^= state >> 27;
        return state * 0x2545F4914F6CDD1DULL;
    }
    
    long long next(long long minVal, long long maxVal) {
        if (minVal > maxVal) std::swap(minVal, maxVal);
        
        uint64_t range = static_cast<uint64_t>(maxVal) - static_cast<uint64_t>(minVal);
        if (range == 0) return minVal;  // Защита от деления на 0
        
        // Корректное равномерное распределение
        uint64_t raw = nextRaw();
       
         
        if (range == (std::numeric_limits<uint64_t>::max)()) 
          return minVal + static_cast<long long>(raw);

        
        // Метод отбраковки для устранения смещения
        uint64_t limit = std::numeric_limits<uint64_t>::max() - 
                        (std::numeric_limits<uint64_t>::max() % (range + 1));
        while (raw > limit) {
            raw = nextRaw();
        }
        
        return minVal + static_cast<long long>(raw % (range + 1));
    }

private:
    uint64_t state;
};

bool file_exists (const std::string &name);
bool ends_with (std::string const &value, std::string const &ending);
std::string resolve_symlink (const std::string &path);
std::string get_home_dir();

std::vector <std::string> files_get_list (const std::string &path);
std::vector <std::string> files_get_list (const std::string &path, const std::string &ext); //ext with dot: ".txt"

std::vector<std::string> get_files_with_extensions (const std::string& directory, const std::vector<std::string>& extensions);
std::vector<std::string> get_directories (const std::string& directory); 
bool is_kit_dir (const std::string &path);
bool is_directory_safe (const std::string &path);

std::string get_file_ext (const std::string &fname);
std::string get_file_path (const std::string &path);
std::string string_file_load (const std::string &fname);
std::string string_to_lower (const std::string &s);

bool findStringIC (const std::string &strHaystack, const std::string &strNeedle);

std::vector <std::string> split_string_to_vector (const std::string& s, const std::string& delimeter, const bool keep_empty);
std::string string_replace_all (const std::string &s, const std::string &from, const std::string &to);
std::string replace_string(std::string subject, const std::string& search, const std::string& replace); 

std::string transform_kit_path_to_local (const std::string &path);
std::string transform_kit_path_from_local (const std::string &path);

std::string bytes_to_file_size (size_t val);
std::string get_string_between (const std::string &s, const std::string &start, const std::string &end);
std::string get_last_part (const std::string &path);

void string_save_to_file (const std::string &fname, const std::string &s);


#if !defined(_WIN32) || !defined(_WIN64)

#define DIR_SEPARATOR "/"

#else

#define DIR_SEPARATOR "\\"

#endif

#endif
