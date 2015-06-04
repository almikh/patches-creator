#include <iostream>
#include <fstream>
#include <map>
#include <set>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include "tinyxml2.h"

using namespace std;
using namespace tinyxml2;

namespace fs = boost::filesystem;
namespace po = boost::program_options;

void parseArgs(int argc, char** argv);
void createStorageFile(const string& path);
set<string> compareWithStorageFile(const string& path);
void copyActualData(const string& dst, const set<string>& diffs);

bool gUpdate = false;
bool gCompare = false;
string gStorage = "storage.xml";
string gTargetDir = "target//";
string gDir = "";

int main(int argc, char** argv) {
	setlocale(LC_ALL, "Russian");

	try {
		parseArgs(argc, argv);
	}
	catch (const std::exception& ex) {
		cerr << "error: " << ex.what() << endl;
		return EXIT_FAILURE;
	}

	if (gUpdate) {
		createStorageFile(gDir);
	}
	else if (gCompare) {
		auto diffs = compareWithStorageFile(gDir);
		copyActualData(gTargetDir, diffs);
	}

	return 0;
}

void parseArgs(int argc, char** argv) {
	po::options_description desc("Options");
	desc.add_options()
		("help,h", "Показать этот экран")
		("update,u", "Обновить данные о директории")
		("storage,s", po::value<string>()->value_name("<file>"), "XML Файл для записи/чтения информации о директории")
		("out-dir,t", po::value<string>()->value_name("<dir>"), "Директория, в которую будут помещены измененные файлы (совместно с флагом -c)")
		("compare,c", "Сравнить две директории и создать файл со списком различий");

	/* input files */
	po::options_description full_desc;
	full_desc.add_options()
		("inp-dir", po::value<string>(), "Directory for processing");
	full_desc.add(desc);

	po::positional_options_description pdesc;
	pdesc.add("inp-dir", 1);

	po::variables_map vm;
	po::command_line_parser parser(argc, argv);
	po::store(parser.options(full_desc).positional(pdesc).run(), vm);
	po::notify(vm);

	if (argc < 2 || vm.count("help")) {
		cout << "Usage: " << argv[0] << " [options] <target directory>" << endl << endl;
		// Сравнивает данные из <inp-dir> и инф. о файлах из storage.xml, и сохраняет различия в <dst-dir>
		cout << "Example: " << endl;
		cout << " " << argv[0] << " -c -s storage.xml -t <dst-dir> <inp-dir>" << endl;
		// Записывает инф-ю о файлах из <inp-dir> и пишет ее в eistorage.xml (его затем использовать для создания набора различий)
		cout << " " << argv[0] << " -u -s storage.xml <inp-dir>" << endl << endl;
		cout << desc << endl;
		exit(EXIT_SUCCESS);
	}

	gUpdate = vm.count("update") > 0;
	gCompare = vm.count("compare") > 0;
	
	if (vm.count("storage")) {
		gStorage = vm["storage"].as<string>();
	}
	
	if (vm.count("out-dir")) {
		gTargetDir = vm["out-dir"].as<string>();
	}

	if (vm.count("inp-dir")) {
		gDir = vm["inp-dir"].as<string>();
	}
	else {
		throw std::runtime_error("Not defined target directory!");
	}
}

void createStorageFile(const string& path) {	
	XMLDocument doc;
	doc.SetValue("files");
	auto root = doc.NewElement("files");
	doc.LinkEndChild(root);

	int width = 42;
	fs::path current(path);
	fs::recursive_directory_iterator end;
	cout << "Go to directory iterator: " << current.string() << " -> " << endl;
	for (fs::recursive_directory_iterator item(current); item != end; ++item) {
		if (!fs::is_directory(item->status())) {
			string filename = item->path().string();	
			if ((int)filename.size()>width) {
				cout << "[file]: ..." << filename.substr(filename.size()-width) << endl;
			}
			else cout << "[file]: " << filename << endl;

			auto file = doc.NewElement("file");
			file->SetAttribute("name", filename.substr(current.string().size()).c_str());
			file->SetAttribute("last_modif", to_string(fs::last_write_time(filename)).c_str());
			root->LinkEndChild(file);
		}
	}
	doc.SaveFile(gStorage.c_str());
}

set<string> compareWithStorageFile(const string& path) {
	map<string, time_t> storage;
	
	XMLDocument doc;
	if (doc.LoadFile(gStorage.c_str())!=XML_NO_ERROR) {
		cerr << "Error reading XML!" << endl;
		doc.PrintError();
	}
	auto root  = doc.FirstChildElement();
	auto file = root->FirstChildElement();
	while (file) {
		string name = file->FirstAttribute()->Value();
		auto time = file->Attribute("last_modif");
		storage[name] = stoll(time);
		file = file->NextSiblingElement();
	}

	set<string> dst;
	fs::path current(path);
	fs::recursive_directory_iterator end;
	cout << "Go to directory iterator: " << current.string() << " -> " << endl;
	for (fs::recursive_directory_iterator item(current); item != end; ++item) {
		if (!fs::is_directory(item->status())) {
			fs::path filename = item->path();

			time_t time = fs::last_write_time(filename);
			auto sfile = filename.string().substr(current.string().size());
			if (storage.count(sfile)>0) {
				if (storage[sfile]!=time) {
					dst.insert(sfile);
					cout << "[mod] " << sfile << endl;
				}
			}
			else {
				dst.insert(sfile);
				cout << "[new] " << sfile << endl; 
			}
		}
	}
	return dst;
}

void copyActualData(const string& dst, const set<string>& diffs) {
	fs::path source = gDir;
	fs::path dest = dst;

	fs::remove_all(dst);
	fs::create_directories(dst);

	for (auto &e: diffs) {
		fs::path destPath = (dest / e).parent_path();
		fs::create_directories(destPath);
		cout << "[copy]: " << e << endl;
		fs::copy_file(source / e, dest / e, fs::copy_option::overwrite_if_exists);
	}
}
