//  ********************************************************************
//  This file is part of Portcullis.
//
//  Portcullis is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  Portcullis is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with Portcullis.  If not, see <http://www.gnu.org/licenses/>.
//  *******************************************************************

#pragma once

#include <fstream>
#include <string>
#include <iostream>
#include <vector>
using std::boolalpha;
using std::ifstream;
using std::string;
using std::vector;

#include <boost/exception/all.hpp>
#include <boost/timer/timer.hpp>
#include <boost/filesystem.hpp>

#include "junction_system.hpp"
using boost::timer::auto_cpu_timer;
using boost::lexical_cast;
using boost::filesystem::absolute;
using boost::filesystem::copy_file;
using boost::filesystem::remove;
using boost::filesystem::exists;
using boost::filesystem::create_symlink;
using boost::filesystem::create_directory;
using boost::filesystem::symbolic_link_exists;
namespace po = boost::program_options;

#ifdef PREFIX
    const string FILT_EXE = string(PREFIX) + "/bin/filter_junctions.py";
#else
    const string FILT_EXE("filter_junctions.py");
#endif    

namespace portcullis {
    
typedef boost::error_info<struct JuncFilterError,string> JuncFilterErrorInfo;
struct JuncFilterException: virtual boost::exception, virtual std::exception { };

const string DEFAULT_FILTER_OUTPUT_DIR = "portcullis_filter_out";
const string DEFAULT_FILTER_OUTPUT_PREFIX = "portcullis";
const string DEFAULT_FILTER_FILE = "default_filter.json";



class JunctionFilter {

private:
    
    string junctionFile;
    string filterFile;
    string outputDir;
    string outputPrefix;
    bool verbose;
    
public:
    
    JunctionFilter(const string& _junctionFile, const string& _filterFile, const string& _outputDir, const string& _outputPrefix, bool _verbose) {
        junctionFile = _junctionFile;
        filterFile = _filterFile;
        outputDir = _outputDir;
        outputPrefix = _outputPrefix;
        verbose = _verbose;
        
    }
    
    virtual ~JunctionFilter() {
    }
    
       

public:
    
    string getFilterFile() const {
        return filterFile;
    }

    void setFilterFile(string filterFile) {
        this->filterFile = filterFile;
    }

    string getJunctionFile() const {
        return junctionFile;
    }

    void setJunctionFile(string junctionFile) {
        this->junctionFile = junctionFile;
    }

    string getOutputDir() const {
        return outputDir;
    }

    void setOutputDir(string outputDir) {
        this->outputDir = outputDir;
    }

    string getOutputPrefix() const {
        return outputPrefix;
    }

    void setOutputPrefix(string outputPrefix) {
        this->outputPrefix = outputPrefix;
    }


    
    
    void filter() {
        
        // Test if provided genome exists
        if (!exists(junctionFile)) {
            BOOST_THROW_EXCEPTION(JuncFilterException() << JuncFilterErrorInfo(string(
                        "Could not find junction file at: ") + junctionFile));
        }
        
        // Test if provided filter config file exists
        if (!exists(filterFile)) {
            BOOST_THROW_EXCEPTION(JuncFilterException() << JuncFilterErrorInfo(string(
                        "Could not find filter configuration file at: ") + filterFile));
        }
        
        cout << "Filtering junctions according to provided configuration file ... ";
        cout.flush();
        
        
        if (!exists(outputDir)) {
            create_directory(outputDir);
        }        
        
        string tmpOutputTabFile = outputDir + "/tmp.tab";
        
        string filterCmd = string("python filter_junctions.py ") +
               " --input=" + junctionFile + 
               " --json=" + filterFile +
               " --output=" + tmpOutputTabFile;
        
        if (verbose) {
            cout << "Running filter script: " << filterCmd << endl;
        }
        
        int exitCode = system(filterCmd.c_str());                    
            
        if (exitCode != 0 || !exists(tmpOutputTabFile)) {
                BOOST_THROW_EXCEPTION(JuncFilterException() << JuncFilterErrorInfo(string(
                        "Failed to successfully filter: ") + junctionFile));
        }
        
        cout << "done" << endl << endl;
        
        // Load junction system
        JunctionSystem originalJuncs(junctionFile);
        
        // Load junction system
        JunctionSystem filteredJunc(tmpOutputTabFile);
        
        uint32_t diff = originalJuncs.getJunctions().size() - filteredJunc.getJunctions().size();
        
        cout << "Filtered out " << diff << " junctions.  " << filteredJunc.getJunctions().size() << " junctions remaining" << endl << endl;
        
        filteredJunc.saveAll(outputDir + "/" + outputPrefix);        
    }
  
    static string helpMessage() {
        return string("\nPortcullis Filter Mode Help.\n\n") +
                      "Usage: portcullis filter [options] <junction-file>\n\n" +
                      "Allowed options";
    }
    
    static int main(int argc, char *argv[]) {
        
        // Portcullis args
        string junctionFile;
        string filterFile;
        
        string outputDir;
        string outputPrefix;
        bool verbose;
        bool help;
        
        // Declare the supported options.
        po::options_description generic_options(helpMessage());
        generic_options.add_options()
                ("output_dir,o", po::value<string>(&outputDir)->default_value(DEFAULT_FILTER_OUTPUT_DIR), 
                    "Output directory for files generated by this program.")
                ("output_prefix,p", po::value<string>(&outputPrefix)->default_value(DEFAULT_FILTER_OUTPUT_PREFIX), 
                    "File name prefix for files generated by this program.")
                ("filter_file,f", po::value<string>(&filterFile)->default_value(DEFAULT_FILTER_FILE), 
                    "The filter configuration file to use.")
                ("verbose,v", po::bool_switch(&verbose)->default_value(false), 
                    "Print extra information")
                ("help", po::bool_switch(&help)->default_value(false), "Produce help message")
                ;

        // Hidden options, will be allowed both on command line and
        // in config file, but will not be shown to the user.
        po::options_description hidden_options("Hidden options");
        hidden_options.add_options()
                ("junction-file,g", po::value<string>(&junctionFile), "Path to the junction file to process.")
                ;

        // Positional option for the input bam file
        po::positional_options_description p;
        p.add("junction-file", 1);
        
        
        // Combine non-positional options
        po::options_description cmdline_options;
        cmdline_options.add(generic_options).add(hidden_options);

        // Parse command line
        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).options(cmdline_options).positional(p).run(), vm);
        po::notify(vm);

        // Output help information the exit if requested
        if (help || argc <= 1) {
            cout << generic_options << endl;
            return 1;
        }
        
        

        auto_cpu_timer timer(1, "\nPortcullis junction filter completed.\nTotal runtime: %ws\n\n");        

        cout << "Running portcullis in junction filter mode" << endl
             << "--------------------------------" << endl << endl;
        
        // Create the prepare class
        JunctionFilter filter(junctionFile, filterFile, outputDir, outputPrefix, verbose);
        filter.filter();
        
        return 0;
    }
};
}