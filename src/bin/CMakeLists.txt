include_directories(../lib)

add_executable(
	mw-sql-tsv-dump

	sql_tsv_dump.cc
)
target_link_libraries(mw-sql-tsv-dump mwdump)
target_link_libraries(mw-sql-tsv-dump boost_iostreams)
