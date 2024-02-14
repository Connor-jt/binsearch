
#include <iostream>
#include <stdint.h>
#include <vector>

#include <fstream>
#include <iostream>
#include <sstream>
#include <filesystem>
#include <cassert>

#include <stdint.h>
#include <wtypes.h>
#include <stdexcept>
using namespace std;

vector<string> files;
// sourced from: https://stackoverflow.com/a/25640066/22277207
void FindFiles(const wstring& directory) {
    wstring tmp = directory + L"\\*";
    string dir(directory.begin(), directory.end());
    WIN32_FIND_DATAW file;
    HANDLE search_handle = FindFirstFileW(tmp.c_str(), &file);
    if (search_handle != INVALID_HANDLE_VALUE) {
        vector<wstring> directories;

        do {
            if (file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                if ((!lstrcmpW(file.cFileName, L".")) || (!lstrcmpW(file.cFileName, L"..")))
                    continue;
                directories.push_back(directory + L"\\" + wstring(file.cFileName));
                continue;
            }
            wstring wide_filename = wstring(file.cFileName);
            string filename(wide_filename.begin(), wide_filename.end());
            files.push_back(dir + "\\" + filename);

        } while (FindNextFileW(search_handle, &file));

        FindClose(search_handle);

        for (vector<wstring>::iterator iter = directories.begin(), end = directories.end(); iter != end; ++iter)
            FindFiles(*iter);
    }
}






static char scan_sig[] = { 0x48, 0x0D, 0x2A, 0x4D, 0x2E, 0xCE, 0x90, 0x66, 0x60, 0x60, 0xF0, 0x0C, 0xD8, 0x78, 0xCF, 0xA2, 0xCC, 0xBE, 0x90, 0x67, 0x6E, 0xDC, 0xF5, 0x15, 0x06, 0xFA, 0xF7, 0x18, 0xA0, 0x80, 0x95, 0x8B };
static uint32_t buffer_length = 16384; // max uint16 length??

vector<uint64_t> segmented_search(string path, char* signature, uint8_t sig_length) {
    if (sig_length <= 1) throw exception("scan signature is too short!!!");

    // open module file
    ifstream file_reader = {};
    file_reader.open(path, ios::binary | ios::ate);
    if (!file_reader.is_open()) throw exception("failed to open filestream");

    uint64_t bytes_left = file_reader.tellg();
    uint64_t bytes_traversed = 0;
    file_reader.seekg(0, ios::beg);

    uint16_t buffer_byte_index = 0;
    uint16_t current_buffer_length = 0;
    char* buffer = new char[buffer_length];

    uint8_t string_match_index = 0;

    vector<uint64_t> results = {};
    while (true) {
        // if we're at the end of the buffer, read into a new buffer
        if (buffer_byte_index == current_buffer_length) {
            if (bytes_left == 0) break; // then we've read all the bytes
            current_buffer_length = buffer_length;
            if (bytes_left < current_buffer_length) current_buffer_length = bytes_left;

            file_reader.read(buffer, current_buffer_length);
            bytes_left -= current_buffer_length;
            bytes_traversed += current_buffer_length;
            buffer_byte_index = 0; // reset our buffer relative index
        }

        char current_byte = buffer[buffer_byte_index];
        if (signature[string_match_index] == current_byte) {
            string_match_index++;
            if (string_match_index == sig_length) {
                results.push_back(bytes_traversed + buffer_byte_index);
                string_match_index = 0;
        }}
        // if the byte we read wasn't the next part of the sequence, but a restart, then the next byte will be the 2nd character
        else if (string_match_index > 0 && signature[0] == current_byte)
             string_match_index = 1;
        else string_match_index = 0;


        buffer_byte_index++;
    }


    delete[] buffer;
    return results;
}





int main(){
    assert(sizeof(scan_sig) < 256);
    cout << "Hello World!\n";
    string directory;
    getline(cin, directory);
    cout << "finding files...\n";
    struct stat s; // what even is this??
    int err = stat(directory.c_str(), &s);
    if (-1 == err) {
        cout << "bad directory.\n";
        return 0;}

    wstring wide_directory(directory.begin(), directory.end());
    FindFiles(wide_directory);
    cout << files.size() << " files found\n\n";
    if (files.size() == 0) {
        cout << "no files found.\n";
        return 0;}

    for (int p = 0; p < files.size(); p++) {
        string mod_path = files[p];
        auto results = segmented_search(mod_path, scan_sig, sizeof(scan_sig));
        for (auto thing : results)
            cout << "pattern found at 0d" << thing << " of \"" << mod_path << "\" \n";
        cout << p << "/" << files.size() << "\n";
    }
    return 0;
}
