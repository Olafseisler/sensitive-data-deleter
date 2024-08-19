# Sensitive Data Deleter

This is an app that scans files and directories for personalized/sensitive data and offers 
to delete them securely if such data is found. The sensitive data is defined
in a configuration file with regular expressions that can be defined by the user 
with some default patterns included. If the user decides to delete the data, the app will
attempt to do so securely by overwriting the data with random bytes and fuzzing the file boundaries 
before calling the OS command to delete the file (that will simply mark the file as free space in the filesystem).

SDD uses the Hyperscan library and multithreading to achieve high performance scanning. It can scan plaintext files,
PDF, and expand .zip archives which also includes MS Office documents. 
The app is written in C++ and uses the Qt framework for the GUI.

## Usage
The basic usage flow upon opening the app is as follows:
1. Define the sensitive data patterns in the configuration tab. The default patterns are already included. 
See the section below for more information on the configuration. Items can also be removed from the list.
2. Click "Add Folder" or "Add File" to add a folder or file to the scan list on the left.
If some folders are too deep (currently up to 10 levels), the app will notify the user and not any deeper items.
3. Click "Scan" to start scanning the files and directories. This may take some time if the scan list is large.
4. The app will show the results in the right hand "Flagged" tab. The user can then decide to delete the files or remove 
them from the "Flagged" list. NB! All files that are NOT removed from the "Flagged" list will be deleted when pressing "Delete".
5. Click "Delete" to delete the files. The app will attempt to delete the files securely by overwriting the data with random bytes.

## Configuration
The scan configuration defines which file types (extensions) are scanned and which regex patterns are used matched against 
file contents. The configuration is defined in a .json file. The app also needs to have a configpath.txt file in the
same directory as the executable that contains the path to the config. Users can add and remove both scan patterns and 
file types using the corresponding buttons. WARNING! users can add any file types they wish, but the app scans only 
those that have plaintext, PDF or Zip archive content (newer, post 2006 MS Office docs are just .xml files wrapped in a zip archive).
If you are not sure, better leave the file types untouched.

Users can change the configuration path by selecting a new config file from the
file dialog opened by clicking the "Load Config" button. Users can also start a new, clean configuration file by clicking
the "New Config" button.

The new configuration file must be a JSON file with the following example structure:
```json
{
  "fileTypes": [
    {
      "description": "Plain text",
      "fileType": ".txt"
    },
    {
      "description": "Portable document format",
      "fileType": ".pdf"
    },
    {
      "description": "MS Word document",
      "fileType": ".docx"
    }
  ],
    "scanPatterns": [
        {
            "name": "Credit card number",
            "pattern": "\\b(?:\\d[ -]*?){13,16}\\b"
        },
        {
            "name": "Email",
            "pattern": "\\b[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\\.[A-Z|a-z]{2,}\\b"
        },
        {
            "name": "Phone number",
            "pattern": "\\b(?:\\+?\\d{1,3}[ -]*)?(?:\\(\\d{1,4}\\)[ -]*)?\\d{1,4}[ -]*\\d{1,4}[ -]*\\d{1,9}\\b"
        }
    ]
}
```
The fileTypes array contains the file types that are scanned. The description is shown in the GUI and the fileType is
the extension.

The scanPatterns array contains the regex patterns that are matched against the file contents. The scan patterns must be 
compatible with Hyperscan regex syntax found 
[here](https://intel.github.io/hyperscan/dev-reference/compilation.html#regular-expression-syntax).

TL,DR: The pretty much anything other than backreferences, lookaheads, and conditionals should work.

## Building 
The app is designed to be built using CMake with the [Vcpkg](https://github.com/microsoft/vcpkg) package manager.
The app is set up to be built for only x64-Windows for now, but should work with slight modification on any x64 platform.
The following system dependencies are required before building:
- Git  
- Cmake 3.25 or later
- Vcpkg
- MSVC 2022 command line tools

To download the source code and build the app, follow these steps (assumes Powershell):
1. Clone the repository:
```shell
git clone https://github.com/Olafseisler/sensitive-data-deleter.git
cd sensitive-data-deleter
```
2. Install the required libraries using vcpkg:
```shell
vcpkg install --triplet x64-windows-static --feature-flags=manifests --recurse 
```
3. Run CMake:
```
    cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=[vcpkg directory]/scripts/buildsystems/vcpkg.cmake" -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Release -DVCPKG_BUILD_TYPE=release -DVCPKG_TARGET_TRIPLET=x64-windows-static -DVCPKG_CRT_LINKAGE=static -DVCPKG_LIBRARY_LINKAGE=static
    cmake --build build --config Release
```
4. Copy the config to the build directory:
```shell
    cp sdd_config.json build/Release
    cp configpath.txt build/Release
```

The app should now be built and ready to run. The executable is located in the build/Release directory.





