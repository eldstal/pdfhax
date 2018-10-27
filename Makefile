pdfjsdump: pdfjsdump.cpp
	g++ -lpodofo -lboost_program_options -lboost_filesystem -lboost_system -o $@ $<

run: pdfjsdump
	mkdir -p js
	./$^ -o js BouncingButton.pdf
	ls js/

