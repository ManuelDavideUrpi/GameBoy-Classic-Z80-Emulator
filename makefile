CFILE="Main"
all:
	@echo "Making..."
	g++ -o $(CFILE).o $(CFILE).cpp -Wno-format-security
	@echo "Complete!"