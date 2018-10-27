#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

#include <podofo/podofo.h>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

using std::vector;
using std::cerr;
using std::cout;
using std::endl;
using std::string;

using namespace PoDoFo;
namespace po = boost::program_options;
namespace fs = boost::filesystem;


const PdfName KEY_JS("JS");
const PdfName KEY_S("S");
const PdfName KEY_LENGTH("Length");
const PdfName NAME_JAVASCRIPT("JavaScript");




struct Config {
public:
  string input_file;
  string output_file;
  vector<int> objid;

  bool parse_args(int argc, char** argv) {
    po::options_description visible("Allowed options");
    visible.add_options()
      ("help,h", "Show this usage information")
      ("input,i",      po::value<string>(&input_file)->required(), "PDF file to use as a template")
      ("output,o",     po::value<string>(&output_file)->required(), "Location of generated PDF")
      ;

    po::options_description hidden("Hidden options");
    hidden.add_options()
      ("objid",      po::value<vector<int> >(&objid)->required(), "Object IDs to remove")
      ;

    po::positional_options_description pos_desc;
    pos_desc.add("objid", -1);

    po::options_description desc;
    desc.add(visible).add(hidden);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv)
              .options(desc)
              .positional(pos_desc)
              .run(),
              vm);

    if (vm.count("help")) {
      cerr << argv[0] << "[options] <object_id> [object_id] ..." << endl;
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

  cerr << "Loading " << conf.input_file << endl;

  try {
    // Load PDF file
    PdfMemDocument doc(conf.input_file.c_str());

    PdfVecObjects& objs = doc.GetObjects();

    PdfVecObjects::iterator o;
    for (o=objs.begin(); o!=objs.end(); ++o) {

      PdfObject* obj = *o;

      vector<int>::iterator banned_id;
      for (banned_id = conf.objid.begin(); banned_id != conf.objid.end(); ++banned_id) {
        if (obj->Reference().ObjectNumber() == (pdf_objnum) *banned_id) {
          cerr << "Removing object: " << obj->Reference().ToString() << endl;
          objs.RemoveObject(obj->Reference());
          o--;
          break;
        }
      }

    }

    doc.Write(conf.output_file.c_str());

  } catch (PdfError& e) {
    cerr << "An error occurred: " << e.what() << endl;
    return 1;
  }

  return 0;
}

