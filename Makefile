
BOOST=-lboost_program_options -lboost_filesystem -lboost_system 

all: pdfjsdump pdfjsinject pdfectomy

pdfjsdump: pdfjsdump.cpp
	g++ -lpodofo $(BOOST) -o $@ $<

pdfjsinject: pdfjsinject.cpp
	g++ -lpodofo $(BOOST) -o $@ $<

pdfectomy: pdfectomy.cpp
	g++ -lpodofo $(BOOST) -o $@ $<

run: pdfjsdump pdfjsinject
	./pdfjsinject -i empty.pdf -o empty_inj.pdf js/alert.js
	podofouncompress empty_inj.pdf empty_inj_unc.pdf
	#mkdir -p dumped
	#./pdfjsdump -o dumped BouncingButton.pdf
	#ls dumped/

