
 /*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */ 

#pragma once

#include <fstream>
#include <string>
#include <boost/filesystem.hpp>

class inputHandler{

public:

    explicit inputHandler(std::string& input)
    {
	    if(input.empty())
	        throw std::runtime_error("Empty Path");

	    if(!boost::filesystem::exists( input ))
	        throw std::runtime_error("Invalid input file");
	        
	    mInputFileStream.open(input, std::ios_base::in);
	    if(!mInputFileStream.is_open())
	        throw std::runtime_error("Failed to open file " );
	}

    std::ifstream& getStream()
    {
    	return mInputFileStream;
	}

    ~inputHandler()
    {
    	if( mInputFileStream && mInputFileStream.is_open())
        	mInputFileStream.close();
	}

private:
    std::ifstream mInputFileStream;
};
