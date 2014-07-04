add_library(
	mwdump

	CompressedDumpReader.cc
	SQLDumpParser.cc
	XMLDumpParser.cc
)
target_link_libraries(mwdump z)
target_link_libraries(mwdump bz2)
target_link_libraries(mwdump archive)
target_link_libraries(mwdump expat)
