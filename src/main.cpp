
/* 
 * Logan : The dumb log analyzer with regex support
 * License : MPL 2.0 [ https://choosealicense.com/licenses/mpl-2.0/ ]
 * Author : John J < mail [at] jomon [dot] in>
 * Version : 0.1
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */ 

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <regex>
#include <string>
#include <thread>
#include <cstdbool>

// Boost
#include <boost/asio.hpp>
#include <boost/program_options.hpp>

// Thrid party
#include <nlohmann/json.hpp>   // https://github.com/nlohmann/json
#include <ProgressBar.hpp>     // https://github.com/prakhar1989/progress-cpp

//local
#include <inputHandler.hpp>    // ifstream abstraction
#include <concurrentQueue.hpp> // conurrent queue
#include <GitSHA1.h>

namespace po = boost::program_options;

int main ( int argc, char** argv ) {

    try{
    
        po::options_description desc("Supported options");
        std::string inputFile, ruleFile, ouputJsonFile;
        bool enableWritetoFile = false, enableJsonOut = false;
        po::variables_map vm;
        
        desc.add_options()
        ("help,h",                                             "Print help message")
        ("input,i",  po::value< std::string >(&inputFile),     "Input log file name")
        ("rule,r",   po::value< std::string >(&ruleFile),      "Rule file name")
        ("write,w",  po::bool_switch(&enableWritetoFile),      "Enable writing fitered output to stdout")
        ("output,o", po::value< std::string >(&ouputJsonFile), "Output JSON file name to write filtered putput json")
        ("version,v", "Version Details" );
        
        try{
            po::store(po::parse_command_line(argc, argv, desc), vm);
            po::notify(vm);    

            if (vm.count("version") ) {
                std::cerr << "Version : " << g_GIT_SHA1 << std::endl;
                exit(0);
            }
            
            if (vm.count("help") || 1 == argc ) {
                std::cerr << desc << std::endl;
                std::cerr << "Version : " << g_GIT_SHA1 << std::endl;
                exit(0);
            }
            
            if (vm.count("input") && !inputFile.empty() ) {
                std::cerr << "Selected input file : " << inputFile << std::endl;
            }
            
            if (vm.count("rule") && !ruleFile.empty() ) {
                std::cerr << "Selected rule file : " << ruleFile << std::endl;
            }
            
            if(enableWritetoFile){
                std::cerr << "Writing the matched results to stdout" << std::endl;
            }
            
            if (vm.count("output") && !ouputJsonFile.empty() ) {
                std::cerr << "Selected ouput file : " << ouputJsonFile << std::endl;
                enableJsonOut = true;
            }
            
            if( !enableWritetoFile && !enableJsonOut ){
                std::cerr << "No operation specified (use -w or -o ), exiting" << std::endl;
                std::exit(0);
            }
            
        }catch(const po::error& e){
            std::cerr << "ERROR: " << e.what() << std::endl << std::endl; 
            std::cerr << desc << std::endl; 
            return(-1);
        }

        // Mark start time        
        auto start_time = std::chrono::system_clock::now();
        
        // Get rules from rule json file
        inputHandler rulehandler( ruleFile );
        nlohmann::json rules;
        try{
            rulehandler.getStream() >> rules;
        }catch(nlohmann::json::parse_error& e){
            std::cerr << "Error in parsing rule file : " << e.what() << '\n'
                      << "byte position of error: " << e.byte << std::endl;
        }
        
        // load capture list from rules
        nlohmann::json captureList = rules["capture_list"];
        std::cerr << "Capture list count : " << captureList.size() << std::endl;

        // load gegex from rule file
        std::string rule_regex = rules.value("regex", "");
        if(!rule_regex.empty()){
            std::cerr << "Loaded regex : " << rule_regex << std::endl;
        } else {
            std::cerr << "Got empty rule from rule file, exiting !!";
            std::exit(-1);
        }
        
        // setup regex
        std::regex rgx;
        try{
            rgx.assign(rule_regex);
        }catch(const std::regex_error& err){
            std::cerr << "Regex error : " << err.what() << std::endl;
        }
        
        inputHandler loghandler( inputFile );
        
        boost::asio::io_service io_service;
        boost::asio::io_service::work work( io_service );
        
        unsigned long long process_count = 0, inline_count = 0, write_count = 0;
        
        std::mutex process_count_mutex;
        bool feed_complete = false;
        std::promise<void> parallel_search_done;
        
        //out file
        std::ofstream outputJsonFileSteam(ouputJsonFile);
        std::mutex outputWriteMutex;
        std::once_flag write_once_flag;

        unsigned int max_thread_count = std::thread::hardware_concurrency();        
        concurrentQueue<std::string> inputBuffer( max_thread_count * 3 );
        
        outputJsonFileSteam << "{ \"filteredmsg\" : [";
        
        // The line processer closure
        auto process_line = [&](){
            
            std::string line;
            std::smatch matches_line;
            std::atomic_bool done{false};

            // Pop from queue
            do{
                done = inputBuffer.try_pop(line);
            }while( ! done.load() );
            
            //regex search
            if(std::regex_search(line, matches_line, rgx)) {
                
                // if enabled write to stdout
                if(enableWritetoFile){
                    std::cout << matches_line[0].str() << std::endl;
                }
                
                // get capture groups and process if needed
                if( (enableJsonOut) && ( captureList.size() <=  matches_line.size())){
                    
                    std::stringstream outobject;
                    outobject << "{";
                    
                    // Get capture group key from json and associated value from regex smatch
                    for(size_t count = 0 ; count < captureList.size(); count++ ){
                        outobject <<  captureList[std::to_string(count+1)] 
                                  << " : \"" << matches_line[count+1].str() 
                                  << "\"";
                        if( (count + 1) != captureList.size() ){
                            outobject << ",";
                        }
                    }

                    outobject << "}";
                    
                    // Write output to file
                    {
                        bool first_time = false;
                        std::lock_guard<std::mutex> guard(outputWriteMutex);
                        
                        //Todo : Find better way to manage printing of first line
                        std::call_once(write_once_flag, 
                                        [&outputJsonFileSteam, &outobject, &first_time]()
                                        {
                                            outputJsonFileSteam << outobject.str();
                                            first_time = true;
                                        } 
                        );

                        if(!first_time)
                            outputJsonFileSteam << "," << outobject.str();
                        
                        write_count++;
                    }
                }
            }
            
            // Increment processed line count & set promise if done
            {
                std::lock_guard<std::mutex> mlock(process_count_mutex);
                process_count++;
                if( feed_complete && ( process_count == inline_count )){
                    parallel_search_done.set_value();
                }
            }
        }; //  auto process_line

        // Setup thread pool
        std::vector<std::thread> threadpool;
        std::cerr << "Max thread count : " << max_thread_count << std::endl;
        
        auto worker = [&io_service](){ io_service.run(); };
        for( size_t i = 0; i < max_thread_count; i++  ){
            threadpool.emplace_back(worker);
        }
        
        // Get total line count - 
        unsigned int line_count = std::count(
            std::istreambuf_iterator<char>(loghandler.getStream()), 
            std::istreambuf_iterator<char>(), 
            '\n'
        );

        // clear fail and eof bits
        loghandler.getStream().clear();
        //Set input position indicator to beginning
        loghandler.getStream().seekg(0, std::ios::beg); 

        ProgressBar progressBar(line_count/(5000), 70);
        unsigned int progressCounter = 0;

        // Feed input lines to queue
        while( loghandler.getStream() ){
            
            // Get line
            std::string line;
            std::getline(loghandler.getStream(), line);
            bool ret = false;

            // Push the line to queue and post 
            do{
                //Todo : Better queu push with proper wait usinf cv
                ret = inputBuffer.try_push(line);
            } while( false == ret);
            io_service.post( process_line );
            
            // increment count
            inline_count++;
            ++progressCounter;
            
            // Updates progress bar
            if( progressCounter >= 5000 ){
                ++progressBar;
                progressBar.display();
                progressCounter = 0;
            }
        }

        // Set input feed completion
        feed_complete = true;
        
        // Wait for the promise
        parallel_search_done.get_future().wait();

        // Stop service and join the threads
        io_service.stop();
        std::for_each(threadpool.begin(), threadpool.end(), 
                        std::bind(&std::thread::join, std::placeholders::_1 ));

        progressBar.done();
        
        // Write end for JSON and close file - 
        if( enableJsonOut && outputJsonFileSteam.is_open()){
            outputJsonFileSteam << "]}";
            outputJsonFileSteam.close();

            std::cerr << "Added elements : " << write_count << std::endl;
        }
    
        //Estimate total time taken
        auto end_time = std::chrono::system_clock::now();
        auto duration = end_time - start_time;
        std::cerr << "Completed processing " << inline_count <<  " lines in " 
                  << std::chrono::duration_cast<std::chrono::seconds>(duration).count() 
                  << " Seconds" << std::endl;
        
    }catch( const std::exception& exp ){
        std::cerr <<  "Failed : " << exp.what() << std::endl;
        exit(-1);
    }

    return ( 0 );
}
