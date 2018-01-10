#ifndef OS_H
#define OS_H

// contains os specific functions

bool working_dir(const std::string&);
bool get_working_dir(std::string&);
void register_handlers();
bool drop_root(unsigned);
bool canonical_path(const std::string&,std::string&);
bool is_directory(const std::string&);
long long filesize(const std::string&);

#endif // OS_H
