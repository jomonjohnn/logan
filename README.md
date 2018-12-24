# logan 
Just another Log Analyzer/parser. Tired of analyzing large log files with regex on sublime and Logan was born as a quick hack.

logan will match lines from an input file based on the regex provided via a rule file. The matching lines can be written to stdout or the same can be saved as a json file based on the capturte groups in the regex.

## Design

Initial plan was to supply the input line while posting to io service, later found that its a bad idea, hence a fixed size queue is utlized to feed the lines to multiple consumers. The current operation is as follows,

* Open rule file and load regex
* Open input file, get a line and add to queue
* asio will do the processing parallely

## Building

Use camke to build logan,

```bash
git clone https://github.com/jomonjohnn/logan.git
mkdir loganbuild && cd loganbuild
cmake -DCMAKE_BUILD_TYPE=Release ../logan
```

Tested on _Ubuntu 18.04_ with _g++ 7.3.0_ and _Boost 1.65.1_

## Usage

Invoke loagan with a input file and rule file

```bash
./logan -i <input file> -r <json rule file> -o <output json file>
```

Providing _-w_ switch will write matching lines to stdout, hence by default all debug prints are redirected to stderr. 
```bash
./logan -i <input file> -r <json rule file> -o <output json file> -w > out.txt
```

## Authors
* Jomon John < mail [at] jomon [dot] in>

## Contributing
Any suggestions/improvemts/pull requests are welcome. 

## License
[MPL 2.0](https://choosealicense.com/licenses/mpl-2.0/)

## Dependencies
* nlohmann/json - https://github.com/nlohmann/json
* prakhar1989/progress-cpp - https://github.com/prakhar1989/progress-cpp
* Boost.Asio - https://www.boost.org/doc/libs/1_69_0/doc/html/boost_asio.html
* Boost.Program_options - https://www.boost.org/doc/libs/1_69_0/doc/html/program_options.html
* Boost.Filesystem - https://www.boost.org/doc/libs/1_69_0/libs/filesystem/doc/index.htm

