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

#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
using std::shared_ptr;
using std::make_shared;
using std::string;
using std::vector;
using std::stringstream;
using std::unordered_map;

#include <boost/algorithm/string.hpp>
#include <boost/exception/all.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
using boost::filesystem::exists;
using boost::filesystem::path;
using boost::lexical_cast;

#include <htslib/faidx.h>

namespace portcullis { namespace bam {

typedef boost::error_info<struct BamError,string> BamErrorInfo;
struct BamException: virtual boost::exception, virtual std::exception { };

    
enum Strand {
    POSITIVE,
    NEGATIVE,
    UNKNOWN
};
    
static Strand strandFromBool(bool reverseStrand) {
    return reverseStrand ? NEGATIVE : POSITIVE;
}

static Strand strandFromChar(char strand) {
    switch(strand) {
        case '+':
            return POSITIVE;
        case '-':
            return NEGATIVE;
        case '?':
            return UNKNOWN;
    }

    return UNKNOWN;
}

static char strandToChar(Strand strand) {
    
    switch(strand) {
        case POSITIVE:
            return '+';
        case NEGATIVE:
            return '-';
        case UNKNOWN:
            return '?';
    }

    return '?';
}

static string strandToString(Strand strand) {
    
    switch(strand) {
        case POSITIVE:
            return string("POSITIVE");
        case NEGATIVE:
            return string("NEGATIVE");
        case UNKNOWN:
            return string("UNKNOWN");
    }

    return string("UNKNOWN");
}

enum class Strandedness : std::uint8_t {
    UNSTRANDED = 0,
    FIRSTSTRAND = 1,
    SECONDSTRAND = 2,
    UNKNOWN = 3
};

enum class Orientation : std::uint8_t {
    F = 0,
    R = 1,
    FR = 2,
    RF = 3,
    FF = 4,
    RR = 5,
    UNKNOWN = 6
};

inline const string strandednessToString(Strandedness ss) {
    switch (ss) {
        case Strandedness::UNSTRANDED:   return "UNSTRANDED";
        case Strandedness::FIRSTSTRAND:  return "FIRSTSTRAND";
        case Strandedness::SECONDSTRAND: return "SECONDSTRAND";
        case Strandedness::UNKNOWN: return "UNKNOWN";
        default:      return "[Unknown StrandSpecific type]";
    }
}

inline const Strandedness strandednessFromString(string& ss) {
    
    if (boost::iequals(ss, "UNSTRANDED")) {
        return Strandedness::UNSTRANDED;
    }
    else if (boost::iequals(ss, "FIRSTSTRAND")) {
        return Strandedness::FIRSTSTRAND;
    }
    else if (boost::iequals(ss, "SECONDSTRAND")) {
        return Strandedness::SECONDSTRAND;
    }
    else if (boost::iequals(ss, "UNKNOWN")) {
        return Strandedness::UNKNOWN;
    }
        
    return Strandedness::UNKNOWN;
}

struct RefSeq {
    int32_t index;
    string name;
    uint32_t length;
    
    RefSeq() : RefSeq(-1, "", 0) {};
    
    RefSeq(const int32_t _index, const string& _name, const uint32_t _length) {
        index = _index;
        name = _name;
        length = _length;
    }
    
    string toString() const {
        return name + " (Index: " + lexical_cast<string>(index) + "; Length: " + lexical_cast<string>(length) + ")";
    }
};

typedef shared_ptr<RefSeq> RefSeqPtr;
typedef vector<portcullis::bam::RefSeqPtr> RefSeqPtrList;
typedef unordered_map<int32_t, RefSeqPtr> RefSeqPtrIndexMap;

class BamHelper {
        
public:
    
    static bool isCoordSortedBam(const path& bamFile);
    
    static bool isNewerIndexPresent(const path& bamFile, bool useCsi);
    
    /**
     * Creates a command that can be used to merge multiple BAM files with samtools
     * @param bamFiles The paths to each BAM file to merge
     * @param mergedBamFile The output file
     * @param threads Number of threads to use during merging
     * @return Command line
     */
    static string createMergeBamCmd(const vector<path>& bamFiles, const path& mergedBamFile, uint16_t threads);
    
    
    /**
     * Creates a samtools command that can be used to sort a bam file.  Assumes 
     * entries will be sorted by position using 1 thread and 1GB or RAM.
     * @param unsortedFile The bam file that needs sorting
     * @param sortedFile The path to the new sorted bam file which will be created     
     * @return The command that can be used to sort the bam file
     */
    static string createSortBamCmd(const path& unsortedFile, const path& sortedFile) {
        return createSortBamCmd(unsortedFile, sortedFile, false, 1, string("1G"));
    }
    
    /**
     * Creates a samtools command that can be used to sort a bam file
     * @param unsortedFile The bam file that needs sorting
     * @param sortedFile The path to the new sorted bam file which will be created
     * @param sortByName If true, bam entries are sorted by name, otherwise by position
     * @param threads Number of threads to use
     * @param memory Amount of memory to request
     * @return The command that can be used to sort the bam file
     */
    static string createSortBamCmd( const path& unsortedFile, 
                                    const path& sortedFile, 
                                    bool sortByName, 
                                    uint16_t threads, 
                                    const string& memory);
    /**
     * Creates a samtools command that can be used to index a sorted bam file
     * @param sortedBam Path to a sorted bam file to index
     * @return The command that can be used to index the sorted bam file
     */
    static string createIndexBamCmd(const path& sortedBam, bool useCsi);
    
};
}}
