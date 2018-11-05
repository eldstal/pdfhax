/*
 * Generate a PDF with a PDFun game engine and any user javascript
 */

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


const PdfName KEY_TYPE("Type");
const PdfName KEY_ROOT("Root");
const PdfName KEY_NAMES("Names");
const PdfName KEY_AP("AP");
const PdfName KEY_JS("JS");
const PdfName KEY_S("S");
const PdfName KEY_LENGTH("Length");
const PdfName NAME_JAVASCRIPT("JavaScript");
const PdfName NAME_CATALOG("Catalog");



struct recursive_directory_range
{

  typedef fs::recursive_directory_iterator iterator;
  recursive_directory_range(fs::path p) : p_(p) {}

  iterator begin() { return fs::recursive_directory_iterator(p_); }
  iterator end() { return fs::recursive_directory_iterator(); }

  fs::path p_;
};

struct Script {
  string name;
  PdfReference stream;
};

enum Variety {
  Vector = 1,
  TwoDee = 2,
  ThreeDee = 3
};


struct Config {
public:
  bool disable_engine;
  string output_file;
  Variety variety = Vector;
  vector<string> javascript;

  bool parse_args(int argc, char** argv) {
    po::options_description visible("Allowed options");
    visible.add_options()
      ("help,h", "Show this usage information")
      ("no-engine,n",      po::bool_switch(&disable_engine)->required(), "Don't auto-include js/pdfun_vec/*.js")
      ("output,o",     po::value<string>(&output_file)->default_value("pdfun.pdf"), "Location of generated PDF")
      ;

    po::options_description hidden("Hidden options");
    hidden.add_options()
      ("javascript",      po::value<vector<string> >(&javascript)->required(), "Acrobat JS file(s) to include")
      ;

    po::positional_options_description pos_desc;
    pos_desc.add("javascript", -1);

    po::options_description desc;
    desc.add(visible).add(hidden);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv)
              .options(desc)
              .positional(pos_desc)
              .run(),
              vm);

    if (vm.count("help")) {
      cerr << argv[0] << "[options] <javascript.js> [javascript.js] ..." << endl;
      cerr << visible << endl;
      return false;
    }

    po::notify(vm);

    return true;
  }
};

PdfObject* get_or_create_dict_obj(PdfVecObjects& doc_objs, PdfObject* parent, PdfName key) {

  PdfObject* ret = NULL;
  if (!parent->GetDictionary().HasKey(key)) {
    PdfDictionary dict;
    ret = doc_objs.CreateObject(dict);
    parent->GetDictionary().AddKey(key, ret->Reference());
  } else {
    ret = doc_objs.GetObject(parent->GetDictionary().GetKey(key)->GetReference());
  }

  return ret;

}

void find_engine_js(Variety variety, vector<string>& filenames) {
  // List all js files under js/pdfun_{variety}/
  string path = "js/pdfun_";
  switch (variety) {
    case Vector:
      path = path + "vec";
      break;
    default:
      return;
  }

  for (auto it : recursive_directory_range(path))
  {
    if (fs::extension(it) == std::string(".js")) {
      filenames.push_back(it.path().string());
    }
  }
}

void add_javascript_to_metadata(PdfDocument* doc, vector<Script> scripts) {

  PdfVecObjects& doc_objs = *(doc->GetObjects());

  // The Catalog object sould contain a reference to \Names
  // \Names contain two references. \AP and \JavaScript.
  // \JavaScript should contain \Names, an array of [ (string) <ref> (string) <ref> ... ]
  // \AP is some sort of icon data on the form [ (string) null (string null ]

  PdfObject* catalog = NULL;
  PdfVecObjects::iterator o;
  for (auto& obj : doc_objs) {
    if (obj->IsDictionary() &&
        obj->GetDictionary().HasKey(KEY_TYPE) &&
        obj->GetDictionary().GetKey(KEY_TYPE)->IsName() &&
        obj->GetDictionary().GetKey(KEY_TYPE)->GetName() == NAME_CATALOG)  {
      catalog = obj;
      break;
    }
  }

  if (catalog == NULL) {
    cerr << "Unable to find Catalog object. Can't insert JS in metadata." << endl;
    return;
  }

  PdfObject* names = get_or_create_dict_obj(doc_objs, catalog, KEY_NAMES);
  PdfObject* javascript = get_or_create_dict_obj(doc_objs, names, NAME_JAVASCRIPT);
  //PdfObject* ap = get_or_create_dict_obj(doc_objs, names, KEY_AP);


  if (!javascript->GetDictionary().HasKey(KEY_NAMES)) {
    PdfArray names_array;
    javascript->GetDictionary().AddKey(KEY_NAMES, names_array);
  }
  PdfArray& jsnames = javascript->GetDictionary().GetKey(KEY_NAMES)->GetArray();


  // Add each of the scripts to this names array
  for (auto& script_ref : scripts) {
    jsnames.push_back(PdfString(script_ref.name));
    jsnames.push_back(script_ref.stream);
  }
}


int main(int argc, char** argv) {
  Config conf;
  if (!conf.parse_args(argc, argv)) {
    return 1;
  }

  if (!conf.disable_engine) {
    vector<string> engine_files;
    find_engine_js(conf.variety, engine_files);
    if (engine_files.size() == 0) {
      cerr << "WARNING: Unable to locate PDFun engine files." << endl;
    }

    conf.javascript.insert(conf.javascript.begin(), engine_files.begin(), engine_files.end());
  }


  vector<Script> inserted_objects;

  try {
    // Our document, which has a single page
    PdfStreamedDocument doc(conf.output_file.c_str(), ePdfVersion_1_7);
    PdfPainter painter;

    PdfPage* page = doc.CreatePage(PdfRect(0, 0, 1280, 720));
    painter.SetPage(page);
    painter.FinishPage();

    PdfVecObjects* objs = doc.GetObjects();

    if (true) {
      vector<string>::iterator jsfile;
      for (jsfile = conf.javascript.begin(); jsfile != conf.javascript.end(); ++jsfile) {
        cerr << "Adding script: " << *jsfile << endl;

        // Read in the JS code
        std::ifstream js(*jsfile);
        std::stringstream jsbuffer;
        jsbuffer << js.rdbuf();
        string javascript = jsbuffer.str();



        // The injected javascript is split into two objects.
        // * An anonymous stream containing the data
        // * A dictionary with [\S] = \JavaScript and [\JS] = {ref to the stream}


        // The stream with the actual javascript data in it.
        // This is uncompressed, for now
        PdfDictionary container_dict;
        container_dict.AddKey(KEY_LENGTH, PdfObject((pdf_int64) javascript.size()));
        PdfObject* container = objs->CreateObject(PdfVariant(container_dict));
        container->GetStream()->Set(javascript.c_str(), javascript.size());

        pdf_objnum obj_num = container->Reference().ObjectNumber();
        pdf_gennum gen_num = container->Reference().GenerationNumber();
        cerr << "JS stream is " << obj_num << "_" << gen_num << endl;


        // The dictionary with the JavaScript type and a reference to the actual code
        PdfDictionary ref_dict;
        ref_dict.AddKey(KEY_S, NAME_JAVASCRIPT);
        ref_dict.AddKey(KEY_JS, container->Reference());
        PdfObject* ref = objs->CreateObject(ref_dict);


        Script inserted;
        inserted.name = fs::path(*jsfile).filename().string();
        inserted.stream = ref->Reference();
        inserted_objects.push_back(inserted);
      }
    }

    add_javascript_to_metadata(&doc, inserted_objects);

    doc.Close();

  } catch (PdfError& e) {
    cerr << "An error occurred: " << e.what() << endl;
    return 1;
  }

  return 0;
}

