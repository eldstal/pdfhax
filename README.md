# pdfhax
## Requirements
[podofo](http://podofo.sourceforge.net/), `boost::filesystem` and `boost::program_options`

## pdfjsdump
Invoke on a PDF to extract all identified javascript from it (includes compressed blobs).

## pdfjsinject
Inject one or more javascript files into a PDF and add the metadata needed to get Acrobat Reader to automatically execute it on load.

## pdfectomy
Invoke on a PDF and provide numerical object number(s). The PDF will have those objects removed.
