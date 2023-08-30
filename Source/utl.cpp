#include <iostream>
#include <fstream>

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

    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
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
  DIR *directory;
  struct dirent *dir_entry;

  std::vector <std::string> result;

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
  DIR *directory;
  struct dirent *dir_entry;

  std::vector <std::string> result;

  directory = opendir(path.c_str());
  if (! directory)
      return result;

  std::string t = dir_entry->d_name;


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
    WIN32_FIND_DATAA findData;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    std::string full_path = path + "\\*";
    std::vector<std::string> result;

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
    WIN32_FIND_DATAA findData;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    std::string full_path = path + "\\*";
    std::vector<std::string> result;

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

#endif

  return result;
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
bool findStringIC(const std::string & strHaystack, const std::string & strNeedle)
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

