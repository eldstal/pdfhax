
BOOST=-lboost_program_options -lboost_filesystem -lboost_system 

all: bin/pdfjsdump bin/pdfjsinject bin/pdfectomy

bin/%: %.cpp
	mkdir -p bin
	g++ -lpodofo $(BOOST) -o $@ $<

run: bin/pdfjsdump bin/pdfjsinject
	mkdir -p generated
	./bin/pdfjsinject -i samples/latex.pdf -o generated/latex_inj.pdf js/alert.js
	echo "generated/latex_inj.pdf should run your JS on load now"
	# also generate an uncompressed variant, for debugging
	podofouncompress generated/latex_inj.pdf generated/latex_inj_unc.pdf
	#mkdir -p dumped
	#./bin/pdfjsdump -o dumped/ samples/BouncingButton.pdf
	#ls dumped/

