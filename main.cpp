#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
using filesystem::path;

path operator""_p(const char* data, std::size_t sz) {
    return path(data, data + sz);
}

bool Preprocess(const path& file_path, istream& src, ostream& dst, const vector<path>& include_directories){
    // src поток на чтение файла file_path
    // dst поток для записи в файл
    path parent = file_path.parent_path();//для поиска в этой дериктории
    vector<path> directories {include_directories.begin(),include_directories.end()};
    directories.push_back(parent);
    static regex num_reg (R"/(\s*#\s*include\s*"([^"]*)"\s*)/");
    static regex num_reg2 (R"/(\s*#\s*include\s*<([^>]*)>\s*)/");
    smatch m;
    string buff;
    size_t line=1;
    while (getline(src, buff)){
        path new_path;
        size_t mistakes = 0;
        if (regex_match(buff, m, num_reg) || regex_match(buff, m, num_reg2)){         
            for (auto dir=directories.rbegin(); dir!=directories.rend(); dir++){
                path pare = *dir;
                new_path = path(pare.string() + '/' + static_cast<string>(m[1]));
                fstream new_src(new_path, ios::in);
                if(new_src.is_open()){
                    if(!Preprocess(new_path, new_src, dst, include_directories)){
                        mistakes++;
                    }
                }else{
                    mistakes++;
                }
            }
            if (mistakes >= directories.size()){
                cout<<"unknown include file "<<new_path.filename().string()<< " at file "<<file_path.string()<<" at line " <<line<<'\n' ;
                return false;
            }
        } else {  
            dst<<buff<<'\n'; 
        }
        line++;
    }
    return true;
}





bool Preprocess(const path& in_file, const path& out_file, const vector<path>& include_directories){    
    fstream src(in_file, ios::in);
    //istream src(in_file);
    if(src.is_open()){
        fstream dst(out_file, ios::out); 
        bool answer=Preprocess(in_file, src, dst, include_directories);
        return answer;
    }
    return false;
    

    
}

string GetFileContents(string file) {
    ifstream stream(file);
    return {(istreambuf_iterator<char>(stream)), istreambuf_iterator<char>()};
}

void Test() {
    error_code err;
    filesystem::remove_all("sources"_p, err);
    filesystem::create_directories("sources"_p / "include2"_p / "lib"_p, err);
    filesystem::create_directories("sources"_p / "include1"_p, err);
    filesystem::create_directories("sources"_p / "dir1"_p / "subdir"_p, err);

    {
        ofstream file("sources/a.cpp");
        file << "// this comment before include\n"
                "#include \"dir1/b.h\"\n"
                "// text between b.h and c.h\n"
                "#include \"dir1/d.h\"\n"
                "\n"
                "int SayHello() {\n"
                "    cout << \"hello, world!\" << endl;\n"
                "#   include<dummy.txt>\n"
                "}\n"s;
    }
    {
        ofstream file("sources/dir1/b.h");
        file << "// text from b.h before include\n"
                "#include \"subdir/c.h\"\n"
                "// text from b.h after include"s;
    }
    {
        ofstream file("sources/dir1/subdir/c.h");
        file << "// text from c.h before include\n"
                "#include <std1.h>\n"
                "// text from c.h after include\n"s;
    }
    {
        ofstream file("sources/dir1/d.h");
        file << "// text from d.h before include\n"
                "#include \"lib/std2.h\"\n"
                "// text from d.h after include\n"s;
    }
    {
        ofstream file("sources/include1/std1.h");
        file << "// std1\n"s;
    }
    {
        ofstream file("sources/include2/lib/std2.h");
        file << "// std2\n"s;
    }
Preprocess("sources"_p / "a.cpp"_p, "sources"_p / "a.in"_p,
                                  {"sources"_p / "include1"_p,"sources"_p / "include2"_p});
    assert((!Preprocess("sources"_p / "a.cpp"_p, "sources"_p / "a.in"_p,
                                  {"sources"_p / "include1"_p,"sources"_p / "include2"_p})));

    ostringstream test_out;
    test_out << "// this comment before include\n"
                "// text from b.h before include\n"
                "// text from c.h before include\n"
                "// std1\n"
                "// text from c.h after include\n"
                "// text from b.h after include\n"
                "// text between b.h and c.h\n"
                "// text from d.h before include\n"
                "// std2\n"
                "// text from d.h after include\n"
                "\n"
                "int SayHello() {\n"
                "    cout << \"hello, world!\" << endl;\n"s;

    assert(GetFileContents("sources/a.in"s) == test_out.str());
}

int main() {
    Test();
}