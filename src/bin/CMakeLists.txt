include_directories(../lib)

add_executable(
	mw-sql-tsv-dump

	sql_tsv_dump.cc
)
target_link_libraries(mw-sql-tsv-dump mwdump)

add_executable(
	mw-xml-dump-info

	xml_dump_info.cc
)
target_link_libraries(mw-xml-dump-info mwdump)
