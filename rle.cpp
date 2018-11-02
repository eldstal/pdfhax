/*
 * Generate best-case Run-Length-encoded string that decompresses
 * to a given size (just a repeated set of the character Q.
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

using std::vector;
using std::cerr;
using std::cout;
using std::endl;
using std::string;

namespace po = boost::program_options;


struct Config {
public:
  uint64_t length;

  bool parse_args(int argc, char** argv) {
    po::options_description visible("Allowed options");
    visible.add_options()
      ("help,h", "Show this usage information")
      ;

    po::options_description hidden("Hidden options");
    hidden.add_options()
      ("length",      po::value<uint64_t>(&length)->required(), "Object IDs to remove")
      ;

    po::positional_options_description pos_desc;
    pos_desc.add("length", 1);

    po::options_description desc;
    desc.add(visible).add(hidden);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv)
              .options(desc)
              .positional(pos_desc)
              .run(),
              vm);

    if (vm.count("help")) {
      cerr << argv[0] << "<length>" << endl;
      cerr << visible << endl;
      return false;
    }

    po::notify(vm);

    return true;
  }
};



int main(int argc, char** argv) {
  Config conf;
  if (!conf.parse_args(argc, argv)) {
    return 1;
  }

  while (conf.length > 128) {
    cout << (uint8_t) 129 << 'Q';
    conf.length -= 128;
  }

  cout << (uint8_t) (257 - conf.length) << 'Q';


  return 0;
}

