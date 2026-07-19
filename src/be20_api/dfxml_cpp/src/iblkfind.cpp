/**
 * @file    fiwalker.c++
 * @brief   demonstrates mapping offset back to filename from fiwalk dfxml
 * @version 1.0
 * @author  Joel Young <jdyoung@nps.edu>
 *
 * License: Public Domain, Work of US Navy
 *
 * Compile with:
 *   g++ -std=c++11 -I. -c -o dfxml_reader.o dfxml_reader.cpp
 *   g++ -std=c++11 -I. -o fiwalker fiwalker.c++ dfxml_reader.o -lexpat
 *
 * Requires recent g++ as it uses set::emplace.  Tested on g++ 4.8.2
 *
 * Invocation:
 *   ./fiwalker somefiwalk.dfxml
 *
 * Todo:
 *   1. Move to .h
 *   2. Capture additional info to identify file
 *       partition, inode, used, alloc, name_type, etc?
 *   3. Separate file info into separate structure
 *   4. Figure out why dfxml_reader knows about sector_size for
 *      byte_runs but not fs_offset
 */

#include "dfxml_config.h"

#include <iostream>
#include <string>
#include <cstdlib>
#include <map>
#include <set>
#include <limits>

#include "dfxml_reader.h"


/** @brief items of interest from dfxml byte_run data type
 *
 * Stores data from dfxml byte_run section.  Provides default
 * ordering based on offset in image and run length.
 */
class byte_run_t {
  public: ///@{ @name Data Items
    uint64_t file_offset;
    uint64_t img_offset;
    uint64_t len;

    std::string filename;
    ///@}

  public: ///@t @name Constructors
    /** @brief Construct the byte_run
     *
     * @param _file_offset
     * @param _img_offset
     * @param _len
     * @param _filename
     */
    byte_run_t(uint64_t _file_offset,
               uint64_t _img_offset,  uint64_t _len,
               const std::string& _filename) :
      file_offset(_file_offset)
     ,img_offset(_img_offset)
     ,len(_len)
     ,filename(_filename) {
    }
    ///@}

  public: ///@{ Methods
    /** @brief Order byte_runs by offset in image and then by length
     *
     * @param rhs
     */
    inline bool operator<(const byte_run_t& rhs) const {
      return (img_offset < rhs.img_offset) or
             (img_offset == rhs.img_offset and len < rhs.len);
    }

    /** @brief Make byte_run streamable
     *
     * @param[in,out] out  Stream to write to
     * @param[in]     item Item to stream out
     * @return The stream passed in
     */
    friend std::ostream& operator<<(std::ostream& out, const byte_run_t& item) {
      out << "[" << item.file_offset
          << " " << item.img_offset  << " " << item.len
          << " " << item.filename << "]";
      return out;
    }
    ///@}
};

/** @brief Provides mapping of offsets to byte_run records
 *
 * Reads byte_run entries from a fiwalk dfxml file and allows
 * user to query for byte_runs that overlap a desired offset.
 */
class extents_t {
  private:
    std::string dfxml_filename;

  public:
    //using extents_set_t = std::set<byte_run_t>;
    typedef std::set<byte_run_t> extents_set_t;

    extents_set_t extents;

  public: ///@{ @name Constructors

    /** @brief Load the byte_runs from a provided dfxml file
     *
     * @param[in]  dfxml file name
     */
    extents_t(const std::string& p_dfxml_filename) :
      dfxml_filename(p_dfxml_filename),extents() {

        dfxml::file_object_reader::read_dfxml(
            dfxml_filename,
            [&] (dfxml::file_object& fi) { // lambda function to process byte_runs into the extents set
                for(const auto& item : fi.byte_runs) {
                    extents.emplace(item.file_offset,item.img_offset, item.len, fi.filename());
                }
            });
    }
    ///@}

  public: ///@{ @name Member Functions

    /** @brief find the byte_run (if any) containing the provided offset
     *
     * Returns a reference to the byte_run or throws a std::range_error
     * if no byte_run found.
     *
     * @param[in] offset
     */
    const byte_run_t& find(uint64_t offset) {

      auto it = extents.upper_bound(byte_run_t(0,offset,std::numeric_limits<uint64_t>::max(),""));

      if (it == extents.begin()) {
        throw std::range_error("Item not in dataset");;
      }

      --it;

      if (it->img_offset + it->len > offset) {
        return *it;
      } else {
        throw std::range_error("Item not in dataset");;
      }
    }
    ///@}
};


int main(int argc, char** argv) {

  // load the extents from the dfxml
  extents_t extents(argv[1]);

  // A test list of offsets to look up
  std::vector<uint64_t> offsets = {
        2098176, 2621440, 3146752, 3163136, 3195904,
        3212288, 3687424, 4195328, 8908800, 8913920,
        9044992, 9208832, 9305088, 9306112, 9307136,
        9388032, 9389056, 9418752, 9418751, 0
  };

  // Try to look up each of the offsets:
  for (uint64_t offset : offsets) {
    try {
      std::cerr << extents.find(offset) << " (" << offset << ")" << std::endl;
    } catch (const std::runtime_error& e) {
      std::cerr << "Not in file: " << offset << " : " << e.what() << "\n";
    }
  }

  return 0;
}
