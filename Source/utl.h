/*
written at 2023-24 by Peter Semiletov
this code is the public domain
*/


#ifndef UTL_H
#define UTL_H

#include <string>
#include <vector>

bool file_exists (const std::string &name);
bool ends_with (std::string const &value, std::string const &ending);
std::string resolve_symlink (const std::string &path);
std::string get_home_dir();

std::vector <std::string> files_get_list (const std::string &path);
std::vector <std::string> files_get_list (const std::string &path, const std::string &ext); //ext with dot: ".txt"

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

#endif
