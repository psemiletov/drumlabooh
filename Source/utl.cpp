/*
written at 2023-24 by Peter Semiletov
this code is the public domain
*/


#include <iostream>
#include <fstream>

#include <filesystem>


#include <algorithm>
#include <string>
#include <cctype>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>


#if !defined(_WIN32) || !defined(_WIN64)

#include <unistd.h>
#include <dirent.h>

#else

#include <Windows.h>
#include <shlobj.h>
//#include <shlobj_core.h>
//#include <KnownFolders.h>
//#include <wchar.h>

#endif


#if !defined(_WIN32) || !defined(_WIN64)

#define DIR_SEPARATOR "/"

#else

#define DIR_SEPARATOR "\\"

#endif


#include "utl.h"


using namespace std;


bool file_exists (const string &name)
{
  if (name.empty())
     return false;

  struct stat buffer;
  return (stat (name.c_str(), &buffer) == 0);
}


bool ends_with (std::string const & value, std::string const & ending)
{
  if (ending.size() > value.size())
     return false;

  return std::equal (ending.rbegin(), ending.rend(), value.rbegin());
}


std::string resolve_symlink (const std::string &path)
{
#if !defined(_WIN32) || !defined(_WIN64)

  bool is_symlink = false;

    struct stat buf;
    int r = stat (path.c_str(), &buf);

    if (S_ISLNK(buf.st_mode))
       is_symlink = true;

    //if (S_ISREG(buf.st_mode)) printf (" stat says file\n");
    if (is_symlink)
       {
        char resolved_fname[FILENAME_MAX];
        int count = readlink (path.c_str(), resolved_fname, sizeof(resolved_fname));
        if (count >= 0)
          {
           resolved_fname[count] = '\0';
           return resolved_fname;
          }
     }

   return path;

#else

   return path;

#endif
}


#if !defined(_WIN32) || !defined(_WIN64)

std::vector <std::string> files_get_list (const std::string &path)
{
  std::vector <std::string> result;

  if (path.empty())
     return result;

  DIR *directory;
  struct dirent *dir_entry = NULL;

  directory = opendir (path.c_str());
  if (! directory)
     return result;

  std::string t;

  while ((dir_entry = readdir (directory)))
        {
         t = dir_entry->d_name;

         if (t != "." && t != "..")
             result.push_back (path + "/" + t);
        }

  closedir (directory);

  return result;
}


std::vector <std::string> files_get_list (const std::string &path, const std::string &ext) //ext with dot: ".txt"
{
  std::vector <std::string> result;

  if (path.empty())
     return result;

  DIR *directory;


  directory = opendir (path.c_str());
  if (! directory)
      return result;

  std::string t;// = dir_entry->d_name;

  struct dirent *dir_entry = NULL;

   while (dir_entry = readdir (directory))
         {
          // std::cout << dir_entry->d_name << std::endl;
          t = dir_entry->d_name;
          if (t.rfind (ext) != string::npos)
            result.push_back (path + "/" + t);
         }

   closedir (directory);
   return result;
}


#else

std::vector <std::string> files_get_list (const std::string &path)
{

  std::vector<std::string> result;

  if (path.empty())
     return result;


    WIN32_FIND_DATAA findData;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    std::string full_path = path + "\\*";

    hFind = FindFirstFileA (full_path.c_str(), &findData);

    std::string t;

    if (hFind == INVALID_HANDLE_VALUE)
        //throw std::runtime_error("Invalid handle value! Please check your path...");
       return result;

    while (FindNextFileA(hFind, &findData) != 0)
      {
       t = findData.cFileName;

       if (t != "." && t != "..")
           result.push_back (path + "/" + t);
     }

    FindClose(hFind);

    return result;
}



std::vector <std::string> files_get_list (const std::string &path, const std::string &ext) //ext with dot: ".txt"
{

  std::vector<std::string> result;

  if (path.empty())
     return result;



    WIN32_FIND_DATAA findData;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    std::string full_path = path + "\\*";


    hFind = FindFirstFileA (full_path.c_str(), &findData);

    std::string t;

    if (hFind == INVALID_HANDLE_VALUE)
        //throw std::runtime_error("Invalid handle value! Please check your path...");
       return result;

    while (FindNextFileA(hFind, &findData) != 0)
      {
        t = findData.cFileName;

      if (t.rfind (ext) != string::npos)
            result.push_back (path + "/" + t);

    }

    FindClose(hFind);

    return result;
}


#endif



std::string get_home_dir()
{
  std::string result;

#if !defined(_WIN32) || !defined(_WIN64)

  const char *homedir = getenv ("HOME");

  if (homedir != NULL)
     result = homedir;

#else

  char homeDirStr[MAX_PATH];

  if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, 0, homeDirStr)))
    result = homeDirStr;

/*
     PWSTR path = NULL;
    HRESULT hres = SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &path);*/
    //%USERPROFILE%\Documents

#endif

  return result;
}



std::string get_file_ext (const std::string &fname)
{
  std::string::size_type i = fname.rfind('.');
  if (i != std::string::npos)
     return fname.substr (i+1);
  else
      return string();
}


std::string get_file_path (const std::string &path)
{
  size_t i = path.rfind ("/", path.length());

  if (i != std::string::npos)
     return path.substr (0, i);

  return std::string();
}


std::string string_file_load (const string &fname)
{
 if (fname.empty())
    return string();

 std::ifstream t (fname.c_str());
 std::string s ((std::istreambuf_iterator<char>(t)),
                 std::istreambuf_iterator<char>());

 return s;
}


/// Try to find in the Haystack the Needle - ignore case
bool findStringIC (const std::string & strHaystack, const std::string & strNeedle)
{
  auto it = std::search(
    strHaystack.begin(), strHaystack.end(),
    strNeedle.begin(),   strNeedle.end(),
    [](unsigned char ch1, unsigned char ch2) { return std::toupper(ch1) == std::toupper(ch2); }
  );
  return (it != strHaystack.end() );
}


std::string string_to_lower (const std::string &s)
{
  string result = s;

  std::for_each (
                 result.begin(),
                 result.end(),
                 [](char & c) {
                               c = ::tolower(c);
                              });

  return result;
}


vector <string> split_string_to_vector (const string& s, const string& delimeter, const bool keep_empty)
{
  vector <string> result;

  if (delimeter.empty())
     {
      result.push_back (s);
      return result;
     }

  string::const_iterator substart = s.begin(), subend;

  while (true)
        {
         subend = search (substart, s.end(), delimeter.begin(), delimeter.end());

         string temp (substart, subend);

         if (keep_empty || ! temp.empty())
             result.push_back (temp);

         if (subend == s.end())
             break;

         substart = subend + delimeter.size();
        }

  return result;
}



string string_replace_all (const string &s, const string &from, const string &to)
{
  string result = s;
  size_t i = 0;
  do
    {
     i = result.find (from);
     if (i != string::npos)
         result.replace (i, from.length(), to);
    }
  while (i != string::npos);

  return result;
}


std::string replace_string (std::string subject, const std::string& search, const std::string& replace) 
{
    size_t pos = 0;
    while ((pos = subject.find(search, pos)) != std::string::npos) {
         subject.replace(pos, search.length(), replace);
         pos += replace.length();
    }
    return subject;
}


std::string transform_kit_path_to_local (const std::string &path) 
{
  std::string result = path;
  if (result.rfind ("$HOME", 0) == 0)
     result = replace_string (result, "$HOME", get_home_dir()); 
    
  return result;  
}


std::string transform_kit_path_from_local (const std::string &path) 
{
  std::string result = path;
  if (result.rfind (get_home_dir(), 0) == 0)
     result = replace_string (result, get_home_dir(), "$HOME"); 
    
  return result;  
}


std::string bytes_to_file_size (size_t val)
{
  if (val >= 1073741824)
     return std::to_string (val / 1073741824) + " gbytes";

  if (val >= 1048576)
     return std::to_string (val / 1048576) + " mbytes";

  if (val >= 1024)
     return std::to_string (val / 1024) + " kbytes";

  return std::to_string (val) + " bytes";;
}


std::string get_string_between (const std::string &s, const std::string &start, const std::string &end)
{
  std::string result;
  size_t first = s.find (start);
  
  if (first == std::string::npos)
     return result;

  first = first + start.length();

 // std::cout << "first:" << first << std::endl;
  
  size_t last = s.find (end);

  //std::cout << "last:" << last << std::endl;
    
  if (last == std::string::npos)
     return result;
  
  result = s.substr (first, last - first);
  
  return result; 
}


std::string get_last_part (const std::string &path)
{
  std::string result;
  size_t pos = path.rfind ("/");
  
  if (pos == std::string::npos)
     return result;
  
  result = path.substr (pos + 1, path.length() - pos);
  
  return result; 
}


void string_save_to_file (const std::string &fname, const std::string &s)
{
  std::ofstream file (fname);
  if (file.is_open())
  file << s;
  file.close();
}

/*
  
 std::string directory = "./my_folder";
    
    // Список желаемых расширений (например, ".txt", ".cpp")
    std::vector<std::string> extensions = {".txt", ".cpp"};
  
 */ 

// Функция для получения списка файлов с указанными расширениями
std::vector<std::string> get_files_with_extensions (const std::string& directory, const std::vector<std::string>& extensions) 
{
    std::vector<std::string> files;

    // Проходим по всем элементам в директории
    for (const auto& entry : std::filesystem::directory_iterator(directory)) 
        {
        // Проверяем, что это обычный файл
         if (entry.is_regular_file()) 
           {
            // Получаем расширение файла в нижнем регистре
            std::string ext = entry.path().extension().string();
            // Приводим к нижнему регистру для нечувствительного сравнения
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            
            // Проверяем, есть ли расширение в списке допустимых
            for (const auto& valid_ext : extensions) {
                std::string temp_ext = valid_ext;
                // Приводим искомое расширение к нижнему регистру
                std::transform(temp_ext.begin(), temp_ext.end(), temp_ext.begin(), ::tolower);
                
                // Если расширение совпадает, добавляем путь к файлу в результат
                if (ext == temp_ext || (ext.empty() && temp_ext == ".")) {
                    files.push_back(entry.path().string());
                }
            }
        }
    }
    
    std::sort(files.begin(), files.end());
    
    return files;
}


// Функция для получения списка каталогов в указанной директории с сортировкой по алфавиту
std::vector<std::string> get_directories (const std::string& directory) 
{
    std::vector<std::string> directories;
    
    // Проходим по всем элементам в директории
    for (const auto& entry : filesystem::directory_iterator(directory)) 
    {
        // Проверяем, что это директория
        if (entry.is_directory()) {
            directories.push_back(entry.path().string());
        }
    }
    
    // Сортируем список каталогов по алфавиту
    std::sort(directories.begin(), directories.end());
    
    return directories;
}


//#if !defined(_WIN32) || !defined(_WIN64)
bool is_kit_dir (const std::string& path)
{
  std::string dir_pattern;
  
 // dir_pattern += DIR_SEPARATOR; 
  
  dir_pattern += "drum_dirs";
   
  //dir_pattern += DIR_SEPARATOR;
  
       
  return (path.find (dir_pattern) != std::string::npos);  
}
//#else
/*
bool is_kit_dir (const std::string& path)
{
 
  bool result = false;
  
  if (path.find ("c:\\drum_dirs") != std::string::npos)
     result = true;

   
 if (path.find ("d:\\drum_dirs") != std::string::npos)
     result = true;

 // dir_pattern += DIR_SEPARATOR; 
  
   
  //dir_pattern += DIR_SEPARATOR;
  
       
  return result;
}

#endif
*/