include_directories(../lib)

add_executable(
	mw-sql-tsv-dump

	sql_tsv_dump.cc
)
target_link_libraries(mw-sql-tsv-dump mwdump)
target_link_libraries(mw-sql-tsv-dump z)

add_executable(
	mw-xml-dump-info

	xml_dump_info.cc
)
target_link_libraries(mw-xml-dump-info mwdump)
target_link_libraries(mw-xml-dump-info z)
target_link_libraries(mw-xml-dump-info expat)
