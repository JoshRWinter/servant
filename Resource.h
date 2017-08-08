#include <fstream>

class Resource{
public:
	Resource(const std::string&);
	Resource(const Resource&)=delete;
	Resource(Resource&&);
	Resource &operator=(const Resource&)=delete;
	const std::string &name()const;
	int get(char*,int);
	int size()const;
	const char *type()const;

private:
	static void check_valid(const std::string&);
	static const char *get_type(const std::string&);
	static void get_ext(const std::string&,std::string&);

	int fsize;
	std::string fname;
	std::ifstream rsrc;
	const char *content_type;
};
