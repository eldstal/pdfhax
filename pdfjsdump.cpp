#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include <podofo/podofo.h>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

using std::cerr;
using std::cout;
using std::endl;
using std::string;

using namespace PoDoFo;
namespace po = boost::program_options;
namespace fs = boost::filesystem;


const PdfName KEY_JS("JS");
const PdfName KEY_S("S");
const PdfName NAME_JAVASCRIPT("JavaScript");



struct Config {
public:
  string input_file;
  string output_dir = ".";

  bool clobber = false;
  bool print_only = false;


  bool parse_args(int argc, char** argv) {
    po::options_description visible("Allowed options");
    visible.add_options()
      ("help,h", "Show this usage information")
      ("clobber,c",  po::bool_switch(&this->clobber), "Allow overwriting of output files")
      ("stdout,c",  po::bool_switch(&this->print_only), "Print all javascript code directly to stdout. No files will be created.")
      ("output_dir,o", po::value<string>(&this->output_dir), "Directory where dumped js files are placed")
      ;

    po::options_description hidden("Hidden options");
    hidden.add_options()
      ("input",      po::value<string>(&this->input_file)->required(), "PDF file to dump")
      ;

    po::positional_options_description pos_desc;
    pos_desc.add("input", 1);

    po::options_description desc;
    desc.add(visible).add(hidden);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv)
              .options(desc)
              .positional(pos_desc)
              .run(),
              vm);

    if (vm.count("help")) {
      cerr << argv[0] << "[options] <input.pdf>" << endl;
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


  if (conf.clobber) {
    cerr << "Warning: --clobber enabled. Overwriting conflicting files." << endl;
  }

  cerr << "Loading " << conf.input_file << endl;

  try {
    // Load PDF file
    const PdfMemDocument doc(conf.input_file.c_str());

    // Iterate over objects
    PdfVecObjects objs = doc.GetObjects();
    TIVecObjects o;
    for (o=objs.begin(); o!=objs.end(); ++o) {

      // Some objects contain a dictonary where
      // [/S] = /JavaScript
      // [/JS] = code string
      // or
      // [/JS] = ref to a stream with the code

      if (!(*o)->IsDictionary()) {
        continue;
      }

      const PdfDictionary dict = (*o)->GetDictionary();

      // /S /JavaScript
      if (!dict.HasKey(KEY_S)) continue;
      const PdfObject* s = dict.GetKey(KEY_S);
      if (!s->IsName()) continue;
      if (s->GetName() != NAME_JAVASCRIPT) continue;


      // If this is a string, it's plain javascript
      // if it's a reference, it's a reference to a javascript stream.
      if (!dict.HasKey(KEY_JS)) continue;

      const PdfObject* js = dict.GetKey(KEY_JS);

      // Identifying numbers and the raw JS code
      pdf_objnum obj_num;
      pdf_gennum gen_num;
      string javascript;

      if (js->IsString()) {
        cerr << "Found plain JS string." << endl;
        obj_num = (*o)->Reference().ObjectNumber();
        gen_num = (*o)->Reference().GenerationNumber();
        javascript = js->GetString().GetString();

      } else if (js->IsReference()) {
        const PdfReference ref = js->GetReference();
        const PdfObject* container = objs.GetObject(ref);

        if (!container->HasStream()) {
          cerr << "Expected reference to a stream, found reference to " << container->GetDataTypeString() << endl;
          continue;
        }

        const PdfStream* stream = container->GetStream();

        pdf_long buflen;
        char*  buf;

        stream->GetFilteredCopy(&buf, &buflen);

        cerr << "Found reference to JS, length " << buflen << endl;

        obj_num = container->Reference().ObjectNumber();
        gen_num = container->Reference().GenerationNumber();
        javascript = string(buf, buflen);


        podofo_free(buf);

      } else {
        cerr << "Found unknown datatype: " << js->GetDataTypeString() << endl;
        continue;
      }



      // We have the code. Dump it as the user requested.
      if (conf.print_only) {
        cout << javascript;
      } else {
        std::stringstream fn;
        fn << conf.output_dir << "/" << obj_num << "_" << gen_num << ".js";
        string filename = fn.str();

        if (fs::exists(filename)) {
          if (conf.clobber) {
            cerr << "Overwriting " << filename << endl;
          } else {
            cerr << "File exists. Not overwriting " << filename << endl;
            continue;
          }
        }

        std::ofstream f(filename, std::ios_base::out | std::ios_base::trunc);
        f << javascript;
        f.close();

      }

    }


  } catch (PdfError& e) {
    cerr << "An error occurred: " << e.what() << endl;
    return 1;
  }

  return 0;
}

