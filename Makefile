SAMPLES=$(wildcard samples/*.pdf)
SAMPLES_UNCOMPRESSED=$(patsubst samples/%.pdf,generated/%_unc.pdf,$(SAMPLES))


BOOST=-lboost_program_options -lboost_filesystem -lboost_system 

all: bin/pdfjsdump bin/pdfjsinject bin/pdfectomy $(SAMPLES_UNCOMPRESSED)

bin/%: %.cpp
	mkdir -p bin
	g++ -std=c++11 -o $@ $< -lpodofo $(BOOST)

generated/%_unc.pdf: samples/%.pdf
	podofouncompress $^ $@

run: bin/pdfjsdump bin/pdfjsinject
	mkdir -p generated
	./bin/pdfjsinject -i samples/latex.pdf -o generated/latex_inj.pdf js/alert.js
	echo "generated/latex_inj.pdf should run your JS on load now"
	# also generate an uncompressed variant, for debugging
	podofouncompress generated/latex_inj.pdf generated/latex_inj_unc.pdf
	#mkdir -p dumped
	#./bin/pdfjsdump -o dumped/ samples/BouncingButton.pdf
	#ls dumped/

