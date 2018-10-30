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
const PdfName NAME_XREF("XRef");




struct Config {
public:
  string input_file;
  string output_file;
  vector<string> javascript;

  bool parse_args(int argc, char** argv) {
    po::options_description visible("Allowed options");
    visible.add_options()
      ("help,h", "Show this usage information")
      ("input,i",      po::value<string>(&input_file)->required(), "PDF file to use as a template")
      ("output,o",     po::value<string>(&output_file)->required(), "Location of generated PDF")
      ;

    po::options_description hidden("Hidden options");
    hidden.add_options()
      ("javascript",      po::value<vector<string> >(&javascript)->required(), "JS file(s) to inject")
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

void add_objects_to_xref(PdfVecObjects& doc_objs, vector<PdfReference> scripts) {

  // In the xref object, there's a reference to the \Root.
  // \Root should contain a reference to \Names
  // \Names contain two references. \AP and \JavaScript.
  // \JavaScript should contain \Names, an array of [ (string) <ref> (string) <ref> ... ]
  // \AP is some sort of icon data on the form [ (string) null (string null ]

  PdfObject* xref = NULL;
  PdfVecObjects::iterator o;
  for (auto& obj : doc_objs) {
    if (obj->IsDictionary() &&
        obj->GetDictionary().HasKey(KEY_TYPE) &&
        obj->GetDictionary().GetKey(KEY_TYPE)->IsName() &&
        obj->GetDictionary().GetKey(KEY_TYPE)->GetName() == NAME_XREF)  {
      xref = obj;
      break;
    }
  }

  if (xref == NULL) {
    cerr << "Unable to find XRef object. Can't insert JS in metadata." << endl;
    return;
  }


  PdfObject* catalog = get_or_create_dict_obj(doc_objs, xref, KEY_ROOT);
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
    jsnames.push_back(PdfString("dummy_name"));
    jsnames.push_back(script_ref);
  }
}


int main(int argc, char** argv) {
  Config conf;
  if (!conf.parse_args(argc, argv)) {
    return 1;
  }

  cerr << "Loading " << conf.input_file << endl;
  vector<PdfReference> inserted_objects;

  try {
    // Load PDF file
    PdfMemDocument doc(conf.input_file.c_str());

    PdfVecObjects& objs = doc.GetObjects();

    vector<string>::iterator jsfile;
    for (jsfile = conf.javascript.begin(); jsfile != conf.javascript.end(); ++jsfile) {
      cerr << "Injecting " << *jsfile << endl;

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
      PdfObject* container = objs.CreateObject(PdfVariant(container_dict));
      container->GetStream()->Set(javascript.c_str(), javascript.size());

      pdf_objnum obj_num = container->Reference().ObjectNumber();
      pdf_gennum gen_num = container->Reference().GenerationNumber();
      cerr << "JS stream is " << obj_num << "_" << gen_num << endl;


      // The dictionary with the JavaScript type and a reference to the actual code
      PdfDictionary ref_dict;
      ref_dict.AddKey(KEY_S, NAME_JAVASCRIPT);
      ref_dict.AddKey(KEY_JS, container->Reference());
      PdfObject* ref = objs.CreateObject(ref_dict);

      inserted_objects.push_back(ref->Reference());
    }

    add_objects_to_xref(objs, inserted_objects);

    doc.Write(conf.output_file.c_str());

  } catch (PdfError& e) {
    cerr << "An error occurred: " << e.what() << endl;
    return 1;
  }

  return 0;
}

